#include "json.h"
#include <iostream>
#include <string>
using namespace std;
using namespace parrot;

int main()
{
    unique_ptr<Json> json(new Json());
    json->createRootObject();

    string tmp = "world";
    json->setValue("/hello", 1);

    cout << json->toString() << endl;

    
    return 0;
}
