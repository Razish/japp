#include "cg_local.h"
#include "bg_lua.h"

#ifdef JPLUA

static const char SERVER_META[] = "Server.meta";

//Func: GetServer()
//Retn: Server object if we have a valid snapshot
//		nil if we do not have a valid snapshot
int JPLua_GetServer( lua_State *L ) {
	if ( cg.snap ) {
		luaL_getmetatable( L, SERVER_META );
		lua_setmetatable( L, -2 );
	}
	else
		lua_pushnil( L );

	return 1;
}

//Func: tostring(Server)
//Retn: string representing the Server instance (for debug/error messages)
static int JPLua_Server_ToString( lua_State *L ) {
	lua_pushfstring( L, "Server(%s)", cgs.japp.serverName );
	return 1;
}

//Func: Server:GetName()
//Retn: string of the server's sv_hostname
static int JPLua_Server_GetName( lua_State *L ) {
	lua_pushstring( L, cgs.japp.serverName );
	return 1;
}

//Func: Server:GetSSF()
//Retn: 32bit integer bit-field of the server's support flags
static int JPLua_Server_GetSSF( lua_State *L ) {
	lua_pushinteger( L, cg.japp.SSF );
	return 1;
}

//Func: Server:GetPlayers()
//Retn: Indexed table of Player objects ordered by clientNum
static int JPLua_Server_GetPlayers( lua_State *L ) {
	int top, i = 1, clientNum;

	lua_newtable( L );
	top = lua_gettop( L );

	for ( clientNum=0; clientNum<MAX_CLIENTS; clientNum++ ) {
		if ( cgs.clientinfo[clientNum].infoValid ) {
			lua_pushnumber( L, i++ );
			JPLua_Player_CreateRef( L, clientNum );
			lua_settable( L, top );
		}
	}

	return 1;
}

static const struct luaL_Reg jplua_server_meta[] = {
	{ "__tostring",	JPLua_Server_ToString },
	{ "GetName",	JPLua_Server_GetName },
	{ "GetSSF",		JPLua_Server_GetSSF },
	{ "GetPlayers",	JPLua_Server_GetPlayers },
	{ NULL,			NULL }
};

// Register the Server class for Lua
void JPLua_Register_Server( lua_State *L ) {
	const luaL_Reg *r;
	luaL_newmetatable( L, SERVER_META ); // Create metatable for Server class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	// fill metatable with fields
	for ( r=jplua_server_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Server class metatable from the stack
}

#endif // JPLUA
