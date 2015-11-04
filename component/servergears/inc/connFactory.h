#ifndef __COMPONENT_SERVERGEAR_INC_CONNFACTORY_H__
#define __COMPONENT_SERVERGEAR_INC_CONNFACTORY_H__

#include <memory>

namespace parrot
{
template <typename Conn, typename Cfg> class ConnFactory
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
    void setConfig(const Cfg* config)
    {
        _config = config;
    }
    
    std::unique_ptr<Conn> create()
    {
        return std::unique_ptr<Conn>(new Conn(*_config));
    }

  private:
    const Cfg* _config;
};
}

#endif
