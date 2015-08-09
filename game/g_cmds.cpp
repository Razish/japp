// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "bg_saga.h"
#include "g_admin.h"
#include "ui/menudef.h" // for the voice chats
#include "bg_lua.h"
#include "bg_public.h"
#include "JAPP/jp_csflags.h"

//rww - for getting bot commands...
int AcceptBotCommand( char *cmd, gentity_t *pl );
//end rww

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

void Cmd_NPC_f( gentity_t *ent );

void Cmd_Score_f( gentity_t *ent ) {
	ent->client->scoresWaiting = qtrue;
}

//RAZTODO: Move ConcatArgs to g_utils.c
char *ConcatArgs( int start ) {
	int		i, c, tlen, len;
	static char	line[MAX_STRING_CHARS];
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = trap->Argc();
	for ( i = start; i < c; i++ ) {
		trap->Argv( i, arg, sizeof(arg) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 )
			break;
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

// Give items to a client
static void G_Give( gentity_t *ent, const char *name, const char *args, int argc ) {
	const gitem_t	*it;
	int				i;
	qboolean		give_all = qfalse;
	gentity_t		*it_ent;
	trace_t			trace;

	if ( !Q_stricmp( name, "all" ) )
		give_all = qtrue;

	if ( give_all ) {
		for ( i = 0; i < HI_NUM_HOLDABLE; i++ )
			ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
	}

	if ( give_all || !Q_stricmp( name, "health" ) ) {
		if ( argc == 3 )
			ent->health = Q_clampi( 1, atoi( args ), ent->client->ps.stats[STAT_MAX_HEALTH] );
		else {
			if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
				ent->health = bgSiegeClasses[ent->client->siegeClass].maxhealth;
			else
				ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "armor" ) || !Q_stricmp( name, "shield" ) ) {
		if ( argc == 3 )
			ent->client->ps.stats[STAT_ARMOR] = Q_clampi( 0, atoi( args ), ent->client->ps.stats[STAT_MAX_HEALTH] );
		else if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 )
			ent->client->ps.stats[STAT_ARMOR] = bgSiegeClasses[ent->client->siegeClass].maxarmor;
		else
			ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "force" ) ) {
		if ( argc == 3 )
			ent->client->ps.fd.forcePower = Q_clampi( 0, atoi( args ), ent->client->ps.fd.forcePowerMax );
		else
			ent->client->ps.fd.forcePower = ent->client->ps.fd.forcePowerMax;

		if ( !give_all )
			return;
	}

	if ( give_all || !Q_stricmp( name, "weapons" ) ) {
		ent->client->ps.stats[STAT_WEAPONS] = (1 << (LAST_USEABLE_WEAPON + 1)) - (1 << WP_NONE);
		if ( !give_all )
			return;
	}

	if ( !give_all && !Q_stricmp( name, "weaponnum" ) ) {
		ent->client->ps.stats[STAT_WEAPONS] |= (1 << atoi( args ));
		return;
	}

	if ( give_all || !Q_stricmp( name, "ammo" ) ) {
		int num = 999;
		if ( argc == 3 )
			num = Q_clampi( 0, atoi( args ), 999 );
			num = atoi( args );
		for ( i = AMMO_BLASTER; i < AMMO_MAX; i++ )
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
		it_ent->classname = (char *)it->classname;
		G_SpawnItem( it_ent, it );
		if ( !it_ent || !it_ent->inuse )
			return;
		FinishSpawningItem( it_ent );
		if ( !it_ent || !it_ent->inuse )
			return;
		memset( &trace, 0, sizeof(trace) );
		Touch_Item( it_ent, ent, &trace );
		if ( it_ent->inuse )
			G_FreeEntity( it_ent );
	}
}

static void Cmd_Give_f( gentity_t *ent ) {
	char name[MAX_TOKEN_CHARS] = { 0 };

	trap->Argv( 1, name, sizeof(name) );
	G_Give( ent, name, ConcatArgs( 2 ), trap->Argc() );
}

static void Cmd_GiveOther_f( gentity_t *ent ) {
	char		name[MAX_TOKEN_CHARS], otherindex[MAX_TOKEN_CHARS];
	int			i;
	gentity_t	*otherEnt = NULL;

	trap->Argv( 1, otherindex, sizeof(otherindex) );
	if ( !otherindex[0] ) {
		trap->SendServerCommand( ent - g_entities, "print \"giveother requires that the second argument be a client index number.\n\"" );
		return;
	}

	i = atoi( otherindex );
	if ( i < 0 || i >= MAX_CLIENTS ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%i is not a client index.\n\"", i ) );
		return;
	}

	otherEnt = &g_entities[i];
	if ( !otherEnt->inuse || !otherEnt->client ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%i is not an active client.\n\"", i ) );
		return;
	}

	trap->Argv( 2, name, sizeof(name) );

	G_Give( otherEnt, name, ConcatArgs( 3 ), trap->Argc() - 1 );
}

// Sets client to godmode
static void Cmd_God_f( gentity_t *ent ) {
	ent->flags ^= FL_GODMODE;
	trap->SendServerCommand( ent - g_entities, va( "print \"godmode %s\n\"",
		(ent->flags & FL_GODMODE) ? "ON" : "OFF" ) );
}

static void Cmd_Ignore_f( gentity_t *ent ) {
	char arg[MAX_NETNAME];
	int clientNum;

	if ( trap->Argc() != 2 ) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Usage: \\ignore <client>\n\"" );
		return;
	}

	trap->Argv( 1, arg, sizeof(arg) );

	if ( atoi( arg ) == -1 ) {
		if ( ent->client->pers.ignore == (0xFFFFFFFFu & ~(1 << (ent - g_entities) )) ) {
			ent->client->pers.ignore = 0u;
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Unignoring " S_COLOR_WHITE
				"everyone\n\"" );
		}
		else {
			ent->client->pers.ignore = 0xFFFFFFFFu & ~(1 << (ent - g_entities));
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Ignoring " S_COLOR_WHITE
				"everyone\n\"" );
		}
	}
	else {
		clientNum = G_ClientFromString( ent, arg, FINDCL_SUBSTR | FINDCL_PRINT );
		if ( clientNum == -1 ) {
			return;
		}

		ent->client->pers.ignore ^= (1 << clientNum);
		trap->SendServerCommand( ent - g_entities, va( "print \"%s %s\n\"",
			(ent->client->pers.ignore & (1 << clientNum)) ? S_COLOR_YELLOW "Ignoring" : S_COLOR_GREEN "Unignoring",
			g_entities[clientNum].client->pers.netname ) );
	}
}

// Sets client to notarget
static void Cmd_Notarget_f( gentity_t *ent ) {
	ent->flags ^= FL_NOTARGET;
	trap->SendServerCommand( ent - g_entities, va( "print \"notarget %s\n\"", (ent->flags & FL_NOTARGET) ? "ON" : "OFF" ) );
}

static void Cmd_Noclip_f( gentity_t *ent ) {
	ent->client->noclip = !ent->client->noclip;
	trap->SendServerCommand( ent - g_entities, va( "print \"noclip %s\n\"", ent->client->noclip ? "ON" : "OFF" ) );
}

// This is just to help generate the level pictures for the menus. It goes to the intermission immediately and sends over
//	a command to the client to resize the view, hide the scoreboard, and take a special screenshot
static void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !ent->client->pers.localClient ) {
		trap->SendServerCommand( ent - g_entities, "print \"The levelshot command must be executed by a local client\n\"" );
		return;
	}

	// doesn't work in single player
	if ( level.gametype == GT_SINGLE_PLAYER ) {
		trap->SendServerCommand( ent - g_entities, "print \"Must not be in singleplayer mode for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap->SendServerCommand( ent - g_entities, "clientLevelShot" );
}

static gentity_t *G_AttackerCheck( gentity_t *ent ) {
	qboolean otherKiller = qfalse;
	gentity_t *attacker = NULL;

	if ( ent->client->ps.otherKiller >= 0 && ent->client->ps.otherKiller < MAX_CLIENTS ) {
		attacker = &g_entities[ent->client->ps.otherKiller];
	}

	if ( attacker && attacker->client && ent->client->ps.otherKillerTime > level.time
		&& attacker->client->sess.sessionTeam != TEAM_SPECTATOR && attacker->client->tempSpectate < level.time
		&& (attacker->client->sess.sessionTeam != ent->client->sess.sessionTeam
		|| (g_gametype.integer < GT_TEAM && attacker->client->sess.sessionTeam == TEAM_FREE)) && attacker->health > 0 )
	{
		otherKiller = qtrue;
	}

	return otherKiller ? attacker : NULL;
}

// commit suicide
void Cmd_Kill_f( gentity_t *ent ) {
	gentity_t *attacker = NULL;

	if ( ent->client->ps.fallingToDeath && !japp_allowFallSuicide.integer ) {
		return;
	}

	if ( ent->client->pers.adminData.isSlept ) {
		return;
	}

	//OSP: pause
	if ( level.pause.state != PAUSE_NONE ) {
		return;
	}

	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && level.numPlayingClients > 1 && !level.warmupTime
		&& !g_allowDuelSuicide.integer ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "ATTEMPTDUELKILL" ) ) );
		return;
	}

	G_LeaveVehicle( ent, qfalse );
	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	attacker = G_AttackerCheck( ent );
	if ( attacker ) {
		player_die( ent, attacker, attacker, 100000, MOD_SUICIDE );
	}
	else {
		player_die( ent, ent, ent, 100000, MOD_SUICIDE );
	}
}

static int G_ClientNumFromNetname( char *name ) {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i<MAX_CLIENTS; i++, ent++ ) {
		if ( ent->inuse && ent->client && !Q_stricmp( ent->client->pers.netname, name ) )
			return i;
	}

	return -1;
}

static void Cmd_KillOther_f( gentity_t *ent ) {
	if ( trap->Argc() > 1 ) {
		char sArg[MAX_STRING_CHARS] = { 0 };
		int entNum = 0;

		trap->Argv( 1, sArg, sizeof(sArg) );

		entNum = G_ClientNumFromNetname( sArg );

		if ( entNum >= 0 && entNum < MAX_CLIENTS ) {
			gentity_t *kEnt = &g_entities[entNum];

			if ( kEnt->inuse && kEnt->client ) {
				kEnt->flags &= ~FL_GODMODE;
				kEnt->client->ps.stats[STAT_HEALTH] = kEnt->health = -999;
				player_die( kEnt, kEnt, kEnt, 100000, MOD_SUICIDE );
			}
		}
	}
}

// Let everyone know about a team change
void BroadcastTeamChange( gclient_t *client, int oldTeam ) {
	client->ps.fd.forceDoInit = 1; //every time we change teams make sure our force powers are set right

	if ( level.gametype == GT_SIEGE )
		return;

	if ( client->sess.sessionTeam == TEAM_RED ) {
		G_Announce( va( "%s" S_COLOR_WHITE " %s", client->pers.netname,
			G_GetStringEdString( "MP_SVGAME", "JOINEDTHEREDTEAM" ) ) );
	}
	else if ( client->sess.sessionTeam == TEAM_BLUE ) {
		G_Announce( va( "%s" S_COLOR_WHITE " %s", client->pers.netname,
			G_GetStringEdString( "MP_SVGAME", "JOINEDTHEBLUETEAM" ) ) );
	}
	else if ( client->sess.sessionTeam == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		G_Announce( va( "%s" S_COLOR_WHITE " %s", client->pers.netname,
			G_GetStringEdString( "MP_SVGAME", "JOINEDTHESPECTATORS" ) ) );
	}
	else if ( client->sess.sessionTeam == TEAM_FREE ) {
		if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL ) {
			G_Announce( va( "%s" S_COLOR_WHITE " %s", client->pers.netname,
				G_GetStringEdString( "MP_SVGAME", "JOINEDTHEBATTLE" ) ) );
		}
	}

	G_LogPrintf( level.log.console, "setteam:  %i %s %s\n", client - level.clients, TeamName( oldTeam ), TeamName( client->sess.sessionTeam ) );
}

static qboolean G_PowerDuelCheckFail( gentity_t *ent ) {
	int loners = 0, doubles = 0;

	if ( !ent->client || ent->client->sess.duelTeam == DUELTEAM_FREE )
		return qtrue;

	G_PowerDuelCount( &loners, &doubles, qfalse );

	if ( ent->client->sess.duelTeam == DUELTEAM_LONE && loners >= 1 )
		return qtrue;

	if ( ent->client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2 )
		return qtrue;

	return qfalse;
}

qboolean g_dontPenalizeTeam = qfalse;
qboolean g_preventTeamBegin = qfalse;
qboolean SetTeam( gentity_t *ent, const char *s, qboolean forced ) {
	int					team, oldTeam, clientNum, specClient;
	gclient_t			*client;
	spectatorState_t	specState;

	// this prevents rare creation of invalid players
	if ( !ent->inuse ) {
		return qfalse;
	}

	// see what change is requested
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;
	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	}
	else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	}
	else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	}
	else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	}
	else if ( level.gametype >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) )
			team = TEAM_RED;
		else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) )
			team = TEAM_BLUE;
		else
			team = PickTeam( clientNum );

		if ( g_teamForceBalance.integer && !g_jediVmerc.integer ) {
			int counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent - g_entities, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent - g_entities, TEAM_RED );

			// We allow a spread of two
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] > 1 ) {
				trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "TOOMANYRED" ) ) );
				return qfalse; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] > 1 ) {
				trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "TOOMANYBLUE" ) ) );
				return qfalse; // ignore the request
			}
			// It's ok, the team we are switching to has less or same number of players
		}
	}
	// force them to spectators if there aren't any spots free
	else {
		team = TEAM_FREE;
	}

	oldTeam = client->sess.sessionTeam;

	if ( level.gametype == GT_SIEGE ) {
		// sorry, can't do that.
		if ( client->tempSpectate >= level.time && team == TEAM_SPECTATOR )
			return qfalse;

		if ( team == oldTeam && team != TEAM_SPECTATOR )
			return qfalse;

		client->sess.siegeDesiredTeam = team;
		if ( client->sess.sessionTeam != TEAM_SPECTATOR && team != TEAM_SPECTATOR ) {
			// not a spectator now, and not switching to spec, so you have to wait til you die.
			//	trap->SendServerCommand( ent-g_entities, va( "print \"You will be on the selected team the next time you respawn.\n\"" ) );
			if ( ent->client->tempSpectate < level.time ) {
				// Kill them so they automatically respawn in the team they wanted.
				if ( ent->health > 0 ) {
					ent->flags &= ~FL_GODMODE;
					ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
					player_die( ent, ent, ent, 100000, MOD_TEAM_CHANGE );
				}
			}

			if ( ent->client->sess.sessionTeam != ent->client->sess.siegeDesiredTeam )
				SetTeamQuick( ent, ent->client->sess.siegeDesiredTeam, qfalse );

			return qtrue;
		}
	}

	// override decision if limiting the players
	if ( level.gametype == GT_DUEL && level.numNonSpectatorClients >= 2 )
		team = TEAM_SPECTATOR;
	else if ( level.gametype == GT_POWERDUEL && (level.numPlayingClients >= 3 || G_PowerDuelCheckFail( ent )) )
		team = TEAM_SPECTATOR;
	else if ( g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer )
		team = TEAM_SPECTATOR;

	// decide if we will allow the change
	if ( team == oldTeam && team != TEAM_SPECTATOR )
		return qfalse;

	// check if the team is locked
	if ( !forced && team != oldTeam && level.lockedTeams[team] ) {
		trap->SendServerCommand( clientNum, "print \"" S_COLOR_YELLOW "This team is locked\n\"" );
		return qfalse;
	}

	// execute the team change

	// if the player was dead leave the body
	if ( client->ps.stats[STAT_HEALTH] <= 0 && client->sess.sessionTeam != TEAM_SPECTATOR )
		MaintainBodyQueue( ent );

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		g_dontPenalizeTeam = qtrue;
		player_die( ent, ent, ent, 100000, MOD_SUICIDE );
		g_dontPenalizeTeam = qfalse;
	}

	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		// so you don't get dropped to the bottom of the queue for changing skins, etc.
		if ( level.gametype != GT_DUEL || oldTeam != TEAM_SPECTATOR )
			client->sess.spectatorTime = level.time;
		G_ClearVote( ent );
	}

	client->sess.sessionTeam = (team_t)team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	BroadcastTeamChange( client, oldTeam );

	// make a disappearing effect where they were before teleporting them to the appropriate spawn point, if we were not
	//	on the spec team
	if ( oldTeam != TEAM_SPECTATOR ) {
		gentity_t *tent = G_TempEntity( &client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = clientNum;
	}

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( clientNum ) )
		return qfalse;

	if ( !g_preventTeamBegin )
		ClientBegin( clientNum, qfalse );

	return qtrue;
}

