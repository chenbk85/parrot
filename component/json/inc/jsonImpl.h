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
    bool parse(const char* buff, uint32_t len);

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
    void setValue(const char* key, std::unique_ptr<Json>& v);

    template <typename T>
    void setValue(const char* key, const std::vector<T>& v);
    void setValue(const char* key, const std::vector<std::string>& v);
    void setValue(const char* key, const std::vector<std::unique_ptr<Json>>& v);

    rapidjson::Value* getObject();
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
