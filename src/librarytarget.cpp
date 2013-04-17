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

#include "librarytarget.h"
#include "os.h"
#include "jobqueue.h"
#include "compiler.h"
#include "oscommandjob.h"
#include "linkeroptions.h"
#include "luacpputil.h"
#include "logger.h"
#include "compileroptions.h"
#include "meiquecache.h"
#include "meiquescript.h"

LibraryTarget::LibraryTarget(const std::string& targetName, MeiqueScript* script): CompilableTarget(targetName, script)
{
    std::string outputFileName;
    // get link type
    getLuaField("_libType");
    int libType = lua_tocpp<int>(luaState(), -1);
    lua_pop(luaState(), 1);
    switch (libType) {
        case 1:
            m_linkType = LinkerOptions::SharedLibrary;
            outputFileName = cache()->compiler()->nameForSharedLibrary(name());
            break;
        case 2:
            m_linkType = LinkerOptions::StaticLibrary;
            outputFileName = cache()->compiler()->nameForStaticLibrary(name());
            break;
        default:
            throw Error("Unknown library type! " + libType);
    }
    setOutputFileName(outputFileName);
}

bool LibraryTarget::doEmitJobs(JobQueue* queue)
{
    StringList objects;
    Compiler* compiler = cache()->compiler();
    OSCommandJob* linkJob = new OSCommandJob;

    createCompilationJobs(linkJob, compiler, queue, &objects);

    if (objects.empty() || !OS::fileExists(outputFileName())) {
        linkJob->setWorkingDirectory(OS::pwd());
        linkJob->setName(outputFileName());
        linkJob->setCommand(compiler->link(outputFileName(), objects, linkerOptions()));
        linkJob->setType(Job::Linking);
        queue->addJob(linkJob);
    } else {
        delete linkJob;
        return false;
    }
    return true;
}

void LibraryTarget::fillCompilerAndLinkerOptions(CompilerOptions* compilerOptions, LinkerOptions* linkerOptions)
{
    CompilableTarget::fillCompilerAndLinkerOptions(compilerOptions, linkerOptions);

    const std::string buildPath = script()->buildDir() + directory();

    // Add link options if filling linkerOptions for other targets.
    if (m_linkerOptions != linkerOptions) {
        if (m_linkType == LinkerOptions::SharedLibrary) {
            linkerOptions->addLibraryPath(buildPath);
            linkerOptions->addLibrary(name());
        } else if (m_linkType == LinkerOptions::StaticLibrary) {
            linkerOptions->addStaticLibrary(buildPath + outputFileName());
        }
    }

    linkerOptions->setLinkType(m_linkType);
    compilerOptions->setCompileForLibrary(true);
}
