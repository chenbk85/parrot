#ifndef __BASE_SYS_INC_CONFIG_H__
#define __BASE_SYS_INC_CONFIG_H__

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include "urlParser.h"

namespace parrot
{
struct ServerInfo
{
    bool _isFront = true;

    // Server type. E.g.: room, area etc.
    std::string _serverType;

    // Server id, global unique. E.g.: room-srv-1, area-srv-1
    std::string _serverId;

    // Some cloud services use NAT, so the virtual machine
    // cann't 'listen' to a public IP. Instead, it can only
    // listen to a private IP. This field is used for this
    // situation.
    std::string _publicIp;

    // This IP/port is used to handle client's requests.
    std::string _frontIp;
    uint16_t _frontPort = 0;

    // This IP/port is used to handle rpc requests.
    std::string _rpcIp;
    uint16_t _rpcPort = 0;

    // WsURL. 
    std::string _frontWsUrl;
    std::unique_ptr<UrlInfo> _frontWsUrlInfo;
    std::string _rpcWsUrl;
    std::unique_ptr<UrlInfo> _rpcWsUrlInfo;
};

struct Config
{
    virtual ~Config() = default;

    // The group of process.
    std::string _group = "";
    // The user of process.
    std::string _user = "";
    // The lock file path.
    std::string _lockFilePath = "";

    uint32_t _frontThreadTimeout = 45;
    uint32_t _rpcThreadTimeout = 45;
    uint32_t _rpcReqTimeout = 5;
    
    // This server's info.
    ServerInfo _thisServer;
    // All neighbor servers' map.
    std::map<std::string, ServerInfo> _neighborSrvMap;

    // The front thread pool size.
    uint8_t _frontThreadPoolSize      = 2;
    uint32_t _frontThreadMaxConnCount = 100000;

    // Log configurations.

    // The log file path.
    std::string _logPath = "";
    // The name of the log.
    std::string _logName = "";
    // The max size of one log file.
    uint64_t _logSize = 104857600L; // 100MB.
    // The number of log rotation.
    uint32_t _rotateNum = 5;
    // 0: DEBUG, 1: INFO, 2: WARN, 3: ERROR, 4: FATAL
    uint8_t _logLevel = 0;

    // MySQL configurations.
    std::string _mysqlDBName     = "";
    std::string _mysqlUserName   = "";
    std::string _mysqlUserPassWd = "";

    // MongoDB configurations.
    std::string _mongoUrl = "";
};
}

#endif
