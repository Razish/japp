#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

#ifdef PROJECT_GAME
extern std::unordered_map<std::string, int> jplua_client_commands;
extern std::unordered_map<std::string, int> jplua_server_commands;
#elif defined PROJECT_CGAME
extern std::unordered_map<std::string, int> jplua_console_commands;
extern std::unordered_map<std::string, int> jplua_server_commands;
#endif

static const stringID_table_t jplua_events[JPLUA_EVENT_MAX] = {
	ENUM2STRING( JPLUA_EVENT_UNLOAD ),
	ENUM2STRING( JPLUA_EVENT_RUNFRAME ),
	ENUM2STRING( JPLUA_EVENT_CHATMSGRECV ),
	ENUM2STRING( JPLUA_EVENT_CHATMSGSEND ),
	ENUM2STRING( JPLUA_EVENT_CLIENTBEGIN ),
	ENUM2STRING( JPLUA_EVENT_CLIENTCOMMAND ),
	ENUM2STRING( JPLUA_EVENT_CLIENTCONNECT ),
	ENUM2STRING( JPLUA_EVENT_CLIENTDISCONNECT ),
	ENUM2STRING( JPLUA_EVENT_CLIENTINFO ),
	ENUM2STRING( JPLUA_EVENT_CLIENTSPAWN ),
	ENUM2STRING( JPLUA_EVENT_CLIENTUSERINFOCHANGED ),
	ENUM2STRING( JPLUA_EVENT_HUD ),
	ENUM2STRING( JPLUA_EVENT_VEHICLEHUD),
	ENUM2STRING( JPLUA_EVENT_CONNECTSCREEN),
	ENUM2STRING( JPLUA_EVENT_PAIN ),
	ENUM2STRING( JPLUA_EVENT_PLAYERDEATH ),
	ENUM2STRING( JPLUA_EVENT_SABERTOUCH ),
};

// called by lua
int JPLua_Event_AddListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, 1 );

	if ( lua_type( L, 1 ) != LUA_TSTRING || lua_type( L, 2 ) != LUA_TFUNCTION ) {
#if defined(PROJECT_GAME)
		G_LogPrintf( level.log.console, "JPLua: AddListener failed, function signature invalid registering %s (plugin: "
			"%s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
#elif defined(PROJECT_CGAME)
		trap->Print( "JPLua: AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up to "
			"date?\n", listenerArg, JPLua.currentPlugin->name );
#endif
		return 0;
	}

	for ( i = 0; i < JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			JPLua.currentPlugin->eventListeners[i] = luaL_ref( L, LUA_REGISTRYINDEX );
			return 0;
		}
	}

#if defined(PROJECT_GAME)
	G_LogPrintf( level.log.console, "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n",
		listenerArg, JPLua.currentPlugin->name );
#elif defined(PROJECT_CGAME)
	trap->Print( "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n",
		listenerArg, JPLua.currentPlugin->name );
#endif

	return 0;
}

// called by lua
int JPLua_Event_RemoveListener( lua_State *L ) {
	int i = 0;
	const char *listenerArg = lua_tostring( L, 1 );

	if ( lua_type( L, 1 ) != LUA_TSTRING ) {
#if defined(PROJECT_GAME)
		G_LogPrintf( level.log.console, "JPLua: RemoveListener failed, function signature invalid registering %s (plugin:"
			" %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
#elif defined(PROJECT_CGAME)
		trap->Print( "JPLua: RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it up to"
			" date?\n", listenerArg, JPLua.currentPlugin->name );
#endif
		return 0;
	}

	for ( i = 0; i < JPLUA_EVENT_MAX; i++ ) {
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) ) {
			luaL_unref( L, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[i] );
			JPLua.currentPlugin->eventListeners[i] = 0;
			return 0;
		}
	}

#if defined(PROJECT_GAME)
	G_LogPrintf( level.log.console, "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to "
		"date?\n", listenerArg, JPLua.currentPlugin->name );
#elif defined(PROJECT_CGAME)
	trap->Print( "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n",
		listenerArg, JPLua.currentPlugin->name );
#endif

	return 0;
}

#endif // JPLUA

void JPLua_Event_Shutdown( void ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while (JPLua_IteratePlugins(&plugin)) {
		// fire the unload event
		if (plugin->eventListeners[JPLUA_EVENT_UNLOAD]) {
			lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_UNLOAD]);
			JPLua_Call(JPLua.state, 0, 0);
		}
	}
