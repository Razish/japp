#pragma once

#ifdef JPLUA

#define JPLUA_INTERNALS

extern "C" {
	#include "../lua/lua.h"
	#include "../lua/lualib.h"
	#include "../lua/lauxlib.h"
	#include "../lua/ldo.h"
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
	int traceback( lua_State *L );

	struct importTable_t {
		const char *name;
		lua_CFunction function;
	};

	typedef struct command_s {
		int handle;
		plugin_t *owner;
	}command_t;

	class StackCheck {
	private:
		lua_State *state;
		int top;

	public:
		StackCheck( lua_State *L ) : state(L), top( lua_gettop(state) ) {}
		StackCheck() = delete;
		StackCheck( const StackCheck& ) = delete;
		StackCheck& operator=( const StackCheck& ) = delete;
		~StackCheck() {
		#ifdef _DEBUG
			int newTop = lua_gettop( state );
			if ( newTop != top ) {
				trap->Print( "%s: top of stack %i -> %i\n", Q_FUNCTION_VERBOSE, top, newTop );
				traceback( state );
			}
			assert( top == newTop );
		#endif
		}
	};


} // namespace JPLua

#endif // JPLUA
