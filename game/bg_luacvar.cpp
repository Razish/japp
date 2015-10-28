#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif
#include "bg_luainternal.h"

#ifdef JPLUA

#include <unordered_map>

namespace JPLua {

	static const char CVAR_META[] = "Cvar.meta";

	//Func: CreateCvar(name [, value [, flags] ])
	//Retn: An Cvar object, creating one if necessary
	int CreateCvar( lua_State *L ) {
		const char *name = lua_tostring( L, 1 );

		trap->Cvar_Register( NULL, name, lua_tostring( L, 2 ), lua_tointeger( L, 3 ) );

		Cvar_CreateRef( L, name );

		return 1;
	}

	//Func: GetCvar(name)
	//Retn: nil if Cvar does not exist
	//		Cvar object if Cvar already exists
	int GetCvar( lua_State *L ) {
		Cvar_CreateRef( L, lua_tostring( L, 1 ) );
		return 1;
	}

	//Func: tostring(Cvar)
	//Retn: string representing the Cvar instance (for debug/error messages)
	static int Cvar_ToString( lua_State *L ) {
		luaCvar_t *cvar = CheckCvar( L, 1 );

		if ( cvar ) {
			lua_pushfstring( L, "Cvar(%s)", cvar->name );
		}
		else {
			lua_pushstring( L, "Cvar(nil)" );
		}

		return 1;
	}

	//Func: Cvar:GetName()
	//Retn: string of the Cvar's name
	static int Cvar_GetName( lua_State *L ) {
		luaCvar_t *cvar = CheckCvar( L, 1 );

		if ( cvar ) {
			lua_pushstring( L, cvar->name );
		}
		else {
			lua_pushnil( L );
		}

		return 1;
	}

	//Func: Cvar:GetDefault()
	//Retn: string of the Cvar's default value
	static int Cvar_GetDefault( lua_State *L ) {
		//	luaCvar_t *luaCvar = CheckCvar( L, 1 );

		//No way to get the cvar's default value without engine funcs
		//RAZTODO: search the local vmCvar table anyway?
		lua_pushnil( L );

		return 1;
	}

	//Func: Cvar:GetFlags()
	//Retn: bit-mask of the Cvar's behaviour flags
	static int Cvar_GetFlags( lua_State *L ) {
		//	luaCvar_t *luaCvar = CheckCvar( L, 1 );

		// No way to get the cvar's flags without engine funcs
		//RAZTODO: search the local vmCvar table anyway?
		lua_pushnil( L );

		return 1;
	}

	//Func: Cvar:GetInteger()
	//Retn: integer of the Cvar's value
	static int Cvar_GetInteger( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
		if ( buf[0] ) {
			lua_pushinteger( L, atoi( buf ) );
		}
		else {
			lua_pushnil( L );
		}

		return 1;
	}

	//Func: Cvar:GetString()
	//Retn: string of the Cvar's value
	static int Cvar_GetString( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
		lua_pushstring( L, buf );

		return 1;
	}

	//Func: Cvar:GetFloat()
	//Retn: floating point number of the Cvar's value
	static int Cvar_GetFloat( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
		if ( buf[0] ) {
			lua_pushnumber( L, (lua_Number)atof( buf ) );
		}
		else {
			lua_pushnil( L );
		}

		return 1;
	}

	//Func: Cvar:GetBoolean()
	//Retn: boolean value of the Cvar's value
	static int Cvar_GetBoolean( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( luaCvar->name, buf, sizeof(buf) );
		if ( buf[0] ) {
			lua_pushboolean( L, atoi( buf ) ? 1 : 0 );
		}
		else {
			lua_pushnil( L );
		}

		return 1;
	}

	//Func: Cvar:Reset()
	//Retn: --
	static int Cvar_Reset( lua_State *L ) {
		//	luaCvar_t *luaCvar = CheckCvar( L, 1 );
		//RAZTODO: Search the local vmCvar table anyway?

		return 0;
	}

	//Func: Cvar:Set(value)
	//Retn: --
	static int Cvar_Set( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );

		if ( luaCvar ) {
			trap->Cvar_Set( luaCvar->name, lua_tostring( L, 2 ) );
		}

		return 0;
	}

	static std::unordered_map<std::string, int> updateList;

	static int Cvar_SetCallback( lua_State *L ) {
		luaCvar_t *luaCvar = CheckCvar( L, 1 );

		if ( luaCvar && lua_isfunction( L, 2 ) ) {
			updateList[luaCvar->name] = luaL_ref( L, LUA_REGISTRYINDEX );
		}
		return 0;
	}

	void Cvar_Update(const char *name){
		if ( !name ) {
			return;
		}
		int handle = updateList[name];
		if ( handle != 0 ) {
			lua_rawgeti( ls.L, LUA_REGISTRYINDEX, handle );
			Call( ls.L, 0, 0 );
		}
	}

	// Push a Cvar instance for a client number onto the stack
	void Cvar_CreateRef( lua_State *L, const char *name ) {
		luaCvar_t *luaCvar = NULL;
		char buf[MAX_CVAR_VALUE_STRING];

		trap->Cvar_VariableStringBuffer( name, buf, sizeof(buf) );
		//FIXME: check if cvar exists somehow? can't rely on empty strings, they are still legitimate cvars

		luaCvar = (luaCvar_t *)lua_newuserdata( L, sizeof(luaCvar_t) );
		Q_strncpyz( luaCvar->name, name, sizeof(luaCvar->name) );

		luaL_getmetatable( L, CVAR_META );
		lua_setmetatable( L, -2 );
	}

	// Ensure the value at the specified index is a valid Cvar instance,
	// Return the instance if it is, otherwise return NULL.
	luaCvar_t *CheckCvar( lua_State *L, int idx ) {
		void *ud = luaL_checkudata( L, idx, CVAR_META );
		luaL_argcheck( L, ud != NULL, 1, "'Cvar' expected" );
		return (luaCvar_t *)ud;
	}

	static const struct luaL_Reg luaCvarMeta[] = {
		{ "__tostring", Cvar_ToString },
		{ "GetName", Cvar_GetName },
		{ "GetDefault", Cvar_GetDefault },
		{ "GetFlags", Cvar_GetFlags },

		{ "GetInteger", Cvar_GetInteger },
		{ "GetString", Cvar_GetString },
		{ "GetFloat", Cvar_GetFloat },
		{ "GetBoolean", Cvar_GetBoolean },

		{ "Reset", Cvar_Reset },
		{ "Set", Cvar_Set },
		{ "SetUpdateCallback", Cvar_SetCallback },
		{ NULL, NULL }
	};

	// Register the Cvar class for Lua
	void Register_Cvar( lua_State *L ) {
		const luaL_Reg *r;

		luaL_newmetatable( L, CVAR_META ); // Create metatable for Cvar class, push on stack

		// Lua won't attempt to directly index userdata, only via metatables
		// Make this metatable's __index loopback to itself so can index the object directly
		lua_pushstring( L, "__index" );
		lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
		lua_settable( L, -3 ); // metatable.__index = metatable

		// fill metatable with fields
		for ( r = luaCvarMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}

		lua_pop( L, -1 ); // Pop the Cvar class metatable from the stack
	}

} // namespace JPLua

#endif // JPLUA
