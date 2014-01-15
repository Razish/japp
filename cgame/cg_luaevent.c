#include "cg_local.h"
#include "cg_lua.h"

#ifdef JPLUA

//================================
//		EVENT HANDLERS
//================================

stringID_table_t jplua_events[JPLUA_EVENT_MAX] = {
	ENUM2STRING(JPLUA_EVENT_UNLOAD),
	ENUM2STRING(JPLUA_EVENT_RUNFRAME),
	ENUM2STRING(JPLUA_EVENT_HUD),
	ENUM2STRING(JPLUA_EVENT_CHATMSGRECV),
	ENUM2STRING(JPLUA_EVENT_CHATMSGSEND),
	ENUM2STRING(JPLUA_EVENT_CLIENTCONNECT),
	ENUM2STRING(JPLUA_EVENT_CLIENTINFO),
	ENUM2STRING(JPLUA_EVENT_PAIN),
	ENUM2STRING(JPLUA_EVENT_SABERTOUCH),
};

// called by Lua
int JPLua_Event_AddListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, -2 );

	if ( lua_type( L, -1 ) != LUA_TFUNCTION || lua_type( L, -2 ) != LUA_TSTRING ) {
		trap->Print( "JPLua: AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	// loop 'til we find the event we want to listen for
	for ( i=0; i<JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			JPLua.currentPlugin->eventListeners[i] = luaL_ref( L, LUA_REGISTRYINDEX );
			return 0;
		}
	}

	trap->Print( "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
	return 0;
}

// called by Lua
int JPLua_Event_RemoveListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, -1 );

	if ( lua_type( L, -1 ) != LUA_TSTRING ) {
		trap->Print( "JPLua: RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	// loop 'til we find the event we want to remove the listener for
	for ( i=0; i<JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			luaL_unref( L, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[i] );
			JPLua.currentPlugin->eventListeners[i] = 0;
			return 0;
		}
	}

	trap->Print( "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
	return 0;
}

#endif // JPLUA

void JPLua_Event_Shutdown( void ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		jplua_plugin_command_t *consoleCmd = JPLua.currentPlugin->consoleCmds, *nextConsoleCmd = consoleCmd;
		jplua_plugin_command_t *serverCmd = JPLua.currentPlugin->serverCmds, *nextServerCmd = serverCmd;

		// fire the unload event
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] );
			JPLUACALL( JPLua.state, 0, 0 );
		}

		//RAZTODO: Refcount because multiple plugins can register the same command
		//Remove all console commands
		while ( nextConsoleCmd ) {
			lua_unref( JPLua.state, consoleCmd->handle );
			nextConsoleCmd = consoleCmd->next;

			free( consoleCmd );
			consoleCmd = nextConsoleCmd;
		}

		// remove all server commands
		while ( nextServerCmd ) {
			lua_unref( JPLua.state, serverCmd->handle );
			nextServerCmd = serverCmd->next;

			free( serverCmd );
			serverCmd = nextServerCmd;
		}

		JPLua.currentPlugin->consoleCmds = NULL;
		JPLua.currentPlugin->serverCmds = NULL;
	}
#endif // JPLUA
}

void JPLua_Event_RunFrame( void ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] );
			JPLUACALL( JPLua.state, 0, 0 );
		}
	}
#endif // JPLUA
}

qboolean JPLua_Event_HUD( void ) {
	qboolean ret = qfalse;

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_HUD] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_HUD] );
			JPLUACALL( JPLua.state, 0, 1 );
			if ( !ret )
				ret = !!lua_tointeger( JPLua.state, -1 );
		}
	}
#endif //JPLUA

	return ret;
}

char *JPLua_Event_ChatMessageRecieved( const char *msg ) {
	static char tmpMsg[MAX_SAY_TEXT] = {0}; // although a chat message can only be MAX_SAY_TEXT long..-name?

	Q_strncpyz( tmpMsg, msg, MAX_SAY_TEXT );

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] );

			lua_pushstring( JPLua.state, tmpMsg );
			JPLUACALL( JPLua.state, 1, 1 );

			// returned nil, no use passing it to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
				return NULL;
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGRECV), expected string or nil but got %s", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
		}
	}
#endif // JPLUA

	return tmpMsg;
}

char *JPLua_Event_ChatMessageSent( const char *msg ) {
	static char tmpMsg[MAX_TOKEN_CHARS] = {0}; // although a chat message can only be MAX_SAY_TEXT long..-name?

	Q_strncpyz( tmpMsg, msg, sizeof( tmpMsg ) );

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] );

			lua_pushstring( JPLua.state, tmpMsg );
			JPLUACALL( JPLua.state, 1, 1 );

			// returned nil, no use passing it to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
				return NULL;
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGSEND), expected string or nil but got %s", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
		}
	}
#endif // JPLUA

	return tmpMsg;
}

