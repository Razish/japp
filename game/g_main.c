// Copyright (C) 1999-2000 Id Software, Inc.
//

#include "g_local.h"
#include "g_ICARUScb.h"
#include "g_nav.h"
#include "bg_saga.h"
#include "JAPP/jp_crash.h"
#include "bg_lua.h"
#include "JAPP/jp_cinfo.h"
#include "JAPP/jp_ssflags.h"

level_locals_t	level;

int		eventClearTime = 0;
static int navCalcPathTime = 0;
extern int fatalErrors;

int killPlayerTimer = 0;

gentity_t g_entities[MAX_GENTITIES];
static gclient_t g_clients[MAX_CLIENTS]; //Raz: Not directly accessed. level.clients[] should be used

qboolean gDuelExit = qfalse;

void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );
void G_ROFF_NotetrackCallback( gentity_t *cent, const char *notetrack );

extern stringID_table_t setTable[];

qboolean G_ParseSpawnVars( qboolean inSubBSP );


int NAVNEW_ClearPathBetweenPoints( vector3 *start, vector3 *end, vector3 *mins, vector3 *maxs, int ignore, int clipmask );
qboolean NAV_CheckNodeFailedForEnt( gentity_t *ent, int nodeNum );
qboolean G_EntIsUnlockedDoor( int entityNum );
qboolean G_EntIsDoor( int entityNum );
qboolean G_EntIsBreakable( int entityNum );
qboolean G_EntIsRemovableUsable( int entNum );
void CP_FindCombatPointWaypoints( void );

static qboolean G_IsTeamableEntity( const gentity_t *ent ) {
	if ( !ent->inuse ) {
		return qfalse;
	}

	if ( !ent->team ) {
		return qfalse;
	}

	if ( ent->flags & FL_TEAMSLAVE ) {
		return qfalse;
	}

	if ( ent->r.contents == CONTENTS_TRIGGER ) {
		return qfalse;
	}

	return qtrue;
}

// Chain together all entities with a matching team field.
// Entity teams are used for item groups and multi-entity mover groups.
// All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
// All but the last will have the teamchain field set to the next one
static void G_FindTeams( void ) {
	gentity_t *e, *e2;
	int i, j, c, c2;

	c = 0;
	c2 = 0;
	for ( i = MAX_CLIENTS, e = g_entities + i; i < level.num_entities; i++, e++ ) {
		if ( !G_IsTeamableEntity( e ) ) {
			continue;
		}

		e->teammaster = e;
		c++;
		c2++;
		for ( j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++ ) {
			if ( !G_IsTeamableEntity( e2 ) ) {
				continue;
			}

			if ( !Q_stricmp( e->team, e2->team ) ) {
				c2++;
				e2->teamchain = e->teamchain;
				e->teamchain = e2;
				e2->teammaster = e;
				e2->flags |= FL_TEAMSLAVE;

				// make sure that targets only point at the master
				if ( e2->targetname ) {
					e->targetname = e2->targetname;
					e2->targetname = NULL;
				}
			}
		}
	}

#ifdef _DEBUG
	trap->Print( "G_FindTeams: %i teams with %i entities\n", c, c2 );
#endif
}

#include "JAPP/jp_promode.h"

static void SetCInfo( int check, uint32_t bit ) {
	uint32_t cinfo = (unsigned)jp_cinfo.integer;

	if ( check )	cinfo |= bit;
	else			cinfo &= ~bit;

	trap->Cvar_Set( "jp_cinfo", va( "%u", cinfo ) );
	trap->Cvar_Update( &jp_cinfo );
}

static void CVU_Duel( void ) {
	SetCInfo( (g_privateDuel.bits & PRIVDUEL_WEAP), CINFO_PRIVDUELWEAP );
}

static void CVU_Warmup( void ) {
	level.warmupTime = -1;
}

static void CVU_BusyAttack( void ) {
	SetCInfo( !japp_allowBusyAttack.integer, CINFO_NOBUSYATK );
}

static void CVU_Butterfly( void ) {
	SetCInfo( !japp_allowButterfly.integer, CINFO_NOBUTTERFLY );
}

static void CVU_DFA( void ) {
	SetCInfo( !japp_allowDFA.integer, CINFO_NODFA );
}

static void CVU_Kata( void ) {
	SetCInfo( !japp_allowKata.integer, CINFO_NOKATA );
}

static void CVU_Ledge( void ) {
	SetCInfo( japp_allowLedgeGrab.integer, CINFO_LEDGEGRAB );
}

static void CVU_SPCartwheel( void ) {
	SetCInfo( !japp_allowSPCartwheel.integer, CINFO_NOSPCARTWHEEL );
}

static void CVU_Stab( void ) {
	SetCInfo( !japp_allowStab.integer, CINFO_NOSTAB );
}

static void CVU_ToggleAtk( void ) {
	SetCInfo( japp_allowToggleSpecialAttacks.integer, CINFO_TOGGLESPECIALATK );
}

static void CVU_FixRoll( void ) {
	SetCInfo( japp_fixRoll.bits & 1, CINFO_JK2ROLL1 );
	SetCInfo( japp_fixRoll.bits & 2, CINFO_JK2ROLL2 );
	SetCInfo( japp_fixRoll.bits & 4, CINFO_JK2ROLL3 );
}

static void CVU_Flipkick( void ) {
	SetCInfo( japp_flipKick.integer, CINFO_FLIPKICK );
}

static void CVU_YellowDFA( void ) {
	SetCInfo( japp_improveYellowDFA.integer, CINFO_YELLOWDFA );
}

static void CVU_Promode( void ) {
	SetCInfo( japp_promode.integer, CINFO_CPMPHYSICS );
	CPM_UpdateSettings( !!(jp_cinfo.bits & CINFO_CPMPHYSICS) );
}

static void CVU_HeadSlide( void ) {
	SetCInfo( japp_slideOnHead.integer, CINFO_HEADSLIDE );
}

static void CVU_VQ3Physics( void ) {
	SetCInfo( japp_vq3physics.integer, CINFO_VQ3PHYS );
}

static void CVU_WeaponPU( void ) {
	SetCInfo( japp_weaponPickupAlways.integer, CINFO_ALWAYSPICKUPWEAP );
}

static void CVU_WeaponRoll( void ) {
	SetCInfo( japp_weaponRoll.integer, CINFO_WEAPONROLL );
}

static void CVU_CInfo( void ) {
	CPM_UpdateSettings( !!(jp_cinfo.bits & CINFO_CPMPHYSICS) );
}


typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	const char	*cvarName, *defaultString;
	void( *update )(void);
	uint32_t	cvarFlags;
	qboolean	trackChange; // track this variable, and announce if changed
} cvarTable_t;

#define XCVAR_DECL
#include "g_xcvar.h"
#undef XCVAR_DECL

static cvarTable_t gameCvarTable[] = {
#define XCVAR_LIST
#include "g_xcvar.h"
#undef XCVAR_LIST
};
static int gameCvarTableSize = ARRAY_LEN( gameCvarTable );

const char *G_Cvar_DefaultString( const vmCvar_t *vmCvar ) {
	int i = 0;
	const cvarTable_t *cv = NULL;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar == vmCvar ) {
			return cv->defaultString;
		}
	}

	return NULL;
}

void G_RegisterCvars( void ) {
	int i = 0;
	cvarTable_t *cv = NULL;

	// register all cvars
	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );

	// now update them
	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->update )
			cv->update();
	}
}

void G_CacheGametype( void ) {
	// check some things
	if ( g_gametype.string[0] && isalpha( g_gametype.string[0] ) ) {
		int gt = BG_GetGametypeForString( g_gametype.string );
		if ( gt == -1 ) {
			trap->Print( "Gametype '%s' unrecognised, defaulting to FFA/Deathmatch\n", g_gametype.string );
			level.gametype = GT_FFA;
		}
		else
			level.gametype = gt;
	}
	else if ( g_gametype.integer < 0 || level.gametype >= GT_MAX_GAME_TYPE ) {
		trap->Print( "g_gametype %i is out of range, defaulting to 0\n", level.gametype );
		level.gametype = GT_FFA;
	}
	else
		level.gametype = atoi( g_gametype.string );

	trap->Cvar_Set( "g_gametype", va( "%i", level.gametype ) );
}

void G_UpdateCvars( void ) {
	int i = 0;
	cvarTable_t *cv = NULL;

	for ( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount != modCount ) {
				if ( cv->update )
					cv->update();

				if ( cv->trackChange )
					trap->SendServerCommand( -1, va( "print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string ) );
			}
		}
	}
}

char gSharedBuffer[MAX_G_SHARED_BUFFER_SIZE];

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );
extern void RemoveAllWP( void );
extern void BG_ClearVehicleParseParms( void );
extern gentity_t *SelectRandomDeathmatchSpawnPoint( void );
extern void SP_info_jedimaster_start( gentity_t *ent );

#define LOG_DIRECTORY "logs/sv/"
static void G_OpenLog( const char *filename, fileHandle_t *f, qboolean sync ) {
	trap->FS_Open( filename, f, sync ? FS_APPEND_SYNC : FS_APPEND );
	if ( *f )
		trap->Print( "Logging to %s\n", filename );
	else
		trap->Print( "WARNING: Couldn't open logfile: %s\n", filename );
}

static void G_CloseLog( fileHandle_t *f ) {
	if ( !*f )
		return;

	trap->FS_Close( *f );
	*f = NULL_FILE;
}

