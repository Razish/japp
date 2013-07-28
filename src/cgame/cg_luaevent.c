#include "cg_local.h"
#include "cg_lua.h"
#include "cg_engine.h"

#ifdef JPLUA

//================================
//		EVENT HANDLERS
//================================

stringID_table_t jplua_events[JPLUA_EVENT_MAX] = 
{
	ENUM2STRING(JPLUA_EVENT_UNLOAD),
	ENUM2STRING(JPLUA_EVENT_RUNFRAME),
	ENUM2STRING(JPLUA_EVENT_HUD),
	ENUM2STRING(JPLUA_EVENT_CHATMSGRECV),
	ENUM2STRING(JPLUA_EVENT_CHATMSGSEND),
	ENUM2STRING(JPLUA_EVENT_CLIENTCONNECT),
	ENUM2STRING(JPLUA_EVENT_PAIN),
	ENUM2STRING(JPLUA_EVENT_SABERTOUCH),
};

int JPLua_Event_AddListener( lua_State *L )
{//Called by Lua!
	int i = 0;
	const char *listenerArg = lua_tostring( L, -2 );

	if ( lua_type( L, -1 ) != LUA_TFUNCTION || lua_type( L, -2 ) != LUA_TSTRING )
	{
		CG_Printf( "JPLua: AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	for ( i=0; i<JPLUA_EVENT_MAX; i++ )
	{//Loop 'til we find the event we want to listen for
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) )
		{
			JPLua.currentPlugin->eventListeners[i] = luaL_ref( L, LUA_REGISTRYINDEX );
			return 0;
		}
	}

	CG_Printf( "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
	return 0;
}

int JPLua_Event_RemoveListener( lua_State *L )
{//Called by Lua!
	int i = 0;
	const char *listenerArg = lua_tostring( L, -1 );

	if ( lua_type( L, -1 ) != LUA_TSTRING )
	{
		CG_Printf( "JPLua: RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
		return 0;
	}

	for ( i=0; i<JPLUA_EVENT_MAX; i++ )
	{//Loop 'til we find the event we want to remove the listener for
		if ( !Q_stricmp( listenerArg, jplua_events[i].name ) )
		{
			luaL_unref( L, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[i] );
			JPLua.currentPlugin->eventListeners[i] = 0;
			return 0;
		}
	}

	CG_Printf( "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
	return 0;
}

#endif // JPLUA

void JPLua_Event_Shutdown( void )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *consoleCmd = JPLua.currentPlugin->consoleCmds, *nextConsoleCmd = consoleCmd;
		jplua_plugin_command_t *serverCmd = JPLua.currentPlugin->serverCmds, *nextServerCmd = serverCmd;

		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] )
		{//Fire the unload event
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] );
			JPLUACALL( JPLua.state, 0, 0 );
		}

		//RAZTODO: Refcount because multiple plugins can register the same command
		while ( nextConsoleCmd )
		{//Remove all console commands
			lua_unref( JPLua.state, consoleCmd->handle );
			#ifndef OPENJK
				ENG_Cmd_RemoveCommand( consoleCmd->command );
			#endif // !OPENJK
			nextConsoleCmd = consoleCmd->next;

			free( consoleCmd );
			consoleCmd = nextConsoleCmd;
		}

		while ( nextServerCmd )
		{//Remove all server commands
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

void JPLua_Event_RunFrame( void )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_RUNFRAME] );
			JPLUACALL( JPLua.state, 0, 0 );
		}
	}
#endif // JPLUA
}

qboolean JPLua_Event_HUD( void )
{
	qboolean ret = qfalse;

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_HUD] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_HUD] );
			JPLUACALL( JPLua.state, 0, 1 );
			if ( !ret )
				ret = !!lua_tointeger( JPLua.state, -1 );
		}
	}
#endif //JPLUA

	return ret;
}

char *JPLua_Event_ChatMessageRecieved( const char *msg )
{
	static char tmpMsg[MAX_SAY_TEXT] = {0}; //Although a chat message can only be MAX_SAY_TEXT long..-name?

	Q_strncpyz( tmpMsg, msg, MAX_SAY_TEXT );

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGRECV] );

			lua_pushstring( JPLua.state, tmpMsg );
			JPLUACALL( JPLua.state, 1, 1 );

			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
			{// returned nil, no use passing it to other plugins
				return NULL;
			}
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGRECV), expected string or nil but got %s", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
		}
	}
#endif // JPLUA

	return tmpMsg;
}

char *JPLua_Event_ChatMessageSent( const char *msg )
{
	static char tmpMsg[MAX_TOKEN_CHARS] = {0}; //Although a chat message can only be MAX_SAY_TEXT long..-name?

	Q_strncpyz( tmpMsg, msg, sizeof( tmpMsg ) );

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CHATMSGSEND] );

			lua_pushstring( JPLua.state, tmpMsg );
			JPLUACALL( JPLua.state, 1, 1 );

			if ( lua_type( JPLua.state, -1 ) == LUA_TNIL )
			{// returned nil, no use passing it to other plugins
				return NULL;
			}
			else if ( lua_type( JPLua.state, -1 ) == LUA_TSTRING )
				Q_strncpyz( tmpMsg, lua_tostring( JPLua.state, -1 ), MAX_SAY_TEXT );
			else
				Com_Printf( "Invalid return value in %s (JPLUA_EVENT_CHATMSGSEND), expected string or nil but got %s", JPLua.currentPlugin->name, lua_typename( JPLua.state, -1 ) );
		}
	}
#endif // JPLUA

	return tmpMsg;
}

void JPLua_Event_ClientConnect( int clientNum )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_CLIENTCONNECT] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );

			JPLUACALL( JPLua.state, 1, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_Pain( int clientNum, int damage )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_PAIN] );

			// Create a player instance for this client number and push on stack
			JPLua_Player_CreateRef( JPLua.state, clientNum );
			lua_pushinteger( JPLua.state, damage );

			JPLUACALL( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}

void JPLua_Event_SaberTouch( int victim, int attacker )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_SABERTOUCH] )
		{
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_SABERTOUCH] );

			JPLua_Player_CreateRef( JPLua.state, victim );
			JPLua_Player_CreateRef( JPLua.state, attacker );

			JPLUACALL( JPLua.state, 2, 0 );
		}
	}
#endif // JPLUA
}

qboolean JPLua_Event_ConsoleCommand( void )
{
	qboolean ret = qfalse;

#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->consoleCmds;

		while ( cmd )
		{
			int top = 0;
			int i = 0;

			if ( !Q_stricmp( CG_Argv( 0 ), cmd->command ) && cmd->handle )
			{
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<trap_Argc(); i++ )
				{
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

qboolean JPLua_Event_ServerCommand( void )
{
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->serverCmds;

		while ( cmd )
		{
			int top = 0;
			int i = 0;

			if ( !Q_stricmp( CG_Argv( 0 ), cmd->command ) )
			{
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<trap_Argc(); i++ )
				{
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
