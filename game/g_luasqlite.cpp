#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_luainternal.h"

#ifdef JPLUA

#if !defined(NO_SQL)

namespace JPLua {

	static const char* SQLite = "sqlite.meta";

	luaSQLite_t *CheckSQLite( lua_State *L, int idx ) {
		void *ud = luaL_checkudata( L, idx, SQLite );
		luaL_argcheck( L, ud != NULL, 1, "'SQLite' expected" );
		return (luaSQLite_t *)ud;
	}

	void SQLite_CreateRef( lua_State *L, sqlite3 *db, const char *path ) {
		luaSQLite_t *data = NULL;

		data = (luaSQLite_t *)lua_newuserdata( L, sizeof(luaSQLite_t) );
		Q_strncpyz( data->path, path, sizeof(data->path) );
		data->handle = db;
		luaL_getmetatable( L, SQLite );
		lua_setmetatable( L, -2 );
	}

	static int SQLite_ToString( lua_State *L ) {
		luaSQLite_t *data = CheckSQLite( L, 1 );
		lua_pushfstring( L, "SQLite Database (Database: %s)", data->path );
		return 1;
	}

	int SQLite_Open( lua_State *L ) {
		const char *dbpath = luaL_checkstring( L, 2 );
		const char *path = va( "japlus/%s", dbpath ); // FIXME: do this properly, maybe sqlite can take a file in memory

		sqlite3 *db = NULL;
		int error = sqlite3_open( path, &db );
		if ( error != SQLITE_OK ) {
			trap->Print( "SQLite Connection Error: %s", sqlite3_errmsg( db ) );
			sqlite3_close( db );
			lua_pushnil( L );
			return 1;
		}
		SQLite_CreateRef( L, db, path );
		return 1;
	}

	static int SQLite_Close( lua_State *L ) {
		luaSQLite_t *data = CheckSQLite( L, 1 );
		sqlite3_close( data->handle );
		return 0;
	}

	static int SQLite_IsOpen( lua_State *L ) {
		luaSQLite_t *data = CheckSQLite( L, 1 );
		lua_pushboolean( L, data->handle != NULL ? 1 : 0 );
		return 1;
	}

	static int SQLite_Query( lua_State *L ) {
		luaSQLite_t *data = CheckSQLite( L, 1 );
		const char *query = luaL_checkstring( L, 2 );

		sqlite3_stmt *stmt;
		int error = sqlite3_prepare_v2( data->handle, query, -1, &stmt, NULL );
		if ( error != SQLITE_OK ) {
			trap->Print( "SQLite Query Error: %s", sqlite3_errmsg( data->handle ) );
			sqlite3_finalize( stmt );
			sqlite3_close( data->handle );
			lua_pushnil( L );
			return 1;
		}

		int count = sqlite3_column_count( stmt );
		lua_newtable( L );
		int top = lua_gettop( L );
		int row = 0;
		while ( (error = sqlite3_step( stmt )) != SQLITE_ROW && error != SQLITE_DONE ) {
			lua_pushinteger( L, row );
			lua_newtable( L ); /// Row - Table
			int tempTop = lua_gettop( L );
			for ( int i = 0; i < count; i++ ) {
				const char *name = sqlite3_column_name( stmt, i );
				switch ( sqlite3_column_type( stmt, i ) ) {

				case SQLITE_INTEGER: {
					lua_pushstring( L, name );
						lua_pushinteger( L, sqlite3_column_int( stmt, i ) );
						lua_settable( L, tempTop );
				} break;

				case SQLITE_FLOAT: {
					lua_pushstring( L, name );
						lua_pushnumber( L, sqlite3_column_double( stmt, i ) );
						lua_settable( L, tempTop );
				} break;

				case SQLITE_BLOB: {
					lua_pushstring( L, name );
						lua_pushstring( L, (const char *)sqlite3_column_blob( stmt, i ) );
						lua_settable( L, tempTop );
				} break;

				case SQLITE_NULL: {
					lua_pushstring( L, name );
						lua_pushnil( L );
						lua_settable( L, tempTop );
				} break;

				case SQLITE3_TEXT: {
					lua_pushstring( L, name );
						lua_pushstring( L, (const char *)sqlite3_column_text( stmt, i ) );
						lua_settable( L, tempTop );
				} break;

				default: {
				} break;

				}
				lua_settable( L, top );
			}
			row++;
		}
		sqlite3_finalize( stmt );
		return 1;
	}

	static const struct luaL_Reg sqliteMeta[] = {
		{ "__gc", SQLite_Close },
		{ "__tostring", SQLite_ToString },
		{ "Close", SQLite_Close },
		{ "IsOpen", SQLite_IsOpen },
		{ "Query", SQLite_Query },
		{ NULL, NULL },
	};

	void Register_SQLite( lua_State *L ) {
		luaL_newmetatable( L, SQLite );

		lua_pushstring( L, "__index" );
		lua_pushvalue( L, -2 );
		lua_settable( L, -3 );

		for ( const luaL_Reg *r = sqliteMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}

		lua_pop( L, -1 );
	}

} // namespace JPLua

#endif // !NO_SQL

#endif // JPLUA
