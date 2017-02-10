#include "timing_wheel.h"

#include "event_loop.h"
#include "log.h"

namespace lei {

  TimingWheel::TimingWheel(EventLoop * event_loop, const uint32_t & size)
    : event_loop_(event_loop), 
      size_(size),
      idx_(0),
      datas_(size, std::unordered_set<uint32_t>()) {
    if (size & (size - 1)) {
      LOG4CPLUS_ERROR_FMT(lei::zLog, "size must be an integer power of 2. size:%u.", size);
      abort();
    }
  }

  TimingWheel::~TimingWheel() {}

  void TimingWheel::CheckOvertime(const uint64_t & count) {
    for (uint64_t i = 0; i < count; ++i) {
      if (++idx_ == size_) {
        idx_ = 0;
      }

      UINT32_USET & id_set = datas_[idx_];
      if (!id_set.empty()) {
        size_t need_del = id_set.size();
        if (need_del > del_ids_.size()) {
          del_ids_.resize(need_del);
        }
        
        size_t index = 0;
        UINT32_USET_IT end = id_set.end();
        for (UINT32_USET_IT it = id_set.begin(); it != end; ++it) {
          del_ids_[index++] = *it;
        }

        const uint32_t & now = event_loop_->now_sec();
        for (index = 0; index < need_del; ++index) {
          uint32_t id = del_ids_[index];
          EventLoop::EH_MAP::iterator it = event_loop_->handlers_.find(id);
          if (it != event_loop_->handlers_.end()) {
            std::shared_ptr<EventHandler> seh = it->second.lock();
            if (seh) {
              seh->ResetTimingWheel(now);
            } else {
              id_set.erase(id);
            }
          } else {
            id_set.erase(id);
          }
        }
      }
    }
  }

  uint32_t TimingWheel::Register(const uint32_t & id, const uint32_t & timeout) {
    uint32_t index = (idx_ + timeout) & (size_ - 1);
    datas_[index].insert(id);
    return index;
  }

  void TimingWheel::Remove(const uint32_t & index, const uint32_t & id) {
    datas_[index].erase(id);
  }

}
