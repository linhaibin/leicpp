#ifndef _LEI_EVENT_LOOP_THREAD_H_
#define _LEI_EVENT_LOOP_THREAD_H_

#include <thread>
#include <string>

namespace lei {

  class TcpServer;
  class EventLoop;

  class EventLoopThread {
    public:
      EventLoopThread(
          const std::string & name,
          const std::shared_ptr<EventLoop> & event_loop);
      ~EventLoopThread();

      void Run();
    private:

      std::string name_;
      std::shared_ptr<EventLoop> event_loop_;

      std::thread thread_;
  };

}

#endif//_LEI_EVENT_LOOP_THREAD_H_
