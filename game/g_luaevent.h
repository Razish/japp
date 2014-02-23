#pragma once

#ifdef JPLUA

#include "../lua/lua.h"

typedef enum jplua_event_e {
	JPLUA_EVENT_UNLOAD=0,
	JPLUA_EVENT_RUNFRAME,
	JPLUA_EVENT_CLIENTCONNECT,
	JPLUA_EVENT_CLIENTDISCONNECT,
	JPLUA_EVENT_CLIENTBEGIN,
	JPLUA_EVENT_CLIENTSPAWN,
	JPLUA_EVENT_CLIENTCOMMAND,
	JPLUA_EVENT_CLIENTUSERINFOCHANGED,
	JPLUA_EVENT_PLAYERDEATH,
	JPLUA_EVENT_PAIN,
	JPLUA_EVENT_MAX
} jplua_event_t;

int JPLua_Event_AddListener( lua_State *L );
int JPLua_Event_RemoveListener( lua_State *L );

#endif // JPLUA

void JPLua_Event_Shutdown( void );
void JPLua_Event_RunFrame( void );
const char *JPLua_Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime );
void JPLua_Event_ClientDisconnect( int clientNum );
void JPLua_Event_ClientBegin( int clientNum );
void JPLua_Event_ClientSpawn( int clientNum, qboolean firstSpawn );
qboolean JPLua_Event_ClientCommand( int clientNum );
qboolean JPLua_Event_ServerCommand( void );
qboolean JPLua_Event_ClientUserinfoChanged( int clientNum, char *userinfo );
void JPLua_Event_PlayerDeath( int clientNum, int mod, int inflictor );
void JPLua_Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod );
