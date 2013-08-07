// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"
#include "g_admin.h"
#include "g_engine.h"

#include "ui/menudef.h"			// for the voice chats

//rww - for getting bot commands...
int AcceptBotCommand(char *cmd, gentity_t *pl);
//end rww

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

void Cmd_NPC_f( gentity_t *ent );
void SetTeamQuick(gentity_t *ent, int team, qboolean doBegin);

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char		entry[MAX_STRING_CHARS] = {0};
	char		string[1022] = {0};
	int			stringlength=0;
	int			i, j;
	gclient_t	*cl = NULL;
	int			numSorted, scoreFlags=0, accuracy, perfect;

	// send the latest information on all clients
	numSorted = level.numConnectedClients;
	
	if ( !Client_Supports( ent, CSF_SCOREBOARD_LARGE ) )
	{
		CAP( numSorted, MAX_CLIENT_SCORE_SEND );
	}

	for ( i=0; i<numSorted; i++ )
	{
		int ping;

		cl = &level.clients[level.sortedClients[i]];

		ping		= (cl->pers.connected == CON_CONNECTING) ? -1 : Com_Clampi( 0, 999, cl->ps.ping );
		accuracy	= (cl->accuracy_shots) ? cl->accuracy_hits * 100 / cl->accuracy_shots : 0;
		perfect		= (cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0);

		if ( !Client_Supports( ent, CSF_SCOREBOARD_KD ) )
		{// base, no K/D
			Com_sprintf( entry,
				sizeof( entry ),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				level.sortedClients[i],
				cl->ps.persistant[PERS_SCORE],
				ping,
				(level.time - cl->pers.enterTime)/60000,
				scoreFlags,
				g_entities[level.sortedClients[i]].s.powerups,
				accuracy, 
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
				cl->ps.persistant[PERS_DEFEND_COUNT], 
				cl->ps.persistant[PERS_ASSIST_COUNT], 
				perfect,
				cl->ps.persistant[PERS_CAPTURES] );
		}
		else
		{// client mod hints K/D
			Com_sprintf( entry,
				sizeof( entry ),
				" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
				level.sortedClients[i],
				cl->ps.persistant[PERS_SCORE],
				ping,
				(level.time - cl->pers.enterTime)/60000,
				scoreFlags,
				g_entities[level.sortedClients[i]].s.powerups,
				accuracy, 
				cl->ps.persistant[PERS_IMPRESSIVE_COUNT],
				cl->ps.persistant[PERS_EXCELLENT_COUNT],
				cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], 
				cl->ps.persistant[PERS_DEFEND_COUNT], 
				cl->ps.persistant[PERS_ASSIST_COUNT], 
				perfect,
				cl->ps.persistant[PERS_CAPTURES],
				cl->ps.persistant[PERS_KILLED] );
		}

		//Protect against client overflow
		j = strlen( entry );
		if ( stringlength + j >= 1022 )
			break;

		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	trap_SendServerCommand( ent-g_entities, va( "scores %i %i %i%s",
											level.numConnectedClients,
											level.teamScores[TEAM_RED],
											level.teamScores[TEAM_BLUE],
											string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
ConcatArgs
==================
*/
char	*ConcatArgs( int start ) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;		// skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( (unsigned char) *in++ );
	}

	*out = 0;
}

/*
==================
StringIsInteger
==================
*/
qboolean StringIsInteger( const char *s ) {
	int			i=0, len=0;
	qboolean	foundDigit=qfalse;

	for ( i=0, len=strlen( s ); i<len; i++ )
	{
		if ( !isdigit( s[i] ) )
			return qfalse;

		foundDigit = qtrue;
	}

	return foundDigit;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, const char *s ) {
	gclient_t	*cl;
	int			idnum;
	char		cleanName[MAX_NETNAME];

	if ( StringIsInteger( s ) )
	{// numeric values could be slot numbers
		idnum = atoi( s );
		if ( idnum >= 0 && idnum < level.maxclients )
		{
			cl = &level.clients[idnum];
			if ( cl->pers.connected == CON_CONNECTED )
				return idnum;
		}
	}

	for ( idnum=0,cl=level.clients; idnum < level.maxclients; idnum++,cl++ )
	{// check for a name match
		if ( cl->pers.connected != CON_CONNECTED )
			continue;

		Q_strncpyz( cleanName, cl->pers.netname, sizeof( cleanName ) );
		Q_CleanStr( cleanName );
		if ( !Q_stricmp( cleanName, s ) )
			return idnum;
	}

	trap_SendServerCommand( to-g_entities, va( "print \"User %s is not on the server\n\"", s ) );
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void G_Give( gentity_t *ent, const char *name, const char *args, int argc )
{
	gitem_t		*it;
	int			i;
	qboolean	give_all = qfalse;
	gentity_t	*it_ent;
	trace_t		trace;

	if ( !Q_stricmp( name, "all" ) )
		give_all = qtrue;

	if ( give_all )
	{
		for ( i=0; i<HI_NUM_HOLDABLE; i++ )
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
	}

	if ( give_all || !Q_stricmp( name, "health") )
	{
		if ( argc == 3 )
			ent->health = Com_Clampi( 1, ent->client->ps.stats[STAT_MAX_HEALTH], atoi( args ) );
		else
		{
			if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
				ent->health = bgSiegeClasses[ent->client->siegeClass].maxhealth;
			else
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "armor" ) || !Q_stricmp( name, "shield" ) )
	{
		if ( argc == 3 )
			ent->client->ps.stats[STAT_ARMOR] = Com_Clampi( 0, ent->client->ps.stats[STAT_MAX_HEALTH], atoi( args ) );
		else
		{
			if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
				ent->client->ps.stats[STAT_ARMOR] = bgSiegeClasses[ent->client->siegeClass].maxarmor;
			else
				ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];
		}

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "force" ) )
	{
		if ( argc == 3 )
			ent->client->ps.fd.forcePower = Com_Clampi( 0, ent->client->ps.fd.forcePowerMax, atoi( args ) );
		else
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax;

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "weapons" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON+1)) - ( 1 << WP_NONE );
		if ( !give_all )
			return;
	}
	
	if ( !give_all && !Q_stricmp( name, "weaponnum" ) )
	{
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi( args ));
		return;
	}

	if ( give_all || !Q_stricmp( name, "ammo" ) )
	{
		int num = 999;
		if ( argc == 3 )
			num = atoi( args );
		for ( i=0; i<MAX_WEAPONS; i++ )
			ent->client->ps.ammo[i] = num;
		if ( !give_all )
			return;
	}

	if ( !Q_stricmp( name, "excellent" ) ) {
		ent->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "impressive" ) ) {
		ent->client->ps.persistant[PERS_IMPRESSIVE_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "gauntletaward" ) ) {
		ent->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "defend" ) ) {
		ent->client->ps.persistant[PERS_DEFEND_COUNT]++;
		return;
	}
	if ( !Q_stricmp( name, "assist" ) ) {
		ent->client->ps.persistant[PERS_ASSIST_COUNT]++;
		return;
	}

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it )
			return;

		it_ent = G_Spawn();
		VectorCopy( &ent->r.currentOrigin, &it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		Touch_Item( it_ent, ent, &trace );
		if ( it_ent->inuse )
			G_FreeEntity( it_ent );
	}
}

void Cmd_Give_f( gentity_t *ent )
{
	char name[MAX_TOKEN_CHARS] = {0};

	trap_Argv( 1, name, sizeof( name ) );
	G_Give( ent, name, ConcatArgs( 2 ), trap_Argc() );
}

void Cmd_GiveOther_f( gentity_t *ent )
{
	char		name[MAX_TOKEN_CHARS] = {0};
	int			i;
	char		otherindex[MAX_TOKEN_CHARS];
	gentity_t	*otherEnt = NULL;

	trap_Argv( 1, otherindex, sizeof( otherindex ) );
	if ( !otherindex[0] )
	{
		trap_SendServerCommand( ent-g_entities, "print \"giveother requires that the second argument be a client index number.\n\"" );
		return;
	}

	i = atoi( otherindex );
	if ( i < 0 || i >= MAX_CLIENTS )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"%i is not a client index.\n\"", i ) );
		return;
	}

	otherEnt = &g_entities[i];
	if ( !otherEnt->inuse || !otherEnt->client )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"%i is not an active client.\n\"", i ) );
		return;
	}

	trap_Argv( 2, name, sizeof( name ) );

	G_Give( otherEnt, name, ConcatArgs( 3 ), trap_Argc()-1 );
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->flags ^= FL_GODMODE;
	if ( !(ent->flags & FL_GODMODE) )
		msg = "godmode OFF";
	else
		msg = "godmode ON";

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->flags ^= FL_NOTARGET;
	if ( !(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF";
	else
		msg = "notarget ON";

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char *msg = NULL;

	ent->client->noclip = !ent->client->noclip;
	if ( !ent->client->noclip )
		msg = "noclip OFF";
	else
		msg = "noclip ON";

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent )
{
	if ( !ent->client->pers.localClient )
	{
		trap_SendServerCommand(ent-g_entities, "print \"The levelshot command must be executed by a local client\n\"");
		return;
	}

	// doesn't work in single player
	if ( level.gametype == GT_SINGLE_PLAYER )
	{
		trap_SendServerCommand(ent-g_entities, "print \"Must not be in singleplayer mode for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent-g_entities, "clientLevelShot" );
}


#if 0
/*
==================
Cmd_TeamTask_f

From TA.
==================
*/
void Cmd_TeamTask_f( gentity_t *ent ) {
	char userinfo[MAX_INFO_STRING];
	char		arg[MAX_TOKEN_CHARS];
	int task;
	int client = ent->client - level.clients;

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	task = atoi( arg );

	trap_GetUserinfo(client, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "teamtask", va("%d", task));
	trap_SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);
}
#endif // 0



/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->ps.fallingToDeath && !japp_allowFallSuicide.integer )
		return;

	//OSP: pause
	if ( level.pause.state != PAUSE_NONE )
		return;

	if ((level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) &&
		level.numPlayingClients > 1 && !level.warmupTime)
	{
		if (!g_allowDuelSuicide.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "ATTEMPTDUELKILL")) );
			return;
		}
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die (ent, ent, ent, 100000, MOD_SUICIDE);
}

gentity_t *G_GetDuelWinner(gclient_t *client)
{
	gclient_t *wCl;
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		wCl = &level.clients[i];
		
		if (wCl && wCl != client && /*wCl->ps.clientNum != client->ps.clientNum &&*/
			wCl->pers.connected == CON_CONNECTED && wCl->sess.sessionTeam != TEAM_SPECTATOR)
		{
			return &g_entities[wCl->ps.clientNum];
		}
	}

	return NULL;
}

static int G_ClientNumFromNetname(char *name)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->client &&
			!Q_stricmp(ent->client->pers.netname, name))
		{
			return ent->s.number;
		}
		i++;
	}

	return -1;
}

void Cmd_KillOther_f( gentity_t *ent ) {
	if ( trap_Argc() > 1 )
	{
		char sArg[MAX_STRING_CHARS] = {0};
		int entNum = 0;

		trap_Argv( 1, sArg, sizeof( sArg ) );

		entNum = G_ClientNumFromNetname( sArg );

		if ( entNum >= 0 && entNum < MAX_GENTITIES )
		{
			gentity_t *kEnt = &g_entities[entNum];

			if ( kEnt->inuse && kEnt->client )
			{
				kEnt->flags &= ~FL_GODMODE;
				kEnt->client->ps.stats[STAT_HEALTH] = kEnt->health = -999;
				player_die( kEnt, kEnt, kEnt, 100000, MOD_SUICIDE );
			}
		}
	}
}

/*
=================
BroadCastTeamChange

Let everyone know about a team change
=================
*/
void BroadcastTeamChange( gclient_t *client, int oldTeam )
{
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if (level.gametype == GT_SIEGE)
	{ //don't announce these things in siege
		return;
	}

	if ( client->sess.sessionTeam == TEAM_RED ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEREDTEAM")) );
	} else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBLUETEAM")));
	} else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
		client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHESPECTATORS")));
	} else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		{
			/*
			gentity_t *currentWinner = G_GetDuelWinner(client);

			if (currentWinner && currentWinner->client)
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s %s\n\"",
				currentWinner->client->pers.netname, G_GetStringEdString("MP_SVGAME", "VERSUS"), client->pers.netname));
			}
			else
			{
				trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
				client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
			}
			*/
			//NOTE: Just doing a vs. once it counts two players up
		}
		else
		{
			trap_SendServerCommand( -1, va("cp \"%s" S_COLOR_WHITE " %s\n\"",
			client->pers.netname, G_GetStringEdString("MP_SVGAME", "JOINEDTHEBATTLE")));
		}
	}

	G_LogPrintf ( "setteam:  %i %s %s\n",
				  client - &level.clients[0],
				  TeamName ( oldTeam ),
				  TeamName ( client->sess.sessionTeam ) );
}

