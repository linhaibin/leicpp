#pragma once

#include "event_handler.h"

namespace lei {

  class EventLoop;

  class TimerHandler : public EventHandler {
    public:
      TimerHandler(EventLoop * event_loop_);
      virtual ~TimerHandler();

      virtual bool HandleRead();
      virtual bool HandleWrite() { return true; }

      virtual int GetFd() { return timer_fd_; }

    private:
      int timer_fd_;
      EventLoop * event_loop_;
  };

}
