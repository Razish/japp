#pragma once

#ifdef JPLUA

#define JPLUA_INTERNALS

extern "C" {
	#include "../lua/lua.h"
	#include "../lua/lualib.h"
	#include "../lua/lauxlib.h"
}

#include "game/bg_lua.h"

namespace JPLua {

	#define JPLUA_EXTENSION ".lua"

	extern struct luaState_t {
		bool initialised;
		plugin_t *plugins, *currentPlugin;

		lua_State *L;
	} ls;

	void DPrintf( const char *msg, ... );

	// these all act on a given lua state
	qboolean Call( lua_State *L, int argCount, int resCount );
	int StackDump( lua_State *L );
	int Push_ToString( lua_State *L );
	int Push_Pairs( lua_State *L );
	void ArgAsString( lua_State *L, char *out, int bufsize );
	void ReadVector( float *out, int numComponents, lua_State *L, int idx );
	void ReadColour( float *out, int numComponents, lua_State *L, int idx );
	void ReadFloats( float *out, int numComponents, lua_State *L, int idx );
	void PushInfostring( lua_State *L, const char *info );
	void PopInfostring( lua_State *L, char *info );

	struct importTable_t {
		const char *name;
		lua_CFunction function;
	};

	typedef struct command_s {
		int handle;
		plugin_t *owner;
	}command_t;


} // namespace JPLua

#endif // JPLUA
