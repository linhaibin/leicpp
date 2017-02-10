#include "event_loop.h"

#include <iostream>
#include <mutex>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <assert.h>

#include "tcp_server.h"
#include "common.h"
#include "event_handler.h"
#include "inform_handler.h"
#include "log.h"

namespace lei {

  EventLoop::EventLoop(const std::string & name, TcpServer * tcp_server, bool test)
    : thread_id_(std::this_thread::get_id()),
      name_(name),
      tcp_server_(tcp_server),
      epfd_(::epoll_create1(EPOLL_CLOEXEC)),
      test_(test),
      ep_events_(InitEventSize),
      event_queue_(InitQueueSize),
      inform_handler_(new InformHandler(this)),
      timer_handler_(new TimerHandler(this)),
      timing_wheel_(this, InitTimeout) {
        if (epfd_ < 0) {
          abort();
        }
        ProcessEvent(
            inform_handler_, EVENT_ACTION_REGISTER, EVENT_TYPE_READ);
        ProcessEvent(
            timer_handler_, EVENT_ACTION_REGISTER, EVENT_TYPE_READ);

        W_COUNT = 0;
        R_COUNT = 0;
        MAX_READ_BUFFSIZE = 0;
        MAX_WRITE_BUFFSIZE = 0;
        last_count_time_ = 0;
  }

  EventLoop::~EventLoop() {
    ::close(epfd_);
    Event * e = nullptr;
    //while (!event_queue_.empty()) {
    //  e = event_queue_.front();
    //  e->~Event();
    //  free((void *)e);
    //  event_queue_.pop();
    //}
    while (event_queue_.pop(e)) {
      e->~Event();
      free((void *)e);
      e = nullptr;
    }
    inform_handler_ = nullptr;
    timer_handler_ = nullptr;
    handlers_.clear();
  }

