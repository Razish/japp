#include "g_local.h"
#include "g_lua.h"

#ifdef JPLUA

//================================
//		EVENT HANDLERS
//================================

static const stringID_table_t jplua_events[JPLUA_EVENT_MAX] = {
	ENUM2STRING(JPLUA_EVENT_UNLOAD),
	ENUM2STRING(JPLUA_EVENT_RUNFRAME),
	ENUM2STRING(JPLUA_EVENT_CLIENTCONNECT),
	ENUM2STRING(JPLUA_EVENT_CLIENTDISCONNECT),
	ENUM2STRING(JPLUA_EVENT_CLIENTBEGIN),
	ENUM2STRING(JPLUA_EVENT_CLIENTSPAWN),
	ENUM2STRING(JPLUA_EVENT_CLIENTCOMMAND),
	ENUM2STRING(JPLUA_EVENT_CLIENTUSERINFOCHANGED),
	ENUM2STRING(JPLUA_EVENT_PLAYERDEATH),
	ENUM2STRING(JPLUA_EVENT_PAIN),
};

// called by lua
int JPLua_Event_AddListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, 1 );

	if ( lua_type( L, 1 ) != LUA_TSTRING || lua_type( L, 2 ) != LUA_TFUNCTION ) {
		G_LogPrintf( level.log.console, "JPLua: AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	for ( i=0; i<JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			JPLua.currentPlugin->eventListeners[i] = luaL_ref( L, LUA_REGISTRYINDEX );
			return 0;
		}
	}

	G_LogPrintf( level.log.console, "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );

	return 0;
}

// called by lua
int JPLua_Event_RemoveListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, 1 );

	if ( lua_type( L, 1 ) != LUA_TSTRING ) {
		G_LogPrintf( level.log.console, "JPLua: RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	for ( i=0; i<JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			luaL_unref( L, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[i] );
			JPLua.currentPlugin->eventListeners[i] = 0;
			return 0;
		}
	}

	G_LogPrintf( level.log.console, "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );

	return 0;
}

#endif // JPLUA

void JPLua_Event_Shutdown( void ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *clientCmd = JPLua.currentPlugin->clientCmds, *nextClientCmd = clientCmd;
		jplua_plugin_command_t *serverCmd = JPLua.currentPlugin->serverCmds, *nextServerCmd = serverCmd;

		// fire the unload event
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] );
			JPLua_Call( JPLua.state, 0, 0 );
		}

		//RAZTODO: Refcount because multiple plugins can register the same command
		// remove all console commands
		while ( nextClientCmd ) {
			luaL_unref( JPLua.state, LUA_REGISTRYINDEX, clientCmd->handle );
			nextClientCmd = clientCmd->next;

			free( clientCmd );
			clientCmd = nextClientCmd;
		}

		// remove all server commands
		while ( nextServerCmd ) {
			luaL_unref( JPLua.state, LUA_REGISTRYINDEX, serverCmd->handle );
			nextServerCmd = serverCmd->next;

			free( serverCmd );
			serverCmd = nextServerCmd;
		}

		JPLua.currentPlugin->clientCmds = NULL;
		JPLua.currentPlugin->serverCmds = NULL;
	}
#endif // JPLUA
}

void JPLua_Event_RunFrame( void ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] );
			JPLua_Call( JPLua.state, 0, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_ClientSpawn( int clientNum, qboolean firstSpawn ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] );

			// Create a player instance for this client number and push on stack

			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushboolean( JPLua.state, firstSpawn );
			JPLua_Call( JPLua.state, 2, 0 );
		}
	}
#endif
}

const char *JPLua_Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

			lua_pushinteger( JPLua.state, clientNum );
			JPLua_PushInfostring( JPLua.state, userinfo );
			lua_pushstring( JPLua.state, IP );
			lua_pushboolean( JPLua.state, !!firstTime );

			JPLua_Call( JPLua.state, 4, 1 );

			// connection allowed, pass to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
				continue;

			// denied, no use passing it to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				return lua_tostring( JPLua.state, -1 );
			else {
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CLIENTCONNECT), expected string or nil but got %s\n", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
				return NULL;
			}
		}
	}
#endif
	return NULL;
}

