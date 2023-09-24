#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_luainternal.h"

#ifdef JPLUA

#if !defined(NO_SQL)

namespace JPLua {

static const char *MySQLLib = "mysql.meta";
static const char *MySQLResult = "mysqlres.meta";

MYSQL *CheckMySQL(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, MySQLLib);
    luaL_argcheck(L, ud != NULL, 1, "'MySQL' expected");
    return (MYSQL *)ud;
}

MYSQL_RES **CheckMySQLResult(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, MySQLResult);
    luaL_argcheck(L, ud != NULL, 1, "'MySQLResult' expected");
    return (MYSQL_RES **)ud;
}

void MySQLResult_CreateRef(lua_State *L, MYSQL *sql, MYSQL_RES **data) {
    data = (MYSQL_RES **)lua_newuserdata(L, sizeof(*data));
    *data = mysql_store_result(sql);
    luaL_getmetatable(L, MySQLResult);
    lua_setmetatable(L, -2);
}

static int MySQL_ToString(lua_State *L) {
    MYSQL *data = CheckMySQL(L, 1);
    lua_pushfstring(L, "MySQL Connection (IP: %s || Database: %s)", data->host, data->db);
    return 1;
}

static int MySQLResult_ToString(lua_State *L) {
    MYSQL_RES **data = CheckMySQLResult(L, 1);
    lua_pushfstring(L, "MySQL Result (Rows: %d )", mysql_num_rows(*data));
    return 1;
}

int MySQL_Open(lua_State *L) {
    const char *host = luaL_checkstring(L, 2);
    const char *user = luaL_checkstring(L, 3);
    const char *db = luaL_checkstring(L, 4);
    const char *password = luaL_checkstring(L, 5);

    int port;
    if (!lua_isnil(L, 6)) {
        port = 3306;
    } else {
        port = lua_tointeger(L, 6);
    }

    MYSQL *conn = (MYSQL *)lua_newuserdata(L, sizeof(MYSQL));

    if (!mysql_init(conn)) {
        lua_pushnil(L);
    }

    if (!mysql_real_connect(conn, host, user, password, db, port, NULL, 0)) {
        lua_pushnil(L);
    }

    luaL_getmetatable(L, MySQLLib);
    lua_setmetatable(L, -2);

    return 1;
}

static int MySQL_Ping(lua_State *L) {
    MYSQL *conn = CheckMySQL(L, 1);
    lua_pushinteger(L, mysql_ping(conn));
    return 1;
}

static int MySQL_SelectDB(lua_State *L) {
    MYSQL *conn = CheckMySQL(L, 1);
    const char *db = luaL_checkstring(L, 2);

    if (!mysql_select_db(conn, db)) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, mysql_error(conn));
    }

    return 1;
}

static int MySQL_SetCharSet(lua_State *L) {
    MYSQL *conn = CheckMySQL(L, 1);
    const char *charset = luaL_checkstring(L, 2);

    if (!mysql_set_character_set(conn, charset)) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, mysql_error(conn));
    }

    return 1;
}

static int MySQL_Escape(lua_State *L) {
    MYSQL *conn = CheckMySQL(L, 1);
    size_t len = 0;
    const char *content = luaL_checklstring(L, 2, &len);

    if (len <= 0) {
        lua_pushnil(L);
        return 1;
    }

    char *buf = (char *)malloc(len);
    if (!buf) {
        lua_pushnil(L);
        return 1;
    }

    len = mysql_real_escape_string(conn, buf, content, len);
    lua_pushlstring(L, buf, len);
    free(buf);
    return 1;
}

static int MySQL_Query(lua_State *L) {
    int err;
    MYSQL *conn = CheckMySQL(L, 1);
    MYSQL_RES **result = NULL;
    size_t len;
    const char *query = luaL_checklstring(L, 2, &len);

    if (len <= 0) {
        lua_pushnil(L);
        return 1;
    }

    err = mysql_real_query(conn, query, len);
    if (err) {
        lua_pushnil(L);
        trap->Print("MySQL Error: %s", mysql_error(conn));
    } else {
        // FIXME: conn isn't right?
        MySQLResult_CreateRef(L, conn, result);
    }

    return 1;
}

static int MySQL_gc(lua_State *L) {
    MYSQL *conn = CheckMySQL(L, 1);
    mysql_close(conn);
    return 0;
}

static int MySQLResult_Count(lua_State *L) {
    MYSQL_RES **result = CheckMySQLResult(L, 1);
    lua_pushinteger(L, mysql_num_rows(*result));
    return 1;
}

static int MySQLResult_Get(lua_State *L) {
    MYSQL_RES **result = CheckMySQLResult(L, 1);
    MYSQL_ROW row = NULL;

    MYSQL_FIELD *fieldlist = mysql_fetch_fields(*result);
    int fieldcount = mysql_num_fields(*result);

    lua_newtable(L);
    int top = lua_gettop(L);
    for (int count = 1; (row = mysql_fetch_row(*result)); count++) {
        lua_pushinteger(L, count);
        lua_newtable(L);
        int tempTop = lua_gettop(L);
        for (int i = 0; i < fieldcount; i++) {
            lua_pushstring(L, fieldlist[i].name);
            lua_pushstring(L, row[i]);
            lua_settable(L, tempTop);
        }
        lua_settable(L, top);
    }

    return 1;
}

static int MySQLResult_gc(lua_State *L) {
    MYSQL_RES **result = CheckMySQLResult(L, 1);

    if (result) {
        mysql_free_result(*result);
    }

    return 0;
}

#if 0
	static const struct luaL_Reg mysqlclient_f[] = {
		{ "connect", MySQL_Open },
		{ NULL, NULL },
	};
#endif

static const struct luaL_Reg mysqlclient_m[] = {
    {"__gc", MySQL_gc},       {"__tostring", MySQL_ToString}, {"Ping", MySQL_Ping}, {"Selectdb", MySQL_SelectDB}, {"Setcharset", MySQL_SetCharSet},
    {"Escape", MySQL_Escape}, {"Query", MySQL_Query},         {NULL, NULL},
};

static const struct luaL_Reg mysqlresult_lib[] = {
    {"__gc", MySQLResult_gc}, {"__tostring", MySQLResult_ToString}, {"Count", MySQLResult_Count}, {"Get", MySQLResult_Get}, {NULL, NULL},
};

void Register_MySQL(lua_State *L) {
    luaL_newmetatable(L, MySQLLib);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (const luaL_Reg *r = mysqlclient_m; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    lua_pop(L, -1);

    luaL_newmetatable(L, MySQLResult);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (const luaL_Reg *r = mysqlresult_lib; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }
}

} // namespace JPLua

#endif // !NO_SQL

#endif
