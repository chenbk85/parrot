#ifndef __BASE_SYS_INC_CONFIG_H__
#define __BASE_SYS_INC_CONFIG_H__

#include <string>
#include <cstdint>

namespace parrot
{
    struct Config
    {
        virtual ~Config() = default;

        std::string              _group             = "";
        std::string              _user              = "";

        std::string              _serverType        = "";
        std::string              _serverId          = "";
        bool                     _frontServer       = false;

        std::string              _lockFilePath      = "";

        // Log configurations.
        std::string              _logPath           = "";
        std::string              _logName           = "";
        uint64_t                 _logSize           = 104857600L; // 100MB.
        uint32_t                 _rotateNum         = 1;
        // 0 means log everything.
        uint8_t                  _logLevel          = 0;

        // MySQL configurations.
        std::string              _mysqlDBName       = "";
        std::string              _mysqlUserName     = "";
        std::string              _mysqlUserPassWd   = "";

        // MongoDB configurations.
        std::string              _mongoUrl          = "";
    };
}
