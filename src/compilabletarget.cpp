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

#include "compilabletarget.h"
#include <fstream>
#include <algorithm>

#include "luacpputil.h"
#include "compiler.h"
#include "logger.h"
#include "config.h"
#include "os.h"
#include "filehash.h"
#include "compileroptions.h"
#include "linkeroptions.h"
#include "stdstringsux.h"
#include "jobqueue.h"
#include "job.h"

CompilableTarget::CompilableTarget(const std::string& targetName, MeiqueScript* script)
    : LuaTarget(targetName, script), m_compilerOptions(0), m_linkerOptions(0)
{
}

CompilableTarget::~CompilableTarget()
{
    delete m_compilerOptions;
    delete m_linkerOptions;
}

JobQueue* CompilableTarget::doRun(Compiler* compiler)
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);

    if (files.empty())
        Error() << "Compilable target '" << name() << "' has no files!";

    if (!m_compilerOptions)
        fillCompilerAndLinkerOptions();

    JobQueue* queue = new JobQueue;
    std::string sourceDir = config().sourceRoot() + directory();
    std::string buildDir = OS::pwd();

    bool needLink = false;
    StringList objects;
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it) {
        std::string source = sourceDir + *it;
        std::string output = *it + ".o";

        bool compileIt = !OS::fileExists(output);
        StringList dependents = getFileDependencies(source);
        if (!compileIt)
            compileIt = config().isHashGroupOutdated(dependents);

        if (compileIt) {
            Job* job = compiler->compile(source, output, m_compilerOptions);
            job->addJobListenner(this);
            job->setWorkingDirectory(buildDir);
            job->setDescription("Compiling " + *it);
            queue->addJob(job);
            m_job2Sources[job] = dependents;
            needLink = true;
        }
        objects.push_back(output);
    }

    if (needLink || !OS::fileExists(name())) {
        Job* job = compiler->link(name(), objects, m_linkerOptions);
        job->setWorkingDirectory(buildDir);
        job->setDescription("Linking " + name());
        job->setDependencies(queue->idleJobs());
        queue->addJob(job);
    }
    return queue;
}
void CompilableTarget::jobFinished(Job* job)
{
    if (!job->result())
        config().updateHashGroup(m_job2Sources[job]);
}

void CompilableTarget::preprocessFile(const std::string& source, const std::string& baseDir, StringList* deps)
{
    std::string absSource(source);
    if (absSource.empty())
        return;
    if (absSource[0] != '/')
        absSource.insert(0, baseDir);
    if (std::find(deps->begin(), deps->end(), absSource) != deps->end())
        return;
    deps->push_back(absSource);

    std::ifstream fp(absSource.c_str());
    if (!fp) {
        Debug() << "Include file not found: " << absSource;
        return;
    }

    std::string buffer1;
    std::string buffer2;

    while (!fp.eof()) {
        std::getline(fp, buffer1);
        trim(buffer1);

        if (buffer1[0] != '#')
            continue;
        buffer1.erase(0, 1);

        while (buffer1[buffer1.size() - 1] == '\\') {
            if (fp.eof())
                return;
            std::getline(fp, buffer2);
            buffer1.erase(buffer1.size() - 1, 1);
            buffer1 += buffer2;
            trim(buffer1);
        }

        if (buffer1.compare(0, 7, "include") != 0)
            continue;
        buffer1.erase(0, 7);
        trim(buffer1);
        if (buffer1[ 0 ] == '"') {
            std::string includedFile = buffer1.substr(1, buffer1.size()-2);
            preprocessFile(includedFile, baseDir, deps);
        }
    }
}

StringList CompilableTarget::getFileDependencies(const std::string& source)
{
    std::string baseDir = config().sourceRoot() + directory();
    StringList dependents;
    // FIXME: There is a large room for improviments here, we need to cache some results
    //        to avoid doing a lot of things twice.
    preprocessFile(source, baseDir, &dependents);
    return dependents;
}

void CompilableTarget::fillCompilerAndLinkerOptions()
{
    m_compilerOptions = new CompilerOptions;
    m_linkerOptions = new LinkerOptions;
    getLuaField("_packages");
    lua_State* L = luaState();
    // loop on all used packages
    int tableIndex = lua_gettop(L);
    lua_pushnil(L);  /* first key */
    while (lua_next(L, tableIndex) != 0) {

        StringMap map;
        readLuaTable<StringMap>(L, lua_gettop(L), map);

        m_compilerOptions->addIncludePath(map["includePaths"]);
        m_compilerOptions->addCustomFlag(map["cflags"]);
        m_linkerOptions->addCustomFlag(map["linkerFlags"]);
        m_linkerOptions->addLibraryPath(map["libraryPaths"]);
        m_linkerOptions->addLibraries(split(map["linkLibraries"]));
        lua_pop(L, 1); // removes 'value'; keeps 'key' for next iteration
    }

    getLuaField("_linkLibraries");
    StringList list;
    readLuaList(L, lua_gettop(L), list);
    m_linkerOptions->addLibraries(list);
}

void CompilableTarget::clean()
{
    // get sources
    getLuaField("_files");
    StringList files;
    readLuaList(luaState(), lua_gettop(luaState()), files);
    StringList::const_iterator it = files.begin();
    for (; it != files.end(); ++it)
        OS::rm(directory() + *it + ".o");
}
