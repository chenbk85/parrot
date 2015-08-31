#ifndef __COMPONENT_JSON_INC_JSONIMPL_H__
#define __COMPONENT_JSON_INC_JSONIMPL_H__

#include <memory>

#include "rapidjson/pointer.h"
#include ""

namespace parrot
{
    class JsonImpl
    {
      public:
        JsonImpl();
        ~JsonImpl();

      public:
        static std::unique_ptr<JsonImpl> parse(const char * buff, uint32_t len);

      public:
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
        void getValue(const string &key, uint32_t &v);
        void getValue(const string &key, int32_t &v);
        void getValue(const string &key, uint64_t &v);
        void getValue(const string &key, int64_t &v);
        void getValue(const string &key, float &v);
        void getValue(const string &key, double &v);
        void getValue(const string &key, string &v);
        void getValue(const string &key, bool &v);
        void getValue(const string &key, std::unique_ptr<Json> &v);

        void getValue(const string &key, std::vector<uint32_t> &v);
        void getValue(const string &key, std::vector<int32_t> &v);
        void getValue(const string &key, std::vector<uint64_t> &v);
        void getValue(const string &key, std::vector<int32_t> &v);
        void getValue(const string &key, std::vector<float> &v);
        void getValue(const string &key, std::vector<double> &v);
        void getValue(const string &key, std::vector<string> &v);
        void getValue(const string &key, std::vector<bool> &v);
        void getValue(const string &key, std::vector<std::unique_ptr<Json>> &v);

        void setValue(const string &key, uint32_t v);
        void setValue(const string &key, int32_t v);
        void setValue(const string &key, uint64_t v);
        void setValue(const string &key, int64_t v);
        void setValue(const string &key, float v);
        void setValue(const string &key, double v);
        void setValue(const string &key, string &v);
        void setValue(const string &key, bool v);
        void setValue(const string &key, std::unique_ptr<Json> &v);

        void setValue(const string &key, std::vector<uint32_t> &v);
        void setValue(const string &key, std::vector<int32_t> &v);
        void setValue(const string &key, std::vector<uint64_t> &v);
        void setValue(const string &key, std::vector<int32_t> &v);
        void setValue(const string &key, std::vector<float> &v);
        void setValue(const string &key, std::vector<double> &v);
        void setValue(const string &key, std::vector<string> &v);
        void setValue(const string &key, std::vector<bool> &v);
        void setValue(const string &key, std::vector<std::unique_ptr<Json>> &v);

        void toString();
    };
}

#endif
