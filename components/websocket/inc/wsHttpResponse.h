#ifndef __COMPONENT_WEBSOCKET_INC_WSHTTPRESPONSE_H__
#define __COMPONENT_WEBSOCKET_INC_WSHTTPRESPONSE_H__

namespace parrot
{
    class WsHttpResponse
    {
        using HeaderDic = std::unordered_map<std::string, std::string>;
        enum class eParseState
        {
            Receving,
            Parsing
        };

      public:
        explicit WsHttpResponse(WsTranslayer &trans);

      public:
        Codes work();
        Codes getResult() const;
        
      private:
        eParseState                              _state;
        WsTranslayer &                           _trans;
        HeaderDic                                _headerDic;
        std::string                              _lastHeader;
        uint32_t                                 _lastParsePos;
        uint32_t                                 _httpBodyLen;

        Codes                                    _httpResult;
    };
}



#endif