#endif
}


void JPLua_Event_RunFrame( void ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_RUNFRAME] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_RUNFRAME] );
			JPLua_Call( JPLua.state, 0, 0 );
		}
	}
#endif // JPLUA
}

#if defined(PROJECT_CGAME)
char *JPLua_Event_ChatMessageRecieved( const char *msg ) {
#elif defined(PROJECT_GAME)
char *JPLua_Event_ChatMessageRecieved(int clientNum, const char *msg, int type ) {
#endif
	static char tmpMsg[MAX_SAY_TEXT] = { 0 }; // although a chat message can only be MAX_SAY_TEXT long..-name?
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
#endif

	Q_strncpyz( tmpMsg, msg, MAX_SAY_TEXT );

#ifdef JPLUA
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] );

#if defined(PROJECT_CGAME)
			lua_pushstring( JPLua.state, tmpMsg );
			JPLua_Call( JPLua.state, 1, 1 );
#elif defined(PROJECT_GAME)
			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushstring( JPLua.state, tmpMsg );
			lua_pushinteger(JPLua.state, type);
			JPLua_Call( JPLua.state, 3, 1 );
#endif

			// returned nil, no use passing it to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
				return NULL;
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else {
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGRECV), expected string or nil but got %s",
					plugin->name, lua_typename( JPLua.state, -1 ) );
			}
		}
	}
#endif // JPLUA

	return tmpMsg;
}

#ifdef PROJECT_CGAME
char *JPLua_Event_ChatMessageSent( const char *msg, messageMode_t mode, int targetClient ) {
	static char tmpMsg[MAX_STRING_CHARS] = { 0 }; // although a chat message can only be MAX_SAY_TEXT long..-name?
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
#endif

	Q_strncpyz( tmpMsg, msg, sizeof(tmpMsg) );

#ifdef JPLUA
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] );

			lua_pushstring( JPLua.state, tmpMsg );
			lua_pushinteger( JPLua.state, mode );
			lua_pushinteger( JPLua.state, targetClient );
			JPLua_Call( JPLua.state, 3, 1 );

			// returned nil, no use passing it to other plugins
			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
				return NULL;
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else {
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGSEND), expected string or nil but got %s\n",
					plugin->name, lua_typename( JPLua.state, -1 ) );
				return NULL;
			}
		}
	}
#endif // JPLUA

	return tmpMsg;
}
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientBegin( int clientNum ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] );

			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLua_Call( JPLua.state, 1, 0 );
		}
	}
#endif
}
#endif

#ifdef PROJECT_GAME
qboolean JPLua_Event_ClientCommand(int clientNum){
	qboolean ret = qfalse;
#ifdef JPLUA
	int top, i, numArgs = trap->Argc();
	char cmd[MAX_TOKEN_CHARS] = { '\0' };
	trap->Argv(0, cmd, sizeof(cmd));
	jplua_plugin_t *plugin = NULL;
	while (JPLua_IteratePlugins(&plugin)) {
		if (plugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND]) {
			lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND]);

			JPLua_Player_CreateRef(JPLua.state, clientNum);

			//Push table of arguments
			lua_newtable(JPLua.state);
			top = lua_gettop(JPLua.state);
			for (i = 0; i < numArgs; i++) {
				char argN[MAX_TOKEN_CHARS];
				trap->Argv(i, argN, sizeof(argN));
				lua_pushnumber(JPLua.state, i + 1);
				lua_pushstring(JPLua.state, argN);
				lua_settable(JPLua.state, top);
			}

			JPLua_Call(JPLua.state, 2, 1);
			if (lua_type(JPLua.state, -1) == LUA_TNIL){
				continue;
			}
			else if (lua_type(JPLua.state, -1) == LUA_TNUMBER){
				ret = qtrue;
				break;
			}
			else{
				trap->Print("Invalid return value in %s (JPLUA_EVENT_CLIENTCOMMAND), expected integer or nil but got %s\n", plugin->name, lua_typename(JPLua.state, -1));
			}
		}
	}
	int handle = jplua_client_commands[cmd];
	if (handle){
		ret = qtrue;
		lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, handle);

		JPLua_Player_CreateRef(JPLua.state, clientNum);
		//Push table of arguments
		lua_newtable(JPLua.state);
		top = lua_gettop(JPLua.state);
		for (i = 1; i < numArgs; i++) {
			char argN[MAX_TOKEN_CHARS];
			trap->Argv(i, argN, sizeof(argN));
			lua_pushnumber(JPLua.state, i);
			lua_pushstring(JPLua.state, argN);
			lua_settable(JPLua.state, top);
		}
		JPLua_Call(JPLua.state, 2, 0);
	}
