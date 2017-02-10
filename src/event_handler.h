#pragma once

#include <stdint.h>

#include <atomic>

#include "common.h"

namespace lei {

  class EventHandler {
    public:
      static std::atomic_uint ID_COUNT;

      EventHandler() : status_(EVENT_TYPE_DEFAULT), id_(++ID_COUNT) {}
      virtual ~EventHandler() {}

      inline uint32_t id() { return id_; }

      virtual bool HandleRead() = 0;
      virtual bool HandleWrite() = 0;

      virtual int GetFd() = 0;

      virtual int WriteInLoop(
          const char * data, const uint32_t & len) { return 0; }

      virtual void ResetTimingWheel(const uint32_t & now) {}
      virtual void RemoveTimingWheel() {}
      virtual void RegisterTimingWheel(const uint32_t & timeout) {}

      virtual void Clear() {}

      inline EVENT_TYPE GetEventStatus() { return status_; }
      //设置状态
      inline void SetEventStatus(const EVENT_TYPE & et) { status_ = et; }
      //增加标志位
      inline void AddEventStatus(const EVENT_TYPE & et) { status_ |= et; }
      //清除标志位
      inline void ClearEventStatus(const EVENT_TYPE & et) { status_ &= (~et); }

    private:
      //int fd_;
      uint32_t id_; //event_handler专用标示。
      EVENT_TYPE status_;
  };

}
