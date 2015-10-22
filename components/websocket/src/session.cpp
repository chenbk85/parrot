#include "session.h"
#include <sstream>

namespace parrot
{
Session::Session():
    _isBound(false),
    _uid(),
    _frontSid(),
    _serverMap(),
    _pktHdr(),
    _backThreadPtr(nullptr),
    _frontThreadPtr(nullptr),
    _connUniqueId(nullptr),
    _ip()
{

}

std::string Session::toString() const
{
    std::ostringstream ostr;
    ostr << "[ Uid: " << _uid << ", frontSid: " << _frontSid
         << ", IP: " << _ip << " ]";

    return ostr.str();
}
}