qboolean G_PowerDuelCheckFail(gentity_t *ent)
{
	int			loners = 0;
	int			doubles = 0;

	if (!ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE)
	{
		return qtrue;
	}

	G_PowerDuelCount(&loners, &doubles, qfalse);

	if (ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1)
	{
		return qtrue;
	}

	if (ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2)
	{
		return qtrue;
	}

	return qfalse;
}

/*
=================
SetTeam
=================
*/
qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
void SetTeam( gentity_t *ent, char *s ) {
	int					team, oldTeam;
	gclient_t			*client;
	int					clientNum;
	spectatorState_t	specState;
	int					specClient;
	int					teamLeader;

	//Raz: this prevents rare creation of invalid players
	if ( !ent->inuse )
		return;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client-level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( level.gametype >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			team = PickTeam( clientNum );
		}

		if ( g_teamForceBalance.integer && !g_jediVmerc.integer ) {
			int		counts[TEAM_NUM_TEAMS];

			//[ClientNumFix]
			counts[TEAM_BLUE] = TeamCount( ent-g_entities, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent-g_entities, TEAM_RED );
			//[/ClientNumFix]

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_DARKSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED_SWITCH")) );
				}
				else
				*/
				{
				//[ClientNumFix]
				trap_SendServerCommand( ent-g_entities, 
				//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYRED")) );
				}
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				//For now, don't do this. The legalize function will set powers properly now.
				/*
				if (g_forceBasedTeams.integer && ent->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					trap_SendServerCommand( ent->client->ps.clientNum, 
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE_SWITCH")) );
				}
				else
				*/
				{
				//[ClientNumFix]
				trap_SendServerCommand( ent-g_entities, 
				//[/ClientNumFix]
						va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "TOOMANYBLUE")) );
				}
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}

		//For now, don't do this. The legalize function will set powers properly now.
		/*
		if (g_forceBasedTeams.integer)
		{
			if (team == TEAM_BLUE && ent->client->ps.fd.forceSide != FORCE_LIGHTSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBELIGHT")) );
				return;
			}
			if (team == TEAM_RED && ent->client->ps.fd.forceSide != FORCE_DARKSIDE)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "MUSTBEDARK")) );
				return;
			}
		}
		*/

	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	//[BugFix41]
	oldTeam = client->sess.sessionTeam;
	//[/BugFix41]

	if (level.gametype == GT_SIEGE)
	{
		if (client->tempSpectate >= level.time &&
			team == TEAM_SPECTATOR)
		{ //sorry, can't do that.
			return;
		}

		//[BugFix41]
		if ( team == oldTeam && team != TEAM_SPECTATOR ) {
			return;
		}
		//[/BugFix41]

		client->sess.siegeDesiredTeam = team;
		//oh well, just let them go.
		/*
		if (team != TEAM_SPECTATOR)
		{ //can't switch to anything in siege unless you want to switch to being a fulltime spectator
			//fill them in on their objectives for this team now
			trap_SendServerCommand(ent-g_entities, va("sb %i", client->sess.siegeDesiredTeam));

			trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time the round begins.\n\"") );
			return;
		}
		*/
		if (client->sess.sessionTeam != TEAM_SPECTATOR &&
			team != TEAM_SPECTATOR)
		{ //not a spectator now, and not switching to spec, so you have to wait til you die.
			//trap_SendServerCommand( ent-g_entities, va("print \"You will be on the selected team the next time you respawn.\n\"") );
			qboolean doBegin;
			if (ent->client->tempSpectate >= level.time)
			{
				doBegin = qfalse;
			}
			else
			{
				doBegin = qtrue;
			}

			if (doBegin)
			{
				// Kill them so they automatically respawn in the team they wanted.
				if (ent->health > 0)
				{
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die( ent, ent, ent, 100000, MOD_TEAM_CHANGE ); 
				}
			}

			if (ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam)
			{
				SetTeamQuick(ent, ent->client->sess.siegeDesiredTeam, qfalse);
			}

			return;
		}
	}

	// override decision if limiting the players
	if ( (level.gametype == GT_DUEL)
		&& level.numNonSpectatorClients >= 2 )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( (level.gametype == GT_POWERDUEL)
		&& (level.numPlayingClients >= 3 || G_PowerDuelCheckFail(ent)) )
	{
		team = TEAM_SPECTATOR;
	}
	else if ( g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer )
	{
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	//[BugFix41]
	// moved this up above the siege check
	//oldTeam = client->sess.sessionTeam;
	//[/BugFix41]
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	//
	// execute the team change
	//

	//If it's siege then show the mission briefing for the team you just joined.
//	if (level.gametype == GT_SIEGE && team != TEAM_SPECTATOR)
//	{
//		trap_SendServerCommand(clientNum, va("sb %i", team));
//	}

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR ) {
		MaintainBodyQueue(ent);
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		g_dontPenalizeTeam = qfalse;

	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		if ( (level.gametype != GT_DUEL) || (oldTeam != TEAM_SPECTATOR) )	{//so you don't get dropped to the bottom of the queue for changing skins, etc.
			client->sess.spectatorTime = level.time;
		}
		G_ClearVote( ent );
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	client->sess.teamLeader = qfalse;
	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		teamLeader = TeamLeader( team );
		// if there is no team leader or the team leader is a bot and this client is not a bot
		if ( teamLeader == -1 || ( !(g_entities[clientNum].r.svFlags & SVF_BOT) && (g_entities[teamLeader].r.svFlags & SVF_BOT) ) ) {
			//SetLeader( team, clientNum );
		}
	}
	// make sure there is a team leader on the team the player came from
	if ( oldTeam == TEAM_RED || oldTeam == TEAM_BLUE ) {
		CheckTeamLeader( oldTeam );
	}

	BroadcastTeamChange( client, oldTeam );

	//make a disappearing effect where they were before teleporting them to the appropriate spawn point,
	//if we were not on the spec team
	if (oldTeam != TEAM_SPECTATOR)
	{
		gentity_t *tent = G_TempEntity( &client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( clientNum ) )
		return;

	if (!g_preventTeamBegin)
	{
		ClientBegin( clientNum, qfalse );
	}
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
extern void G_LeaveVehicle( gentity_t *ent, qboolean ConCheck );
void StopFollowing( gentity_t *ent ) {
	int i=0;
	ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;	
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;	
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	//Raz: Bug fix with vehicles, from OJP code
	G_LeaveVehicle( ent, qfalse ); // clears m_iVehicleNum as well
	//ent->client->ps.m_iVehicleNum = 0;
	ent->client->ps.viewangles.roll = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = 0;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.legsAnim = 0;
	ent->client->ps.legsTimer = 0;
	ent->client->ps.torsoAnim = 0;
	ent->client->ps.torsoTimer = 0;
	//Raz: JK2 gametypes
	ent->client->ps.isJediMaster = qfalse; // major exploit if you are spectating somebody and they are JM and you reconnect
	ent->client->ps.cloakFuel = 100; // so that fuel goes away after stop following them
	ent->client->ps.jetpackFuel = 100; // so that fuel goes away after stop following them
	ent->health = ent->client->ps.stats[STAT_HEALTH] = 100; // so that you don't keep dead angles if you were spectating a dead person
	ent->client->ps.bobCycle = 0;
	for ( i=0; i<PW_NUM_POWERUPS; i++ )
		ent->client->ps.powerups[i] = 0;
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	oldTeam = ent->client->sess.sessionTeam;

	if ( trap_Argc() != 2 ) {
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTBLUETEAM")) );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTREDTEAM")) );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTFREETEAM")) );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PRINTSPECTEAM")) );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	if (gEscaping)
	{
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( level.gametype == GT_DUEL
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {//in a tournament game
		//disallow changing teams
		trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Duel\n\"" );
		return;
		//FIXME: why should this be a loss???
		//ent->client->sess.losses++;
	}

	if (level.gametype == GT_POWERDUEL)
	{ //don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		trap_SendServerCommand( ent-g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );

	//Raz: update team switch time only if team change really happened
	if ( oldTeam != ent->client->sess.sessionTeam )
		ent->client->switchTeamTime = level.time + 5000;
}

/*
=================
Cmd_DuelTeam_f
=================
*/
void Cmd_DuelTeam_f(gentity_t *ent)
{
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	if (level.gametype != GT_POWERDUEL)
	{ //don't bother doing anything if this is not power duel
		return;
	}

	/*
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"You cannot change your duel team unless you are a spectator.\n\""));
		return;
	}
	*/

	if ( trap_Argc() != 2 )
	{ //No arg so tell what team we're currently on.
		oldTeam = ent->client->sess.duelTeam;
		switch ( oldTeam )
		{
		case DUELTEAM_FREE:
			trap_SendServerCommand( ent-g_entities, va("print \"None\n\"") );
			break;
		case DUELTEAM_LONE:
			trap_SendServerCommand( ent-g_entities, va("print \"Single\n\"") );
			break;
		case DUELTEAM_DOUBLE:
			trap_SendServerCommand( ent-g_entities, va("print \"Double\n\"") );
			break;
		default:
			break;
		}
		return;
	}

	if ( ent->client->switchDuelTeamTime > level.time )
	{ //debounce for changing
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	trap_Argv( 1, s, sizeof( s ) );

	oldTeam = ent->client->sess.duelTeam;

	if (!Q_stricmp(s, "free"))
	{
		ent->client->sess.duelTeam = DUELTEAM_FREE;
	}
	else if (!Q_stricmp(s, "single"))
	{
		ent->client->sess.duelTeam = DUELTEAM_LONE;
	}
	else if (!Q_stricmp(s, "double"))
	{
		ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"'%s' not a valid duel team.\n\"", s) );
	}

	if (oldTeam == ent->client->sess.duelTeam)
	{ //didn't actually change, so don't care.
		return;
	}

	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
	{ //ok..die
		int curTeam = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = oldTeam;
		G_Damage(ent, ent, ent, NULL, &ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE);
		ent->client->sess.duelTeam = curTeam;
	}
	//reset wins and losses
	ent->client->sess.wins = 0;
	ent->client->sess.losses = 0;

	//get and distribute relevent paramters
	if ( ClientUserinfoChanged( ent->s.number ) )
		return;

	ent->client->switchDuelTeamTime = level.time + 5000;
}

int G_TeamForSiegeClass(const char *clName)
{
	int i = 0;
	int team = SIEGETEAM_TEAM1;
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam(team);
	siegeClass_t *scl;

	if (!stm)
	{
		return 0;
	}

	while (team <= SIEGETEAM_TEAM2)
	{
		scl = stm->classes[i];

		if (scl && scl->name[0])
		{
			if (!Q_stricmp(clName, scl->name))
			{
				return team;
			}
		}

		i++;
		if (i >= MAX_SIEGE_CLASSES || i >= stm->numClasses)
		{
			if (team == SIEGETEAM_TEAM2)
			{
				break;
			}
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam(team);
			i = 0;
		}
	}

	return 0;
}

/*
=================
Cmd_SiegeClass_f
=================
*/
void Cmd_SiegeClass_f( gentity_t *ent )
{
	char className[64];
	int team = 0;
	int preScore;
	qboolean startedAsSpec = qfalse;

	if (level.gametype != GT_SIEGE)
	{ //classes are only valid for this gametype
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (trap_Argc() < 1)
	{
		return;
	}

	if ( ent->client->switchClassTime > level.time )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSSWITCH")) );
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		startedAsSpec = qtrue;
	}

	trap_Argv( 1, className, sizeof( className ) );

	team = G_TeamForSiegeClass(className);

	if (!team)
	{ //not a valid class name
		return;
	}

	if (ent->client->sess.sessionTeam != team)
	{ //try changing it then
		g_preventTeamBegin = qtrue;
		if (team == TEAM_RED)
		{
			SetTeam(ent, "red");
		}
		else if (team == TEAM_BLUE)
		{
			SetTeam(ent, "blue");
		}
		g_preventTeamBegin = qfalse;

		if (ent->client->sess.sessionTeam != team)
		{ //failed, oh well
			if (ent->client->sess.sessionTeam != TEAM_SPECTATOR ||
				ent->client->sess.siegeDesiredTeam != team)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOCLASSTEAM")) );
				return;
			}
		}
	}

	//preserve 'is score
	preScore = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality(team, className);

	//Set the session data
	strcpy(ent->client->sess.siegeClass, className);

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( ent->s.number ) )
		return;

	if (ent->client->tempSpectate < level.time)
	{
		// Kill him (makes sure he loses flags, etc)
		if (ent->health > 0 && !startedAsSpec)
		{
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die (ent, ent, ent, 100000, MOD_SUICIDE);
		}

		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec)
		{ //respawn them instantly.
			ClientBegin( ent->s.number, qfalse );
		}
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = preScore;

	ent->client->switchClassTime = level.time + 5000;
}

