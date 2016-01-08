#ifndef __COMPONENT_SERVERGEAR_INC_JOBFACTORY_H__
#define __COMPONENT_SERVERGEAR_INC_JOBFACTORY_H__

#include <list>
#include <memory>
#include "jobHandler.h"
#include "job.h"

namespace parrot
{

template <typename T, typename J> class JobFactory
{
    using HdrJobListMap =
        std::unordered_map<JobHandler*, std::list<std::unique_ptr<Job>>>;

  public:
    JobFactory();

  public:
    // TODO: Add desc.
    void add(JobHandler* hdr, T&& t);

    // TODO: Add desc.
    void loadJobs(HdrJobListMap& hdrJobListMap);

    // TODO: Add desc.
    void loadJobsWithoutCreate(HdrJobListMap& hdrJobListMap);

  private:
    std::unordered_map<JobHandler*, std::list<T>> _hdrMap;
};

/////////////////////////
template <typename T, typename J> JobFactory<T, J>::JobFactory() : _hdrMap()
{
}

template <typename T, typename J>
void JobFactory<T, J>::add(JobHandler* hdr, T&& t)
{
    _hdrMap[hdr].emplace_back(std::forward<T>(t));
}

template <typename T, typename J>
void JobFactory<T, J>::loadJobs(HdrJobListMap& hdrJobListMap)
{
    for (auto& kv : _hdrMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        std::unique_ptr<J> job(new J());
        job->bind(std::move(kv.second));
        hdrJobListMap[kv.first].push_back(std::move(job));
    }
}

template <typename T, typename J>
void JobFactory<T, J>::loadJobsWithoutCreate(HdrJobListMap& hdrJobListMap)
{
    for (auto& kv : _hdrMap)
    {
        if (kv.second.empty())
        {
            continue;
        }

        auto& jobList = hdrJobListMap[kv.first];
        jobList.splice(jobList.end(), std::move(kv.second));
        kv.second.clear();
    }
}
};

#endif
