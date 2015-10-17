#pragma once

namespace JPLua {

#ifdef JPLUA
	enum event_t {
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
	};

#ifdef JPLUA_INTERNALS
	int Event_AddListener( lua_State *L );
	int Event_RemoveListener( lua_State *L );
#endif // JPLUA_INTERNALS

#endif

	void Event_Shutdown( qboolean restart );
	void Event_RunFrame( void );

#if defined(PROJECT_CGAME)
	char *Event_ChatMessageRecieved( const char *msg );
#elif defined(PROJECT_GAME)
	char *Event_ChatMessageRecieved( int clientNum, const char *msg, int type );
#endif

#ifdef PROJECT_CGAME
	char *Event_ChatMessageSent( const char *msg, messageMode_t mode, int targetClient );
#endif

#ifdef PROJECT_GAME
	void Event_ClientBegin( int clientNum );
#endif

#ifdef PROJECT_GAME
	qboolean Event_ClientCommand( int clientNum );
#endif

#if defined(PROJECT_GAME)
	const char *Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime );
#elif defined(PROJECT_CGAME)
	void Event_ClientConnect( int clientNum );
#endif

#ifdef PROJECT_GAME
	void Event_ClientDisconnect( int clientNum );
#endif

#ifdef PROJECT_CGAME
	void Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo );
#endif

#ifdef PROJECT_GAME
	void Event_ClientSpawn( int clientNum, qboolean firstSpawn );
#endif

#ifdef PROJECT_GAME
	qboolean Event_ClientUserinfoChanged( int clientNum, char *userinfo );
#endif

#ifdef PROJECT_CGAME
	qboolean Event_HUD( void );
#endif

#ifdef PROJECT_CGAME
	qboolean Event_VehicleHUD( void );
#endif

#ifdef PROJECT_CGAME
	qboolean Event_ConnectScreen( void );
#endif

#if defined(PROJECT_GAME)
	void Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod );
#elif defined(PROJECT_CGAME)
	void Event_Pain( int clientNum, int health );
#endif

#if defined(PROJECT_GAME)
	void Event_PlayerDeath( int clientNum, int mod, int inflictor );
#elif defined(PROJECT_CGAME)
	bool Event_PlayerDeath( int clientNum, int mod, int inflictor );
#endif

#ifdef PROJECT_CGAME
	void Event_SaberTouch( int victim, int attacker );
#endif

#ifdef PROJECT_CGAME
	qboolean Event_ConsoleCommand( void );
#endif

	qboolean Event_ServerCommand( void );

} // namespace JPLua
