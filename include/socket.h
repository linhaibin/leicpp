#ifndef _LEI_SOCKET_H_
#define _LEI_SOCKET_H_

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

namespace lei {
  class Socket {
    public:
      const static int RET_OK = 0;
      const static int RET_ERR = -1;

      const static int LISTENQ = 1024;

      Socket(const int & fd, const std::string & local_addr, const std::string & peer_addr) :
        fd_(fd),
        local_addr_(local_addr),
        peer_addr_(peer_addr) {}
      ~Socket();

      static int CreateFd();
      static Socket* Listen(const std::string & addr,
          const int & port,
          const int & SNDBUF = 0,
          const int & RCVBUF = 0);

      inline int Send(const char * buf, const uint32_t & len) {
        return ::send(fd_, buf, len, 0);
      }

      inline int Recv(char * buf, const uint32_t & len) {
        return ::recv(fd_, buf, len, 0);
      }

      int SetNonblock();
      int SetReuseAddr();
      //set SOL_SOCKET SO_RCVBUF
      int SetRcvBufSize(const int & buf_size);
      //set SOL_SOCKET SO_SNDBUF
      int SetSndBufSize(const int & buf_size);
      //set IPPROTO_TCP TCP_NODELAY
      int SetNoDelay();
      int Bind(const struct sockaddr_in & addr);
      int Listen();
      int Accept(std::string & local_addr, std::string & peer_addr);

      inline int GetFd() { return fd_; }

      inline const std::string & local_addr() { return local_addr_; }
      inline void set_local_addr(const std::string & local_addr) { local_addr_ = local_addr; }
      inline const std::string & peer_addr() { return peer_addr_; }
      inline void set_peer_addr(const std::string & peer_addr) { peer_addr_ = peer_addr; }

    private:
      int fd_;
      std::string local_addr_;
      std::string peer_addr_;
  };
}

#endif//_LEI_SOCKET_H_