void JPLua_Event_ClientConnect( int clientNum ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLUACALL( JPLua.state, 1, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTINFO] ) {
			int top1, top2, i;

			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTINFO] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			for ( i=0; i<2; i++ ) {
				clientInfo_t *ci = i ? newInfo : oldInfo;
				lua_newtable( JPLua.state );
				top1 = lua_gettop( JPLua.state );

				lua_pushstring( JPLua.state, "colorOverride" );
				lua_newtable( JPLua.state ); top2 = lua_gettop( JPLua.state );
				lua_pushstring( JPLua.state, "r" );				lua_pushnumber( JPLua.state, ci->colorOverride.r );	lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "g" );				lua_pushnumber( JPLua.state, ci->colorOverride.g );	lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "b" );				lua_pushnumber( JPLua.state, ci->colorOverride.b );	lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "a" );				lua_pushnumber( JPLua.state, ci->colorOverride.a );	lua_settable( JPLua.state, top2 );
				lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "saberName" );		lua_pushstring( JPLua.state, ci->saberName );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "saber2Name" );	lua_pushstring( JPLua.state, ci->saber2Name );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "name" );			lua_pushstring( JPLua.state, ci->name );			lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "team" );			lua_pushnumber( JPLua.state, ci->team );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "duelTeam" );		lua_pushnumber( JPLua.state, ci->duelTeam );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "botSkill" );		lua_pushnumber( JPLua.state, ci->botSkill );		lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "color1" );
				lua_newtable( JPLua.state ); top2 = lua_gettop( JPLua.state );
				lua_pushstring( JPLua.state, "r" );				lua_pushnumber( JPLua.state, ci->color1.r );		lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "g" );				lua_pushnumber( JPLua.state, ci->color1.g );		lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "b" );				lua_pushnumber( JPLua.state, ci->color1.b );		lua_settable( JPLua.state, top2 );
				lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "color2" );
				lua_newtable( JPLua.state ); top2 = lua_gettop( JPLua.state );
				lua_pushstring( JPLua.state, "r" );				lua_pushnumber( JPLua.state, ci->color2.r );		lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "g" );				lua_pushnumber( JPLua.state, ci->color2.g );		lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "b" );				lua_pushnumber( JPLua.state, ci->color2.b );		lua_settable( JPLua.state, top2 );
				lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "rgb1" );
				lua_newtable( JPLua.state ); top2 = lua_gettop( JPLua.state );
				lua_pushstring( JPLua.state, "r" );				lua_pushnumber( JPLua.state, ci->rgb1.r );			lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "g" );				lua_pushnumber( JPLua.state, ci->rgb1.g );			lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "b" );				lua_pushnumber( JPLua.state, ci->rgb1.b );			lua_settable( JPLua.state, top2 );
				lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "rgb2" );
				lua_newtable( JPLua.state ); top2 = lua_gettop( JPLua.state );
				lua_pushstring( JPLua.state, "r" );				lua_pushnumber( JPLua.state, ci->rgb2.r );			lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "g" );				lua_pushnumber( JPLua.state, ci->rgb2.g );			lua_settable( JPLua.state, top2 );
				lua_pushstring( JPLua.state, "b" );				lua_pushnumber( JPLua.state, ci->rgb2.b );			lua_settable( JPLua.state, top2 );
				lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "icolor1" );		lua_pushnumber( JPLua.state, ci->icolor1 );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "icolor2" );		lua_pushnumber( JPLua.state, ci->icolor2 );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "score" );			lua_pushnumber( JPLua.state, ci->score );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "location" );		lua_pushnumber( JPLua.state, ci->location );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "health" );		lua_pushnumber( JPLua.state, ci->health );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "armor" );			lua_pushnumber( JPLua.state, ci->armor );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "curWeapon" );		lua_pushnumber( JPLua.state, ci->curWeapon );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "handicap" );		lua_pushnumber( JPLua.state, ci->handicap );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "wins" );			lua_pushnumber( JPLua.state, ci->wins );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "losses" );		lua_pushnumber( JPLua.state, ci->losses );			lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "teamTask" );		lua_pushnumber( JPLua.state, ci->teamTask );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "powerups" );		lua_pushnumber( JPLua.state, ci->powerups );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "modelName" );		lua_pushstring( JPLua.state, ci->saberName );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "skinName" );		lua_pushstring( JPLua.state, ci->saberName );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "forcePowers" );	lua_pushstring( JPLua.state, ci->forcePowers );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "teamName" );		lua_pushstring( JPLua.state, ci->teamName );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "deferred" );		lua_pushboolean( JPLua.state, ci->deferred );		lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "gender" );		lua_pushnumber( JPLua.state, ci->gender );			lua_settable( JPLua.state, top1 );
			}

			JPLUACALL( JPLua.state, 3, 0 );
		}
	}
#endif
}

void JPLua_Event_Pain( int clientNum, int health ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushinteger( JPLua.state, health );

			JPLUACALL( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_SaberTouch( int victim, int attacker ) {
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_SABERTOUCH] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_SABERTOUCH] );

			JPLua_Player_CreateRef( JPLua.state, victim );
			JPLua_Player_CreateRef( JPLua.state, attacker );

			JPLUACALL( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}

qboolean JPLua_Event_ConsoleCommand( void ) {
	qboolean ret = qfalse;

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->consoleCmds;

		while ( cmd ) {
			int top = 0;
			int i = 0;

			if ( !Q_stricmp( CG_Argv( 0 ), cmd->command ) && cmd->handle ) {
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				// push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<trap->Cmd_Argc(); i++ ) {
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, CG_Argv( i ) );
					lua_settable( JPLua.state, top );
				}

				JPLUACALL( JPLua.state, 1, 0 );
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
	for ( JPLua.currentPlugin = JPLua.plugins; JPLua.currentPlugin; JPLua.currentPlugin = JPLua.currentPlugin->next ) {
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->serverCmds;

		while ( cmd ) {
			int top = 0;
			int i = 0;

			if ( !Q_stricmp( CG_Argv( 0 ), cmd->command ) ) {
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<trap->Cmd_Argc(); i++ ) {
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, CG_Argv( i ) );
					lua_settable( JPLua.state, top );
				}

				JPLUACALL( JPLua.state, 1, 1 );
				return qtrue;
			}

			cmd = cmd->next;
		}
	}
#endif // JPLUA

	return qfalse;
}
