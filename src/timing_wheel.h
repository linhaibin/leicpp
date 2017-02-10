#pragma once

#include <vector>
#include <unordered_set>

namespace lei {

  class EventLoop;

  class TimingWheel {
    public:
      TimingWheel(EventLoop * event_loop, const uint32_t & size);
      ~TimingWheel();
      void CheckOvertime(const uint64_t & count);
      uint32_t Register(const uint32_t & id, const uint32_t & timeout);
      void Remove(const uint32_t & index, const uint32_t & id);

      inline uint32_t size() { return size_; }
    private:
      EventLoop * event_loop_;

      uint32_t size_; //几秒超时，size就多大
      uint32_t idx_; //当前检测过的索引

      typedef std::unordered_set<uint32_t> UINT32_USET;
      typedef UINT32_USET::iterator UINT32_USET_IT;

      typedef std::vector<uint32_t> UINT32_VEC;
      typedef UINT32_VEC::iterator UINT32_VEC_IT;

      std::vector<UINT32_USET > datas_;

      UINT32_VEC del_ids_;
  };

}
