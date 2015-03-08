#pragma once
#include "mysql.h"
#include "sqlite3.h"

#ifdef JPLUA

MYSQL *JPLua_CheckMySQL(lua_State *L, int idx);
MYSQL_RES **JPLua_CheckMySQLResult(lua_State *L, int idx);
void JPLua_MySQLResult_CreateRef(lua_State *L, MYSQL_RES *res);
void JPLua_Register_MySQL(lua_State* L);
int JPLua_MySQL_Open(lua_State *L);

typedef struct jplua_sqlite_s{
	char path[MAX_QPATH];
	sqlite3 *handle;
}jplua_sqlite_t;
void JPLua_SQLite_CreateRef(lua_State *L, sqlite3 *db, const char *path);
void JPLua_Register_SQLite(lua_State *L);
jplua_sqlite_t *JPLua_CheckSQLite(lua_State *L, int idx);
int JPLua_SQLite_Open(lua_State *L);

#endif