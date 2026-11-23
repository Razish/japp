#pragma once

#ifdef JPLUA

#if !defined(NO_SQL) && (!defined(NO_MYSQL) || !defined(NO_SQLITE))
#if !defined(NO_MYSQL)
#include "mysql/mysql.h"
#endif
#if !defined(NO_SQLITE)
#include "sqlite/sqlite3.h"
#endif

namespace JPLua {

#if !defined(NO_SQLITE)
struct luaSQLite_t {
    char path[MAX_QPATH];
    sqlite3 *handle;
};
#endif

#ifdef JPLUA_INTERNALS
#if !defined(NO_MYSQL)
MYSQL *CheckMySQL(lua_State *L, int idx);
MYSQL_RES **CheckMySQLResult(lua_State *L, int idx);
void MySQLResult_CreateRef(lua_State *L, MYSQL_RES *res);
void Register_MySQL(lua_State *L);
int MySQL_Open(lua_State *L);
#endif

#if !defined(NO_SQLITE)
void SQLite_CreateRef(lua_State *L, sqlite3 *db, const char *path);
void Register_SQLite(lua_State *L);
luaSQLite_t *CheckSQLite(lua_State *L, int idx);
int SQLite_Open(lua_State *L);
#endif
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // !NO_SQL && (!NO_MYSQL || !NO_SQLITE)

#endif // JPLUA
