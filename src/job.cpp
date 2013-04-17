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

#include "job.h"
#include "joblistenner.h"
#include "mutexlocker.h"

Job::Job(Job *parent)
    : m_parent(parent)
    , m_childCount(0)
    , m_type(Compilation)
    , m_result(0)
{
    if (m_parent) {
        pthread_mutex_init(&m_childCountMutex, 0);
        m_parent->m_childCount++;
    }
}

void Job::detachFromParent()
{
    if (m_parent) {
        MutexLocker locker(&m_childCountMutex);
        m_parent->m_childCount--;
    }
}

void* initJobThread(void* ptr)
{
    Job* job = reinterpret_cast<Job*>(ptr);

    job->m_result = job->doRun();

    std::list<JobListenner*>::iterator it = job->m_listenners.begin();
    for (; it != job->m_listenners.end(); ++it)
        (*it)->jobFinished(job);

    return 0;
}

void Job::run()
{
    pthread_create(&m_thread, 0, initJobThread, this);
}

void Job::addJobListenner(JobListenner* listenner)
{
    m_listenners.push_back(listenner);
}
