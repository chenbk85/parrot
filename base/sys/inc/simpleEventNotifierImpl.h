#ifndef __BASE_SYS_INC_SIMPLEEVENTNOTIFIERIMPL_H__
#define __BASE_SYS_INC_SIMPLEEVENTNOTIFIERIMPL_H__

#include <mutex>
#include <condition_variable>

namespace parrot
{
    class SimpleEventNotifierImpl
    {
      public:
        SimpleEventNotifierImpl();
        ~SimpleEventNotifierImpl();
        SimpleEventNotifierImpl(const SimpleEventNotifierImpl&) = delete;
        SimpleEventNotifierImpl& operator=(const SimpleEventNotifierImpl&) = delete;

      public:
        uint32_t waitIoEvents(int32_t ms);
        void stopWaiting();

      public:
        bool                           _waiting;
        uint32_t                       _signalCount;
        std::mutex                     _lock;
        std::condition_variable        _condVar;        
    };
}

#endif
