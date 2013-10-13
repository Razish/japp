#include "g_local.h"
#include "g_lua.h"

#ifdef JPLUA

//================================
//		EVENT HANDLERS
//================================

stringID_table_t jplua_events[JPLUA_EVENT_MAX] = 
{
	ENUM2STRING(JPLUA_EVENT_UNLOAD),
	ENUM2STRING(JPLUA_EVENT_RUNFRAME),
//	ENUM2STRING(JPLUA_EVENT_CLIENTCONNECT),
};

int JPLua_Event_AddListener( lua_State *L )
{//Called by Lua!
	int i = 0;
	const char *listenerArg = lua_tostring( L, -2 );

	if ( lua_type( L, -1 ) != LUA_TFUNCTION || lua_type( L, -2 ) != LUA_TSTRING )
	{
		G_LogPrintf( "JPLua: AddListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
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

	G_LogPrintf( "JPLua: AddListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
	return 0;
}

int JPLua_Event_RemoveListener( lua_State *L )
{//Called by Lua!
	int i = 0;
	const char *listenerArg = lua_tostring( L, -1 );

	if ( lua_type( L, -1 ) != LUA_TSTRING )
	{
		G_LogPrintf( "JPLua: RemoveListener failed, function signature invalid registering %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
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

	G_LogPrintf( "JPLua: RemoveListener failed, could not find event %s (plugin: %s) - Is it up to date?\n", listenerArg, JPLua.currentPlugin->name );
	
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
		jplua_plugin_command_t *clientCmd = JPLua.currentPlugin->clientCmds, *nextClientCmd = clientCmd;
		jplua_plugin_command_t *serverCmd = JPLua.currentPlugin->serverCmds, *nextServerCmd = serverCmd;

		if ( JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] )
		{//Fire the unload event
			lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, JPLua.currentPlugin->eventListeners[JPLUA_EVENT_UNLOAD] );
			JPLUACALL( JPLua.state, 0, 0 );
		}

		//RAZTODO: Refcount because multiple plugins can register the same command
		while ( nextClientCmd )
		{//Remove all console commands
			lua_unref( JPLua.state, clientCmd->handle );
			nextClientCmd = clientCmd->next;

			free( clientCmd );
			clientCmd = nextClientCmd;
		}

		while ( nextServerCmd )
		{//Remove all server commands
			lua_unref( JPLua.state, serverCmd->handle );
			nextServerCmd = serverCmd->next;

			free( serverCmd );
			serverCmd = nextServerCmd;
		}

		JPLua.currentPlugin->clientCmds = NULL;
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

/*
void JPLua_Event_ClientConnect( int clientNum )
{
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
}
*/

qboolean JPLua_Event_ClientCommand( void )
{
	qboolean ret = qfalse;
#ifdef JPLUA
	for ( JPLua.currentPlugin = JPLua.plugins;
			JPLua.currentPlugin;
			JPLua.currentPlugin = JPLua.currentPlugin->next
		)
	{
		jplua_plugin_command_t *cmd = JPLua.currentPlugin->clientCmds;

		while ( cmd )
		{
			int top = 0, i = 0, numArgs = trap->Argc();
			char arg1[MAX_TOKEN_CHARS] = {0};

			trap->Argv( 0, arg1, sizeof( arg1 ) );

			if ( !Q_stricmp( arg1, cmd->command ) )
			{
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<numArgs; i++ )
				{
					char argN[MAX_TOKEN_CHARS] = {0};
					trap->Argv( i, argN, sizeof( argN ) );
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, argN );
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
			int top = 0, i = 0, numArgs = trap->Argc();
			char arg1[MAX_TOKEN_CHARS] = {0};

			trap->Argv( 0, arg1, sizeof( arg1 ) );

			if ( !Q_stricmp( arg1, cmd->command ) )
			{
				lua_rawgeti( JPLua.state, LUA_REGISTRYINDEX, cmd->handle );

				//Push table of arguments
				lua_newtable( JPLua.state );
				top = lua_gettop( JPLua.state );
				for ( i=1; i<numArgs; i++ )
				{
					char argN[MAX_TOKEN_CHARS] = {0};
					trap->Argv( i, argN, sizeof( argN ) );
					lua_pushnumber( JPLua.state, i );
					lua_pushstring( JPLua.state, argN );
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