/*
=================
Cmd_ForceChanged_f
=================
*/
void Cmd_ForceChanged_f( gentity_t *ent )
{
	char fpChStr[1024];
	const char *buf;
//	Cmd_Kill_f(ent);
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
	{ //if it's a spec, just make the changes now
		//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "FORCEAPPLIED")) );
		//No longer print it, as the UI calls this a lot.
		WP_InitForcePowers( ent );
		goto argCheck;
	}

	buf = G_GetStringEdString("MP_SVGAME", "FORCEPOWERCHANGED");

	strcpy(fpChStr, buf);

	trap_SendServerCommand( ent-g_entities, va("print \"%s%s\n\n\"", S_COLOR_GREEN, fpChStr) );

	ent->client->ps.fd.forceDoInit = 1;
argCheck:
	if (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
	{ //If this is duel, don't even bother changing team in relation to this.
		return;
	}

	if (trap_Argc() > 1)
	{
		char	arg[MAX_TOKEN_CHARS];

		trap_Argv( 1, arg, sizeof( arg ) );

		if (arg[0])
		{ //if there's an arg, assume it's a combo team command from the UI.
			Cmd_Team_f(ent);
		}
	}
}

extern qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
extern qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
qboolean G_SetSaber(gentity_t *ent, int saberNum, char *saberName, qboolean siegeOverride)
{
	char truncSaberName[MAX_QPATH] = {0};

	if ( !siegeOverride && level.gametype == GT_SIEGE && ent->client->siegeClass != -1 &&
		(bgSiegeClasses[ent->client->siegeClass].saberStance || bgSiegeClasses[ent->client->siegeClass].saber1[0] || bgSiegeClasses[ent->client->siegeClass].saber2[0]) )
	{ //don't let it be changed if the siege class has forced any saber-related things
		return qfalse;
	}

	Q_strncpyz( truncSaberName, saberName, sizeof( truncSaberName ) );

	if ( saberNum == 0 && !Q_stricmp( "none", truncSaberName ) || !Q_stricmp( "remove", truncSaberName ) )
	{ //can't remove saber 0 like this
		Q_strncpyz( truncSaberName, DEFAULT_SABER, sizeof( truncSaberName ) );
	}

	//Set the saber with the arg given. If the arg is
	//not a valid sabername defaults will be used.
	WP_SetSaber( ent->s.number, ent->client->saber, saberNum, truncSaberName );

	if ( !ent->client->saber[0].model[0] )
	{
		assert(0); //should never happen!
		Q_strncpyz( ent->client->pers.saber1, DEFAULT_SABER, sizeof( ent->client->pers.saber1 ) );
	}
	else
		Q_strncpyz( ent->client->pers.saber1, ent->client->saber[0].name, sizeof( ent->client->pers.saber1 ) );

	if ( !ent->client->saber[1].model[0] )
		Q_strncpyz( ent->client->pers.saber2, "none", sizeof( ent->client->pers.saber2 ) );
	else
		Q_strncpyz( ent->client->pers.saber2, ent->client->saber[1].name, sizeof( ent->client->pers.saber2 ) );

	if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) )
	{
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	return qtrue;
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	//[BugFix38]
	// can't follow another spectator
	if ( level.clients[ i ].tempSpectate >= level.time ) {
		return;
	}
	//[/BugFix38]

	// if they are playing a tournement game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {
		//WTF???
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int		clientnum;
	int		original;
	qboolean	looped = qfalse;

	// if they are playing a tournement game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& ent->client->sess.sessionTeam == TEAM_FREE ) {\
		//WTF???
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients )
		{
			//Raz: Avoid /team follow1 crash
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = 0;
				looped = qtrue;
			}
		}
		if ( clientnum < 0 ) {
			if ( looped )
			{
				clientnum = original;
				break;
			}
			else
			{
				clientnum = level.maxclients - 1;
				looped = qtrue;
			}
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].tempSpectate >= level.time ) {
			return;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}

void Cmd_FollowNext_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, 1 );
}

void Cmd_FollowPrev_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, -1 );
}

/*
==================
G_Say
==================
*/

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, char *locMsg )
{
	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if ( other->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam(ent, other) ) {
		return;
	}
	if ( mode == SAY_ADMIN && !other->client->pers.adminUser )
		return;
	/*
	// no chatting to players in tournements
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)
		&& other->client->sess.sessionTeam == TEAM_FREE
		&& ent->client->sess.sessionTeam != TEAM_FREE ) {
		//Hmm, maybe some option to do so if allowed?  Or at least in developer mode...
		return;
	}
	*/
	//They've requested I take this out.

	if (level.gametype == GT_SIEGE &&
		ent->client && (ent->client->tempSpectate >= level.time || ent->client->sess.sessionTeam == TEAM_SPECTATOR) &&
		other->client->sess.sessionTeam != TEAM_SPECTATOR &&
		other->client->tempSpectate < level.time)
	{ //siege temp spectators should not communicate to ingame players
		return;
	}

	if (locMsg)
	{
		trap_SendServerCommand( other-g_entities, va("%s \"%s\" \"%s\" \"%c\" \"%s\"", 
			mode == SAY_TEAM ? "ltchat" : "lchat",
			name, locMsg, color, message));
	}
	else
	{
		trap_SendServerCommand( other-g_entities, va("%s \"%s%c%c%s\"", 
			mode == SAY_TEAM ? "tchat" : "chat",
			name, Q_COLOR_ESCAPE, color, message));
	}
}

#define EC			"\x19"
#define CHANNEL_EC	"\x10"
#define ADMIN_EC	"\x11"
#define PRIVATE_EC	"\x12"

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			j;
	gentity_t	*other;
	int			color;
	char		name[96];
	// don't let text be too long for malicious reasons
	char		text[MAX_SAY_TEXT];
	char		location[64];
	char		*locMsg = NULL;
	qboolean	isMeCmd = qfalse;
	qboolean	returnToSender = qfalse;

	if ( level.gametype < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	//[Admin]
	if ( !ent->client->pers.adminData.canTalk )
		return;
	//[/Admin]

	if ( strstr( ent->client->pers.netname, "<Admin>" ) || Q_strchrs( chatText, "\n\r" ) )
		returnToSender = qtrue;

	switch ( mode ) {
	default:
	case SAY_ADMIN:
		G_LogPrintf( "amsay: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf (name, sizeof(name), "^3<Admin	>^7%s%c%c"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_YELLOW;
		break;
	case SAY_ALL:
		if ( !Q_stricmpn( chatText, "/me ", 4 ) )
		{//A /me command
			isMeCmd = qtrue;
			chatText += 4; //Skip "^7* "
		}
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		if ( isMeCmd )
		{
			Com_sprintf( name, sizeof( name ), "^7* %s^7"EC" ", ent->client->pers.netname );
			color = COLOR_WHITE;
		}
		else
		{
			Com_sprintf( name, sizeof( name ), "%s^7"EC": ", ent->client->pers.netname );
			color = COLOR_GREEN;
		}
		break;
	case SAY_TEAM:
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if (Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"(%s%c%c"EC")"EC": ", 
				ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if (target && level.gametype >= GT_TEAM &&
			target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg(ent, location, sizeof(location)))
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
		{
			Com_sprintf (name, sizeof(name), EC"[%s%c%c"EC"]"EC": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, locMsg );
		return;
	}

	if ( returnToSender )
	{//Raz: Silly kids, make it look like they said something
		G_SayTo( ent, ent, mode, color, name, text, locMsg );
		return;
	}

	// echo the text to the console
	if ( dedicated.integer ) {
		G_Printf( "%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text, locMsg );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap_Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT )
	{
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, SAY_ALL, p );
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( gentity_t *ent ) {
	char *p = NULL;

	if ( trap_Argc () < 2 )
		return;

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT )
	{
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_Say( ent, NULL, (level.gametype>=GT_TEAM) ? SAY_TEAM : SAY_ALL, p );
}

/*
==================
Cmd_Tell_f
==================
*/
#if 1
void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p;
	char		arg[MAX_TOKEN_CHARS];

	if ( trap_Argc () < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = ClientNumberFromString( ent, arg );
	if ( targetNum == -1 ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT )
	{
		p[MAX_SAY_TEXT-1] = '\0';
		G_SecurityLogPrintf( "Cmd_Tell_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT)) {
		G_Say( ent, ent, SAY_TELL, p );
	}
}

//siege voice command
void Cmd_VoiceCommand_f(gentity_t *ent)
{
	gentity_t *te;
	char arg[MAX_TOKEN_CHARS];
	char *s;
	int i = 0;

	if (trap_Argc() < 2)
	{
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		ent->client->tempSpectate >= level.time)
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOICECHATASSPEC")) );
		return;
	}

	trap_Argv(1, arg, sizeof(arg));

	if (arg[0] == '*')
	{ //hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
		return;
	}

	s = va("*%s", arg);

	//now, make sure it's a valid sound to be playing like this.. so people can't go around
	//screaming out death sounds or whatever.
	while (i < MAX_CUSTOM_SIEGE_SOUNDS)
	{
		if (!bg_customSiegeSoundNames[i])
		{
			break;
		}
		if (!Q_stricmp(bg_customSiegeSoundNames[i], s))
		{ //it matches this one, so it's ok
			break;
		}
		i++;
	}

	if (i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i])
	{ //didn't find it in the list
		return;
	}

	te = G_TempEntity(&vec3_origin, EV_VOICECMD_SOUND);
	te->s.groundEntityNum = ent->s.number;
	te->s.eventParm = G_SoundIndex((char *)bg_customSiegeSoundNames[i]);
	te->r.svFlags |= SVF_BROADCAST;
}
#endif

static char	*gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};
static size_t numgc_orders = ARRAY_LEN( gc_orders );

void Cmd_GameCommand_f( gentity_t *ent ) {
	int				targetNum;
	unsigned int	order;
	gentity_t	*target;
	char		arg[MAX_TOKEN_CHARS] = {0};

	if ( trap_Argc() != 3 ) {
		trap_SendServerCommand( ent-g_entities, va( "print \"Usage: gc <player id> <order 0-%d>\n\"", numgc_orders - 1 ) );
		return;
	}

	trap_Argv( 2, arg, sizeof( arg ) );
	order = atoi( arg );

	if ( order < 0 || order >= numgc_orders ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Bad order: %i\n\"", order));
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = ClientNumberFromString( ent, arg );
	if ( targetNum == -1 )
		return;

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client )
		return;

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order] );
	G_Say( ent, target, SAY_TELL, gc_orders[order] );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT) )
		G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	//[BugFix31]
	//This wasn't working for non-spectators since s.origin doesn't update for active players.
	if(ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR )
	{//active players use currentOrigin
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( &ent->r.currentOrigin ) ) );
	}
	else
	{
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( &ent->s.origin ) ) );
	}
	//trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", vtos( ent->s.origin ) ) );
	//[/BugFix31]
}

static const char *gameNames[] = {
	"Free For All",
	"Holocron FFA",
	"Jedi Master",
	"Duel",
	"Power Duel",
	"Single Player",
	"Team FFA",
	"Siege",
	"Capture the Flag",
	"Capture the Ysalamiri"
};

/*
==================
G_ClientNumberFromName

Finds the client number of the client with the given name
==================
*/
int G_ClientNumberFromName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

/*
==================
SanitizeString2

Rich's revised version of SanitizeString
==================
*/
void SanitizeString2( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = in[i];
		r++;
		i++;
	}
	out[r] = 0;
}

void SanitizeString3( char *in, char *out )
{
	int i = 0;
	int r = 0;

	while (in[i])
	{
		if (i >= MAX_NAME_LENGTH-1)
		{ //the ui truncates the name here..
			break;
		}

		if (in[i] == '^')
		{
			if (in[i+1] >= 48 && //'0'
				in[i+1] <= 57) //'9'
			{ //only skip it if there's a number after it for the color
				i += 2;
				continue;
			}
			else
			{ //just skip the ^
				i++;
				continue;
			}
		}

		if (in[i] < 32)
		{
			i++;
			continue;
		}

		out[r] = tolower(in[i]);
		r++;
		i++;
	}
	out[r] = 0;
}

/*
==================
G_ClientNumberFromStrippedName

Same as above, but strips special characters out of the names before comparing.
==================
*/
int G_ClientNumberFromStrippedName ( const char* name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	gclient_t*	cl;

	// check for a name match
	SanitizeString2( (char*)name, s2 );
	for ( i=0, cl=level.clients ; i < level.numConnectedClients ; i++, cl++ ) 
	{
		SanitizeString2( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) 
		{
			return i;
		}
	}

	return -1;
}

