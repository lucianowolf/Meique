/*
    This file is part of the Meique project
    Copyright (C) 2009-2013 Hugo Parente Lima <hugo.pl@gmail.com>

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

#include "meique.h"
#include "logger.h"
#include "meiquescript.h"
#include "compiler.h"
#include "compilerfactory.h"
#include "target.h"
#include "compilabletarget.h"
#include "jobmanager.h"
#include "jobqueue.h"
#include "meiqueversion.h"
#include <vector>
#include <sstream>
#include "statemachine.h"
#include "meiquecache.h"
#include <fstream>
#include <iomanip>

#define MEIQUECACHE "meiquecache.lua"

enum {
    HasVersionArg = 1,
    HasHelpArg,
    NormalArgs,
    DumpProject,
    Found,
    NotFound,
    Yes,
    No,
    Ok,
    TestAction,
    InstallAction,
    UninstallAction,
    BuildAction,
    CleanAction
};

Meique::Meique(int argc, const char** argv) : m_args(argc, argv), m_jobManager(new JobManager), m_script(0), m_firstRun(false)
{
    int jobLimit = m_args.intArg("j", OS::numberOfCPUCores() + 1);
    if (jobLimit <= 0)
        throw Error("You should use a number greater than zero in -j option.");
    m_jobManager->setJobCountLimit(jobLimit);
}

Meique::~Meique()
{
    m_jobManager->finish();
    delete m_script;
    delete m_jobManager;
}

int Meique::checkArgs()
{
    std::string verboseValue = OS::getEnv("VERBOSE");
    std::istringstream s(verboseValue);
    s >> ::verbosityLevel;

    if (m_args.boolArg("d"))
        ::coloredOutputEnabled = false;
    if (m_args.boolArg("version"))
        return HasVersionArg;
    if (m_args.boolArg("help"))
        return HasHelpArg;
    if (m_args.boolArg("meique-dump-project"))
        return DumpProject;
    return NormalArgs;
}

int Meique::lookForMeiqueCache()
{
    std::ifstream file(MEIQUECACHE);
    return file ? Found : NotFound;
}

int Meique::lookForMeiqueLua()
{
    if (m_args.numberOfFreeArgs() == 0)
        return NotFound;
    std::string path = m_args.freeArg(0);
    return OS::fileExists(path + "/meique.lua") ? Found : NotFound;
}

int Meique::configureProject()
{
    std::string meiqueLuaPath = OS::normalizeDirPath(m_args.freeArg(0));
    m_script = new MeiqueScript(meiqueLuaPath + "/meique.lua", &m_args);
    m_script->setSourceDir(meiqueLuaPath);
    m_script->setBuildDir(OS::pwd());
    m_firstRun = true;

    try {
        m_script->exec();
    } catch (const Error&) {
        m_script->cache()->setAutoSave(false);
        throw;
    }
    return m_args.boolArg("s") ? 0 : Ok;
}

int Meique::dumpProject()
{
    std::ifstream file(MEIQUECACHE);
    if (!file)
        throw Error(MEIQUECACHE " not found.");
    file.close();

    m_script = new MeiqueScript;
    m_script->setBuildDir("./");
    m_script->exec();

    std::cout << "Project: " << OS::baseName(m_script->sourceDir()) << std::endl;

    for (Target* target : m_script->targets()) {
        if (!target->isCompilableTarget())
            continue;
        CompilableTarget* ctarget = static_cast<CompilableTarget*>(target);

        std::cout << "Target: " << ctarget->name() << std::endl;
        for (const std::string& fileName : target->files()) {
            if (fileName.empty())
                continue;
            std::string absPath = fileName[0] == '/' ? fileName : m_script->sourceDir() + target->directory() + fileName;
            absPath = OS::normalizeFilePath(absPath);
            std::cout << "File: " << absPath << std::endl;
            auto lastDot = absPath.find_last_of(".");
            if (lastDot != std::string::npos) {
                absPath.replace(lastDot, absPath.size() - lastDot, ".h");
                if (OS::fileExists(absPath))
                    std::cout << "File: " << absPath << std::endl;
            }

            // TODO: Defines
        }

        std::cout << "Include: " << OS::normalizeDirPath(m_script->sourceDir() + target->directory()) << std::endl;
        for (const std::string& inc : ctarget->includeDirectories())
            std::cout << "Include: " << OS::normalizeDirPath(inc) << std::endl;
    }
    return 0;
}

int Meique::getBuildAction()
{
    if (!m_script) {
        m_script = new MeiqueScript;
        m_script->setBuildDir(OS::pwd());
        m_script->exec();
    }

    if (m_args.boolArg("c"))
        return CleanAction;
    else if (m_args.boolArg("i"))
        return InstallAction;
    else if (m_args.boolArg("t"))
        return TestAction;
    else if (m_args.boolArg("u"))
        return UninstallAction;
    else
        return BuildAction;
}

TargetList Meique::getChosenTargets()
{
    int ntargets = m_args.numberOfFreeArgs();
    StringList targetNames;
    for (int i = m_firstRun ? 1 : 0; i < ntargets; ++i)
        targetNames.push_back(m_args.freeArg(i));

    TargetList targets;
    if (targetNames.empty()) {
        targets = m_script->targets();
    } else {
        for (const std::string& targetName : targetNames)
            targets.push_back(m_script->getTarget(targetName));
    }

    if (targets.empty())
        throw Error("There's no targets!");

    return targets;
}

void Meique::buildTarget(Target* target, int& d)
{
    target->setStatus(Target::CheckingChildren);

    bool hasDependencies = false;
    for (Target* dep : target->dependencies()) {
        if (dep->status() == Target::CheckingChildren)
            Error() << "Cyclic dependency found in your targets!";
        if (dep->status() == Target::JobEmitted)
            continue;

        hasDependencies = true;
        int oldD = d;
        buildTarget(dep, d);
        d = std::max(d, oldD);
    }

    if (!hasDependencies) {
        d = 0;
        Warn() << "Target " << target->name() << " is ok!";
        target->emitJobs(m_jobManager->queue());
        target->setStatus(Target::JobEmitted);
    } else {
        d++;
//        target->cacheJobs();
        Warn() << "========== " << d;
        if (d == 1) {
            Warn() << "Cache jobs from " << target->name();
        }
    }

}

int Meique::buildTargets()
{
    int d = 0;
    for (Target* target : getChosenTargets())
        buildTarget(target, d);
}

int Meique::cleanTargets()
{
    for (Target* target : getChosenTargets())
        target->clean();
    return 0;
}

int Meique::installTargets()
{
    for (Target* target : getChosenTargets())
        target->install();
    return 0;
}

static void writeTestResults(LogWriter& s, int result, unsigned long start, unsigned long end) {
    if (result)
        s << Red << "FAILED";
    else
        s << Green << "Passed";
    s << NoColor << ' ' << std::setiosflags(std::ios::fixed) << std::setprecision(2) << (end - start)/1000.0 << "s";
}

int Meique::testTargets()
{
    bool hasRegex = m_args.numberOfFreeArgs() > 0;
    auto tests = m_script->getTests(hasRegex ? m_args.freeArg(0) : std::string());
    if (tests.empty()) {
        Notice() << "No tests to run :-(";
        return 0;
    }

    Log log((m_script->buildDir() + "meiquetest.log").c_str());
    bool verboseMode = ::verbosityLevel != 0;
    int i = 1;
    const int total = tests.size();
    for (const StringList& testPieces : tests) {
        StringList::const_iterator j = testPieces.begin();
        const std::string& testName = *j;
        const std::string& testCmd = *(++j);
        const std::string& testDir = *(++j);

        OS::mkdir(testDir);
        std::string output;

        // Write a nice output.
        if (!verboseMode) {
            Notice() << std::setw(3) << std::setfill(' ') << std::right << i << '/' << total << ": " << testName << NoBreak;
            Notice() << ' ' << std::setw(48 - testName.size() + 1) << std::setfill('.') << ' ' << NoBreak;
        }

        unsigned long start = OS::getTimeInMillis();
        Debug() << i << ": Test Command: " << NoBreak;
        int res = OS::exec(testCmd, StringList(), &output, testDir, OS::MergeErr);
        unsigned long end = OS::getTimeInMillis();

        if (verboseMode) {
            std::istringstream in(output);
            std::string line;
            while (!in.eof()) {
                std::getline(in, line);
                Debug() << i << ": " << line;
            }
            Debug s;
            s << i << ": Test result: ";
            writeTestResults(s, res, start, end);
            Debug();
        } else {
            Notice s;
            writeTestResults(s, res, start, end);
        }
        log << ":: Running test: " << testName;
        log << output;
        ++i;
    }
    return 0;
}

int Meique::uninstallTargets()
{
    for (Target* target : getChosenTargets())
        target->uninstall();
    return 0;
}

void Meique::exec()
{
    StateMachine<Meique> machine(this);

    machine[STATE(Meique::checkArgs)][HasHelpArg] = STATE(Meique::showHelp);
    machine[STATE(Meique::checkArgs)][HasVersionArg] = STATE(Meique::showVersion);
    machine[STATE(Meique::checkArgs)][NormalArgs] = STATE(Meique::lookForMeiqueCache);
    machine[STATE(Meique::checkArgs)][DumpProject] = STATE(Meique::dumpProject);

    machine[STATE(Meique::lookForMeiqueCache)][Found] = STATE(Meique::getBuildAction);
    machine[STATE(Meique::lookForMeiqueCache)][NotFound] = STATE(Meique::lookForMeiqueLua);

    machine[STATE(Meique::lookForMeiqueLua)][Found] = STATE(Meique::configureProject);
    machine[STATE(Meique::lookForMeiqueLua)][NotFound] = STATE(Meique::showHelp);

    machine[STATE(Meique::configureProject)][Ok] = STATE(Meique::getBuildAction);

    machine[STATE(Meique::getBuildAction)][TestAction] = STATE(Meique::testTargets);
    machine[STATE(Meique::getBuildAction)][InstallAction] = STATE(Meique::installTargets);
    machine[STATE(Meique::getBuildAction)][UninstallAction] = STATE(Meique::uninstallTargets);
    machine[STATE(Meique::getBuildAction)][BuildAction] = STATE(Meique::buildTargets);
    machine[STATE(Meique::getBuildAction)][CleanAction] = STATE(Meique::cleanTargets);

    machine.execute(STATE(Meique::checkArgs));
}

int Meique::showVersion()
{
    std::cout << "Meique version " MEIQUE_VERSION << std::endl;
    std::cout << "Copyright 2009-2013 Hugo Parenaate Lima <hugo.pl@gmail.com>\n";
    return 0;
}

int Meique::showHelp()
{
    std::cout << "Usage: meique OPTIONS TARGET\n\n";
    std::cout << "When in configure mode, TARGET is the directory of meique.lua file.\n";
    std::cout << "When in build mode, TARGET is the target name.\n\n";
    std::cout << "General options:\n";
    std::cout << " --help                             Print this message and exit.\n";
    std::cout << " --version                          Print the version number of meique and exit.\n";
    std::cout << "Config mode options for this project:\n";
    std::cout << " --debug                            Create a debug build.\n";
    std::cout << " --release                          Create a release build.\n";
    std::cout << " --install-prefix                   Install directory used by install, this directory\n";
    std::cout << "                                    is prepended onto all install directories.\n";
/*
    if (options.size()) {
        OptionsMap::const_iterator it = options.begin();
        for (; it != options.end(); ++it) {
            std::cout << " --" << std::left;
            std::cout.width(33);
            std::cout << it->first;
            std::cout << it->second.description;
            if (!it->second.defaultValue.empty())
                std::cout << " [default value: " << it->second.defaultValue << ']';
            std::cout << std::endl;
        }
    }
*/
    std::cout << "Build mode options:\n";
    std::cout << " -jN                                Allow N jobs at once, default to number of\n";
    std::cout << "                                    cores + 1.\n";
    std::cout << " -d                                 Disable colored output\n";
    std::cout << " -s                                 Stop after configure step.\n";
    std::cout << " -c [target [, target2 [, ...]]]    Clean a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -i [target [, target2 [, ...]]]    Install a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -u [target [, target2 [, ...]]]    Uninstall a specific target or all targets if\n";
    std::cout << "                                    none was specified.\n";
    std::cout << " -t [regex]                         Run tests matching a regular expression, all\n";
    std::cout << "                                    tests if none was specified.\n";
    return 0;
}
