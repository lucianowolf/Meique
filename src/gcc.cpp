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
#include "logger.h"
#include "os.h"
#include "compileroptions.h"
#include "linkeroptions.h"

bool Gcc::isAvailable() const
{
    return true;
}

bool Gcc::compile(const std::string& fileName, const std::string& output, const CompilerOptions* options) const
{
    // TODO: Identify what to use, g++ or gcc
    StringList args;
    args.push_back("-c");
    args.push_back(fileName);
    args.push_back("-o");
    args.push_back(output);
    StringList flags = options->customFlags();
    std::copy(flags.begin(), flags.end(), std::back_inserter(args));
    StringList paths = options->includePaths();
    std::copy(paths.begin(), paths.end(), std::back_inserter(args));
    StringList defines = options->defines();
    std::copy(defines.begin(), defines.end(), std::back_inserter(args));
    return !OS::exec("g++", args);
}

void Gcc::link(const std::string& output, const StringList& objects, const LinkerOptions* options) const
{
    StringList args = objects;
    args.push_back("-o");
    args.push_back(output);
    StringList flags = options->customFlags();
    std::copy(flags.begin(), flags.end(), std::back_inserter(args));
    StringList paths = options->libraryPath();
    std::copy(paths.begin(), paths.end(), std::back_inserter(args));
    StringList libraries = options->libraries();
    std::copy(libraries.begin(), libraries.end(), std::back_inserter(args));
    OS::exec("g++", args);
}