// If the client being followed leaves the game, or you just want to drop to free floating spectator mode
void StopFollowing( gentity_t *ent ) {
	int i = 0;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_SPECTATOR;
	ent->client->sess.sessionTeam = TEAM_SPECTATOR;
	ent->client->sess.spectatorState = SPECTATOR_FREE;
	ent->client->ps.pm_flags &= ~PMF_FOLLOW;
	ent->r.svFlags &= ~SVF_BOT;
	ent->client->ps.clientNum = ent - g_entities;
	ent->client->ps.weapon = WP_NONE;
	//Raz: Bug fix with vehicles, from OJP code
	G_LeaveVehicle( ent, qfalse ); // clears m_iVehicleNum as well
	ent->client->ps.emplacedIndex = 0;
	//ent->client->ps.m_iVehicleNum = 0;
	ent->client->ps.viewangles.roll = 0.0f;
	ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
	ent->client->ps.forceHandExtendTime = 0;
	ent->client->ps.zoomMode = 0;
	ent->client->ps.zoomLocked = qfalse;
	ent->client->ps.zoomLockTime = 0;
	ent->client->ps.saberMove = LS_NONE;
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
	ent->client->ps.pm_type = PM_SPECTATOR;
	ent->client->ps.eFlags &= ~EF_DISINTEGRATION;
	for ( i = 0; i < PW_NUM_POWERUPS; i++ )
		ent->client->ps.powerups[i] = 0;
}

static void Cmd_Team_f( gentity_t *ent ) {
	int			oldTeam;
	char		s[MAX_TOKEN_CHARS];

	oldTeam = ent->client->sess.sessionTeam;

	if ( trap->Argc() != 2 ) {
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PRINTBLUETEAM" ) ) );
			break;
		case TEAM_RED:
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PRINTREDTEAM" ) ) );
			break;
		case TEAM_FREE:
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PRINTFREETEAM" ) ) );
			break;
		case TEAM_SPECTATOR:
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PRINTSPECTEAM" ) ) );
			break;
		default:
			assert( !"Unknown old team in Cmd_Team_f" );
			break;
		}
		return;
	}

	if ( ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSWITCH" ) ) );
		return;
	}

	if ( gEscaping ) {
		return;
	}

	if ( ent->client->pers.adminData.isSlept ) {
		return;
	}


	// if they are playing a tournement game, count as a loss
	if ( level.gametype == GT_DUEL && ent->client->sess.sessionTeam == TEAM_FREE ) {
		// disallow changing teams
		trap->SendServerCommand( ent - g_entities, "print \"Cannot switch teams in Duel\n\"" );
		return;
	}

	if ( level.gametype == GT_POWERDUEL ) {
		// don't let clients change teams manually at all in powerduel, it will be taken care of through automated stuff
		trap->SendServerCommand( ent - g_entities, "print \"Cannot switch teams in Power Duel\n\"" );
		return;
	}

	trap->Argv( 1, s, sizeof(s) );

	SetTeam( ent, s, qfalse );

	// update team switch time only if team change really happened
	if ( oldTeam != ent->client->sess.sessionTeam ) {
		ent->client->switchTeamTime = level.time + 5000;
	}
}

static void Cmd_DuelTeam_f( gentity_t *ent ) {
	int oldTeam;
	char s[MAX_TOKEN_CHARS];

	// don't bother doing anything if this is not power duel
	if ( level.gametype != GT_POWERDUEL )
		return;

	if ( trap->Argc() != 2 ) {
		// No arg so tell what team we're currently on.
		oldTeam = ent->client->sess.duelTeam;
		switch ( oldTeam ) {
		case DUELTEAM_FREE:
			trap->SendServerCommand( ent - g_entities, va( "print \"None\n\"" ) );
			break;
		case DUELTEAM_LONE:
			trap->SendServerCommand( ent - g_entities, va( "print \"Single\n\"" ) );
			break;
		case DUELTEAM_DOUBLE:
			trap->SendServerCommand( ent - g_entities, va( "print \"Double\n\"" ) );
			break;
		default:
			break;
		}
		return;
	}

	if ( ent->client->switchDuelTeamTime > level.time ) {
		// debounce for changing
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSWITCH" ) ) );
		return;
	}

	trap->Argv( 1, s, sizeof(s) );

	oldTeam = ent->client->sess.duelTeam;

	if ( !Q_stricmp( s, "free" ) )		ent->client->sess.duelTeam = DUELTEAM_FREE;
	else if ( !Q_stricmp( s, "single" ) )	ent->client->sess.duelTeam = DUELTEAM_LONE;
	else if ( !Q_stricmp( s, "double" ) )	ent->client->sess.duelTeam = DUELTEAM_DOUBLE;
	else trap->SendServerCommand( ent - g_entities, va( "print \"'%s' not a valid duel team.\n\"", s ) );

	// didn't actually change, so don't care.
	if ( oldTeam == ent->client->sess.duelTeam )
		return;

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		// ok..die
		int curTeam = ent->client->sess.duelTeam;
		ent->client->sess.duelTeam = oldTeam;
		G_Damage( ent, ent, ent, NULL, &ent->client->ps.origin, 99999, DAMAGE_NO_PROTECTION, MOD_SUICIDE );
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

static int G_TeamForSiegeClass( const char *clName ) {
	int i = 0, team = SIEGETEAM_TEAM1;
	siegeTeam_t *stm = BG_SiegeFindThemeForTeam( team );
	siegeClass_t *scl;

	if ( !stm )
		return 0;

	while ( team <= SIEGETEAM_TEAM2 ) {
		scl = stm->classes[i];

		if ( scl && scl->name[0] ) {
			if ( !Q_stricmp( clName, scl->name ) )
				return team;
		}

		i++;
		if ( i >= MAX_SIEGE_CLASSES || i >= stm->numClasses ) {
			if ( team == SIEGETEAM_TEAM2 )
				break;
			team = SIEGETEAM_TEAM2;
			stm = BG_SiegeFindThemeForTeam( team );
			i = 0;
		}
	}

	return 0;
}

static void Cmd_SiegeClass_f( gentity_t *ent ) {
	char className[64];
	int team = 0, preScore;
	qboolean startedAsSpec = qfalse;

	// classes are only valid for this gametype
	if ( level.gametype != GT_SIEGE )
		return;

	if ( !ent->client )
		return;

	if ( trap->Argc() < 1 )
		return;

	if ( ent->client->switchClassTime > level.time ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOCLASSSWITCH" ) ) );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR )
		startedAsSpec = qtrue;

	trap->Argv( 1, className, sizeof(className) );

	team = G_TeamForSiegeClass( className );

	// not a valid class name
	if ( !team )
		return;

	if ( ent->client->sess.sessionTeam != team ) {
		// try changing it then
		g_preventTeamBegin = qtrue;
		if ( team == TEAM_RED )
			SetTeam( ent, "red", qfalse );
		else if ( team == TEAM_BLUE )
			SetTeam( ent, "blue", qfalse );
		g_preventTeamBegin = qfalse;

		if ( ent->client->sess.sessionTeam != team ) {
			// failed, oh well
			if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR || ent->client->sess.siegeDesiredTeam != team ) {
				trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOCLASSTEAM" ) ) );
				return;
			}
		}
	}

	//preserve 'is score
	preScore = ent->client->ps.persistant[PERS_SCORE];

	//Make sure the class is valid for the team
	BG_SiegeCheckClassLegality( team, className );

	//Set the session data
	strcpy( ent->client->sess.siegeClass, className );

	// get and distribute relevent paramters
	if ( !ClientUserinfoChanged( ent->s.number ) )
		return;

	if ( ent->client->tempSpectate < level.time ) {
		// Kill him (makes sure he loses flags, etc)
		if ( ent->health > 0 && !startedAsSpec ) {
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die( ent, ent, ent, 100000, MOD_SUICIDE );
		}

		// respawn them instantly.
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || startedAsSpec )
			ClientBegin( ent->s.number, qfalse );
	}
	//set it back after we do all the stuff
	ent->client->ps.persistant[PERS_SCORE] = preScore;

	ent->client->switchClassTime = level.time + 5000;
}

static void Cmd_ForceChanged_f( gentity_t *ent ) {
	// if it's a spec, just make the changes now
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || japp_instantForceSwitch.integer) {
		WP_InitForcePowers( ent );
	}
	else {
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_GREEN "%s\n\"",
			G_GetStringEdString( "MP_SVGAME", "FORCEPOWERCHANGED" ) ) );
		ent->client->ps.fd.forceDoInit = 1;
	}

	// if this is duel, don't even bother changing team in relation to this.
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		return;
	}

	if ( trap->Argc() > 1 ) {
		char arg[MAX_TOKEN_CHARS];

		trap->Argv( 1, arg, sizeof(arg) );

		// if there's an arg, assume it's a combo team command from the UI.
		if ( arg[0] ) {
			Cmd_Team_f( ent );
		}
	}
}

qboolean WP_SaberStyleValidForSaber( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int saberAnimLevel );
qboolean WP_UseFirstValidSaberStyle( saberInfo_t *saber1, saberInfo_t *saber2, int saberHolstered, int *saberAnimLevel );
qboolean G_SetSaber( gentity_t *ent, int saberNum, const char *saberName, qboolean siegeOverride ) {
	char truncSaberName[MAX_QPATH] = { 0 };

	if ( !siegeOverride && level.gametype == GT_SIEGE && ent->client->siegeClass != -1 &&
		(bgSiegeClasses[ent->client->siegeClass].saberStance || bgSiegeClasses[ent->client->siegeClass].saber1[0] || bgSiegeClasses[ent->client->siegeClass].saber2[0]) ) { //don't let it be changed if the siege class has forced any saber-related things
		return qfalse;
	}

	Q_strncpyz( truncSaberName, saberName, sizeof(truncSaberName) );

	// can't remove saber 0 like this
	if ( saberNum == 0 && (!Q_stricmp( "none", truncSaberName ) || !Q_stricmp( "remove", truncSaberName )) )
		Q_strncpyz( truncSaberName, DEFAULT_SABER, sizeof(truncSaberName) );

	// Set the saber with the arg given. If the arg is not a valid sabername defaults will be used.
	WP_SetSaber( ent->s.number, ent->client->saber, saberNum, truncSaberName );

	if ( !ent->client->saber[0].model[0] ) {
		assert( !"G_SetSaber: Missing first saber" ); //should never happen!
		Q_strncpyz( ent->client->pers.saber1, DEFAULT_SABER, sizeof(ent->client->pers.saber1) );
	}
	else
		Q_strncpyz( ent->client->pers.saber1, ent->client->saber[0].name, sizeof(ent->client->pers.saber1) );

	if ( !ent->client->saber[1].model[0] )
		Q_strncpyz( ent->client->pers.saber2, "none", sizeof(ent->client->pers.saber2) );
	else
		Q_strncpyz( ent->client->pers.saber2, ent->client->saber[1].name, sizeof(ent->client->pers.saber2) );

	if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered,
		ent->client->ps.fd.saberAnimLevel ) ) {
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered,
			&ent->client->ps.fd.saberAnimLevel );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
	}

	return qtrue;
}

static void Cmd_Follow_f( gentity_t *ent ) {
	int		i;
	char	arg[MAX_TOKEN_CHARS];

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	if ( trap->Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
			StopFollowing( ent );
		return;
	}

	trap->Argv( 1, arg, sizeof(arg) );
	i = G_ClientFromString( ent, arg, FINDCL_SUBSTR | FINDCL_PRINT );
	if ( i == -1 )
		return;

	// can't follow self
	if ( &level.clients[i] == ent->client )
		return;

	// can't follow another spectator
	if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR )
		return;

	// can't follow another spectator
	if ( level.clients[i].tempSpectate >= level.time )
		return;

	// if they are playing a tournement game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && ent->client->sess.sessionTeam == TEAM_FREE )
		ent->client->sess.losses++; //WTF???

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		if ( !SetTeam( ent, "spectator", qfalse ) )
			return;
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum, original;
	qboolean looped = qfalse;

	if ( ent->client->sess.spectatorState == SPECTATOR_NOT && ent->client->switchTeamTime > level.time ) {
		trap->SendServerCommand( ent-g_entities, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "NOSWITCH")) );
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && ent->client->sess.sessionTeam == TEAM_FREE )
		ent->client->sess.losses++; //WTF???

	// first set them to spectator
	if ( ent->client->sess.spectatorState == SPECTATOR_NOT ) {
		if ( !SetTeam( ent, "spectator", qfalse ) )
			return;
		// fix: update team switch time only if team change really happend
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR)
			ent->client->switchTeamTime = level.time + 5000;
	}

	if ( dir != 1 && dir != -1 )
		trap->Error( ERR_DROP, "Cmd_FollowCycle_f: bad dir %i", dir );

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;

	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			if ( looped )
				break;
			else {
				clientnum = 0;
				looped = qtrue;
			}
		}
		if ( clientnum < 0 ) {
			if ( looped )
				break;
			else {
				clientnum = level.maxclients - 1;
				looped = qtrue;
			}
		}

		// can only follow connected clients
		if ( level.clients[clientnum].pers.connected != CON_CONNECTED )
			continue;

		// can't follow another spectator
		if ( level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR )
			continue;

		// can't follow another spectator
		if ( level.clients[clientnum].tempSpectate >= level.time )
			return;

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}

static void Cmd_FollowNext_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, 1 );
}

static void Cmd_FollowPrev_f( gentity_t *ent ) {
	Cmd_FollowCycle_f( ent, -1 );
}

static void G_SayTo( gentity_t *ent, gentity_t *other, int mode, char color, const char *name, const char *message, char *locMsg ) {
	// valid client
	if ( !other || !other->inuse || !other->client || other->client->pers.connected != CON_CONNECTED ) {
		return;
	}

	// only send team messages to those on your team
	if ( mode == SAY_TEAM  && !OnSameTeam( ent, other ) ) {
		return;
	}

	// only send admin messages to other admins and yourself
	if ( mode == SAY_ADMIN && !other->client->pers.adminUser && ent != other ) {
		return;
	}

	// this client is ignoring us
	if ( other->client->pers.ignore & (1 << (ent - g_entities)) ) {
		return;
	}

	if ( level.gametype == GT_SIEGE && ent->client
		&& (ent->client->tempSpectate >= level.time || ent->client->sess.sessionTeam == TEAM_SPECTATOR)
		&& other->client->sess.sessionTeam != TEAM_SPECTATOR && other->client->tempSpectate < level.time )
	{
		// siege temp spectators should not communicate to ingame players
		return;
	}

	if ( locMsg )
	{
		trap->SendServerCommand( other - g_entities, va( "%s \"%s\" \"%s\" \"%c\" \"%s\"",
			(mode == SAY_TEAM) ? "ltchat" : "lchat", name, locMsg, color, message ) );
	}
	else
	{
		trap->SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\"",
			(mode == SAY_TEAM) ? "tchat" : "chat", name, Q_COLOR_ESCAPE, color, message ) );
	}
}

#define EC			"\x19"
#define CHANNEL_EC	"\x10"
#define ADMIN_EC	"\x11"
#define PRIVATE_EC	"\x12"

static void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int			i;
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

	if ( ent->client->pers.adminData.silenced ) {
		return;
	}

	//RAZTODO: strip ext ascii/control chars
	if ( strstr( ent->client->pers.netname, "<Admin>" ) || Q_strchrs( chatText, "\n\r\x0b" ) ) {
		returnToSender = qtrue;
	}

	switch ( mode ) {
	default:
	case SAY_ADMIN:
		G_LogPrintf( level.log.console, "amsay: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof(name), S_COLOR_YELLOW"<Admin>" S_COLOR_WHITE "%s%c%c" EC ": ",
			ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE
		);
		color = COLOR_YELLOW;
		break;
	case SAY_ALL:
		if ( !Q_stricmpn( chatText, "/me ", 4 ) ) {
			// A /me command
			isMeCmd = qtrue;
			chatText += 4; //Skip "^7* "
		}
		G_LogPrintf( level.log.console, "say: %s: %s\n", ent->client->pers.netname, chatText );
		if ( isMeCmd ) {
			Com_sprintf( name, sizeof(name), S_COLOR_WHITE "* %s" S_COLOR_WHITE EC " ", ent->client->pers.netname );
			color = COLOR_WHITE;
		}
		else {
			Com_sprintf( name, sizeof(name), "%s" S_COLOR_WHITE EC ": ", ent->client->pers.netname );
			color = COLOR_GREEN;
		}
		break;
	case SAY_TEAM:
		G_LogPrintf( level.log.console, "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof(location) ) ) {
			Com_sprintf( name, sizeof(name), EC "(%s%c%c" EC ")" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE,
				COLOR_WHITE
			);
			locMsg = location;
		}
		else {
			Com_sprintf( name, sizeof(name), EC "(%s%c%c" EC ")" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE,
				COLOR_WHITE
			);
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if ( target && level.gametype >= GT_TEAM && target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			Team_GetLocationMsg( ent, location, sizeof(location) ) ) {
			Com_sprintf( name, sizeof(name), EC "[%s%c%c" EC "]" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
			locMsg = location;
		}
		else
			Com_sprintf( name, sizeof(name), EC "[%s%c%c" EC "]" EC ": ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_MAGENTA;
		break;
	}

	Q_strncpyz( text, chatText, sizeof(text) );

	if ( returnToSender ) {
		//Raz: Silly kids, make it look like they said something
		G_LogPrintf( level.log.security, "%s attempted to send a bogus message: %s\n", ent->client->pers.netnameClean, text );
		G_SayTo( ent, ent, mode, color, name, text, locMsg );
		return;
	}

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, locMsg );
		return;
	}

	// echo the text to the console
	if ( dedicated.integer ) {
		trap->Print( "%s%s\n", name, text );
	}

	// send it to all the apropriate clients
	for ( i = 0, other = g_entities; i < level.maxclients; i++, other++ ) {
		G_SayTo( ent, other, mode, color, name, text, locMsg );
	}
}

