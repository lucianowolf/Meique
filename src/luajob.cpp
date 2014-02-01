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

#include "luajob.h"
#include "lualocker.h"
#include "lua.h"
#include "luacpputil.h"
#include "logger.h"

LuaJob::LuaJob(lua_State* L, int args) : m_L(L)
{
    assert(lua_type(L, -(args + 1)) == LUA_TFUNCTION);
    assert(lua_gettop(L) >= args + 1);

    // Put the function and all args into a array
    const int tableSize = args + 1;
    lua_createtable(L, tableSize, 0);
    lua_insert(L, -tableSize - 1);
    for (int i = 0; i < tableSize; ++i)
        lua_rawseti(L, i - tableSize - 1, i+1);

    // store the table on lua registry
    lua_pushlightuserdata(L, this); // key
    lua_insert(L, -2);
    lua_settable(L, LUA_REGISTRYINDEX);
}

int LuaJob::doRun()
{
    LuaLocker locker(m_L);

    OS::ChangeWorkingDirectory dirChanger(workingDirectory());
    // Get the lua function and put it on lua stack
    lua_pushlightuserdata(m_L, this); // key
    lua_gettable(m_L, LUA_REGISTRYINDEX);
    int objlen = lua_objlen(m_L, -1);
    for (int i = 1; i <= objlen; ++i)
        lua_rawgeti(m_L, -i, objlen - i + 1);

    try {
        luaPCall(m_L, objlen - 1, 0);
        lua_pop(m_L, 2);
    } catch (const Error& e) {
        e.show();
        return 1;
    }
    return 0;
}