int G_ClientNumberFromStrippedName2( const char *name )
{
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	int			i;
	int			num;
	gclient_t*	cl;

	//	First check for clientNum match
	if ( name[0] >= '0' && name[0] <= '9' )
	{
		num = atoi( name );
		if ( num >=0 && num < MAX_CLIENTS )
			return num;
		else
			return -1;
	}

	//	Failed, check for a name match
	SanitizeString3( (char *)name, s2 );

	for ( i=0, cl=level.clients; i<level.numConnectedClients; i++, cl++ ) 
	{
		SanitizeString3( cl->pers.netname, n2 );

		if ( strstr( n2, s2 ) )
			return i;
	}

	//Failed, target client does not exist
	return -1;
}


/*
==================
Cmd_CallVote_f
==================
*/
extern void SiegeClearSwitchData(void); //g_saga.c
qboolean G_VoteAllready( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	if ( !level.warmupTime )
	{
		trap_SendServerCommand( ent-g_entities, "print \"allready is only available during warmup.\n\"" );
		return qfalse;
	}
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteCapturelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 0x7FFFFFFF, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteClientkick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = atoi ( arg2 );

	if ( n < 0 || n >= MAX_CLIENTS )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"invalid client number %d.\n\"", n ) );
		return qfalse;
	}

	if ( g_entities[n].client->pers.connected == CON_DISCONNECTED )
	{
		trap_SendServerCommand( ent-g_entities, va("print \"there is no client with the client number %d.\n\"", n ) );
		return qfalse;
	}
		
	Com_sprintf( level.voteString, sizeof(level.voteString ), "%s %s", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, g_entities[n].client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteCointoss( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteFraglimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 0x7FFFFFFF, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteGametype( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int gt = atoi( arg2 );

	if ( arg2[0] && isalpha( arg2[0] ) )
	{// ffa, ctf, tdm, etc
		gt = BG_GetGametypeForString( arg2 );
		if ( gt == -1 )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2 ) );
			gt = GT_FFA;
		}
	}
	else if ( gt < 0 || gt >= GT_MAX_GAME_TYPE )
	{// numeric but out of range
		trap_SendServerCommand( ent-g_entities, va( "print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt ) );
		gt = GT_FFA;
	}

	if ( gt == GT_SINGLE_PLAYER )
	{// logically invalid gametypes, or gametypes not fully implemented in MP
		trap_SendServerCommand( ent-g_entities, "print \"This gametype is not supported (%s).\n\"" );
		return qfalse;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, gt );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[gt] );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VotePromode( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteShootFromEye( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteSpeedcaps( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteSuicideDropFlag( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteKick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int clientid = ClientNumberFromString( ent, arg2 );
	gentity_t *target = NULL;

	if ( clientid == -1 )
		return qfalse;

	target = &g_entities[clientid];
	if ( !target || !target->inuse || !target->client )
		return qfalse;

	Com_sprintf( level.voteString, sizeof( level.voteString ), "clientkick %d", clientid );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", target->client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

const char *G_GetArenaInfoByMap( const char *map );

void Cmd_MapList_f( gentity_t *ent ) {
	int i, toggle=0;
	char map[24] = "--", buf[512] = {0};

	Q_strcat( buf, sizeof( buf ), "Map list:" );

	for ( i=0; i<level.arenas.num; i++ )
	{
		Q_strncpyz( map, Info_ValueForKey( level.arenas.infos[i], "map" ), sizeof( map ) );
		Q_CleanColorStr( map );

		if ( G_DoesMapSupportGametype( map, level.gametype ) )
		{
			char *tmpMsg = va( " ^%c%s", (++toggle&1) ? '2' : '3', map );
			if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) )
			{
				trap_SendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
				buf[0] = '\0';
			}
			Q_strcat( buf, sizeof( buf ), tmpMsg );
		}
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
}

qboolean G_VoteMap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char s[MAX_STRING_CHARS] = {0}, *mapName = NULL, *mapName2 = NULL;
	const char *arenaInfo;

	if ( numArgs < 3 )
	{// didn't specify a map, show available maps
		Cmd_MapList_f( ent );
		return qfalse;
	}

	if ( !G_DoesMapSupportGametype( arg2, level.gametype ) )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME" ) ) );
		return qfalse;
	}

	//preserve the map rotation
	trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
	if ( *s )
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
	else
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );

	arenaInfo = G_GetArenaInfoByMap(arg2);
	if ( arenaInfo )
	{
		mapName = Info_ValueForKey(arenaInfo, "longname");
		mapName2 = Info_ValueForKey(arenaInfo, "map");
	}

	if ( !mapName || !mapName[0] )
		mapName = "ERROR";

	if ( !mapName2 || !mapName2[0] )
		mapName2 = "ERROR";

	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "map %s (%s)", mapName, mapName2 );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteNextmap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char	s[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof(s) );
	if ( !*s ) {
		trap_SendServerCommand( ent-g_entities, "print \"nextmap not set.\n\"" );
		return qfalse;
	}
	SiegeClearSwitchData();
	Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap");
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VotePause( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteShuffle( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteTimelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	float tl = Com_Clamp( 0.0f, 35790.0f, atof( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %.3f", arg1, tl );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof( level.voteDisplayString ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	return qtrue;
}

qboolean G_VoteWarmup( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Com_Clampi( 0, 1, atoi( arg2 ) );
	Com_sprintf( level.voteString, sizeof( level.voteString ), "g_doWarmup %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	return qtrue;
}

typedef struct voteString_s {
	const char		*string;
	const char		*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean		(*func)(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int				numArgs;	// number of REQUIRED arguments, not total/optional arguments
	unsigned int	validGT;	// bit-flag of valid gametypes
	qboolean		voteDelay;	// if true, will delay executing the vote string after it's accepted by japp_voteDelay
	const char		*shortHelp;	// NULL if no arguments needed
	const char		*longHelp;		// long help text, accessible with /vhelp and maybe /callvote help
} voteString_t;

static voteString_t validVoteStrings[] = {
	//	vote string				aliases										# args	valid gametypes							exec delay		short help	long help
	{	"allready",				"ready",			G_VoteAllready,			0,		GTB_ALL,								qfalse,			NULL,		"" },
	{	"capturelimit",			"caps",				G_VoteCapturelimit,		1,		GTB_CTF|GTB_CTY,						qtrue,			"<num>",	"" },
	{	"clientkick",			NULL,				G_VoteClientkick,		1,		GTB_ALL,								qfalse,			"<num>",	"" },
	{	"cointoss",				"coinflip",			G_VoteCointoss,			0,		GTB_ALL,								qfalse,			NULL,		"" },
	{	"fraglimit",			"frags",			G_VoteFraglimit,		1,		GTB_ALL & ~(GTB_SIEGE|GTB_CTF|GTB_CTY),	qtrue,			"<num>",	"" },
	{	"g_gametype",			"gametype gt mode",	G_VoteGametype,			1,		GTB_ALL,								qtrue,			"<name>",	"" },
	{	"japp_promode",			"promode cpm",		G_VotePromode,			1,		GTB_ALL,								qtrue,			"<0-1>",	"" },
	{	"japp_shootFromEye",	"shootfromeye",		G_VoteShootFromEye,		1,		GTB_ALL,								qfalse,			"<0-1>",	"" },
	{	"japp_speedCaps",		"speedcaps",		G_VoteSpeedcaps,		1,		GTB_CTF|GTB_CTY,						qfalse,			"<0-1>",	"" },
	{	"japp_suicideDropFlag",	"killdropflag",		G_VoteSuicideDropFlag,	1,		GTB_CTF|GTB_CTY,						qfalse,			"<0-1>",	"Drop flag when you /kill yourself" },
	{	"kick",					NULL,				G_VoteKick,				1,		GTB_ALL,								qfalse,			"<name>",	"" },
	{	"map",					NULL,				G_VoteMap,				0,		GTB_ALL,								qtrue,			"<name>",	"" },
	{	"map_restart",			"restart",			NULL,					0,		GTB_ALL,								qtrue,			NULL,		"Restarts the current map\nExample: callvote map_restart" },
	{	"nextmap",				NULL,				G_VoteNextmap,			0,		GTB_ALL,								qtrue,			NULL,		"" },
	{	"pause",				NULL,				G_VotePause,			0,		GTB_ALL,								qfalse,			NULL,		"" },
	{	"shuffle",				NULL,				G_VoteShuffle,			0,		GTB_ALL & ~(GTB_NOTTEAM),				qtrue,			NULL,		"" },
	{	"timelimit",			"time",				G_VoteTimelimit,		1,		GTB_ALL,								qtrue,			"<num>",	"" },
	{	"warmup",				"dowarmup",			G_VoteWarmup,			1,		GTB_ALL,								qtrue,			"<0-1>",	"" },
};
static const int validVoteStringsSize = ARRAY_LEN( validVoteStrings );

void Cmd_CallVote_f( gentity_t *ent ) {
	int			i=0, numArgs=trap_Argc();
	char		arg1[MAX_CVAR_VALUE_STRING] = {0}, arg2[MAX_CVAR_VALUE_STRING] = {0};
	voteString_t *vote = NULL;

	if ( !g_allowVote.integer )
	{// not allowed to vote at all
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTE")) );
		return;
	}

	else if ( level.voteTime || level.voteExecuteTime >= level.time )
	{// vote in progress
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEINPROGRESS")) );
		return;
	}

	else if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL && ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{// can't vote as a spectator, except in (power)duel
		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSPECVOTE" ) ) );
		return;
	}

	// make sure it is a valid command to vote on
	numArgs = trap_Argc();
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	if ( numArgs > 1 )
		Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) ); //trap_Argv( 2, arg2, sizeof( arg2 ) );

	//Raz: callvote exploit, filter \n and \r ==> in both args
	if ( Q_strchrs( arg1, ";\r\n" ) || Q_strchrs( arg2, ";\r\n" ) )
	{
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	for ( i=0; i<validVoteStringsSize; i++ )
	{// check for invalid votes
		if ( (japp_voteDisable.integer & (1<<i)) )
			continue;

		if ( !Q_stricmp( arg1, validVoteStrings[i].string ) )
			break;

		if ( validVoteStrings[i].aliases )
		{// see if they're using an alias, and set arg1 to the actual vote string
			char tmp[MAX_TOKEN_CHARS] = {0}, *p = NULL, *delim = " ";
			Q_strncpyz( tmp, validVoteStrings[i].aliases, sizeof( tmp ) );
			p = strtok( tmp, delim );
			while ( p != NULL )
			{
				if ( !Q_stricmp( arg1, p ) )
				{
					Q_strncpyz( arg1, validVoteStrings[i].string, sizeof( arg1 ) );
					goto validVote;
				}
				p = strtok( NULL, delim );
			}
		}
	}
	if ( i == validVoteStringsSize )
	{// invalid vote string, abandon ship
		char buf[1024] = {0};
		int toggle = 0;
		trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent-g_entities, "print \"Allowed vote strings are: \"" );
		for ( i=0; i<validVoteStringsSize; i++ )
		{
			if ( (japp_voteDisable.integer & (1<<i)) )
				continue;

			toggle = !toggle;
			if ( validVoteStrings[i].shortHelp ) {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s %s ",
													toggle?'2':'3',
													validVoteStrings[i].string,
													validVoteStrings[i].shortHelp ) );
			}
			else {
				Q_strcat( buf, sizeof( buf ), va( "^%c%s ",
													toggle?'2':'3',
													validVoteStrings[i].string ) );
			}
		}

		//RAZTODO: buffer and send in multiple messages in case of overflow
		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if ( !(vote->validGT & (1<<level.gametype)) ) {
		trap_SendServerCommand( ent-g_entities, va( "print \"%s is not applicable in this gametype.\n\"", arg1 ) );
		return;
	}

	if ( numArgs < vote->numArgs+2 ) {
		trap_SendServerCommand( ent-g_entities, va( "print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp ) );
		return;
	}

	level.votingGametype = qfalse;

	level.voteExecuteDelay = vote->voteDelay ? japp_voteDelay.integer : 0;

	if ( level.voteExecuteTime )
	{// there is still a vote to be executed, execute it and store the new vote
		level.voteExecuteTime = 0;
		if ( level.votePoll )
			trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	if ( vote->func )
	{
		if ( !vote->func( ent, numArgs, arg1, arg2 ) )
			return;
	}
	else
	{
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
		Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	}
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap_SendServerCommand( -1, va( "print \"%s^7 %s (%s)\n\"", ent->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLCALLEDVOTE" ), level.voteStringClean ) );

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	level.votePoll = qfalse;

	for ( i=0; i<level.maxclients; i++ ) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	ent->client->mGameFlags |= PSG_VOTED;
	ent->client->pers.vote = 1;

	trap_SetConfigstring( CS_VOTE_TIME,		va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );	
	trap_SetConfigstring( CS_VOTE_YES,		va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO,		va( "%i", level.voteNo ) );	
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEINPROG")) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_VOTED ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "VOTEALREADY")) );
		return;
	}
	if (level.gametype != GT_DUEL &&
		level.gametype != GT_POWERDUEL)
	{
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOVOTEASSPEC")) );
			return;
		}
	}

	trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "PLVOTECAST")) );

	ent->client->mGameFlags |= PSG_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap_SetConfigstring( CS_VOTE_YES, va("%i", level.voteYes ) );
	} else {
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap_SetConfigstring( CS_VOTE_NO, va("%i", level.voteNo ) );	
	}

	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vector3		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent-g_entities, va("print \"usage: setviewpos x y z yaw\n\""));
		return;
	}

	VectorClear( &angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin.data[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles.yaw = atof( buffer );

	TeleportPlayer( ent, &origin, &angles );
}

//[BugFix38]
void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck ) {

	if (ent->client->ps.m_iVehicleNum)
	{ //tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if (veh->inuse && veh->client && veh->m_pVehicle)
		{
			if ( ConCheck ) { // check connection
				clientConnected_t pCon = ent->client->pers.connected;
				ent->client->pers.connected = CON_DISCONNECTED;
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
				ent->client->pers.connected = pCon;
			} else { // or not.
				veh->m_pVehicle->m_pVehicleInfo->Eject(veh->m_pVehicle, (bgEntity_t *)ent, qtrue);
			}
		}
	}

	ent->client->ps.m_iVehicleNum = 0;
}
//[/BugFix38]

int G_ItemUsable(playerState_t *ps, int forcedUse)
{
	vector3 fwd, fwdorg, dest, pos;
	vector3 yawonly;
	vector3 mins, maxs;
	vector3 trtest;
	trace_t tr;

	//Raz: dead players shouldn't use items
	if ( ps->stats[STAT_HEALTH] <= 0 )
		return 0;

	if (ps->m_iVehicleNum)
	{
		return 0;
	}
	
	if (ps->pm_flags & PMF_USE_ITEM_HELD)
	{ //force to let go first
		return 0;
	}

	if (!forcedUse)
	{
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if (!BG_IsItemSelectable(ps, forcedUse))
	{
		return 0;
	}

	switch (forcedUse)
	{
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if (ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH])
		{
			return 0;
		}

		if (ps->stats[STAT_HEALTH] <= 0)
		{
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if (ps->eFlags & EF_SEEKERDRONE)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED);
			return 0;

		}

		return 1;
	case HI_SENTRY_GUN:
		if (ps->fd.sentryDeployed)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED);
			return 0;
		}

		yawonly.roll = 0;
		yawonly.pitch = 0;
		yawonly.yaw = ps->viewangles.yaw;

		VectorSet( &mins, -8, -8, 0 );
		VectorSet( &maxs, 8, 8, 24 );

		AngleVectors(&yawonly, &fwd, NULL, NULL);

		fwdorg.x = ps->origin.x + fwd.x*64;
		fwdorg.y = ps->origin.y + fwd.y*64;
		fwdorg.z = ps->origin.z + fwd.z*64;

		trtest.x = fwdorg.x + fwd.x*16;
		trtest.y = fwdorg.y + fwd.y*16;
		trtest.z = fwdorg.z + fwd.z*16;

		trap_Trace(&tr, &ps->origin, &mins, &maxs, &trtest, ps->clientNum, MASK_PLAYERSOLID);

		if ((tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid)
		{
			G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM);
			return 0;
		}

		return 1;
	case HI_SHIELD:
		VectorSet( &mins, -8, -8, 0 );
		VectorSet( &maxs,  8,  8, 8 );

		AngleVectors (&ps->viewangles, &fwd, NULL, NULL);
		fwd.z = 0;
		VectorMA(&ps->origin, 64, &fwd, &dest);
		trap_Trace(&tr, &ps->origin, &mins, &maxs, &dest, ps->clientNum, MASK_SHOT );
		if (tr.fraction > 0.9 && !tr.startsolid && !tr.allsolid)
		{
			VectorCopy(&tr.endpos, &pos);
			VectorSet( &dest, pos.x, pos.y, pos.z - 4096 );
			trap_Trace( &tr, &pos, &mins, &maxs, &dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid )
			{
				return 1;
			}
		}
		G_AddEvent(&g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM);
		return 0;
	case HI_JETPACK: //do something?
		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK:
		return 1;
	default:
		return 1;
	}
}

