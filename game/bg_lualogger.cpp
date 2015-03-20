#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"
#include "bg_lualogger.h"

#ifdef JPLUA

static const char LOGGER_META[] = "Logger.meta";

#if defined(PROJECT_GAME)
static const char *pluginDir = "lua/sv/";
#elif defined(PROJECT_CGAME)
static const char *pluginDir = "lua/cl/";
#endif

//Func: GetLogger( string fileName )
//Retn: Logger object
int JPLua_GetLogger( lua_State *L ) {
	const char *path = NULL;

	//TODO: force writing to plugin directory?
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TSTRING, 1, "'string' expected" );
	path = lua_tostring( L, 1 );

	JPLua_Logger_CreateRef( L, path );
	return 1;
}

//Func: tostring( Logger )
//Retn: string representing the Logger instance (for debug/error messages)
static int JPLua_Logger_ToString( lua_State *L ) {
	jplua_logger_t *logger = JPLua_CheckLogger( L, 1 );
	lua_pushfstring( L, "Logger(%s)", logger->fileName );
	return 1;
}

//Func: Write( string message )
//Retn: --
int JPLua_Logger_Write( lua_State *L ) {
	jplua_logger_t *logger = JPLua_CheckLogger( L, 1 );

	if ( lua_type( L, 2 ) != LUA_TSTRING ) {
		luaL_argcheck( L, 1, 2, "'string' expected" );
	}

#if defined(PROJECT_GAME)
	G_LogPrintf( logger->fileHandle, "%s\n", lua_tostring( L, 2 ) );
#elif defined(PROJECT_CGAME)
	CG_LogPrintf( logger->fileHandle, "%s\n", lua_tostring( L, 2 ) );
#endif

	return 0;
}

//Func: Logger:Close()
//Retn: --
int JPLua_Logger_Close( lua_State *L ) {
	jplua_logger_t *logger = JPLua_CheckLogger( L, 1 );

	trap->FS_Close( logger->fileHandle );
	logger->fileHandle = NULL_FILE;
	logger->fileName[0] = '\0';

	return 0;
}

// Push a Logger instance onto the stack
void JPLua_Logger_CreateRef( lua_State *L, const char *path ) {
	const fsMode_t mode = FS_APPEND;
	jplua_logger_t *logger = NULL;

	logger = (jplua_logger_t *)lua_newuserdata( L, sizeof(jplua_logger_t) );
	Com_sprintf( logger->fileName, sizeof(logger->fileName), "%s%s/%s", pluginDir, JPLua.currentPlugin->name, path );
	trap->FS_Open( logger->fileName, &logger->fileHandle, mode );

	luaL_getmetatable( L, LOGGER_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Logger instance,
// Return the instance if it is, otherwise return NULL.
jplua_logger_t *JPLua_CheckLogger( lua_State *L, int idx ) {
	void *ud = luaL_checkudata( L, idx, LOGGER_META );
	luaL_argcheck( L, ud != NULL, 1, "'Logger' expected" );
	return (jplua_logger_t *)ud;
}

static const struct luaL_Reg jplua_logger_meta[] = {
	{ "__tostring", JPLua_Logger_ToString },
	{ "Write", JPLua_Logger_Write },
	{ "Close", JPLua_Logger_Close },
	{ NULL, NULL }
};

// Register the Logger class for Lua
void JPLua_Register_Logger( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, LOGGER_META ); // Create metatable for Logger class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	// fill metatable with fields
	for ( r = jplua_logger_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Logger class metatable from the stack
}

#endif // JPLUA
