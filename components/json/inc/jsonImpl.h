#ifndef __COMPONENT_JSON_INC_JSONIMPL_H__
#define __COMPONENT_JSON_INC_JSONIMPL_H__

#include <memory>
#include <string>
#include <vector>
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
        void getValue(const char *key, uint32_t &v);
        void getValue(const char *key, int32_t &v);
        void getValue(const char *key, uint64_t &v);
        void getValue(const char *key, int64_t &v);
        void getValue(const char *key, float &v);
        void getValue(const char *key, double &v);
        void getValue(const char *key, std::string &v);
        void getValue(const char *key, bool &v);
        void getValue(const char *key, std::unique_ptr<JsonImpl> &v);

        void getValue(const char *key, std::vector<uint32_t> &v);
        void getValue(const char *key, std::vector<int32_t> &v);
        void getValue(const char *key, std::vector<uint64_t> &v);
        void getValue(const char *key, std::vector<int64_t> &v);
        void getValue(const char *key, std::vector<float> &v);
        void getValue(const char *key, std::vector<double> &v);
        void getValue(const char *key, std::vector<std::string> &v);
        void getValue(const char *key, std::vector<bool> &v);
        void getValue(const char *key, 
                      std::vector<std::unique_ptr<JsonImpl>> &v);

        void setValue(const char *key, uint32_t v);
        void setValue(const char *key, int32_t v);
        void setValue(const char *key, uint64_t v);
        void setValue(const char *key, int64_t v);
        void setValue(const char *key, float v);
        void setValue(const char *key, double v);
        void setValue(const char *key, std::string &v);
        void setValue(const char *key, bool v);
        void setValue(const char *key, std::unique_ptr<JsonImpl> &v);

        void setValue(const char *key, std::vector<uint32_t> &v);
        void setValue(const char *key, std::vector<int32_t> &v);
        void setValue(const char *key, std::vector<uint64_t> &v);
        void setValue(const char *key, std::vector<int64_t> &v);
        void setValue(const char *key, std::vector<float> &v);
        void setValue(const char *key, std::vector<double> &v);
        void setValue(const char *key, std::vector<std::string> &v);
        void setValue(const char *key, std::vector<bool> &v);
        void setValue(const char *key, 
                      std::vector<std::unique_ptr<JsonImpl>> &v);

        bool containsKey(const char *key);

//        void foreach(
//            std::function<void(const std::string &k, const JsonValue &v)> cb);

        void toString();

      private:
        rapidjson::Value*       _value;
        rapidjson::Document*    _doc;
    };
}

#endif