void saberKnockDown(gentity_t *saberent, gentity_t *saberOwner, gentity_t *other);

void Cmd_ToggleSaber_f(gentity_t *ent)
{
	if (ent->client->ps.fd.forceGripCripple)
	{ //if they are being gripped, don't let them unholster their saber
		if (ent->client->ps.saberHolstered)
		{
			return;
		}
	}

	if (ent->client->ps.saberInFlight)
	{
		if (ent->client->ps.saberEntityNum)
		{ //turn it off in midair
			saberKnockDown(&g_entities[ent->client->ps.saberEntityNum], ent, ent);
		}
		return;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return;
	}

	if (ent->client->ps.weapon != WP_SABER)
	{
		return;
	}

//	if (ent->client->ps.duelInProgress && !ent->client->ps.saberHolstered)
//	{
//		return;
//	}

	if (ent->client->ps.duelTime >= level.time)
	{
		return;
	}

	if (ent->client->ps.saberLockTime >= level.time)
	{
		return;
	}

	if (ent->client && ent->client->ps.weaponTime < 1)
	{
		if (ent->client->ps.saberHolstered == 2)
		{
			ent->client->ps.saberHolstered = 0;

			if (ent->client->saber[0].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			}
			if (ent->client->saber[1].soundOn)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
			}
		}
		else
		{
			ent->client->ps.saberHolstered = 2;
			if (ent->client->saber[0].soundOff)
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
			}
			if (ent->client->saber[1].soundOff &&
				ent->client->saber[1].model[0])
			{
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
			}
			//prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

extern qboolean WP_SaberCanTurnOffSomeBlades( saberInfo_t *saber );
void Cmd_SaberAttackCycle_f(gentity_t *ent)
{
	int selectLevel = 0;
	qboolean usingSiegeStyle = qfalse;
	
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( ent->client->ps.weapon != WP_SABER )
	{
        return;
	}
	/*
	if (ent->client->ps.weaponTime > 0)
	{ //no switching attack level when busy
		return;
	}
	*/

	if (ent->client->saber[0].model[0] && ent->client->saber[1].model[0])
	{ //no cycling for akimbo
		if ( WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) )
		{//can turn second saber off 
			if ( ent->client->ps.saberHolstered == 1 )
			{//have one holstered
				//unholster it
				G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOn);
				ent->client->ps.saberHolstered = 0;
				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if ( ent->client->ps.saberHolstered == 0 )
			{//have none holstered
				if ( (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
				{//can't turn it off manually
				}
				else if ( ent->client->saber[1].bladeStyle2Start > 0
					&& (ent->client->saber[1].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
				{//can't turn it off manually
				}
				else
				{
					//turn it off
					G_Sound(ent, CHAN_AUTO, ent->client->saber[1].soundOff);
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if (d_saberStanceDebug.integer)
			{
				trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\"") );
			}
			return;
		}
	}
	else if (ent->client->saber[0].numBlades > 1
		&& WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) )
	{ //use staff stance then.
		if ( ent->client->ps.saberHolstered == 1 )
		{//second blade off
			if ( ent->client->ps.saberInFlight )
			{//can't turn second blade back on if it's in the air, you naughty boy!
				if (d_saberStanceDebug.integer)
				{
					trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\"") );
				}
				return;
			}
			//turn it on
			G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOn);
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if ( ent->client->saber[0].stylesForbidden )
			{//have a style we have to use
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
				if ( ent->client->ps.weaponTime <= 0 )
				{ //not busy, set it now
					ent->client->ps.fd.saberAnimLevel = selectLevel;
				}
				else
				{ //can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = selectLevel;
				}
			}
		}
		else if ( ent->client->ps.saberHolstered == 0 )
		{//both blades on
			if ( (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE) )
			{//can't turn it off manually
			}
			else if ( ent->client->saber[0].bladeStyle2Start > 0
				&& (ent->client->saber[0].saberFlags2&SFL2_NO_MANUAL_DEACTIVATE2) )
			{//can't turn it off manually
			}
			else
			{
				//turn second one off
				G_Sound(ent, CHAN_AUTO, ent->client->saber[0].soundOff);
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if ( ent->client->saber[0].singleBladeStyle != SS_NONE )
				{
					if ( ent->client->ps.weaponTime <= 0 )
					{ //not busy, set it now
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					}
					else
					{ //can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
					}
				}
			}
		}
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\"") );
		}
		return;
	}

	if (ent->client->saberCycleQueue)
	{ //resume off of the queue if we haven't gotten a chance to update it yet
		selectLevel = ent->client->saberCycleQueue;
	}
	else
	{
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	if (level.gametype == GT_SIEGE &&
		ent->client->siegeClass != -1 &&
		bgSiegeClasses[ent->client->siegeClass].saberStance)
	{ //we have a flag of useable stances so cycle through it instead
		int i = selectLevel+1;

		usingSiegeStyle = qtrue;

		while (i != selectLevel)
		{ //cycle around upward til we hit the next style or end up back on this one
			if (i >= SS_NUM_SABER_STYLES)
			{ //loop back around to the first valid
				i = SS_FAST;
			}

			if (bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i))
			{ //we can use this one, select it and break out.
				selectLevel = i;
				break;
			}
			i++;
		}

		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\"") );
		}
	}
	else
	{
		selectLevel++;
		if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
		{
			selectLevel = FORCE_LEVEL_1;
		}
		if (d_saberStanceDebug.integer)
		{
			trap_SendServerCommand( ent-g_entities, va("print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\"") );
		}
	}
/*
#ifndef FINAL_BUILD
	switch ( selectLevel )
	{
	case FORCE_LEVEL_1:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sfast\n\"", S_COLOR_BLUE) );
		break;
	case FORCE_LEVEL_2:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %smedium\n\"", S_COLOR_YELLOW) );
		break;
	case FORCE_LEVEL_3:
		trap_SendServerCommand( ent-g_entities, va("print \"Lightsaber Combat Style: %sstrong\n\"", S_COLOR_RED) );
		break;
	}
#endif
*/
	if ( !usingSiegeStyle )
	{
		//make sure it's valid, change it if not
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
	}

	if (ent->client->ps.weaponTime <= 0)
	{ //not busy, set it now
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = selectLevel;
	}
	else
	{ //can't set it now or we might cause unexpected chaining, so queue it
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = selectLevel;
	}
}

qboolean G_OtherPlayersDueling(void)
{
	int i = 0;
	gentity_t *ent;

	while (i < MAX_CLIENTS)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->client && ent->client->ps.duelInProgress)
		{
			return qtrue;
		}
		i++;
	}

	return qfalse;
}