#endif
	return ret;
}

#endif

#if defined(PROJECT_GAME)
const char *JPLua_Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

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
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CLIENTCONNECT), expected string or nil but got %s\n",
					plugin->name, lua_typename( JPLua.state, -1 ) );
				return NULL;
			}
		}
	}
#endif
	return NULL;
}
#elif defined(PROJECT_CGAME)
void JPLua_Event_ClientConnect( int clientNum ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLua_Call( JPLua.state, 1, 0 );
		}
	}
#endif // JPLUA
}
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientDisconnect( int clientNum ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] );

			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLua_Call( JPLua.state, 1, 0 );
		}
	}
#endif
}
#endif

#ifdef PROJECT_CGAME
void JPLua_Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTINFO] ) {
			int top1, top2, i;

			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTINFO] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			for ( i = 0; i < 2; i++ ) {
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

				lua_pushstring( JPLua.state, "color1" );		JPLua_Vector_CreateRef( JPLua.state, ci->color1.r, ci->color1.g, ci->color1.b );	lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "color2" );		JPLua_Vector_CreateRef( JPLua.state, ci->color2.r, ci->color2.g, ci->color2.b );	lua_settable( JPLua.state, top1 );

				lua_pushstring( JPLua.state, "rgb1" );			JPLua_Vector_CreateRef( JPLua.state, ci->rgb1.r, ci->rgb1.g, ci->rgb1.b );	lua_settable( JPLua.state, top1 );
				lua_pushstring( JPLua.state, "rgb2" );			JPLua_Vector_CreateRef( JPLua.state, ci->rgb2.r, ci->rgb2.g, ci->rgb2.b );	lua_settable( JPLua.state, top1 );
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

			JPLua_Call( JPLua.state, 3, 0 );
		}
	}
#endif
}
#endif

#ifdef PROJECT_GAME
void JPLua_Event_ClientSpawn( int clientNum, qboolean firstSpawn ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] );

			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushboolean( JPLua.state, firstSpawn );
			JPLua_Call( JPLua.state, 2, 0 );
		}
	}
#endif
}
#endif

#ifdef PROJECT_GAME
qboolean JPLua_Event_ClientUserinfoChanged( int clientNum, char *userinfo ) {
	qboolean ret = qfalse;
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] );

			lua_pushinteger( JPLua.state, clientNum );
			JPLua_PushInfostring( JPLua.state, userinfo );

			JPLua_Call( JPLua.state, 2, 1 );

			// they wanted to modify it, parse it out and apply it
			if ( lua_type( JPLua.state, -1 ) == LUA_TTABLE ) {
				JPLua_PopInfostring( JPLua.state, userinfo );
				ret = qtrue;
			}
			else if ( lua_type( JPLua.state, -1 ) != LUA_TNIL ) {
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CLIENTUSERINFOCHANGED), expected table or nil but"
					"got %s\n", plugin->name, lua_typename( JPLua.state, -1 ) );
			}
		}
	}
#endif
	return ret;
}
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_HUD( void ) {
	qboolean ret = qfalse;

#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_HUD] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_HUD] );
			JPLua_Call( JPLua.state, 0, 1 );
			if ( !ret )
				ret = !!lua_tointeger( JPLua.state, -1 );
		}
	}
#endif //JPLUA

	return ret;
}
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_VehicleHUD( void ) {
	qboolean ret = qfalse;

#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_VEHICLEHUD] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_VEHICLEHUD] );
			JPLua_Call( JPLua.state, 0, 1 );
			if ( !ret )
				ret = !!lua_tointeger( JPLua.state, -1 );
		}
	}
#endif //JPLUA

	return ret;
}
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_ConnectScreen( void ) {
	qboolean ret = qfalse;

#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_CONNECTSCREEN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CONNECTSCREEN] );
			JPLua_Call( JPLua.state, 0, 1 );
			if ( !ret )
				ret = !!lua_tointeger( JPLua.state, -1 );
		}
	}
