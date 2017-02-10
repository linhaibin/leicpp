#pragma once

namespace lei {

  typedef short EVENT_TYPE;
  enum {
    EVENT_TYPE_DEFAULT = 0x00,
    EVENT_TYPE_READ = 0x01,
    EVENT_TYPE_WRITE = 0x02,
  };

  typedef short EVENT_ACTION;
  enum {
    //互斥
    EVENT_ACTION_DEFAULT = 0,
    EVENT_ACTION_WRITE = 1,
    EVENT_ACTION_REGISTER = 2,
    EVENT_ACTION_REMOVE = 3,
  };

  typedef unsigned long long TypeMsec;
  TypeMsec GetCurrentTimeMsec();
}
