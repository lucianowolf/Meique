/*
    This file is part of the Meique project
    Copyright (C) 2010 Hugo Parente Lima <hugo.pl@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef JOBMANAGERQUEUE_H
#define JOBMANAGERQUEUE_H

#include <list>
#include <pthread.h>
#include <semaphore.h>

#include "jobqueue.h"
#include "joblistenner.h"

class Job;
class JobManager;

class JobManagerQueue : public JobQueue, private JobListenner
{
public:
    JobManagerQueue(JobManager* manager);
    ~JobManagerQueue();

    virtual void addJob(Job* job);
    Job* getNextJob();

    void unblock();
private:
    JobManager* m_manager;
    sem_t m_numJobsSemaphore;
    pthread_cond_t m_haveJobsCond;

    virtual void jobFinished(Job*);
};

#endif // JOBQUEUE_H
