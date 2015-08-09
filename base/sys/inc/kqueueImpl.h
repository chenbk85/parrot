#ifndef __BASE_SYS_INC_KQUEUEIMPL_H__
#define __BASE_SYS_INC_KQUEUEIMPL_H__

#if defined(__APPLE__)

#include <cstdint>
#include <memory>

namespace parrot
{
    class IoEvent;
    class EventTrigger;
    enum class eIoAction : uint16_t;

    class KqueueImpl
    {
      public:
        KqueueImpl(uint32_t maxEvCount) noexcept;
        ~KqueueImpl();
        KqueueImpl(const KqueueImpl &kq) = delete;
        KqueueImpl& operator=(const KqueueImpl &kq) = delete;

      public:
        void create();
        void waitIoEvents(int32_t ms);
        void addEvent(IoEvent *ev, eIoAction action);
        void updateEventAction(IoEvent *ev, eIoAction action);
        void delEvent(IoEvent *ev);
        IoEvent * getIoEvent(uint32_t idx) const noexcept;
        int getAction(uint32_t idx) const noexcept;
        void closeKqueue();

      private:
        int32_t                                 _kqueueFd;
        uint32_t                                _keventCount;
        std::unique_ptr<EventTrigger>           _trigger;
        std::unique_ptr<struct kevent[]>        _events;
    };
}

#endif // #if defined(__APPLE__)
#endif