void Cmd_EngageDuel_f(gentity_t *ent)
{
	trace_t *tr;
//	vector3 forward, fwdOrg;
	int weapon = WP_SABER;

	if ( !(g_privateDuel.integer & PRIVDUEL_ALLOW) )
		return;

	// not allowed if you're not alive or not ingame
	if ( ent->health <= 0 ||
		 ent->client->tempSpectate >= level.time ||
		 ent->client->sess.sessionTeam == TEAM_SPECTATOR )
		return;

	if ( !(g_privateDuel.integer & PRIVDUEL_TEAM) && level.gametype >= GT_TEAM )
	{// no private dueling in team modes
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NODUEL_GAMETYPE")) );
		return;
	}

	if (ent->client->ps.duelTime >= level.time)
		return;

#if 0
	if ( !(g_privateDuel.integer & PRIVDUEL_WEAP) && ent->client->ps.weapon != WP_SABER )
		return;
#endif
	if ( (g_privateDuel.integer & PRIVDUEL_WEAP) ) {
		weapon = ent->client->ps.weapon;
	}

	if (ent->client->ps.saberInFlight)
		return;

	if (ent->client->ps.duelInProgress)
		return;

	//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	if ( !(g_privateDuel.integer & PRIVDUEL_MULTI) && ent->client->ps.fd.privateDuelTime > level.time) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "CANTDUEL_JUSTDID")) );
		return;
	}

	//Raz: Multi-duel
	if ( !(g_privateDuel.integer & PRIVDUEL_MULTI) && G_OtherPlayersDueling() ) {
		trap_SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "CANTDUEL_BUSY")) );
		return;
	}

	// unlagged!
	tr = RealTrace( ent, 256.0f );
	
	if ( tr->fraction < 1.0f && tr->entityNum < MAX_CLIENTS )
	{
		gentity_t *challenged = &g_entities[tr->entityNum];

		if ( !challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
		//	challenged->client->ps.weapon != WP_SABER ||
			challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight )
		{
			return;
		}

		if ( level.gametype >= GT_TEAM && OnSameTeam( ent, challenged ) )
			return;

		if ( challenged->client->ps.duelIndex == ent->s.number
			&& challenged->client->ps.duelTime >= level.time )
		{
			trap_SendServerCommand( -1, va( "print \"%s "S_COLOR_WHITE"%s %s "S_COLOR_WHITE"(%s)!\n\"", challenged->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLDUELACCEPT" ), ent->client->pers.netname, weaponData[challenged->client->pers.duel.weapon].longName ) );

			ent->client->ps.duelInProgress			= qtrue;
			challenged->client->ps.duelInProgress	= qtrue;

			// copy the start pos
			VectorCopy( &ent->client->ps.origin, &ent->client->pers.duel.startPos );
			VectorCopy( &challenged->client->ps.origin, &challenged->client->pers.duel.startPos );

			ent->client->ps.duelTime		= level.time + 2000;
			challenged->client->ps.duelTime	= level.time + 2000;

			G_AddEvent( ent, EV_PRIVATE_DUEL, 1 );
			G_AddEvent( challenged, EV_PRIVATE_DUEL, 1 );

			if ( challenged->client->pers.duel.weapon == WP_SABER ) {
				// Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)
				if ( !ent->client->ps.saberHolstered )
				{
					if ( ent->client->saber[0].soundOff )
						G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOff );
					if ( ent->client->saber[1].soundOff && ent->client->saber[1].model[0] )
						G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );

					ent->client->ps.weaponTime		= 400;
					ent->client->ps.saberHolstered	= 2;
				}
				if ( !challenged->client->ps.saberHolstered )
				{
					if ( challenged->client->saber[0].soundOff )
						G_Sound( challenged, CHAN_AUTO, challenged->client->saber[0].soundOff );
					if ( challenged->client->saber[1].soundOff && challenged->client->saber[1].model[0] )
						G_Sound( challenged, CHAN_AUTO, challenged->client->saber[1].soundOff );

					challenged->client->ps.weaponTime		= 400;
					challenged->client->ps.saberHolstered	= 2;
				}
			}

			else // reset their weapon times
				ent->client->ps.weaponTime = challenged->client->ps.weaponTime = 1000;

			ent->client->ps.weapon = challenged->client->ps.weapon = challenged->client->pers.duel.weapon;

			// set health etc
			ent->health = ent->client->ps.stats[STAT_HEALTH] = g_privateDuelHealth.integer;
			ent->client->ps.stats[STAT_ARMOR] = g_privateDuelShield.integer;
		}
		else
		{// Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			// set their desired dueling weapon.
			ent->client->pers.duel.weapon = weapon;

			trap_SendServerCommand( challenged-g_entities, va( "cp \"%s "S_COLOR_WHITE"%s\n"S_COLOR_YELLOW"Weapon: "S_COLOR_WHITE"%s\n\"", ent->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLDUELCHALLENGE" ), weaponData[weapon].longName ) );
			trap_SendServerCommand( ent-g_entities, va( "cp \"%s %s\n"S_COLOR_YELLOW"Weapon: "S_COLOR_WHITE"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PLDUELCHALLENGED" ), challenged->client->pers.netname, weaponData[weapon].longName ) );
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend		= HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime	= level.time + 1000;

		ent->client->ps.duelIndex	= challenged->s.number;
		ent->client->ps.duelTime	= level.time + 5000;
	}
}

#ifndef FINAL_BUILD
extern stringID_table_t animTable[MAX_ANIMATIONS+1];

void Cmd_DebugSetSaberMove_f(gentity_t *self)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	self->client->ps.saberMove = atoi(arg);
	self->client->ps.saberBlocked = BLOCKED_BOUNCE_MOVE;

	if (self->client->ps.saberMove >= LS_MOVE_MAX)
	{
		self->client->ps.saberMove = LS_MOVE_MAX-1;
	}

	Com_Printf("Anim for move: %s\n", animTable[saberMoveData[self->client->ps.saberMove].animToUse].name);
}

void Cmd_DebugSetBodyAnim_f(gentity_t *self, int flags)
{
	int argNum = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int i = 0;

	if (argNum < 2)
	{
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if (!arg[0])
	{
		return;
	}

	while (i < MAX_ANIMATIONS)
	{
		if (!Q_stricmp(arg, animTable[i].name))
		{
			break;
		}
		i++;
	}

	if (i == MAX_ANIMATIONS)
	{
		Com_Printf("Animation '%s' does not exist\n", arg);
		return;
	}

	G_SetAnim(self, NULL, SETANIM_BOTH, i, flags, 0);

	Com_Printf("Set body anim to %s\n", arg);
}
#endif

void StandardSetBodyAnim(gentity_t *self, int anim, int flags)
{
	G_SetAnim(self, NULL, SETANIM_BOTH, anim, flags, 0);
}

void DismembermentTest(gentity_t *self);

void Bot_SetForcedMovement(int bot, int forward, int right, int up);

#ifndef FINAL_BUILD
extern void DismembermentByNum(gentity_t *self, int num);
extern void G_SetVehDamageFlags( gentity_t *veh, int shipSurf, int damageLevel );
#endif

qboolean TryGrapple(gentity_t *ent)
{
	if (ent->client->ps.weaponTime > 0)
	{ //weapon busy
		return qfalse;
	}
	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{ //force power or knockdown or something
		return qfalse;
	}
	if (ent->client->grappleState)
	{ //already grappling? but weapontime should be > 0 then..
		return qfalse;
	}

	if (ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE)
	{
		return qfalse;
	}

	if (ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered)
	{
		Cmd_ToggleSaber_f(ent);
		if (!ent->client->ps.saberHolstered)
		{ //must have saber holstered
			return qfalse;
		}
	}

	//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_PA_1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
	if (ent->client->ps.torsoAnim == BOTH_KYLE_GRAB)
	{ //providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if (ent->client->ps.legsAnim == ent->client->ps.torsoAnim)
		{
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		return qtrue;
	}

	return qfalse;
}

void Cmd_TargetUse_f( gentity_t *ent )
{
	if ( trap_Argc() > 1 )
	{
		char sArg[MAX_STRING_CHARS] = {0};
		gentity_t *targ;

		trap_Argv( 1, sArg, sizeof( sArg ) );
		targ = G_Find( NULL, FOFS( targetname ), sArg );

		while ( targ )
		{
			if ( targ->use )
				targ->use( targ, ent, ent );
			targ = G_Find( targ, FOFS( targetname ), sArg );
		}
	}
}

void Cmd_TheDestroyer_f( gentity_t *ent ) {
	if ( !ent->client->ps.saberHolstered || ent->client->ps.weapon != WP_SABER )
		return;

	Cmd_ToggleSaber_f( ent );
}

void Cmd_BotMoveForward_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap_Argc() > 1 );
	trap_Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, arg, -1, -1 );
}

void Cmd_BotMoveBack_f( gentity_t *ent ) {
	int arg = -4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap_Argc() > 1 );
	trap_Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, arg, -1, -1 );
}

void Cmd_BotMoveRight_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap_Argc() > 1 );
	trap_Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, arg, -1 );
}

void Cmd_BotMoveLeft_f( gentity_t *ent ) {
	int arg = -4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap_Argc() > 1 );
	trap_Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, arg, -1 );
}

void Cmd_BotMoveUp_f( gentity_t *ent ) {
	int arg = 4000;
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap_Argc() > 1 );
	trap_Argv( 1, sarg, sizeof( sarg ) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, -1, arg );
}

void Cmd_AddBot_f( gentity_t *ent ) {
	//because addbot isn't a recognized command unless you're the server, but it is in the menus regardless
	trap_SendServerCommand( ent-g_entities, va( "print \"%s.\n\"", G_GetStringEdString( "MP_SVGAME", "ONLY_ADD_BOTS_AS_SERVER" ) ) );
}

void Cmd_Sabercolor_f( gentity_t *ent )
{
	int saberNum, red, green, blue, client=ent->client-level.clients;
	char sNum[8]={0}, sRed[8]={0}, sGreen[8]={0}, sBlue[8]={0};
	char userinfo[MAX_INFO_STRING];

	if ( trap_Argc() < 5 )
	{
		trap_SendServerCommand( ent-g_entities, va( "print \"^3Usage: /sabercolor ^7<^31-2^7> ^7<^10-255^7> <^20-255^7> <^50-255^7>\n\"" ) );
		return;
	}

	trap_Argv( 1, sNum, sizeof( sNum ) );
	trap_Argv( 2, sRed, sizeof( sRed ) );
	trap_Argv( 3, sGreen, sizeof( sGreen ) );
	trap_Argv( 4, sBlue, sizeof( sBlue ) );

	saberNum	= Com_Clampi( 1,	2,		atoi( sNum ) );
	red			= Com_Clampi( 0,	255,	atoi( sRed ) );
	green		= Com_Clampi( 0,	255,	atoi( sGreen ) );
	blue		= Com_Clampi( 0,	255,	atoi( sBlue ) );

//	trap_SendServerCommand( ent-g_entities, va( "print \"saberNum: %i\nred: %i\ngreen: %i\nblue: %i\nclient: %i\n\"", saberNum, red, green, blue, client ) );

	trap_GetUserinfo( client, userinfo, sizeof( userinfo ) );
	Info_SetValueForKey( userinfo, (saberNum==1)?"cp_sbRGB1":"cp_sbRGB2", va( "%i", red | ((green | (blue << 8)) << 8 ) ) );
	Info_SetValueForKey( userinfo, (saberNum==1)?"color1":"color2", va( "%i", SABER_RGB ) );
	trap_SetUserinfo( client, userinfo );
	ClientUserinfoChanged( client );

	return;
}

/*
	Channels description and implementation

/joinchan <identifier> <short-name>
	* Strip invalid (non-alphanumeric?) characters from identifier
	* Strip colour codes from identifier???
	* Create structure to contain identifier and short-name, add to linked list
	* Loop through all clients, check for match against identifier
	
/whoischan <identifier>
	* Strip invalid (non-alphanumeric?) characters from identifier
	* Strip colour codes from identifier???
	* Check for match against identifier
		* Loop through all clients, check for match against identifier
			* Print client names

/leavechan <identifier>
	* Strip invalid (non-alphanumeric?) characters from identifier
	* Strip colour codes from identifier???
	* Check for match against identifier
	* Loop through clients
	* Broadcast message

/msgchan <identifier> <message>
	* Strip invalid (non-alphanumeric?) characters from identifier
	* Strip colour codes from identifier???
	* Check for match against identifier
		* Loop through all clients, check for match against identifier
			* Send message to legacy clients as: "chat \"<^4#^7short-name>PlayerName^7"EC": ^7Message\""
			* Send message to modern clients as: "chat \""CHANNEL_EC"identifier"CHANNEL_EC"PlayerName^7"EC": ^7Message\""

Client info:
	* List of channel identifiers + short names (For legacy support)
	* Number of channels joined (For flood reasons, limit to jp_maxChannels - default 10)

*/

//Filters bad characters out of the channel identifier
static void JP_FilterIdentifier( char *ident )
{
	char *s=ident, *out=ident, c=0;

	while ( (c = *s++) != '\0' )
	{
		if ( c < '$' || c > '}' || c == ';' )
			 continue;
		*out++ = c;
	}
	*out = '\0';
}

static void JP_ListChannels( gentity_t *ent )
{
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);
	channel_t *channel = ent->client->pers.japp.channels;
	char msg[960] = { 0 };

	while ( channel )
	{
		Q_strcat( msg, sizeof( msg ), (legacyClient && channel == ent->client->pers.japp.activeChannel) ? va( "^7- ^3%s ^7[^2%s^7]\n", channel->identifier, channel->shortname ) : !!(legacyClient) ? va( "^7- %s ^7[^2%s^7]\n", channel->identifier, channel->shortname ) : va( "^7- %s\n", channel->identifier ) );
		channel = channel->next;
	}
	trap_SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}

