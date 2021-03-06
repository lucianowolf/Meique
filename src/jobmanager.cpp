/*
    This file is part of the Meique project
    Copyright (C) 2010-2013 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "jobmanager.h"
#include "logger.h"
#include "jobqueue.h"
#include "job.h"
#include <iomanip>
#include "mutexlocker.h"

JobManager::JobManager() : m_maxJobsRunning(1), m_jobsRunning(0), m_errorOccured(false)
{
    pthread_mutex_init(&m_jobsRunningMutex, 0);
    pthread_cond_init(&m_needJobsCond, 0);
    pthread_cond_init(&m_allDoneCond, 0);
}

JobManager::~JobManager()
{
    std::list<JobQueue*>::iterator it = m_queues.begin();
    for (; it != m_queues.end(); ++it)
        delete *it;
}

void JobManager::addJobQueue(JobQueue* queue)
{
    m_queues.push_back(queue);
}

void JobManager::printReportLine(const Job* job) const
{
    int perc = int((100 * m_jobsNotIdle) / m_jobCount);
    Manipulators color = NoColor;
    const char* text = "";

    switch (job->type()) {
    case Job::Compilation:
        color = Green;
        text = "Compiling ";
        break;
    case Job::Linking:
        color = Red;
        text = "Linking ";
        break;
    case Job::CustomTarget:
        color = Blue;
        text = "Running custom target function for ";
        break;
    }

    Notice() << '[' << perc << "%] " << color << text << job->name();
}

bool JobManager::processJobs()
{
    m_jobCount = 0;
    m_jobsNotIdle = 0;
    std::list<JobQueue*>::const_iterator it = m_queues.begin();
    for (; it != m_queues.end(); ++it)
        m_jobCount += (*it)->jobCount();

    m_jobsProcessed = 0;
    while (!m_errorOccured && m_jobsProcessed < m_jobCount) {
        // Select a valid queue
        std::list<JobQueue*>::iterator queueIt = m_queues.begin();
        JobQueue* queue = *queueIt;
        while (queue->hasShowStoppers() || queue->isEmpty()) {
            queue = *(++queueIt);
            if (queueIt == m_queues.end())
                goto finish;
        }

        MutexLocker locker(&m_jobsRunningMutex);
        if (m_jobsRunning >= m_maxJobsRunning)
            pthread_cond_wait(&m_needJobsCond, &m_jobsRunningMutex);

        if (m_errorOccured)
            break;

        // Now select a valid job
        if (Job* job = queue->getNextJob()) {
            job->addJobListenner(this);
            job->run();
            m_jobsRunning++;
            m_jobsNotIdle++;
            printReportLine(job);
        }
    }

    finish:

    MutexLocker locker(&m_jobsRunningMutex);
    if (m_jobsRunning) {
        Notice() << "Waiting for unfinished jobs...";
        pthread_cond_wait(&m_allDoneCond, &m_jobsRunningMutex);
    }

    return !m_errorOccured;
}

void JobManager::jobFinished(Job* job)
{
    MutexLocker locker(&m_jobsRunningMutex);
    m_jobsRunning--;
    m_jobsProcessed++;
    if (job->result())
        m_errorOccured = true;
    if (m_jobsRunning < m_maxJobsRunning)
        pthread_cond_signal(&m_needJobsCond);
    if (m_jobsRunning == 0)
        pthread_cond_signal(&m_allDoneCond);
}
