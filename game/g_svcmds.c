// Copyright (C) 1999-2000 Id Software, Inc.
//

// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"

//RAZFIXME: Use G_ClientFromString
gclient_t *ClientForString( const char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanName[MAX_STRING_CHARS];

	// numeric values could be slot numbers
	if ( Q_StringIsInteger( s ) ) {
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients ) {
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED ) {
				return cl;
			}
		}
	}

	// check for a name match
	for ( idnum=0,cl=level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		Q_strncpyz(cleanName, cl->pers.netname, sizeof(cleanName));
		Q_CleanString( cleanName, STRIP_COLOUR );
		if ( !Q_stricmp( cleanName, s ) ) {
			return cl;
		}
	}

	trap->Print( "User %s is not on the server\n", s );
	return NULL;
}

int QDECL G_SortPlayersByScoreRate( const void *a, const void *b )
{
	gclient_t	*cla = &level.clients[ *((int*)a) ],
				*clb = &level.clients[ *((int*)b) ];
	float arate, brate;

	if ( cla->pers.connectTime <= 0 && clb->pers.connectTime <= 0 )	return 0;
	if ( cla->pers.connectTime <= 0 )								return 1;
	if ( clb->pers.connectTime <= 0 )								return -1;

	arate = cla->ps.persistant[PERS_SCORE] / (level.time-cla->pers.connectTime);
	brate = clb->ps.persistant[PERS_SCORE] / (level.time-clb->pers.connectTime);

	if ( arate > brate )
		return -1;
	if ( brate > arate )
		return 1;

	return 0;
}

void G_ShuffleTeams(void)
{
	int i=0, idnum=0, cTeam=0, cnt=0;
	gentity_t *cl_ent = NULL;
	int	sortClients[MAX_CLIENTS];

	for ( i=0; i<level.numConnectedClients; i++ )
	{
		idnum = level.sortedClients[i];
		cl_ent = &g_entities[idnum];

		if ( cl_ent->client->sess.sessionTeam != TEAM_RED && cl_ent->client->sess.sessionTeam != TEAM_BLUE )
			continue;

		sortClients[cnt++] = level.sortedClients[i];
	}

	qsort( sortClients, cnt, sizeof( int ), G_SortPlayersByScoreRate );

	for ( i=0; i<cnt; i++ )
	{
		idnum = sortClients[i];
		cl_ent = &g_entities[idnum];

		cTeam = (i % 2) + TEAM_RED;

		if ( cTeam != cl_ent->client->sess.sessionTeam )
		{
			if ( cTeam == TEAM_RED )
				SetTeam( cl_ent, "r", qtrue );
			else if ( cTeam == TEAM_BLUE )
				SetTeam( cl_ent, "b", qtrue );
		}
	}

	trap->SendServerCommand( -1, "cp \""S_COLOR_RED"Teams have been shuffled!\n\"");
}

char *ConcatArgs( int start );

/*
=================
ConsoleCommand

=================
*/
const char *G_GetArenaInfoByMap( const char *map );

static void SV_AddBot_f( void ) {
	float skill;
	int delay;
	char name[MAX_TOKEN_CHARS], altname[MAX_TOKEN_CHARS], string[MAX_TOKEN_CHARS], team[MAX_TOKEN_CHARS];

	// are bots enabled?
	if ( !trap->Cvar_VariableIntegerValue( "bot_enable" ) )
		return;

	// name
	trap->Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		trap->Print( "Syntax: addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		return;
	}

	// skill
	trap->Argv( 2, string, sizeof( string ) );
	if ( !string[0] )
		skill = 4;
	else
		skill = atof( string );

	// team
	trap->Argv( 3, team, sizeof( team ) );

	// delay
	trap->Argv( 4, string, sizeof( string ) );
	if ( !string[0] )
		delay = 0;
	else
		delay = atoi( string );

	// alternative name
	trap->Argv( 5, altname, sizeof( altname ) );

	G_AddBot( name, skill, team, delay, altname );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 && trap->Cvar_VariableIntegerValue( "cl_running" ) )
		trap->SendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
}

static void SV_AdminAdd_f( void ) {
	char	argUser[MAX_TOKEN_CHARS] = {0},
			argPass[MAX_TOKEN_CHARS] = {0},
			argPrivs[MAX_TOKEN_CHARS] = {0},
			argRank[MAX_TOKEN_CHARS] = {0},
			*argMsg = NULL;
			

	if ( trap->Argc() < 5 ) {
		trap->Print( "Syntax: adminadd <user> <pass> <privileges> <rank> <login message>\n" );
		return;
	}

	trap->Argv( 1,	argUser,	sizeof( argUser ) );
	trap->Argv( 2,	argPass,	sizeof( argPass ) );
	trap->Argv( 3,	argPrivs,	sizeof( argPrivs ) );
	trap->Argv( 4,	argRank,	sizeof( argRank ) );
	argMsg = ConcatArgs( 5 );
	
	AM_AddAdmin( argUser, argPass, atoi( argPrivs ), atoi( argRank ), argMsg  );
	AM_SaveAdmins();
}

