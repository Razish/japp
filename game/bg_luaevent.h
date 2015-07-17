#pragma once

#ifdef JPLUA

#include "../lua/lua.h"

typedef enum jplua_event_e {
	JPLUA_EVENT_UNLOAD = 0,

	JPLUA_EVENT_RUNFRAME,
	JPLUA_EVENT_CHATMSGRECV,
	JPLUA_EVENT_CHATMSGSEND,
	JPLUA_EVENT_CLIENTBEGIN,
	JPLUA_EVENT_CLIENTCOMMAND,
	JPLUA_EVENT_CLIENTCONNECT,
	JPLUA_EVENT_CLIENTDISCONNECT,
	JPLUA_EVENT_CLIENTINFO,
	JPLUA_EVENT_CLIENTSPAWN,
	JPLUA_EVENT_CLIENTUSERINFOCHANGED,
	JPLUA_EVENT_HUD,
	JPLUA_EVENT_VEHICLEHUD,
	JPLUA_EVENT_CONNECTSCREEN,
	JPLUA_EVENT_PAIN,
	JPLUA_EVENT_PLAYERDEATH,
	JPLUA_EVENT_SABERTOUCH,

	JPLUA_EVENT_MAX
} jplua_event_t;

int JPLua_Event_AddListener( lua_State *L );
int JPLua_Event_RemoveListener( lua_State *L );

#endif // JPLUA

void JPLua_Event_Shutdown(qboolean restart);
void JPLua_Event_RunFrame( void );

#if defined(PROJECT_CGAME)
char *JPLua_Event_ChatMessageRecieved(const char *msg);
#elif defined(PROJECT_GAME)
char *JPLua_Event_ChatMessageRecieved(int clientNum, const char *msg, int type );
#endif
#ifdef PROJECT_CGAME
char *JPLua_Event_ChatMessageSent( const char *msg, messageMode_t mode, int targetClient );
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientBegin( int clientNum );
qboolean JPLua_Event_ClientCommand( int clientNum );
#endif

#if defined(PROJECT_GAME)
const char *JPLua_Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime );
#elif defined(PROJECT_CGAME)
void JPLua_Event_ClientConnect( int clientNum );
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientDisconnect( int clientNum );
#endif

#ifdef PROJECT_CGAME
void JPLua_Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo );
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientSpawn( int clientNum, qboolean firstSpawn );
qboolean JPLua_Event_ClientUserinfoChanged( int clientNum, char *userinfo );
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_HUD( void );
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_VehicleHUD( void );
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_ConnectScreen( void );
#endif

#if defined(PROJECT_GAME)
void JPLua_Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod );
#elif defined(PROJECT_CGAME)
void JPLua_Event_Pain( int clientNum, int health );
#endif

#ifdef PROJECT_GAME
void JPLua_Event_PlayerDeath( int clientNum, int mod, int inflictor );
#endif

#ifdef PROJECT_CGAME
void JPLua_Event_SaberTouch( int victim, int attacker );
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_ConsoleCommand( void );
#endif

qboolean JPLua_Event_ServerCommand( void );