static void G_AddSingleBox( const vector3 *mins, const vector3 *maxs ) {
	gentity_t *ent = G_Spawn();

	ent->r.contents = CONTENTS_SOLID;
	VectorAverage( mins, maxs, &ent->r.currentOrigin );
	VectorSubtract( mins, &ent->r.currentOrigin, &ent->r.mins );
	VectorSubtract( maxs, &ent->r.currentOrigin, &ent->r.maxs );
	VectorCopy( &ent->r.currentOrigin, &ent->s.origin );
	VectorCopy( &ent->s.origin, &ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	trap->LinkEntity( (sharedEntity_t *)ent );
	// needs to be solid during link for solid calculation
	ent->r.contents = CONTENTS_PLAYERCLIP;
	{
		int x, zd, zu;
		vector3 bmins, bmaxs;

		// encoded bbox
		x = (ent->s.solid & 255);
		zd = ((ent->s.solid>>8) & 255);
		zu = ((ent->s.solid>>16) & 255) - 32;

		bmins.x = bmins.y = -x;
		bmaxs.x = bmaxs.y = x;
		bmins.z = -zd;
		bmaxs.z = zu;

		if ( developer.integer ) {
			Com_Printf( "  placed box at %s\n", vtos( &ent->r.currentOrigin ) );
			Com_Printf( "    mins   %s\n", vtos( &ent->r.mins ) );
			Com_Printf( "    maxs   %s\n", vtos( &ent->r.maxs ) );
			Com_Printf( "    solid  %d\n", ent->s.solid );
			Com_Printf( "    bmins  %s\n", vtos( &bmins ) );
			Com_Printf( "    bmaxs  %s\n", vtos( &bmaxs ) );
		}
	}
}

static int G_AddBox( const vector3 *mins, const vector3 *maxs ) {
	// restrictions on clientside prediction - x,y must be equal and symmetric, and must be <= 255
	// z must be between -255 and 223 (corner case if all are at limit, so use 222 to avoid that issue)
	vector3 singleMins, singleMaxs;
	int xylen = min( min( 510, maxs->x - mins->x ), maxs->y - mins->y );
	int zlen = min( 444, maxs->z - mins->z );
	int count = 0;
	// only even numbers can be predicted correctly
	xylen &= -2;
	zlen &= -2;

	singleMins.x = mins->x;
	while ( 1 ) {
		singleMaxs.x = singleMins.x + xylen;
		singleMins.y = mins->y;

		while ( 1 ) {
			singleMaxs.y = singleMins.y + xylen;
			singleMins.z = mins->z;

			while ( 1 ) {
				singleMaxs.z = singleMins.z + zlen;

				G_AddSingleBox( &singleMins, &singleMaxs );
				count++;

				if ( singleMins.z + zlen >= maxs->z ) {
					break;
				}
				singleMins.z += min( zlen, max( 0, maxs->z - zlen - singleMins.z ) );
			}

			if ( singleMins.y + xylen >= maxs->y ) {
				break;
			}
			singleMins.y += min( xylen, max( 0, maxs->y - xylen - singleMins.y ) );
		}

		if ( singleMins.x + xylen >= maxs->x ) {
			break;
		}

		singleMins.x += min( xylen, max( 0, maxs->x - xylen - singleMins.x ) );
	}

	return count;
}

static void G_SpawnHoleFixes( void ) {
	char mapname[MAX_QPATH], filename[MAX_QPATH];
	int len;
	fileHandle_t f;

	Com_sprintf( mapname, sizeof(mapname), "%s", level.rawmapname );
	// note: only / is replaced with _
	Q_strstrip( mapname, "/\n\r;:.?*<>|\\\"", "_" );
	Com_sprintf( filename, sizeof(filename), "%s_holes.cfg", mapname );

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( len != -1 ) {
		// read mins, maxs out of file
		char *text = (char *)calloc( len + 1, 1 );
		const char *cursor = text;
		vector3 mins, maxs;
		int i;

		Com_Printf( "Loading map holes (%s)\n", filename );
		trap->FS_Read( text, len, f );
		// read contents, parse out values
		COM_BeginParseSession( filename );
		while ( 1 ) {
			int count = 0;
			if ( !cursor[0] ) {
				break;
			}

			VectorClear( &mins );
			VectorClear( &maxs );

			if ( COM_ParseVector( &cursor, &mins ) || COM_ParseVector( &cursor, &maxs ) ) {
				break;
			}

			// fix so mins is actually mins and maxs is actually maxs
			for ( i = 0; i < 3; i++ ) {
				float temp = max( mins.raw[i], maxs.raw[i] );
				mins.raw[i] = min( mins.raw[i], maxs.raw[i] );
				maxs.raw[i] = temp;
			}

			count = G_AddBox( &mins, &maxs );
			Com_Printf( "  placed wall at %s, %s using %i entities\n", vtos( &mins ), vtos( &maxs ), count );

			SkipRestOfLine( &cursor );
		}
		free( text );
		trap->FS_Close( f );
	}
}

void G_InitGame( int levelTime, int randomSeed, int restart ) {
	int			i;
	vmCvar_t	mapname;
	vmCvar_t	ckSum;
	char		cs[MAX_INFO_STRING] = { 0 };
	vmCvar_t	japp_crashHandler;

	trap->Cvar_Register( &japp_crashHandler, "japp_crashHandler", "1", CVAR_ARCHIVE );

	if ( japp_crashHandler.integer ) {
		ActivateCrashHandler();
	}

	//Init RMG to 0, it will be autoset to 1 if there is terrain on the level.
	trap->Cvar_Set( "RMG", "0" );
	RMG.integer = 0;

	//Clean up any client-server ghoul2 instance attachments that may still exist exe-side
	trap->G2API_CleanEntAttachments();

	BG_InitAnimsets(); //clear it out

	B_InitAlloc(); //make sure everything is clean

	trap->SV_RegisterSharedMemory( gSharedBuffer );

	//Load external vehicle data
	BG_VehicleLoadParms();

	srand( randomSeed );

	G_RegisterCvars();

	trap->Cvar_Register( NULL, "ssf", va( "%X", JAPP_SERVER_FLAGS ), CVAR_SERVERINFO );

	G_InitMemory();

	// set some level globals
	memset( &level, 0, sizeof(level) );
	level.time = levelTime;
	level.startTime = levelTime;

	level.follow1 = level.follow2 = -1;

	// logging
	if ( g_logConsole.integer )		G_OpenLog( LOG_DIRECTORY "console.log", &level.log.console, g_logConsole.integer == 2 );
	else							trap->Print( "Not logging console to disk.\n" );
	if ( g_logSecurity.integer )	G_OpenLog( LOG_DIRECTORY "security.log", &level.log.security, g_logSecurity.integer == 2 );
	else							trap->Print( "Not logging security events to disk.\n" );
	if ( g_logAdmin.integer )		G_OpenLog( LOG_DIRECTORY "admin.log", &level.log.admin, g_logAdmin.integer == 2 );
	else							trap->Print( "Not logging admin events to disk.\n" );

	trap->GetServerinfo( cs, sizeof(cs) );
	G_LogPrintf( level.log.console, "------------------------------------------------------------\n" );
	G_LogPrintf( level.log.console, "InitGame: %s\n", cs );
	G_LogPrintf( level.log.console, "\n\nMod version: " JAPP_VERSION "\n" );

	G_LogWeaponInit();

	//Raz: set level.gametype
	G_CacheGametype();

	level.snd_fry = G_SoundIndex( "sound/player/fry.wav" );	// FIXME standing in lava / slime

	level.snd_hack = G_SoundIndex( "sound/player/hacking.wav" );
	level.snd_medHealed = G_SoundIndex( "sound/player/supp_healed.wav" );
	level.snd_medSupplied = G_SoundIndex( "sound/player/supp_supplied.wav" );

	//trap->SP_RegisterServer("mp_svgame");

	Q_strncpyz( level.rawmapname, Info_ValueForKey( cs, "mapname" ), sizeof(level.rawmapname) );

	G_ReadSessionData();

	// initialize all entities for this game
	memset( g_entities, 0, MAX_GENTITIES * sizeof(g_entities[0]) );
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = sv_maxclients.integer;
	memset( g_clients, 0, MAX_CLIENTS * sizeof(g_clients[0]) );
	level.clients = g_clients;

	// set client fields on player ents
	for ( i = 0; i < level.maxclients; i++ ) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients, even if they aren't all used, so numbers inside that range are
	//	NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap->LocateGameData( (sharedEntity_t *)level.gentities, level.num_entities, sizeof(gentity_t),
		&level.clients[0].ps, sizeof(level.clients[0]) );

	//Load sabers.cfg data
	WP_SaberLoadParms();

	NPC_InitGame();

	TIMER_Clear();

	Com_Printf( "-- Initialising ICARUS\n" );
	trap->ICARUS_Init();

	// reserve some spots for dead player bodies
	InitBodyQue();

	ClearRegisteredItems();

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	InitSiegeMode();

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

	navCalculatePaths = (trap->Nav_Load( mapname.string, ckSum.integer ) == qfalse);

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString( qfalse );

	//  add invisible boxes
	G_SpawnHoleFixes();

	// general initialization
	G_FindTeams();

	// make sure we have flags for CTF, etc
	if ( level.gametype >= GT_TEAM ) {
		G_CheckTeamItems();
	}
	else if ( level.gametype == GT_JEDIMASTER ) {
		trap->SetConfigstring( CS_CLIENT_JEDIMASTER, "-1" );
	}

	if ( level.gametype == GT_POWERDUEL ) {
		trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "-1|-1|-1" ) );
	}
	else {
		trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "-1|-1" ) );
	}

	// nmckenzie: DUEL_HEALTH: Default.
	trap->SetConfigstring( CS_CLIENT_DUELHEALTHS, va( "-1|-1|!" ) );
	trap->SetConfigstring( CS_CLIENT_DUELWINNER, va( "-1" ) );

	//Raz: CS_INTERMISSION was not being saved across map_restart
	trap->SetConfigstring( CS_INTERMISSION, "0" );

	SaveRegisteredItems();

	if ( level.gametype == GT_SINGLE_PLAYER || trap->Cvar_VariableIntegerValue( "com_buildScript" ) ) {
		G_SoundIndex( "sound/player/gurp1.wav" );
		G_SoundIndex( "sound/player/gurp2.wav" );
	}

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAISetup( restart );
		BotAILoadMap( restart );
		G_InitBots( restart );
	}
	else {
		// We still want to load arenas even if bot_enable is off so that
		// g_autoMapCycle can work let alone any other code that relies on
		// using arena information that normally wouldn't be loaded :Nervous
		G_LoadArenas();
	}

	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL )
		G_LogPrintf( level.log.console, "Duel Tournament Begun: kill limit %d, win limit: %d\n", fraglimit.integer, duel_fraglimit.integer );

	//not loaded - need to calc paths
	if ( navCalculatePaths ) {
		navCalcPathTime = level.time + START_TIME_NAV_CALC;//make sure all ents are in and linked
	}
	else {//loaded
		//FIXME: if this is from a loadgame, it needs to be sure to write this
		//out whenever you do a savegame since the edges and routes are dynamic...
		//OR: always do a navigator.CheckBlockedEdges() on map startup after nav-load/calc-paths
		//navigator.pathsCalculated = qtrue;//just to be safe?  Does this get saved out?  No... assumed
		trap->Nav_SetPathsCalculated( qtrue );
		//need to do this, because combatpoint waypoints aren't saved out...?
		CP_FindCombatPointWaypoints();
		navCalcPathTime = 0;
	}

	for ( i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++ ) {
		if ( !bg_customSiegeSoundNames[i] ) {
			break;
		}
		G_SoundIndex( bg_customSiegeSoundNames[i] );
	}

	//Raz: JK2 gametypes
	if ( level.gametype == GT_JEDIMASTER ) {//make sure that there's a jedimaster saber spawn point.
		gentity_t *jmsaber = NULL;
		int i = 0;
		for ( i = 0; i < level.num_entities; i++ ) {//scan for jmsaber
			if ( g_entities[i].isSaberEntity ) {
				jmsaber = &g_entities[i];
				break;
			}
		}

		if ( !jmsaber ) {//no JM saber found.  Drop one at one of the player spawnpoints
			gentity_t *spawnpoint = SelectRandomDeathmatchSpawnPoint();

			if ( !spawnpoint ) {//this is bad.
				trap->Error( ERR_DROP, "Couldn't find an FFA spawnpoint to drop the jedimaster saber at!\n" );
				return;
			}

			//create spawnpoint
			jmsaber = G_Spawn();

			G_SetOrigin( jmsaber, &spawnpoint->s.origin );
			SP_info_jedimaster_start( jmsaber );
		}
	}

	//Raz: Load admins + telemarks
	AM_LoadAdmins();
	AM_LoadTelemarks();
	JP_Bans_Init();

#ifdef JPLUA
	//As: BEGIN THE LUAS
	JPLua_Init();
#endif // JPLUA
}

void G_ShutdownGame( int restart ) {
	int i = 0;
	gentity_t *ent;

	//	trap->Print ("==== ShutdownGame ====\n");
	JP_Bans_SaveBans();
	JP_Bans_Clear();
	AM_SaveTelemarks();

	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

	//	Com_Printf("... Gameside GHOUL2 Cleanup\n");
	while ( i < MAX_GENTITIES ) { //clean up all the ghoul2 instances
		ent = &g_entities[i];

		if ( ent->ghoul2 && trap->G2API_HaveWeGhoul2Models( ent->ghoul2 ) ) {
			trap->G2API_CleanGhoul2Models( &ent->ghoul2 );
			ent->ghoul2 = NULL;
		}
		if ( ent->client ) {
			int j = 0;

			while ( j < MAX_SABERS ) {
				if ( ent->client->weaponGhoul2[j] && trap->G2API_HaveWeGhoul2Models( ent->client->weaponGhoul2[j] ) ) {
					trap->G2API_CleanGhoul2Models( &ent->client->weaponGhoul2[j] );
				}
				j++;
			}
		}
		i++;
	}
	if ( g2SaberInstance && trap->G2API_HaveWeGhoul2Models( g2SaberInstance ) ) {
		trap->G2API_CleanGhoul2Models( &g2SaberInstance );
		g2SaberInstance = NULL;
	}
	if ( precachedKyle && trap->G2API_HaveWeGhoul2Models( precachedKyle ) ) {
		trap->G2API_CleanGhoul2Models( &precachedKyle );
		precachedKyle = NULL;
	}

	//	Com_Printf ("... ICARUS_Shutdown\n");
	trap->ICARUS_Shutdown();	//Shut ICARUS down

	//	Com_Printf ("... Reference Tags Cleared\n");
	TAG_Init();	//Clear the reference tags

	G_LogWeaponOutput();

#ifdef JPLUA
	//Raz: Shutdown JPLua, triggers events etc
	JPLua_Shutdown();
#endif // JPLUA

	G_LogPrintf( level.log.console, "ShutdownGame:\n" );
	G_LogPrintf( level.log.console, "------------------------------------------------------------\n" );

	G_CloseLog( &level.log.console );
	G_CloseLog( &level.log.security );

	// write all the client session data so we can get it back
	G_WriteSessionData();

	trap->ROFF_Clean();

	if ( trap->Cvar_VariableIntegerValue( "bot_enable" ) ) {
		BotAIShutdown( restart );
	}

	B_CleanupAlloc(); //clean up all allocations made with B_Alloc

	DeactivateCrashHandler();
}

// If there are less than two tournament players, put a spectator in the game and restart
void AddTournamentPlayer( void ) {
	int			i;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 2 ) {
		return;
	}

	// never change during intermission
	//	if ( level.intermissiontime ) {
	//		return;
	//	}

	nextInLine = NULL;

	for ( i = 0; i < level.maxclients; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !g_allowHighPingDuelist.integer && client->ps.ping >= 999 ) { //don't add people who are lagging out if cvar is not set to allow it.
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0 ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[nextInLine - level.clients], "f", qfalse );
}

// Make the loser a spectator at the back of the line
void RemoveTournamentLoser( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[1];

	if ( level.clients[clientNum].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[clientNum], "s", qfalse );
}

void G_PowerDuelCount( int *loners, int *doubles, qboolean countSpec ) {
	int i = 0;
	gclient_t *cl;

	while ( i < MAX_CLIENTS ) {
		cl = g_entities[i].client;

		if ( g_entities[i].inuse && cl && (countSpec || cl->sess.sessionTeam != TEAM_SPECTATOR) ) {
			if ( cl->sess.duelTeam == DUELTEAM_LONE ) {
				(*loners)++;
			}
			else if ( cl->sess.duelTeam == DUELTEAM_DOUBLE ) {
				(*doubles)++;
			}
		}
		i++;
	}
}

qboolean g_duelAssigning = qfalse;
void AddPowerDuelPlayers( void ) {
	int			i;
	int			loners = 0;
	int			doubles = 0;
	int			nonspecLoners = 0;
	int			nonspecDoubles = 0;
	gclient_t	*client;
	gclient_t	*nextInLine;

	if ( level.numPlayingClients >= 3 ) {
		return;
	}

	nextInLine = NULL;

	G_PowerDuelCount( &nonspecLoners, &nonspecDoubles, qfalse );
	if ( nonspecLoners >= 1 && nonspecDoubles >= 2 ) { //we have enough people, stop
		return;
	}

	//Could be written faster, but it's not enough to care I suppose.
	G_PowerDuelCount( &loners, &doubles, qtrue );

	if ( loners < 1 || doubles < 2 ) { //don't bother trying to spawn anyone yet if the balance is not even set up between spectators
		return;
	}

	//Count again, with only in-game clients in mind.
	loners = nonspecLoners;
	doubles = nonspecDoubles;
	//	G_PowerDuelCount(&loners, &doubles, qfalse);

	for ( i = 0; i < level.maxclients; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.duelTeam == DUELTEAM_FREE ) {
			continue;
		}
		if ( client->sess.duelTeam == DUELTEAM_LONE && loners >= 1 ) {
			continue;
		}
		if ( client->sess.duelTeam == DUELTEAM_DOUBLE && doubles >= 2 ) {
			continue;
		}

		// never select the dedicated follow or scoreboard clients
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ||
			client->sess.spectatorClient < 0 ) {
			continue;
		}

		if ( !nextInLine || client->sess.spectatorTime < nextInLine->sess.spectatorTime ) {
			nextInLine = client;
		}
	}

	if ( !nextInLine ) {
		return;
	}

	level.warmupTime = -1;

	// set them to free-for-all team
	SetTeam( &g_entities[nextInLine - level.clients], "f", qfalse );

	//Call recursively until everyone is in
	AddPowerDuelPlayers();
}

