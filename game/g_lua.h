#pragma once

#include "../lua/lua.h"
#include "../lua/lualib.h"
#include "../lua/lauxlib.h"

#include "g_luacvar.h"
#include "g_luaevent.h"
#include "g_luaplayer.h"

#ifdef JPLUA

void JPLua_Init( void );
void JPLua_Shutdown( void );

int JPLua_StackDump (lua_State *L);

int JPLua_Push_ToString( lua_State *L );
int JPLua_Push_Pairs( lua_State *L );

void JPLua_Util_ArgAsString( lua_State *L, char *out, int bufsize );
void JPLua_DPrintf( const char *msg, ... );

#define JPLUACALL( lua, argCount, resCount ) \
	if ( lua_pcall( lua, argCount, resCount, 0 ) ) { \
		char *err = (char *)lua_tostring( lua, -1 ); \
		Com_Printf( S_COLOR_GREEN"JPLua: "S_COLOR_RED"Error: %s\n", err ); \
		lua_pop( lua, 1 ); \
	}

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
	char name[32];
	char version[8];
	unsigned int requiredJPLuaVersion;
	intptr_t UID;
	int handle;

	int eventListeners[JPLUA_EVENT_MAX];

	jplua_plugin_command_t *clientCmds; //Linked list of client commands
	jplua_plugin_command_t *serverCmds; //Linked list of server commands

	struct jplua_plugin_s *next;
} jplua_plugin_t;

typedef struct jplua_s {
	lua_State *state;
	jplua_plugin_t *plugins;
	jplua_plugin_t *currentPlugin;
	unsigned int version;
	qboolean initialised;
} jplua_t;
extern jplua_t JPLua;

extern stringID_table_t jplua_events[JPLUA_EVENT_MAX];

#endif // JPLUA
