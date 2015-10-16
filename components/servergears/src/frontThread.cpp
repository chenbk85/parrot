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

void FrontThread::registerAddPktCb(void* threadPtr, AddPktFunc&& func)
{
    _addPktFuncMap[threadPtr] = std::move(func);
}

void FrontThread::onPacket(void* threadPtr, std::unique_ptr<WsPacket>&& pkt)
{
    auto it = _threadPktMap.find(threadPtr);
    if (it == _threadPtrMap.end())
    {
        PARROT_ASSERT(false);
    }

    // Append packet to list.
    it->second.push_back(std::move(pkt));

    if (it->second.size() >= Constants::PKT_LIST_SIZE)
    {
        // Dispatch packetes.
        (_pktHandlerFuncMap[treadPtr])(it->second);
        (_pktHandlerFuncMap[treadPtr]).clear();
    }
}

void FrontThread::dispatchPackets()
{
    for (auto &kv : _threadPktMap)
    {
        if (!kv.second.empty())
        {
            (_pktHandlerFuncMap[kv.first])(kv.second);
            (_pktHandlerFuncMap[kv.first]).clear();
        }
    }
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

    using std::placeholders;
    auto onPacketCb = std::bind(FrontThread::onPacket, this, _1, _2);

    for (auto& c : tmpList)
    {
        c->setAction(c->getDefaultAction());
        // c->setRandom();
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

            eventNum = _notifier->waitIoEvents(-1);

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

            dispatchPackets();
        }         // while
    }
    catch (const std::system_error& e)
    {
        LOG_ERROR("FrontThread::run: Errno is " << e.code() << ". Meaning "
                                                << e.what());
        // There's nothing we can do here ...
        PARROT_ASSERT(false);
    }
}
}
