#ifndef __BASE_SYS_INC_EVENTNOTIFIER_H__
#define __BASE_SYS_INC_EVENTNOTIFIER_H__

#include <cstdint>

namespace parrot
{
    class IoEvent;
    enum class eIoAction : uint16_t;
    
    class EventNotifier
    {
      public:
        EventNotifier() {}
        virtual ~EventNotifier() {}

      public:
        virtual void create() = 0;

        virtual uint32_t waitIoEvents(int32_t ms) = 0;

        virtual void addEvent(IoEvent *ev) = 0;
        
        virtual void monitorRead(IoEvent *ev) = 0;

        virtual void monitorWrite(IoEvent *ev) = 0;

        virtual void delEvent(IoEvent *ev) = 0;

        virtual IoEvent* getIoEvent(uint32_t idx) const noexcept = 0;

        virtual void stopWaiting() = 0;
    };
}

#endif