static void Cmd_Say_f( gentity_t *ent ) {
	char *p = NULL;
	char *res = NULL;
	chatType_t type = SAY_ALL;

	if ( trap->Argc() < 2 ) {
		return;
	}

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_LogPrintf( level.log.security, "Cmd_Say_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}
	if ((res = JPLua_Event_ChatMessageRecieved(ent->s.number, p, type))){
		G_Say(ent, NULL, type, res);
	}
	else{
		G_Say(ent, NULL, type, p);
	}
}

static void Cmd_SayAdmin_f( gentity_t *ent ) {
	char *p = NULL;
	char *res = NULL;
	chatType_t type = SAY_ADMIN;

	if ( trap->Argc() < 2 ) {
		return;
	}

	p = ConcatArgs( 1 );

	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_LogPrintf( level.log.security, "Cmd_SayAdmin_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}
	if ((res = JPLua_Event_ChatMessageRecieved(ent->s.number, p, type))){
		G_Say(ent, NULL, type, res);
	}
	else{
		G_Say(ent, NULL, type, p);
	}
}

static void Cmd_SayTeam_f( gentity_t *ent ) {
	char *p = NULL;
	char *res = NULL;
	chatType_t type = (level.gametype >= GT_TEAM) ? SAY_TEAM : SAY_ALL;

	if ( trap->Argc() < 2 ) {
		return;
	}

	p = ConcatArgs( 1 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_LogPrintf( level.log.security, "Cmd_SayTeam_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}
	if ((res = JPLua_Event_ChatMessageRecieved(ent->s.number, p, type))){
		Q_strncpyz(p, res, MAX_SAY_TEXT);
	}
	if ( ent->client->pers.sayTeamMethod == STM_ADMIN ) {
		type = SAY_ADMIN;
	}
	else if ( ent->client->pers.sayTeamMethod == STM_CENTERPRINT ) {
		if ( ent->client->pers.adminUser && AM_HasPrivilege( ent, PRIV_ANNOUNCE ) ) {
			Q_ConvertLinefeeds( p );
			G_Announce( p );
		}
		else {
			trap->SendServerCommand( ent - g_entities, "print \"You are not allowed to execute that command.\n\"" );
		}
		return;
	}
	G_Say( ent, NULL, type, p );
}

static const char *sayTeamMethods[STM_NUM_METHODS] = {
	"team",
	"admin",
	"centerprint"
};

static void Cmd_SayTeamMod_f( gentity_t *ent ) {
	if ( trap->Argc() == 1 ) {
		int i = ent->client->pers.sayTeamMethod;
		i++;
		i %= STM_NUM_METHODS;
		ent->client->pers.sayTeamMethod = (sayTeamMethod_t)i;
	}

	else {
		char arg[64];
		int i;

		trap->Argv( 1, arg, sizeof(arg) );
		for ( i = 0; i < STM_NUM_METHODS; i++ ) {
			if ( !Q_stricmp( arg, sayTeamMethods[i] ) ) {
				ent->client->pers.sayTeamMethod = (sayTeamMethod_t)i;
				break;
			}
		}
		if ( i == STM_NUM_METHODS ) {
			ent->client->pers.sayTeamMethod = STM_TEAM;
		}
	}

	trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_CYAN "redirecting team messages to: "
		S_COLOR_YELLOW "%s\n\"", sayTeamMethods[ent->client->pers.sayTeamMethod] ) );
}

static void Cmd_Tell_f( gentity_t *ent ) {
	int			targetNum;
	gentity_t	*target;
	char		*p, arg[MAX_TOKEN_CHARS];

	if ( trap->Argc() < 2 )
		return;

	trap->Argv( 1, arg, sizeof(arg) );
	targetNum = G_ClientFromString( ent, arg, FINDCL_SUBSTR | FINDCL_PRINT );
	if ( targetNum == -1 )
		return;

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client )
		return;

	p = ConcatArgs( 2 );

	//Raz: BOF
	if ( strlen( p ) > MAX_SAY_TEXT ) {
		p[MAX_SAY_TEXT - 1] = '\0';
		G_LogPrintf( level.log.security, "Cmd_Tell_f from %d (%s) has been truncated: %s\n", ent->s.number, ent->client->pers.netname, p );
	}

	G_LogPrintf( level.log.console, "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT) )
		G_Say( ent, ent, SAY_TELL, p );
}

//siege voice command
static void Cmd_VoiceCommand_f( gentity_t *ent ) {
	gentity_t *te;
	const char *s;
	char arg[MAX_TOKEN_CHARS];
	int i = 0;

	if ( trap->Argc() < 2 )
		return;

	if ( !(japp_allowVoiceChat.bits & (1 << level.gametype)) ) {
		trap->SendServerCommand( ent - g_entities, "print \"voice_cmd is not applicable in this gametype\n\"" );
		return;
	}

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->tempSpectate >= level.time ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOICECHATASSPEC" ) ) );
		return;
	}

	trap->Argv( 1, arg, sizeof(arg) );

	// hmm.. don't expect a * to be prepended already. maybe someone is trying to be sneaky.
	if ( arg[0] == '*' )
		return;

	s = va( "*%s", arg );

	// now, make sure it's a valid sound to be playing like this.. so people can't go around screaming out death sounds or whatever.
	for ( i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++ ) {
		if ( !bg_customSiegeSoundNames[i] )
			break;
		//it matches this one, so it's ok
		if ( !Q_stricmp( bg_customSiegeSoundNames[i], s ) )
			break;
	}

	// didn't find it in the list
	if ( i == MAX_CUSTOM_SIEGE_SOUNDS || !bg_customSiegeSoundNames[i] )
		return;

	te = G_TempEntity( &vec3_origin, EV_VOICECMD_SOUND );
	te->s.groundEntityNum = ent->s.number;
	te->s.eventParm = G_SoundIndex( bg_customSiegeSoundNames[i] );
	te->r.svFlags |= SVF_BROADCAST;
}

static const char *gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};
static size_t numgc_orders = ARRAY_LEN( gc_orders );

static void Cmd_GameCommand_f( gentity_t *ent ) {
	int			targetNum;
	uint32_t	order;
	gentity_t	*target;
	char		arg[MAX_TOKEN_CHARS] = { 0 };

	if ( trap->Argc() != 3 ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_YELLOW "Usage: \\gc <player id> <order 0-%d>"
			"\n\"", numgc_orders - 1 )
		);
		return;
	}

	trap->Argv( 2, arg, sizeof(arg) );
	order = atoi( arg );

	if ( order >= numgc_orders ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"Bad order: %i\n\"", order ) );
		return;
	}

	trap->Argv( 1, arg, sizeof(arg) );
	targetNum = G_ClientFromString( ent, arg, FINDCL_SUBSTR | FINDCL_PRINT );
	if ( targetNum == -1 )
		return;

	target = &g_entities[targetNum];
	if ( !target->inuse || !target->client )
		return;

	G_LogPrintf( level.log.console, "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, gc_orders[order] );
	G_Say( ent, target, SAY_TELL, gc_orders[order] );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !(ent->r.svFlags & SVF_BOT) )
		G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

static void Cmd_Where_f( gentity_t *ent ) {
	if ( ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR )
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", vtos( &ent->r.currentOrigin ) ) );
	else
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", vtos( &ent->s.origin ) ) );
}

void SiegeClearSwitchData( void ); //g_saga.c
static qboolean G_VoteAllready( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	if ( !level.warmupTime ) {
		trap->SendServerCommand( ent - g_entities, "print \"allready is only available during warmup.\n\"" );
		return qfalse;
	}
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteCapturelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 0x7FFFFFFF );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteClientkick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = atoi( arg2 );

	if ( n < 0 || n >= MAX_CLIENTS ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"invalid client number %d.\n\"", n ) );
		return qfalse;
	}

	if ( g_entities[n].client->pers.connected == CON_DISCONNECTED ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"there is no client with the client number %d.\n\"", n ) );
		return qfalse;
	}

	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, g_entities[n].client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteCointoss( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s", arg1 );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteFraglimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 0x7FFFFFFF );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteGametype( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int gt = atoi( arg2 );

	// ffa, ctf, tdm, etc
	if ( arg2[0] && isalpha( arg2[0] ) ) {
		gt = BG_GetGametypeForString( arg2 );
		if ( gt == -1 ) {
			trap->SendServerCommand( ent - g_entities, va( "print \"Gametype (%s) unrecognised, defaulting to FFA/Deathmatch\n\"", arg2 ) );
			gt = GT_FFA;
		}
	}
	// numeric but out of range
	else if ( gt < 0 || gt >= GT_MAX_GAME_TYPE ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"Gametype (%i) is out of range, defaulting to FFA/Deathmatch\n\"", gt ) );
		gt = GT_FFA;
	}

	level.votingGametype = qtrue;
	level.votingGametypeTo = gt;

	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %d", arg1, gt );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "%s %s", arg1, BG_GetGametypeString( gt ) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteInstagib( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VotePromode( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteShootFromEye( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteSpeedcaps( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteSuicideDropFlag( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteKick( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int clientid = G_ClientFromString( ent, arg2, FINDCL_SUBSTR | FINDCL_PRINT );
	gentity_t *target = NULL;

	if ( clientid == -1 )
		return qfalse;

	target = &g_entities[clientid];
	if ( !target || !target->inuse || !target->client )
		return qfalse;

	Com_sprintf( level.voteString, sizeof(level.voteString), "clientkick %d", clientid );
	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "kick %s", target->client->pers.netname );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

const char *G_GetArenaInfoByMap( const char *map );
static void Cmd_MapList_f( gentity_t *ent ) {
	int i, toggle = 0;
	char map[24] = "--", buf[512] = { 0 };

	Q_strcat( buf, sizeof(buf), "Map list:" );

	for ( i = 0; i < level.arenas.num; i++ ) {
		Q_strncpyz( map, Info_ValueForKey( level.arenas.infos[i], "map" ), sizeof(map) );
		Q_CleanString( map, STRIP_COLOUR );

		if ( G_DoesMapSupportGametype( map, level.gametype ) ) {
			const char *tmpMsg = va( " ^%c%s", (++toggle & 1) ? COLOR_GREEN : COLOR_YELLOW, map );
			if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof(buf) ) {
				trap->SendServerCommand( ent - g_entities, va( "print \"%s\"", buf ) );
				buf[0] = '\0';
			}
			Q_strcat( buf, sizeof(buf), tmpMsg );
		}
	}

	trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", buf ) );
}

static qboolean G_VoteMap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	char bspName[MAX_QPATH] = { '\0' };
	const char *mapName = NULL, *mapName2 = NULL, *arenaInfo = NULL;
	fileHandle_t fp = NULL_FILE;

	// didn't specify a map, show available maps
	if ( numArgs < 3 ) {
		Cmd_MapList_f( ent );
		return qfalse;
	}

	if ( strchr( arg2, '\\' ) ) {
		trap->SendServerCommand( ent - g_entities, "print \"Can't have mapnames with a \\\n\"" );
		return qfalse;
	}

	Com_sprintf( bspName, sizeof(bspName), "maps/%s.bsp", arg2 );
	if ( trap->FS_Open( bspName, &fp, FS_READ ) <= 0 ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"Can't find map %s on server\n\"", bspName ) );
		if ( fp != NULL_FILE )
			trap->FS_Close( fp );
		return qfalse;
	}
	trap->FS_Close( fp );

	if ( !japp_voteMapAnyGT.integer && !G_DoesMapSupportGametype( arg2, level.gametype ) ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTE_MAPNOTSUPPORTEDBYGAME" ) ) );
		return qfalse;
	}

	// preserve the map rotation
	if ( nextmap.string[0] ) {
		Com_sprintf( level.voteString, sizeof(level.voteString), "%s %s; set nextmap \"%s\"", arg1, arg2,
			nextmap.string );
	}
	else {
		Com_sprintf( level.voteString, sizeof(level.voteString), "%s %s", arg1, arg2 );
	}

	arenaInfo = G_GetArenaInfoByMap( arg2 );
	if ( arenaInfo ) {
		mapName = Info_ValueForKey( arenaInfo, "longname" );
		mapName2 = Info_ValueForKey( arenaInfo, "map" );
	}

	if ( !mapName || !mapName[0] ) {
		mapName = "ERROR";
	}

	if ( !mapName2 || !mapName2[0] ) {
		mapName2 = "ERROR";
	}

	Com_sprintf( level.voteDisplayString, sizeof(level.voteDisplayString), "map %s (%s)", mapName, mapName2 );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteMapRestart( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = Q_clampi( 0, atoi( arg2 ), 60 );
	if ( numArgs < 3 )
		n = 5;
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteNextmap( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	if ( !nextmap.string[0] ) {
		trap->SendServerCommand( ent - g_entities, "print \"nextmap not set.\n\"" );
		return qfalse;
	}
	SiegeClearSwitchData();
	Com_sprintf( level.voteString, sizeof(level.voteString), "vstr nextmap" );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VotePause( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteShuffle( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s", arg1 );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteTimelimit( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	float tl = Q_clamp( 0.0f, atof( arg2 ), 35790.0f );
	if ( Q_isintegral( tl ) )
		Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, (int)tl );
	else
		Com_sprintf( level.voteString, sizeof(level.voteString), "%s %.3f", arg1, tl );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

static qboolean G_VoteWarmup( gentity_t *ent, int numArgs, const char *arg1, const char *arg2 ) {
	int n = !!atoi( arg2 );
	Com_sprintf( level.voteString, sizeof(level.voteString), "%s %i", arg1, n );
	Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	return qtrue;
}

typedef struct voteString_s {
	const char	*string;
	const char	*aliases;	// space delimited list of aliases, will always show the real vote string
	qboolean( *func )(gentity_t *ent, int numArgs, const char *arg1, const char *arg2);
	int			numArgs;	// number of REQUIRED arguments, not total/optional arguments
	uint32_t	validGT;	// bit-flag of valid gametypes
	qboolean	voteDelay;	// if true, will delay executing the vote string after it's accepted by japp_voteDelay
	const char	*shortHelp;	// NULL if no arguments needed
	const char	*longHelp;	// long help text, accessible with /vhelp and maybe /callvote help
} voteString_t;

static voteString_t validVoteStrings[] = {
	// vote string				aliases										# args	valid gametypes							exec delay		short help	long help
	{ "allready", "ready", G_VoteAllready, 0, GTB_ALL, qfalse, NULL, "" },
	{ "capturelimit", "caps", G_VoteCapturelimit, 1, GTB_CTF | GTB_CTY, qtrue, "<num>", "" },
	{ "clientkick", NULL, G_VoteClientkick, 1, GTB_ALL, qfalse, "<num>", "" },
	{ "cointoss", "coinflip", G_VoteCointoss, 0, GTB_ALL, qfalse, NULL, "" },
	{ "fraglimit", "frags", G_VoteFraglimit, 1, GTB_ALL & ~(GTB_SIEGE | GTB_CTF | GTB_CTY), qtrue, "<num>", "" },
	{ "g_gametype", "gametype gt mode", G_VoteGametype, 1, GTB_ALL, qtrue, "<name>", "" },
	{ "japp_instagib", "instagib insta", G_VoteInstagib, 1, GTB_ALL, qtrue, "<0-1>", "" },
	{ "japp_promode", "promode cpm", G_VotePromode, 1, GTB_ALL, qtrue, "<0-1>", "" },
	{ "japp_shootFromEye", "shootfromeye", G_VoteShootFromEye, 1, GTB_ALL, qfalse, "<0-1>", "" },
	{ "japp_speedCaps", "speedcaps", G_VoteSpeedcaps, 1, GTB_CTF | GTB_CTY, qfalse, "<0-1>", "" },
	{ "japp_suicideDropFlag", "killdropflag", G_VoteSuicideDropFlag, 1, GTB_CTF | GTB_CTY, qfalse, "<0-1>", "Drop flag when you /kill yourself" },
	{ "kick", NULL, G_VoteKick, 1, GTB_ALL, qfalse, "<name>", "" },
	{ "map", NULL, G_VoteMap, 0, GTB_ALL, qtrue, "<name>", "" },
	{ "map_restart", "restart", G_VoteMapRestart, 0, GTB_ALL, qtrue, NULL, "Restarts the current map\nExample: callvote map_restart" },
	{ "nextmap", NULL, G_VoteNextmap, 0, GTB_ALL, qtrue, NULL, "" },
	{ "pause", NULL, G_VotePause, 0, GTB_ALL, qfalse, NULL, "" },
	{ "shuffle", NULL, G_VoteShuffle, 0, GTB_ALL & ~(GTB_NOTTEAM), qtrue, NULL, "" },
	{ "timelimit", "time", G_VoteTimelimit, 1, GTB_ALL, qtrue, "<num>", "" },
	{ "warmup", "dowarmup", G_VoteWarmup, 1, GTB_ALL, qtrue, "<0-1>", "" },
};
static const int validVoteStringsSize = ARRAY_LEN( validVoteStrings );

void SV_ToggleAllowVote_f( void ) {
	if ( trap->Argc() == 1 ) {
		int i = 0;
		for ( i = 0; i < validVoteStringsSize; i++ ) {
			if ( (g_allowVote.bits & (1 << i)) ) {
				trap->Print( "%2d [X] %s\n", i, validVoteStrings[i].string );
			}
			else {
				trap->Print( "%2d [ ] %s\n", i, validVoteStrings[i].string );
			}
		}

		return;
	}
	else {
		char arg[8] = { 0 };
		int index;
		const uint32_t mask = (1 << validVoteStringsSize) - 1;

		trap->Argv( 1, arg, sizeof(arg) );
		index = atoi( arg );

		if ( index < 0 || index >= validVoteStringsSize ) {
			trap->Print( "ToggleAllowVote: Invalid range: %i [0, %i]\n", index, validVoteStringsSize - 1 );
			return;
		}

		trap->Cvar_Set( "g_allowVote", va( "%i", (1 << index) ^ (g_allowVote.bits & mask ) ) );
		trap->Cvar_Update( &g_allowVote );

		trap->Print( "%s %s^7\n", validVoteStrings[index].string, ((g_allowVote.bits & (1 << index))
			? "^2Enabled" : "^1Disabled") );
	}
}

static void Cmd_CallVote_f( gentity_t *ent ) {
	int				i = 0, numArgs = 0;
	char			arg1[MAX_CVAR_VALUE_STRING], arg2[MAX_CVAR_VALUE_STRING];
	voteString_t	*vote = NULL;

	// not allowed to vote at all
	if ( !g_allowVote.integer ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTE" ) ) );
		return;
	}

	// vote in progress
	else if ( level.voteTime || level.voteExecuteTime >= level.time ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEINPROGRESS" ) ) );
		return;
	}

	// can't vote as a spectator, except in (power)duel
	else if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL && ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOSPECVOTE" ) ) );
		return;
	}

	// make sure it is a valid command to vote on
	numArgs = trap->Argc();
	trap->Argv( 1, arg1, sizeof(arg1) );
	if ( numArgs > 1 )
		Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof(arg2) );

	//Raz: callvote exploit, filter \n and \r ==> in both args
	if ( Q_strchrs( arg1, ";\r\n" ) || Q_strchrs( arg2, ";\r\n" ) ) {
		trap->SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	// check for invalid votes
	for ( i = 0; i < validVoteStringsSize; i++ ) {
		if ( !(g_allowVote.bits & (1 << i)) )
			continue;

		if ( !Q_stricmp( arg1, validVoteStrings[i].string ) )
			break;

		// see if they're using an alias, and set arg1 to the actual vote string
		if ( validVoteStrings[i].aliases ) {
			char tmp[MAX_TOKEN_CHARS] = { 0 }, *p = NULL;
			const char *delim = " ";
			Q_strncpyz( tmp, validVoteStrings[i].aliases, sizeof(tmp) );
			p = strtok( tmp, delim );
			while ( p != NULL ) {
				if ( !Q_stricmp( arg1, p ) ) {
					Q_strncpyz( arg1, validVoteStrings[i].string, sizeof(arg1) );
					goto validVote;
				}
				p = strtok( NULL, delim );
			}
		}
	}
	// invalid vote string, abandon ship
	if ( i == validVoteStringsSize ) {
		char buf[MAX_STRING_CHARS] = { 0 };
		int toggle = 0;
		trap->SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap->SendServerCommand( ent - g_entities, "print \"Allowed vote strings are: \"" );
		for ( i = 0; i < validVoteStringsSize; i++ ) {
			if ( !(g_allowVote.bits & (1 << i)) )
				continue;

			toggle = !toggle;
			if ( validVoteStrings[i].shortHelp ) {
				Q_strcat( buf, sizeof(buf), va( "^%c%s %s ", toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string, validVoteStrings[i].shortHelp ) );
			}
			else {
				Q_strcat( buf, sizeof(buf), va( "^%c%s ", toggle ? COLOR_GREEN : COLOR_YELLOW,
					validVoteStrings[i].string ) );
			}
		}

		//RAZTODO: buffer and send in multiple messages in case of overflow
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

validVote:
	vote = &validVoteStrings[i];
	if ( !(vote->validGT & (1 << level.gametype)) ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s is not applicable in this gametype.\n\"", arg1 ) );
		return;
	}

	if ( numArgs < vote->numArgs + 2 ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s requires more arguments: %s\n\"", arg1, vote->shortHelp ) );
		return;
	}

	level.votingGametype = qfalse;

	level.voteExecuteDelay = vote->voteDelay ? japp_voteDelay.integer : 0;

	// there is still a vote to be executed, execute it and store the new vote
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		if ( !level.votePoll )
			trap->SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	// pass the args onto vote-specific handlers for parsing/filtering
	if ( vote->func ) {
		if ( !vote->func( ent, numArgs, arg1, arg2 ) )
			return;
	}
	// otherwise assume it's a command
	else {
		Com_sprintf( level.voteString, sizeof(level.voteString), "%s \"%s\"", arg1, arg2 );
		Q_strncpyz( level.voteDisplayString, level.voteString, sizeof(level.voteDisplayString) );
		Q_strncpyz( level.voteStringClean, level.voteString, sizeof(level.voteStringClean) );
	}
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s (%s)\n\"", ent->client->pers.netname,
		G_GetStringEdString( "MP_SVGAME", "PLCALLEDVOTE" ), level.voteStringClean )
	);

	// start the voting, the caller automatically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;
	level.votePoll = qfalse;

	for ( i = 0; i < level.maxclients; i++ ) {
		level.clients[i].mGameFlags &= ~PSG_VOTED;
		level.clients[i].pers.vote = 0;
	}

	ent->client->mGameFlags |= PSG_VOTED;
	ent->client->pers.vote = 1;

	trap->SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap->SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap->SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap->SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

