#ifndef __BASE_SYS_INC_KQUEUE_H__
#define __BASE_SYS_INC_KQUEUE_H__

#if defined(__APPLE__)

#include <cstdint>

#include "eventNotifier.h"

namespace parrot {
class KqueueImpl;
class IoEvent;

class Kqueue final : public EventNotifier {
  public:
    explicit Kqueue(uint32_t size) noexcept;
    virtual ~Kqueue();
    Kqueue(const Kqueue &) = delete;
    Kqueue &operator=(const Kqueue &) = delete;

  public:
    void create() override;
    uint32_t waitIoEvents(int32_t ms) override;
    void addEvent(IoEvent *ev) override;
    void monitorRead(IoEvent *ev) override;
    void monitorWrite(IoEvent *ev) override;
    void delEvent(IoEvent *ev) override;
    IoEvent *getIoEvent(uint32_t idx) const noexcept override;
    void stopWaiting() override;

  private:
    KqueueImpl *_impl;
};
}

#endif // __APPLE__
#endif // __BASE_SYS_INC_KQUEUE_H__
