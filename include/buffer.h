#pragma once

#include <stdint.h>
#include <string.h>

#include "log.h"

namespace lei {

  class Buffer {
    public:
      Buffer(const uint32_t & size = 2048);
      Buffer(Buffer && b);
      ~Buffer();

      //扩展空间，*2
      bool Grow();
      //调整有效数据的位置，使有效数据左顶格。
      void Adjust();
      //返回可用空间。
      inline uint32_t Space() { return size_ - write_index_; }
      //返回已有数据长度
      inline uint32_t Len() { return write_index_ - read_index_; }
      //返回总空间大小
      inline const uint32_t & Size() { return size_; }
      //有效数据起始位置。
      inline char * StartReadIndex() { return data_ + read_index_; }
      //能写入的空间的起始位置。
      inline char * StartWriteIndex() { return data_ + write_index_; }
      //写入数据
      bool Write(const char * data, const uint32_t & len);
      //告知Buffer已经读出了size空间。Buffer会移动相关索引。
      inline void ReadDone(const uint32_t & size) { read_index_ += size; }
      //告知Buffer已经写了size空间。Buffer会移动相关索引。
      inline void WriteDone(const uint32_t & size) { write_index_ += size; }

      //Just for testing.
      void Dump() {
        char * b = (char *)malloc(size_ + 1);
        memcpy(b, StartReadIndex(), Len());
        b[Len()] = '\0';
        //LOG4CPLUS_DEBUG_FMT(lei::zLog, "Buffer dump:%s, len:%u", b, Len());
        free(b);
      }

    private:
      inline void setToNull() {
        data_ = nullptr;
        size_ = 0;
      }

    private:
      uint32_t read_index_;
      uint32_t write_index_;
      uint32_t size_;
      char * data_;
  };

}
