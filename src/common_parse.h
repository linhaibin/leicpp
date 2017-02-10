//#ifndef _COMMON_PARSE_H_
//#define _COMMON_PARSE_H_
#pragma once

#include <stdint.h>

#define MAX_MSG_SIZE 65535

#pragma pack(1)
struct PHDR {
  PHDR() : len(0), cmd(0), uid(0), sid(0) {}
  uint32_t len;   //! packet length
  uint32_t cmd;   //! command
  uint64_t uid;   //! user id
  uint32_t sid;   //! session_id created by server
};
#pragma pack()

static const uint32_t HEADER_LEN = sizeof(PHDR);
static const uint32_t UINT32_LEN = sizeof(uint32_t);
void push_pack_head(char*, const PHDR&);
void pop_pack_head(const char*, PHDR&);

void EncodeUint32(const uint32_t &, char*);
uint32_t DecodeUint32(const char *);
void EncodeUint64(const uint64_t &, char*);
uint64_t DecodeUint64(const char *);

void EncodePhdr(const PHDR &, char*);
void DecodePhdr(PHDR &, const char*);

//#endif//_COMMON_PARSE_H_
