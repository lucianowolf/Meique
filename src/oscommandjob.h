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

#ifndef OSCOMMANDJOB_H
#define OSCOMMANDJOB_H

#include "job.h"
#include "os.h"

class OSCommandJob : public Job
{
public:
    OSCommandJob(Job* parent = 0);
    void setCommand(const OS::Command& command);
    void setWorkingDirectory(const std::string& dir) { m_workingDir = dir; }
    std::string workingDirectory() { return m_workingDir; }

protected:
    virtual int doRun();
private:
    OS::Command m_command;
    std::string m_workingDir;
};

#endif // OSCOMMANDJOB_H