  const bool & EventLoop::Loop() {
    thread_id_ = std::this_thread::get_id();
    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "loop:%s thread_id:%llu", name_.c_str(), thread_id_);
    DispatchAndProcess();
    return tcp_server_->IsNotClosed();
  }

  void EventLoop::DispatchAndProcess() {
    /*
    for (int i = 0; !tcp_server_->IsClosed(); ++i) {
      sleep(3);
      tcp_server_->mutex_.lock();
      //std::unique_lock<std::mutex> lck(tcp_server_->mutex_, std::defer_lock);
      //lck.lock();
      //std::lock(lck);
      std::cout << name_ << " is run " << i << std::endl; 
      //lck.unlock();
      //std::unlock(lck);
      tcp_server_->mutex_.unlock();
    }
    return;
    */

    struct timeval tm; 
    gettimeofday(&tm, NULL);
    now_sec_ =  tm.tv_sec;
    //LOG4CPLUS_INFO_FMT(lei::zLog, "event_loop:%s, now_sec:%u", name_.c_str(), now_sec_);
    //for (;;) {
      //500毫秒后超时，如果这里设置成阻塞（-1）效率如何？
      int nfds = epoll_wait(epfd_, &*ep_events_.begin(), ep_events_.size(), 500);

      if (nfds > 0) {
        for (int i = 0; i < nfds; ++i) {
          //EventHandler * eh = (EventHandler *)ep_events_[i].data.ptr;
          const struct epoll_event & ee = ep_events_[i];
          auto iter = handlers_.find(ee.data.u32);
          if (handlers_.end() == iter) {
            LOG4CPLUS_ERROR_FMT(lei::zLog, "event_loop:%s not find handler, u32:%u, events:%u", name_.c_str(), ee.data.u32, ee.events);
            continue;
          }
          std::shared_ptr<EventHandler> eh = iter->second.lock();
          if (!eh) {
            LOG4CPLUS_ERROR_FMT(lei::zLog, "event_loop:%s lock failed, u32:%u, events:%u", name_.c_str(), ee.data.u32, ee.events);
            continue;
          }
          if (ee.events & EPOLLIN) {
            //Handling reading.
            //LOG4CPLUS_DEBUG(lei::zLog, "Handling reading");
            if (!eh->HandleRead()) {
              LOG4CPLUS_ERROR(lei::zLog, "Handle read error !");
              RemoveHandler(eh, EVENT_TYPE_READ | EVENT_TYPE_WRITE);
            }
          }
          if (ee.events & EPOLLOUT) {
            //Handling writing.
            //LOG4CPLUS_DEBUG(lei::zLog, "Handling writing");
            if (!eh->HandleWrite()) {
              LOG4CPLUS_ERROR(lei::zLog, "Handle write error !");
              RemoveHandler(eh, EVENT_TYPE_READ | EVENT_TYPE_WRITE);
            }
          }
        }
      } else {
        if (EINTR == errno) {
          LOG4CPLUS_ERROR(lei::zLog, "[ERROR]epoll_wait eintr !");
        } else if (EBADF == errno) {
          LOG4CPLUS_ERROR(lei::zLog, "[ERROR]epoll_wait invalid fd !");
          return ;
        } else if (EFAULT == errno) {
          LOG4CPLUS_ERROR(lei::zLog, "[ERROR]epoll_wait The memory area pointed to by events is not accessible with write permissions.");
        } else if (EINVAL == errno) {
          LOG4CPLUS_ERROR(lei::zLog, "[ERROR]epoll_wait epfd is not an epoll file descriptor, or maxevents is less than or equal to zero.");
          return ;
        } else {
          //std::cout << "[ERROR]epoll_wait unknow error:" << errno << " !" << std::endl;
          return ;
        }
      }
    //}
    //if (now_sec_ - last_count_time_ > 10) {
    //  last_count_time_ = now_sec_;
    //  LOG4CPLUS_INFO_FMT(lei::zLog, "loop:%s, W_COUNT:%llu", name_.c_str(), (uint64_t)W_COUNT);
    //  LOG4CPLUS_INFO_FMT(lei::zLog, "loop:%s, R_COUNT:%llu", name_.c_str(), (uint64_t)R_COUNT);
    //  LOG4CPLUS_INFO_FMT(lei::zLog, "loop:%s, MAX_READ_BUFFSIZE:%u", name_.c_str(), (uint32_t)MAX_READ_BUFFSIZE);
    //  LOG4CPLUS_INFO_FMT(lei::zLog, "loop:%s, MAX_WRITE_BUFFSIZE:%u", name_.c_str(), (uint32_t)MAX_WRITE_BUFFSIZE);
    //}
  }

  void EventLoop::RegisterHandler(
      const std::shared_ptr<EventHandler> & eh,
      const EVENT_TYPE & et) {
    EVENT_TYPE status = eh->GetEventStatus();
    int op = 0;
    if (EVENT_TYPE_DEFAULT == status) {
      handlers_[eh->id()] = eh;
      op = EPOLL_CTL_ADD;
      eh->RegisterTimingWheel(timing_wheel_.size());
    } else {
      op = EPOLL_CTL_MOD;
    }

    eh->AddEventStatus(et);
    status = eh->GetEventStatus();

    struct epoll_event eev;
    eev.events = 0;
    eev.data.u32 = eh->id();
    if (status & EVENT_TYPE_READ) {
      eev.events |= EPOLLIN;
    }
    if (status & EVENT_TYPE_WRITE) {
      eev.events |= EPOLLOUT;
    }

    int ret = ::epoll_ctl(epfd_, op, eh->GetFd(), &eev);
    if (ret < 0) {
      LOG4CPLUS_ERROR(lei::zLog, "RegisterHandler epoll_ctl error !");
    }

    //perror("ADD");
    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "fd:%d registered, et:%d, status:%d, ret:%d, op:%s", eh->GetFd(), et, status, ret, (EPOLL_CTL_ADD == op ? "EPOLL_CTL_ADD" : "EPOLL_CTL_MOD"));
  }

  void EventLoop::RemoveHandler(
      const std::shared_ptr<EventHandler> & eh,
      const EVENT_TYPE & et) {
    eh->ClearEventStatus(et);
    EVENT_TYPE status = eh->GetEventStatus();

    struct epoll_event eev;
    eev.events = 0;
    eev.data.u32 = eh->id();
    if (status & EVENT_TYPE_READ) {
      eev.events |= EPOLLIN;
    }
    if (status & EVENT_TYPE_WRITE) {
      eev.events |= EPOLLOUT;
    }

    int op = 0;
    if (EVENT_TYPE_DEFAULT == status) {
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "[EventLoop] erase id:%u", eh->id());
      handlers_.erase(eh->id());
      eh->RemoveTimingWheel();
      eh->Clear();
      op = EPOLL_CTL_DEL;
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "[EventLoop] erase id:%u done !", eh->id());
    } else {
      op = EPOLL_CTL_MOD;
    }

    int ret = ::epoll_ctl(epfd_, op, eh->GetFd(), &eev);
    //perror("REMOVE");
    if (ret < 0) {
      LOG4CPLUS_ERROR(lei::zLog, "RemoveHandler epoll_ctl error !");
    }
    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "fd:%d removed, et:%d, status:%d, ret:%d, op:%s", eh->GetFd(), et, status, ret, (EPOLL_CTL_DEL == op ? "EPOLL_CTL_DEL" : "EPOLL_CTL_MOD"));
  }

  void EventLoop::ProcessEvent(
      const std::shared_ptr<EventHandler> & eh,
      const EVENT_ACTION & action,
      const EVENT_TYPE & event_type,
      const char * data,
      const uint32_t & len)
  {
    //std::cout << "cid:" << std::this_thread::get_id() << ", tid:" << thread_id_ << std::endl;
    if (std::this_thread::get_id() != thread_id_) {
      //异步调用，异步事件，则加入到事件队列中。
      uint32_t buf_len = len > 0 ? (sizeof(uint32_t) + len) : 0;
      buf_len += sizeof(Event);
      char * buf = (char *)malloc(buf_len);
      //对Event的空间置0，防止Event的构造中有未初始化的成员，
      //进而导致数据错乱。
      ::memset(buf, 0, sizeof(Event));
      //placement new
      Event * e = new(buf) Event(eh, action, event_type);
      if (len > 0) {
        *(uint32_t *)e->Data_ = len;
        ::memcpy(e->Data_ + sizeof(uint32_t), data, len);
        //如果len为0，则说明data无用，此时对应的action和event_type
        //也不会用到data，也不需要保存len了。
        //但是，如果没有设置data(null)和len(=0)，但是action和event_type
        //却指明要使用data，比如发送数据什么的，岂不是会崩溃？
      }
      //std::cout << "push event:" << e << std::endl;

      //这里有个问题。这里放入队列时，eh是有效的。
      //如果在处理该事件之前，eh被销毁了(比如tcp_session断了，被销毁)，
      //那么从队列里取出来的eh就是野指针了。这里得考虑修改。
      //LOG4CPLUS_DEBUG_FMT(lei::zLog, "push eh:%u, action:%d, et:%d, data:%p", eh->id(), e->Action_, e->Event_Type_, (void *)e->Data_);
      event_queue_.push(e);
      inform_handler_->Inform();
    } else {
      //本线程中调用，同步事件，直接处理。
      ProcessEventInLoop(eh, action, event_type, data, len);
    }
  }

  void EventLoop::ProcessEventInLoop(
      const std::shared_ptr<EventHandler> & eh,
      const EVENT_ACTION & action,
      const EVENT_TYPE & event_type,
      const char * data,
      const uint32_t & len)
  {
    //std::cout << std::this_thread::get_id() << "=" << thread_id_ << std::endl;
    //assert(std::this_thread::get_id() == thread_id_);
    if (nullptr == eh) {
      return ;
    }
    if (EVENT_ACTION_WRITE == action) {
      eh->WriteInLoop(data, len);
      //ProcessEvent(eh, EVENT_ACTION_REGISTER, EVENT_TYPE_WRITE);
      RegisterHandler(eh, EVENT_TYPE_WRITE);
    } else if (EVENT_ACTION_REGISTER == action) {
      RegisterHandler(eh, event_type);
    } else if (EVENT_ACTION_REMOVE == action) {
      RemoveHandler(eh, event_type);
    }
  }

  void EventLoop::ProcessEventInQueueInLoop() {
    //std::cout << "ProcessEventInQueueInLoop:" << name_.c_str() << std::endl;
    //mutex_.lock();
    //Event * e = nullptr;
    //while (!event_queue_.empty()) {
    //  e = event_queue_.front();
    //  //std::cout << "ProcessEventInQueueInLoop" << std::endl;
    //  std::shared_ptr<EventHandler> eh = e->Eh_.lock();
    //  if (eh) {
    //    //LOG4CPLUS_DEBUG_FMT(lei::zLog, "pop eh:%u, action:%d, et:%d, data:%p", eh->id(), e->Action_, e->Event_Type_, (void *)e->Data_);
    //    ProcessEventInLoop(eh, e->Action_, e->Event_Type_, e->Data_ + sizeof(uint32_t), *(uint32_t *)e->Data_);
    //  }
    //  e->~Event();
    //  //std::cout << "free event:" << e << std::endl;
    //  free((void *)e);
    //  event_queue_.pop();
    //}
    //mutex_.unlock();

    Event * e = nullptr;
    while (event_queue_.pop(e)) {
      std::shared_ptr<EventHandler> eh = e->Eh_.lock();
      if (eh) {
        ProcessEventInLoop(eh, e->Action_, e->Event_Type_, e->Data_ + sizeof(uint32_t), *(uint32_t *)e->Data_);
      }
      e->~Event();
      free((void *)e);
      e = nullptr;
    }
  }
}
