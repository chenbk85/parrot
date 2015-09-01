#ifndef __COMPONENT_JSON_INC_JSONIMPL_H__
#define __COMPONENT_JSON_INC_JSONIMPL_H__

#include <memory>
#include <string>
#include <cstdint>
#include <functional>

#include "rapidjson/pointer.h"

namespace parrot
{
    class JsonImpl
    {
      public:
        explicit JsonImpl(rapidjson::Value *pv = nullptr);
        ~JsonImpl();

      public:
        bool parse(const char * buff, uint32_t len);

        // getValue
        //
        // getValue from json. If first char of key is '/', it will be 
        // interpreted as json pointer.
        //
        // E.g., for json: 
        //  {
        //    "a": 1, 
        //    "b": "abc", 
        //    "c": ["x", "y"], 
        //    "d": {"e": "f", "g": [1, 2]}
        //  }
        //
        // getValue("a", v) will set v to 1.
        // getValue("b", v) will set v to "abc".
        // getValue("c", v) will set v to vector<string>{"x", "y"}.
        //
        // Json pointers:
        // getValue("/d/e", v) will set v to "f".
        // getValue("/d/g/1", v); will set v to 2.
        //
        void getValue(const std::string &key, uint32_t &v);
        void getValue(const std::string &key, int32_t &v);
        void getValue(const std::string &key, uint64_t &v);
        void getValue(const std::string &key, int64_t &v);
        void getValue(const std::string &key, float &v);
        void getValue(const std::string &key, double &v);
        void getValue(const std::string &key, std::string &v);
        void getValue(const std::string &key, bool &v);
        void getValue(const std::string &key, std::unique_ptr<Json> &v);

        void getValue(const std::string &key, std::vector<uint32_t> &v);
        void getValue(const std::string &key, std::vector<int32_t> &v);
        void getValue(const std::string &key, std::vector<uint64_t> &v);
        void getValue(const std::string &key, std::vector<int32_t> &v);
        void getValue(const std::string &key, std::vector<float> &v);
        void getValue(const std::string &key, std::vector<double> &v);
        void getValue(const std::string &key, std::vector<std::string> &v);
        void getValue(const std::string &key, std::vector<bool> &v);
        void getValue(const std::string &key, 
                      std::vector<std::unique_ptr<Json>> &v);

        void setValue(const std::string &key, uint32_t v);
        void setValue(const std::string &key, int32_t v);
        void setValue(const std::string &key, uint64_t v);
        void setValue(const std::string &key, int64_t v);
        void setValue(const std::string &key, float v);
        void setValue(const std::string &key, double v);
        void setValue(const std::string &key, std::string &v);
        void setValue(const std::string &key, bool v);
        void setValue(const std::string &key, std::unique_ptr<Json> &v);

        void setValue(const std::string &key, std::vector<uint32_t> &v);
        void setValue(const std::string &key, std::vector<int32_t> &v);
        void setValue(const std::string &key, std::vector<uint64_t> &v);
        void setValue(const std::string &key, std::vector<int32_t> &v);
        void setValue(const std::string &key, std::vector<float> &v);
        void setValue(const std::string &key, std::vector<double> &v);
        void setValue(const std::string &key, std::vector<std::string> &v);
        void setValue(const std::string &key, std::vector<bool> &v);
        void setValue(const std::string &key, 
                      std::vector<std::unique_ptr<Json>> &v);

        bool containsKey(const std::string &key);

        void foreach(
            std::function<void(const std::string &k, const JsonValue &v)> cb);

        void toString();

      private:
        rapidjson::Value*       _value;
        rapidjson::Document*    _doc;
    };
}

#endif