static void SV_AdminDel_f( void ) {
	char argUser[MAX_TOKEN_CHARS] = {0};

	if ( trap->Argc() < 2 ) {
		trap->Print( "Syntax: admindel <user>\n" );
		return;
	}

	trap->Argv( 1, argUser, sizeof( argUser ) );

	AM_DeleteAdmin( argUser );
	AM_SaveAdmins();
}

static void SV_AdminList_f( void ) {
	AM_ListAdmins();
}

static void SV_AdminReload_f( void ) {
	AM_LoadAdmins();
}

static void SV_AllReady_f( void ) {
	if ( !level.warmupTime ) {
		trap->Print( "allready is only available during warmup\n" );
		return;
	}

	level.warmupTime = level.time;
}

static void SV_BanAdd_f( void ) {
	char ip[NET_ADDRSTRMAXLEN] = {0}, duration[32] = {0}, *reason = NULL;

	if ( trap->Argc() < 2 ) {
		trap->Print( "Syntax: banadd <ip> <duration> <reason>\n" );
		return;
	}

	trap->Argv( 1, ip, sizeof( ip ) );
	trap->Argv( 2, duration, sizeof( duration ) );
	if ( trap->Argc() >= 4 )
		reason = ConcatArgs( 3 );
	JKG_Bans_AddBanString( ip, duration, reason );
}

static void SV_BanDel_f( void ) {
	char ip[NET_ADDRSTRMAXLEN] = {0};
	byteAlias_t *bIP = NULL;
	
	if ( trap->Argc() < 2 ) {
		trap->Print( "Syntax: bandel <ip>\n" );
		return;
	}

	trap->Argv( 1, ip, sizeof( ip ) );
	bIP = BuildByteFromIP( ip );
	if ( JKG_Bans_Remove( bIP->b ) )
		trap->Print( "Removing ban on %s\n", ip );
	else
		trap->Print( "No ban found for %s\n", ip );
}

static void SV_BanList_f( void ) {
	trap->Print( "Listing bans\n" );
	JKG_Bans_List();
}

static void SV_BanReload_f( void ) {
	trap->Print( "Reloading bans\n" );
	JKG_Bans_LoadBans();
}

static void SV_BotList_f( void ) {
	int i;
	char name[MAX_TOKEN_CHARS], funname[MAX_TOKEN_CHARS], model[MAX_TOKEN_CHARS], personality[MAX_TOKEN_CHARS];

	trap->Print( "name             model            personality              funname\n" );
	for ( i=0; i<level.bots.num; i++ )
	{
		Q_strncpyz( name, Info_ValueForKey( level.bots.infos[i], "name" ), sizeof( name ) );
		Q_CleanString( name, STRIP_COLOUR );

		Q_strncpyz( funname, Info_ValueForKey( level.bots.infos[i], "funname" ), sizeof( funname ) );
		Q_CleanString( funname, STRIP_COLOUR );

		Q_strncpyz( model, Info_ValueForKey( level.bots.infos[i], "model" ), sizeof( model ) );
		Q_CleanString( model, STRIP_COLOUR );

		Q_strncpyz( personality, Info_ValueForKey( level.bots.infos[i], "personality" ), sizeof( personality ) );
		Q_CleanString( personality, STRIP_COLOUR );

		trap->Print( "%-16s %-16s %-20s %-20s\n", name, model, personality, funname );
	}
}

static void SV_Cointoss_f( void ) {
	qboolean heads = !!(Q_irand( 0, QRAND_MAX-1 )&1);
	trap->SendServerCommand( -1, va( "cp \"Cointoss result: %s\n\"", heads ? S_COLOR_GREEN"HEADS" : S_COLOR_YELLOW"TAILS" ) );
	G_LogPrintf( "Cointoss result: %s\n", heads ? S_COLOR_GREEN"HEADS" : S_COLOR_YELLOW"TAILS" );
}

static void SV_EntityList_f( void ) {
	int e = 0;
	gentity_t *check = NULL;

	for ( e=0, check=g_entities;
		e<level.num_entities;
		e++, check++ )
	{
		char buf[256] = {0};

		if ( !check->inuse )
			continue;

		if ( check->s.eType < 0 || check->s.eType >= ET_MAX )
			Q_strcat( buf, sizeof( buf ), va( "%4i: %-3i                ", e, check->s.eType ) );
		else
			Q_strcat( buf, sizeof( buf ), va( "%4i: %-20s ", e, eTypes[check->s.eType] ) );

		if ( check->classname )
			Q_strcat( buf, sizeof( buf ), va( "[%s]", check->classname ) );

		trap->Print( "%s\n", buf );
	}
}

