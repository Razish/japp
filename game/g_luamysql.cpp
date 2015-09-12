#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_lua.h"

#ifdef JPLUA

#if !defined(NO_SQL)

static const char* MySQLLib = "mysql.meta";
static const char* MySQLResult = "mysqlres.meta";

MYSQL *JPLua_CheckMySQL(lua_State *L, int idx) {
	void *ud = luaL_checkudata(L, idx, MySQLLib);
	luaL_argcheck(L, ud != NULL, 1, "'MySQL' expected");
	return (MYSQL *)ud;
}

MYSQL_RES **JPLua_CheckMySQLResult(lua_State *L, int idx) {
	void *ud = luaL_checkudata(L, idx, MySQLResult);
	luaL_argcheck(L, ud != NULL, 1, "'MySQLResult' expected");
	return (MYSQL_RES **)ud;
}

void JPLua_MySQLResult_CreateRef(lua_State *L,MYSQL *sql , MYSQL_RES **data){

	data = (MYSQL_RES**)lua_newuserdata(L, sizeof(MYSQL_RES*));
	*data = mysql_store_result(sql);
	luaL_getmetatable(L, MySQLResult);
	lua_setmetatable(L, -2);

}

static int JPLua_MySQL_ToString(lua_State *L){
	MYSQL *data = JPLua_CheckMySQL(L, 1);
	lua_pushfstring( L, "MySQL Connection (IP: %s || Database: %s)", data->host, data->db );
	return 1;
}

static int JPLua_MySQLResult_ToString(lua_State *L){
	MYSQL_RES **data = JPLua_CheckMySQLResult(L,1);
	lua_pushfstring( L, "MySQL Result (Rows: %d )", mysql_num_rows(*data));

	return 1;
}


int JPLua_MySQL_Open(lua_State *L)
{
	MYSQL *conn;
	const char *host = luaL_checkstring(L,2);
	const char *user = luaL_checkstring(L,3);
	const char *db = luaL_checkstring(L,4);
	const char *password = luaL_checkstring(L,5);
	int port;
	if ( !lua_isnil( L, 6 ) )
		port = 3306;
	else
		port = lua_tointeger(L,6);

	conn = (MYSQL *)lua_newuserdata(L, sizeof(MYSQL));


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

static int JPLua_MySQL_Ping(lua_State *L)
{
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	lua_pushinteger(L, mysql_ping(conn));

	return 1;
}

static int JPLua_MySQL_SelectDB(lua_State *L)
{
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	const char* db = luaL_checkstring(L, 2);

	if (mysql_select_db(conn, db) == 0){
		lua_pushnil(L);
	}
	else{
		lua_pushstring(L, mysql_error(conn));
	}

	return 1;
}

static int JPLua_MySQL_SetCharSet(lua_State *L)
{
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	const char* charset = luaL_checkstring(L, 2);

	if (mysql_set_character_set(conn, charset) == 0)
		lua_pushnil(L);
	else
		lua_pushstring(L, mysql_error(conn));

	return 1;
}

static int JPLua_MySQL_Escape(lua_State *L)
{
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	size_t len = 0;
	const char* content = luaL_checklstring(L, 2, &len);
	char* buf = NULL;

	if (len <= 0) {
		lua_pushnil(L);
		return 1;
	}

	buf = (char *)malloc(len);
	if (!buf) {
		lua_pushnil(L);
		return 1;
	}

	len = mysql_real_escape_string(conn, buf, content, len);
	lua_pushlstring(L, buf, len);
	free(buf);
	return 1;
}

static int JPLua_MySQL_Query(lua_State *L)
{
	int err;
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	size_t len;
	MYSQL_RES **result = NULL;
	const char *query = luaL_checklstring(L, 2, &len);

	if (len <= 0) {
		lua_pushnil(L);
		return 1;
	}

	err = mysql_real_query(conn, query, len);
	if (err) {
		lua_pushnil(L);
		trap->Print("MySQL Error: %s", mysql_error(conn));
	}
	else {
		JPLua_MySQLResult_CreateRef(L, conn, result);
	}

	return 1;
}

static int JPLua_MySQL_gc(lua_State *L)
{
	MYSQL *conn = JPLua_CheckMySQL(L, 1);
	mysql_close(conn);
	return 0;
}

static int JPLua_MySQLResult_Count(lua_State *L)
{
	MYSQL_RES **result = JPLua_CheckMySQLResult(L, 1);
	lua_pushinteger(L, mysql_num_rows(*result));
	return 1;
}

static int JPLua_MySQLResult_Get(lua_State *L){
	int i,top, temptop;
	MYSQL_RES **result = JPLua_CheckMySQLResult(L, 1);
	MYSQL_ROW row = NULL;
	MYSQL_FIELD *fieldlist = mysql_fetch_fields(*result);
	int fieldcount = mysql_num_fields(*result);
	int count = 1;

	lua_newtable(L);
	top = lua_gettop(L);
	while ( (row = mysql_fetch_row( *result )) ) {
		lua_pushinteger(L, count);
		lua_newtable(L);
		temptop = lua_gettop(L);
		for (i = 0; i < fieldcount; i++){
			lua_pushstring(L, fieldlist[i].name); lua_pushstring(L, row[i]); lua_settable(L, temptop);
		}
		lua_settable(L, top);
		count++;
	}

	return 1;
}

static int JPLua_MySQLResult_gc(lua_State* L)
{
	MYSQL_RES **result = JPLua_CheckMySQLResult(L, 1);

	if (result)
		mysql_free_result(*result);

	return 0;
}

#if 0
static const struct luaL_Reg mysqlclient_f[] = {
	{ "connect", JPLua_MySQL_Open },
	{ NULL, NULL },
};
#endif

static const struct luaL_Reg mysqlclient_m[] = {
	{ "__gc", JPLua_MySQL_gc },
	{"__tostring", JPLua_MySQL_ToString},
	{ "Ping", JPLua_MySQL_Ping },
	{ "Selectdb", JPLua_MySQL_SelectDB },
	{ "Setcharset", JPLua_MySQL_SetCharSet },
	{ "Escape", JPLua_MySQL_Escape },
	{ "Query", JPLua_MySQL_Query },
	{ NULL, NULL },
};

static const struct luaL_Reg mysqlresult_lib[] = {
	{ "__gc", JPLua_MySQLResult_gc },
	{"__tostring", JPLua_MySQLResult_ToString},
	{ "Count", JPLua_MySQLResult_Count },
	{ "Get", JPLua_MySQLResult_Get },
	{ NULL, NULL },
};

void JPLua_Register_MySQL(lua_State* L){
	const luaL_Reg *r;

	luaL_newmetatable(L, MySQLLib);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	for (r = mysqlclient_m; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	lua_pop(L, -1);

	luaL_newmetatable(L, MySQLResult);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	for (r = mysqlresult_lib; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}


}

#endif // !NO_SQL

#endif