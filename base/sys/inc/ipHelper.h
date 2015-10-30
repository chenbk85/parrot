#ifndef __BASE_SYS_INC_IPHELPER_H__
#define __BASE_SYS_INC_IPHELPER_H__

#include <string>
#include <list>

#include <netinet/ip.h>

namespace parrot
{
class IPHelper
{
  public:
    IPHelper();
    explicit IPHelper(const std::string &ip);

  public:
    void setIP(const std::string& ip);
    void setIP(const struct in6_addr& addr);
    void setIP(const struct in_addr& addr);

    bool isIPv4() const noexcept;
    bool isIPv6() const noexcept;

    struct in_addr getIPv4Bin() const noexcept;
    struct in6_addr getIPv6Bin() const noexcept;
    std::string getIPStr() const noexcept;

  public:
    static void getIPAddress(const std::string& domainName,
                             std::list<std::string>& ipv4List,
                             std::list<std::string>& ipv6List);

  private:
    int _version; // 4 IPv4, 6 IPv6.
    std::string _ip;
    struct in6_addr _addr6;
    struct in_addr _addr4;
};
}

#endif
