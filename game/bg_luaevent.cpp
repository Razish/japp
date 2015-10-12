#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif
#include "bg_luainternal.h"

#ifdef JPLUA
#include <unordered_map>

namespace JPLua {

#ifdef PROJECT_GAME
	extern std::unordered_map<std::string, command_t> clientCommands;
#elif defined PROJECT_CGAME
	extern std::unordered_map<std::string, command_t> consoleCommands;
#endif
	extern std::unordered_map<std::string, command_t> serverCommands;

	static const stringID_table_t events[JPLUA_EVENT_MAX] = {
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
		ENUM2STRING( JPLUA_EVENT_VEHICLEHUD ),
		ENUM2STRING( JPLUA_EVENT_CONNECTSCREEN ),
		ENUM2STRING( JPLUA_EVENT_PAIN ),
		ENUM2STRING( JPLUA_EVENT_PLAYERDEATH ),
		ENUM2STRING( JPLUA_EVENT_SABERTOUCH ),
	};

	// called by lua
	int Event_AddListener( lua_State *L ) {
		const char *listenerArg = lua_tostring( L, 1 );

		if ( lua_type( L, 1 ) != LUA_TSTRING || lua_type( L, 2 ) != LUA_TFUNCTION ) {
#if defined(PROJECT_GAME)
			G_LogPrintf( level.log.console, "ls. AddListener failed, function signature invalid registering %s "
				"(plugin: %s) - Is it up to date?\n", listenerArg, ls.currentPlugin->name
			);
#elif defined(PROJECT_CGAME)
			trap->Print( "ls. AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up "
				"to date?\n", listenerArg, ls.currentPlugin->name
			);
#endif
			return 0;
		}

		for ( int i = 0; i < JPLUA_EVENT_MAX; i++ ) {
			if ( !Q_stricmp( listenerArg, events[i].name ) ) {
				ls.currentPlugin->eventListeners[i] = luaL_ref( L, LUA_REGISTRYINDEX );
				return 0;
			}
		}

#if defined(PROJECT_GAME)
		G_LogPrintf( level.log.console, "ls. AddListener failed, could not find event %s (plugin: %s) - Is it up to "
			"date?\n", listenerArg, ls.currentPlugin->name
		);
#elif defined(PROJECT_CGAME)
		trap->Print( "ls. AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n",
			listenerArg, ls.currentPlugin->name );
#endif

		return 0;
	}

	// called by lua
	int Event_RemoveListener( lua_State *L ) {
		int i = 0;
		const char *listenerArg = lua_tostring( L, 1 );

		if ( lua_type( L, 1 ) != LUA_TSTRING ) {
#if defined(PROJECT_GAME)
			G_LogPrintf( level.log.console, "ls. RemoveListener failed, function signature invalid registering %s "
				"(plugin: %s) - Is it up to date?\n", listenerArg, ls.currentPlugin->name
			);
#elif defined(PROJECT_CGAME)
			trap->Print( "ls. RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it "
				"up to date?\n", listenerArg, ls.currentPlugin->name
			);
#endif
			return 0;
		}

		for ( i = 0; i < JPLUA_EVENT_MAX; i++ ) {
			if ( !Q_stricmp( listenerArg, events[i].name ) ) {
				luaL_unref( L, LUA_REGISTRYINDEX, ls.currentPlugin->eventListeners[i] );
				ls.currentPlugin->eventListeners[i] = 0;
				return 0;
			}
		}

#if defined(PROJECT_GAME)
		G_LogPrintf( level.log.console, "ls. RemoveListener failed, could not find event %s (plugin: %s) - Is it up "
			"to date?\n", listenerArg, ls.currentPlugin->name
		);
#elif defined(PROJECT_CGAME)
		trap->Print( "ls. RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n",
			listenerArg, ls.currentPlugin->name );
#endif

		return 0;
	}

#endif // JPLUA

	void Event_Shutdown( qboolean restart ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			// fire the unload event
			if ( plugin->eventListeners[JPLUA_EVENT_UNLOAD] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_UNLOAD] );
				lua_pushboolean( ls.L, restart );
				Call( ls.L, 1, 0 );
			}
		}
#endif
	}

	void Event_RunFrame( void ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_RUNFRAME] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_RUNFRAME] );
				Call( ls.L, 0, 0 );
			}
		}
#endif // JPLUA
	}

