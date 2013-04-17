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

void* initThread(void* ptr)
{
    reinterpret_cast<JobManager*>(ptr)->mainLoop();
    return 0;
}

JobManager::JobManager()
    : m_queue(this)
    , m_finishing(false)
    , m_maxJobsRunning(1)
    , m_jobsRunning(0)
    , m_jobsProcessed(0)
    , m_jobCount(0)
    , m_errorOccured(false)
{

    pthread_mutex_init(&m_jobsRunningMutex, 0);
    pthread_cond_init(&m_allDoneCond, 0);
    pthread_create(&m_thread, 0, initThread, this);

}

JobManager::~JobManager()
{
    sem_destroy(&m_runJobsSemaphore);
}

bool JobManager::finish()
{
    m_finishing = true;
    Warn() << "Finish!";
    if (m_queue.isEmpty())
        m_queue.unblock();
    pthread_join(m_thread, 0);
    Warn() << "Finish for real!";
    return !m_errorOccured;
}

JobQueue* JobManager::queue()
{
    return &m_queue;
}

void JobManager::mainLoop()
{
    Warn() << "MAIN LOOP START";
    sem_init(&m_runJobsSemaphore, 0, m_maxJobsRunning);

    Job* job;
    while (job = m_queue.getNextJob()) {
        {
            MutexLocker locker(&m_jobsRunningMutex);
            m_jobsRunning++;
            m_jobsProcessed++;
        }

        job->addJobListenner(this);
        job->run();
        printReportLine(job);
        sem_wait(&m_runJobsSemaphore);
        if (m_errorOccured)
            break;
    }

    MutexLocker locker(&m_jobsRunningMutex);
    if (m_jobsRunning) {
        Notice() << "Waiting for " << m_jobsRunning << " unfinished jobs...";
        pthread_cond_wait(&m_allDoneCond, &m_jobsRunningMutex);
    }

    Warn() << "MAIN LOOP END";
}

void JobManager::printReportLine(const Job* job) const
{
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

    Notice() << '[' << m_jobsProcessed << '/' << m_jobCount << "] " << color << text << job->name();
}

void JobManager::jobFinished(Job* job)
{
    if (job->result())
        m_errorOccured = true;

    sem_post(&m_runJobsSemaphore);
    Warn() << "end job " << job->name();

    MutexLocker locker(&m_jobsRunningMutex);
    m_jobsRunning--;
    if (m_jobsRunning == 0)
        pthread_cond_signal(&m_allDoneCond);
}
