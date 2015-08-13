#ifndef __BASE_SYS_INC_EVENTTRIGGER_H__
#define __BASE_SYS_INC_EVENTTRIGGER_H__

#include <memory>

namespace parrot
{
    enum class eIoAction : uint16_t;

    class EventTrigger : public IoEvent
    {
      public:
        EventTrigger();
        virtual ~EventTrigger();
        EventTrigger(const EventTrigger&) = delete;
        EventTrigger& operator=(const EventTrigger&) = delete;

      public:
        void create();
        void trigger();
        void acknowledge();

      public:
        virtual eIoAction handleIoEvent() override;

      private:
        int                              _pipeFds[2];
        std::unique_ptr<unsigned char[]> _buffer;
    };
}

#endif