#if defined(PROJECT_CGAME)
	char *Event_ChatMessageRecieved( const char *msg ) {
#elif defined(PROJECT_GAME)
	char *Event_ChatMessageRecieved(int clientNum, const char *msg, int type ) {
#endif
		static char tmpMsg[MAX_SAY_TEXT] = { 0 }; // although a chat message can only be MAX_SAY_TEXT long..-name?
#ifdef JPLUA
		plugin_t *plugin = NULL;
#endif

		Q_strncpyz( tmpMsg, msg, MAX_SAY_TEXT );

#ifdef JPLUA
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] );

#if defined(PROJECT_CGAME)
				lua_pushstring( ls.L, tmpMsg );
				Call( ls.L, 1, 1 );
#elif defined(PROJECT_GAME)
				Player_CreateRef( ls.L, clientNum );
				lua_pushstring( ls.L, tmpMsg );
				lua_pushinteger(ls.L, type);
				Call( ls.L, 3, 1 );
#endif

				int retType = lua_type( ls.L, -1 );
				// returned nil, no use passing it to other plugins
				if ( retType == LUA_TNIL ) {
					return NULL;
				}
				else if ( retType == LUA_TSTRING ) {
					Q_strncpyz( tmpMsg, lua_tostring( ls.L, -1 ), sizeof(tmpMsg) );
				}
				else {
					trap->Print( "Invalid return value in %s (JPLUA_EVENT_CHATMSGRECV), expected string or nil but got "
						"%s", plugin->name, lua_typename( ls.L, -1 )
					);
				}
			}
		}
#endif // JPLUA

		return tmpMsg;
	}

#ifdef PROJECT_CGAME
	char *Event_ChatMessageSent( const char *msg, messageMode_t mode, int targetClient ) {
		static char tmpMsg[MAX_STRING_CHARS] = { 0 }; // although a chat message can only be MAX_SAY_TEXT long..-name?
#ifdef JPLUA
		plugin_t *plugin = NULL;
#endif

		Q_strncpyz( tmpMsg, msg, sizeof(tmpMsg) );

#ifdef JPLUA
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] );

				lua_pushstring( ls.L, tmpMsg );
				lua_pushinteger( ls.L, mode );
				lua_pushinteger( ls.L, targetClient );
				Call( ls.L, 3, 1 );

				// returned nil, no use passing it to other plugins
				if ( lua_type( ls.L, -1 ) == LUA_TNIL )
					return NULL;
				else if ( lua_type( ls.L, -1 ) == LUA_TSTRING )
					Q_strncpyz( tmpMsg, lua_tostring( ls.L, -1 ), MAX_SAY_TEXT );
				else {
					trap->Print( "Invalid return value in %s (JPLUA_EVENT_CHATMSGSEND), expected string or nil but got "
						"%s\n", plugin->name, lua_typename( ls.L, -1 )
					);
					return NULL;
				}
			}
		}
#endif // JPLUA

		return tmpMsg;
	}
#endif

#ifdef PROJECT_GAME
	void Event_ClientBegin( int clientNum ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTBEGIN] );

				Player_CreateRef( ls.L, clientNum );

				Call( ls.L, 1, 0 );
			}
		}
#endif
	}
#endif

#ifdef PROJECT_GAME
	qboolean Event_ClientCommand( int clientNum ) {
		qboolean ret = qfalse;
#ifdef JPLUA
		int top, numArgs = trap->Argc();
		char cmd[MAX_TOKEN_CHARS] = {};
		trap->Argv( 0, cmd, sizeof(cmd) );

		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCOMMAND] );

				Player_CreateRef( ls.L, clientNum );

				//Push table of arguments
				lua_newtable( ls.L );
				top = lua_gettop( ls.L );
				for ( int i = 0; i < numArgs; i++) {
					char argN[MAX_TOKEN_CHARS];
					trap->Argv( i, argN, sizeof(argN) );
					lua_pushnumber( ls.L, i + 1 );
					lua_pushstring( ls.L, argN );
					lua_settable( ls.L, top );
				}

				Call( ls.L, 2, 1 );
				int retType = lua_type( ls.L, -1 );
				if ( retType == LUA_TNIL ) {
					continue;
				}
				else if ( retType == LUA_TNUMBER ) {
					ret = qtrue;
					break;
				}
				else {
					trap->Print( "Invalid return value in %s (JPLUA_EVENT_CLIENTCOMMAND), expected integer or nil but "
						"got %s\n", plugin->name, lua_typename( ls.L, -1 )
					);
				}
			}
		}
		command_t &comm = clientCommands[cmd];
		if ( comm.handle ) {
			ret = qtrue;
			lua_rawgeti(ls.L, LUA_REGISTRYINDEX, comm.handle );
			Player_CreateRef( ls.L, clientNum );
			//Push table of arguments
			lua_newtable( ls.L );
			top = lua_gettop( ls.L );
			for ( int i = 1; i < numArgs; i++ ) {
				char argN[MAX_TOKEN_CHARS];
				trap->Argv( i, argN, sizeof(argN) );
				lua_pushnumber( ls.L, i );
				lua_pushstring( ls.L, argN );
				lua_settable( ls.L, top );
			}
			Call( ls.L, 2, 0 );
		}
