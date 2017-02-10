#include "common.h"

#include <sys/time.h>
#include <stdio.h>

namespace lei {
  TypeMsec GetCurrentTimeMsec() {  
    struct timeval tv;  
    gettimeofday(&tv, NULL);  
    return ((TypeMsec)tv.tv_sec * 1000 + (TypeMsec)tv.tv_usec / 1000);  
  }
}
