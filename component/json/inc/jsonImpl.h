#ifndef __COMPONENT_JSON_INC_JSONIMPL_H__
#define __COMPONENT_JSON_INC_JSONIMPL_H__

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

namespace parrot
{
class Json;
class JsonImpl
{
  public:
    JsonImpl();
    JsonImpl(rapidjson::Document* root, rapidjson::Value* pv);
    ~JsonImpl();

  public:
    void createRootObject();
    void createRootArray();
    bool parse(const char* buff, uint64_t len);

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

    template <typename T> void setValue(const char* key, const T& v);
    void setValue(const char* key, const char* v);
    void setValue(const char* key, const std::string &v);
    void setValue(const char* key, std::unique_ptr<Json>& v);

    template <typename T>
    void setValue(const char* key, const std::vector<T>& v);
    void setValue(const char* key, const std::vector<std::string>& v);
    void setValue(const char* key, const std::vector<std::unique_ptr<Json>>& v);

    rapidjson::Value* getObject();

    // From: http://rapidjson.org/md_doc_tutorial.html
    //
    // Note that, an integer value may be obtained in various ways without
    // conversion. For example, A value x containing 123 will make
    // x.IsInt() == x.IsUint() == x.IsInt64() == x.IsUint64() == true.
    // But a value y containing -3000000000 will only makes x.IsInt64() == true.
    // 
    // When obtaining the numeric values, GetDouble() will convert internal
    // integer representation to a double. Note that, int and unsigned can be
    // safely convert to double, but int64_t and uint64_t may lose precision
    // (since mantissa of double is only 52-bits).
    bool isUint32(const char* key);
    bool isUint64(const char* key);
    bool isInt32(const char* key);
    bool isInt64(const char* key);
    bool isDouble(const char* key);
    bool isNumber(const char* key);
    bool isString(const char* key);
    bool isObject(const char* key);
    bool isArray(const char* key);
    
    bool containsKey(const char* key);

    std::string toString();

  private:
    bool _isChild;
    rapidjson::Value* _child;
    rapidjson::Document* _root;
};

template <typename T> void JsonImpl::setValue(const char* key, const T& v)
{
    // Will allow ingeter bool, char, int, uint, long, ...
    // Will allow float, double, ...
    static_assert(std::is_integral<T>::value ||
                      std::is_floating_point<T>::value,
                  "Integer Required.");
    rapidjson::Pointer(key).Set(*_root, v);
}

template <typename T>
void JsonImpl::setValue(const char* key, const std::vector<T>& v)
{
    // Will allow ingeter bool, char, int, uint, long, ...
    // Will allow float, double, ...
    static_assert(std::is_integral<T>::value ||
                      std::is_floating_point<T>::value,
                  "Integer Required.");

    rapidjson::Value val(rapidjson::kArrayType);
    rapidjson::Document::AllocatorType& allocator = _root->GetAllocator();

    for (auto it = v.begin(); it != v.end(); ++it)
    {
        val.PushBack(*it, allocator);
    }

    rapidjson::Pointer(key).Set(*_root, val);
}
}

#endif
