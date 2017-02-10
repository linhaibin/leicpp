#include "timer_handler.h"

#include <sys/timerfd.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include "event_loop.h"
#include "log.h"

namespace lei {

  TimerHandler::TimerHandler(EventLoop * event_loop)
   : event_loop_(event_loop) {
     timer_fd_ = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
     if (timer_fd_ < 0) {
       LOG4CPLUS_ERROR(lei::zLog, "create timer_handler failed !");
       abort();
     }

     struct timespec now;
     if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
       LOG4CPLUS_ERROR(lei::zLog, "clock_gettime failed !");
       abort();
     }
     struct itimerspec new_value;
     new_value.it_value.tv_sec = now.tv_sec + 1;
     new_value.it_value.tv_nsec = now.tv_nsec;
     new_value.it_interval.tv_sec = 1;
     new_value.it_interval.tv_nsec = 0;
     if (timerfd_settime(timer_fd_, TFD_TIMER_ABSTIME, &new_value, NULL) == -1) {
       LOG4CPLUS_ERROR(lei::zLog, "clock_gettime failed !");
       abort();
     }
  }

  TimerHandler::~TimerHandler() {
    event_loop_ = nullptr;
    close(timer_fd_);
  }

  bool TimerHandler::HandleRead() {
    uint64_t count = 0;
    ssize_t s = read(timer_fd_, &count, sizeof(count));
    if (s != sizeof(count)) {
      LOG4CPLUS_ERROR_FMT(lei::zLog, "read timer_fd failed ! readed:%u, target_size:%u", s, sizeof(count));
      abort();
    }
    event_loop_->CheckOvertime(count);
    return true;
  }

}
