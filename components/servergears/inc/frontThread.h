#ifndef __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__
#define __COMPONENT_SERVERGEAR_INC_FRONTTHREAD_H__

namespace parrot
{
class FrontThread : public PoolThread
{
  public:
    FrontThread();

  public:
    void setConfig(const Config* cfg);
    void addConn();

  private:
    std::mutex _newConnListLock;
    std::list<std::shared_ptr<IoEvent>> _newConnList;

    std::unordered_map<uint64_t, std::shared_ptr<IoEvent>> _connMap;
    const Config* _config;
};
}

#endif
