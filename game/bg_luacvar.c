#if defined(_GAME)
#include "g_local.h"
#elif defined(_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

static const char CVAR_META[] = "Cvar.meta";

//Func: CreateCvar(name [, value [, flags] ])
//Retn: An Cvar object, creating one if necessary
int JPLua_CreateCvar( lua_State *L ) {
	const char *name = lua_tostring( L, 1 );

	trap->Cvar_Register( NULL, name, lua_tostring( L, 2 ), lua_tointeger( L, 3 ) );

	JPLua_Cvar_CreateRef( L, name );

	return 1;
}

//Func: GetCvar(name)
//Retn: nil if Cvar does not exist
//		Cvar object if Cvar already exists
int JPLua_GetCvar( lua_State *L ) {
	JPLua_Cvar_CreateRef( L, lua_tostring( L, 1 ) );
	return 1;
}

//Func: tostring(Cvar)
//Retn: string representing the Cvar instance (for debug/error messages)
static int JPLua_Cvar_ToString( lua_State *L ) {
	jplua_cvar_t *cvar = JPLua_CheckCvar( L, 1 );

	if ( cvar )
		lua_pushfstring( L, "Cvar(%s)", cvar->name );
	else
		lua_pushstring( L, "Cvar(nil)" );

	return 1;
}

//Func: Cvar:GetName()
//Retn: string of the Cvar's name
static int JPLua_Cvar_GetName( lua_State *L ) {
	jplua_cvar_t *cvar = JPLua_CheckCvar( L, 1 );

	if ( cvar )
		lua_pushstring( L, cvar->name );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Cvar:GetDefault()
//Retn: string of the Cvar's default value
static int JPLua_Cvar_GetDefault( lua_State *L ) {
	//	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );

	//No way to get the cvar's default value without engine funcs
	//RAZTODO: search the local vmCvar table anyway?
	lua_pushnil( L );

	return 1;
}

//Func: Cvar:GetFlags()
//Retn: bit-mask of the Cvar's behaviour flags
static int JPLua_Cvar_GetFlags( lua_State *L ) {
	//	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );

	// No way to get the cvar's flags without engine funcs
	//RAZTODO: search the local vmCvar table anyway?
	lua_pushnil( L );

	return 1;
}

//Func: Cvar:GetInteger()
//Retn: integer of the Cvar's value
static int JPLua_Cvar_GetInteger( lua_State *L ) {
	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );
	char buf[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
	if ( buf[0] )
		lua_pushinteger( L, atoi( buf ) );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Cvar:GetString()
//Retn: string of the Cvar's value
static int JPLua_Cvar_GetString( lua_State *L ) {
	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );
	char buf[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
	if ( buf[0] )
		lua_pushstring( L, buf );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Cvar:GetFloat()
//Retn: floating point number of the Cvar's value
static int JPLua_Cvar_GetFloat( lua_State *L ) {
	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );
	char buf[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
	if ( buf[0] )
		lua_pushnumber( L, (lua_Number)atof( buf ) );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Cvar:Reset()
//Retn: --
static int JPLua_Cvar_Reset( lua_State *L ) {
	//	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );
	//RAZTODO: Search the local vmCvar table anyway?

	return 0;
}

//Func: Cvar:Set(value)
//Retn: --
static int JPLua_Cvar_Set( lua_State *L ) {
	jplua_cvar_t *luaCvar = JPLua_CheckCvar( L, 1 );

	if ( luaCvar )
		trap->Cvar_Set( luaCvar->name, lua_tostring( L, 2 ) );

	return 0;
}

// Push a Cvar instance for a client number onto the stack
void JPLua_Cvar_CreateRef( lua_State *L, const char *name ) {
	jplua_cvar_t *luaCvar = NULL;
	char buf[MAX_CVAR_VALUE_STRING];

	trap->Cvar_VariableStringBuffer( name, buf, sizeof(buf) );
	//RAZFIXME: This isn't exactly reliable. Could be an empty cvar.
	if ( !buf[0] ) {
		lua_pushnil( L );
		return;
	}

	luaCvar = (jplua_cvar_t *)lua_newuserdata( L, sizeof(jplua_cvar_t) );
	Q_strncpyz( luaCvar->name, name, sizeof(luaCvar->name) );

	luaL_getmetatable( L, CVAR_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Cvar instance,
// Return the instance if it is, otherwise return NULL.
jplua_cvar_t *JPLua_CheckCvar( lua_State *L, int idx ) {
	void *ud = luaL_checkudata( L, idx, CVAR_META );
	luaL_argcheck( L, ud != NULL, 1, "'Cvar' expected" );
	return (jplua_cvar_t *)ud;
}

static const struct luaL_Reg jplua_cvar_meta[] = {
	{ "__tostring", JPLua_Cvar_ToString },
	{ "GetName", JPLua_Cvar_GetName },
	{ "GetDefault", JPLua_Cvar_GetDefault },
	{ "GetFlags", JPLua_Cvar_GetFlags },

	{ "GetInteger", JPLua_Cvar_GetInteger },
	{ "GetString", JPLua_Cvar_GetString },
	{ "GetFloat", JPLua_Cvar_GetFloat },

	{ "Reset", JPLua_Cvar_Reset },
	{ "Set", JPLua_Cvar_Set },
	{ NULL, NULL }
};

// Register the Cvar class for Lua
void JPLua_Register_Cvar( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, CVAR_META ); // Create metatable for Cvar class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	// fill metatable with fields
	for ( r = jplua_cvar_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Cvar class metatable from the stack
}

#endif // JPLUA
