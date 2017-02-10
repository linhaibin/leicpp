#pragma once

#include <memory>
#include <string>

#include "event_handler.h"
#include "socket.h"
#include "buffer.h"
#include "event_loop.h"

namespace lei {

  class TcpSession : public EventHandler {
    public:
      TcpSession(
          const std::shared_ptr<EventLoop> & event_loop,
          const int & fd,
          const std::string & local_addr,
          const std::string & peer_addr,
          const uint32_t & read_buffer_size,
          const uint32_t & write_buffer_size);
      virtual ~TcpSession();

      virtual bool HandleRead();
      virtual bool HandleWrite();
      virtual int GetFd() { return socket_->GetFd(); }
      virtual std::shared_ptr<EventHandler> GetSharedPtr() = 0;
      //获取一个数据包的总长度，>0数据包总长度；=0数据包不完整，
      //还得等待数据；<0出错。
      virtual int GetPacketLength(const char * data, const uint32_t & len) = 0;
      //根据接收的数据解析并处理数据。
      //如果发生致命错误，需要返回false。这种错误需重置连接。
      virtual bool ProcessData(const char * data, const uint32_t & len) = 0;
      int WriteInLoop(const char * data, const uint32_t & len);

      bool Send(const char * data, const uint32_t & len);

      virtual void ResetTimingWheel(const uint32_t & now);
      virtual void RemoveTimingWheel();
      virtual void RegisterTimingWheel(const uint32_t & timeout);
      inline void Stop() {
        event_loop_->ProcessEvent(
            GetSharedPtr(),
            EVENT_ACTION_REMOVE,
            EVENT_TYPE_READ | EVENT_TYPE_WRITE);
      }

      inline const std::string & local_addr() { return socket_->local_addr(); }
      inline const std::string & peer_addr() { return socket_->peer_addr(); }

    private:
      std::shared_ptr<EventLoop> event_loop_;
      std::unique_ptr<Socket> socket_;
      std::unique_ptr<Buffer> read_buffer_;
      std::unique_ptr<Buffer> write_buffer_;

      int timing_wheel_index_;
      uint32_t last_read_time_;
  };

}