qboolean g_dontFrickinCheck = qfalse;

void RemovePowerDuelLosers( void ) {
	int remClients[3];
	int remNum = 0;
	int i = 0;
	gclient_t *cl;

	while ( i < MAX_CLIENTS && remNum < 3 ) {
		//cl = &level.clients[level.sortedClients[i]];
		cl = &level.clients[i];

		if ( cl->pers.connected == CON_CONNECTED ) {
			if ( (cl->ps.stats[STAT_HEALTH] <= 0 || cl->iAmALoser) &&
				(cl->sess.sessionTeam != TEAM_SPECTATOR || cl->iAmALoser) ) { //he was dead or he was spectating as a loser
				remClients[remNum] = cl->ps.clientNum;
				remNum++;
			}
		}

		i++;
	}

	if ( !remNum ) { //Time ran out or something? Oh well, just remove the main guy.
		remClients[remNum] = level.sortedClients[0];
		remNum++;
	}

	i = 0;
	while ( i < remNum ) { //set them all to spectator
		SetTeam( &g_entities[remClients[i]], "s", qfalse );
		i++;
	}

	g_dontFrickinCheck = qfalse;

	//recalculate stuff now that we have reset teams.
	CalculateRanks();
}

void RemoveDuelDrawLoser( void ) {
	int clFirst = 0;
	int clSec = 0;
	int clFailure = 0;

	if ( level.clients[level.sortedClients[0]].pers.connected != CON_CONNECTED ) {
		return;
	}
	if ( level.clients[level.sortedClients[1]].pers.connected != CON_CONNECTED ) {
		return;
	}

	clFirst = level.clients[level.sortedClients[0]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[0]].ps.stats[STAT_ARMOR];
	clSec = level.clients[level.sortedClients[1]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[1]].ps.stats[STAT_ARMOR];

	if ( clFirst > clSec ) {
		clFailure = 1;
	}
	else if ( clSec > clFirst ) {
		clFailure = 0;
	}
	else {
		clFailure = 2;
	}

	if ( clFailure != 2 ) {
		SetTeam( &g_entities[level.sortedClients[clFailure]], "s", qfalse );
	}
	else { //we could be more elegant about this, but oh well.
		SetTeam( &g_entities[level.sortedClients[1]], "s", qfalse );
	}
}

void RemoveTournamentWinner( void ) {
	int			clientNum;

	if ( level.numPlayingClients != 2 ) {
		return;
	}

	clientNum = level.sortedClients[0];

	if ( level.clients[clientNum].pers.connected != CON_CONNECTED ) {
		return;
	}

	// make them a spectator
	SetTeam( &g_entities[clientNum], "s", qfalse );
}

void AdjustTournamentScores( void ) {
	int			clientNum;

	if ( level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
		level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
		level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
		level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED ) {
		int clFirst = level.clients[level.sortedClients[0]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[0]].ps.stats[STAT_ARMOR];
		int clSec = level.clients[level.sortedClients[1]].ps.stats[STAT_HEALTH] + level.clients[level.sortedClients[1]].ps.stats[STAT_ARMOR];
		int clFailure = 0;
		int clSuccess = 0;

		if ( clFirst > clSec ) {
			clFailure = 1;
			clSuccess = 0;
		}
		else if ( clSec > clFirst ) {
			clFailure = 0;
			clSuccess = 1;
		}
		else {
			clFailure = 2;
			clSuccess = 2;
		}

		if ( clFailure != 2 ) {
			clientNum = level.sortedClients[clSuccess];

			level.clients[clientNum].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring( CS_CLIENT_DUELWINNER, va( "%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[clientNum].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
		else {
			clSuccess = 0;
			clFailure = 1;

			clientNum = level.sortedClients[clSuccess];

			level.clients[clientNum].sess.wins++;
			ClientUserinfoChanged( clientNum );
			trap->SetConfigstring( CS_CLIENT_DUELWINNER, va( "%i", clientNum ) );

			clientNum = level.sortedClients[clFailure];

			level.clients[clientNum].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
	else {
		clientNum = level.sortedClients[0];
		if ( level.clients[clientNum].pers.connected == CON_CONNECTED ) {
			level.clients[clientNum].sess.wins++;
			ClientUserinfoChanged( clientNum );

			trap->SetConfigstring( CS_CLIENT_DUELWINNER, va( "%i", clientNum ) );
		}

		clientNum = level.sortedClients[1];
		if ( level.clients[clientNum].pers.connected == CON_CONNECTED ) {
			level.clients[clientNum].sess.losses++;
			ClientUserinfoChanged( clientNum );
		}
	}
}

int QDECL SortRanks( const void *a, const void *b ) {
	gclient_t	*ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	if ( level.gametype == GT_POWERDUEL ) {
		//sort single duelists first
		if ( ca->sess.duelTeam == DUELTEAM_LONE && ca->sess.sessionTeam != TEAM_SPECTATOR ) {
			return -1;
		}
		if ( cb->sess.duelTeam == DUELTEAM_LONE && cb->sess.sessionTeam != TEAM_SPECTATOR ) {
			return 1;
		}

		//others will be auto-sorted below but above spectators.
	}

	// sort special clients last
	if ( ca->sess.spectatorState == SPECTATOR_SCOREBOARD || ca->sess.spectatorClient < 0 ) {
		return 1;
	}
	if ( cb->sess.spectatorState == SPECTATOR_SCOREBOARD || cb->sess.spectatorClient < 0 ) {
		return -1;
	}

	// then connecting clients
	if ( ca->pers.connected == CON_CONNECTING ) {
		return 1;
	}
	if ( cb->pers.connected == CON_CONNECTING ) {
		return -1;
	}


	// then spectators
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ca->sess.spectatorTime < cb->sess.spectatorTime ) {
			return -1;
		}
		if ( ca->sess.spectatorTime > cb->sess.spectatorTime ) {
			return 1;
		}
		return 0;
	}
	if ( ca->sess.sessionTeam == TEAM_SPECTATOR ) {
		return 1;
	}
	if ( cb->sess.sessionTeam == TEAM_SPECTATOR ) {
		return -1;
	}

	// then sort by score
	if ( ca->ps.persistant[PERS_SCORE]
	> cb->ps.persistant[PERS_SCORE] ) {
		return -1;
	}
	if ( ca->ps.persistant[PERS_SCORE]
		< cb->ps.persistant[PERS_SCORE] ) {
		return 1;
	}
	return 0;
}

qboolean gQueueScoreMessage = qfalse;
int gQueueScoreMessageTime = 0;

//A new duel started so respawn everyone and make sure their stats are reset
qboolean G_CanResetDuelists( void ) {
	int i;
	gentity_t *ent;

	i = 0;
	while ( i < 3 ) { //precheck to make sure they are all respawnable
		ent = &g_entities[level.sortedClients[i]];

		if ( !ent->inuse || !ent->client || ent->health <= 0 ||
			ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
			ent->client->sess.duelTeam <= DUELTEAM_FREE ) {
			return qfalse;
		}
		i++;
	}

	return qtrue;
}

qboolean g_noPDuelCheck = qfalse;
void G_ResetDuelists( void ) {
	int i;
	gentity_t *ent;
	gentity_t *tent;

	i = 0;
	while ( i < 3 ) {
		ent = &g_entities[level.sortedClients[i]];

		g_noPDuelCheck = qtrue;
		player_die( ent, ent, ent, 999, MOD_SUICIDE );
		g_noPDuelCheck = qfalse;
		trap->UnlinkEntity( (sharedEntity_t *)ent );
		ClientSpawn( ent );

		// add a teleportation effect
		tent = G_TempEntity( &ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;
		i++;
	}
}

// Recalculates the score ranks of all players
// This will be called on every client connect, begin, disconnect, death, and team change.
void CalculateRanks( void ) {
	int		i;
	int		rank;
	int		score;
	int		newScore;
	//int		nonSpecIndex = -1;
	gclient_t	*cl;

	level.follow1 = -1;
	level.follow2 = -1;
	level.numConnectedClients = 0;
	level.numNonSpectatorClients = 0;
	level.numPlayingClients = 0;
	level.numVotingClients = 0;		// don't count bots

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].pers.connected != CON_DISCONNECTED ) {
			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
				if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR ) {
					level.numNonSpectatorClients++;
					//nonSpecIndex = i;
				}

				// decide if this should be auto-followed
				if ( level.clients[i].pers.connected == CON_CONNECTED ) {
					if ( level.clients[i].sess.sessionTeam != TEAM_SPECTATOR || level.clients[i].iAmALoser )
						level.numPlayingClients++;

					if ( !(g_entities[i].r.svFlags & SVF_BOT) )
						level.numVotingClients++;

					if ( level.follow1 == -1 )		level.follow1 = i;
					else if ( level.follow2 == -1 )		level.follow2 = i;
				}
			}
		}
	}

	qsort( level.sortedClients, level.numConnectedClients,
		sizeof(level.sortedClients[0]), SortRanks );

	// set the rank value for all clients that are connected and not spectators
	if ( level.gametype >= GT_TEAM ) {
		// in team games, rank is just the order of the teams, 0=red, 1=blue, 2=tied
		for ( i = 0; i < level.numConnectedClients; i++ ) {
			cl = &level.clients[level.sortedClients[i]];
			if ( level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 2;
			}
			else if ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) {
				cl->ps.persistant[PERS_RANK] = 0;
			}
			else {
				cl->ps.persistant[PERS_RANK] = 1;
			}
		}
	}
	else {
		rank = -1;
		score = 0;
		for ( i = 0; i < level.numPlayingClients; i++ ) {
			cl = &level.clients[level.sortedClients[i]];
			newScore = cl->ps.persistant[PERS_SCORE];
			if ( i == 0 || newScore != score ) {
				rank = i;
				// assume we aren't tied until the next client is checked
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank;
			}
			else {
				// we are tied with the previous client
				level.clients[level.sortedClients[i - 1]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
			score = newScore;
			if ( level.gametype == GT_SINGLE_PLAYER && level.numPlayingClients == 1 ) {
				level.clients[level.sortedClients[i]].ps.persistant[PERS_RANK] = rank | RANK_TIED_FLAG;
			}
		}
	}

	// set the CS_SCORES1/2 configstrings, which will be visible to everyone
	if ( level.gametype >= GT_TEAM ) {
		trap->SetConfigstring( CS_SCORES1, va( "%i", level.teamScores[TEAM_RED] ) );
		trap->SetConfigstring( CS_SCORES2, va( "%i", level.teamScores[TEAM_BLUE] ) );
	}
	else {
		if ( level.numConnectedClients == 0 ) {
			trap->SetConfigstring( CS_SCORES1, va( "%i", SCORE_NOT_PRESENT ) );
			trap->SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		}
		else if ( level.numConnectedClients == 1 ) {
			trap->SetConfigstring( CS_SCORES1, va( "%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
		}
		else {
			trap->SetConfigstring( CS_SCORES1, va( "%i", level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ) );
			trap->SetConfigstring( CS_SCORES2, va( "%i", level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] ) );
		}

		if ( level.gametype != GT_DUEL && level.gametype != GT_POWERDUEL ) { //when not in duel, use this configstring to pass the index of the player currently in first place
			if ( level.numConnectedClients >= 1 ) {
				trap->SetConfigstring( CS_CLIENT_DUELWINNER, va( "%i", level.sortedClients[0] ) );
			}
			else {
				trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	// see if it is time to end the level
	CheckExitRules();

	// if we are at the intermission or in multi-frag Duel game mode, send the new info to everyone
	if ( level.intermissiontime || level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		gQueueScoreMessage = qtrue;
		gQueueScoreMessageTime = level.time + 500;
		//SendScoreboardMessageToAllClients();
		//rww - Made this operate on a "queue" system because it was causing large overflows
	}
}

// Do this at BeginIntermission time and whenever ranks are recalculated due to enters/exits/forced team changes
void SendScoreboardMessageToAllClients( void ) {
	int i;
	gclient_t *cl;

	for ( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ ) {
		if ( cl->pers.connected == CON_CONNECTED )
			cl->scoresWaiting = qtrue;
	}
}

// When the intermission starts, this will be called for all players.
// If a new client connects, this will be called after the spawn function.
void MoveClientToIntermission( gentity_t *ent ) {
	// take out of follow mode if needed
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		StopFollowing( ent );
	}


	// move to the spot
	VectorCopy( &level.intermission_origin, &ent->s.origin );
	VectorCopy( &level.intermission_origin, &ent->client->ps.origin );
	VectorCopy( &level.intermission_angle, &ent->client->ps.viewangles );
	ent->client->ps.pm_type = PM_INTERMISSION;

	// clean up powerup info
	memset( ent->client->ps.powerups, 0, sizeof(ent->client->ps.powerups) );

	G_LeaveVehicle( ent, qfalse );

	ent->client->ps.rocketLockIndex = ENTITYNUM_NONE;
	ent->client->ps.rocketLockTime = 0;

	ent->client->ps.eFlags = 0;
	ent->s.eFlags = 0;
	ent->client->ps.eFlags2 = 0;
	ent->s.eFlags2 = 0;
	ent->s.eType = ET_GENERAL;
	ent->s.modelindex = 0;
	ent->s.loopSound = 0;
	ent->s.loopIsSoundset = qfalse;
	ent->s.event = 0;
	ent->r.contents = 0;

	// remove grapple
	if ( ent->client->hook ) {
		Weapon_HookFree( ent->client->hook );
	}
}

// This is also used for spectator spawns
void FindIntermissionPoint( void ) {
	gentity_t	*ent = NULL;
	gentity_t	*target;
	vector3		dir;

	// find the intermission spot
	if ( level.gametype == GT_SIEGE
		&& level.intermissiontime
		&& level.intermissiontime <= level.time
		&& gSiegeRoundEnded ) {
		if ( gSiegeRoundWinningTeam == SIEGETEAM_TEAM1 ) {
			ent = G_Find( NULL, FOFS( classname ), "info_player_intermission_red" );
			if ( ent && ent->target2 ) {
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
		else if ( gSiegeRoundWinningTeam == SIEGETEAM_TEAM2 ) {
			ent = G_Find( NULL, FOFS( classname ), "info_player_intermission_blue" );
			if ( ent && ent->target2 ) {
				G_UseTargets2( ent, ent, ent->target2 );
			}
		}
	}
	if ( !ent ) {
		ent = G_Find( NULL, FOFS( classname ), "info_player_intermission" );
	}
	if ( !ent ) {	// the map creator forgot to put in an intermission point...
		SelectSpawnPoint( &vec3_origin, &level.intermission_origin, &level.intermission_angle, TEAM_SPECTATOR );
	}
	else {
		VectorCopy( &ent->s.origin, &level.intermission_origin );
		VectorCopy( &ent->s.angles, &level.intermission_angle );
		// if it has a target, look towards it
		if ( ent->target ) {
			target = G_PickTarget( ent->target );
			if ( target ) {
				VectorSubtract( &target->s.origin, &level.intermission_origin, &dir );
				vectoangles( &dir, &level.intermission_angle );
			}
		}
	}

}

qboolean DuelLimitHit( void );

void BeginIntermission( void ) {
	int			i;
	gentity_t	*client;

	if ( level.intermissiontime ) {
		return;		// already active
	}

	// if in tournement mode, change the wins / losses
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );

		if ( level.gametype != GT_POWERDUEL )
			AdjustTournamentScores();
		if ( DuelLimitHit() )
			gDuelExit = qtrue;
		else
			gDuelExit = qfalse;
	}

	level.intermissiontime = level.time;
	FindIntermissionPoint();

	// move all clients to the intermission point
	for ( i = 0; i < level.maxclients; i++ ) {
		client = g_entities + i;
		if ( !client->inuse )
			continue;
		// respawn if dead
		if ( client->health <= 0 ) {
			if ( level.gametype != GT_POWERDUEL ||
				!client->client ||
				client->client->sess.sessionTeam != TEAM_SPECTATOR ) { //don't respawn spectators in powerduel or it will mess the line order all up
				respawn( client );
			}
		}
		MoveClientToIntermission( client );
	}

	// send the current scoring to all clients
	SendScoreboardMessageToAllClients();

}

qboolean DuelLimitHit( void ) {
	int i;
	gclient_t *cl;

	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
			continue;

		if ( duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer )
			return qtrue;
	}

	return qfalse;
}

void DuelResetWinsLosses( void ) {
	int i;
	gclient_t *cl;

	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED )
			continue;

		cl->sess.wins = 0;
		cl->sess.losses = 0;
	}
}

void SiegeDoTeamAssign( void ); //g_saga.c
extern siegePers_t g_siegePersistant; //g_saga.c

// When the intermission has been exited, the server is either killed or moved to a new level based on the "nextmap"
//	cvar
void ExitLevel( void ) {
	int		i;
	gclient_t *cl;

	// if we are running a tournement map, kick the loser to spectator status,
	// which will automatically grab the next spectator and restart
	if ( level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL ) {
		if ( !DuelLimitHit() ) {
			if ( !level.restarted ) {
				trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				level.intermissiontime = 0;
			}
			return;
		}

		DuelResetWinsLosses();
	}


	if ( level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer &&
		g_siegePersistant.beatingTime ) { //restart same map...
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
	}
	else {
		trap->SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );
	}
	level.intermissiontime = 0;

	if ( level.gametype == GT_SIEGE &&
		g_siegeTeamSwitch.integer ) { //switch out now
		SiegeDoTeamAssign();
	}

	// reset all the scores so we don't enter the intermission again
	level.teamScores[TEAM_RED] = 0;
	level.teamScores[TEAM_BLUE] = 0;
	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.persistant[PERS_SCORE] = 0;
	}

	// we need to do this here before chaning to CON_CONNECTING
	G_WriteSessionData();

	// change all client states to connecting, so the early players into the
	// next level will know the others aren't done reconnecting
	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			level.clients[i].pers.connected = CON_CONNECTING;
		}
	}

}

