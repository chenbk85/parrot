#include "urlParser.h"
#include "stringHelper.h"
#include "macroFuncs.h"

namespace parrot
{
uint16_t UrlParser::getDefaultPortByScheme(const std::string& scheme)
{
    if (scheme == "http" || scheme == "ws")
    {
        return 80;
    }
    else if (scheme == "https" || scheme == "wss")
    {
        return 443;
    }
    else if (scheme == "ftp")
    {
        return 21;
    }
    else if (scheme == "sftp")
    {
        return 22;
    }
    else if (scheme == "mysql")
    {
        return 3306;
    }

    return 0;
}

void UrlParser::parseQueryString(UrlInfo* urlInfo, const std::string& queryStr)
{
    if (queryStr.empty())
    {
        return;
    }

    auto head = queryStr.begin();
    auto end  = head;
    std::string key;

    while (end != queryStr.end())
    {
        if (*end == '=')
        {
            key  = std::string(head, end);
            head = end + 1;

            if (head == queryStr.end())
            {
                PARROT_ASSERT(false);
            }
        }

        if (*end == '&')
        {
            if (key.empty())
            {
                PARROT_ASSERT(false);
            }
            urlInfo->_query.emplace(key, std::string(head, end));
            key  = "";
            head = end + 1;
        }

        ++end;
    }

    if (key.empty())
    {
        PARROT_ASSERT(false);
    }

    urlInfo->_query.emplace(key, std::string(head, end));
}

std::unique_ptr<UrlInfo> UrlParser::parse(const std::string& urlStr)
{
    auto head = urlStr.begin();
    auto tail = head;
    std::unique_ptr<UrlInfo> urlInfo(new UrlInfo());

    // Find scheme.
    while (tail != urlStr.end() && *tail != ':')
    {
        ++tail;
    }

    if (tail == urlStr.end())
    {
        // Bad url.
        urlInfo.reset(nullptr);
        return urlInfo;
    }

    urlInfo->_scheme = std::string(head, tail);
    strToLower(urlInfo->_scheme);

    if (*(tail + 1) != '/' && *(tail + 2) != '/')
    {
        // Bad url.
        urlInfo.reset(nullptr);
        return urlInfo;
    }

    // Go to host.
    tail += 3;
    head = tail;

    if (tail == urlStr.end())
    {
        // Bad url.
        urlInfo.reset(nullptr);
        return urlInfo;
    }

    // Find host.
    while (tail != urlStr.end() && *tail != ':' && *tail != '/')
    {
        ++tail;
    }

    urlInfo->_host      = std::string(head, tail);
    urlInfo->_authority = urlInfo->_host;

    if (tail == urlStr.end())
    {
        urlInfo->_port = getDefaultPortByScheme(urlInfo->_scheme);
        urlInfo->_path = "/";
        return urlInfo;
    }

    bool foundPort = false;
    if (*tail == ':')
    {
        // Find port.
        ++tail;
        head = tail;

        while (tail != urlStr.end() && *tail != '/')
        {
            ++tail;
        }

        urlInfo->_port = std::stoi(std::string(head, tail));
        foundPort = true;
        urlInfo->_authority += ":" + std::to_string(urlInfo->_port);
    }

    if (*tail == '/' && !foundPort)
    {
        urlInfo->_port = getDefaultPortByScheme(urlInfo->_scheme);
    }

    head = tail;
    if (++tail == urlStr.end())
    {
        urlInfo->_path = "/";
        return urlInfo;
    }

    // Find path.
    while (tail != urlStr.end() && *tail != '?' && *tail != '#')
    {
        ++tail;
    }

    if (*(tail - 1) == '/')
    {
        urlInfo->_path = std::string(head, tail - 1);
    }
    else
    {
        urlInfo->_path = std::string(head, tail);
    }

    if (tail == urlStr.end())
    {
        return urlInfo;
    }

    if (*tail == '?')
    {
        ++tail;
        head = tail;

        while (tail != urlStr.end() && *tail != '#')
        {
            ++tail;
        }

        // Parse query string.
        parseQueryString(urlInfo.get(), std::string(head, tail));
    }

    if (*tail == '#')
    {
        ++tail;
        urlInfo->_hash = std::string(tail, urlStr.end());
    }

    return urlInfo;
}
}
