#ifndef __BASE_SYS_INC_CONFIG_H__
#define __BASE_SYS_INC_CONFIG_H__

#include <string>

namespace parrot
{
    struct Config
    {
        virtual ~Config() = default;

        std::string              _serverType        = "";
        std::string              _serverId          = "";
        bool                     _frontServer       = false;

        std::string              _lockFilePath      = "";
    };
}
