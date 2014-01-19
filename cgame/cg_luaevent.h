#pragma once

#ifdef JPLUA

#include "../lua/lua.h"

typedef enum jplua_event_e {
	JPLUA_EVENT_UNLOAD=0,
	JPLUA_EVENT_RUNFRAME,
	JPLUA_EVENT_HUD,
	JPLUA_EVENT_CHATMSGRECV,
	JPLUA_EVENT_CHATMSGSEND,
	JPLUA_EVENT_CLIENTCONNECT,
	JPLUA_EVENT_CLIENTINFO,
	JPLUA_EVENT_PAIN,
	JPLUA_EVENT_SABERTOUCH,
	JPLUA_EVENT_MAX
} jplua_event_t;

int JPLua_Event_AddListener( lua_State *L );
int JPLua_Event_RemoveListener( lua_State *L );

#endif // JPLUA

void JPLua_Event_Shutdown( void );
void JPLua_Event_RunFrame( void );
qboolean JPLua_Event_HUD( void );
char *JPLua_Event_ChatMessageRecieved( const char *msg );
char *JPLua_Event_ChatMessageSent( const char *msg, messageMode_t mode, int targetClient );
void JPLua_Event_ClientConnect( int clientNum );
void JPLua_Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo );
void JPLua_Event_Pain( int clientNum, int health );
void JPLua_Event_SaberTouch( int victim, int attacker );

qboolean JPLua_Event_ServerCommand( void );
qboolean JPLua_Event_ConsoleCommand( void );