// Print to the logfile with a time stamp if it is open
void QDECL G_LogPrintf( fileHandle_t fileHandle, const char *fmt, ... ) {
	va_list argptr;
	char string[1024] = { 0 };
	size_t len;

	if ( g_logFormat.integer == 0 ) {
		int msec = level.time - level.startTime;
		int secs = msec / 1000;
		int mins = secs / 60;
		secs %= 60;
		msec %= 1000;

		Com_sprintf( string, sizeof(string), "%i:%02i ", mins, secs );
	}
	else {
		time_t rawtime;
		time( &rawtime );
		strftime( string, sizeof(string), "[%Y-%m-%d] [%H:%M:%S] ", localtime( &rawtime ) );
	}

	len = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + len, sizeof(string)-len, fmt, argptr );
	va_end( argptr );

	if ( dedicated.integer )
		trap->Print( "%s", string + len );

	if ( !fileHandle )
		return;

	trap->FS_Write( string, strlen( string ), fileHandle );
}

// Append information about this game to the log file
void LogExit( const char *string ) {
	int				i, numSorted;
	gclient_t		*cl;
	//	qboolean		won = qtrue;
	G_LogPrintf( level.log.console, "Exit: %s\n", string );

	level.intermissionQueued = level.time;

	// this will keep the clients from playing any voice sounds
	// that will get cut off when the queued intermission starts
	trap->SetConfigstring( CS_INTERMISSION, "1" );

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > MAX_CLIENTS )
		numSorted = MAX_CLIENTS;

	if ( level.gametype >= GT_TEAM ) {
		G_LogPrintf( level.log.console, "red:%i  blue:%i\n", level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE] );
	}

	for ( i = 0; i < numSorted; i++ ) {
		int		ping;

		cl = &level.clients[level.sortedClients[i]];

		if ( cl->sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		if ( cl->pers.connected == CON_CONNECTING ) {
			continue;
		}

		ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

		G_LogPrintf( level.log.console, "score: %i  ping: %i  client: %i %s\n", cl->ps.persistant[PERS_SCORE], ping,
			level.sortedClients[i], cl->pers.netname );
		//		if (g_singlePlayer.integer && (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL)) {
		//			if (g_entities[cl - level.clients].r.svFlags & SVF_BOT && cl->ps.persistant[PERS_RANK] == 0) {
		//				won = qfalse;
		//			}
		//		}
	}

	//yeah.. how about not.
	/*
	if (g_singlePlayer.integer) {
	if (level.gametype >= GT_CTF) {
	won = level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE];
	}
	trap->SendConsoleCommand( EXEC_APPEND, (won) ? "spWin\n" : "spLose\n" );
	}
	*/
}

qboolean gDidDuelStuff = qfalse; //gets reset on game reinit

// The level will stay at the intermission for a minimum of 5 seconds
// If all players wish to continue, the level will then exit.
// If one or more players have not acknowledged the continue, the game will wait 10 seconds before going on.
void CheckIntermissionExit( void ) {
	int ready, notReady, i;
	uint32_t readyMask;
	gclient_t *cl;

	// see which players are ready
	ready = 0;
	notReady = 0;
	readyMask = 0u;
	for ( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED )
			continue;

		if ( g_entities[i].r.svFlags & SVF_BOT )
			continue;

		if ( cl->readyToExit ) {
			ready++;
			if ( i < 16 ) {
				readyMask |= 1 << i;
			}
		}
		else {
			notReady++;
		}
	}

	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDidDuelStuff &&
		(level.time > level.intermissiontime + 2000) ) {
		gDidDuelStuff = qtrue;

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL ) {
			G_LogPrintf( level.log.console, "Duel Results:\n" );
			//G_LogPrintf( level.log.console, "Duel Time: %d\n", level.time );
			G_LogPrintf( level.log.console, "winner: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
			G_LogPrintf( level.log.console, "loser: %s, score: %d, wins/losses: %d/%d\n",
				level.clients[level.sortedClients[1]].pers.netname,
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE],
				level.clients[level.sortedClients[1]].sess.wins,
				level.clients[level.sortedClients[1]].sess.losses );
		}
		// if we are running a tournement map, kick the loser to spectator status,
		// which will automatically grab the next spectator and restart
		if ( !DuelLimitHit() ) {
			if ( level.gametype == GT_POWERDUEL ) {
				RemovePowerDuelLosers();
				AddPowerDuelPlayers();
			}
			else {
				if ( level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
					level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
					level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
					level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED ) {
					RemoveDuelDrawLoser();
				}
				else {
					RemoveTournamentLoser();
				}
				AddTournamentPlayer();
			}

			if ( g_austrian.integer ) {
				if ( level.gametype == GT_POWERDUEL ) {
					G_LogPrintf( level.log.console, "Power Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						level.clients[level.sortedClients[2]].pers.netname,
						level.clients[level.sortedClients[2]].sess.wins,
						level.clients[level.sortedClients[2]].sess.losses,
						fraglimit.integer );
				}
				else {
					G_LogPrintf( level.log.console, "Duel Initiated: %s %d/%d vs %s %d/%d, kill limit: %d\n",
						level.clients[level.sortedClients[0]].pers.netname,
						level.clients[level.sortedClients[0]].sess.wins,
						level.clients[level.sortedClients[0]].sess.losses,
						level.clients[level.sortedClients[1]].pers.netname,
						level.clients[level.sortedClients[1]].sess.wins,
						level.clients[level.sortedClients[1]].sess.losses,
						fraglimit.integer );
				}
			}

			if ( level.gametype == GT_POWERDUEL ) {
				if ( level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3 ) {
					trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
					trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );
				}
			}
			else {
				if ( level.numPlayingClients >= 2 ) {
					trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
					trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );
				}
			}

			return;
		}

		if ( g_austrian.integer && level.gametype != GT_POWERDUEL ) {
			G_LogPrintf( level.log.console, "Duel Tournament Winner: %s wins/losses: %d/%d\n",
				level.clients[level.sortedClients[0]].pers.netname,
				level.clients[level.sortedClients[0]].sess.wins,
				level.clients[level.sortedClients[0]].sess.losses );
		}

		if ( level.gametype == GT_POWERDUEL ) {
			RemovePowerDuelLosers();
			AddPowerDuelPlayers();

			if ( level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3 ) {
				trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
		else {
			//this means we hit the duel limit so reset the wins/losses
			//but still push the loser to the back of the line, and retain the order for
			//the map change
			if ( level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE] ==
				level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE] &&
				level.clients[level.sortedClients[0]].pers.connected == CON_CONNECTED &&
				level.clients[level.sortedClients[1]].pers.connected == CON_CONNECTED ) {
				RemoveDuelDrawLoser();
			}
			else {
				RemoveTournamentLoser();
			}

			AddTournamentPlayer();

			if ( level.numPlayingClients >= 2 ) {
				trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
				trap->SetConfigstring( CS_CLIENT_DUELWINNER, "-1" );
			}
		}
	}

	if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && !gDuelExit ) { //in duel, we have different behaviour for between-round intermissions
		if ( level.time > level.intermissiontime + 4000 ) { //automatically go to next after 4 seconds
			ExitLevel();
			return;
		}

		for ( i = 0; i < sv_maxclients.integer; i++ ) { //being in a "ready" state is not necessary here, so clear it for everyone
			//yes, I also thinking holding this in a ps value uniquely for each player
			//is bad and wrong, but it wasn't my idea.
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			cl->ps.stats[STAT_CLIENTS_READY] = 0;
		}
		return;
	}

	// copy the readyMask to each player's stats so
	// it can be displayed on the scoreboard
	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		cl->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	// never exit in less than five seconds
	if ( level.time < level.intermissiontime + 5000 ) {
		return;
	}

	if ( d_noIntermissionWait.integer ) { //don't care who wants to go, just go.
		ExitLevel();
		return;
	}

	// if nobody wants to go, clear timer
	if ( !ready ) {
		level.readyToExit = qfalse;
		return;
	}

	// if everyone wants to go, go now
	if ( !notReady ) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if ( !level.readyToExit ) {
		level.readyToExit = qtrue;
		level.exitTime = level.time;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if ( level.time < level.exitTime + 10000 ) {
		return;
	}

	ExitLevel();
}

qboolean ScoreIsTied( void ) {
	int		a, b;

	if ( level.numPlayingClients < 2 ) {
		return qfalse;
	}

	if ( level.gametype >= GT_TEAM ) {
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];
	}

	a = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
	b = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];

	return a == b;
}

qboolean g_endPDuel = qfalse;

