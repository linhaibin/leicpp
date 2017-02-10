#include "inform_handler.h"

#include <iostream>
#include <sys/eventfd.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "event_loop.h"
#include "log.h"

namespace lei {

  InformHandler::InformHandler(EventLoop * event_loop)
    : event_loop_(event_loop),
      fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      informed_(ATOMIC_FLAG_INIT) {
        if (fd_ < 0) {
          abort();
        }
  }

  InformHandler::~InformHandler() {
    ::close(fd_);
  }
  
  bool InformHandler::HandleRead() {
    uint64_t one = 0;
    ssize_t n = ::read(fd_, &one, sizeof(one));
    if (sizeof(one) != n) {
      LOG4CPLUS_ERROR_FMT(lei::zLog, "[ERROR] InformHandler read failed! rn:%u", n);
    }
    event_loop_->ProcessEventInQueueInLoop();
    informed_.clear();
    LOG4CPLUS_DEBUG(lei::zLog, "Informer cleared !");
    return true;
  }
  
  void InformHandler::Inform() {
    if (!informed_.test_and_set()) {
      uint64_t one = 1;
      ssize_t n = ::write(fd_, &one, sizeof(one));
      std::cout << "[DEBUG] Informed !" << std::endl;
      if (sizeof(one) != n) {
        LOG4CPLUS_ERROR_FMT(lei::zLog, "InformHandler inform failed ! wn:%u", n);
        //informed_.clear();
      }
    }
  }

}
