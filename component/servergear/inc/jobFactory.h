#ifndef __COMPONENT_SERVERGEAR_INC_JOBFACTORY_H__
#define __COMPONENT_SERVERGEAR_INC_JOBFACTORY_H__

#include <list>
#include <memory>
#include "jobManager.h"
#include "job.h"

namespace parrot
{

/**
 * This template is used to 
 */
template <typename T, typename J> class JobFactory
{
    using JobMgrListMap =
        std::unordered_map<JobManager*, std::list<std::unique_ptr<Job>>>;

  public:
    JobFactory();

  public:
    // TODO: Add desc.
    void add(JobManager* hdr, T&& t);

    // TODO: Add desc.
    void loadJobs(JobMgrListMap& jobMgrListMap);

    // TODO: Add desc.
    void loadJobsWithoutCreate(JobMgrListMap& jobMgrListMap);

  private:
    std::unordered_map<JobManager*, std::list<T>> _jobMgrListMap;
};

/////////////////////////
template <typename T, typename J> JobFactory<T, J>::JobFactory() : _jobMgrListMap()
{
}

template <typename T, typename J>
void JobFactory<T, J>::add(JobManager* mgr, T&& t)
{
    _jobMgrListMap[mgr].emplace_back(std::forward<T>(t));
}

template <typename T, typename J>
void JobFactory<T, J>::loadJobs(JobMgrListMap& jobMgrListMap)
{
    for (auto& kv : _jobMgrListMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        std::unique_ptr<J> job(new J());
        job->bind(std::move(kv.second));
        jobMgrListMap[kv.first].push_back(std::move(job));
    }
}

template <typename T, typename J>
void JobFactory<T, J>::loadJobsWithoutCreate(JobMgrListMap& jobMgrListMap)
{
    for (auto& kv : _jobMgrListMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        auto& jobList = jobMgrListMap[kv.first];
        jobList.splice(jobList.end(), std::move(kv.second));
        kv.second.clear();
    }
}
}

#endif
