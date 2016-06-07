#ifndef __BASE_SYS_INC_IPHELPER_H__
#define __BASE_SYS_INC_IPHELPER_H__

#include <list>
#include <string>

#include <netinet/ip.h>

namespace parrot
{
class IPHelper
{
  public:
    IPHelper();
    explicit IPHelper(const std::string& ip);

  public:
    /**
     * Set IP address. E.g.:
     * (1) 192.168.1.1,
     * (2) ::FFFF:192.168.1.1,
     * (3) 2001:0db8:85a3:0000:0000:8a2e:0370:7334.
     *
     * @param  ip            The ipv4 address or ipv6 address.
     */
    void setIP(const std::string& ip);

    /**
     * Set IPv6 address in binary form.
     *
     * @param  addr             The IPv6 addr.
     */
    void setIP(const struct in6_addr& addr);

    /**
     * Set IPv4 address in binary form.
     *
     * @param  addr             The IPv4 addr.
     */
    void setIP(const struct in_addr& addr);

    /**
     * Check whether the IP is the IPv4 address .
     *
     * @return      True if IP is IPv4. Otherwise false.
     */
    bool isIPv4() const noexcept;

    /**
     * Check whether the IP is the IPv6 address .
     *
     * @return      True if IP is IPv6. Otherwise false.
     */
    bool isIPv6() const noexcept;

    /**
     * Get binary form IPv4 address.
     *
     * @return      The binary form IPv4 address.
     */
    struct in_addr  getIPv4Bin() const noexcept;

    /**
     * Get binary form IPv6 address.
     *
     * @return      The binary form IPv6 address.
     */
    struct in6_addr getIPv6Bin() const noexcept;

    /**
     * Get text form IP address.
     *
     * @return      The binary form IP address.
     */
    std::string     getIPStr() const noexcept;

  public:

    /**
     * Get IP addresses from domain name.
     *
     * @param   domainName    Domain name.
     * @param   ipv4List      The IPv4 addresses mapped to the domain name.
     * @param   ipv6List      The IPv6 addresses mapped to the domain name.
     */
    static void getIPAddress(const std::string&      domainName,
                             std::list<std::string>& ipv4List,
                             std::list<std::string>& ipv6List);

  private:
    int             _version; ///< 4 IPv4, 6 IPv6.
    std::string     _ip;
    struct in6_addr _addr6;
    struct in_addr  _addr4;
};
}

#endif