void Cmd_JoinChannel_f( gentity_t *ent )
{
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);
	char arg1_ident[32] = { 0 };
	char arg2_shortname[32] = { 0 };
	channel_t *channel = NULL;
	channel_t *prev = NULL;

	if ( trap_Argc() < 2 )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^3Usage: /joinchan <identifier/password> <short-name (optional)>\n\"" );
		return;
	}

	trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
	if ( legacyClient )
		trap_Argv( 2, arg2_shortname, sizeof( arg2_shortname ) );

	JP_FilterIdentifier( arg1_ident );

	channel = ent->client->pers.japp.channels;
	while ( channel )
	{//Try to find an existing channel
		if ( !strcmp( arg1_ident, channel->identifier ) )
		{//Already joined, return
			trap_SendServerCommand( ent-g_entities, va( "print \"^3You are already in channel '%s'\n\"", arg1_ident ) );
			//TODO: update short-name?
			//		for legacy clients
			return;
		}
		prev = channel;
		channel = channel->next;
	}

	//Not in this channel (or any channels) so allocate a new one
	channel = (channel_t *)malloc( sizeof( channel_t ) );
	memset( channel, 0, sizeof( channel_t ) );

	//attach to linked list of channels
	if ( prev )
		prev->next = channel;
	else
		ent->client->pers.japp.channels = channel;

	Q_strncpyz( channel->identifier, arg1_ident, sizeof( channel->identifier ) );
	if ( legacyClient )
		Q_strncpyz( channel->shortname, arg2_shortname, sizeof( channel->shortname ) );

	//Notify about the successful join
	trap_SendServerCommand( ent-g_entities, legacyClient ? va( "print \"Successfully joined channel '%s' (shortname: '%s')\n\"", arg1_ident, arg2_shortname ) :
															va( "print \"Successfully joined channel '%s'\n\"", arg1_ident ) );

	return;
}

void Cmd_WhoisChannel_f( gentity_t *ent )
{
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);
	char msg[960] = { 0 };

	if ( trap_Argc() == 2 && !legacyClient )
	{//Supported client
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		channel_t *chan = NULL;
		qboolean in = qfalse;

		trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
		JP_FilterIdentifier( arg1_ident );

		//Check if they're even in the channel
		for ( chan = ent->client->pers.japp.channels; chan; chan = chan->next )
		{
			if ( !strcmp( arg1_ident, chan->identifier) )
			{
				in = qtrue;
				break;
			}
		}
		if ( !in )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"You are not in channel '%s'\n\"", arg1_ident, msg ) );
			return;
		}

		for ( other = g_entities; other-g_entities < MAX_CLIENTS; other++ )
		{
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED )
			{
				chan = other->client->pers.japp.channels;
				while ( chan )
				{
					if ( !strcmp( chan->identifier, arg1_ident ) )
					{
						Q_strcat( msg, sizeof( msg ), va( "^7- %s\n", other->client->pers.netname ) );
					}
					chan = chan->next;
				}
			}
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"Players in channel '%s':\n%s\"", arg1_ident, msg ) );
	}
	else if ( legacyClient )
	{
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;

		trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
		JP_FilterIdentifier( arg1_ident );

		if ( !ent->client->pers.japp.activeChannel )
		{
			trap_SendServerCommand( ent-g_entities, "print \"You are not in any channels\n\"" );
			return;
		}

		for ( other = g_entities; other-g_entities < MAX_CLIENTS; other++ )
		{
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED )
			{
				channel_t *chan = other->client->pers.japp.channels;
				while ( chan )
				{
					if ( chan == ent->client->pers.japp.activeChannel )
					{
						Q_strcat( msg, sizeof( msg ), va( "^7- %s\n", other->client->pers.netname ) );
					}
					chan = chan->next;
				}
			}
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"Players in channel '%s':\n%s\"", arg1_ident, msg ) );
	}
	else
		trap_SendServerCommand( ent-g_entities, "print \"^3Usage: /whoischan <identifier>\n\"" );

	return;
}

void Cmd_LeaveChannel_f( gentity_t *ent )
{
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);

	if ( trap_Argc() == 2 && !legacyClient )
	{
		channel_t *channel = ent->client->pers.japp.channels;
		channel_t *prev = NULL;
		char arg1_ident[32] = { 0 };

		trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
		JP_FilterIdentifier( arg1_ident );

		while ( channel )
		{
			if ( !strcmp( arg1_ident, channel->identifier ) )
			{
				if ( prev )
					prev->next = channel->next;
				else
					ent->client->pers.japp.channels = channel->next;
				free( channel );

				trap_SendServerCommand( ent-g_entities, va( "print \"Successfully left channel '%s'\n\"", arg1_ident ) );
				if ( ent->client->pers.japp.channels )
				{
					trap_SendServerCommand( ent-g_entities, "print \"You are currently in these channels:\n\"" );
					JP_ListChannels( ent );
				}
				return;
			}
			prev = channel;
			channel = channel->next;
		}
		trap_SendServerCommand( ent-g_entities, va( "print \"^3Error leaving channel '%s'. You were not in the channel.\n\"", arg1_ident ) );
		if ( ent->client->pers.japp.channels )
		{
			trap_SendServerCommand( ent-g_entities, "print \"You are currently in these channels:\n\"" );
			JP_ListChannels( ent );
		}
	}
	else if ( legacyClient )
	{
		//if activeChannel
			//if activeChannel->next
				//set activeChannel to activeChannel->next
			//else (find the last channel)
				//while channelList
					//if channel->next
						//channel = channel->next
					//else
						//free() activeChannel
						//set activeChannel to channel
				//not in any channels, todo..
			//free() activeChannel
	}

	return;
}
void Cmd_MessageChannel_f( gentity_t *ent )
{
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);
	char *msg = ConcatArgs( 2 );
	char name[MAX_STRING_CHARS] = {0};

	if ( trap_Argc() >= 3 && !legacyClient )
	{//Supported client
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		channel_t *chan = NULL;
		qboolean in = qfalse;

		trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
		JP_FilterIdentifier( arg1_ident );

		//Check if they're even in the channel
		for ( chan = ent->client->pers.japp.channels; chan; chan = chan->next )
		{
			if ( !strcmp( arg1_ident, chan->identifier) )
			{
				in = qtrue;
				break;
			}
		}
		if ( !in )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"You are not in channel '%s'\n\"", arg1_ident, msg ) );
			return;
		}

		Com_sprintf( name, sizeof( name ), "%s^7"CHANNEL_EC"%s"CHANNEL_EC": ", ent->client->pers.netname, chan->identifier );

		for ( other = g_entities; other-g_entities < MAX_CLIENTS; other++ )
		{
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED )
			{
				chan = other->client->pers.japp.channels;
				while ( chan )
				{
					if ( !strcmp( chan->identifier, arg1_ident ) )
					{
						G_SayTo( ent, other, SAY_ALL, COLOR_RED, name, msg, NULL );
					}
					chan = chan->next;
				}
			}
		}
	}
	else if ( legacyClient )
	{
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;

		trap_Argv( 1, arg1_ident, sizeof( arg1_ident ) );
		JP_FilterIdentifier( arg1_ident );

		if ( !ent->client->pers.japp.activeChannel )
		{
			trap_SendServerCommand( ent-g_entities, "print \"You are not in any channels\n\"" );
			return;
		}

		Com_sprintf( name, sizeof( name ), "%s^7"CHANNEL_EC"%s"CHANNEL_EC": ", ent->client->pers.netname, ent->client->pers.japp.activeChannel->identifier );

		for ( other = g_entities; other-g_entities < MAX_CLIENTS; other++ )
		{
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED )
			{
				channel_t *chan = other->client->pers.japp.channels;
				while ( chan )
				{
					if ( chan == ent->client->pers.japp.activeChannel )
					{
						G_SayTo( ent, other, SAY_ALL, COLOR_RED, name, msg, NULL );
					}
					chan = chan->next;
				}
			}
		}
	}
	else
		trap_SendServerCommand( ent-g_entities, "print \"^3Usage: /msgchan <identifier> <message>\n\"" );

	return;
}

void Cmd_Drop_f( gentity_t *ent ) {
	char arg[128] = {0};

	if ( ent->client->ps.pm_type == PM_DEAD || ent->client->ps.pm_type == PM_SPECTATOR )
	{
		trap_SendServerCommand( ent-g_entities, "print \"^3You must be alive to drop items\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );

	if ( !Q_stricmp( arg, "flag" ) )
	{
		powerup_t powerup = (ent->client->ps.persistant[PERS_TEAM]==TEAM_RED) ? PW_BLUEFLAG : PW_REDFLAG;

		if ( !japp_allowFlagDrop.integer )
		{
			trap_SendServerCommand( ent-g_entities, "print \"^3Not allowed to drop the flag\n\"" );
			return;
		}

		if ( level.gametype != GT_CTF || (ent->client->ps.powerups[powerup] <= level.time) )
		{
			trap_SendServerCommand( ent-g_entities, "print \"^3No flag to drop\n\"" );
			return;
		}

		if ( ent->client->ps.powerups[ powerup ] > level.time )
		{
			gitem_t *item = BG_FindItemForPowerup( powerup );
			gentity_t *drop = NULL;
			vector3 angs = { 0.0f, 0.0f, 0.0f };

			AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

			drop = Drop_Item( ent, item, angs.yaw );
			//Raz: speed caps
			drop->genericValue1 = trap_Milliseconds() - ent->client->pers.teamState.flagsince;
		//	drop->genericValue2 = ent-g_entities;
		//	drop->genericValue2 |= (1<<5); // 6th bit indicates a client dropped it on purpose
		//	drop->genericValue2 |= level.time << 6;

			ent->client->ps.powerups[powerup] = 0;
		}
	}
	else if ( !Q_stricmp( arg, "weapon" ) )
	{
		weapon_t wp = (weapon_t)ent->client->ps.weapon, newWeap = -1;
		gitem_t *item = NULL;
		gentity_t *drop = NULL;
		vector3 angs = { 0.0f, 0.0f, 0.0f };
		int ammo, i=0;

		if ( !japp_allowWeaponDrop.integer )
		{
			trap_SendServerCommand( ent-g_entities, "print \"^3Not allowed to drop weapons\n\"" );
			return;
		}

		if ( !(ent->client->ps.stats[STAT_WEAPONS] & (1<<wp)) || !ent->client->ps.ammo[weaponData[wp].ammoIndex]
		|| wp == WP_SABER || wp == WP_MELEE || wp == WP_EMPLACED_GUN || wp == WP_TURRET
			|| wp <= WP_NONE || wp > WP_NUM_WEAPONS )
		{// We don't have this weapon or ammo for it, or it's a 'weapon' that can't be dropped
			trap_SendServerCommand( ent-g_entities, "print \"^3Can't drop this weapon\n\"" );
			return;
		}

		item = BG_FindItemForWeapon( wp );

		if ( !ent->client->ps.ammo[weaponData[wp].ammoIndex] )
		{
			trap_SendServerCommand( ent-g_entities, "print \"^3No ammo for this weapon\n\"" );
			return;
		}
		else if ( ent->client->ps.ammo[weaponData[wp].ammoIndex] > item->quantity )//bg_itemlist[BG_GetItemIndexByTag( wp, IT_WEAPON )].quantity )
			ammo = item->quantity;
		else
			ammo = ent->client->ps.ammo[weaponData[wp].ammoIndex];

		AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

		drop = Drop_Item( ent, item, angs.yaw );
		drop->count = ammo;
		ent->client->ps.ammo[weaponData[wp].ammoIndex] -= ammo;
		ent->client->ps.stats[STAT_WEAPONS] &= ~(1<<wp);

#if 1
		for ( i=0; i<WP_NUM_WEAPONS; i++ )
		{
			if ((ent->client->ps.stats[STAT_WEAPONS] & (1 << i)) && i != WP_NONE)
			{ //this one's good
				newWeap = (weapon_t)i;
				break;
			}
		}

		if (newWeap != -1)
		{
			ent->s.weapon = newWeap;
			ent->client->ps.weapon = newWeap;
		}
		else
		{
			ent->s.weapon = 0;
			ent->client->ps.weapon = 0;
		}

		G_AddEvent( ent, EV_NOAMMO, wp );
#else
		for ( i=wp+1; i != wp-1; i++ )
		{
			if ( i == WP_NUM_WEAPONS )
				i = WP_NONE+1;
			if ( ent->client->ps.stats[STAT_WEAPONS] & (1<<i) )
			{
				ent->client->ps.weapon = i;
				break;
			}
		}
		G_AddEvent( ent, EV_NOAMMO, wp );
#endif

		drop->genericValue2 = ent-g_entities;
		drop->genericValue2 |= (1<<5); // 6th bit indicates a client dropped it on purpose
		drop->genericValue2 |= level.time << 6;
		}
	else if ( !Q_stricmp( arg, "powerup" ) )
	{
		//RAZTODO: /drop powerup
		return;
	}
}

#define EXTINFO_SABER	(0x0001)
#define EXTINFO_CMDS	(0x0002)
#define EXTINFO_CLIENT	(0x0004)
#define EXTINFO_ALL		(0x0007)

struct { char *str; int bit; } aminfoSettings[] = {
	{ "all", EXTINFO_ALL },
	{ "saber", EXTINFO_SABER },
	{ "cmds", EXTINFO_CMDS },
	{ "client", EXTINFO_CLIENT },
};
static const size_t numAminfoSettings = ARRAY_LEN( aminfoSettings );

static void Cmd_AMInfo_f( gentity_t *ent )
{
	char buf[MAX_STRING_CHARS-64] = {0};
	int extendedInfo = 0;

	trap_SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"================================================================\n\n\"" );

	if ( trap_Argc() < 2 ) {
		unsigned int i=0;
		Q_strcat( buf, sizeof( buf ), "Try 'aminfo <category>' for more detailed information\nCategories: " );

		// print all categories
		for ( i=0; i<numAminfoSettings; i++ ) {
			if ( i )
				Q_strcat( buf, sizeof( buf ), ", " );
			Q_strcat( buf, sizeof( buf ), aminfoSettings[i].str );
		}

		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\n\"", buf ) );
		buf[0] = '\0';
	}
	else {
		char arg[8] = {0};
		unsigned int i=0;

		trap_Argv( 1, arg, sizeof( arg ) );

		// find out which category they want
		for ( i=0; i<numAminfoSettings; i++ ) {
			if ( !Q_stricmp( arg, aminfoSettings[i].str ) ) {
				extendedInfo = aminfoSettings[i].bit;
				break;
			}
		}
	}

	// mod version + compile date
	if ( !extendedInfo || extendedInfo == EXTINFO_ALL ) {
		char version[256] = {0};
		trap_Cvar_VariableStringBuffer( "version", version, sizeof( version ) );
		trap_SendServerCommand( ent-g_entities, va( "print \"Version:\n    Gamecode: "JAPP_SERVER_VERSION" ("__DATE__")\n    Engine: %s\n\n\"", version ) );
	}

	if ( extendedInfo & EXTINFO_SABER )
	{// saber settings
		Q_strncpyz( buf, "Saber settings:\n", sizeof( buf ) );

		// SP/MP
		if ( d_saberSPStyleDamage.integer )		Q_strcat( buf, sizeof( buf ), "    SP style (default)" );
		else									Q_strcat( buf, sizeof( buf ), "    MP style" );

		// tweaks
			 if ( japp_saberSystem.integer == SABERSYSTEM_JAPP )	Q_strcat( buf, sizeof( buf ), " with JA++ tweaks\n" );
		else if ( japp_saberSystem.integer == SABERSYSTEM_JK2 )		Q_strcat( buf, sizeof( buf ), " with JK2 tweaks\n" );
		else														Q_strcat( buf, sizeof( buf ), " with no tweaks (default)\n" );

		// damage scale
		Q_strcat( buf, sizeof( buf ), va( "    %.05f damage scale", g_saberDamageScale.value ) );
		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		buf[0] = '\0';
	}

	if ( !extendedInfo || (extendedInfo & EXTINFO_CMDS) ) {
		// admin commands
		AM_PrintCommands( ent );

		// regular commands
		G_PrintCommands( ent );
	}

	if ( extendedInfo & EXTINFO_CLIENT )
	{// client support flags
		int i=0;

		Q_strcat( buf, sizeof( buf ), va( "Client support flags: 0x%X\n", ent->client->pers.CSF ) );
		for ( i=0; i<CSF_NUM; i++ ) {
			if ( ent->client->pers.CSF & (1<<i) )	Q_strcat( buf, sizeof( buf ), " [X] " );
			else									Q_strcat( buf, sizeof( buf ), " [ ] " );
			Q_strcat( buf, sizeof( buf ), va( "%s\n", supportFlagNames[i] ) );
		}

		trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		buf[0] = '\0';
	}

	trap_SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"================================================================\n\"" );
}

