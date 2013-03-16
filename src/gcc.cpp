/*
    This file is part of the Meique project
    Copyright (C) 2009-2010 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "gcc.h"
#include "compileroptions.h"
#include "linkeroptions.h"
#include "oscommandjob.h"
#include "os.h"
#include "logger.h"
#include "stdstringsux.h"
#include <algorithm>

Gcc::Gcc() : m_isAvailable(false)
{
    std::string output;
    int retval = OS::exec("g++", "--version", &output);
    if (!retval) {
        size_t it = output.find('\n');
        m_fullName = output.substr(0, it);

        OS::exec("g++", "-dumpversion", &m_version);
        std::string machine;
        OS::exec("g++", "-dumpmachine", &machine);

        m_isAvailable = true;
    }
}

StringList getCompilationArguments(const CompilerOptions* options)
{
    StringList args;
    if (options->compileForLibrary()) {
        if (!contains(args, "-fPIC") && !contains(args, "-fpic"))
            args.push_back("-fPIC");
        args.push_back("-fvisibility=hidden");
    }
    if (options->debugInfoEnabled()) {
        if (!contains(args, "-g") && !contains(args, "-ggdb"))
            args.push_back("-ggdb");
    }

    // custom flags
    StringList flags = options->customFlags();
    std::copy(flags.begin(), flags.end(), std::back_inserter(args));

    // include paths
    StringList paths = options->includePaths();
    StringList::iterator it = paths.begin();
    for (; it != paths.end(); ++it)
        args.push_back("-I\"" + *it + '"');

    // defines
    StringList defines = options->defines();
    it = defines.begin();
    for (; it != defines.end(); ++it)
        args.push_back("-D" + *it);

    return std::move(args);
}

OSCommandJob* Gcc::compile(const std::string& fileName, const std::string& output, const CompilerOptions* options) const
{
    StringList args;
    args.push_back("-c");
    args.push_back(fileName);
    args.push_back("-o");
    args.push_back(output);
    args.merge(getCompilationArguments(options));

    Language lang = identifyLanguage(fileName);
    std::string compiler;
    if (lang == CLanguage)
        compiler = "gcc";
    else if (lang == CPlusPlusLanguage)
        compiler = "g++";
    else
        Error() << "Unknown programming language used for " << fileName;
    return new OSCommandJob(compiler, args);
}

OSCommandJob* Gcc::link(const std::string& output, const StringList& objects, const LinkerOptions* options) const
{
    StringList args = objects;
    std::string linker;

    if (options->linkType() == LinkerOptions::StaticLibrary) {
        linker = "ar";
        args.push_front(output);
        args.push_front("-rcs");
    } else {
        if (options->language() == CPlusPlusLanguage)
            linker = "g++";
        else if (options->language() == CLanguage)
            linker = "gcc";
        else
            Error() << "Unsupported programming language sent to the linker!";

        if (options->linkType() == LinkerOptions::SharedLibrary) {
            if (!contains(args, "-fPIC") && !contains(args, "-fpic"))
                args.push_front("-fPIC");
            args.push_front("-shared");
            args.push_front("-Wl,-soname=" + output);
        }
        args.push_back("-o");
        args.push_back(output);

        // custom flags
        StringList flags = options->customFlags();
        std::copy(flags.begin(), flags.end(), std::back_inserter(args));

        // library paths
        StringList paths = options->libraryPaths();
        StringList::iterator it = paths.begin();
        for (; it != paths.end(); ++it)
            args.push_back("-L\"" + *it + '"');

        // libraries
        StringList libraries = options->libraries();
        it = libraries.begin();
        for (; it != libraries.end(); ++it)
            args.push_back("-l" + *it);

        // static libraries
        StringList staticLibs = options->staticLibraries();
        std::copy(staticLibs.begin(), staticLibs.end(), std::back_inserter(args));

        // Add rpath
        if (paths.size())
            args.push_back("-Wl,-rpath=" + join(paths, ":"));
    }

    return new OSCommandJob(linker, args);
}

std::string Gcc::getCompilationCmdHash(const CompilerOptions* options) const
{
    return join(getCompilationArguments(options), " ");
}

std::string Gcc::nameForExecutable(const std::string& name) const
{
    return name;
}

std::string Gcc::nameForStaticLibrary(const std::string& name) const
{
    return "lib" + name + ".a";
}

std::string Gcc::nameForSharedLibrary(const std::string& name) const
{
    return "lib" + name + ".so";
}