// There will be a delay between the time the exit is qualified for and the time everyone is moved to the intermission
//	spot, so you can see the last frag.
void CheckExitRules( void ) {
	int			i;
	gclient_t	*cl;
	const char *sKillLimit;
	qboolean printLimit = qtrue;

	// if at the intermission, wait for all non-bots to signal ready, then go to next level
	if ( level.intermissiontime ) {
		CheckIntermissionExit();
		return;
	}

	if ( gDoSlowMoDuel ) { //don't go to intermission while in slow motion
		return;
	}

	if ( gEscaping ) {
		int i = 0;
		int numLiveClients = 0;

		while ( i < MAX_CLIENTS ) {
			if ( g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0 ) {
				if ( g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
					!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW) ) {
					numLiveClients++;
				}
			}

			i++;
		}
		if ( gEscapeTime < level.time ) {
			gEscaping = qfalse;
			LogExit( "Escape time ended." );
			return;
		}
		if ( !numLiveClients ) {
			gEscaping = qfalse;
			LogExit( "Everyone failed to escape." );
			return;
		}
	}

	if ( level.intermissionQueued ) {
		//int time = (g_singlePlayer.integer) ? SP_INTERMISSION_DELAY_TIME : INTERMISSION_DELAY_TIME;
		int time = INTERMISSION_DELAY_TIME;
		if ( level.time - level.intermissionQueued >= time ) {
			level.intermissionQueued = 0;
			BeginIntermission();
		}
		return;
	}

	// check for sudden death
	if ( level.gametype != GT_SIEGE ) {
		if ( ScoreIsTied() ) {
			// always wait for sudden death
			if ( (level.gametype != GT_DUEL) || !timelimit.value ) {
				if ( level.gametype != GT_POWERDUEL ) {
					return;
				}
			}
		}
	}

	if ( level.gametype != GT_SIEGE ) {
		if ( timelimit.value > 0.0f && !level.warmupTime ) {
			if ( level.time - level.startTime >= timelimit.value * 60000 ) {
				//				trap->SendServerCommand( -1, "print \"Timelimit hit.\n\"");
				trap->SendServerCommand( -1, va( "print \"%s.\n\"", G_GetStringEdString( "MP_SVGAME", "TIMELIMIT_HIT" ) ) );
				if ( d_powerDuelPrint.integer ) {
					Com_Printf( "POWERDUEL WIN CONDITION: Timelimit hit (1)\n" );
				}
				LogExit( "Timelimit hit." );
				return;
			}
		}
	}

	if ( level.gametype == GT_POWERDUEL && level.numPlayingClients >= 3 ) {
		if ( g_endPDuel ) {
			g_endPDuel = qfalse;
			LogExit( "Powerduel ended." );
		}
		return;
	}

	if ( level.numPlayingClients < 2 ) {
		return;
	}

	if ( level.gametype == GT_DUEL ||
		level.gametype == GT_POWERDUEL ) {
		if ( fraglimit.integer > 1 ) {
			sKillLimit = "Kill limit hit.";
		}
		else {
			sKillLimit = "";
			printLimit = qfalse;
		}
	}
	else {
		sKillLimit = "Kill limit hit.";
	}
	if ( level.gametype < GT_SIEGE && fraglimit.integer ) {
		if ( level.teamScores[TEAM_RED] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va( "print \"Red %s\n\"", G_GetStringEdString( "MP_SVGAME", "HIT_THE_KILL_LIMIT" ) ) );
			if ( d_powerDuelPrint.integer ) {
				Com_Printf( "POWERDUEL WIN CONDITION: Kill limit (1)\n" );
			}
			LogExit( sKillLimit );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= fraglimit.integer ) {
			trap->SendServerCommand( -1, va( "print \"Blue %s\n\"", G_GetStringEdString( "MP_SVGAME", "HIT_THE_KILL_LIMIT" ) ) );
			if ( d_powerDuelPrint.integer ) {
				Com_Printf( "POWERDUEL WIN CONDITION: Kill limit (2)\n" );
			}
			LogExit( sKillLimit );
			return;
		}

		for ( i = 0; i < sv_maxclients.integer; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			if ( cl->sess.sessionTeam != TEAM_FREE ) {
				continue;
			}

			if ( (level.gametype == GT_DUEL || level.gametype == GT_POWERDUEL) && duel_fraglimit.integer && cl->sess.wins >= duel_fraglimit.integer ) {
				if ( d_powerDuelPrint.integer ) {
					Com_Printf( "POWERDUEL WIN CONDITION: Duel limit hit (1)\n" );
				}
				LogExit( "Duel limit hit." );
				gDuelExit = qtrue;
				trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " hit the win limit.\n\"",
					cl->pers.netname ) );
				return;
			}

			if ( cl->ps.persistant[PERS_SCORE] >= fraglimit.integer ) {
				if ( d_powerDuelPrint.integer ) {
					Com_Printf( "POWERDUEL WIN CONDITION: Kill limit (3)\n" );
				}
				LogExit( sKillLimit );
				gDuelExit = qfalse;
				if ( printLimit ) {
					trap->SendServerCommand( -1, va( "print \"%s" S_COLOR_WHITE " %s.\n\"",
						cl->pers.netname,
						G_GetStringEdString( "MP_SVGAME", "HIT_THE_KILL_LIMIT" )
						)
						);
				}
				return;
			}
		}
	}

	if ( level.gametype >= GT_CTF && capturelimit.integer ) {

		if ( level.teamScores[TEAM_RED] >= capturelimit.integer ) {
			trap->SendServerCommand( -1, va( "print \"%s \"", G_GetStringEdString( "MP_SVGAME", "PRINTREDTEAM" ) ) );
			trap->SendServerCommand( -1, va( "print \"%s.\n\"", G_GetStringEdString( "MP_SVGAME", "HIT_CAPTURE_LIMIT" ) ) );
			LogExit( "Capturelimit hit." );
			return;
		}

		if ( level.teamScores[TEAM_BLUE] >= capturelimit.integer ) {
			trap->SendServerCommand( -1, va( "print \"%s \"", G_GetStringEdString( "MP_SVGAME", "PRINTBLUETEAM" ) ) );
			trap->SendServerCommand( -1, va( "print \"%s.\n\"", G_GetStringEdString( "MP_SVGAME", "HIT_CAPTURE_LIMIT" ) ) );
			LogExit( "Capturelimit hit." );
			return;
		}
	}
}

void G_RemoveDuelist( int team ) {
	int i = 0;
	gentity_t *ent;
	while ( i < MAX_CLIENTS ) {
		ent = &g_entities[i];

		if ( ent->inuse && ent->client && ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->sess.duelTeam == team ) {
			SetTeam( ent, "s", qfalse );
		}
		i++;
	}
}

int g_duelPrintTimer = 0;

// Once a frame, check for changes in tournament player state
void CheckTournament( void ) {
	// check because we run 3 game frames before calling Connect and/or ClientBegin
	// for clients on a map_restart
	//	if ( level.numPlayingClients == 0 && (level.gametype != GT_POWERDUEL) ) {
	//		return;
	//	}

	if ( level.gametype == GT_POWERDUEL ) {
		if ( level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3 ) {
			trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
		}
	}
	else {
		if ( level.numPlayingClients >= 2 ) {
			trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
		}
	}

	if ( level.gametype == GT_DUEL ) {
		// pull in a spectator if needed
		if ( level.numPlayingClients < 2 && !level.intermissiontime && !level.intermissionQueued ) {
			AddTournamentPlayer();

			if ( level.numPlayingClients >= 2 ) {
				trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i", level.sortedClients[0], level.sortedClients[1] ) );
			}
		}

		if ( level.numPlayingClients >= 2 ) {
			// nmckenzie: DUEL_HEALTH
			if ( g_showDuelHealths.integer ) {
				playerState_t *ps1, *ps2;
				ps1 = &level.clients[level.sortedClients[0]].ps;
				ps2 = &level.clients[level.sortedClients[1]].ps;
				trap->SetConfigstring( CS_CLIENT_DUELHEALTHS, va( "%i|%i|!",
					ps1->stats[STAT_HEALTH], ps2->stats[STAT_HEALTH] ) );
			}
		}

		if ( !g_doWarmup.integer ) {
			// don't care about any of this stuff then, just add people and leave me alone
			level.warmupTime = 0;
			return;
		}
		else {
			// if we don't have two players, go back to "waiting for players"
			if ( level.numPlayingClients != 2 ) {
				if ( level.warmupTime != -1 ) {
					level.warmupTime = -1;
					trap->SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
					G_LogPrintf( level.log.console, "Warmup:\n" );
				}
				return;
			}

			if ( level.warmupTime == 0 ) {
				return;
			}

			// if all players have arrived, start the countdown
			if ( level.warmupTime < 0 ) {
				if ( level.numPlayingClients == 2 ) {
					// fudge by -1 to account for extra delays
					level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;

					if ( level.warmupTime < level.time + 3000 ) {
						// this is an unpleasant hack to keep the level from resetting completely on the client
						// this happens when two map_restarts are issued rapidly
						level.warmupTime = level.time + 3000;
					}
					trap->SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
				}
				return;
			}

			// if the warmup time has counted down, restart
			if ( level.time > level.warmupTime ) {
				level.warmupTime += 10000;
				trap->Cvar_Set( "g_restarted", "1" );
				trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
				level.restarted = qtrue;
				return;
			}
		}
	}
	else if ( level.gametype == GT_POWERDUEL ) {
		if ( level.numPlayingClients < 2 ) { //hmm, ok, pull more in.
			g_dontFrickinCheck = qfalse;
		}

		if ( level.numPlayingClients > 3 ) { //umm..yes..lets take care of that then.
			int lone = 0, dbl = 0;

			G_PowerDuelCount( &lone, &dbl, qfalse );
			if ( lone > 1 ) {
				G_RemoveDuelist( DUELTEAM_LONE );
			}
			else if ( dbl > 2 ) {
				G_RemoveDuelist( DUELTEAM_DOUBLE );
			}
		}
		else if ( level.numPlayingClients < 3 ) { //hmm, someone disconnected or something and we need em
			int lone = 0, dbl = 0;

			G_PowerDuelCount( &lone, &dbl, qfalse );
			if ( lone < 1 ) {
				g_dontFrickinCheck = qfalse;
			}
			else if ( dbl < 1 ) {
				g_dontFrickinCheck = qfalse;
			}
		}

		// pull in a spectator if needed
		if ( level.numPlayingClients < 3 && !g_dontFrickinCheck ) {
			AddPowerDuelPlayers();

			if ( level.numPlayingClients >= 3 &&
				G_CanResetDuelists() ) {
				gentity_t *te = G_TempEntity( &vec3_origin, EV_GLOBAL_DUEL );
				te->r.svFlags |= SVF_BROADCAST;
				//this is really pretty nasty, but..
				te->s.otherEntityNum = level.sortedClients[0];
				te->s.otherEntityNum2 = level.sortedClients[1];
				te->s.groundEntityNum = level.sortedClients[2];

				trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );
				G_ResetDuelists();

				g_dontFrickinCheck = qtrue;
			}
			else if ( level.numPlayingClients > 0 || level.numConnectedClients > 0 ) {
				if ( g_duelPrintTimer < level.time ) { //print once every 10 seconds
					int lone = 0, dbl = 0;

					G_PowerDuelCount( &lone, &dbl, qtrue );
					if ( lone < 1 ) {
						G_Announce( G_GetStringEdString( "MP_SVGAME", "DUELMORESINGLE" ) );
					}
					else {
						G_Announce( G_GetStringEdString( "MP_SVGAME", "DUELMOREPAIRED" ) );
					}
					g_duelPrintTimer = level.time + 10000;
				}
			}

			if ( level.numPlayingClients >= 3 && level.numNonSpectatorClients >= 3 ) { //pulled in a needed person
				if ( G_CanResetDuelists() ) {
					gentity_t *te = G_TempEntity( &vec3_origin, EV_GLOBAL_DUEL );
					te->r.svFlags |= SVF_BROADCAST;
					//this is really pretty nasty, but..
					te->s.otherEntityNum = level.sortedClients[0];
					te->s.otherEntityNum2 = level.sortedClients[1];
					te->s.groundEntityNum = level.sortedClients[2];

					trap->SetConfigstring( CS_CLIENT_DUELISTS, va( "%i|%i|%i", level.sortedClients[0], level.sortedClients[1], level.sortedClients[2] ) );

					if ( g_austrian.integer ) {
						G_LogPrintf( level.log.console, "Duel Initiated: %s %d/%d vs %s %d/%d and %s %d/%d, kill limit: %d\n",
							level.clients[level.sortedClients[0]].pers.netname,
							level.clients[level.sortedClients[0]].sess.wins,
							level.clients[level.sortedClients[0]].sess.losses,
							level.clients[level.sortedClients[1]].pers.netname,
							level.clients[level.sortedClients[1]].sess.wins,
							level.clients[level.sortedClients[1]].sess.losses,
							level.clients[level.sortedClients[2]].pers.netname,
							level.clients[level.sortedClients[2]].sess.wins,
							level.clients[level.sortedClients[2]].sess.losses,
							fraglimit.integer );
					}
					//trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
					//FIXME: This seems to cause problems. But we'd like to reset things whenever a new opponent is set.
				}
			}
		}
		else { //if you have proper num of players then don't try to add again
			g_dontFrickinCheck = qtrue;
		}

		level.warmupTime = 0;
		return;
	}
	else if ( level.warmupTime != 0 ) {
		int		counts[TEAM_NUM_TEAMS];
		qboolean	notEnough = qfalse;

		if ( level.gametype >= GT_TEAM ) {
			counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

			if ( counts[TEAM_RED] < 1 || counts[TEAM_BLUE] < 1 ) {
				notEnough = qtrue;
			}
		}
		else if ( level.numPlayingClients < 2 ) {
			notEnough = qtrue;
		}

		if ( notEnough ) {
			if ( level.warmupTime != -1 ) {
				level.warmupTime = -1;
				trap->SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
				G_LogPrintf( level.log.console, "Warmup:\n" );
			}
			return; // still waiting for team members
		}

		if ( level.warmupTime == 0 || level.warmupTime == -1 ) {
			return;
		}

		// if the warmup is changed at the console, restart it
		/*
		if ( g_warmup.modificationCount != level.warmupModificationCount ) {
		level.warmupModificationCount = g_warmup.modificationCount;
		level.warmupTime = -1;
		}
		*/

		// if all players have arrived, start the countdown
		if ( level.warmupTime < 0 ) {
			// fudge by -1 to account for extra delays
			level.warmupTime = level.time + (g_warmup.integer - 1) * 1000;
			trap->SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
			return;
		}

		// if the warmup time has counted down, restart
		if ( level.time > level.warmupTime ) {
			level.warmupTime += 10000;
			trap->Cvar_Set( "g_restarted", "1" );
			trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
			level.restarted = qtrue;
			return;
		}
	}
}

