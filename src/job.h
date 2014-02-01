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

#ifndef JOB_H
#define JOB_H
#include "basictypes.h"
#include <pthread.h>
#include <functional>

class Job
{
public:
    enum Type {
        Compilation,
        Linking,
        CustomTarget
    };

    Job();
    virtual ~Job() {}
    void run();
    void setName(const std::string& name) { m_name = name; }
    std::string name() const { return m_name; }
    int result() const { return m_result; }

    void setWorkingDirectory(const std::string& dir) { m_workingDir = dir; }
    std::string workingDirectory() { return m_workingDir; }

    std::function<void(Job*)> onFinished;

protected:
    virtual int doRun() = 0;
private:
    std::string m_name;
    pthread_t m_thread;
    int m_result;
    std::string m_workingDir;

    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    friend void* initJobThread(void*);
};

#endif // JOB_H
