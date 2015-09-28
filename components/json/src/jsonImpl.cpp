#include <rapidjson/pointer.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <iostream>
using namespace std;

#include "macroFuncs.h"
#include "jsonImpl.h"
#include "json.h"

namespace parrot
{
using rapidjson::Pointer;

JsonImpl::JsonImpl() : _isChild(false), _child(nullptr), _root(nullptr)
{
}

JsonImpl::JsonImpl(rapidjson::Document* root, rapidjson::Value* pv)
    : _isChild(true), _child(pv), _root(root)
{
}

JsonImpl::~JsonImpl()
{
    _child = nullptr;

    if (!_isChild)
    {
        delete _root;
        _root = nullptr;
    }
}

void JsonImpl::createRootObject()
{
    PARROT_ASSERT(!_isChild);
    if (_root)
    {
        delete _root;
    }
    _root = new rapidjson::Document();
    _root->SetObject();
}

void JsonImpl::createRootArray()
{
    PARROT_ASSERT(!_isChild);
    if (_root)
    {
        delete _root;
    }
    _root = new rapidjson::Document();
    _root->SetArray();
}

bool JsonImpl::parse(const char* buff, uint32_t len)
{
    PARROT_ASSERT(!_isChild);
    if (_root)
    {
        delete _root;
    }

    _root = new rapidjson::Document();
    _root->Parse(buff, len);
    if (_root->HasParseError())
    {
        return false;
    }

    return true;
}

void JsonImpl::getValue(const char* key, uint32_t& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetUint();
}

void JsonImpl::getValue(const char* key, int32_t& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetInt();
}

void JsonImpl::getValue(const char* key, uint64_t& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetUint64();
}

void JsonImpl::getValue(const char* key, int64_t& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetInt64();
}

void JsonImpl::getValue(const char* key, float& v)
{
    double d = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
                   ->GetDouble();
    v = static_cast<float>(d);
}

void JsonImpl::getValue(const char* key, double& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetDouble();
}

void JsonImpl::getValue(const char* key, std::string& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetString();
}

void JsonImpl::getValue(const char* key, bool& v)
{
    v = (_isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root))
            ->GetBool();
}

void JsonImpl::getValue(const char* key, std::unique_ptr<Json>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);
    v.reset(new Json(new JsonImpl(_root, t)));
}

void JsonImpl::getValue(const char* key, std::vector<uint32_t>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetUint());
    }
}

void JsonImpl::getValue(const char* key, std::vector<int32_t>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetInt());
    }
}

void JsonImpl::getValue(const char* key, std::vector<uint64_t>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetUint64());
    }
}

void JsonImpl::getValue(const char* key, std::vector<int64_t>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetInt64());
    }
}

void JsonImpl::getValue(const char* key, std::vector<float>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back(static_cast<float>((*t)[i].GetDouble()));
    }
}

void JsonImpl::getValue(const char* key, std::vector<double>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetDouble());
    }
}

void JsonImpl::getValue(const char* key, std::vector<std::string>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back((*t)[i].GetString());
    }
}

void JsonImpl::getValue(const char* key, std::vector<bool>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.push_back((*t)[i].GetBool());
    }
}

void JsonImpl::getValue(const char* key, std::vector<std::unique_ptr<Json>>& v)
{
    auto t = _isChild ? Pointer(key).Get(*_child) : Pointer(key).Get(*_root);

    for (auto i = 0u; i != t->Size(); ++i)
    {
        v.emplace_back(new Json(new JsonImpl(_root, &(*t)[i])));
    }
}

//////////////////////////////////////////////////////////////////////////
void JsonImpl::setValue(const char* key, const char* v)
{
    Pointer(key).Set(*_root, v);
}

void JsonImpl::setValue(const char* key, std::unique_ptr<Json>& v)
{
    Pointer(key).Set(*_root, *(v->_impl->getObject()));
}

void JsonImpl::setValue(const char* key, const std::vector<std::string>& v)
{
    rapidjson::Value val(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& allocator = _root->GetAllocator();

    for (auto it = v.begin(); it != v.end(); ++it)
    {
        val.PushBack(rapidjson::Value(it->c_str(), allocator), allocator);
    }

    Pointer(key).Set(*_root, val);
}

void JsonImpl::setValue(const char* key,
                        const std::vector<std::unique_ptr<Json>>& v)
{
    rapidjson::Value val(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& allocator = _root->GetAllocator();

    for (auto it = v.begin(); it != v.end(); ++it)
    {
        val.PushBack(*((*it)->_impl->getObject()), allocator);
    }

    Pointer(key).Set(*_root, val);
}

rapidjson::Value* JsonImpl::getObject()
{
    return _isChild ? _child : _root;
}

bool JsonImpl::containsKey(const char* key)
{
    return Pointer(key).Get(*getObject()) == nullptr ? false : true;
}

std::string JsonImpl::toString()
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    getObject()->Accept(writer);
    return sb.GetString();
}
}
