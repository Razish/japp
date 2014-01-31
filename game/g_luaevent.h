#pragma once

#ifdef JPLUA

#include "../lua/lua.h"

typedef enum jplua_event_e {
	JPLUA_EVENT_UNLOAD=0,
	JPLUA_EVENT_RUNFRAME,
	JPLUA_EVENT_CLIENTCONNECT,
	JPLUA_EVENT_CLIENTDISCONNECT,
	JPLUA_EVENT_CLIENTSPAWN,
	JPLUA_EVENT_CLIENTCOMMAND,
	JPLUA_EVENT_MAX
} jplua_event_t;

int JPLua_Event_AddListener( lua_State *L );
int JPLua_Event_RemoveListener( lua_State *L );

#endif // JPLUA

void JPLua_Event_Shutdown( void );
void JPLua_Event_RunFrame( void );
const char *JPLua_Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime );
void JPLua_Event_ClientDisconnect( int clientNum );
void JPLua_Event_ClientSpawn( int clientNum, qboolean firstSpawn );
qboolean JPLua_Event_ClientCommand( int clientNum );
qboolean JPLua_Event_ServerCommand( void );
