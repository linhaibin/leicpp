#pragma once

#include <atomic>

#include "event_handler.h"

namespace lei {

  class EventLoop;

  /*
   * 用来通知event_loop_处理事件队列中的事件。
   */
  class InformHandler : public EventHandler {
    public:
      InformHandler(EventLoop * event_loop);
      virtual ~InformHandler();

      virtual bool HandleRead();
      virtual bool HandleWrite() { return true; }
      virtual int GetFd() { return fd_; }

      void Inform();

    private:
      EventLoop * event_loop_;
      int fd_;
      std::atomic_flag informed_;
  };

}
