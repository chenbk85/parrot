#ifndef __BASE_SYS_INC_EVENTNOTIFIER_H__
#define __BASE_SYS_INC_EVENTNOTIFIER_H__

namespace parrot
{
    class EventNotifier
    {
      public:
        EventNotifier();
        virtual ~EventNotifier();

      public:
        virtual void create() = 0;
        virtual uint32_t waitIoEvents(int32_t ms) = 0;
        virtual void addEvent(IoEvent *ev, eIoAction action) = 0;
        virtual void updateEventAction(IoEvent *ev eIoAction action) = 0;
        virtual void delEvent(IoEvent *ev, eIoAction action) = 0;
        virtual IoEvent* getIoEvent(uint32_t idx) const noexcept = 0;
        virtual int getFilter(uint32_t idx) const noexcept = 0;
    };
}

#endif