#endif
		return ret;
	}

#endif

#if defined(PROJECT_GAME)
	const char *Event_ClientConnect( int clientNum, const char *userinfo, const char *IP, qboolean firstTime ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

				lua_pushinteger( ls.L, clientNum );
				PushInfostring( ls.L, userinfo );
				lua_pushstring( ls.L, IP );
				lua_pushboolean( ls.L, !!firstTime );

				Call( ls.L, 4, 1 );

				// connection allowed, pass to other plugins
				if ( lua_type( ls.L, -1 ) == LUA_TNIL )
					continue;

				// denied, no use passing it to other plugins
				if ( lua_type( ls.L, -1 ) == LUA_TSTRING )
					return lua_tostring( ls.L, -1 );
				else {
					trap->Print( "Invalid return value in %s (JPLUA_EVENT_CLIENTCONNECT), expected string or nil but "
						"got %s\n", plugin->name, lua_typename( ls.L, -1 )
					);
					return NULL;
				}
			}
		}
#endif
		return NULL;
	}
#elif defined(PROJECT_CGAME)
	void Event_ClientConnect( int clientNum ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

				Player_CreateRef( ls.L, clientNum );

				Call( ls.L, 1, 0 );
			}
		}
#endif // JPLUA
	}
#endif

#ifdef PROJECT_GAME
	void Event_ClientDisconnect( int clientNum ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTDISCONNECT] );

				Player_CreateRef( ls.L, clientNum );

				Call( ls.L, 1, 0 );
			}
		}
#endif
	}
#endif

#ifdef PROJECT_CGAME
	void Event_ClientInfoUpdate( int clientNum, clientInfo_t *oldInfo, clientInfo_t *newInfo ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTINFO] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTINFO] );

				// Create a player instance for this client number and push on stack
				Player_CreateRef( ls.L, clientNum );

				for ( int i = 0; i < 2; i++ ) {
					clientInfo_t *ci = i ? newInfo : oldInfo;
					lua_newtable( ls.L );
					int top1 = lua_gettop( ls.L );

					lua_pushstring( ls.L, "colorOverride" );
						lua_newtable( ls.L );
						int top2 = lua_gettop( ls.L );
							lua_pushstring( ls.L, "r" );
							lua_pushnumber( ls.L, ci->colorOverride.r );
							lua_settable( ls.L, top2 );
							lua_pushstring( ls.L, "g" );
							lua_pushnumber( ls.L, ci->colorOverride.g );
							lua_settable( ls.L, top2 );
							lua_pushstring( ls.L, "b" );
							lua_pushnumber( ls.L, ci->colorOverride.b );
							lua_settable( ls.L, top2 );
							lua_pushstring( ls.L, "a" );
							lua_pushnumber( ls.L, ci->colorOverride.a );
							lua_settable( ls.L, top2 );
						lua_settable( ls.L, top1 );

					lua_pushstring( ls.L, "saberName" );
						lua_pushstring( ls.L, ci->saberName );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "saber2Name" );
						lua_pushstring( ls.L, ci->saber2Name );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "name" );
						lua_pushstring( ls.L, ci->name );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "team" );
						lua_pushnumber( ls.L, ci->team );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "duelTeam" );
						lua_pushnumber( ls.L, ci->duelTeam );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "botSkill" );
						lua_pushnumber( ls.L, ci->botSkill );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "color1" );
						Vector_CreateRef( ls.L, &ci->color1 );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "color2" );
						Vector_CreateRef( ls.L, &ci->color2 );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "rgb1" );
						Vector_CreateRef( ls.L, ci->rgb1.r, ci->rgb1.g, ci->rgb1.b );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "rgb2" );
						Vector_CreateRef( ls.L, ci->rgb2.r, ci->rgb2.g, ci->rgb2.b );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "icolor1" );
						lua_pushnumber( ls.L, ci->icolor1 );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "icolor2" );
						lua_pushnumber( ls.L, ci->icolor2 );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "score" );
						lua_pushnumber( ls.L, ci->score );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "location" );
						lua_pushnumber( ls.L, ci->location );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "health" );
						lua_pushnumber( ls.L, ci->health );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "armor" );
						lua_pushnumber( ls.L, ci->armor );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "curWeapon" );
						lua_pushnumber( ls.L, ci->curWeapon );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "handicap" );
						lua_pushnumber( ls.L, ci->handicap );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "wins" );
						lua_pushnumber( ls.L, ci->wins );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "losses" );
						lua_pushnumber( ls.L, ci->losses );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "teamTask" );
						lua_pushnumber( ls.L, ci->teamTask );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "powerups" );
						lua_pushnumber( ls.L, ci->powerups );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "modelName" );
						lua_pushstring( ls.L, ci->saberName );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "skinName" );
						lua_pushstring( ls.L, ci->saberName );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "forcePowers" );
						lua_pushstring( ls.L, ci->forcePowers );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "teamName" );
						lua_pushstring( ls.L, ci->teamName );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "deferred" );
						lua_pushboolean( ls.L, ci->deferred );
						lua_settable( ls.L, top1 );
					lua_pushstring( ls.L, "gender" );
						lua_pushnumber( ls.L, ci->gender );
						lua_settable( ls.L, top1 );
				}

				Call( ls.L, 3, 0 );
			}
		}
