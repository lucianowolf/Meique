#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <list>

class Job;

class JobQueue {
public:
    JobQueue() {}
    virtual ~JobQueue();
    virtual void addJob(Job* job);
    bool isEmpty() const { return m_jobs.empty(); }

protected:
    std::list<Job*> m_jobs;

private:
    JobQueue(const JobQueue&);
    JobQueue& operator=(const JobQueue&);
};

#endif
