#include "buffer.h"

#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace lei {

  Buffer::Buffer(const uint32_t & size)
    : size_(size),
      read_index_(0),
      write_index_(0),
      data_(nullptr) {
        assert(size > 0);
        data_ = (char *)malloc(size);
  }

  Buffer::Buffer(Buffer && b)
    : data_(b.data_),
      size_(b.size_) {
        b.setToNull();
  }

  Buffer::~Buffer() {
    if (nullptr != data_) {
      free(data_);
      setToNull();
    }
  }

  bool Buffer::Grow() {
    uint32_t new_size = size_ << 1;
    char * p = (char *)realloc(data_, new_size);
    if (nullptr == p) {
      return false;
    }
    data_ = p;
    size_ = new_size;
    return true;
  }

  void Buffer::Adjust() {
    if (0 != read_index_) {
      uint32_t len = Len();
      if (len > 0) {
        //std::cout << "c1:" << data_ << ", start:" << static_cast<const void *>(StartReadIndex()) << ", len:" << len << ", readindex:" << read_index_ << ", writeindex:" << write_index_ << std::endl;
        memmove(data_, StartReadIndex(), len);
        //std::cout << "c2" << std::endl;
        write_index_ -= read_index_;
        read_index_ = 0;
      } else {
        write_index_ = 0;
        read_index_ = 0;
      }
    }
  }

  bool Buffer::Write(const char * data, const uint32_t & len) {
    while (Space() < len) {
      if (!Grow()) {
        return false;
      }
    }
    memcpy(StartWriteIndex(), data, len);
    WriteDone(len);
    return true;
  }

}
