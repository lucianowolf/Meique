#include "jobqueue.h"
#include "basictypes.h"
#include "job.h"

JobQueue::~JobQueue()
{
    deleteAll(m_jobs);
}

void JobQueue::addJob(Job* job)
{
    m_jobs.push_back(job);
}

