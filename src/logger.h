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

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <list>
#include <map>
#include "os.h"

#ifndef NOCOLOR
    #define COLOR_END "\033[0m"
    #define COLOR_WHITE "\033[1;37m"
    #define COLOR_YELLOW "\033[1;33m"
    #define COLOR_GREEN "\033[0;32m"
    #define COLOR_RED "\033[0;31m"
    #define COLOR_MAGENTA "\033[1;35m"
#else
    #define COLOR_END ""
    #define COLOR_WHITE ""
    #define COLOR_YELLOW ""
    #define COLOR_GREEN ""
    #define COLOR_RED ""
    #define COLOR_MAGENTA ""
#endif

// definied in config.cpp
extern int verboseMode;

struct green {};
struct red {};
struct yellow {};
struct magenta {};
struct nocolor {};

std::ostream& operator<<(std::ostream& out, const green&);
std::ostream& operator<<(std::ostream& out, const red&);
std::ostream& operator<<(std::ostream& out, const yellow&);
std::ostream& operator<<(std::ostream& out, const nocolor&);
std::ostream& operator<<(std::ostream& out, const magenta&);

class BaseLogger : public std::ostream
{
public:
    enum Options
    {
        None = 0,
        ShowPid = 1,
        Quiet = 2
    };

    BaseLogger(std::ostream& output, Options options = None) : m_stream(output), m_options(options) {}
    ~BaseLogger()
    {
        if (m_options & Quiet)
            return;
        m_stream << nocolor() << std::endl;
    }
    std::ostream& operator()() { return m_stream; };
    template <typename T>
    std::ostream& operator<<(const T& t)
    {
        if (m_options & Quiet)
            return *this;
        return m_stream << t;
    }

protected:
    std::ostream& m_stream;
    unsigned int m_options;
};

template<typename T>
inline std::ostream& operator<<(std::ostream& out, const std::list<T>& list)
{
    out << '[';
    if (list.size()) {
        out << list.front();
        typename std::list<T>::const_iterator it = ++list.begin();
        for (; it != list.end(); ++it)
            out << ", " << *it;
    }
    out << ']';
    return out;
}

template<typename K, typename V>
inline std::ostream& operator<<(std::ostream& out, const std::map<K, V>& map)
{
    out << '{';
    if (map.size()) {
        out << map.begin()->first << " -> " << map.begin()->second;
        typename std::map<K, V>::const_iterator it = ++map.begin();
        for (; it != map.end(); ++it)
            out << ", " << it->first << " -> " << it->second;
    }
    out << '}';
    return out;
}

class Warn : public BaseLogger
{
public:
    Warn() : BaseLogger(std::cout)
    {
        m_stream << COLOR_YELLOW "WARNING" COLOR_END " :: ";
    }
};

struct MeiqueError
{
    MeiqueError();
    static bool errorAlreadyset;
};

class Error : public BaseLogger
{
public:
    Error() : BaseLogger(std::cerr)
    {
        m_stream << COLOR_RED "ERROR" COLOR_END " :: ";
    }
    ~Error()
    {
        if (!MeiqueError::errorAlreadyset)
            throw MeiqueError();
    }
};

class Notice : public BaseLogger
{
public:
    Notice() : BaseLogger(std::cout) {}
};

class Debug : public BaseLogger
{
public:
    Debug(int level = 1) : BaseLogger(std::cout)
    {
        if (level > verboseMode)
            m_options |= Quiet;
    }
};

#endif // LOGGER_H
