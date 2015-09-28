#include "jsonImpl.h"
#include "json.h"

#include <iostream>
using namespace std;

namespace parrot
{
Json::Json() : _impl(new JsonImpl())
{
}

Json::Json(JsonImpl* impl) : _impl(impl)
{
}

Json::~Json()
{
    delete _impl;
    _impl = nullptr;
}

void Json::createRootObject()
{
    _impl->createRootObject();
}

void Json::createRootArray()
{
    _impl->createRootArray();
}

bool Json::parse(const char* buff, uint32_t len)
{
    return _impl->parse(buff, len);
}

void Json::getValue(const char* key, uint32_t& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, int32_t& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, uint64_t& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, int64_t& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, float& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, double& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::string& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, bool& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::unique_ptr<Json>& v)
{
    _impl->getValue(key, v);
}

/////// for vector.
void Json::getValue(const char* key, std::vector<uint32_t>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<int32_t>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<uint64_t>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<int64_t>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<float>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<double>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<std::string>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<bool>& v)
{
    _impl->getValue(key, v);
}

void Json::getValue(const char* key, std::vector<std::unique_ptr<Json>>& v)
{
    _impl->getValue(key, v);
}

void Json::setValue(const char* key, const uint32_t& v)
{
    _impl->setValue(key, v);
}
void Json::setValue(const char* key, const int32_t& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const uint64_t& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const int64_t& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const float& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const double& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const char* v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const bool& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, std::unique_ptr<Json>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<uint32_t>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<int32_t>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<uint64_t>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<int64_t>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<float>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<double>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<std::string>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, const std::vector<bool>& v)
{
    _impl->setValue(key, v);
}

void Json::setValue(const char* key, std::vector<std::unique_ptr<Json>>& v)
{
    _impl->setValue(key, v);
}

bool Json::containsKey(const char* key)
{
    return _impl->containsKey(key);
}

std::string Json::toString()
{
    return _impl->toString();
}
}
