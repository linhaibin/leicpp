#include "tcp_session.h"

#include <string>
#include <iostream>

#include "tcp_server.h"
#include "socket.h"
#include "buffer.h"
#include "event_loop.h"
#include "log.h"
#include "common.h"

namespace lei {

  TcpSession::TcpSession(
      const std::shared_ptr<EventLoop> & event_loop,
      const int & fd,
      const std::string & local_addr,
      const std::string & peer_addr,
      const uint32_t & read_buffer_size,
      const uint32_t & write_buffer_size)
    : event_loop_(event_loop),
    socket_(new Socket(fd, local_addr, peer_addr)),
    read_buffer_(new Buffer(read_buffer_size)),
    write_buffer_(new Buffer(write_buffer_size)) {
      socket_->SetNonblock();
      socket_->SetNoDelay();

      timing_wheel_index_ = -1;
      last_read_time_ = 0;
  }

  TcpSession::~TcpSession() {
    socket_ = nullptr;
  }

  bool TcpSession::HandleRead() {
    int nread = -1;
    do {
      nread = socket_->Recv(
          read_buffer_->StartWriteIndex(), read_buffer_->Space());
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "[TcpSession]id:%u HandleRead nread:%d", id(), nread);
      if (nread > 0) {
        read_buffer_->WriteDone(nread);
        read_buffer_->Dump();
        break;
      } else if (nread < 0) {
        if (EAGAIN == errno || EWOULDBLOCK == errno) {
          //The socket is marked nonblocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.
          //下一次再尝试读。
          return true;
        } else if (EINTR != errno) {
          LOG4CPLUS_ERROR_FMT(lei::zLog, "Read error ! errno:%d, (%s,%s)", errno, socket_->local_addr().c_str(), socket_->peer_addr().c_str());
          return false;
        }
      } else if (0 == nread) {
        LOG4CPLUS_ERROR_FMT(lei::zLog, "Read finished ! errno:%d, (%s,%s), buffer_space:%u", errno, socket_->local_addr().c_str(), socket_->peer_addr().c_str(), read_buffer_->Space());
        return false;
      }
    } while(nread < 0 && errno == EINTR);

    if (read_buffer_->Space() == 0) {
      read_buffer_->Grow();
      if (read_buffer_->Size() > event_loop_->MAX_READ_BUFFSIZE) {
        event_loop_->MAX_READ_BUFFSIZE = read_buffer_->Size();
      }
    }

