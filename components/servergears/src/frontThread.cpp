#include <system_error>

#include "eventNotifier.h"
#include "epoll.h"
#include "kqueue.h"
#include "simpleEventNotifier.h"

#include "config.h"
#include "wsServerConn.h"
#include "frontThread.h"
#include "macroFuncs.h"

namespace parrot
{
FrontThread::FrontThread()
    : PoolThread(),
      _newConnListLock(),
      _newConnList(),
      _connMap(),
      _notifier(nullptr),
      _config(nullptr)
{
}

void FrontThread::setConfig(const Config* cfg)
{
    _config = cfg;
}

void FrontThread::beforeStart()
{
#if defined(__linux__)
    _notifier.reset(new Epoll(_config->_frontThreadMaxConnCount));
#elif defined(__APPLE__)
    _notifier.reset(new Kqueue(_config->_frontThreadMaxConnCount));
#elif defined(_WIN32)
//    _notifier.reset(new
//    SimpleEventNotifier(_config->_frontThreadMaxConnCount));
#endif
    _notifier->create();
}

void FrontThread::stop()
{
    _notifier->stopWaiting();
    ThreadBase::stop();
}

void FrontThread::addConn(std::shared_ptr<WsServerConn>&& conn)
{
    _newConnListLock.lock();
    _newConnList.push_back(std::move(conn));
    _newConnListLock.unlock();

    _notifier->stopWaiting();
}

void FrontThread::addConn(std::list<std::shared_ptr<WsServerConn>>& connList)
{
    _newConnListLock.lock();
    // Append the connList to the _newConnList.
    _newConnList.splice(_newConnList.end(), connList);
    _newConnListLock.unlock();

    _notifier->stopWaiting();
}

void FrontThread::addConnToNotifier()
{
    std::list<std::shared_ptr<WsServerConn>> tmpList;

    _newConnListLock.lock();
    std::swap(tmpList, _newConnList);
    _newConnListLock.unlock();

    for (auto& c : tmpList)
    {
        c->setAction(c->getDefaultAction());
        _notifier->addEvent(c.get());
        _connMap[c->getUniqueKey()] = std::move(c);
    }
}

void FrontThread::run()
{
    uint32_t eventNum  = 0;
    uint32_t idx       = 0;
    WsServerConn* conn = nullptr;
    eIoAction act      = eIoAction::None;

    try
    {
        while (!isStopped())
        {
            addConnToNotifier();

            eventNum = _notifier->waitIoEvents(1000);

            for (idx = 0; idx != eventNum; ++idx)
            {
                // We are sure that the IoEvnet is WsServerConn,
                // so we can use static_cast.
                conn = static_cast<WsServerConn*>(_notifier->getIoEvent(idx));
                act  = conn->handleIoEvent();
                conn->setNextAction(act);

                switch (act)
                {
                    case eIoAction::Read:
                    case eIoAction::Write:
                    case eIoAction::ReadWrite:
                    {
                        _notifier->updateEventAction(act);
                    }
                    break;

                    case eIoAction::Remove:
                    {
                        _notifier->delEvent(conn);
                        _connMap.erase(conn->getUniqueKey());
                    }
                    break;

                    default:
                    {
                        PARROT_ASSERT(false);
                    }
                    break;
                } // switch
            }     // for
        }         // while
    }
    catch (const std::system_error& e)
    {
        LOG_ERROR("FrontThread::run: Errno is " << e.code() << ". Meaning "
                                                << e.what());
        // What we can do here? Probably nothing but an assertion.
        PARROT_ASSERT(false);
    }
}
}
