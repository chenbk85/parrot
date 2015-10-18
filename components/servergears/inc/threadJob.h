#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

namespace parrot
{
struct ThreadJob
{
    enum class JobType
    {
        Bind,
        Kick
    };

    std::function<void(bool, const std::string &)> _onBindCb;
    JobType _jobType;
};
}

#endif
