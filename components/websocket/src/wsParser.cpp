#include <string>

#include "wsTranslayer.h"
#include "wsParser.h"
#include "macroFuncs.h"

namespace parrot
{
    WsParser::WsParser(WsTranslayer trans):
        _lastParsePos(0)
    {
    }

    void WsParser::parseHeader()
    {
        if (_payloadLen == 126)
        {
            
            _headerLen = _masked ? (2 + 2 + 4) : (2 + 2);
        }
        else if (_payloadLen == 127)
        {
            _headerLen = _masked ? (2 + 8 + 4) : (2 + 8);
        }
        else
        {
            _headerLen = _masked ? (2 + 4) : 2;
        }

    }

    eCodes WsParser::doParse()
    {
        switch (_state)
        {
            case eParseState::Begin:
            {
                if (_trans._recvVec.size() - _pktBeginPos < 2)
                {
                    return eCodes::ST_NeedRecv;
                }
                break;

                char firstChar = _trans._recvVec[_lastParsePos++];
                _fin = firstChar & (1 << 8);
                if ((firstChar) & (1 << 7) != 0 || 
                    (firstChar) & (1 << 6) != 0 ||
                    (firstChar) & (1 << 5) != 0)
                {
                    _parseResult = eCodes::WS_ProtocolError;
                    return eCodes::Complete;
                }

                char secondChar = _trans._recvVec[_lastParsePos++];
                _masked = secondChar & (1 << 8);
                _payloadLen = secondChar & 127u;

                if (_payloadLen == 126)
                {
                    _headerLen = _masked ? (2 + 2 + 4) : (2 + 2);
                }
                else if (_payloadLen == 127)
                {
                    _headerLen = _masked ? (2 + 8 + 4) : (2 + 8);
                }
                else
                {
                    _headerLen = _masked ? (2 + 4) : 2;
                }
                
                if (_trans._recvVec.size() - _pktBeginPos < _headerLen)
                {
                    _state = eParseState::RecevingHeader;
                    return eCodes::ST_NeedRecv;
                }

                parseHeader();
            }
            break;

            case eParseState::RecevingHeader:
            {

            }
            break;

            case eParseState::RecevingBody:
            {

            }
            break;
        }
    }

    eCodes WsParser::parse()
    {
        while (true)
        {
        
        }
    }

}
