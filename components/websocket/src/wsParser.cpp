#include "wsTranslayer.h"
#include "wsParser.h"

namespace parrot
{
    WsParser::WsParser(WsTranslayer trans):
        _trans(trans)
    {
    }

    WsParser::~WsParser()
    {
        _trans = nullptr;
    }

    Codes WsParser::parseHandshake()
    {
    }

    Codes WsParser::parse()
    {
    }
}