#endif //JPLUA

	return ret;
}
#endif

#if defined(PROJECT_GAME)
void JPLua_Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_PAIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PAIN] );

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
#elif defined(PROJECT_CGAME)
void JPLua_Event_Pain( int clientNum, int health ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_PAIN] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PAIN] );

			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushinteger( JPLua.state, health );

			JPLua_Call( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}
#endif

#ifdef PROJECT_GAME
void JPLua_Event_PlayerDeath( int clientNum, int mod, int inflictor ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] );

			JPLua_Player_CreateRef( JPLua.state, clientNum ); // victim
			lua_pushinteger( JPLua.state, mod ); // method of death

			if ( inflictor >= MAX_CLIENTS || inflictor < 0 ) {
				// -1 means inflictor is not a player
				lua_pushnil( JPLua.state ); // nil because not player
			}
			else {
				JPLua_Player_CreateRef( JPLua.state, inflictor );
			}

			JPLua_Call( JPLua.state, 3, 0 );
		}
	}
#endif // JPLUA
}
#endif

#ifdef PROJECT_CGAME
void JPLua_Event_SaberTouch( int victim, int attacker ) {
#ifdef JPLUA
	jplua_plugin_t *plugin = NULL;
	while ( JPLua_IteratePlugins( &plugin ) ) {
		if ( plugin->eventListeners[JPLUA_EVENT_SABERTOUCH] ) {
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_SABERTOUCH] );

			JPLua_Player_CreateRef( JPLua.state, victim );
			JPLua_Player_CreateRef( JPLua.state, attacker );

			JPLua_Call( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}
#endif

#ifdef PROJECT_CGAME
qboolean JPLua_Event_ConsoleCommand( void ){
	qboolean ret = qfalse;
#ifdef JPLUA
	int top, i;
	int handle = jplua_console_commands[CG_Argv( 0 )];
	if (handle){

		lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, handle);

		lua_pushstring( JPLua.state, CG_Argv( 0 ) );
		//Push table of arguments
		lua_newtable(JPLua.state);
		top = lua_gettop(JPLua.state);
		for (i = 1; i < trap->Cmd_Argc(); i++) {
			lua_pushnumber(JPLua.state, i);
			lua_pushstring(JPLua.state, CG_Argv( i ));
			lua_settable(JPLua.state, top);
		}
		lua_pushstring( JPLua.state, ConcatArgs( 0 ) );
		JPLua_Call(JPLua.state, 3, 0);
		ret = qtrue;
	}
#endif
	return ret;

}

#endif

#if defined(PROJECT_GAME)
qboolean JPLua_Event_ServerCommand( void ) {
	qboolean ret = qfalse;
#ifdef JPLUA
	int top, i, numArgs = trap->Argc();
	char arg1[MAX_TOKEN_CHARS];
	trap->Argv( 0, arg1, sizeof(arg1) );
	int handle = jplua_server_commands[arg1];
	if (handle){
		lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, handle);
		//Push table of arguments
		lua_newtable(JPLua.state);
		top = lua_gettop(JPLua.state);
		for ( i = 1; i < numArgs; i++ ) {
			char argN[MAX_TOKEN_CHARS];
			trap->Argv( i, argN, sizeof(argN) );
			lua_pushnumber( JPLua.state, i );
			lua_pushstring( JPLua.state, argN );
			lua_settable( JPLua.state, top );
		}
		JPLua_Call(JPLua.state, 1, 0);
		ret = qtrue;
	}
#endif // JPLUA
	return ret;
}
#elif defined(PROJECT_CGAME)
qboolean JPLua_Event_ServerCommand( void ) {
	qboolean ret = qfalse;
#ifdef JPLUA
	int top, i;
	int handle = jplua_server_commands[CG_Argv( 0 )];
	if (handle){
		lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, handle);
		//Push table of arguments
		lua_newtable(JPLua.state);
		top = lua_gettop(JPLua.state);
		for (i = 1; i < trap->Cmd_Argc(); i++) {
			lua_pushnumber(JPLua.state, i);
			lua_pushstring(JPLua.state, CG_Argv( i ));
			lua_settable(JPLua.state, top);
		}
		JPLua_Call(JPLua.state, 1, 0);
		ret = qtrue;
	}
#endif // JPLUA
	return ret;
}
#endif
