#pragma once

#include <vector>
#include <queue>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <unordered_map>
#include <sys/epoll.h>

#include <boost/lockfree/queue.hpp>

#include "common.h"
#include "inform_handler.h"
#include "timing_wheel.h"
#include "timer_handler.h"

namespace lei {

  class TcpServer;
  class EventHandler;

  struct Event {
    Event(
        const std::shared_ptr<EventHandler> & eh,
        const EVENT_ACTION & action, 
        const EVENT_TYPE & event_type) 
          : Eh_(eh), Action_(action), Event_Type_(event_type) {
    }

    ~Event() {
      /*
         if (nullptr != Data_) {
         delete[] Data_;
         Data_ = nullptr;
         Action_ = EVENT_ACTION_DEFAULT;
         }
         */
      Eh_.reset();
    }

    std::weak_ptr<EventHandler> Eh_;
    EVENT_ACTION Action_;
    EVENT_TYPE Event_Type_;
    //char * Data_;
    char Data_[0];
  };

  /*
   *
   */
  //InLoop结尾的成员必须在同一线程中调用。
  class EventLoop {
    public:
      friend TimingWheel;

      EventLoop(const std::string & name, TcpServer * tcp_server, bool test = false);
      ~EventLoop();

      const bool & Loop();
      void DispatchAndProcess();

      //往epfd_中注册eh的需要监听的事件。
      void RegisterHandler(
          const std::shared_ptr<EventHandler> & eh,
          const EVENT_TYPE & et);
      //按et指定的，移除eh在epfd_中的监听类型。
      //比如原来eh原本有WRITE|READ，移除READ后，epfd_仍继续监听WRITE。
      void RemoveHandler(
          const std::shared_ptr<EventHandler> & eh,
          const EVENT_TYPE & et);

      //void ProcessEvent(EventHandler * eh, const EVENT_ACTION & action);
      //处理各种事件请求，比如对某一handler请求发送数据。
      //如果调用线程和被调用loop处于同一线程(event_loop)则立即处理。
      //否则生成一个事件Event放入队列中，等待inform_handler_通知后同步处理。
      void ProcessEvent(
          const std::shared_ptr<EventHandler> & eh,
          const EVENT_ACTION & action,
          const EVENT_TYPE & event_type,
          const char * data = nullptr,
          const uint32_t & len = 0);
      //处理事件请求。只能在本线程(同一event_loop)中调用。
      //本event_loop之外调用本event_loop进行事件处理，要使用ProcessEvent。
      void ProcessEventInLoop(
          const std::shared_ptr<EventHandler> & eh,
          const EVENT_ACTION & action,
          const EVENT_TYPE & event_type,
          const char * data = nullptr,
          const uint32_t & len = 0);

      //处理时间队列中的事件，在本线程中处理。
      void ProcessEventInQueueInLoop();

      //检测超时的连接
      inline void CheckOvertime(const uint64_t & count) {
        timing_wheel_.CheckOvertime(count);
      }

      inline uint32_t now_sec() { return now_sec_; }
      inline TimingWheel & timing_wheel() { return timing_wheel_; }

      std::atomic_ullong W_COUNT;
      std::atomic_ullong R_COUNT;
      std::atomic_uint MAX_READ_BUFFSIZE;
      std::atomic_uint MAX_WRITE_BUFFSIZE;
      uint32_t last_count_time_;
    private:
      static const int InitEventSize = 64; //from libev
      static const int InitQueueSize = 1024;
      static const int InitTimeout = 1 << 7; //2的整数次幂 //1 << 7

      std::thread::id thread_id_;
      std::string name_;
      TcpServer * tcp_server_;

      int epfd_;
      std::vector<struct epoll_event> ep_events_;

      typedef std::unordered_map<uint32_t, std::weak_ptr<EventHandler> > EH_MAP;
      EH_MAP handlers_;

      /*
       * 事件队列，用于同步不同线程的调用。
       * 比如，其他线程(event_loop)要对本线程中的某一
       * TcpSession进行数据发送操作，则会将此事件先放
       * 入event_queue_。等到inform_handler接到通知，
       * 一并进行处理。
       */
      //std::mutex mutex_;
      //std::queue<struct Event *> event_queue_;
      boost::lockfree::queue<struct Event *> event_queue_;
      std::shared_ptr<InformHandler> inform_handler_;
      std::shared_ptr<TimerHandler> timer_handler_;

      uint32_t now_sec_;
      TimingWheel timing_wheel_;

      bool test_;
  };

}
