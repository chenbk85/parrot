#include "jsonImpl.h"

namespace parrot
{
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

        // Currently we have to copy. Wait for the author to add new api to
        // avoid copying.
        string str(buff, len);
        _doc->pares(str.c_str());

        if (_doc->HasParseError())
        {
            return false;
        }

        return true;
    }

    void JsonImpl::getValue(const string &key, uint32_t &v)
    {
        if (_value)
        {
            rapidjson::Value &val = (*_value)[key];
            v = val.GetUint();
        }
        else
        {
            rapidjson::Value &val = (*_doc)[key];
            v = val.GetUint();
        }
    }

    void JsonImpl::getValue(const string &key, int32_t &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetInt();
    }

    void JsonImpl::getValue(const string &key, uint64_t &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetUint64();
    }

    void JsonImpl::getValue(const string &key, int64_t &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetInt64();
    }

    void JsonImpl::getValue(const string &key, float &v)
    {
        rapidjson::Value &val = _doc[key];
        v = static_cast<float>(val.GetDouble());
    }

    void JsonImpl::getValue(const string &key, double &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetDouble();
    }

    void JsonImpl::getValue(const string &key, string &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetString();
    }

    void JsonImpl::getValue(const string &key, bool &v)
    {
        rapidjson::Value &val = _doc[key];
        v = val.GetBool();
    }

    void JsonImpl::getValue(const string &key, std::unique_ptr<JsonImpl> &v)
    {
        rapidjson::value &val = _doc[key];
        v.reset(new JsonImpl(&val));
    }

    void JsonImpl::getValue(const std::string &key, std::vector<uint32_t> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetUint());
        }
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<int32_t> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetInt());
        }        
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<uint64_t> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetUint64());
        }        
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<int32_t> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetInt());
        }        
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<float> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(static_cast<float>(val[i].GetDouble()));
        }
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<double> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetDouble());
        }
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<std::string> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetString());
        }
    }

    void Jsonimpl::getValue(const std::string &key, std::vector<bool> &v)
    {
        rapidjson::Value &val = _doc[key];
        
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(val[i].GetBool());
        }
    }

    void Jsonimpl::getValue(const std::string &key, 
                            std::vector<std::unique_ptr<JsonImpl>> &v)
    {
        rapidjson::value &val = _doc[key];
        for (auto i = 0u; i < val.Size(); ++i)
        {
            v.emplace_back(std::unique_ptr<JsonImpl>(new JsonImpl(&val[i])));
        }
    }
}