static void Cmd_Vote_f( gentity_t *ent ) {
	char		msg[64];

	if ( !level.voteTime ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTEINPROG" ) ) );
		return;
	}
	if ( ent->client->mGameFlags & PSG_VOTED ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEALREADY" ) ) );
		return;
	}
	if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL ) {
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOVOTEASSPEC" ) ) );
			return;
		}
	}

	trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "PLVOTECAST" ) ) );

	ent->client->mGameFlags |= PSG_VOTED;

	trap->Argv( 1, msg, sizeof(msg) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		ent->client->pers.vote = 1;
		trap->SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	}
	else {
		level.voteNo++;
		ent->client->pers.vote = 2;
		trap->SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
	}

	// a majority will be determined in CheckVote, which will also account for players entering or leaving
}

static void Cmd_SetViewpos_f( gentity_t *ent ) {
	vector3		origin, angles;
	char		buffer[MAX_TOKEN_CHARS];
	int			i;

	if ( trap->Argc() != 5 ) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Usage: \\setviewpos x y z yaw\n\"" );
		return;
	}

	VectorClear( &angles );
	for ( i = 0; i < 3; i++ ) {
		trap->Argv( i + 1, buffer, sizeof(buffer) );
		origin.raw[i] = atof( buffer );
	}

	trap->Argv( 4, buffer, sizeof(buffer) );
	angles.yaw = atof( buffer );

	TeleportPlayer( ent, &origin, &angles );
}

void G_LeaveVehicle( gentity_t* ent, qboolean ConCheck ) {
	if ( ent->client->ps.m_iVehicleNum ) {
		// tell it I'm getting off
		gentity_t *veh = &g_entities[ent->client->ps.m_iVehicleNum];

		if ( veh->inuse && veh->client && veh->m_pVehicle ) {
			if ( ConCheck ) {
				// check connection
				clientConnected_t pCon = ent->client->pers.connected;
				ent->client->pers.connected = CON_DISCONNECTED;
				veh->m_pVehicle->m_pVehicleInfo->Eject( veh->m_pVehicle, (bgEntity_t *)ent, qtrue );
				ent->client->pers.connected = pCon;
			}
			else // or not.
				veh->m_pVehicle->m_pVehicleInfo->Eject( veh->m_pVehicle, (bgEntity_t *)ent, qtrue );
		}
	}

	ent->client->ps.m_iVehicleNum = 0;
}

int G_ItemUsable( playerState_t *ps, int forcedUse ) {
	vector3 fwd, fwdorg, dest, pos, yawonly;
	vector3 mins, maxs;
	vector3 trtest;
	trace_t tr;

	//Raz: dead players shouldn't use items
	if ( ps->stats[STAT_HEALTH] <= 0 )
		return 0;

	if ( ps->m_iVehicleNum )
		return 0;

	if ( ps->pm_flags & PMF_USE_ITEM_HELD )
		return 0;

	if ( !forcedUse )
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;

	if ( !BG_IsItemSelectable( ps, forcedUse ) )
		return 0;

	switch ( forcedUse ) {
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if ( ps->stats[STAT_HEALTH] <= 0 || ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] )
			return 0;
		return 1;

	case HI_SEEKER:
		if ( ps->eFlags & EF_SEEKERDRONE ) {
			G_AddEvent( &g_entities[ps->clientNum], EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED );
			return 0;
		}
		return 1;

	case HI_SENTRY_GUN:
		if ( ps->fd.sentryDeployed ) {
			G_AddEvent( &g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED );
			return 0;
		}

		yawonly.roll = 0;
		yawonly.pitch = 0;
		yawonly.yaw = ps->viewangles.yaw;

		VectorSet( &mins, -8, -8, 0 );
		VectorSet( &maxs, 8, 8, 24 );

		AngleVectors( &yawonly, &fwd, NULL, NULL );

		fwdorg.x = ps->origin.x + fwd.x * 64;
		fwdorg.y = ps->origin.y + fwd.y * 64;
		fwdorg.z = ps->origin.z + fwd.z * 64;

		trtest.x = fwdorg.x + fwd.x * 16;
		trtest.y = fwdorg.y + fwd.y * 16;
		trtest.z = fwdorg.z + fwd.z * 16;

		trap->Trace( &tr, &ps->origin, &mins, &maxs, &trtest, ps->clientNum, MASK_PLAYERSOLID, qfalse, 0, 0 );

		if ( (tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid ) {
			G_AddEvent( &g_entities[ps->clientNum], EV_ITEMUSEFAIL, SENTRY_NOROOM );
			return 0;
		}

		return 1;

	case HI_SHIELD:
		VectorSet( &mins, -8, -8, 0 );
		VectorSet( &maxs, 8, 8, 8 );

		AngleVectors( &ps->viewangles, &fwd, NULL, NULL );
		fwd.z = 0;
		VectorMA( &ps->origin, 64, &fwd, &dest );
		trap->Trace( &tr, &ps->origin, &mins, &maxs, &dest, ps->clientNum, MASK_SHOT, qfalse, 0, 0 );
		if ( tr.fraction > 0.9f && !tr.startsolid && !tr.allsolid ) {
			VectorCopy( &tr.endpos, &pos );
			VectorSet( &dest, pos.x, pos.y, pos.z - 4096 );
			trap->Trace( &tr, &pos, &mins, &maxs, &dest, ps->clientNum, MASK_SOLID, qfalse, 0, 0 );
			if ( !tr.startsolid && !tr.allsolid )
				return 1;
		}
		G_AddEvent( &g_entities[ps->clientNum], EV_ITEMUSEFAIL, SHIELD_NOROOM );
		return 0;

	case HI_JETPACK: //do something?
	case HI_HEALTHDISP:
	case HI_AMMODISP:
	case HI_EWEB:
	case HI_CLOAK:
		return 1;

	default:
		return 1;
	}
}

void saberKnockDown( gentity_t *saberent, gentity_t *saberOwner, gentity_t *other );
void Cmd_ToggleSaber_f( gentity_t *ent ) {
	// if they are being gripped, don't let them unholster their saber
	if ( ent->client->ps.fd.forceGripCripple && ent->client->ps.saberHolstered )
		return;

	if ( ent->client->ps.saberInFlight ) {
		// turn it off in midair
		if ( ent->client->ps.saberEntityNum )
			saberKnockDown( &g_entities[ent->client->ps.saberEntityNum], ent, ent );
		return;
	}

	if ( ent->client->ps.forceHandExtend != HANDEXTEND_NONE )
		return;

	if ( ent->client->ps.weapon != WP_SABER )
		return;

	if ( ent->client->ps.duelTime >= level.time )
		return;

	if ( ent->client->ps.saberLockTime >= level.time )
		return;

	if ( ent->client && ent->client->ps.weaponTime < 1 ) {
		if ( ent->client->ps.saberHolstered == 2 ) {
			ent->client->ps.saberHolstered = 0;

			if ( ent->client->saber[0].soundOn )
				G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOn );
			if ( ent->client->saber[1].soundOn )
				G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOn );
		}
		else {
			ent->client->ps.saberHolstered = 2;
			if ( ent->client->saber[0].soundOff )
				G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOff );
			if ( ent->client->saber[1].soundOff && ent->client->saber[1].model[0] )
				G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );
			// prevent anything from being done for 400ms after holster
			ent->client->ps.weaponTime = 400;
		}
	}
}

void Cmd_SaberAttackCycle_f( gentity_t *ent ) {
	int selectLevel = 0;
	qboolean usingSiegeStyle = qfalse;

	if ( !ent || !ent->client )
		return;

	if ( ent->client->ps.weapon != WP_SABER )
		return;

	if ( ent->client->saber[0].model[0] && ent->client->saber[1].model[0] ) {
		// no cycling for akimbo
		if ( WP_SaberCanTurnOffSomeBlades( &ent->client->saber[1] ) ) {
			// can turn second saber off
			if ( ent->client->ps.saberHolstered == 1 ) {
				// have one holstered, unholster it
				G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOn );
				ent->client->ps.saberHolstered = 0;
				//g_active should take care of this, but...
				ent->client->ps.fd.saberAnimLevel = SS_DUAL;
			}
			else if ( ent->client->ps.saberHolstered == 0 ) {
				// have none holstered
				if ( (ent->client->saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE) ) {
					// can't turn it off manually
				}
				else if ( ent->client->saber[1].bladeStyle2Start > 0 && (ent->client->saber[1].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2) ) {
					// can't turn it off manually
				}
				else {
					// turn it off
					G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );
					ent->client->ps.saberHolstered = 1;
					//g_active should take care of this, but...
					ent->client->ps.fd.saberAnimLevel = SS_FAST;
				}
			}

			if ( d_saberStanceDebug.integer )
				trap->SendServerCommand( ent - g_entities, va( "print \"SABERSTANCEDEBUG: Attempted to toggle dual saber blade.\n\"" ) );
			return;
		}
	}
	else if ( ent->client->saber[0].numBlades > 1 && WP_SaberCanTurnOffSomeBlades( &ent->client->saber[0] ) ) {
		// use staff stance then.
		if ( ent->client->ps.saberHolstered == 1 ) {
			// second blade off
			if ( ent->client->ps.saberInFlight ) {
				// can't turn second blade back on if it's in the air, you naughty boy!
				if ( d_saberStanceDebug.integer )
					trap->SendServerCommand( ent - g_entities, va( "print \"SABERSTANCEDEBUG: Attempted to toggle staff blade in air.\n\"" ) );
				return;
			}
			//turn it on
			G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOn );
			ent->client->ps.saberHolstered = 0;
			//g_active should take care of this, but...
			if ( ent->client->saber[0].stylesForbidden ) {
				// have a style we have to use
				WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );
				// not busy, set it now
				if ( ent->client->ps.weaponTime <= 0 )
					ent->client->ps.fd.saberAnimLevel = selectLevel;
				else // can't set it now or we might cause unexpected chaining, so queue it
					ent->client->saberCycleQueue = selectLevel;
			}
		}
		else if ( ent->client->ps.saberHolstered == 0 ) {
			// both blades on
			if ( (ent->client->saber[0].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE) ) {
				// can't turn it off manually
			}
			else if ( ent->client->saber[0].bladeStyle2Start > 0 && (ent->client->saber[0].saberFlags2 & SFL2_NO_MANUAL_DEACTIVATE2) ) {
				// can't turn it off manually
			}
			else {
				//turn second one off
				G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOff );
				ent->client->ps.saberHolstered = 1;
				//g_active should take care of this, but...
				if ( ent->client->saber[0].singleBladeStyle != SS_NONE ) {
					// not busy, set it now
					if ( ent->client->ps.weaponTime <= 0 )
						ent->client->ps.fd.saberAnimLevel = ent->client->saber[0].singleBladeStyle;
					else // can't set it now or we might cause unexpected chaining, so queue it
						ent->client->saberCycleQueue = ent->client->saber[0].singleBladeStyle;
				}
			}
		}
		if ( d_saberStanceDebug.integer )
			trap->SendServerCommand( ent - g_entities, va( "print \"SABERSTANCEDEBUG: Attempted to toggle staff blade.\n\"" ) );
		return;
	}

	// resume off of the queue if we haven't gotten a chance to update it yet
	if ( ent->client->saberCycleQueue ) {
		selectLevel = ent->client->saberCycleQueue;
	}
	else {
		selectLevel = ent->client->ps.fd.saberAnimLevel;
	}

	if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 && bgSiegeClasses[ent->client->siegeClass].saberStance ) {
		// we have a flag of useable stances so cycle through it instead
		int i;

		usingSiegeStyle = qtrue;

		for ( i = selectLevel + 1; i != selectLevel; i++ ) {
			// cycle around upward til we hit the next style or end up back on this one
			if ( i >= SS_NUM_SABER_STYLES )
				i = SS_FAST;

			if ( bgSiegeClasses[ent->client->siegeClass].saberStance & (1 << i) ) {
				// we can use this one, select it and break out.
				selectLevel = i;
				break;
			}
		}

		if ( d_saberStanceDebug.integer )
			trap->SendServerCommand( ent - g_entities, va( "print \"SABERSTANCEDEBUG: Attempted to cycle given class stance.\n\"" ) );
	}
	else {
		selectLevel++;
		if ( selectLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
			selectLevel = FORCE_LEVEL_1;
		if ( d_saberStanceDebug.integer )
			trap->SendServerCommand( ent - g_entities, va( "print \"SABERSTANCEDEBUG: Attempted to cycle stance normally.\n\"" ) );
	}

	// make sure it's valid, change it if not
	if ( !usingSiegeStyle )
		WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &selectLevel );

	// not busy, set it now
	if ( ent->client->ps.weaponTime <= 0 )
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = selectLevel;
	else // can't set it now or we might cause unexpected chaining, so queue it
		ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = selectLevel;
}

