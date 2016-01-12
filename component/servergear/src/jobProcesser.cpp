#include "jobProcesser.h"
#include "jobManager.h"

namespace parrot
{
void JobProcesser::addJob(std::list<std::unique_ptr<Job>>&& jobList)
{
    _jobList = std::move(jobList);
}

void JobProcesser::dispatchJobs()
{
    loadJobs();
    
    for (auto& kv : _jobMgrListMap)
    {
        if (!kv.second.empty())
        {
            (kv.first)->addJob(kv.second);
            kv.second.clear();
        }
    }

    _jobList.clear();
}
}
