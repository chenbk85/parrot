#ifndef __BASE_SYS_INC_TIMEOUTHANDLER_H__
#define __BASE_SYS_INC_TIMEOUTHANDLER_H__

namespace parrot {
template <typename T> class TimeoutHandler {
  public:
    virtual ~TimeoutHandler() {
    }

  public:
    virtual void onTimeout(T *) = 0;
};
}

#endif
