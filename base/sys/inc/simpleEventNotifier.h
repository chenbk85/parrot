#ifndef __BASE_SYS_INC_SIMPLEEVENTNOTIFIER_H__
#define __BASE_SYS_INC_SIMPLEEVENTNOTIFIER_H__

#include <cstdint>

namespace parrot
{
    class IoEvent;
    class EventNotifier;
    class SimpleEventNotifierImpl;

    /**
     * All platforms can use this notifer.
     */
    class SimpleEventNotifier : public EventNotifier
    {
      public:
        SimpleEventNotifier();
        virtual ~SimpleEventNotifier();
        SimpleEventNotifier(const SimpleEventNotifier&) = delete;
        SimpleEventNotifier& operator=(const SimpleEventNotifier&) = delete;

      public:
        void create() override;
        uint32_t waitIoEvents(int32_t ms) override;
        void addEvent(IoEvent *ev) override;
        void monitorRead(IoEvent *ev) override;
        void monitorWrite(IoEvent *ev) override;
        void delEvent(IoEvent *ev) override;
        IoEvent *getIoEvent(uint32_t idx) const;
        void stopWaiting() override;

      private:
        SimpleEventNotifierImpl *_impl;
    };
}

#endif