static qboolean G_OtherPlayersDueling( void ) {
	int i;
	gentity_t *ent;

	for ( i = 0, ent = g_entities; i < level.maxclients; i++, ent++ ) {
		if ( ent && ent->inuse && ent->client && ent->client->ps.duelInProgress )
			return qtrue;
	}

	return qfalse;
}

void Cmd_EngageDuel_f( gentity_t *ent ) {
	trace_t *tr;
	int weapon = WP_SABER;

	if ( !(g_privateDuel.bits & PRIVDUEL_ALLOW) ) {
		return;
	}

	// not allowed if you're not alive or not ingame
	if ( ent->health <= 0
		|| ent->client->tempSpectate >= level.time
		|| ent->client->sess.sessionTeam == TEAM_SPECTATOR )
	{
		return;
	}


	if ( ent->client->pers.adminData.isSlept ) {
		return;
	}

	// no private dueling in team modes
	if ( !(g_privateDuel.bits & PRIVDUEL_TEAM) && level.gametype >= GT_TEAM ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"",
			G_GetStringEdString( "MP_SVGAME", "NODUEL_GAMETYPE" ) ) );
		return;
	}

	if ( ent->client->ps.duelInProgress || ent->client->ps.duelTime >= level.time ) {
		return;
	}

	if ( ent->client->ps.saberInFlight ) {
		return;
	}

	//New: Don't let a player duel if he just did and hasn't waited 10 seconds yet (note: If someone challenges him, his duel timer will reset so he can accept)
	if ( !(g_privateDuel.bits & PRIVDUEL_MULTI) && ent->client->ps.fd.privateDuelTime > level.time ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "CANTDUEL_JUSTDID" ) ) );
		return;
	}

	//Raz: Multi-duel
	if ( !(g_privateDuel.bits & PRIVDUEL_MULTI) && G_OtherPlayersDueling() ) {
		trap->SendServerCommand( ent - g_entities, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "CANTDUEL_BUSY" ) ) );
		return;
	}

	if ( (g_privateDuel.bits & PRIVDUEL_WEAP) ) {
		if ( !(ent->client->pers.CSF & CSF_WEAPONDUEL) || trap->Argc() == 1 ) {
			weapon = ent->client->ps.weapon;
		}
		else {
			weapon = BG_FindWeapon( ConcatArgs( 1 ) );
			if ( weapon == WP_NONE ) {
				weapon = ent->client->ps.weapon;
			}
		}
	}

	tr = G_RealTrace( ent, 256.0f );

	if ( tr->fraction < 1.0f && tr->entityNum < MAX_CLIENTS ) {
		gentity_t *challenged = &g_entities[tr->entityNum];

		if ( !challenged || !challenged->client || !challenged->inuse ||
			challenged->health < 1 || challenged->client->ps.stats[STAT_HEALTH] < 1 ||
			//	challenged->client->ps.weapon != WP_SABER ||
			challenged->client->ps.duelInProgress ||
			challenged->client->ps.saberInFlight ) {
			return;
		}


		if ( level.gametype >= GT_TEAM && OnSameTeam( ent, challenged ) ) {
			return;
		}

		if ( challenged->client->pers.adminData.isSlept ) {
			return;
		}

		if ( challenged->client->ps.duelIndex == ent->s.number && challenged->client->ps.duelTime >= level.time ) {
			trap->SendServerCommand( -1, va( "print \"%s " S_COLOR_WHITE "%s %s " S_COLOR_WHITE "(%s)!\n\"",
				challenged->client->pers.netname, G_GetStringEdString( "MP_SVGAME", "PLDUELACCEPT" ),
				ent->client->pers.netname, weaponData[challenged->client->pers.duelWeapon].longName ) );




			ent->client->ps.duelInProgress = challenged->client->ps.duelInProgress = qtrue;
			ent->client->ps.duelTime = challenged->client->ps.duelTime = level.time + 2000;

			// copy the start pos
			VectorCopy( &ent->client->ps.origin, &ent->client->pers.duelStartPos );
			VectorCopy( &challenged->client->ps.origin, &challenged->client->pers.duelStartPos );

			G_AddEvent( ent, EV_PRIVATE_DUEL, 1 );
			G_AddEvent( challenged, EV_PRIVATE_DUEL, 1 );

			ent->duelStartTick = level.time;
			challenged->duelStartTick = level.time;
			ent->duelHitCount = 0;
			challenged->duelHitCount = 0;

			if ( challenged->client->pers.duelWeapon == WP_SABER ) {
				// Holster their sabers now, until the duel starts (then they'll get auto-turned on to look cool)
				if ( !ent->client->ps.saberHolstered ) {
					if ( ent->client->saber[0].soundOff ) {
						G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOff );
					}
					if ( ent->client->saber[1].soundOff && ent->client->saber[1].model[0] ) {
						G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );
					}

					ent->client->ps.weaponTime = 400;
					ent->client->ps.saberHolstered = 2;
				}
				if ( !challenged->client->ps.saberHolstered ) {
					if ( challenged->client->saber[0].soundOff ) {
						G_Sound( challenged, CHAN_AUTO, challenged->client->saber[0].soundOff );
					}
					if ( challenged->client->saber[1].soundOff && challenged->client->saber[1].model[0] ) {
						G_Sound( challenged, CHAN_AUTO, challenged->client->saber[1].soundOff );
					}

					challenged->client->ps.weaponTime = 400;
					challenged->client->ps.saberHolstered = 2;
				}
			}

			else {
				// reset their weapon times
				ent->client->ps.weaponTime = challenged->client->ps.weaponTime = 1000;
			}

			ent->client->ps.weapon = challenged->client->ps.weapon = challenged->client->pers.duelWeapon;

			// set health etc
			ent->health
				= challenged->health
				= ent->client->ps.stats[STAT_HEALTH]
				= challenged->client->ps.stats[STAT_HEALTH]
				= g_privateDuelHealth.integer;

			ent->client->ps.stats[STAT_ARMOR]
				= challenged->client->ps.stats[STAT_ARMOR]
				= g_privateDuelShield.integer;

		}
		else {
			// Print the message that a player has been challenged in private, only announce the actual duel initiation in private
			// set their desired dueling weapon.
			ent->client->pers.duelWeapon = weapon;

			trap->SendServerCommand( challenged - g_entities, va( "cp \"%s " S_COLOR_WHITE "%s\n" S_COLOR_YELLOW
				"Weapon: " S_COLOR_WHITE "%s\n\"", ent->client->pers.netname,
				G_GetStringEdString( "MP_SVGAME", "PLDUELCHALLENGE" ), weaponData[weapon].longName )
			);
			trap->SendServerCommand( ent - g_entities, va( "cp \"%s %s\n" S_COLOR_YELLOW "Weapon: " S_COLOR_WHITE "%s"
				"\n\"", G_GetStringEdString( "MP_SVGAME", "PLDUELCHALLENGED" ), challenged->client->pers.netname,
				weaponData[weapon].longName )
			);
		}

		challenged->client->ps.fd.privateDuelTime = 0; //reset the timer in case this player just got out of a duel. He should still be able to accept the challenge.

		ent->client->ps.forceHandExtend = HANDEXTEND_DUELCHALLENGE;
		ent->client->ps.forceHandExtendTime = level.time + 1000;

		ent->client->ps.duelIndex = challenged->s.number;
		ent->client->ps.duelTime = level.time + 5000;
	}
}

#ifdef _DEBUG
void DismembermentByNum( gentity_t *self, int num );
void G_SetVehDamageFlags( gentity_t *veh, int shipSurf, int damageLevel );
#endif

qboolean TryGrapple( gentity_t *ent ) {
	// weapon busy
	if ( ent->client->ps.weaponTime > 0 ) {
		return qfalse;
	}

	// force power or knockdown or something
	if ( ent->client->ps.forceHandExtend != HANDEXTEND_NONE ) {
		return qfalse;
	}

	// already grappling? but weapontime should be > 0 then..
	if ( ent->client->grappleState ) {
		return qfalse;
	}

	if ( ent->client->ps.weapon != WP_SABER && ent->client->ps.weapon != WP_MELEE ) {
		return qfalse;
	}

	if ( ent->client->ps.weapon == WP_SABER && !ent->client->ps.saberHolstered ) {
		Cmd_ToggleSaber_f( ent );
		// must have saber holstered
		if ( !ent->client->ps.saberHolstered ) {
			return qfalse;
		}
	}

	G_SetAnim( ent, &ent->client->pers.cmd, SETANIM_BOTH, /*BOTH_KYLE_PA_1*/BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
	if ( ent->client->ps.torsoAnim == BOTH_KYLE_GRAB ) {
		// providing the anim set succeeded..
		ent->client->ps.torsoTimer += 500; //make the hand stick out a little longer than it normally would
		if ( ent->client->ps.legsAnim == ent->client->ps.torsoAnim ) {
			ent->client->ps.legsTimer = ent->client->ps.torsoTimer;
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		return qtrue;
	}

	return qfalse;
}

static void Cmd_TargetUse_f( gentity_t *ent ) {
	if ( trap->Argc() > 1 ) {
		char sArg[MAX_STRING_CHARS] = { 0 };
		gentity_t *targ = NULL;

		trap->Argv( 1, sArg, sizeof(sArg) );

		do {
			targ = G_Find( NULL, FOFS( targetname ), sArg );
			if ( targ->use ) {
				targ->use( targ, ent, ent );
			}
			JPLua_Entity_CallFunction(targ, JPLUA_ENTITY_USE, (intptr_t)ent, (intptr_t)ent);
		} while ( targ );
	}
	else {
		trace_t *tr = G_RealTrace( ent, 0.0f );
		if ( tr->fraction < 1.0f && tr->entityNum > MAX_CLIENTS && tr->entityNum < ENTITYNUM_MAX_NORMAL ) {
			gentity_t *targ = g_entities + tr->entityNum;
			if ( targ->use ) {
				targ->use( targ, ent, ent );
			}
			JPLua_Entity_CallFunction(targ, JPLUA_ENTITY_USE, (intptr_t)ent, (intptr_t)ent);
		}
	}
}

#define BOT_MOVE_ARG 4000
static void Cmd_BotMoveForward_f( gentity_t *ent ) {
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof(sarg) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, BOT_MOVE_ARG, -1, -1 );
}

static void Cmd_BotMoveBack_f( gentity_t *ent ) {
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof(sarg) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -BOT_MOVE_ARG, -1, -1 );
}

static void Cmd_BotMoveRight_f( gentity_t *ent ) {
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof(sarg) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, BOT_MOVE_ARG, -1 );
}

static void Cmd_BotMoveLeft_f( gentity_t *ent ) {
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof(sarg) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, -BOT_MOVE_ARG, -1 );
}

static void Cmd_BotMoveUp_f( gentity_t *ent ) {
	int bCl = 0;
	char sarg[MAX_STRING_CHARS];

	assert( trap->Argc() > 1 );
	trap->Argv( 1, sarg, sizeof(sarg) );

	assert( sarg[0] );
	bCl = atoi( sarg );
	Bot_SetForcedMovement( bCl, -1, -1, BOT_MOVE_ARG );
}

static void Cmd_Sabercolor_f( gentity_t *ent ) {
	int saberNum, red, green, blue, client = ent->client - level.clients;
	byte r = 0, g = 0, b = 0;
	const char *temp;
	char sNum[8] = {}, sRed[8] = {}, sGreen[8] = {}, sBlue[8] = {}, userinfo[MAX_INFO_STRING] = {};

	trap->GetUserinfo( client, userinfo, sizeof(userinfo) );
	temp = Info_ValueForKey( userinfo, "cp_sbRGB1" );
	b = (atoi(temp) >> 16) & 0xf;
	g = (atoi(temp) >> 8) & 0xf;
	r = atoi(temp) & 0xf;

	if (trap->Argc() < 5) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Usage: \\sabercolor " S_COLOR_WHITE
			"<" S_COLOR_YELLOW "1-2" S_COLOR_WHITE "> "
			"<" S_COLOR_RED "0-255" S_COLOR_WHITE "> "
			"<" S_COLOR_GREEN "0-255" S_COLOR_WHITE "> "
			"<" S_COLOR_CYAN "0-255" S_COLOR_WHITE ">\n\""
		);
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_WHITE "Current: Saber 1: "
			"<" S_COLOR_RED "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_GREEN "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_CYAN "%i" S_COLOR_WHITE ">\n\"", r, g, b )
		);
		temp = Info_ValueForKey( userinfo, "cp_sbRGB2" );
		b = (atoi(temp) >> 16) & 0xf;
		g = (atoi(temp) >> 8) & 0xf;
		r = atoi(temp) & 0xf;
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_WHITE "Current: Saber 2: "
			"<" S_COLOR_RED "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_GREEN "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_CYAN "%i" S_COLOR_WHITE ">\n\"", r, g, b )
		);

		return;
	}

	trap->Argv(1, sNum, sizeof(sNum));
	trap->Argv(2, sRed, sizeof(sRed));
	trap->Argv(3, sGreen, sizeof(sGreen));
	trap->Argv(4, sBlue, sizeof(sBlue));

	saberNum = Q_clampi(1, atoi(sNum), 2);
	red = Q_clampi(0, atoi(sRed), 255);
	green = Q_clampi(0, atoi(sGreen), 255);
	blue = Q_clampi(0, atoi(sBlue), 255);

	Info_SetValueForKey(userinfo, (saberNum == 1) ? "cp_sbRGB1" : "cp_sbRGB2", va("%i", red | ((green | (blue << 8)) << 8)));
	Info_SetValueForKey(userinfo, (saberNum == 1) ? "color1" : "color2", va("%i", SABER_RGB));
	trap->SetUserinfo(client, userinfo);
	ClientUserinfoChanged(client);

	return;
}

static void Cmd_Playertint_f( gentity_t *ent ) {
	int red, green, blue, client = ent->client - level.clients;
	byte r = 0, g = 0, b = 0;
	char sRed[8] = { 0 }, sGreen[8] = { 0 }, sBlue[8] = { 0 };
	char userinfo[MAX_INFO_STRING];
	trap->GetUserinfo(client, userinfo, sizeof(userinfo));
	r = ent->client->ps.customRGBA[0];
	g = ent->client->ps.customRGBA[1];
	b = ent->client->ps.customRGBA[2];

	if ( trap->Argc() < 4 ) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Usage: \\playertint " S_COLOR_WHITE
			"<" S_COLOR_YELLOW "1-2" S_COLOR_WHITE "> "
			"<" S_COLOR_RED "0-255" S_COLOR_WHITE "> "
			"<" S_COLOR_GREEN "0-255" S_COLOR_WHITE "> "
			"<" S_COLOR_CYAN "0-255" S_COLOR_WHITE ">\n\""
		);
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_WHITE "Current: "
			"<" S_COLOR_RED "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_GREEN "%i" S_COLOR_WHITE "> "
			"<" S_COLOR_CYAN "%i" S_COLOR_WHITE ">\n\"", r, g, b )
		);
		return;
	}

	trap->Argv(1, sRed, sizeof(sRed));
	trap->Argv(2, sGreen, sizeof(sGreen));
	trap->Argv(3, sBlue, sizeof(sBlue));

	red = Q_clampi(0, atoi(sRed), 255);
	green = Q_clampi(0, atoi(sGreen), 255);
	blue = Q_clampi(0, atoi(sBlue), 255);

	//TODO: force a lower bound
	ent->client->ps.customRGBA[0] = red;
	ent->client->ps.customRGBA[1] = green;
	ent->client->ps.customRGBA[2] = blue;
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
static void JP_FilterIdentifier( char *ident ) {
	char *s = ident, *out = ident, c = 0;

	Q_CleanString( ident, STRIP_COLOUR );
	while ( (c = *s++) != '\0' ) {
		if ( c < '$' || c > '}' || c == ';' )
			continue;
		*out++ = c;
	}
	*out = '\0';
}