    do {
      int packet_len = GetPacketLength(
          read_buffer_->StartReadIndex(), read_buffer_->Len());
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "HandleRead packet_len:%d", packet_len);
      if (packet_len > 0) {
        if (read_buffer_->Len() < packet_len) {
          //数据不完整，还需等待数据。
          return true;
        }
        if (ProcessData(read_buffer_->StartReadIndex(), packet_len)) {
          read_buffer_->ReadDone(packet_len);

          TcpServer::R_COUNT++;
          event_loop_->R_COUNT++;
          if (TcpServer::R_COUNT % 100 == 0) {
            LOG4CPLUS_INFO_FMT(lei::zLog, "R_COUNT:%llu", (uint64_t)TcpServer::R_COUNT);
            LOG4CPLUS_INFO_FMT(lei::zLog, "W_COUNT:%llu", (uint64_t)TcpServer::W_COUNT);
          }
          //break;
        } else {
          LOG4CPLUS_ERROR_FMT(lei::zLog, "ProcessData failed, (%s,%s)", socket_->local_addr().c_str(), socket_->peer_addr().c_str());
          return false;
        }
      } else if (0 == packet_len) {
        //数据不完成，还需等待数据。
        //return true;
        break;
      } else {
        //包长度异常。
        LOG4CPLUS_ERROR_FMT(lei::zLog, "GetPacketLength error, len:%d, (%s,%s)", packet_len, socket_->local_addr().c_str(), socket_->peer_addr().c_str());
        return false;
      }
    } while(true);

    read_buffer_->Adjust();

    last_read_time_ = event_loop_->now_sec();
    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "tcp_session.id:%u, last_read_time:%u", id(), last_read_time_);

    return true;  
  }

  bool TcpSession::HandleWrite() {
    int wn = 0;
    do {
      //一直写，直到buf全写完。如果一次性写不完，
      //是不是表示网络拥堵、缓冲满了，而应该下次再写？
      //可以测试一下，两种状态的效率。zebra采用的是后者。
      wn = socket_->Send(
          write_buffer_->StartReadIndex(), write_buffer_->Len());
      if (wn > 0) {
        //LOG4CPLUS_DEBUG_FMT(lei::zLog, "wrote wn:%u", wn);
        write_buffer_->ReadDone(wn);
        break;
        //return true;
      } else if (0 == wn) {
        LOG4CPLUS_ERROR_FMT(lei::zLog, "Write finished ! (%s,%s)", socket_->local_addr().c_str(), socket_->peer_addr().c_str());
        return false;
      } else if (wn < 0) {
        if (EINTR != errno) {
          LOG4CPLUS_ERROR_FMT(lei::zLog, "Write finished ! errno:%d, (%s,%s)", errno, socket_->local_addr().c_str(), socket_->peer_addr().c_str());
          return false;
        } else if (EAGAIN == errno || EWOULDBLOCK == errno) {
          //The socket is marked nonblocking and the requested operation would block.
          //下一次再写
          return true;
        }
      }
    } while(wn < 0 && EINTR == errno);

    if (write_buffer_->Len() == 0) {
      //没有数据可写了。取消fd监听。
      event_loop_->ProcessEvent(GetSharedPtr(), EVENT_ACTION_REMOVE, EVENT_TYPE_WRITE);
    }

    write_buffer_->Dump();
    write_buffer_->Adjust();

    return true;  
  }

  /*int TcpSession::GetPacketLength(const char * data, const uint32_t & len) {
    if (len < 4) {
      return 0;
    }
    return DecodeUint32(data);
  }*/

  /*bool TcpSession::ProcessData(const char * data, const uint32_t & len) {
    return Send(data, len);
  }*/

  int TcpSession::WriteInLoop(const char * data, const uint32_t & len) {
    write_buffer_->Write(data, len);
    if (write_buffer_->Size() > event_loop_->MAX_WRITE_BUFFSIZE) {
      event_loop_->MAX_WRITE_BUFFSIZE = write_buffer_->Size();
    }
    TcpServer::W_COUNT++;
    event_loop_->W_COUNT++;
    if (TcpServer::W_COUNT % 100 == 0) {
      LOG4CPLUS_INFO_FMT(lei::zLog, "SERVER R_COUNT:%llu", (uint64_t)TcpServer::R_COUNT);
      LOG4CPLUS_INFO_FMT(lei::zLog, "SERVER W_COUNT:%llu", (uint64_t)TcpServer::W_COUNT);
    }
    return 0;
  }

  bool TcpSession::Send(const char * data, const uint32_t & len) {
    event_loop_->ProcessEvent(
        GetSharedPtr(), EVENT_ACTION_WRITE, EVENT_TYPE_DEFAULT, data, len);
    return true;
  }

  void TcpSession::ResetTimingWheel(const uint32_t & now) {
    TimingWheel & tw = event_loop_->timing_wheel();
    uint32_t size = tw.size();
    if (last_read_time_ + size <= now) {
      //读超时
      LOG4CPLUS_ERROR_FMT(lei::zLog, "Read timeout ! id:%u, now:%u, last_read_time:%u", id(), now, last_read_time_);
      Stop();
    } else {
      RemoveTimingWheel();
      RegisterTimingWheel(size - (now - last_read_time_));
    }
  }

  void TcpSession::RemoveTimingWheel() {
    if (timing_wheel_index_ >= 0) {
      TimingWheel & tw = event_loop_->timing_wheel();
      tw.Remove(timing_wheel_index_, id());
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "timing wheel removed! id:%u", id());
      timing_wheel_index_ = -1;
    }
  }

  void TcpSession::RegisterTimingWheel(const uint32_t & timeout) {
    TimingWheel & tw = event_loop_->timing_wheel();
    timing_wheel_index_ = tw.Register(id(), timeout);
    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "timing wheel registered! id:%u, timeout:%u, index:%u", id(), timeout, timing_wheel_index_);
  }

}
