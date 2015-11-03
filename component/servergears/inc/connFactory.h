#ifndef __COMPONENT_SERVERGEAR_INC_CONNFACTORY_H__
#define __COMPONENT_SERVERGEAR_INC_CONNFACTORY_H__

#include <memory>

namespace parrot
{
struct Config;

template <typename Conn> class ConnFactory
{
  private:
    ConnFactory() : _config(nullptr)
    {
    }

  public:
    static ConnFactory* getInstance()
    {
        static ConnFactory factory;
        return &factory;
    }

  public:
    void setConfig(const Config* config);
    std::unique_ptr<Conn> create();

  private:
    const Config* _config;
};

template <typename Conn> void ConnFactory<Conn>::setConfig(const Config* config)
{
    _config = config;
}

template <typename Conn> std::unique_ptr<Conn> ConnFactory<Conn>::create()
{
    return std::unique_ptr<Conn>(new Conn(_config));
}
}

#endif
