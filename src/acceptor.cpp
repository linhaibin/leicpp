#include "acceptor.h"

#include <iostream>
#include <stdlib.h>

#include "tcp_server.h"
#include "log.h"

namespace lei {

  Acceptor::Acceptor(TcpServer * tcp_server,
      const std::string & listen_addr,
      const int & port,
      const int & SNDBUF,
      const int & RCVBUF) : tcp_server_(tcp_server),
        socket_(Socket::Listen(listen_addr, port, SNDBUF, RCVBUF)) {
    if (socket_ == nullptr) {
      abort();
    }
  }

  Acceptor::~Acceptor() {
  }

  bool Acceptor::HandleRead() {
    std::string local_addr, peer_addr;
    int fd = socket_->Accept(local_addr, peer_addr);
    LOG4CPLUS_INFO_FMT(lei::zLog, "Accepted a fd:%d.", fd);
    tcp_server_->AddNewSession(fd, local_addr, peer_addr);
    return true;
  }

  bool Acceptor::HandleWrite() { 
    return true;
  }

}
