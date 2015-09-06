#ifndef __COMPONENT_JSON_INC_JSON_H__
#define __COMPONENT_JSON_INC_JSON_H__

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

// Client should only include this file if they want to use json.
// The reason why not use template to implement getValue and SetValue
// is that I don't want expose the jsonImpl.h file which includes
// the rapidjson header files.
namespace parrot
{
    class JsonImpl;
    class Json
    {
        friend class JsonImpl;
        
      public:
        Json();
        // Internal api.
        explicit Json(JsonImpl *impl);
        ~Json();
        Json(const Json &) = delete;
        Json & operator=(const Json &) = delete;

      public:
        void createRootObject();
        void createRootArray();
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
        void getValue(const char *key, std::unique_ptr<Json> &v);

        void getValue(const char *key, std::vector<uint32_t> &v);
        void getValue(const char *key, std::vector<int32_t> &v);
        void getValue(const char *key, std::vector<uint64_t> &v);
        void getValue(const char *key, std::vector<int64_t> &v);
        void getValue(const char *key, std::vector<float> &v);
        void getValue(const char *key, std::vector<double> &v);
        void getValue(const char *key, std::vector<std::string> &v);
        void getValue(const char *key, std::vector<bool> &v);
        void getValue(const char *key, 
                      std::vector<std::unique_ptr<Json>> &v);

        void setValue(const char *key, const uint32_t &v);
        void setValue(const char *key, const int32_t &v);
        void setValue(const char *key, const uint64_t &v);
        void setValue(const char *key, const int64_t &v);
        void setValue(const char *key, const float &v);
        void setValue(const char *key, const double &v);
        void setValue(const char *key, const char *v);
        void setValue(const char *key, const bool &v);
        void setValue(const char *key, std::unique_ptr<Json> &v);

        void setValue(const char *key, const std::vector<uint32_t> &v);
        void setValue(const char *key, const std::vector<int32_t> &v);
        void setValue(const char *key, const std::vector<uint64_t> &v);
        void setValue(const char *key, const std::vector<int64_t> &v);
        void setValue(const char *key, const std::vector<float> &v);
        void setValue(const char *key, const std::vector<double> &v);
        void setValue(const char *key, const std::vector<std::string> &v);
        void setValue(const char *key, const std::vector<bool> &v);
        void setValue(const char *key, 
                      std::vector<std::unique_ptr<Json>> &v);

        bool containsKey(const char *key);

        std::string toString();

      private:
        JsonImpl * _impl;
    };
}

#endif
