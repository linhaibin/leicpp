#pragma once

#include <vector>
#include <mutex>
#include <atomic>

#include "tcp_session.h"

namespace lei {

  class Acceptor;
  class EventLoopThread;
  class EventLoop;
  class EventHandler;

  class TcpServer {
    public: 
      static std::atomic_ullong W_COUNT;
      static std::atomic_ullong R_COUNT;

      TcpServer(
          const int & thread_num,
          const std::string & listen_addr,
          const int & port,
          const int & SNDBUF = 0, //socket fd的发送缓冲大小
          const int & RCVBUF = 0  //socket fd的接受缓冲大小
          );
      virtual ~TcpServer();

      void AddNewSession(
          const int & fd,
          const std::string & local_addr,
          const std::string & peer_addr);

      virtual std::shared_ptr<EventHandler> NewSession(
          const std::shared_ptr<EventLoop> & event_loop,
          const int & fd,
          const std::string & local_addr,
          const std::string & peer_addr) = 0;

      std::shared_ptr<EventLoop> NextLoop();

      bool Start();
      void DoAcceptLoopOnce();
      const bool & IsNotClosed() { return not_close_; }

      std::mutex mutex_;
    private:
      typedef std::vector<std::shared_ptr<EventLoopThread> > ELT_VEC;
      typedef ELT_VEC::iterator ELT_VEC_IT;

      typedef std::vector<std::shared_ptr<EventLoop> > EL_VEC;
      typedef EL_VEC::iterator EL_VEC_IT;

      //一个线程执行一个event_loop_循环，
      //一个server包含多个线程，每个线程负责多个fd的监听。
      int thread_num_;
      ELT_VEC event_loop_threads_;
      EL_VEC event_loops_;

      std::shared_ptr<Acceptor> acceptor_;
      std::shared_ptr<EventLoop> accept_loop_;
      //std::shared_ptr<EventLoopThread> accept_loop_thread_;

      bool not_close_;
      int next_loop_;
  };

}
