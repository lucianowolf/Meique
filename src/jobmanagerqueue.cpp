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

#include "jobmanagerqueue.h"
#include "jobmanager.h"
#include "job.h"
#include "logger.h"

JobManagerQueue::JobManagerQueue(JobManager* manager)
    : m_manager(manager)
{
    sem_init(&m_numJobsSemaphore, 0, 0);
    pthread_cond_init(&m_haveJobsCond, 0);
}

JobManagerQueue::~JobManagerQueue()
{
    deleteAll(m_jobs);
    sem_destroy(&m_numJobsSemaphore);
    pthread_cond_destroy(&m_haveJobsCond);
}

void JobManagerQueue::addJob(Job* job)
{
    printf("PUSH JOB %s on %p\n", job->name().c_str(), this);
    job->addJobListenner(this);
    m_jobs.push_back(job);
    m_manager->increaseJobCount();
    sem_post(&m_numJobsSemaphore);
    pthread_cond_signal(&m_haveJobsCond);
}

void JobManagerQueue::unblock()
{
    printf("UNBLOCK on %p\n", this);
    sem_post(&m_numJobsSemaphore);
}

void JobManagerQueue::jobFinished(Job* job)
{
    job->detachFromParent();
    pthread_cond_signal(&m_haveJobsCond);
}

Job* JobManagerQueue::getNextJob()
{
    if (!m_manager->isFinishing())
        sem_wait(&m_numJobsSemaphore);

    if (m_jobs.empty())
        return 0;

    std::list<Job*>::iterator it = m_jobs.begin();
    while ((*it)->hasChildren()) {
        ++it;

        if (it == m_jobs.end()) {
            pthread_mutex_t mutex;
            pthread_mutex_init(&mutex, 0);
            Warn() << (*m_jobs.begin())->name() << " BLOCKING!";
            pthread_cond_wait(&m_haveJobsCond, &mutex);
            pthread_mutex_destroy(&mutex);

            it = m_jobs.begin();
            puts("LETS TRY AGAIN!");
        }
    }
    printf("POP JOB %s on %p\n", (*it)->name().c_str(), this);
    Job* job = *it;
    m_jobs.erase(it);
    return job;
}