static void JP_ListChannels( gentity_t *ent ) {
	qboolean legacyClient = !(ent->client->pers.CSF & CSF_CHAT_FILTERS);
	channel_t *channel = ent->client->pers.channels;
	char msg[960] = { 0 };

	while ( channel ) {
		if ( legacyClient ) {
			if ( channel == ent->client->pers.activeChannel ) {
				Q_strcat( msg, sizeof(msg), va( S_COLOR_WHITE "- " S_COLOR_YELLOW "%s " S_COLOR_WHITE "[" S_COLOR_GREEN
					"%s" S_COLOR_WHITE "]\n", channel->identifier, channel->shortname )
				);
			}
			else {
				Q_strcat( msg, sizeof(msg), va( S_COLOR_WHITE "- %s " S_COLOR_WHITE "[" S_COLOR_GREEN "%s"
					S_COLOR_WHITE "]\n", channel->identifier, channel->shortname )
				);
			}
		}
		else {
			Q_strcat( msg, sizeof(msg), va( S_COLOR_WHITE"- %s\n", channel->identifier ) );
		}
		channel = channel->next;
	}
	trap->SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

static void Cmd_JoinChannel_f( gentity_t *ent ) {
	qboolean legacyClient = !Client_Supports( ent, CSF_CHAT_FILTERS );
	char arg1_ident[32] = { 0 };
	char arg2_shortname[32] = { 0 };
	channel_t *channel = NULL, *prev = NULL;

	if ( trap->Argc() < 2 ) {
		if ( legacyClient ) {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW
				"Usage: \\joinchan <identifier/password> [short-name]>\n\""
			);
		}
		else {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW
				"Usage: \\joinchan <identifier/password>\n\""
			);
		}
		return;
	}

	trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
	if ( legacyClient )
		trap->Argv( 2, arg2_shortname, sizeof(arg2_shortname) );

	JP_FilterIdentifier( arg1_ident );

	// Try to find an existing channel
	channel = ent->client->pers.channels;
	while ( channel ) {
		if ( !strcmp( arg1_ident, channel->identifier ) ) {
			// Already joined, return
			trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_YELLOW "You are already in channel '%s'"
				"\n\"", arg1_ident )
			);
			//TODO: update short-name?
			//		for legacy clients
			return;
		}
		prev = channel;
		channel = channel->next;
	}

	//Not in this channel (or any channels) so allocate a new one
	channel = (channel_t *)malloc( sizeof(channel_t) );
	memset( channel, 0, sizeof(channel_t) );

	//attach to linked list of channels
	if ( prev )
		prev->next = channel;
	else
		ent->client->pers.channels = channel;

	Q_strncpyz( channel->identifier, arg1_ident, sizeof(channel->identifier) );
	if ( legacyClient )
		Q_strncpyz( channel->shortname, arg2_shortname, sizeof(channel->shortname) );

	//Notify about the successful join
	trap->SendServerCommand( ent - g_entities, legacyClient
		? va( "print \"Successfully joined channel '%s' (shortname: '%s')\n\"", arg1_ident, arg2_shortname )
		: va( "print \"Successfully joined channel '%s'\n\"", arg1_ident )
	);

	return;
}

static void Cmd_WhoisChannel_f( gentity_t *ent ) {
	qboolean legacyClient = !Client_Supports( ent, CSF_CHAT_FILTERS );
	char msg[960] = { 0 };

	// Supported client
	if ( trap->Argc() == 2 && !legacyClient ) {
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		channel_t *chan = NULL;
		qboolean in = qfalse;
		int i;

		trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
		JP_FilterIdentifier( arg1_ident );

		//Check if they're even in the channel
		for ( chan = ent->client->pers.channels; chan; chan = chan->next ) {
			if ( !strcmp( chan->identifier, arg1_ident ) ) {
				in = qtrue;
				break;
			}
		}
		if ( !in ) {
			trap->SendServerCommand( ent - g_entities, va( "print \"You are not in channel '%s'\n\"", arg1_ident, msg ) );
			return;
		}

		for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED ) {
				chan = other->client->pers.channels;
				while ( chan ) {
					if ( !strcmp( chan->identifier, arg1_ident ) )
						Q_strcat( msg, sizeof(msg), va( S_COLOR_WHITE "- %s\n", other->client->pers.netname ) );
					chan = chan->next;
				}
			}
		}
		trap->SendServerCommand( ent - g_entities, va( "print \"Players in channel '%s':\n%s\"", arg1_ident, msg ) );
	}
	else if ( legacyClient ) {
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		int i;

		trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
		JP_FilterIdentifier( arg1_ident );

		if ( !ent->client->pers.activeChannel ) {
			trap->SendServerCommand( ent - g_entities, "print \"You are not in any channels\n\"" );
			return;
		}

		for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED ) {
				channel_t *chan = other->client->pers.channels;
				while ( chan ) {
					if ( chan == ent->client->pers.activeChannel )
						Q_strcat( msg, sizeof(msg), va( S_COLOR_WHITE "- %s\n", other->client->pers.netname ) );
					chan = chan->next;
				}
			}
		}
		trap->SendServerCommand( ent - g_entities, va( "print \"Players in channel '%s':\n%s\"", arg1_ident, msg ) );
	}
	else
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Usage: \\whoischan <identifier/password>\n\"" );

	return;
}

static void Cmd_LeaveChannel_f( gentity_t *ent ) {
	qboolean legacyClient = !Client_Supports( ent, CSF_CHAT_FILTERS );

	if ( trap->Argc() == 2 && !legacyClient ) {
		channel_t *channel = ent->client->pers.channels;
		channel_t *prev = NULL;
		char arg1_ident[32] = { 0 };

		trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
		JP_FilterIdentifier( arg1_ident );

		while ( channel ) {
			if ( !strcmp( arg1_ident, channel->identifier ) ) {
				if ( prev )
					prev->next = channel->next;
				else
					ent->client->pers.channels = channel->next;
				free( channel );

				trap->SendServerCommand( ent - g_entities, va( "print \"Successfully left channel '%s'\n\"", arg1_ident ) );
				if ( ent->client->pers.channels ) {
					trap->SendServerCommand( ent - g_entities, "print \"You are currently in these channels:\n\"" );
					JP_ListChannels( ent );
				}
				return;
			}
			prev = channel;
			channel = channel->next;
		}
		trap->SendServerCommand( ent - g_entities, va( "print \"" S_COLOR_YELLOW "Error leaving channel '%s'. You "
			"were not in the channel.\n\"", arg1_ident )
		);
		if ( ent->client->pers.channels ) {
			trap->SendServerCommand( ent - g_entities, "print \"You are currently in these channels:\n\"" );
			JP_ListChannels( ent );
		}
	}
	else if ( legacyClient ) {
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

static void Cmd_MessageChannel_f( gentity_t *ent ) {
	qboolean legacyClient = !Client_Supports( ent, CSF_CHAT_FILTERS );
	char *msg = ConcatArgs( 2 );
	char name[MAX_STRING_CHARS] = { 0 };

	// Supported client
	if ( trap->Argc() >= 3 && !legacyClient ) {
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		channel_t *chan = NULL;
		qboolean in = qfalse;
		int i;

		trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
		JP_FilterIdentifier( arg1_ident );

		//Check if they're even in the channel
		for ( chan = ent->client->pers.channels; chan; chan = chan->next ) {
			if ( !strcmp( arg1_ident, chan->identifier ) ) {
				in = qtrue;
				break;
			}
		}
		if ( !in ) {
			trap->SendServerCommand( ent - g_entities, va( "print \"You are not in channel '%s'\n\"", arg1_ident, msg ) );
			return;
		}

		Com_sprintf( name, sizeof(name), "%s" S_COLOR_WHITE CHANNEL_EC "%s" CHANNEL_EC ": ", ent->client->pers.netname,
			chan->identifier
		);

		for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED ) {
				chan = other->client->pers.channels;
				while ( chan ) {
					if ( !strcmp( chan->identifier, arg1_ident ) )
						G_SayTo( ent, other, SAY_ALL, COLOR_RED, name, msg, NULL );
					chan = chan->next;
				}
			}
		}
	}
	else if ( legacyClient ) {
		char arg1_ident[32] = { 0 };
		gentity_t *other = NULL;
		int i;

		trap->Argv( 1, arg1_ident, sizeof(arg1_ident) );
		JP_FilterIdentifier( arg1_ident );

		if ( !ent->client->pers.activeChannel ) {
			trap->SendServerCommand( ent - g_entities, "print \"You are not in any channels\n\"" );
			return;
		}

		Com_sprintf( name, sizeof(name), "%s" S_COLOR_WHITE CHANNEL_EC "%s" CHANNEL_EC ": ", ent->client->pers.netname,
			ent->client->pers.activeChannel->identifier
		);

		for ( i = 0, other = g_entities; i < MAX_CLIENTS; i++, other++ ) {
			if ( other->inuse && other->client && other->client->pers.connected == CON_CONNECTED ) {
				channel_t *chan = other->client->pers.channels;
				while ( chan ) {
					if ( chan == ent->client->pers.activeChannel )
						G_SayTo( ent, other, SAY_ALL, COLOR_RED, name, msg, NULL );
					chan = chan->next;
				}
			}
		}
	}
	else {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW
			"Usage: \\msgchan <identifier/password> <message>\n\""
		);
	}

	return;
}

static void Cmd_Drop_f( gentity_t *ent ) {
	char arg[128] = { 0 };

	if ( ent->client->ps.pm_type == PM_DEAD || ent->client->ps.pm_type == PM_SPECTATOR ) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "You must be alive to drop items\n\"" );
		return;
	}

	trap->Argv( 1, arg, sizeof(arg) );

	if ( !Q_stricmp( arg, "flag" ) ) {
		powerup_t powerup = (ent->client->ps.persistant[PERS_TEAM] == TEAM_RED) ? PW_BLUEFLAG : PW_REDFLAG;

		if ( !japp_allowFlagDrop.integer ) {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Not allowed to drop the flag\n\"" );
			return;
		}

		if ( level.gametype != GT_CTF || (ent->client->ps.powerups[powerup] <= level.time) ) {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "No flag to drop\n\"" );
			return;
		}

		if ( ent->client->ps.powerups[powerup] > level.time ) {
			const gitem_t *item = BG_FindItemForPowerup( powerup );
			gentity_t *drop = NULL;
			vector3 angs = { 0.0f, 0.0f, 0.0f };

			AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

			drop = Drop_Item( ent, item, angs.yaw );
			//Raz: speed caps
			drop->genericValue1 = trap->Milliseconds() - ent->client->pers.teamState.flagsince;
			//	drop->genericValue2 = ent-g_entities;
			//	drop->genericValue2 |= (1<<5); // 6th bit indicates a client dropped it on purpose
			//	drop->genericValue2 |= level.time << 6;

			ent->client->ps.powerups[powerup] = 0;
		}
	}
	else if ( !Q_stricmp( arg, "weapon" ) ) {
		weapon_t wp = (weapon_t)ent->client->ps.weapon, newWeap = WP_NONE;
		const gitem_t *item = NULL;
		gentity_t *drop = NULL;
		vector3 angs = { 0.0f, 0.0f, 0.0f };
		int ammo, i = 0;

		if ( !japp_allowWeaponDrop.integer ) {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Not allowed to drop weapons\n\"" );
			return;
		}

		if ( !(ent->client->ps.stats[STAT_WEAPONS] & (1 << wp)) || !ent->client->ps.ammo[weaponData[wp].ammoIndex]
			|| wp == WP_SABER || wp == WP_MELEE || wp == WP_EMPLACED_GUN || wp == WP_TURRET
			|| wp <= WP_NONE || wp > WP_NUM_WEAPONS )
		{
			// we don't have this weapon or ammo for it, or it's a 'weapon' that can't be dropped
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Can't drop this weapon\n\"" );
			return;
		}

		item = BG_FindItemForWeapon( wp );

		if ( !ent->client->ps.ammo[weaponData[wp].ammoIndex] ) {
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "No ammo for this weapon\n\"" );
			return;
		}
		//FIXME: use bg_itemlist[BG_GetItemIndexByTag( wp, IT_WEAPON )].quantity?
		else if ( ent->client->ps.ammo[weaponData[wp].ammoIndex] > item->quantity ) {
			ammo = item->quantity;
		}
		else {
			ammo = ent->client->ps.ammo[weaponData[wp].ammoIndex];
		}

		AngleVectors( &ent->client->ps.viewangles, &angs, NULL, NULL );

		drop = Drop_Item( ent, item, angs.yaw );
		drop->count = ammo;
		ent->client->ps.ammo[weaponData[wp].ammoIndex] -= ammo;
		ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << wp);

		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			if ( (ent->client->ps.stats[STAT_WEAPONS] & (1 << i)) && i != WP_NONE ) {
				// this one's good
				newWeap = (weapon_t)i;
				break;
			}
		}

		if ( newWeap != WP_NONE ) {
			ent->s.weapon = newWeap;
			ent->client->ps.weapon = newWeap;
		}
		else {
			ent->s.weapon = 0;
			ent->client->ps.weapon = 0;
		}

		G_AddEvent( ent, EV_NOAMMO, wp );

		drop->genericValue2 = ent - g_entities;
		drop->genericValue2 |= (1 << 5); // 6th bit indicates a client dropped it on purpose
		drop->genericValue2 |= level.time << 6;
	}
	else if ( !Q_stricmp( arg, "powerup" ) ) {
		gentity_t *drop = NULL;
		vector3 angs = { 0.0f, 0.0f, 0.0f };
		powerup_t powerup = PW_NONE;
		const gitem_t *item = NULL;

		if (ent->client->ps.powerups[PW_FORCE_ENLIGHTENED_DARK] >= level.time && ent->client->pers.adminData.logineffect != PW_FORCE_ENLIGHTENED_DARK) {
			powerup = PW_FORCE_ENLIGHTENED_DARK;
		}
		else if (ent->client->ps.powerups[PW_FORCE_ENLIGHTENED_LIGHT] >= level.time && ent->client->pers.adminData.logineffect != PW_FORCE_ENLIGHTENED_LIGHT){
			powerup = PW_FORCE_ENLIGHTENED_LIGHT;
		}
		else if (ent->client->ps.powerups[PW_FORCE_BOON] >= level.time && ent->client->pers.adminData.logineffect != PW_FORCE_BOON){
			powerup = PW_FORCE_BOON;
		}
		else{
			return;
		}

		item = BG_FindItemForPowerup(powerup);
		AngleVectors(&ent->client->ps.viewangles, &angs, NULL, NULL);

		drop = Drop_Item(ent, item, angs.yaw);
		drop->count = (ent->client->ps.powerups[powerup] - level.time) / 1000;
		if (drop->count < 1) {
			drop->count = 1;
		}
		ent->client->ps.powerups[powerup] = 0;
		return;
	}
}

#define EXTINFO_SABER	(0x0001u)
#define EXTINFO_CMDS	(0x0002u)
#define EXTINFO_CLIENT	(0x0004u)
#define EXTINFO_ALL		(0x0007u)

static struct amInfoSetting_s {
	const char *str;
	uint32_t bit;
} aminfoSettings[] = {
	{ "all", EXTINFO_ALL },
	{ "saber", EXTINFO_SABER },
	{ "cmds", EXTINFO_CMDS },
	{ "client", EXTINFO_CLIENT },
};
static const size_t numAminfoSettings = ARRAY_LEN( aminfoSettings );

static void PB_Callback( const char *buffer, int clientNum ) {
	trap->SendServerCommand( clientNum, va( "print \"%s\"", buffer ) );
}

