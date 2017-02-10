#include "event_loop_thread.h"

#include <iostream>
#include <unistd.h>
#include <mutex>
#include <sys/epoll.h>

#include "tcp_server.h"
#include "event_loop.h"
#include "log.h"

namespace lei {

  EventLoopThread::EventLoopThread(
      const std::string & name,
      const std::shared_ptr<EventLoop> & event_loop)
    : name_(name),
      event_loop_(event_loop),
      thread_(&EventLoopThread::Run, this) {
  }

  EventLoopThread::~EventLoopThread() {
    if (thread_.joinable()) {
      thread_.join();
      LOG4CPLUS_INFO_FMT(lei::zLog, "%s stopped.", name_.c_str());
    }
  }

  void EventLoopThread::Run() {
    while (event_loop_->Loop()) ;
  }

}
