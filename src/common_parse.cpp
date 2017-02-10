#include "common_parse.h"

#include <arpa/inet.h>
//#include <netinet/in.h>
#include <stdio.h>

void push_pack_head(char* dest, const PHDR& header) {
  *reinterpret_cast<uint32_t*>(dest) = ::htonl(header.len);
  dest += sizeof(uint32_t);
  *reinterpret_cast<uint32_t*>(dest) = ::htonl(header.cmd);
  dest += sizeof(uint32_t);
  *reinterpret_cast<uint64_t*>(dest) = ::htobe64(header.uid);
  dest += sizeof(uint64_t);
  *reinterpret_cast<uint32_t*>(dest) = ::htonl(header.sid);
}

void pop_pack_head(const char* src, PHDR& header) {
  header.len = ::ntohl(*reinterpret_cast<const uint32_t*>(src));
  src += sizeof(uint32_t);
  header.cmd = ::ntohl(*reinterpret_cast<const uint32_t*>(src));
  src += sizeof(uint32_t);
  header.uid = ::be64toh(*reinterpret_cast<const uint64_t*>(src));
  src += sizeof(uint64_t);
  header.sid = ::ntohl(*reinterpret_cast<const  uint32_t*>(src));
}

void EncodeUint32(const uint32_t & n, char * buf) {
  buf[0] = char((n >> 24) & 0xFF);
  buf[1] = char((n >> 16) & 0xFF);
  buf[2] = char((n >> 8) & 0xFF);
  buf[3] = char(n & 0xFF);
}

uint32_t DecodeUint32(const char * buf) {
  const unsigned char *b = (const unsigned char *)buf;
  return (uint32_t(b[0]) << 24)
    | (uint32_t(b[1]) << 16) 
    | (uint32_t(b[2]) << 8)
    | uint32_t(b[3]);
}

void EncodeUint64(const uint64_t & n, char* buf) {
  buf[0] = char((n >> 56) & 0xFF);
  buf[1] = char((n >> 48) & 0xFF);
  buf[2] = char((n >> 40) & 0xFF);
  buf[3] = char((n >> 32) & 0xFF);
  buf[4] = char((n >> 24) & 0xFF);
  buf[5] = char((n >> 16) & 0xFF);
  buf[6] = char((n >> 8) & 0xFF);
  buf[7] = char(n & 0xFF);
}

uint64_t DecodeUint64(const char * buf) {
  const unsigned char *b = (const unsigned char *)buf;
  return (uint64_t(b[0]) << 56)
    | (uint64_t(b[1]) << 48)
    | (uint64_t(b[2]) << 40)
    | (uint64_t(b[3]) << 32)
    | (uint64_t(b[4]) << 24)
    | (uint64_t(b[5]) << 16)
    | (uint64_t(b[6]) << 8)
    | uint64_t(b[7]);
}

void EncodePhdr(const PHDR & h, char * buf) {
  EncodeUint32(h.len, buf);
  EncodeUint32(h.cmd, buf + 4);
  EncodeUint64(h.uid, buf + 8);
  EncodeUint32(h.sid, buf + 16);
}

void DecodePhdr(PHDR & h, const char * buf) {
  h.len = DecodeUint32(buf);
  h.cmd = DecodeUint32(buf + 4);
  h.uid = DecodeUint64(buf + 8);
  h.sid = DecodeUint32(buf + 16);
}

