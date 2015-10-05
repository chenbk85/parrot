#ifndef __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_MAINTHREAD_H__

#include <string>
#include <cstdint>

namespace parrot
{
class Config;

class MainThread
{
  public:
    explicit MainThread(const Config& cfg);

  public:
    virtual void beforeStart();
    virtual void start();
    void frontListen(const string& ip, uint16_t port);
    void run();
    void stop();

  protected:
    const Config& _config;
};
}

#endif
