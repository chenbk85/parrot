#ifndef __BASE_UTIL_INC_URLPARSER_H__
#define __BASE_UTIL_INC_URLPARSER_H__

#include <string>
#include <cstdint>
#include <memory>
#include <map>

namespace parrot
{
struct UrlInfo
{
    std::string _scheme                            = "";
    std::string _host                              = "";
    uint16_t _port                                 = 0;
    std::string _authority                         = "";
    std::string _path                              = "";
    std::multimap<std::string, std::string> _query = {};
    std::string _hash                              = "";
};

// Simple URL parser. It parses url like following.
//
// From RFC 3986:
//   foo://example.com:8042/over/there?name=ferret#nose
//   \_/   \______________/\_________/ \_________/ \__/
//    |           |            |            |        |
// scheme     authority       path        query   fragment
class UrlParser
{
  public:
    // This function simply finds the fields of UrlInfo from urlStr.
    //
    // Here are some limitations:
    // * It doesn't escape the URL.
    // * It will NOT parse URL like this: urn:example:animal:ferret:nose.
    static std::unique_ptr<UrlInfo> parse(const std::string& urlStr);

  private:
    static uint16_t getDefaultPortByScheme(const std::string& shceme);
    static void parseQueryString(UrlInfo* urlInfo, const std::string& queryStr);
};
}

#endif
