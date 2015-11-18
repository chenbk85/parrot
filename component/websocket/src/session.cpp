#include "session.h"
#include <sstream>

namespace parrot
{
Session::Session():
    _isBound(false),
    _uid(),
    _frontSid(),
    _serverMap(),
    _jobHandlerPtr(nullptr),
    _frontThreadPtr(nullptr),
    _connUniqueId(0),
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
