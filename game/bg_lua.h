#pragma once

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

#include "bg_luacvar.h"
#include "bg_luaevent.h"
#include "bg_luaplayer.h"
#ifdef _CGAME
#include "cg_luaserver.h"
#endif
#include "bg_luavector.h"
#include "bg_luafs.h"

#ifdef JPLUA

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

typedef struct jplua_plugin_command_s {
	char command[64];
	int handle;
	struct jplua_plugin_command_s *next;
} jplua_plugin_command_t;

typedef struct jplua_plugin_s {
	char name[32], longname[32];
	char version[8];
	unsigned int requiredJPLuaVersion;
	intptr_t UID;
	int handle;

	int eventListeners[JPLUA_EVENT_MAX]; // references to listener functions in lua stored in the registry

#ifdef _CGAME
	jplua_plugin_command_t *consoleCmds; //Linked list of console commands
	jplua_plugin_command_t *serverCmds; //Linked list of server commands
#else
	jplua_plugin_command_t *clientCmds; // linked list of client commands
	jplua_plugin_command_t *serverCmds; // linked list of server commands
#endif

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