#endif
	}
#endif

#ifdef PROJECT_GAME
	void Event_ClientSpawn( int clientNum, qboolean firstSpawn ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTSPAWN] );

				Player_CreateRef( ls.L, clientNum );
				lua_pushboolean( ls.L, firstSpawn );
				Call( ls.L, 2, 0 );
			}
		}
#endif
	}
#endif

#ifdef PROJECT_GAME
	qboolean Event_ClientUserinfoChanged( int clientNum, char *userinfo ) {
		qboolean ret = qfalse;
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CLIENTUSERINFOCHANGED] );

				lua_pushinteger( ls.L, clientNum );
				PushInfostring( ls.L, userinfo );

				Call( ls.L, 2, 1 );

				// they wanted to modify it, parse it out and apply it
				int retType = lua_type( ls.L, -1 );
				if ( retType == LUA_TTABLE ) {
					PopInfostring( ls.L, userinfo );
					ret = qtrue;
				}
				else if ( retType != LUA_TNIL ) {
					trap->Print( "Invalid return value in %s (JPLUA_EVENT_CLIENTUSERINFOCHANGED), expected table or nil"
						" but got %s\n", plugin->name, lua_typename( ls.L, -1 )
					);
				}
			}
		}
#endif
		return ret;
	}
#endif

#ifdef PROJECT_CGAME
	qboolean Event_HUD( void ) {
		qboolean ret = qfalse;

#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_HUD] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_HUD] );
				Call( ls.L, 0, 1 );
				if ( !ret )
					ret = !!lua_tointeger( ls.L, -1 );
			}
		}
#endif //JPLUA

		return ret;
	}
#endif

#ifdef PROJECT_CGAME
	qboolean Event_VehicleHUD( void ) {
		qboolean ret = qfalse;

#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_VEHICLEHUD] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_VEHICLEHUD] );
				Call( ls.L, 0, 1 );
				if ( !ret )
					ret = !!lua_tointeger( ls.L, -1 );
			}
		}
#endif //JPLUA

		return ret;
	}
#endif

#ifdef PROJECT_CGAME
	qboolean Event_ConnectScreen( void ) {
		qboolean ret = qfalse;

#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_CONNECTSCREEN] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_CONNECTSCREEN] );
				Call( ls.L, 0, 1 );
				if ( !ret )
					ret = !!lua_tointeger( ls.L, -1 );
			}
		}
#endif //JPLUA

		return ret;
	}
#endif

#if defined(PROJECT_GAME)
	void Event_Pain( int target, int inflictor, int attacker, int health, int armor, uint32_t dflags, int mod ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_PAIN] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PAIN] );

				Player_CreateRef( ls.L, target );
				Player_CreateRef( ls.L, inflictor );
				Player_CreateRef( ls.L, attacker );
				lua_pushinteger( ls.L, health );
				lua_pushinteger( ls.L, armor );
				lua_pushinteger( ls.L, dflags );
				lua_pushinteger( ls.L, mod );

				Call( ls.L, 7, 0 );
			}
		}