static void Cmd_AMInfo_f( gentity_t *ent ) {
	int extendedInfo = 0;
	printBufferSession_t pb;

	Q_NewPrintBuffer( &pb, MAX_STRING_CHARS / 2, PB_Callback, ent - g_entities );

	Q_PrintBuffer( &pb, S_COLOR_YELLOW "================================================================"
		S_COLOR_WHITE "\n\n" );

	if ( trap->Argc() < 2 ) {
		unsigned int i = 0;
		Q_PrintBuffer( &pb, "Try 'aminfo <category>' for more detailed information\nCategories: " );

		// print all categories
		for ( i = 0; i < numAminfoSettings; i++ ) {
			if ( i ) {
				Q_PrintBuffer( &pb, ", " );
			}
			Q_PrintBuffer( &pb, aminfoSettings[i].str );
		}

		Q_PrintBuffer( &pb, "\n\n" );
	}
	else {
		char arg[8] = { 0 };
		unsigned int i = 0;

		trap->Argv( 1, arg, sizeof(arg) );

		// find out which category they want
		for ( i = 0; i < numAminfoSettings; i++ ) {
			if ( !Q_stricmp( arg, aminfoSettings[i].str ) ) {
				extendedInfo = aminfoSettings[i].bit;
				break;
			}
		}
	}

	// mod version + compile date
	if ( !extendedInfo || extendedInfo == EXTINFO_ALL ) {
		char version[256] = { 0 };
		trap->Cvar_VariableStringBuffer( "version", version, sizeof(version) );
		Q_PrintBuffer( &pb, "Version:\n    Gamecode: " JAPP_VERSION "\n" );
#ifdef _DEBUG
		Q_PrintBuffer( &pb, "Debug build\n" );
#endif
		Q_PrintBuffer( &pb, va( "    Engine: %s\n\n", version ) );
	}

	if ( extendedInfo & EXTINFO_SABER ) {
		// saber settings
		Q_PrintBuffer( &pb, "Saber settings:\n" );

		// SP/MP
		Q_PrintBuffer( &pb, va( "  %s" S_COLOR_WHITE "style\n",
			d_saberSPStyleDamage.integer ? S_COLOR_GREEN "SP " : S_COLOR_RED "MP ") );

		// JA++ tweaks
		if ( japp_saberTweaks.integer ) {
			const uint32_t tweaks = japp_saberTweaks.integer;
			Q_PrintBuffer( &pb, "  JA++ tweaks:\n" );

			Q_PrintBuffer( &pb, va( "    Interpolation %s" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_INTERPOLATE) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled" ) );
			Q_PrintBuffer( &pb, va( "    Prolonged swing damage %s" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_PROLONGDAMAGE) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled" ) );
			Q_PrintBuffer( &pb, va( "    Deflection %s" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_POSDEFLECTION) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled" ) );
			Q_PrintBuffer( &pb, va( "    Special moves %s" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_SPECIALMOVES) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled" ) );
			Q_PrintBuffer( &pb, va( "    Trace size %s" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_TRACESIZE) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled") );
			Q_PrintBuffer( &pb, va( "    Reduce blocks %s " S_COLOR_WHITE "(%s%.02f " S_COLOR_WHITE "- %s%.02f"
				S_COLOR_WHITE ") * %s%.02f" S_COLOR_WHITE "\n",
				(tweaks & SABERTWEAK_REDUCEBLOCKS) ? S_COLOR_GREEN"enabled" : S_COLOR_RED"disabled",
				(japp_saberBlockChanceMin.value != atoff( G_Cvar_DefaultString( &japp_saberBlockChanceMin ) ))
				? S_COLOR_RED : S_COLOR_GREEN, japp_saberBlockChanceMin.value,
				(japp_saberBlockChanceMax.value != atoff( G_Cvar_DefaultString( &japp_saberBlockChanceMax ) ))
				? S_COLOR_RED : S_COLOR_GREEN, japp_saberBlockChanceMax.value,
				(japp_saberBlockChanceScale.value != atoff( G_Cvar_DefaultString( &japp_saberBlockChanceScale ) ))
				? S_COLOR_RED : S_COLOR_GREEN, japp_saberBlockChanceScale.value ) );

#ifdef _DEBUG
			if ( tweaks & SABERTWEAK_REDUCEBLOCKS ) {
				int ourLevel, theirLevel;
				for ( ourLevel = 1; ourLevel <= 3; ourLevel++ ) {
					for ( theirLevel = 1; theirLevel <= 3; theirLevel++ ) {
						const float diff = (float)(theirLevel - ourLevel); // range [0, 2]
						const float parity = japp_saberBlockStanceParity.value; // range [0, 3]
						const float chanceMin = japp_saberBlockChanceMin.value;
						const float chanceMax = japp_saberBlockChanceMax.value;
						const float chanceScalar = japp_saberBlockChanceScale.value;
						const float chance = Q_clamp( chanceMin, (1.0f - (diff / parity)) * chanceScalar, chanceMax );
						Q_PrintBuffer( &pb, va( "      %i blocking %i: %.03f\n", ourLevel, theirLevel, chance ) );
					}
				}
			}
#endif // _DEBUG

			Q_PrintBuffer( &pb, va( "      %s%.03f " S_COLOR_WHITE "stance parity\n",
				(japp_saberBlockStanceParity.value != atoff( G_Cvar_DefaultString( &japp_saberBlockStanceParity ) ))
				? S_COLOR_RED : S_COLOR_GREEN, japp_saberBlockStanceParity.value ) );

			Q_PrintBuffer( &pb, S_COLOR_WHITE "\n" );
		}

		// damage scale
		Q_PrintBuffer( &pb, va( "  %s%.03f " S_COLOR_WHITE "damage scale\n",
			(g_saberDamageScale.value != atoff( G_Cvar_DefaultString( &g_saberDamageScale ) ))
			? S_COLOR_RED : S_COLOR_GREEN, g_saberDamageScale.value ) );

		// idle damage
		Q_PrintBuffer( &pb, va( "  " S_COLOR_WHITE "Idle damage %s" S_COLOR_WHITE "\n",
			(japp_saberIdleDamage.integer != atoff( G_Cvar_DefaultString( &japp_saberIdleDamage ) ))
			? S_COLOR_RED : S_COLOR_GREEN,
			japp_saberIdleDamage.integer ? "enabled" : "disabled" ) );

		Q_PrintBuffer( &pb, "\n" );
	}

	if ( !extendedInfo || (extendedInfo & EXTINFO_CMDS) ) {
		// admin commands
		AM_PrintCommands( ent, &pb );

		// regular commands
		G_PrintCommands( ent, &pb );
	}

	if ( extendedInfo & EXTINFO_CLIENT ) {
		// client support flags
		int i = 0;

		Q_PrintBuffer( &pb, va( "Client support flags: 0x%X\n", ent->client->pers.CSF ) );
		for ( i = 0; i < CSF_NUM; i++ ) {
			if ( ent->client->pers.CSF & (1 << i) ) {
				Q_PrintBuffer( &pb, " [X] " );
			}
			else {
				Q_PrintBuffer( &pb, " [ ] " );
			}
			Q_PrintBuffer( &pb, va( "%s\n", supportFlagNames[i] ) );
		}

		//RAZTODO: cp_pluginDisable?
		Q_PrintBuffer( &pb, va( "Client plugin disabled: %i\n", ent->client->pers.CPD ) );
		for ( i = 0; i < CPD_NUM; i++ ) {
			if ( ent->client->pers.CPD & (1 << i) ) {
				Q_PrintBuffer( &pb, " [X] " );
			}
			else {
				Q_PrintBuffer( &pb, " [ ] " );
			}
			Q_PrintBuffer( &pb, va( "%s\n", clientPluginDisableNames[i] ) );
		}
	}

	Q_PrintBuffer( &pb, S_COLOR_YELLOW "================================================================\n" );

	Q_DeletePrintBuffer( &pb );
}

static void Cmd_Ready_f( gentity_t *ent ) {
	const char *publicMsg = NULL;
	gentity_t *e = NULL;
	int i = 0;

	if ( !g_doWarmup.integer || level.warmupTime == 0 || level.restarted || level.allReady ) {
		return;
	}

	ent->client->pers.ready = !ent->client->pers.ready;

	if ( ent->client->pers.ready ) {
		publicMsg = va( "cp \"%s\n" S_COLOR_WHITE "is ready\"", ent->client->pers.netname );
		trap->SendServerCommand( ent - g_entities, va( "cp \"" S_COLOR_GREEN "You are ready\"" ) );
	}
	else {
		publicMsg = va( "cp \"%s\n" S_COLOR_YELLOW "is NOT ready\"", ent->client->pers.netname );
		trap->SendServerCommand( ent - g_entities, va( "cp \"" S_COLOR_YELLOW "You are NOT ready\"" ) );
	}

	// send public message to everyone BUT this client, so they see their own message
	for ( i = 0, e = g_entities; i < level.maxclients; i++, e++ ) {
		if ( e != ent ) {
			trap->SendServerCommand( e - g_entities, publicMsg );
		}
	}
}

static void Cmd_Origin_f( gentity_t *ent ) {
	char		arg1[64];
	int			targetClient;
	gentity_t	*targ;

	//Self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof(arg1) );
	targetClient = (trap->Argc() > 1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR | FINDCL_PRINT ) : ent - g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	// can't see ghosted clients, let's just send their own coords =]
	if ( targ->client->pers.adminData.isGhost && !AM_HasPrivilege( ent, PRIV_GHOST ) ) {
		targ = ent;
	}

	trap->SendServerCommand( ent - g_entities, va( "print \"Origin: %s\nAngles: %s\n\"", vtos( &targ->client->ps.origin ),
		vtos( &targ->client->ps.viewangles ) ) );
}

static void G_ResetSaberStyle( gentity_t *ent ) {
	// dual
	if ( ent->client->saber[0].model[0] && ent->client->saber[1].model[0] )
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_DUAL;
	// staff
	else if ( (ent->client->saber[0].saberFlags&SFL_TWO_HANDED) )
		ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = SS_STAFF;
	else {
		ent->client->sess.saberLevel = Q_clampi( SS_FAST, ent->client->sess.saberLevel, SS_STRONG );
		ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel;

		// limit our saber style to our force points allocated to saber offense
		if ( level.gametype != GT_SIEGE && ent->client->ps.fd.saberAnimLevel > ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
			ent->client->ps.fd.saberAnimLevelBase = ent->client->ps.fd.saberAnimLevel = ent->client->ps.fd.saberDrawAnimLevel = ent->client->sess.saberLevel = ent->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	// let's just make sure the styles we chose are cool
	if ( level.gametype != GT_SIEGE ) {
		if ( !WP_SaberStyleValidForSaber( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, ent->client->ps.fd.saberAnimLevel ) ) {
			WP_UseFirstValidSaberStyle( &ent->client->saber[0], &ent->client->saber[1], ent->client->ps.saberHolstered, &ent->client->ps.fd.saberAnimLevel );
			ent->client->ps.fd.saberAnimLevelBase = ent->client->saberCycleQueue = ent->client->ps.fd.saberAnimLevel;
		}
	}
}

static void Cmd_Saber_f( gentity_t *ent ) {
	int argc = trap->Argc();
	int numSabers = 1;

	if ( Q_stricmp( ent->client->pers.saber2, "none" ) ) {
		numSabers++;
	}

	if ( argc == 1 ) {
		if ( numSabers == 1 ) {
			trap->SendServerCommand( ent - g_entities, va( "print \"Saber is %s\n\"", ent->client->pers.saber1 ) );
		}
		else {
			trap->SendServerCommand( ent - g_entities, va( "print \"Sabers are %s and %s\n\"", ent->client->pers.saber1,
				ent->client->pers.saber2 ) );
		}
	}
	else if ( !(japp_allowSaberSwitch.bits & (1 << level.gametype)) ) {
		trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Not allowed to change saber\n\"" );
		return;
	}
	else {
		char saber1[MAX_TOKEN_CHARS], saber2[MAX_TOKEN_CHARS], userinfo[MAX_INFO_STRING];
		const char *saber, *key, *value;
		saberInfo_t oldSabers[MAX_SABERS];
		int i;
		qboolean valid = qtrue;

		// busy
		if ( ent->client->ps.weaponTime > 0
			|| ent->client->ps.saberMove > LS_READY
			|| ent->client->ps.fd.forcePowersActive
		//	|| ent->client->ps.groundEntityNum == ENTITYNUM_NONE
			|| ent->client->ps.duelInProgress
			|| BG_InKnockDown( ent->client->ps.legsAnim )
			|| BG_InRoll( &ent->client->ps, ent->client->ps.legsAnim ) )
		{
			trap->SendServerCommand( ent - g_entities, "print \"" S_COLOR_YELLOW "Not allowed to change saber when busy"
				"\n\"" );
			return;
		}

		//HACK: should really find a way around this :D
		if ( japp_antiUserinfoFlood.integer && ent->userinfoSpam >= 12 ) {
			return;
		}

		// first saber
		trap->Argv( 1, saber1, sizeof(saber1) );

		// second saber if specified
		if ( argc == 3 ) {
			trap->Argv( 2, saber2, sizeof(saber2) );
		}
		else {
			Q_strncpyz( saber2, "none", sizeof(saber2) );
		}

		memcpy( oldSabers, ent->client->saber, sizeof(saberInfo_t) * MAX_SABERS );
		if ( !G_SetSaber( ent, 0, saber1, qfalse ) || !G_SetSaber( ent, 1, saber2, qfalse )
			|| !ClientUserinfoChanged( ent - g_entities ) )
		{
			memcpy( ent->client->saber, oldSabers, sizeof(saberInfo_t) * MAX_SABERS );
			G_SetSaber( ent, 0, oldSabers[0].name, qfalse );
			G_SetSaber( ent, 1, oldSabers[1].name, qfalse );
			valid = qfalse;
		}

		// make sure the saber models are updated
		G_SaberModelSetup( ent );

		if ( valid ) {
			trap->GetUserinfo( ent - g_entities, userinfo, sizeof(userinfo) );
			for ( i = 0; i < MAX_SABERS; i++ ) {
				saber = (i & 1) ? ent->client->pers.saber2 : ent->client->pers.saber1;
				key = va( "saber%d", i + 1 );
				value = Info_ValueForKey( userinfo, key );
				if ( Q_stricmp( value, saber ) ) {
					// they don't match up, force the user info
					Info_SetValueForKey( userinfo, key, saber );
					trap->SetUserinfo( ent - g_entities, userinfo );
				}
			}
		}

		G_ResetSaberStyle( ent );
	}
}

// must be in alphabetical order
static const emote_t emotes[] = {
	{ "aimgun", BOTH_STAND5TOAIM, MAX_ANIMATIONS, EMF_HOLD | EMF_HOLSTER },
	{ "amhiltthrow1", BOTH_SABERTHROW1START, BOTH_SABERTHROW1STOP, EMF_NONE },
	{ "amhiltthrow2", BOTH_SABERTHROW2START, BOTH_SABERTHROW2STOP, EMF_NONE },
	{ "atease", BOTH_STAND4, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "beg", BOTH_KNEES2, MAX_ANIMATIONS, EMF_HOLD | EMF_HOLSTER },
	{ "breakdance", BOTH_FORCE_GETUP_B4, MAX_ANIMATIONS, EMF_NONE },
	{ "breakdance2", BOTH_FORCE_GETUP_B6, MAX_ANIMATIONS, EMF_NONE },
	{ "cower", BOTH_COWER1_START, BOTH_COWER1, EMF_HOLSTER },
	{ "dance1", BOTH_TURNSTAND1, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD },
	{ "dance2", BOTH_TURNSTAND4, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "dance3", BOTH_TURNSTAND5, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD },
	{ "die", BOTH_HIT1, BOTH_FORCE_GETUP_F2, EMF_STATIC | EMF_HOLD },
	{ "die2", BOTH_DEATH15, BOTH_FORCE_GETUP_F2, EMF_STATIC | EMF_HOLD },
	{ "fabulous", BOTH_K7_S7_TR, MAX_ANIMATIONS, EMF_HOLD },
	{ "finishinghim", BOTH_SABERKILLER1, MAX_ANIMATIONS, EMF_STATIC },
	{ "harlem", BOTH_FORCE_DRAIN_GRABBED, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD },
	{ "heal", BOTH_FORCEHEAL_START, BOTH_FORCEHEAL_STOP, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "hello", BOTH_SILENCEGESTURE1, MAX_ANIMATIONS, EMF_NONE },
	{ "hips", BOTH_STAND5TOSTAND8, BOTH_STAND8TOSTAND5, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "kneel", BOTH_CROUCH3, BOTH_UNCROUCH3, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "kneel2", BOTH_ROSH_PAIN, BOTH_UNCROUCH3, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "neo", BOTH_FORCE_GETUP_B4, MAX_ANIMATIONS, EMF_NONE },
	{ "nod", BOTH_HEADNOD, MAX_ANIMATIONS, EMF_NONE },
	{ "noisy", BOTH_SONICPAIN_HOLD, BOTH_SONICPAIN_END, EMF_HOLD },
	{ "power", BOTH_FORCE_GETUP_F2, MAX_ANIMATIONS, EMF_NONE },
	{ "radio", BOTH_TALKCOMM1START, BOTH_TALKCOMM1STOP, EMF_HOLD | EMF_HOLSTER },
	{ "shake", BOTH_HEADSHAKE, MAX_ANIMATIONS, EMF_NONE },
	{ "shovel", BOTH_TUSKENATTACK2, MAX_ANIMATIONS, EMF_NONE },
	{ "sit1", BOTH_SIT1, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD },
	{ "sit2", BOTH_SIT2, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "sit3", BOTH_SIT3, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "sit4", BOTH_SIT4, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "sit6", BOTH_SIT6, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "sleep", BOTH_SLEEP1, BOTH_SLEEP1GETUP, EMF_HOLD | EMF_HOLSTER | EMF_STATIC },
	{ "smack1", BOTH_FORCEGRIP3THROW, MAX_ANIMATIONS, EMF_NONE },
	{ "smack2", BOTH_TOSS1, MAX_ANIMATIONS, EMF_NONE },
	{ "stand", BOTH_STAND8, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLD },
	{ "stepback", BOTH_FORCE_2HANDEDLIGHTNING, MAX_ANIMATIONS, EMF_NONE },
	{ "suggest", BOTH_STAND1_TALK3, MAX_ANIMATIONS, EMF_NONE },
	{ "surrender", TORSO_SURRENDER_START, TORSO_SURRENDER_STOP, EMF_HOLD | EMF_HOLSTER },
	{ "victory", BOTH_TAVION_SCEPTERGROUND, MAX_ANIMATIONS, EMF_NONE },
	{ "wait", BOTH_STAND10, BOTH_STAND10TOSTAND1, EMF_STATIC | EMF_HOLD | EMF_HOLSTER },
	{ "won", TORSO_HANDSIGNAL1, MAX_ANIMATIONS, EMF_NONE },
};
static const size_t numEmotes = ARRAY_LEN( emotes );

static int emotecmp( const void *a, const void *b ) {
	return strcmp( (const char *)a, ((const emote_t *)b)->name );
}

qboolean SetEmote( gentity_t *ent, const emote_t *emote ) {
	forceHandAnims_t handExtend = HANDEXTEND_TAUNT;
	int emoteTime;

	if ( !(japp_allowEmotes.bits & (1 << level.gametype)) ) {
		trap->SendServerCommand( ent - g_entities, "print \"Emotes are not allowed in this gametype\n\"" );
		return qfalse;
	}

	// busy
	if ( ent->client->ps.weaponTime > 0 || ent->client->ps.saberMove > LS_READY || ent->client->ps.fd.forcePowersActive
		|| ent->client->ps.groundEntityNum == ENTITYNUM_NONE || ent->client->ps.duelInProgress
		|| BG_InKnockDown( ent->client->ps.legsAnim ) || BG_InRoll( &ent->client->ps, ent->client->ps.legsAnim )
		|| ent->client->ps.forceHandExtend != HANDEXTEND_NONE || ent->client->emote.freeze )
	{
		return qfalse;
	}

	if ( emote->flags & EMF_STATIC ) {
		// emotes that require you to be standing still
		VectorClear( &ent->client->ps.velocity );
		handExtend = HANDEXTEND_DODGE;
	}
	if ( emote->flags & EMF_HOLD ) {
		// hold animation on torso
		emoteTime = INT32_MAX;
	}
	else {
		// once off actions
		emoteTime = level.time + BG_AnimLength( ent->localAnimIndex, emote->animLoop );
	}

	// holster saber if necessary
	if ( (emote->flags & EMF_HOLSTER) && ent->client->ps.weapon == WP_SABER && ent->client->ps.saberHolstered < 2 ) {
		ent->client->ps.saberCanThrow = qfalse;
		ent->client->ps.forceRestricted = qtrue;
		ent->client->ps.saberMove = LS_NONE;
		ent->client->ps.saberBlocked = 0;
		ent->client->ps.saberBlocking = 0;
		ent->client->ps.saberHolstered = 2;
		if ( ent->client->saber[0].soundOff ) {
			G_Sound( ent, CHAN_AUTO, ent->client->saber[0].soundOff );
		}
		if ( ent->client->saber[1].model[0] && ent->client->saber[1].soundOff ) {
			G_Sound( ent, CHAN_AUTO, ent->client->saber[1].soundOff );
		}
	}

	ent->client->ps.forceHandExtend = handExtend;
	ent->client->ps.forceHandExtendTime = emoteTime;
	ent->client->ps.forceDodgeAnim = emote->animLoop;
	ent->client->emote.nextAnim = emote->animLeave;
	ent->client->emote.freeze = qtrue;

	return qtrue;
}

static void RegularEmote( gentity_t *ent, const char *emoteName ) {
	const emote_t *emote = (emote_t *)bsearch( emoteName, emotes, numEmotes, sizeof(emote_t), emotecmp );
	if ( !emote ) {
		assert( !"Emote not found" );
		return;
	}
	SetEmote( ent, emote );
}

#define EMOTE( x ) static void Cmd_Emote_##x( gentity_t *ent ) { RegularEmote( ent, XSTRING(x) ); }
EMOTE( aimgun )
EMOTE( amhiltthrow1 )
EMOTE( amhiltthrow2 )
EMOTE( atease )
EMOTE( beg )
EMOTE( breakdance )
EMOTE( breakdance2 )
EMOTE( cower )
EMOTE( dance1 )
EMOTE( dance2 )
EMOTE( dance3 )
EMOTE( die1 )
EMOTE( die2 )
EMOTE( fabulous )
EMOTE( finishinghim )
EMOTE( harlem )
EMOTE( heal )
EMOTE( hello )
EMOTE( hips )
EMOTE( kneel )
EMOTE( kneel2 )
EMOTE( neo )
EMOTE( nod )
EMOTE( noisy )
EMOTE( power )
EMOTE( radio )
EMOTE( shake )
EMOTE( shovel )
EMOTE( sit1 )
EMOTE( sit2 )
EMOTE( sit3 )
EMOTE( sit4 )
EMOTE( sit6 )
EMOTE( sleep )
EMOTE( smack1 )
EMOTE( smack2 )
EMOTE( stand )
EMOTE( stepback )
EMOTE( suggest )
EMOTE( victory )
EMOTE( surrender )
EMOTE( wait )
EMOTE( won )

static void Cmd_Emote_Knockdown(gentity_t *ent){
	G_Knockdown(ent);
}

extern void MakeDeadSaber(gentity_t *ent);
static void Cmd_Emote_Dropsaber(gentity_t *ent){
	gentity_t *saber = &g_entities[ent->client->ps.saberEntityNum];
	if (saber){
		MakeDeadSaber(saber);
	}
}

static void Cmd_Emote_hug( gentity_t *ent ) {
	static const emote_t emoteHugger = { "hugger", BOTH_HUGGER1, BOTH_HUGGERSTOP1, EMF_HOLSTER },
		emoteHuggee = { "huggee", BOTH_HUGGEE1, BOTH_HUGGEESTOP1, EMF_HOLSTER };
	trace_t *tr = G_RealTrace( ent, 40.0f );
	if ( tr->fraction < 1.0f && tr->entityNum < MAX_CLIENTS ) {
		gentity_t *other = g_entities + tr->entityNum;

		if ( Client_Disabled( other, CPD_ANNOYINGEMOTES ) ) {
			return;
		}

		if ( SetEmote( ent, &emoteHugger ) && SetEmote( other, &emoteHuggee ) ) {
			vector3 entDir, otherDir, entAngles, otherAngles;

			VectorSubtract( &other->client->ps.origin, &ent->client->ps.origin, &otherDir );
			VectorCopy( &ent->client->ps.viewangles, &entAngles );
			entAngles.yaw = vectoyaw( &otherDir );
			SetClientViewAngle( ent, &entAngles );

			VectorSubtract( &ent->client->ps.origin, &other->client->ps.origin, &entDir );
			VectorCopy( &other->client->ps.viewangles, &otherAngles );
			otherAngles.yaw = vectoyaw( &entDir );
			SetClientViewAngle( other, &otherAngles );
		}
	}
}

static void Cmd_Emote_kiss( gentity_t *ent ) {
	static const emote_t emoteKisser = { "kisser", BOTH_KISSER, BOTH_KISSER1STOP, EMF_STATIC | EMF_HOLSTER },
		emoteKissee = { "kissee", BOTH_KISSEE, MAX_ANIMATIONS, EMF_STATIC | EMF_HOLSTER };
	trace_t *tr = G_RealTrace( ent, 40.0f );
	if ( tr->fraction < 1.0f && tr->entityNum < MAX_CLIENTS ) {
		gentity_t *other = g_entities + tr->entityNum;

		if ( Client_Disabled( other, CPD_ANNOYINGEMOTES ) ) {
			return;
		}

		vector3 entDir, otherDir, entAngles, otherAngles;

		VectorSubtract( &other->client->ps.origin, &ent->client->ps.origin, &otherDir );
		VectorCopy( &ent->client->ps.viewangles, &entAngles );
		entAngles.yaw = vectoyaw( &otherDir );
		SetClientViewAngle( ent, &entAngles );
		SetEmote( ent, &emoteKisser );

		VectorSubtract( &ent->client->ps.origin, &other->client->ps.origin, &entDir );
		VectorCopy( &other->client->ps.viewangles, &otherAngles );
		otherAngles.yaw = vectoyaw( &entDir );
		SetClientViewAngle( other, &otherAngles );
		SetEmote( other, &emoteKissee );
	}
}

static void Cmd_Jetpack_f( gentity_t *ent ) {
	const gitem_t *item = BG_FindItemForHoldable( HI_JETPACK );

	if ( ent->client->ps.duelInProgress || !(japp_allowJetpack.bits & (1 << level.gametype)) ) {
		return;
	}

	if ( ent->client->jetPackOn ) {
		Jetpack_Off( ent );
	}

	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] ^= (1 << item->giTag);
}

#define CMDFLAG_NOINTERMISSION	(0x0001u)
#define CMDFLAG_CHEAT			(0x0002u)
#define CMDFLAG_ALIVE			(0x0004u)

#define CMDFAIL_NOINTERMISSION	(0x0001u)
#define CMDFAIL_CHEAT			(0x0002u)
#define CMDFAIL_ALIVE			(0x0004u)
#define CMDFAIL_GAMETYPE		(0x0008u)

typedef struct command_s {
	const char	*name;
	void( *func )(gentity_t *ent);
	uint32_t	validGT; // bit-flag of valid gametypes
	uint32_t	flags;
} command_t;

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((command_t*)b)->name );
}

static const command_t commands[] = {
	{ "amaimgun", Cmd_Emote_aimgun, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amhiltthrow1", Cmd_Emote_amhiltthrow1, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amhiltthrow2", Cmd_Emote_amhiltthrow2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amatease", Cmd_Emote_atease, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "ambeg", Cmd_Emote_beg, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "ambreakdance", Cmd_Emote_breakdance, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "ambreakdance2", Cmd_Emote_breakdance2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amcower", Cmd_Emote_cower, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdance1", Cmd_Emote_dance1, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdance2", Cmd_Emote_dance2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdance3", Cmd_Emote_dance3, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdie", Cmd_Emote_die1, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdie2", Cmd_Emote_die2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amdropsaber", Cmd_Emote_Dropsaber, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amfabulous", Cmd_Emote_fabulous, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amfinishinghim", Cmd_Emote_finishinghim, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amharlem", Cmd_Emote_harlem, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amheal", Cmd_Emote_heal, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amhello", Cmd_Emote_hello, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amhips", Cmd_Emote_hips, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amhug", Cmd_Emote_hug, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "aminfo", Cmd_AMInfo_f, GTB_ALL, 0 },
	{ "amkiss", Cmd_Emote_kiss, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amkneel", Cmd_Emote_kneel, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amkneel2", Cmd_Emote_kneel2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amknockmedown", Cmd_Emote_Knockdown, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amneo", Cmd_Emote_neo, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amnod", Cmd_Emote_nod, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amnoisy", Cmd_Emote_noisy, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "ampower", Cmd_Emote_power, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amradio", Cmd_Emote_radio, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsay", Cmd_SayAdmin_f, GTB_ALL, 0 },
	{ "amshake", Cmd_Emote_shake, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amshovel", Cmd_Emote_shovel, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsit1", Cmd_Emote_sit1, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsit2", Cmd_Emote_sit2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsit3", Cmd_Emote_sit3, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsit4", Cmd_Emote_sit4, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsit6", Cmd_Emote_sit6, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsleep", Cmd_Emote_sleep, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsmack1", Cmd_Emote_smack1, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsmack2", Cmd_Emote_smack2, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amstand", Cmd_Emote_stand, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amstepback", Cmd_Emote_stepback, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsuggest", Cmd_Emote_suggest, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amvictory", Cmd_Emote_victory, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amsurrender", Cmd_Emote_surrender, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amwait", Cmd_Emote_wait, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "amwon", Cmd_Emote_won, GTB_ALL, CMDFLAG_NOINTERMISSION | CMDFLAG_ALIVE },
	{ "callvote", Cmd_CallVote_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "debugBMove_Back", Cmd_BotMoveBack_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "debugBMove_Forward", Cmd_BotMoveForward_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "debugBMove_Left", Cmd_BotMoveLeft_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "debugBMove_Right", Cmd_BotMoveRight_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "debugBMove_Up", Cmd_BotMoveUp_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "drop", Cmd_Drop_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "duelteam", Cmd_DuelTeam_f, GTB_DUEL | GTB_POWERDUEL, CMDFLAG_NOINTERMISSION },
	{ "follow", Cmd_Follow_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "follownext", Cmd_FollowNext_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "followprev", Cmd_FollowPrev_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "forcechanged", Cmd_ForceChanged_f, GTB_ALL, 0 },
	{ "gc", Cmd_GameCommand_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "give", Cmd_Give_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "giveother", Cmd_GiveOther_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "god", Cmd_God_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "ignore", Cmd_Ignore_f, GTB_ALL, 0 },
	{ "jetpack", Cmd_Jetpack_f, GTB_ALL & ~GTB_SIEGE, 0 },
	{ "joinchan", Cmd_JoinChannel_f, GTB_ALL, 0 },
	{ "kill", Cmd_Kill_f, GTB_ALL, CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "killother", Cmd_KillOther_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "leavechan", Cmd_LeaveChannel_f, GTB_ALL, 0 },
	{ "levelshot", Cmd_LevelShot_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "msgchan", Cmd_MessageChannel_f, GTB_ALL, 0 },
	{ "noclip", Cmd_Noclip_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "notarget", Cmd_Notarget_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
	{ "npc", Cmd_NPC_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "origin", Cmd_Origin_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "playertint", Cmd_Playertint_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "ready", Cmd_Ready_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "saber", Cmd_Saber_f, GTB_ALL, 0 },
	{ "sabercolor", Cmd_Sabercolor_f, GTB_ALL, 0 },
	{ "say", Cmd_Say_f, GTB_ALL, 0 },
	{ "say_team", Cmd_SayTeam_f, GTB_ALL, 0 },
	{ "say_team_mod", Cmd_SayTeamMod_f, GTB_ALL, 0 },
	{ "score", Cmd_Score_f, GTB_ALL, 0 },
	{ "setviewpos", Cmd_SetViewpos_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_NOINTERMISSION },
	{ "siegeclass", Cmd_SiegeClass_f, GTB_SIEGE, CMDFLAG_NOINTERMISSION },
	{ "team", Cmd_Team_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "tell", Cmd_Tell_f, GTB_ALL, 0 },
	{ "t_use", Cmd_TargetUse_f, GTB_ALL, CMDFLAG_CHEAT | CMDFLAG_ALIVE },
	{ "voice_cmd", Cmd_VoiceCommand_f, GTB_ALL, 0 },
	{ "vote", Cmd_Vote_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "where", Cmd_Where_f, GTB_ALL, CMDFLAG_NOINTERMISSION },
	{ "whoischan", Cmd_WhoisChannel_f, GTB_ALL, 0 },
	{ "wrists", Cmd_Kill_f, GTB_ALL, CMDFLAG_ALIVE | CMDFLAG_NOINTERMISSION },
};
static size_t numCommands = ARRAY_LEN( commands );

// returns the flags that failed to pass, or 0 if the command is allowed to be executed
uint32_t G_CmdValid( const gentity_t *ent, const command_t *cmd ) {
	if ( (cmd->flags & CMDFLAG_NOINTERMISSION) && level.intermissiontime )
		return CMDFAIL_NOINTERMISSION;

	if ( (cmd->flags & CMDFLAG_CHEAT) && !sv_cheats.integer )
		return CMDFAIL_CHEAT;

	if ( (cmd->flags & CMDFLAG_ALIVE)
		&& (ent->health <= 0
		|| ent->client->tempSpectate >= level.time
		|| ent->client->sess.sessionTeam == TEAM_SPECTATOR) ) {
		return CMDFAIL_ALIVE;
	}

	if ( !(cmd->validGT & (1 << level.gametype)) )
		return CMDFAIL_GAMETYPE;

	return 0u;
}

void ClientCommand( int clientNum ) {
	gentity_t		*ent = NULL;
	char			cmd[MAX_TOKEN_CHARS] = { 0 };
	const command_t	*command = NULL;

	ent = g_entities + clientNum;
	if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
		G_LogPrintf( level.log.security, "ClientCommand(%d) without an active connection\n", clientNum );
		return; // not fully in game yet
	}

	trap->Argv( 0, cmd, sizeof(cmd) );

	//rww - redirect bot commands
	if ( strstr( cmd, "bot_" ) && AcceptBotCommand( cmd, ent ) )
		return;
	//end rww

	else if ( AM_HandleCommands( ent, cmd ) )
		return;

	//Raz: JPLua
	if ( JPLua_Event_ClientCommand( clientNum ) )
		return;

	command = (command_t *)bsearch( cmd, commands, numCommands, sizeof(commands[0]), cmdcmp );
	if ( !command ) {
		trap->SendServerCommand( clientNum, va( "print \"Unknown command %s\n\"", cmd ) );
		return;
	}

	switch ( G_CmdValid( ent, command ) ) {
	case 0:
		command->func( ent );
		break;

	case CMDFAIL_NOINTERMISSION:
		trap->SendServerCommand( clientNum, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "CANNOT_TASK_INTERMISSION" ), cmd ) );
		break;

	case CMDFAIL_CHEAT:
		trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "NOCHEATS" ) ) );
		break;

	case CMDFAIL_ALIVE:
		trap->SendServerCommand( clientNum, va( "print \"%s\n\"", G_GetStringEdString( "MP_SVGAME", "MUSTBEALIVE" ) ) );
		break;

	case CMDFAIL_GAMETYPE:
		trap->SendServerCommand( clientNum, va( "print \"%s is not applicable in this gametype\n\"", command->name ) );
		break;

	default:
		break;
	}
}

void G_PrintCommands( gentity_t *ent, printBufferSession_t *pb ) {
	const command_t *command = NULL;
	int toggle = 0;
	unsigned int count = 0;
	const unsigned int limit = 72;
	size_t i;

	Q_PrintBuffer( pb, "Regular commands:\n   " );

	for ( i = 0, command = commands; i < numCommands; i++, command++ ) {
		const char *tmpMsg = NULL;

		// if it's not allowed to be executed at the moment, continue
		if ( G_CmdValid( ent, command ) ) {
			continue;
		}

		tmpMsg = va( " ^%c%s", (++toggle & 1 ? COLOR_GREEN : COLOR_YELLOW), command->name );

		//newline if we reach limit
		if ( count >= limit ) {
			tmpMsg = va( "\n   %s", tmpMsg );
			count = 0;
		}

		count += strlen( tmpMsg );
		Q_PrintBuffer( pb, tmpMsg );
	}

	Q_PrintBuffer( pb, S_COLOR_WHITE "\n\n" );
}
