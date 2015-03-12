#pragma once

extern "C" {
	#include "../lua/lua.h"
	#include "../lua/lualib.h"
	#include "../lua/lauxlib.h"
}

#include "bg_luacvar.h"
#include "bg_luaevent.h"
#include "bg_luaplayer.h"
#ifdef _CGAME
#include "cg_luaserver.h"
#endif
#include "bg_luavector.h"
#include "bg_luafs.h"
#ifdef _GAME
#include "g_luasql.h"
#endif

#ifdef JPLUA
#include <unordered_map>

void JPLua_Init( void );
void JPLua_Shutdown( void );
int JPLua_StackDump( lua_State *L );

int JPLua_Push_ToString( lua_State *L );
int JPLua_Push_Pairs( lua_State *L );

void JPLua_Util_ArgAsString( lua_State *L, char *out, int bufsize );
void JPLua_ReadVector( float *out, int numComponents, lua_State *L, int idx );
void JPLua_ReadColour( float *out, int numComponents, lua_State *L, int idx );
void JPLua_ReadFloats( float *out, int numComponents, lua_State *L, int idx );
void JPLua_DPrintf( const char *msg, ... );
qboolean JPLua_Call( lua_State *L, int argCount, int resCount );

void JPLua_PushInfostring( lua_State *L, const char *info );
void JPLua_PopInfostring( lua_State *L, char *info );

typedef struct jplua_cimport_table_s {
	const char *name;
	lua_CFunction function;
} jplua_cimport_table_t;

typedef struct jplua_plugin_s {
	char name[32], longname[32];
	char version[8];
	unsigned int requiredJPLuaVersion;
	intptr_t UID;
	int handle;

	int eventListeners[JPLUA_EVENT_MAX]; // references to listener functions in lua stored in the registry

	struct jplua_plugin_s *next;
} jplua_plugin_t;

qboolean JPLua_IteratePlugins( jplua_plugin_t **plugin );

typedef struct jplua_s {
	lua_State *state;
	jplua_plugin_t *plugins, *currentPlugin;
	qboolean initialised;
} jplua_t;
extern jplua_t JPLua;

extern const uint32_t JPLUA_VERSION;

#endif // JPLUA