void JPLua_Event_ClientDisconnect( int clientNum ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLua_Call( JPLua.state, 1, 0 );
		}
	}
#endif
}

void JPLua_Event_ClientBegin( int clientNum ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLua_Call( JPLua.state, 1, 0 );
		}
	}
#endif
}

qboolean JPLua_Event_ClientCommand( int clientNum ) {
	qboolean ret = qfalse;
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		int top, i, numArgs = trap->Argc();
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->clientCmds;

		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			//Push table of arguments
			lua_newtable( JPLua.state );
			top = lua_gettop( JPLua.state );
			for ( i=0; i<numArgs; i++ ) {
				char argN[MAX_TOKEN_CHARS];
				trap->Argv( i, argN, sizeof( argN ) );
				lua_pushnumber( JPLua.state, i+1 );
				lua_pushstring( JPLua.state, argN );
				lua_settable( JPLua.state, top );
			}

			JPLua_Call( JPLua.state, 2, 0 );
		}

		while ( cmd ) {
			char arg1[MAX_TOKEN_CHARS];

			trap->Argv( 0, arg1, sizeof( arg1 ) );

			if ( !Q_stricmp( arg1, cmd->command ) ) {
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				JPLua_Player_CreateRef( JPLua.state, clientNum );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<numArgs; i++ ) {
					char argN[MAX_TOKEN_CHARS];
					trap->Argv( i, argN, sizeof( argN ) );
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, argN );
					lua_settable( JPLua.state, top );
				}

				JPLua_Call( JPLua.state, 2, 0 );
				if ( !ret )
					ret = qtrue;
			}

			cmd = cmd->next;
		}
	}
#endif // JPLUA
	return ret;
}

qboolean JPLua_Event_ServerCommand( void ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->serverCmds;

		while ( cmd ) {
			int top, i, numArgs = trap->Argc();
			char arg1[MAX_TOKEN_CHARS];

			trap->Argv( 0, arg1, sizeof( arg1 ) );

			if ( !Q_stricmp( arg1, cmd->command ) ) {
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<numArgs; i++ ) {
					char argN[MAX_TOKEN_CHARS];
					trap->Argv( i, argN, sizeof( argN ) );
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, argN );
					lua_settable( JPLua.state, top );
				}

				JPLua_Call( JPLua.state, 1, 1 );
				return qtrue;
			}

			cmd = cmd->next;
		}
	}
#endif // JPLUA
	return qfalse;
}

qboolean JPLua_Event_ClientUserinfoChanged( int clientNum, char *userinfo ) {
	qboolean ret = qfalse;
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] );

			lua_pushinteger( JPLua.state, clientNum );
			JPLua_PushInfostring( JPLua.state, userinfo );

			JPLua_Call( JPLua.state, 2, 1 );

			// they wanted to modify it, parse it out and apply it
			if ( lua_type( JPLua.state, -1 ) == LUA_TTABLE ) {
				JPLua_PopInfostring( JPLua.state, userinfo );
				ret = qtrue;
			}
			else if ( lua_type( JPLua.state, -1 ) != LUA_TNIL )
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CLIENTUSERINFOCHANGED), expected table or nil but got %s\n", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
		}
	}
#endif
	return ret;
}

void JPLua_Event_PlayerDeath( int clientNum, int mod, int inflictor ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum ); // victim
			lua_pushinteger( JPLua.state, mod ); // method of death

			if ( inflictor >= MAX_CLIENTS || inflictor < 0 ) // -1 will hit this (which is passed in player_die if inflictor is not a player)
				lua_pushnil( JPLua.state ); // nil because not player
			else
				JPLua_Player_CreateRef( JPLua.state, inflictor );

			JPLua_Call( JPLua.state, 3, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] );

			JPLua_Player_CreateRef( JPLua.state, target );
			JPLua_Player_CreateRef( JPLua.state, inflictor );
			JPLua_Player_CreateRef( JPLua.state, attacker );
			lua_pushinteger( JPLua.state, health );
			lua_pushinteger( JPLua.state, armor );
			lua_pushinteger( JPLua.state, dflags );
			lua_pushinteger( JPLua.state, mod );

			JPLua_Call( JPLua.state, 7, 0 );
		}
	}
#endif // JPLUA
}
