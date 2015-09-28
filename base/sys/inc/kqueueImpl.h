#ifndef __BASE_SYS_INC_KQUEUEIMPL_H__
#define __BASE_SYS_INC_KQUEUEIMPL_H__

#if defined(__APPLE__)

#include <cstdint>
#include <memory>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

namespace parrot
{
class IoEvent;
class EventTrigger;

class KqueueImpl
{
  public:
    KqueueImpl(uint32_t maxEvCount) noexcept;
    ~KqueueImpl();
    KqueueImpl(const KqueueImpl& kq) = delete;
    KqueueImpl& operator=(const KqueueImpl& kq) = delete;

  public:
    void create();
    uint32_t waitIoEvents(int32_t ms);
    void addEvent(IoEvent* ev);
    void monitorRead(IoEvent* ev);
    void monitorWrite(IoEvent* ev);
    void delEvent(IoEvent* ev);
    IoEvent* getIoEvent(uint32_t idx) const noexcept;
    void stopWaiting();
    void close();

  private:
    void msToTimespec(struct timespec* ts, uint32_t ms);

  private:
    int32_t _kqueueFd;
    uint32_t _keventCount;
    std::unique_ptr<EventTrigger> _trigger;
    std::unique_ptr<struct kevent[]> _events;
};
}

#endif // __APPLE__
#endif // __BASE_SYS_INC_KQUEUEIMPL_H__
