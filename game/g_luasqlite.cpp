#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_lua.h"

#ifdef JPLUA

static const char* SQLite = "sqlite.meta";

jplua_sqlite_t *JPLua_CheckSQLite(lua_State *L, int idx){
	void *ud = luaL_checkudata(L, idx, SQLite);
	luaL_argcheck(L, ud != NULL, 1, "'SQLite' expected");
	return (jplua_sqlite_t*)ud;
}

void JPLua_SQLite_CreateRef(lua_State *L, sqlite3 *db, const char *path){
	jplua_sqlite_t *data = NULL;

	data = (jplua_sqlite_t *)lua_newuserdata(L, sizeof(jplua_sqlite_t));
	Q_strncpyz(data->path, path, sizeof(data->path));
	data->handle = db;
	luaL_getmetatable(L, SQLite);
	lua_setmetatable(L, -2);
}

static int JPLua_SQLite_ToString(lua_State *L){
	jplua_sqlite_t *data = JPLua_CheckSQLite(L, 1);
	lua_pushfstring(L, "SQLite Database (Database: %s)", data->path);
	return 1;
}

int JPLua_SQLite_Open(lua_State *L){
	sqlite3 *db = NULL;
	int error;
	const char *dbpath = luaL_checkstring(L, 2);
	const char *path = va("japlus/%s", dbpath);

	if ((error = sqlite3_open(path, &db))){
		trap->Print("SQLite Connection Error: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		lua_pushnil(L);
		return 1;
	}
	JPLua_SQLite_CreateRef(L, db, path);
	return 1;
}

static int JPLua_SQLite_Close(lua_State *L){
	jplua_sqlite_t *data = JPLua_CheckSQLite(L, 1);
	sqlite3_close(data->handle);
	return 0;
}

static int JPLua_SQLite_IsOpen(lua_State *L){
	jplua_sqlite_t *data = JPLua_CheckSQLite(L, 1);
	lua_pushboolean(L, data->handle != NULL ? 1 : 0);
	return 1;
}

static int JPLua_SQLite_Query(lua_State *L){
    jplua_sqlite_t *data = JPLua_CheckSQLite(L, 1);
	const char *query = luaL_checkstring(L, 2);
	int error;
	int count;
	int row = 0;
	int top, temptop;
	sqlite3_stmt *stmt;
	if ((error = sqlite3_prepare_v2(data->handle, query, -1, &stmt, NULL)) != SQLITE_OK){
		trap->Print("SQLite Query Error: %s", sqlite3_errmsg(data->handle));
		sqlite3_finalize(stmt);
		sqlite3_close(data->handle);
		lua_pushnil(L);
		return 1;
	}
	count = sqlite3_column_count(stmt);
	lua_newtable(L);
	top = lua_gettop(L);
	while ((error = sqlite3_step(stmt)) != SQLITE_ROW && error != SQLITE_DONE){
		lua_pushinteger(L, row);
		lua_newtable(L); /// Row - Table
		temptop = lua_gettop(L);
		for (int i = 0; i < count; i++){
			const char *name = sqlite3_column_name(stmt, i);
			switch (sqlite3_column_type(stmt, i)){
				case SQLITE_INTEGER:
					lua_pushstring(L, name); lua_pushinteger(L, sqlite3_column_int(stmt, i)); lua_settable(L, temptop);
					break;
				case SQLITE_FLOAT:
					lua_pushstring(L, name); lua_pushnumber(L, sqlite3_column_double(stmt, i)); lua_settable(L, temptop);
					break;
				case SQLITE_BLOB:
					lua_pushstring(L, name); lua_pushstring(L, (const char *)sqlite3_column_blob(stmt, i)); lua_settable(L, temptop);
					break;
				case SQLITE_NULL:
					lua_pushstring(L, name); lua_pushnil(L); lua_settable(L, temptop);
					break;
				case SQLITE3_TEXT:
					lua_pushstring(L, name); lua_pushstring(L, (const char *)sqlite3_column_text(stmt, i)); lua_settable(L, temptop);
					break;
				default:
					break;
			}
			lua_settable(L, top);


		}
		row++;
	}
	sqlite3_finalize(stmt);
	return 1;
}

static const struct luaL_Reg jplua_sqlite_meta[] = {
	{ "__gc", JPLua_SQLite_Close },
	{ "__tostring", JPLua_SQLite_ToString },
	{ "Close", JPLua_SQLite_Close },
	{ "IsOpen", JPLua_SQLite_IsOpen },
	{ "Query", JPLua_SQLite_Query },
	{ NULL, NULL },
};

void JPLua_Register_SQLite(lua_State *L){
	const luaL_Reg *r;

	luaL_newmetatable(L, SQLite);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	for (r = jplua_sqlite_meta; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	lua_pop(L, -1);

}

#endif