static void SV_ForceTeam_f( void ) {
	gclient_t	*cl;
	char		str[MAX_TOKEN_CHARS];

	// find the player
	trap->Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl )
		return;

	// set the team
	trap->Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str, qtrue );
}

static void SV_GameMemory_f( void ) {
	G_ShowGameMem();
}

static void SV_Gametype_f( void ) {
	trap->Print( "Gametype is currently: %s\n", BG_GetGametypeString( level.gametype ) );
}

static void SV_ListMaps_f( void ) {
	int i;
	char map[24] = "--", longname[32] = "--", type[64] = "--";

	trap->Print( "  map                     longname                        types\n" );
	trap->Print( "  --------                --------                        --------\n" );

	for ( i=0; i<level.arenas.num; i++ )
	{
		Q_strncpyz( map, Info_ValueForKey( level.arenas.infos[i], "map" ), sizeof( map ) );
		Q_CleanString( map, STRIP_COLOUR );

		Q_strncpyz( longname, Info_ValueForKey( level.arenas.infos[i], "longname" ), sizeof( longname ) );
		Q_CleanString( longname, STRIP_COLOUR );

		Q_strncpyz( type, Info_ValueForKey( level.arenas.infos[i], "type" ), sizeof( type ) );
		Q_CleanString( type, STRIP_COLOUR );

		trap->Print( "  %-24s%-32s%-64s\n", map, longname, type );
	}
}

#ifdef JPLUA
static void SV_Lua_f( void ) {
	char *args = NULL;

	if ( trap->Argc() < 2 || !JPLua.state )
		return;

	args = ConcatArgs( 1 );

	trap->Print( S_COLOR_CYAN"Executing Lua code: %s\n", args );
	if ( luaL_dostring( JPLua.state, args ) != 0 )
		trap->Print( S_COLOR_RED"Lua Error: %s\n", lua_tostring( JPLua.state, -1 ) );
}

static void SV_LuaReload_f( void ) {
	JPLua_Shutdown();
	JPLua_Init();
}
#endif

static void SV_Pause_f( void ) {
	//OSP: pause
	if ( level.pause.state == PAUSE_NONE ) {
		level.pause.state = PAUSE_PAUSED;
		level.pause.time = level.time + japp_pauseTime.integer*1000;
	}
	else if ( level.pause.state == PAUSE_PAUSED ) {
		level.pause.state = PAUSE_UNPAUSING;
		level.pause.time = level.time + japp_unpauseTime.integer*1000;
	}
}

static void SV_Say_f( void ) {
	if ( dedicated.integer )
		trap->SendServerCommand( -1, va( "print \"server: %s\n\"", ConcatArgs( 1 ) ) );
}

static void SV_ShuffleTeams_f( void ) {
	// gametype is already filtered in callvote, but filter it here as-well
	if ( level.gametype >= GT_TEAM )
		G_ShuffleTeams();
}

typedef struct svCommand_s {
	const char	*cmd;
	void		(*func)( void );
	//TODO: help function/string
} svCommand_t;

static const svCommand_t svCommands[] = {
	{ "addbot",						SV_AddBot_f },
	{ "adminadd",					SV_AdminAdd_f },
	{ "admindel",					SV_AdminDel_f },
	{ "adminlist",					SV_AdminList_f },
	{ "adminreload",				SV_AdminReload_f },
	{ "allready",					SV_AllReady_f },
	{ "banadd",						SV_BanAdd_f },
	{ "bandel",						SV_BanDel_f },
	{ "banlist",					SV_BanList_f },
	{ "banreload",					SV_BanReload_f },
	{ "botlist",					SV_BotList_f },
	{ "cointoss",					SV_Cointoss_f },
	{ "entitylist",					SV_EntityList_f },
	{ "forceteam",					SV_ForceTeam_f },
	{ "gametype",					SV_Gametype_f },
	{ "game_memory",				SV_GameMemory_f },
	{ "lsmaps",						SV_ListMaps_f },
	{ "lua",						SV_Lua_f },
	{ "lua_reload",					SV_LuaReload_f },
	{ "pause",						SV_Pause_f },
	{ "say",						SV_Say_f },
	{ "shuffle",					SV_ShuffleTeams_f },
	{ "toggleuserinfovalidation",	SV_ToggleUserinfoValidation_f },
};
static const int numSvCommands = ARRAY_LEN( svCommands );

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((svCommand_t*)b)->cmd );
}

qboolean ConsoleCommand( void ) {
	svCommand_t *command = NULL;
	char cmd[MAX_TOKEN_CHARS] = {0};

	trap->Argv( 0, cmd, sizeof( cmd ) );

	if ( JPLua_Event_ServerCommand() )
		return qtrue;

	command = (svCommand_t *)bsearch( cmd, svCommands, numSvCommands, sizeof( svCommands[0] ), cmdcmp );
	if ( !command )
		return qfalse;

	command->func();

	return qtrue;
}

