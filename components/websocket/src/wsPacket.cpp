#include "wsPacket.h"

namespace parrot
{
    WsParser::WsParser():
    {
    }

    WsParser::~WsParser()
    {

    }

    eCodes WsParser::parse()
    {
        if (_trans._recvVec.size() - _lastParsePos < 2)
        {
            return ST_NeedRecv;
        }
    }
}

