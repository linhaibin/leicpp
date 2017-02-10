#include "tcp_server.h"

#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "acceptor.h"
#include "event_loop.h"
#include "event_loop_thread.h"
#include "log.h"

namespace lei {

  std::atomic_ullong TcpServer::W_COUNT(0);
  std::atomic_ullong TcpServer::R_COUNT(0);

  TcpServer::TcpServer(const int & thread_num,
      const std::string & listen_addr,
      const int & port,
      const int & SNDBUF,
      const int & RCVBUF) 
    : event_loop_threads_(thread_num, nullptr),
      event_loops_(thread_num, nullptr),
      acceptor_(new Acceptor(this, listen_addr, port, SNDBUF, RCVBUF)), 
      accept_loop_(nullptr),
      //accept_loop_thread_(nullptr),
      thread_num_(thread_num) {
      not_close_ = true;
      next_loop_ = 0;
  }

  TcpServer::~TcpServer() {
    not_close_ = false;
    //accept_loop_thread_ = nullptr;
    accept_loop_ = nullptr;
    //if (nullptr != accept_loop_thread_) {
    //  delete accept_loop_thread_;
    //  accept_loop_thread_ = nullptr;
    //}
    for (auto & it : event_loop_threads_) {
      if (nullptr != it) {
        //delete it;
        it = nullptr;
      }
    }
    acceptor_ = nullptr;
    //if (nullptr != acceptor_) {
    //  delete acceptor_;
    //  acceptor_ = nullptr;
    //}
    for (auto & it : event_loops_) {
      if (nullptr != it) {
        //delete it;
        it = nullptr;
      }
    } 
  }

  void TcpServer::AddNewSession(const int & fd, const std::string & local_addr, const std::string & peer_addr) {
    std::shared_ptr<EventLoop> el = NextLoop();
    std::shared_ptr<EventHandler> eh = NewSession(el, fd, local_addr, peer_addr);
    el->ProcessEvent(eh, EVENT_ACTION_REGISTER, EVENT_TYPE_READ);
  }

  std::shared_ptr<EventLoop> TcpServer::NextLoop() {
    std::shared_ptr<EventLoop> loop = accept_loop_;
    if (!event_loops_.empty()) {
      loop = event_loops_[next_loop_++];
      if (next_loop_ == thread_num_) {
        next_loop_ = 0;
      }
    }
    return loop;
  }

  bool TcpServer::Start() {
    accept_loop_ = std::make_shared<EventLoop>("server_acceptor_loop", this, false);
    //accept_loop_thread_ = std::make_shared<EventLoopThread>("acceptor_thread", accept_loop_);
    accept_loop_->ProcessEvent(
        acceptor_, EVENT_ACTION_REGISTER, EVENT_TYPE_READ);

    int size = event_loop_threads_.size();
    for (int i = 0; i < size; ++i) {
      std::string name = "loop_" + std::to_string(i);
      //event_loops_[i] = new EventLoop(name, this);
      event_loops_[i] = std::make_shared<EventLoop>(name, this);
      name = "thread_" + std::to_string(i);
      //event_loop_threads_[i] = new EventLoopThread(name, this, event_loops_[i]);
      event_loop_threads_[i] = std::make_shared<EventLoopThread>(name, event_loops_[i]);
      LOG4CPLUS_INFO_FMT(lei::zLog, "%s is running.", name.c_str());
    }
  }

  void TcpServer::DoAcceptLoopOnce() {
    accept_loop_->DispatchAndProcess();
  }
}