#endif // JPLUA
	}
#elif defined(PROJECT_CGAME)
	void Event_Pain( int clientNum, int health ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_PAIN] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PAIN] );

				Player_CreateRef( ls.L, clientNum );
				lua_pushinteger( ls.L, health );

				Call( ls.L, 2, 0 );
			}
		}
#endif // JPLUA
	}
#endif

#ifdef PROJECT_GAME
	void Event_PlayerDeath( int clientNum, int mod, int inflictor ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_PLAYERDEATH] );

				Player_CreateRef( ls.L, clientNum ); // victim
				lua_pushinteger( ls.L, mod ); // method of death

				if ( inflictor >= MAX_CLIENTS || inflictor < 0 ) {
					// -1 means inflictor is not a player
					lua_pushnil( ls.L ); // nil because not player
				}
				else {
					Player_CreateRef( ls.L, inflictor );
				}

				Call( ls.L, 3, 0 );
			}
		}
#endif // JPLUA
	}
#endif

#ifdef PROJECT_CGAME
	void Event_SaberTouch( int victim, int attacker ) {
#ifdef JPLUA
		plugin_t *plugin = NULL;
		while ( IteratePlugins( &plugin ) ) {
			if ( plugin->eventListeners[JPLUA_EVENT_SABERTOUCH] ) {
				lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_SABERTOUCH] );

				Player_CreateRef( ls.L, victim );
				Player_CreateRef( ls.L, attacker );

				Call( ls.L, 2, 0 );
			}
		}
#endif // JPLUA
	}
#endif

#ifdef PROJECT_CGAME
	qboolean Event_ConsoleCommand( void ){
		qboolean ret = qfalse;
#ifdef JPLUA
		int top, i;
		command_t &cmd = consoleCommands[CG_Argv( 0 )];
		if (cmd.handle){

			lua_rawgeti(ls.L, LUA_REGISTRYINDEX, cmd.handle);

			lua_pushstring( ls.L, CG_Argv( 0 ) );
			//Push table of arguments
			lua_newtable(ls.L);
			top = lua_gettop(ls.L);
			for (i = 1; i < trap->Cmd_Argc(); i++) {
				lua_pushnumber(ls.L, i);
				lua_pushstring(ls.L, CG_Argv( i ));
				lua_settable(ls.L, top);
			}
			lua_pushstring( ls.L, ConcatArgs( 0 ) );
			Call(ls.L, 3, 0);
			ret = qtrue;
		}
#endif
		return ret;

	}

#endif

#if defined(PROJECT_GAME)
	qboolean Event_ServerCommand( void ) {
		qboolean ret = qfalse;
#ifdef JPLUA
		int top, i, numArgs = trap->Argc();
		char arg1[MAX_TOKEN_CHARS];
		trap->Argv( 0, arg1, sizeof(arg1) );
		command_t &cmd = serverCommands[arg1];
		if (cmd.handle){
			lua_rawgeti(ls.L, LUA_REGISTRYINDEX, cmd.handle);
			//Push table of arguments
			lua_newtable(ls.L);
			top = lua_gettop(ls.L);
			for ( i = 1; i < numArgs; i++ ) {
				char argN[MAX_TOKEN_CHARS];
				trap->Argv( i, argN, sizeof(argN) );
				lua_pushnumber( ls.L, i );
				lua_pushstring( ls.L, argN );
				lua_settable( ls.L, top );
			}
			Call(ls.L, 1, 0);
			ret = qtrue;
		}
#endif // JPLUA
		return ret;
	}
#elif defined(PROJECT_CGAME)
	qboolean Event_ServerCommand( void ) {
		qboolean ret = qfalse;
#ifdef JPLUA
		int top, i;
		command_t &cmd = serverCommands[CG_Argv( 0 )];
		if ( cmd.handle ){
			lua_rawgeti(ls.L, LUA_REGISTRYINDEX, cmd.handle);
			//Push table of arguments
			lua_newtable(ls.L);
			top = lua_gettop(ls.L);
			for (i = 1; i < trap->Cmd_Argc(); i++) {
				lua_pushnumber(ls.L, i);
				lua_pushstring(ls.L, CG_Argv( i ));
				lua_settable(ls.L, top);
			}
			Call(ls.L, 1, 0);
			ret = qtrue;
		}
#endif // JPLUA
		return ret;
	}
#endif

} // namespace JPLua