void G_KickAllBots( void ) {
	int i;
	gclient_t	*cl;

	for ( i = 0; i < sv_maxclients.integer; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			continue;
		}
		trap->SendConsoleCommand( EXEC_INSERT, va( "clientkick \"%d\"\n", i ) );
	}
}

void CheckVote( void ) {
	if ( level.voteExecuteTime && level.voteExecuteTime < level.time ) {
		level.voteExecuteTime = 0;
		trap->SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );

		if ( level.votingGametype ) {
			if ( level.gametype != level.votingGametypeTo ) { //If we're voting to a different game type, be sure to refresh all the map stuff
				const char *nextMap = G_RefreshNextMap( level.votingGametypeTo, qtrue );

				if ( level.votingGametypeTo == GT_SIEGE ) { //ok, kick all the bots, cause the aren't supported!
					G_KickAllBots();
					//just in case, set this to 0 too... I guess...maybe?
					//trap->Cvar_Set("bot_minplayers", "0");
				}

				if ( nextMap && nextMap[0] ) {
					trap->SendConsoleCommand( EXEC_APPEND, va( "map %s\n", nextMap ) );
				}
			}
			else { //otherwise, just leave the map until a restart
				G_RefreshNextMap( level.votingGametypeTo, qfalse );
			}

			if ( g_fraglimitVoteCorrection.integer ) { //This means to auto-correct fraglimit when voting to and from duel.
				const int currentGT = level.gametype;
				const int currentFL = fraglimit.integer;
				const int currentTL = timelimit.integer;

				if ( (level.votingGametypeTo == GT_DUEL || level.votingGametypeTo == GT_POWERDUEL) && currentGT != GT_DUEL && currentGT != GT_POWERDUEL ) {
					if ( currentFL > 3 || !currentFL ) { //if voting to duel, and fraglimit is more than 3 (or unlimited), then set it down to 3
						trap->SendConsoleCommand( EXEC_APPEND, "fraglimit 3\n" );
					}
					if ( currentTL ) { //if voting to duel, and timelimit is set, make it unlimited
						trap->SendConsoleCommand( EXEC_APPEND, "timelimit 0\n" );
					}
				}
				else if ( (level.votingGametypeTo != GT_DUEL && level.votingGametypeTo != GT_POWERDUEL) &&
					(currentGT == GT_DUEL || currentGT == GT_POWERDUEL) ) {
					if ( currentFL && currentFL < 20 ) { //if voting from duel, an fraglimit is less than 20, then set it up to 20
						trap->SendConsoleCommand( EXEC_APPEND, "fraglimit 20\n" );
					}
				}
			}

			level.votingGametype = qfalse;
			level.votingGametypeTo = 0;
		}
	}
	if ( !level.voteTime ) {
		return;
	}
	if ( level.time - level.voteTime >= VOTE_TIME || (level.voteYes + level.voteNo == 0 && !level.votePoll) ) {
		trap->SendServerCommand( -1, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEFAILED" ), level.voteStringClean ) );
	}
	else {
		if ( level.voteYes > level.numVotingClients / 2 ) {
			// execute the command, then remove the vote
			trap->SendServerCommand( -1, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEPASSED" ), level.voteStringClean ) );
			if ( !level.votePoll )
				level.voteExecuteTime = level.time + level.voteExecuteDelay;
		}

		// same behavior as a timeout
		else if ( level.voteNo >= (level.numVotingClients + 1) / 2 )
			trap->SendServerCommand( -1, va( "print \"%s (%s)\n\"", G_GetStringEdString( "MP_SVGAME", "VOTEFAILED" ), level.voteStringClean ) );

		else if ( level.votePoll ) {
			static int lastPrint = 0;
			if ( lastPrint < level.time - 5000 ) {
				char msg[MAX_STRING_CHARS - 128];
				Com_sprintf( msg, sizeof(msg), va( "%s\ncalled a poll\n\n%s", level.voteStringPollCreator,
					level.voteStringPoll ) );
				G_Announce( msg );
				trap->Print( "%s\n", msg );
				lastPrint = level.time;
			}
			return;
		}
		else // still waiting for a majority
			return;
	}
	level.voteTime = 0;
	trap->SetConfigstring( CS_VOTE_TIME, "" );
}

void CheckReady( void ) {
	int i = 0, readyCount = 0, playerCount;
	float f = 0.0f, t = 0.0f;
	gentity_t *ent = NULL;
	uint16_t readyMask = 0u;

	if ( !g_doWarmup.integer || !level.warmupTime || !level.numPlayingClients || level.restarted || level.allReady ) {
		return;
	}

	playerCount = level.numPlayingClients;
	for ( i = 0, ent = g_entities; i < sv_maxclients.integer; i++, ent++ ) {
		if ( !ent->inuse || ent->client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( ent->client->pers.ready ) {
			readyCount++;
			if ( i < 16 ) {
				readyMask |= (1 << i);
			}
		}
		if ( ent->r.svFlags & SVF_BOT ) {
			playerCount--;
		}
	}

	for ( i = 0, ent = g_entities; i < sv_maxclients.integer; i++, ent++ ) {
		if ( !ent->inuse || ent->client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		ent->client->ps.stats[STAT_CLIENTS_READY] = readyMask;
	}

	f = (float)readyCount / (float)playerCount;
	t = (japp_readyThreshold.value - f); // 100 players 33 ready 0.50 needed, [f, t] = [0.33333, 0.16666], numRequired = total*t
	if ( f >= japp_readyThreshold.value ) {
		level.warmupTime = level.time + 3000;
		level.allReady = qtrue;
	}
	else if ( playerCount ) {
		static int lastPrint = 0;
		if ( lastPrint < level.time - g_warmupPrintDelay.integer ) {
			char msg[MAX_STRING_CHARS / 2] = { 0 };
			Com_sprintf( msg, sizeof(msg), S_COLOR_GREEN"Waiting for players to ready up!\n%i more needed\n\nType /ready",
				(int)ceilf( (float)(playerCount) * t ) );
			G_Announce( msg );
			lastPrint = level.time;
		}
	}
}

void PrintTeam( int team, char *message ) {
	int i;

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[i].sess.sessionTeam != team ) {
			continue;
		}
		trap->SendServerCommand( i, message );
	}
}

void CheckCvars( void ) {
	static int lastMod = -1;

	if ( g_password.modificationCount != lastMod ) {
		char password[MAX_INFO_STRING];
		char *c = password;
		lastMod = g_password.modificationCount;

		strcpy( password, g_password.string );
		while ( *c ) {
			if ( *c == '%' ) {
				*c = '.';
			}
			c++;
		}
		trap->Cvar_Set( "g_password", password );

		if ( *g_password.string && Q_stricmp( g_password.string, "none" ) ) {
			trap->Cvar_Set( "g_needpass", "1" );
		}
		else {
			trap->Cvar_Set( "g_needpass", "0" );
		}
	}
}

void proxMineThink( gentity_t *ent );
#include "b_local.h"

// Runs thinking code for this frame if necessary
void G_RunThink( gentity_t *ent ) {
	float	thinktime;

	//OSP: pause
	//	If paused, push nextthink
	if ( level.pause.state != PAUSE_NONE ) {
		if ( ent - g_entities >= sv_maxclients.integer && ent->nextthink > level.time )
			ent->nextthink += level.time - level.previousTime;

		// special case, mines need update here
		if ( ent->think == proxMineThink && ent->genericValue15 > level.time )
			ent->genericValue15 += level.time - level.previousTime;
	}

	thinktime = ent->nextthink;
	if ( thinktime <= 0 ) {
		goto runicarus;
	}
	if ( thinktime > level.time ) {
		goto runicarus;
	}

	ent->nextthink = 0;
	if ( !ent->think ) {
		//trap->Error( ERR_DROP, "NULL ent->think");
		goto runicarus;
	}
	ent->think( ent );

runicarus:
	if ( ent->inuse ) {
		SaveNPCGlobals();
		if ( NPCInfo == NULL && ent->NPC != NULL )
			SetNPCGlobals( ent );
		trap->ICARUS_MaintainTaskManager( ent->s.number );
		RestoreNPCGlobals();
	}
}

int g_LastFrameTime = 0;
int g_TimeSinceLastFrame = 0;

qboolean gDoSlowMoDuel = qfalse;
int gSlowMoDuelTime = 0;

//#define _G_FRAME_PERFANAL

void NAV_CheckCalcPaths( void ) {
	if ( navCalcPathTime && navCalcPathTime < level.time ) {//first time we've ever loaded this map...
		vmCvar_t	mapname;
		vmCvar_t	ckSum;

		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap->Cvar_Register( &ckSum, "sv_mapChecksum", "", CVAR_ROM );

		//clear all the failed edges
		trap->Nav_ClearAllFailedEdges();

		//Calculate all paths
		NAV_CalculatePaths( mapname.string, ckSum.integer );

		trap->Nav_CalculatePaths( qfalse );

#ifdef _DEBUG
		if ( fatalErrors )
			Com_Printf( S_COLOR_RED"Not saving .nav file due to fatal nav errors\n" );
		else
#endif
			if ( trap->Nav_Save( mapname.string, ckSum.integer ) == qfalse )
				Com_Printf( "Unable to save navigations data for map \"%s\" (checksum:%d)\n", mapname.string, ckSum.integer );
		navCalcPathTime = 0;
	}
}

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime( void ) {
	return level.time;
}

void ClearNPCGlobals( void );
void AI_UpdateGroups( void );
void SiegeCheckTimers( void );
void WP_SaberStartMissileBlockCheck( gentity_t *self, usercmd_t *ucmd );
extern void Jedi_Decloak( gentity_t *self );
qboolean G_PointInBounds( vector3 *point, vector3 *mins, vector3 *maxs );

int g_siegeRespawnCheck = 0;
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );

