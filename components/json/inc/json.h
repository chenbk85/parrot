#ifndef __COMPONENT_JSON_INC_JSON_H__
#define __COMPONENT_JSON_INC_JSON_H__

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

// Client should only include this file if they want to use json.
// The reason why not use template to implement getValue and setValue
// is that I don't want expose the jsonImpl.h file which includes
// the rapidjson header files.
//
// Json class can only be moved, 'copy' is not supported.
namespace parrot
{
class JsonImpl;
class Json
{
    friend class JsonImpl;

  public:
    Json();
    explicit Json(JsonImpl* impl); // Internal api.
    ~Json();

    Json(const Json&) = delete;
    Json& operator=(const Json&) = delete;

    // If we have self defined destructor, there will be no
    // default move constructor nor move operator=.
    Json(Json&& json) = default;
    Json& operator=(Json&&) = default;

  public:
    // createRootObject
    //
    // Init a json, the root will be an object. toString() returns: {}
    void createRootObject();

    // createRootArray
    //
    // Init a json. the root will be an array. toString() returns: []
    void createRootArray();

    // parse
    //
    // parse string to json.
    //
    // Param:
    // * buff   The buffer pointer.
    // * len    The length of the buffer.
    //
    // Return
    //  True if succesfully parsed the buffer.
    bool parse(const char* buff, uint32_t len);

    // getValue
    //
    // getValue from json.
    //
    // E.g., for json:
    //  {
    //    "a": 1,
    //    "b": "abc",
    //    "c": ["x", "y"],
    //    "d": {"e": "f", "g": [1, 2]}
    //  }
    //
    // getValue("/a", v) will set v to 1.
    // getValue("/b", v) will set v to "abc".
    // getValue("/c", v) will set v to vector<string>{"x", "y"}.
    // getValue("/d/e", v) will set v to "f".
    // getValue("/d/g/1", v); will set v to 2.
    void getValue(const char* key, uint32_t& v);
    void getValue(const char* key, int32_t& v);
    void getValue(const char* key, uint64_t& v);
    void getValue(const char* key, int64_t& v);
    void getValue(const char* key, float& v);
    void getValue(const char* key, double& v);
    void getValue(const char* key, std::string& v);
    void getValue(const char* key, bool& v);
    void getValue(const char* key, std::unique_ptr<Json>& v);

    void getValue(const char* key, std::vector<uint32_t>& v);
    void getValue(const char* key, std::vector<int32_t>& v);
    void getValue(const char* key, std::vector<uint64_t>& v);
    void getValue(const char* key, std::vector<int64_t>& v);
    void getValue(const char* key, std::vector<float>& v);
    void getValue(const char* key, std::vector<double>& v);
    void getValue(const char* key, std::vector<std::string>& v);
    void getValue(const char* key, std::vector<bool>& v);
    void getValue(const char* key, std::vector<std::unique_ptr<Json>>& v);

    // setValue
    //
    // setValue to json.
    // E.g.1, for json {}
    //
    // setValue("/a", 1)
    // setValue("/b", "abc")
    // setValue("/c", vector<string>{"x", "y"})
    // setValue("/d/e", "f")
    // setValue("/d/g", vetor<int>{1, 2})
    //
    // After above calls, the json will be:
    // {"a":1,"b":"abc","c":["x","y"],"d":{"e":"f","g":[1,2]}}
    //
    //
    // E.g.2, for json []
    // setValue("/0", 1)
    // setValue("/1", 2)
    //
    // After the calls, json will be: [1, 2].
    void setValue(const char* key, const uint32_t& v);
    void setValue(const char* key, const int32_t& v);
    void setValue(const char* key, const uint64_t& v);
    void setValue(const char* key, const int64_t& v);
    void setValue(const char* key, const float& v);
    void setValue(const char* key, const double& v);
    void setValue(const char* key, const char* v);
    void setValue(const char* key, const bool& v);
    void setValue(const char* key, std::unique_ptr<Json>& v);

    void setValue(const char* key, const std::vector<uint32_t>& v);
    void setValue(const char* key, const std::vector<int32_t>& v);
    void setValue(const char* key, const std::vector<uint64_t>& v);
    void setValue(const char* key, const std::vector<int64_t>& v);
    void setValue(const char* key, const std::vector<float>& v);
    void setValue(const char* key, const std::vector<double>& v);
    void setValue(const char* key, const std::vector<std::string>& v);
    void setValue(const char* key, const std::vector<bool>& v);
    void setValue(const char* key, std::vector<std::unique_ptr<Json>>& v);

    // containsKey
    //
    // Checks wether the json contains key.
    //
    // Usage: containsKey("/abc/ddd");
    //
    // Param:
    // * key   The key needs to be checked.
    //
    // Return
    //  True if json contains the key.
    bool containsKey(const char* key);

    // toString
    //
    // Converts the underline json implement to string.
    //
    // Return
    //  The json string.
    std::string toString();

  private:
    JsonImpl* _impl;
};
}

#endif
