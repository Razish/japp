#pragma once

#ifdef JPLUA

#if !defined(NO_SQL)
#include "mysql/mysql.h"
#include "sqlite/sqlite3.h"

namespace JPLua {

struct luaSQLite_t {
    char path[MAX_QPATH];
    sqlite3 *handle;
};

#ifdef JPLUA_INTERNALS
MYSQL *CheckMySQL(lua_State *L, int idx);
MYSQL_RES **CheckMySQLResult(lua_State *L, int idx);
void MySQLResult_CreateRef(lua_State *L, MYSQL_RES *res);
void Register_MySQL(lua_State *L);
int MySQL_Open(lua_State *L);

void SQLite_CreateRef(lua_State *L, sqlite3 *db, const char *path);
void Register_SQLite(lua_State *L);
luaSQLite_t *CheckSQLite(lua_State *L, int idx);
int SQLite_Open(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // !NO_SQL

#endif // JPLUA