// Advances the non-player objects in the world
void G_RunFrame( int levelTime ) {
	int			i;
	gentity_t	*ent;
#ifdef _G_FRAME_PERFANAL
	int			iTimer_ItemRun = 0;
	int			iTimer_ROFF = 0;
	int			iTimer_ClientEndframe = 0;
	int			iTimer_GameChecks = 0;
	int			iTimer_Queues = 0;
	void		*timer_ItemRun;
	void		*timer_ROFF;
	void		*timer_ClientEndframe;
	void		*timer_GameChecks;
	void		*timer_Queues;
#endif
	static int lastMsgTime = 0;

	if ( level.gametype == GT_SIEGE &&
		g_siegeRespawn.integer &&
		g_siegeRespawnCheck < level.time ) { //check for a respawn wave
		int i = 0;
		gentity_t *clEnt;
		while ( i < MAX_CLIENTS ) {
			clEnt = &g_entities[i];

			if ( clEnt->inuse && clEnt->client &&
				clEnt->client->tempSpectate >= level.time &&
				clEnt->client->sess.sessionTeam != TEAM_SPECTATOR ) {
				respawn( clEnt );
				clEnt->client->tempSpectate = 0;
			}
			i++;
		}

		g_siegeRespawnCheck = level.time + g_siegeRespawn.integer * 1000;
	}

	if ( gDoSlowMoDuel ) {
		if ( level.restarted ) {
			char buf[128];
			float tFVal = 0;

			trap->Cvar_VariableStringBuffer( "timescale", buf, sizeof(buf) );

			tFVal = atof( buf );

			trap->Cvar_Set( "timescale", "1" );
			if ( tFVal == 1.0f ) {
				gDoSlowMoDuel = qfalse;
			}
		}
		else {
			float timeDif = (level.time - gSlowMoDuelTime); //difference in time between when the slow motion was initiated and now
			float useDif = 0; //the difference to use when actually setting the timescale

			if ( timeDif < 150 ) {
				trap->Cvar_Set( "timescale", "0.1f" );
			}
			else if ( timeDif < 1150 ) {
				useDif = (timeDif / 1000); //scale from 0.1f up to 1
				if ( useDif < 0.1f ) {
					useDif = 0.1f;
				}
				if ( useDif > 1.0f ) {
					useDif = 1.0f;
				}
				trap->Cvar_Set( "timescale", va( "%f", useDif ) );
			}
			else {
				char buf[128];
				float tFVal = 0;

				trap->Cvar_VariableStringBuffer( "timescale", buf, sizeof(buf) );

				tFVal = atof( buf );

				trap->Cvar_Set( "timescale", "1" );
				if ( timeDif > 1500 && tFVal == 1.0f ) {
					gDoSlowMoDuel = qfalse;
				}
			}
		}
	}

	// if we are waiting for the level to restart, do nothing
	if ( level.restarted ) {
		return;
	}

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;

	//OSP: pause
	if ( level.pause.state != PAUSE_NONE ) {
		static int lastCSTime = 0;
		int dt = level.time - level.previousTime;

		// compensate for timelimit and warmup time
		if ( level.warmupTime > 0 )
			level.warmupTime += dt;
		level.startTime += dt;

		// floor start time to avoid time flipering
		if ( (level.time - level.startTime) % 1000 >= 500 )
			level.startTime += (level.time - level.startTime) % 1000;

		// initial CS update time, needed!
		if ( !lastCSTime )
			lastCSTime = level.time;

		// client needs to do the same, just adjust the configstrings periodically
		// i can't see a way around this mess without requiring a client mod.
		if ( lastCSTime < level.time - 500 ) {
			lastCSTime += 500;
			trap->SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );
			if ( level.warmupTime > 0 )
				trap->SetConfigstring( CS_WARMUP, va( "%i", level.warmupTime ) );
		}
	}
	if ( level.pause.state == PAUSE_PAUSED ) {
		if ( lastMsgTime < level.time - 500 ) {
			G_Announce( va( "Match has been paused.\n%.0f seconds remaining",
				ceilf( (level.pause.time - level.time) / 1000.0f ) ) );
			lastMsgTime = level.time;
		}

		if ( level.time > level.pause.time - (japp_unpauseTime.integer * 1000) )
			level.pause.state = PAUSE_UNPAUSING;
	}
	if ( level.pause.state == PAUSE_UNPAUSING ) {
		if ( lastMsgTime < level.time - 500 ) {
			G_Announce( va( "MATCH IS UNPAUSING\nin %.0f...", ceilf( (level.pause.time - level.time) / 1000.0f ) ) );
			lastMsgTime = level.time;
		}

		if ( level.time > level.pause.time ) {
			level.pause.state = PAUSE_NONE;
			trap->SendServerCommand( -1, "cp \"Fight!\n\"" );
		}
	}

	if ( g_allowNPC.integer ) {
		NAV_CheckCalcPaths();
	}

	AI_UpdateGroups();

	if ( g_allowNPC.integer ) {
		if ( d_altRoutes.integer ) {
			trap->Nav_CheckAllFailedEdges();
		}
		trap->Nav_ClearCheckedNodes();

		//remember last waypoint, clear current one
		for ( i = 0; i < level.num_entities; i++ ) {
			ent = &g_entities[i];

			if ( !ent->inuse )
				continue;

			if ( ent->waypoint != WAYPOINT_NONE
				&& ent->noWaypointTime < level.time ) {
				ent->lastWaypoint = ent->waypoint;
				ent->waypoint = WAYPOINT_NONE;
			}
			if ( d_altRoutes.integer ) {
				trap->Nav_CheckFailedNodes( (sharedEntity_t *)ent );
			}
		}

		//Look to clear out old events
		ClearPlayerAlertEvents();
	}

	g_TimeSinceLastFrame = (level.time - g_LastFrameTime);

	// get any cvar changes
	G_UpdateCvars();



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start( &timer_ItemRun );
#endif
	//
	// go through all allocated objects
	//
	ent = &g_entities[0];
	for ( i = 0; i<level.num_entities; i++, ent++ ) {
		if ( !ent->inuse ) {
			continue;
		}

		// clear events that are too old
		if ( level.time - ent->eventTime > EVENT_VALID_MSEC ) {
			if ( ent->s.event ) {
				ent->s.event = 0;	// &= EV_EVENT_BITS;
				if ( ent->client ) {
					ent->client->ps.externalEvent = 0;
					// predicted events should never be set to zero
					//ent->client->ps.events[0] = 0;
					//ent->client->ps.events[1] = 0;
				}
			}
			if ( ent->freeAfterEvent ) {
				// tempEntities or dropped items completely go away after their event
				if ( ent->s.eFlags & EF_SOUNDTRACKER ) { //don't trigger the event again..
					ent->s.event = 0;
					ent->s.eventParm = 0;
					ent->s.eType = 0;
					ent->eventTime = 0;
				}
				else {
					G_FreeEntity( ent );
					continue;
				}
			}
			else if ( ent->unlinkAfterEvent ) {
				// items that will respawn will hide themselves after their pickup event
				ent->unlinkAfterEvent = qfalse;
				trap->UnlinkEntity( (sharedEntity_t *)ent );
			}
		}

		// temporary entities don't think
		if ( ent->freeAfterEvent ) {
			continue;
		}

		if ( !ent->r.linked && ent->neverFree ) {
			continue;
		}

		if ( ent->s.eType == ET_MISSILE ) {
			//OSP: pause
			if ( level.pause.state == PAUSE_NONE )
				G_RunMissile( ent );
			else {// During a pause, gotta keep track of stuff in the air
				ent->s.pos.trTime += level.time - level.previousTime;
				G_RunThink( ent );
			}
			continue;
		}

		if ( ent->s.eType == ET_ITEM || ent->physicsObject ) {
#if 0 //use if body dragging enabled?
			if ( ent->s.eType == ET_BODY ) { //special case for bodies
				float grav = 3.0f;
				float mass = 0.14f;
				float bounce = 1.15f;

				G_RunExPhys( ent, grav, mass, bounce, qfalse, NULL, 0 );
			}
			else {
				G_RunItem( ent );
			}
#else
			G_RunItem( ent );
#endif
			continue;
		}

		if ( ent->s.eType == ET_MOVER ) {
			G_RunMover( ent );
			continue;
		}

		//fix for self-deactivating areaportals in Siege
		if ( ent->s.eType == ET_MOVER && level.gametype == GT_SIEGE && level.intermissiontime ) {
			if ( !Q_stricmp( "func_door", ent->classname ) && ent->moverState != MOVER_POS1 ) {
				SetMoverState( ent, MOVER_POS1, level.time );
				if ( ent->teammaster == ent || !ent->teammaster ) {
					trap->AdjustAreaPortalState( (sharedEntity_t *)ent, qfalse );
				}

				//stop the looping sound
				ent->s.loopSound = 0;
				ent->s.loopIsSoundset = qfalse;
			}
			continue;
		}

		if ( i < MAX_CLIENTS ) {
			G_CheckClientTimeouts( ent );

			if ( ent->client->inSpaceIndex && ent->client->inSpaceIndex != ENTITYNUM_NONE ) { //we're in space, check for suffocating and for exiting
				gentity_t *spacetrigger = &g_entities[ent->client->inSpaceIndex];

				if ( !spacetrigger->inuse ||
					!G_PointInBounds( &ent->client->ps.origin, &spacetrigger->r.absmin, &spacetrigger->r.absmax ) ) { //no longer in space then I suppose
					ent->client->inSpaceIndex = 0;
				}
				else { //check for suffocation
					if ( ent->client->inSpaceSuffocation < level.time ) { //suffocate!
						if ( ent->health > 0 && ent->takedamage ) { //if they're still alive..
							G_Damage( ent, spacetrigger, spacetrigger, NULL, &ent->client->ps.origin, Q_irand( 50, 70 ), DAMAGE_NO_ARMOR, MOD_SUICIDE );

							if ( ent->health > 0 ) { //did that last one kill them?
								//play the choking sound
								G_EntitySound( ent, CHAN_VOICE, G_SoundIndex( va( "*choke%d.wav", Q_irand( 1, 3 ) ) ) );

								//make them grasp their throat
								ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
								ent->client->ps.forceHandExtendTime = level.time + 2000;
							}
						}

						ent->client->inSpaceSuffocation = level.time + Q_irand( 100, 200 );
					}
				}
			}

			if ( ent->client->isHacking && level.pause.state == PAUSE_NONE ) { //hacking checks
				gentity_t *hacked = &g_entities[ent->client->isHacking];
				vector3 angDif;

				VectorSubtract( &ent->client->ps.viewangles, &ent->client->hackingAngles, &angDif );

				//keep him in the "use" anim
				if ( ent->client->ps.torsoAnim != BOTH_CONSOLE1 )
					G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_CONSOLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
				else
					ent->client->ps.torsoTimer = 500;
				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;

				if ( !(ent->client->pers.cmd.buttons & BUTTON_USE) ) { //have to keep holding use
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if ( !hacked || !hacked->inuse ) { //shouldn't happen, but safety first
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if ( !G_PointInBounds( &ent->client->ps.origin, &hacked->r.absmin, &hacked->r.absmax ) ) { //they stepped outside the thing they're hacking, so reset hacking time
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
				else if ( VectorLength( &angDif ) > 10.0f ) { //must remain facing generally the same angle as when we start
					ent->client->isHacking = 0;
					ent->client->ps.hackingTime = 0;
				}
			}

#define JETPACK_DEFUEL_RATE		200 //approx. 20 seconds of idle use from a fully charged fuel amt
#define JETPACK_REFUEL_RATE		150 //seems fair
			if ( ent->client->jetPackOn && level.pause.state == PAUSE_NONE ) { //using jetpack, drain fuel
				if ( ent->client->jetPackDebReduce < level.time ) {
					if ( ent->client->pers.cmd.upmove > 0 ) { //take more if they're thrusting
						ent->client->ps.jetpackFuel -= 2;
					}
					else {
						ent->client->ps.jetpackFuel--;
					}

					if ( ent->client->ps.jetpackFuel <= 0 ) { //turn it off
						ent->client->ps.jetpackFuel = 0;
						Jetpack_Off( ent );
					}
					ent->client->jetPackDebReduce = level.time + JETPACK_DEFUEL_RATE;
				}
			}
			else if ( ent->client->ps.jetpackFuel < 100 && level.pause.state == PAUSE_NONE ) { //recharge jetpack
				if ( ent->client->jetPackDebRecharge < level.time ) {
					ent->client->ps.jetpackFuel++;
					ent->client->jetPackDebRecharge = level.time + JETPACK_REFUEL_RATE;
				}
			}

#define CLOAK_DEFUEL_RATE		200 //approx. 20 seconds of idle use from a fully charged fuel amt
#define CLOAK_REFUEL_RATE		150 //seems fair
			if ( ent->client->ps.powerups[PW_CLOAKED] && level.pause.state == PAUSE_NONE ) { //using cloak, drain battery
				if ( ent->client->cloakDebReduce < level.time ) {
					ent->client->ps.cloakFuel--;

					if ( ent->client->ps.cloakFuel <= 0 ) { //turn it off
						ent->client->ps.cloakFuel = 0;
						Jedi_Decloak( ent );
					}
					ent->client->cloakDebReduce = level.time + CLOAK_DEFUEL_RATE;
				}
			}
			else if ( ent->client->ps.cloakFuel < 100 ) { //recharge cloak
				if ( ent->client->cloakDebRecharge < level.time ) {
					ent->client->ps.cloakFuel++;
					ent->client->cloakDebRecharge = level.time + CLOAK_REFUEL_RATE;
				}
			}

			if ( level.gametype == GT_SIEGE &&
				ent->client->siegeClass != -1 &&
				(bgSiegeClasses[ent->client->siegeClass].classflags & (1 << CFL_STATVIEWER)) ) { //see if it's time to send this guy an update of extended info
				if ( ent->client->siegeEDataSend < level.time ) {
					G_SiegeClientExData( ent );
					ent->client->siegeEDataSend = level.time + 1000; //once every sec seems ok
				}
			}

			if ( level.pause.state == PAUSE_NONE
				&& !level.intermissiontime
				&& !(ent->client->ps.pm_flags & PMF_FOLLOW)
				&& ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
				WP_ForcePowersUpdate( ent, &ent->client->pers.cmd );
				WP_SaberPositionUpdate( ent, &ent->client->pers.cmd );
				WP_SaberStartMissileBlockCheck( ent, &ent->client->pers.cmd );
			}

			if ( g_allowNPC.integer ) {
				//This was originally intended to only be done for client 0.
				//Make sure it doesn't slow things down too much with lots of clients in game.
				NAV_FindPlayerWaypoint( i );
			}

			trap->ICARUS_MaintainTaskManager( ent->s.number );

			G_RunClient( ent );
			continue;
		}
		else if ( ent->s.eType == ET_NPC ) {
			int j;
			// turn off any expired powerups
			for ( j = 0; j < MAX_POWERUPS; j++ ) {
				if ( ent->client->ps.powerups[j] < level.time ) {
					ent->client->ps.powerups[j] = 0;
				}
			}

			WP_ForcePowersUpdate( ent, &ent->client->pers.cmd );
			WP_SaberPositionUpdate( ent, &ent->client->pers.cmd );
			WP_SaberStartMissileBlockCheck( ent, &ent->client->pers.cmd );
		}

		G_RunThink( ent );

		if ( g_allowNPC.integer ) {
			ClearNPCGlobals();
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ItemRun = trap->PrecisionTimer_End( timer_ItemRun );
#endif

	SiegeCheckTimers();

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start( &timer_ROFF );
#endif
	trap->ROFF_UpdateEntities();
#ifdef _G_FRAME_PERFANAL
	iTimer_ROFF = trap->PrecisionTimer_End( timer_ROFF );
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start( &timer_ClientEndframe );
#endif
	// perform final fixups on the players
	ent = &g_entities[0];
	for ( i = 0; i < level.maxclients; i++, ent++ ) {
		if ( ent->inuse ) {
			ClientEndFrame( ent );
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_ClientEndframe = trap->PrecisionTimer_End( timer_ClientEndframe );
#endif



#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start( &timer_GameChecks );
#endif
	// see if it is time to do a tournement restart
	CheckTournament();

	// see if it is time to end the level
	CheckExitRules();

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// check for allready during warmup
	CheckReady();

	// for tracking changes
	CheckCvars();

#ifdef _G_FRAME_PERFANAL
	iTimer_GameChecks = trap->PrecisionTimer_End( timer_GameChecks );
#endif

#ifdef _G_FRAME_PERFANAL
	trap->PrecisionTimer_Start( &timer_Queues );
#endif
	//At the end of the frame, send out the ghoul2 kill queue, if there is one
	G_SendG2KillQueue();

	if ( gQueueScoreMessage ) {
		if ( gQueueScoreMessageTime < level.time ) {
			SendScoreboardMessageToAllClients();

			gQueueScoreMessageTime = 0;
			gQueueScoreMessage = 0;
		}
	}
#ifdef _G_FRAME_PERFANAL
	iTimer_Queues = trap->PrecisionTimer_End( timer_Queues );
#endif


	JPLua_Event_RunFrame();


#ifdef _G_FRAME_PERFANAL
	Com_Printf( "---------------\nItemRun: %i\nROFF: %i\nClientEndframe: %i\nGameChecks: %i\nQueues: %i\n---------------\n",
		iTimer_ItemRun,
		iTimer_ROFF,
		iTimer_ClientEndframe,
		iTimer_GameChecks,
		iTimer_Queues );
#endif

	g_LastFrameTime = level.time;
}

const char *G_GetStringEdString( const char *refSection, const char *refName ) {
	// Well, it would've been lovely doing it the above way, but it would mean mixing languages for the client depending
	//	on what the server is. So we'll mark this as a stringed reference with @@@ and send the refname to the client,
	//	and when it goes to print it will get scanned for the stringed reference indication and dealt with properly.

	static char text[1024] = { 0 };

	Com_sprintf( text, sizeof(text), "@@@%s", refName );

	return text;
}

static void G_SpawnRMGEntity( void ) {
	if ( G_ParseSpawnVars( qfalse ) )
		G_SpawnGEntityFromSpawnVars( qfalse );
}

static void _G_ROFF_NotetrackCallback( int entID, const char *notetrack ) {
	G_ROFF_NotetrackCallback( &g_entities[entID], notetrack );
}

static int G_ICARUS_PlaySound( void ) {
	T_G_ICARUS_PLAYSOUND *sharedMem = (T_G_ICARUS_PLAYSOUND *)gSharedBuffer;
	return Q3_PlaySound( sharedMem->taskID, sharedMem->entID, sharedMem->name, sharedMem->channel );
}
static qboolean G_ICARUS_Set( void ) {
	T_G_ICARUS_SET *sharedMem = (T_G_ICARUS_SET *)gSharedBuffer;
	return Q3_Set( sharedMem->taskID, sharedMem->entID, sharedMem->type_name, sharedMem->data );
}
static void G_ICARUS_Lerp2Pos( void ) {
	T_G_ICARUS_LERP2POS *sharedMem = (T_G_ICARUS_LERP2POS *)gSharedBuffer;
	Q3_Lerp2Pos( sharedMem->taskID, sharedMem->entID, &sharedMem->origin, sharedMem->nullAngles ? NULL : &sharedMem->angles, sharedMem->duration );
}
static void G_ICARUS_Lerp2Origin( void ) {
	T_G_ICARUS_LERP2ORIGIN *sharedMem = (T_G_ICARUS_LERP2ORIGIN *)gSharedBuffer;
	Q3_Lerp2Origin( sharedMem->taskID, sharedMem->entID, &sharedMem->origin, sharedMem->duration );
}
static void G_ICARUS_Lerp2Angles( void ) {
	T_G_ICARUS_LERP2ANGLES *sharedMem = (T_G_ICARUS_LERP2ANGLES *)gSharedBuffer;
	Q3_Lerp2Angles( sharedMem->taskID, sharedMem->entID, &sharedMem->angles, sharedMem->duration );
}
static int G_ICARUS_GetTag( void ) {
	T_G_ICARUS_GETTAG *sharedMem = (T_G_ICARUS_GETTAG *)gSharedBuffer;
	return Q3_GetTag( sharedMem->entID, sharedMem->name, sharedMem->lookup, &sharedMem->info );
}
static void G_ICARUS_Lerp2Start( void ) {
	T_G_ICARUS_LERP2START *sharedMem = (T_G_ICARUS_LERP2START *)gSharedBuffer;
	Q3_Lerp2Start( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Lerp2End( void ) {
	T_G_ICARUS_LERP2END *sharedMem = (T_G_ICARUS_LERP2END *)gSharedBuffer;
	Q3_Lerp2End( sharedMem->entID, sharedMem->taskID, sharedMem->duration );
}
static void G_ICARUS_Use( void ) {
	T_G_ICARUS_USE *sharedMem = (T_G_ICARUS_USE *)gSharedBuffer;
	Q3_Use( sharedMem->entID, sharedMem->target );
}
static void G_ICARUS_Kill( void ) {
	T_G_ICARUS_KILL *sharedMem = (T_G_ICARUS_KILL *)gSharedBuffer;
	Q3_Kill( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Remove( void ) {
	T_G_ICARUS_REMOVE *sharedMem = (T_G_ICARUS_REMOVE *)gSharedBuffer;
	Q3_Remove( sharedMem->entID, sharedMem->name );
}
static void G_ICARUS_Play( void ) {
	T_G_ICARUS_PLAY *sharedMem = (T_G_ICARUS_PLAY *)gSharedBuffer;
	Q3_Play( sharedMem->taskID, sharedMem->entID, sharedMem->type, sharedMem->name );
}
static int G_ICARUS_GetFloat( void ) {
	T_G_ICARUS_GETFLOAT *sharedMem = (T_G_ICARUS_GETFLOAT *)gSharedBuffer;
	return Q3_GetFloat( sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value );
}
static int G_ICARUS_GetVector( void ) {
	T_G_ICARUS_GETVECTOR *sharedMem = (T_G_ICARUS_GETVECTOR *)gSharedBuffer;
	return Q3_GetVector( sharedMem->entID, sharedMem->type, sharedMem->name, &sharedMem->value );
}
static int G_ICARUS_GetString( void ) {
	T_G_ICARUS_GETSTRING *sharedMem = (T_G_ICARUS_GETSTRING *)gSharedBuffer;
	char *crap = NULL; //I am sorry for this -rww
	char **morecrap = &crap; //and this
	int r = Q3_GetString( sharedMem->entID, sharedMem->type, sharedMem->name, morecrap );

	if ( crap )
		strcpy( sharedMem->value, crap );

	return r;
}
static void G_ICARUS_SoundIndex( void ) {
	T_G_ICARUS_SOUNDINDEX *sharedMem = (T_G_ICARUS_SOUNDINDEX *)gSharedBuffer;
	G_SoundIndex( sharedMem->filename );
}
static int G_ICARUS_GetSetIDForString( void ) {
	T_G_ICARUS_GETSETIDFORSTRING *sharedMem = (T_G_ICARUS_GETSETIDFORSTRING *)gSharedBuffer;
	return GetIDForString( setTable, sharedMem->string );
}
static qboolean G_NAV_ClearPathToPoint( int entID, vector3 *pmins, vector3 *pmaxs, vector3 *point, int clipmask, int okToHitEnt ) {
	return NAV_ClearPathToPoint( &g_entities[entID], pmins, pmaxs, point, clipmask, okToHitEnt );
}
static qboolean G_NPC_ClearLOS2( int entID, const vector3 *end ) {
	return NPC_ClearLOS2( &g_entities[entID], end );
}
static qboolean	G_NAV_CheckNodeFailedForEnt( int entID, int nodeNum ) {
	return NAV_CheckNodeFailedForEnt( &g_entities[entID], nodeNum );
}

gameImport_t *trap = NULL;
Q_EXPORT gameExport_t* QDECL GetModuleAPI( int apiVersion, gameImport_t *import ) {
	static gameExport_t ge = { 0 };

	assert( import );
	trap = import;
	Com_Printf = trap->Print;
	Com_Error = trap->Error;

	memset( &ge, 0, sizeof(ge) );

	if ( apiVersion != GAME_API_VERSION ) {
		trap->Print( "Mismatched GAME_API_VERSION: expected %i, got %i\n", GAME_API_VERSION, apiVersion );
		return NULL;
	}

	ge.InitGame = G_InitGame;
	ge.ShutdownGame = G_ShutdownGame;
	ge.ClientConnect = ClientConnect;
	ge.ClientBegin = ClientBegin;
	ge.ClientUserinfoChanged = ClientUserinfoChanged;
	ge.ClientDisconnect = ClientDisconnect;
	ge.ClientCommand = ClientCommand;
	ge.ClientThink = ClientThink;
	ge.RunFrame = G_RunFrame;
	ge.ConsoleCommand = ConsoleCommand;
	ge.BotAIStartFrame = BotAIStartFrame;
	ge.ROFF_NotetrackCallback = _G_ROFF_NotetrackCallback;
	ge.SpawnRMGEntity = G_SpawnRMGEntity;
	ge.ICARUS_PlaySound = G_ICARUS_PlaySound;
	ge.ICARUS_Set = G_ICARUS_Set;
	ge.ICARUS_Lerp2Pos = G_ICARUS_Lerp2Pos;
	ge.ICARUS_Lerp2Origin = G_ICARUS_Lerp2Origin;
	ge.ICARUS_Lerp2Angles = G_ICARUS_Lerp2Angles;
	ge.ICARUS_GetTag = G_ICARUS_GetTag;
	ge.ICARUS_Lerp2Start = G_ICARUS_Lerp2Start;
	ge.ICARUS_Lerp2End = G_ICARUS_Lerp2End;
	ge.ICARUS_Use = G_ICARUS_Use;
	ge.ICARUS_Kill = G_ICARUS_Kill;
	ge.ICARUS_Remove = G_ICARUS_Remove;
	ge.ICARUS_Play = G_ICARUS_Play;
	ge.ICARUS_GetFloat = G_ICARUS_GetFloat;
	ge.ICARUS_GetVector = G_ICARUS_GetVector;
	ge.ICARUS_GetString = G_ICARUS_GetString;
	ge.ICARUS_SoundIndex = G_ICARUS_SoundIndex;
	ge.ICARUS_GetSetIDForString = G_ICARUS_GetSetIDForString;
	ge.NAV_ClearPathToPoint = G_NAV_ClearPathToPoint;
	ge.NPC_ClearLOS2 = G_NPC_ClearLOS2;
	ge.NAVNEW_ClearPathBetweenPoints = NAVNEW_ClearPathBetweenPoints;
	ge.NAV_CheckNodeFailedForEnt = G_NAV_CheckNodeFailedForEnt;
	ge.NAV_EntIsUnlockedDoor = G_EntIsUnlockedDoor;
	ge.NAV_EntIsDoor = G_EntIsDoor;
	ge.NAV_EntIsBreakable = G_EntIsBreakable;
	ge.NAV_EntIsRemovableUsable = G_EntIsRemovableUsable;
	ge.NAV_FindCombatPointWaypoints = CP_FindCombatPointWaypoints;
	ge.BG_GetItemIndexByTag = BG_GetItemIndexByTag;

	return &ge;
}

// This is the only way control passes into the module.
// This must be the very first function compiled into the .q3vm file
Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 ) {
	switch ( command ) {
	case GAME_INIT:
		G_InitGame( arg0, arg1, arg2 );
		return 0;

	case GAME_SHUTDOWN:
		G_ShutdownGame( arg0 );
		return 0;

	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect( arg0, arg1, arg2 );

	case GAME_CLIENT_THINK:
		ClientThink( arg0, NULL );
		return 0;

	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged( arg0 );
		return 0;

	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect( arg0 );
		return 0;

	case GAME_CLIENT_BEGIN:
		ClientBegin( arg0, qtrue );
		return 0;

	case GAME_CLIENT_COMMAND:
		ClientCommand( arg0 );
		return 0;

	case GAME_RUN_FRAME:
		G_RunFrame( arg0 );
		return 0;

	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();

	case BOTAI_START_FRAME:
		return BotAIStartFrame( arg0 );

	case GAME_ROFF_NOTETRACK_CALLBACK:
		_G_ROFF_NotetrackCallback( arg0, (const char *)arg1 );
		return 0;

	case GAME_SPAWN_RMG_ENTITY:
		G_SpawnRMGEntity();
		return 0;

	case GAME_ICARUS_PLAYSOUND:
		return G_ICARUS_PlaySound();

	case GAME_ICARUS_SET:
		return G_ICARUS_Set();

	case GAME_ICARUS_LERP2POS:
		G_ICARUS_Lerp2Pos();
		return 0;

	case GAME_ICARUS_LERP2ORIGIN:
		G_ICARUS_Lerp2Origin();
		return 0;

	case GAME_ICARUS_LERP2ANGLES:
		G_ICARUS_Lerp2Angles();
		return 0;

	case GAME_ICARUS_GETTAG:
		return G_ICARUS_GetTag();

	case GAME_ICARUS_LERP2START:
		G_ICARUS_Lerp2Start();
		return 0;

	case GAME_ICARUS_LERP2END:
		G_ICARUS_Lerp2End();
		return 0;

	case GAME_ICARUS_USE:
		G_ICARUS_Use();
		return 0;

	case GAME_ICARUS_KILL:
		G_ICARUS_Kill();
		return 0;

	case GAME_ICARUS_REMOVE:
		G_ICARUS_Remove();
		return 0;

	case GAME_ICARUS_PLAY:
		G_ICARUS_Play();
		return 0;

	case GAME_ICARUS_GETFLOAT:
		return G_ICARUS_GetFloat();

	case GAME_ICARUS_GETVECTOR:
		return G_ICARUS_GetVector();

	case GAME_ICARUS_GETSTRING:
		return G_ICARUS_GetString();

	case GAME_ICARUS_SOUNDINDEX:
		G_ICARUS_SoundIndex();
		return 0;

	case GAME_ICARUS_GETSETIDFORSTRING:
		return G_ICARUS_GetSetIDForString();

	case GAME_NAV_CLEARPATHTOPOINT:
		return G_NAV_ClearPathToPoint( arg0, (vector3 *)arg1, (vector3 *)arg2, (vector3 *)arg3, arg4, arg5 );

	case GAME_NAV_CLEARLOS:
		return G_NPC_ClearLOS2( arg0, (const vector3 *)arg1 );

	case GAME_NAV_CLEARPATHBETWEENPOINTS:
		return NAVNEW_ClearPathBetweenPoints( (vector3 *)arg0, (vector3 *)arg1, (vector3 *)arg2, (vector3 *)arg3, arg4, arg5 );

	case GAME_NAV_CHECKNODEFAILEDFORENT:
		return NAV_CheckNodeFailedForEnt( &g_entities[arg0], arg1 );

	case GAME_NAV_ENTISUNLOCKEDDOOR:
		return G_EntIsUnlockedDoor( arg0 );

	case GAME_NAV_ENTISDOOR:
		return G_EntIsDoor( arg0 );

	case GAME_NAV_ENTISBREAKABLE:
		return G_EntIsBreakable( arg0 );

	case GAME_NAV_ENTISREMOVABLEUSABLE:
		return G_EntIsRemovableUsable( arg0 );

	case GAME_NAV_FINDCOMBATPOINTWAYPOINTS:
		CP_FindCombatPointWaypoints();
		return 0;

	case GAME_GETITEMINDEXBYTAG:
		return BG_GetItemIndexByTag( arg0, arg1 );

	default:
		break;
	}

	return -1;
}
