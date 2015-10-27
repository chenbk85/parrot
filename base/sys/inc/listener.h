#ifndef __BASE_SYS_INC_LISTENER_H__
#define __BASE_SYS_INC_LISTENER_H__

#include <cstdint>
#include <string>
#include <list>
#include <vector>

#include "ipHelper.h"
#include "ioEvent.h"
#include "sysDefinitions.h"

namespace parrot
{

template <typename Conn, template <typename> typename ConnFactory>
class Listener : public IoEvent
{
  public:
    Listener(uint16_t listenPort, const std::string& listenIp);
    Listener(const Listener&) = delete;
    Listener& operator=(const Listener&) = delete;

  public:
    void setJobHandler(std::vector<JobHandler*>&& jobHandler);

    void setJobHandler(std::vector<JobHandler*>& jobHandler);

    eIoAction handleIoEvent() override;

    void startListen();

  private:
    eCodes doAccept(int& cliFd, IPHelper& cliIP, uint16_t& cliPort);

  private:
    std::vector<JobHandler*> _jobHandlerVec;
    uint32_t _jobHandlerVecIdx;
    uint64_t _connUniqueIdIdx;
    IPHelper _listenIp;
    uint16_t _listenPort;
};

template <typename Conn, template <typename> typename ConnFactory>
Listener<ConnFactory>::Listener(uint16_t listenPort,
                                const std::string& listenIp)
    : _jobHandlerVec(),
      _jobHandlerVecIdx(0),
      _connUniqueIdIdx(0),
      _listenIp(listenIp),
      _listenPort(listenPort)
{
    PARROT_ASSERT(_listenPort != 0);
}

template <typename Conn, template <typename> typename ConnFactory>
void Listener<ConnFactory>::setJobHandler(std::vector<JobHandler*>&& jobHandler)
{
    _jobHandlerVec = std::move(jobhandler);
}

template <typename Conn, template <typename> typename ConnFactory>
void Listener<ConnFactory>::setJobHandler(std::vector<JobHandler*>& jobHandler)
{
    _jobHandlerVec = jobhandler;
}

template <typename Conn, template <typename> typename ConnFactory>
eIoAction Listener<ConnFactory>::handleIoEvent() override
{
    eIoAction act = getNotifiedAction();

    if (act == eIoAction::Remove)
    {
        return act;
    }

    PARROT_ASSERT(act == eIoAction::Read);

    eCodes code   = ST_Init;
    int fd        = -1;
    uint16_t port = 0;
    IPHelper ipHelper;

    std::list<std::unique_ptr<Conn>> connList;
    std::unique_ptr<Conn> conn;

    do
    {
        code = doAccept(fd, ipHelper, port);
        
        conn = std::move(ConnFactory::getInstance()->create()));
        conn->setFd(fd);
        conn->setNextAction(eIoAction::Read);
        conn->setRemoteAddr(ipHelper.getIpStr());
        conn->setUniqueKey(_connUniqueIdIdx++);
        connList.push_back(std::move(conn));
    } while (code == eCodes::ST_Ok);

    _jobHandlerVec[_jobHandlerVecIdx]->addJob(std::move(connList));
    _jobHandlerVecIdx = (_jobHandlerVecIdx + 1) % _jobHandlerVec.size();
    
    return eIoAction::Read;
}

template <typename Conn, template <typename> typename ConnFactory>
void Listener<ConnFactory>::startListen()
{
    socklen_t addrLen;
    struct sockaddr* addr;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;

    int fd = 0;

    if (_listenIp.isIPv6())
    {
        std::memset(&addr6, 0, sizeof(struct sockaddr_in6));
        addrLen           = sizeof(struct sockaddr_in6);
        addr              = (struct sockaddr*)&addr6;
        addr6.sin6_family = AF_INET6;
        addr6.sin6_port   = uniHtons(_listenPort);

        auto addr6Bin = _listenIp.getIPv6Bin();
        std::memcpy(&addr6.sin6_addr, &addr6Bin, addrLen);

        // Create socket for IPv6.
        fd = ::socket(AF_INET6, SOCK_STREAM, 0);
    }
    else
    {
        std::memset(&addr4, 0, sizeof(struct sockaddr_in));
        addrLen               = sizeof(struct sockaddr_in);
        addr                  = (struct sockaddr*)&addr4;
        addr4.sin_family      = AF_INET;
        addr4.sin_port        = uniHtons(_listenPort);
        addr4.sin_addr.s_addr = _listenIp.getIPv4Bin().s_addr;

        // Create socket for IPv4
        fd = ::socket(AF_INET, SOCK_STREAM, 0);
    }

    if (fd == -1)
    {
        throw std::system_error(errno, std::system_category(),
                                "Listener::startListen: create socket");
    }

    int ret = 0;

    try
    {
        // Set reuse addr before bind. Or it won't work.
        // On linux or mac, we can reuse addr. but on windows, we
        // cannot.
        setReuseAddr(fd);

        // Close fd when any exec-family functions succeeded.
        ret = ::fcntl(fd, F_SETFD, FD_CLOEXEC);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: fcntl");
        }

        // Bind.
        ret = ::bind(fd, addr, addrLen);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: bind");
        }

        // Listen.
        ret = ::listen(fd, 10000);
        if (ret == -1)
        {
            throw std::system_error(errno, std::system_category(),
                                    "Listener::startListen: listen");
        }

        // Enable non blocking.
        setNonBlock(fd, true);

        // Disable Nagle's algorithm.
        setNoDelay(fd);

        // Finally set the fd.
        setFd(fd);
    }
    catch (...)
    {
        if (fd != -1)
        {
            ::close(fd);
        }
        throw;
    }

    setNextAction(eIoAction::Read);
}

template <typename Conn, template <typename> typename ConnFactory>
eCodes
Listener<ConnFactory>::doAccept(int& cliFd, IPHelper& cliIP, uint16_t& cliPort)
{
    struct sockaddr_in6 cliAddr6;
    struct sockaddr* cliAddr = reinterpret_cast<struct sockaddr*>(&cliAddr6);
    socklen_t cliAddrLen     = sizeof(struct sockaddr_in6);

    // IPv4 also work.
    cliFd = ::accept(getFd(), cliAddr, &cliAddrLen);
    if (cliFd == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            return eCodes::ST_RetryLater;
        }

        throw std::system_error(errno, std::system_category(),
                                "Listener::doAccept");
    }

    if (AF_INET == cliAddr->sa_family)
    {
        struct sockaddr_in* cliAddr4 =
            reinterpret_cast<struct sockaddr_in*>(&cliAddr6);
        cliIP.setIP(cliAddr4->sin_addr);
        cliPort = cliAddr4->sin_port;
    }
    else
    {
        cliIP.setIP(cliAddr6.sin6_addr);
        cliPort = cliAddr6.sin6_port;
    }

    return eCodes::ST_Ok;
}
}

#endif
