#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_luainternal.h"

#ifdef JPLUA

#if !defined(NO_SQL) && !defined(NO_SQLITE)

#if defined(__EMSCRIPTEN__) || defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

namespace JPLua {

static const char *SQLite = "sqlite.meta";

luaSQLite_t *CheckSQLite(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, SQLite);
    luaL_argcheck(L, ud != NULL, 1, "'SQLite' expected");
    return (luaSQLite_t *)ud;
}

void SQLite_CreateRef(lua_State *L, sqlite3 *db, const char *path) {
    luaSQLite_t *data = NULL;

    data = (luaSQLite_t *)lua_newuserdata(L, sizeof(luaSQLite_t));
    Q_strncpyz(data->path, path, sizeof(data->path));
    data->handle = db;
    luaL_getmetatable(L, SQLite);
    lua_setmetatable(L, -2);
}

static int SQLite_ToString(lua_State *L) {
    luaSQLite_t *data = CheckSQLite(L, 1);
    lua_pushfstring(L, "SQLite Database (Database: %s)", data->path);
    return 1;
}

int SQLite_Open(lua_State *L) {
    const char *dbpath = luaL_checkstring(L, 2);
    char fullPath[MAX_OSPATH] = {};

    if (!Q_stricmp(dbpath, ":memory:")) {
        Q_strncpyz(fullPath, ":memory:", sizeof(fullPath));
    } else {
        if (!dbpath[0] || strstr(dbpath, "..") || dbpath[0] == '/' || dbpath[0] == '\\') {
            trap->Print("SQLite Connection Error: invalid database path '%s'\n", dbpath);
            lua_pushnil(L);
            return 1;
        }

        char gameDir[MAX_QPATH] = {};
        trap->Cvar_VariableStringBuffer("fs_game", gameDir, sizeof(gameDir));
        if (!gameDir[0]) {
            Q_strncpyz(gameDir, "base", sizeof(gameDir));
        }

        char homePath[MAX_OSPATH] = {};
        trap->Cvar_VariableStringBuffer("fs_homepath", homePath, sizeof(homePath));
        if (homePath[0]) {
            Com_sprintf(fullPath, sizeof(fullPath), "%s/%s/%s", homePath, gameDir, dbpath);
        } else {
            Com_sprintf(fullPath, sizeof(fullPath), "%s/%s", gameDir, dbpath);
        }

#if defined(__EMSCRIPTEN__) || defined(__unix__) || defined(__APPLE__)
        // Ensure parent directories exist for SQLite database creation.
        for (char *p = fullPath + 1; *p; ++p) {
            if (*p == '/') {
                *p = '\0';
                mkdir(fullPath, 0755);
                *p = '/';
            }
        }
#endif
    }

    sqlite3 *db = NULL;
    int error = sqlite3_open(fullPath, &db);
    if (error != SQLITE_OK) {
        trap->Print("SQLite Connection Error: %s\n", db ? sqlite3_errmsg(db) : "out of memory");
        if (db) {
            sqlite3_close(db);
        }
        lua_pushnil(L);
        return 1;
    }
    SQLite_CreateRef(L, db, dbpath);
    return 1;
}

static int SQLite_Close(lua_State *L) {
    luaSQLite_t *data = CheckSQLite(L, 1);
    if (data->handle) {
        sqlite3_close(data->handle);
        data->handle = NULL;
    }
    return 0;
}

static int SQLite_IsOpen(lua_State *L) {
    luaSQLite_t *data = CheckSQLite(L, 1);
    lua_pushboolean(L, data->handle != NULL ? 1 : 0);
    return 1;
}

static int SQLite_Query(lua_State *L) {
    luaSQLite_t *data = CheckSQLite(L, 1);
    const char *query = luaL_checkstring(L, 2);

    if (!data->handle) {
        lua_pushnil(L);
        lua_pushstring(L, "database is closed");
        return 2;
    }

    sqlite3_stmt *stmt = NULL;
    int error = sqlite3_prepare_v2(data->handle, query, -1, &stmt, NULL);
    if (error != SQLITE_OK) {
        const char *errmsg = sqlite3_errmsg(data->handle);
        trap->Print("SQLite Query Error: %s\n", errmsg);
        if (stmt) {
            sqlite3_finalize(stmt);
        }
        lua_pushnil(L);
        lua_pushstring(L, errmsg);
        return 2;
    }

    int count = sqlite3_column_count(stmt);
    lua_newtable(L);
    int top = lua_gettop(L);
    int row = 1;
    while ((error = sqlite3_step(stmt)) == SQLITE_ROW) {
        lua_pushinteger(L, row++);
        lua_newtable(L); /// Row - Table
        int tempTop = lua_gettop(L);
        for (int i = 0; i < count; i++) {
            const char *name = sqlite3_column_name(stmt, i);
            switch (sqlite3_column_type(stmt, i)) {

            case SQLITE_INTEGER: {
                lua_pushstring(L, name);
                lua_pushinteger(L, sqlite3_column_int64(stmt, i));
                lua_settable(L, tempTop);
            } break;

            case SQLITE_FLOAT: {
                lua_pushstring(L, name);
                lua_pushnumber(L, sqlite3_column_double(stmt, i));
                lua_settable(L, tempTop);
            } break;

            case SQLITE_BLOB: {
                lua_pushstring(L, name);
                lua_pushlstring(L, (const char *)sqlite3_column_blob(stmt, i), sqlite3_column_bytes(stmt, i));
                lua_settable(L, tempTop);
            } break;

            case SQLITE_NULL: {
                lua_pushstring(L, name);
                lua_pushnil(L);
                lua_settable(L, tempTop);
            } break;

            case SQLITE3_TEXT: {
                lua_pushstring(L, name);
                lua_pushlstring(L, (const char *)sqlite3_column_text(stmt, i), sqlite3_column_bytes(stmt, i));
                lua_settable(L, tempTop);
            } break;

            default: {
            } break;
            }
        }
        lua_settable(L, top);
    }

    if (error != SQLITE_DONE) {
        const char *errmsg = sqlite3_errmsg(data->handle);
        trap->Print("SQLite Step Error: %s\n", errmsg);
        sqlite3_finalize(stmt);
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, errmsg);
        return 2;
    }

    sqlite3_finalize(stmt);
    return 1;
}

static const struct luaL_Reg sqliteMeta[] = {
    {"__gc", SQLite_Close}, {"__tostring", SQLite_ToString}, {"Close", SQLite_Close}, {"IsOpen", SQLite_IsOpen}, {"Query", SQLite_Query}, {NULL, NULL},
};

void Register_SQLite(lua_State *L) {
    luaL_newmetatable(L, SQLite);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (const luaL_Reg *r = sqliteMeta; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    lua_pop(L, -1);
}

} // namespace JPLua

#endif // !NO_SQL

#endif // JPLUA
