#pragma once
#include "mysql.h"

#ifdef JPLUA
MYSQL *JPLua_CheckMySQL(lua_State *L, int idx);
MYSQL_RES **JPLua_CheckMySQLResult(lua_State *L, int idx);
void JPLua_MySQLResult_CreateRef(lua_State *L, MYSQL_RES *res);
void JPLua_Register_MySQL(lua_State* L);
int JPLua_MySQL_Open(lua_State *L);
#endif