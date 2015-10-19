#ifndef __COMPONENT_SERVERGEAR_INC_THREADJOB_H__
#define __COMPONENT_SERVERGEAR_INC_THREADJOB_H__

#include <memory>

#include "sharedFunction.h"

namespace parrot
{
struct Session;

using ThreadJobBase = SharedFunction<Session, bool, std::shared_ptr<Session>>;

struct ThreadJob : public ThreadJobBase
{
    enum class JobType
    {
        Bind,
        Kick
    };

    ThreadJob(JobType jobType,
              std::shared_ptr<Session>& sp,
              std::function<void(bool&, std::shared_ptr<Session>&)>&& func)
        : ThreadJobBase(sp, std::move(func)), _jobType(jobType)
    {
    }

    JobType _jobType;
};
}

#endif
