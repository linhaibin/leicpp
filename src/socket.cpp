#include "socket.h"

#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "log.h"

namespace lei {

  Socket::~Socket() {
    if (fd_ > 0) {
      ::close(fd_);
      LOG4CPLUS_INFO_FMT(lei::zLog, "socket(%s,%s) is closed !", local_addr_.c_str(), peer_addr_.c_str());
    }
  }

  int Socket::CreateFd() {
    int fd = ::socket(PF_INET, SOCK_STREAM, 0);  
    if (-1 == 0) {
      return RET_ERR;
    }
    return fd;
  }

  Socket* Socket::Listen(
      const std::string & str_addr,
      const int & port,
      const int & SNDBUF,
      const int & RCVBUF) {
    Socket* socket = new Socket(CreateFd(), "", "");
    if (RET_OK != socket->SetNonblock()) {
      delete socket;
      socket = nullptr;
      return socket;
    }
    if (RET_OK != socket->SetReuseAddr()) {
      delete socket;
      socket = nullptr;
      return socket;
    }

    struct sockaddr_in serveraddr;
    ::bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = PF_INET;
    ::inet_aton(str_addr.c_str(), &serveraddr.sin_addr);
    serveraddr.sin_port = htons(port);
    if (RET_OK != socket->Bind(serveraddr)) {
      delete socket;
      socket = nullptr;
      return socket;
    }

    if (SNDBUF > 0) {
      if (RET_OK != socket->SetSndBufSize(SNDBUF)) {
        delete socket;
        socket = nullptr;
        return socket;
      }
    }
    if (RCVBUF > 0) {
      if (RET_OK != socket->SetRcvBufSize(RCVBUF)) {
        delete socket;
        socket = nullptr;
        return socket;
      }
    }

    if (RET_OK != socket->Listen()) {
      delete socket;
      socket = nullptr;
      return socket;
    }
    return socket;
  }

  int Socket::SetNonblock() {
    int opts = ::fcntl(fd_, F_GETFL);
    if (opts < 0) {
      LOG4CPLUS_ERROR_FMT(lei::zLog, "[ERROR]F_GETFL error ! opts:%d", opts);
      return RET_ERR;
    }
    opts |= O_NONBLOCK;
    if (::fcntl(fd_, F_SETFL, opts) < 0) {
      LOG4CPLUS_ERROR_FMT(lei::zLog, "[ERROR]F_SETFL error ! opts:%d", opts);
      return RET_ERR;
    }
    return RET_OK;
  }
  
  int Socket::SetReuseAddr() {
    int on = 1;
    if(::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0){
      LOG4CPLUS_ERROR(lei::zLog, "[ERROR]set SO_REUSEADDR error !");
      return RET_ERR;
    }
    return RET_OK;
  }

  int Socket::SetRcvBufSize(const int & buf_size) {
    if (::setsockopt(
          fd_, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size)) != 0) {
      return RET_ERR;
    }
    return RET_OK;
  }

  int Socket::SetSndBufSize(const int & buf_size) {
    if (::setsockopt(
          fd_, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size)) != 0) {
      return RET_ERR;
    }
    return RET_OK;
  }

  int Socket::SetNoDelay() {
    int on = 1;
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0) {
      return RET_ERR;
    }
    return RET_OK;
  }

  int Socket::Bind(const struct sockaddr_in & addr) {
    if (::bind(fd_, (sockaddr *)&addr, sizeof(addr)) != 0) {
      LOG4CPLUS_ERROR(lei::zLog, "[ERROR]bind error !");
      return RET_ERR; 
    }
    return RET_OK;
  }

  int Socket::Listen() {
    if (::listen(fd_, LISTENQ) != 0) {
      LOG4CPLUS_ERROR(lei::zLog, "[ERROR]listen error !");
      return RET_ERR;
    }
    return RET_OK;
  }
  
  int Socket::Accept(std::string & local_addr, std::string & peer_addr) {
    struct sockaddr_in peer_sa;
    ::bzero(&peer_sa, sizeof(sockaddr_in));
    socklen_t peer_salen = sizeof(sockaddr_in);
    int fd = -1;
    if ((fd = ::accept4(fd_, (struct sockaddr *)&peer_sa, &peer_salen, SOCK_NONBLOCK)) == -1) {
      LOG4CPLUS_ERROR(lei::zLog, "[ERROR]accept4 error !");
      return fd;
    }
    std::stringstream ss; 
    ss << ::inet_ntoa(peer_sa.sin_addr) << ":" << std::to_string(ntohs(peer_sa.sin_port));
    peer_addr = ss.str();

    struct sockaddr_in local_sa;
    ::bzero(&local_sa, sizeof(sockaddr_in));
    socklen_t local_salen = sizeof(sockaddr_in);
    if (::getsockname(fd, (struct sockaddr *)&local_sa, &local_salen) == 0) {
      std::stringstream ss; 
      ss << ::inet_ntoa(local_sa.sin_addr) << ":" << std::to_string(ntohs(local_sa.sin_port));
      local_addr = ss.str();
    }
    LOG4CPLUS_INFO_FMT(lei::zLog, "sock(%s,%s) accepted, fd:%d.", local_addr.c_str(), peer_addr.c_str(), fd);
    return fd;
  }

}
