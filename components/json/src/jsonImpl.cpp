#include "jsonImpl.h"
#include "rapidjson/pointer.h"

namespace parrot
{
    using rapidjson::Pointer;
    
    JsonImpl::JsonImpl(rapidjson::Value *pv):
        _value(pv),
        _doc(nullptr)
    {
    }

    JsonImpl::~JsonImpl()
    {
        _value = nullptr;

        delete _doc;
        _doc = nullptr;
    }

    bool JsonImpl::parse(const char *buff, uint32_t len)
    {
        if (_doc)
        {
            delete _doc;
        }

        _doc = new rapidjson::Document();
        _doc->Parse(buff, len);
        if (_doc->HasParseError())
        {
            return false;
        }

        return true;
    }

    void JsonImpl::getValue(const char *key, uint32_t &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetUint();
    }

    void JsonImpl::getValue(const char *key, int32_t &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetInt();
    }

    void JsonImpl::getValue(const char *key, uint64_t &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetUint64();
    }

    void JsonImpl::getValue(const char *key, int64_t &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetInt64();
    }

    void JsonImpl::getValue(const char *key, float &v)
    {
        double d =
            (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetDouble();
        v = static_cast<float>(d);
    }

    void JsonImpl::getValue(const char *key, double &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetDouble();        
    }

    void JsonImpl::getValue(const char *key, std::string &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetString();
    }

    void JsonImpl::getValue(const char *key, bool &v)
    {
        v = (_value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc))->
            GetBool();        
    }

    void JsonImpl::getValue(const char *key, std::unique_ptr<JsonImpl> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);
        v.reset(new JsonImpl(t));
    }

    void JsonImpl::getValue(const char *key, std::vector<uint32_t> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetUint());
        }
    }

    void JsonImpl::getValue(const char *key, std::vector<int32_t> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetInt());
        }        
    }

    void JsonImpl::getValue(const char *key, std::vector<uint64_t> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetUint64());
        }        
    }

    void JsonImpl::getValue(const char *key, std::vector<int64_t> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetInt64());
        }
    }

    void JsonImpl::getValue(const char *key, std::vector<float> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back(static_cast<float>((*t)[i].GetDouble()));
        }
    }

    void JsonImpl::getValue(const char *key, std::vector<double> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetDouble());
        }
    }

    void JsonImpl::getValue(const char *key, std::vector<std::string> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back((*t)[i].GetString());
        }
    }

    void JsonImpl::getValue(const char *key, std::vector<bool> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.push_back((*t)[i].GetBool());
        }
    }

    void JsonImpl::getValue(const char *key, 
                            std::vector<std::unique_ptr<JsonImpl>> &v)
    {
        auto t = _value ? Pointer(key).Get(*_value) : Pointer(key).Get(*_doc);

        for (auto i = 0u; i != t->Size(); ++i)
        {
            v.emplace_back(new JsonImpl(&(*t)[i]));
        }
    }
/*
    void JsonImpl::setValue(const char *key, uint32_t v)
    {

    }
    
    void JsonImpl::setValue(const char *key, int32_t v)
    {

    }
    
    void JsonImpl::setValue(const char *key, uint64_t v)
    {

    }
    
    void JsonImpl::setValue(const char *key, int64_t v)
    {

    }
    
    void JsonImpl::setValue(const char *key, float v)
    {

    }
    
    void JsonImpl::setValue(const char *key, double v)
    {
        
    }
    
    void JsonImpl::setValue(const char *key, std::string &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, bool v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::unique_ptr<JsonImpl> &v)
    {

    }

    void JsonImpl::setValue(const char *key, std::vector<uint32_t> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<int32_t> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<uint64_t> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<int32_t> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<float> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<double> &v);
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<std::string> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, std::vector<bool> &v)
    {

    }
    
    void JsonImpl::setValue(const char *key, 
                            std::vector<std::unique_ptr<JsonImpl>> &v)
    {

    }
*/
}
