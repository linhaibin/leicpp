#pragma once

#include <memory>

#include "socket.h"
#include "event_handler.h"

namespace lei {

  class TcpServer;

  class Acceptor : public EventHandler {
    public:
      Acceptor(TcpServer * tcp_server,
          const std::string & listen_addr,
          const int & port,
          const int & SNDBUF = 0,
          const int & RCVBUF = 0);
      virtual ~Acceptor();

      virtual bool HandleRead();
      virtual bool HandleWrite();

      virtual int GetFd() { return socket_->GetFd(); }
    private:
      TcpServer * tcp_server_;
      std::unique_ptr<Socket> socket_;
  };

}