static void Cmd_Ready_f( gentity_t *ent ) {
	char *publicMsg = NULL;
	gentity_t *e = NULL;
	int i = 0;

	ent->client->pers.ready = !ent->client->pers.ready;

	if ( ent->client->pers.ready ) {
		publicMsg = va( "cp \"%s\n^7is ready\"", ent->client->pers.netname );
		trap_SendServerCommand( ent-g_entities, va( "cp \"^2You are ready\"" ) );
	}
	else {
		publicMsg = va( "cp \"%s\n^3is NOT ready\"", ent->client->pers.netname );
		trap_SendServerCommand( ent-g_entities, va( "cp \"^3You are NOT ready\"" ) );
	}

	// send public message to everyone BUT this client, so they see their own message
	for ( i=0, e=g_entities; i<level.maxclients; i++, e++ ) {
		if ( e != ent ) {
			trap_SendServerCommand( e-g_entities, publicMsg );
		}
	}
}

#define CMDFLAG_NOINTERMISSION	0x0001
#define CMDFLAG_CHEAT			0x0002
#define CMDFLAG_ALIVE			0x0004

#define CMDFAIL_NOINTERMISSION	0x0001
#define CMDFAIL_CHEAT			0x0002
#define CMDFAIL_ALIVE			0x0004
#define CMDFAIL_GAMETYPE		0x0008

typedef struct command_s {
	const char		*name;
	void			(*func)(gentity_t *ent);
	unsigned int	validGT;	// bit-flag of valid gametypes
	unsigned int	flags;
} command_t;

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((command_t*)b)->name );
}

command_t commands[] = {
	{ "addbot",				Cmd_AddBot_f,				GTB_ALL,					0 },
	{ "aminfo",				Cmd_AMInfo_f,				GTB_ALL,					0 },
	{ "callvote",			Cmd_CallVote_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "debugBMove_Back",	Cmd_BotMoveBack_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "debugBMove_Forward",	Cmd_BotMoveForward_f,		GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "debugBMove_Left",	Cmd_BotMoveLeft_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "debugBMove_Right",	Cmd_BotMoveRight_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "debugBMove_Up",		Cmd_BotMoveUp_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "drop",				Cmd_Drop_f,					GTB_ALL,					0 }, //Raz: added
	{ "duelteam",			Cmd_DuelTeam_f,				GTB_DUEL|GTB_POWERDUEL,		CMDFLAG_NOINTERMISSION },
	{ "follow",				Cmd_Follow_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "follownext",			Cmd_FollowNext_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "followprev",			Cmd_FollowPrev_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "forcechanged",		Cmd_ForceChanged_f,			GTB_ALL,					0 },
	{ "gc",					Cmd_GameCommand_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "give",				Cmd_Give_f,					GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "giveother",			Cmd_GiveOther_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "god",				Cmd_God_f,					GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "joinchan",			Cmd_JoinChannel_f,			GTB_ALL,					0 }, //Raz: added
	{ "kill",				Cmd_Kill_f,					GTB_ALL,					CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "killother",			Cmd_KillOther_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
//	{ "kylesmash",			TryGrapple,					GTB_ALL,					0 },
	{ "leavechan",			Cmd_LeaveChannel_f,			GTB_ALL,					0 }, //Raz: added
	{ "levelshot",			Cmd_LevelShot_f,			GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "msgchan",			Cmd_MessageChannel_f,		GTB_ALL,					0 }, //Raz: added
	{ "noclip",				Cmd_Noclip_f,				GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "notarget",			Cmd_Notarget_f,				GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE|CMDFLAG_NOINTERMISSION },
	{ "npc",				Cmd_NPC_f,					GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "ready",				Cmd_Ready_f	,				GTB_ALL,					CMDFLAG_NOINTERMISSION }, //Raz: added
	{ "sabercolor",			Cmd_Sabercolor_f,			GTB_ALL,					0 }, //Raz: added
	{ "say",				Cmd_Say_f,					GTB_ALL,					0 },
	{ "say_team",			Cmd_SayTeam_f,				GTB_ALL,					0 },
	{ "score",				Cmd_Score_f,				GTB_ALL,					0 },
	{ "setviewpos",			Cmd_SetViewpos_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_NOINTERMISSION },
	{ "siegeclass",			Cmd_SiegeClass_f,			GTB_SIEGE,					CMDFLAG_NOINTERMISSION },
	{ "team",				Cmd_Team_f,					GTB_ALL,					CMDFLAG_NOINTERMISSION },
//	{ "teamtask",			Cmd_TeamTask_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "tell",				Cmd_Tell_f,					GTB_ALL,					0 },
	{ "thedestroyer",		Cmd_TheDestroyer_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "t_use",				Cmd_TargetUse_f,			GTB_ALL,					CMDFLAG_CHEAT|CMDFLAG_ALIVE },
	{ "voice_cmd",			Cmd_VoiceCommand_f,			GTB_ALL & ~(GTB_NOTTEAM),	0 },
	{ "vote",				Cmd_Vote_f,					GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "where",				Cmd_Where_f,				GTB_ALL,					CMDFLAG_NOINTERMISSION },
	{ "whoischan",			Cmd_WhoisChannel_f,			GTB_ALL,					0 }, //Raz: added
	{ NULL,					NULL,						GTB_ALL,					0 },
};
static size_t numCommands = ARRAY_LEN( commands );

/*
=================
ClientCommand
=================
*/

//returns the flags that failed to pass
// or 0 if the command is allowed to be executed
unsigned int G_CmdValid( const gentity_t *ent, const command_t *cmd )
{
	if ( (cmd->flags & CMDFLAG_NOINTERMISSION)
		&& level.intermissiontime )
		return CMDFAIL_NOINTERMISSION;

	if ( (cmd->flags & CMDFLAG_CHEAT)
		&& !sv_cheats.integer )
		return CMDFAIL_CHEAT;

	if ( (cmd->flags & CMDFLAG_ALIVE)
		&& (ent->health <= 0
			|| ent->client->tempSpectate >= level.time
			|| ent->client->sess.sessionTeam == TEAM_SPECTATOR) )
		return CMDFAIL_ALIVE;

	if ( !(cmd->validGT & (1<<level.gametype)) )
		return CMDFAIL_GAMETYPE;

	return 0;
}

void ClientCommand( int clientNum ) {
	gentity_t	*ent = NULL;
	char		cmd[MAX_TOKEN_CHARS] = {0};
	command_t	*command = NULL;

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
		#ifndef OPENJK
			char tmpIP[NET_ADDRSTRMAXLEN] = {0};
			NET_AddrToString( tmpIP, sizeof( tmpIP ), &svs->clients[clientNum].netchan.remoteAddress );
		#else
			char *tmpIP = "Unknown";
		#endif // !OPENJK
		G_SecurityLogPrintf( "ClientCommand(%d) without an active connection [IP: %s]\n", clientNum, tmpIP );
		return;		// not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	//Raz: Bypass sv_floodProtect for admins o_o
	#ifndef OPENJK
		if ( ent->client->pers.adminUser && (ent->client->pers.adminUser->privs & PRIV_BYPASSFLOOD) )
			svs->clients[clientNum].nextReliableTime = svs->time;
	#endif // OPENJK

	//rww - redirect bot commands
	if ( strstr( cmd, "bot_" ) && AcceptBotCommand( cmd, ent ) )
		return;
	//end rww

	//[Admin]
	else if ( AM_HandleCommands( ent, cmd ) )
		return;
	//[/Admin]

	//Raz: JPLua
	if ( JPLua_Event_ClientCommand() )
		return;

	command = (command_t *)bsearch( cmd, commands, numCommands, sizeof( commands[0] ), cmdcmp );
	if ( !command ) {
		trap_SendServerCommand( clientNum, va( "print \"Unknown command %s\n\"", cmd ) );
		return;
	}

	switch ( G_CmdValid( ent, command ) )
	{
	case 0:
		command->func( ent );
		break;

	case CMDFAIL_NOINTERMISSION:
		trap_SendServerCommand( clientNum, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "CANNOT_TASK_INTERMISSION" ), cmd ) );
		break;
	case CMDFAIL_CHEAT:
		trap_SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOCHEATS" ) ) );
		break;
	case CMDFAIL_ALIVE:
		trap_SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "MUSTBEALIVE" ) ) );
		break;
	case CMDFAIL_GAMETYPE:
		trap_SendServerCommand( clientNum, va( "print \"%s is not applicable in this gametype\n\"", command->name ) );
		break;
	default:
		break;
	}
}

void G_PrintCommands( gentity_t *ent )
{
	const command_t *command = NULL;
	char buf[256] = {0};
	int toggle = 0;
	unsigned int count = 0;
	const unsigned int limit = 72;

	Q_strcat( buf, sizeof( buf ), "Regular commands:\n   " );

	for ( command=commands; command && command->name; command++ )
	{
		char *tmpMsg = NULL;

		// if it's not allowed to be executed at the moment, continue
		if ( G_CmdValid( ent, command ) )
			continue;

		tmpMsg = va( " ^%c%s", (++toggle&1?'2':'3'), command->name );

		//newline if we reach limit
		if ( count >= limit ) {
			tmpMsg = va( "\n   %s", tmpMsg );
			count = 0;
		}

		if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) )
		{
			trap_SendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
			buf[0] = '\0';
		}
		count += strlen( tmpMsg );
		Q_strcat( buf, sizeof( buf ), tmpMsg );
	}

	trap_SendServerCommand( ent-g_entities, va( "print \"%s\n\n\"", buf ) );
}
