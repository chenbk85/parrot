#include "json.h"
#include <iostream>
#include <string>
using namespace std;
using namespace parrot;

static std::string createJsonObject()
{
    unique_ptr<Json> json(new Json());
    json->createRootObject();
    json->setValue("/hello", "world");
    json->setValue("/year", 2015);
    json->setValue("/student", vector<string>{"Tom", "Hellen"});
    json->setValue("/apple/iphone/OS", "ios9");

    return json->toString();
}

static void parseObjectJson(const string &jsonStr)
{
    cout << jsonStr << endl;
    unique_ptr<Json> json(new Json());
    json->parse(jsonStr.c_str(), jsonStr.length());

    int value = 0;
    json->getValue("/year", value);

    cout << "Year is " << value << endl;
    vector<string> strVec;
    json->getValue("/student", strVec);

    for (auto &s : strVec)
    {
        cout << s << endl;
    }

    unique_ptr<Json> child;
    json->getValue("/apple", child);
    cout << child->toString() << endl;
}


static std::string createJsonArray()
{
    unique_ptr<Json> json(new Json());
    json->createRootArray();
    json->setValue("/0", "abc");
    json->setValue("/1", "cdf");

    return json->toString();
}

static void parseArrayJson(const string &jsonStr)
{
    cout << jsonStr << endl;
}

int main()
{
    string jsonStr = createJsonObject();
    parseObjectJson(jsonStr);

    jsonStr = createJsonArray();
    parseArrayJson(jsonStr);
}
