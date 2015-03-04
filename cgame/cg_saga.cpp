#include "cg_local.h"
#include "bg_saga.h"

int cgSiegeRoundState = 0;
int cgSiegeRoundTime = 0;

static char team1[512], team2[512];
int team1Timed = 0, team2Timed = 0;
int cgSiegeTeam1PlShader = 0, cgSiegeTeam2PlShader = 0;

static char cgParseObjectives[MAX_SIEGE_INFO_SIZE];

extern void CG_LoadCISounds( clientInfo_t *ci, qboolean modelloaded );

void CG_DrawSiegeMessage( const char *str, int objectiveScreen );
void CG_DrawSiegeMessageNonMenu( const char *str );
void CG_SiegeBriefingDisplay( int team, qboolean dontShow );

void CG_PrecacheSiegeObjectiveAssetsForTeam( int myTeam ) {
	char teamstr[64], objstr[256];
	static char foundobjective[MAX_SIEGE_INFO_SIZE];

	//Raz: moved to heap
	foundobjective[0] = '\0';

	if ( !siege_valid ) {
		trap->Error( ERR_DROP, "Siege data does not exist on client!\n" );
		return;
	}

	if ( myTeam == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		int i;
		for ( i = 1; i < 32; i++ ) {
			// eh, just try 32 I guess
			Com_sprintf( objstr, sizeof(objstr), "Objective%i", i );

			if ( BG_SiegeGetValueGroup( cgParseObjectives, objstr, foundobjective ) ) {
				char str[MAX_QPATH];

				if ( BG_SiegeGetPairedValue( foundobjective, "sound_team1", str ) )
					trap->S_RegisterSound( str );
				if ( BG_SiegeGetPairedValue( foundobjective, "sound_team2", str ) )
					trap->S_RegisterSound( str );
				if ( BG_SiegeGetPairedValue( foundobjective, "objgfx", str ) )
					trap->R_RegisterShaderNoMip( str );
				if ( BG_SiegeGetPairedValue( foundobjective, "mapicon", str ) )
					trap->R_RegisterShaderNoMip( str );
				if ( BG_SiegeGetPairedValue( foundobjective, "litmapicon", str ) )
					trap->R_RegisterShaderNoMip( str );
				if ( BG_SiegeGetPairedValue( foundobjective, "donemapicon", str ) )
					trap->R_RegisterShaderNoMip( str );
			}
			else
				break;
		}
	}
}

void CG_PrecachePlayersForSiegeTeam( int team ) {
	siegeTeam_t *stm;
	int i;

	stm = BG_SiegeFindThemeForTeam( team );

	if ( !stm )
		return;

	for ( i = 0; i < stm->numClasses; i++ ) {
		siegeClass_t *scl = stm->classes[i];

		if ( scl->forcedModel[0] ) {
			clientInfo_t fake;

			memset( &fake, 0, sizeof(fake) );
			Q_strncpyz( fake.modelName, scl->forcedModel, sizeof(fake.modelName) );

			trap->R_RegisterModel( va( "models/players/%s/model.glm", scl->forcedModel ) );
			if ( scl->forcedSkin[0] ) {
				trap->R_RegisterSkin( va( "models/players/%s/model_%s.skin", scl->forcedModel, scl->forcedSkin ) );
				Q_strncpyz( fake.skinName, scl->forcedSkin, sizeof(fake.skinName) );
			}
			else
				Q_strncpyz( fake.skinName, "default", sizeof(fake.skinName) );

			//precache the sounds for the model...
			CG_LoadCISounds( &fake, qtrue );
		}
	}
}

void CG_InitSiegeMode( void ) {
	char levelname[MAX_QPATH] = { '\0' };
	char btime[1024] = { '\0' };
	char teams[2048] = { '\0' };
	char teamIcon[MAX_QPATH] = { '\0' };
	const char *s;
	static char teamInfo[MAX_SIEGE_INFO_SIZE] = { 0 };
	int len = 0, i = 0, j = 0;
	siegeClass_t *cl;
	siegeTeam_t *sTeam;
	fileHandle_t f;

	//Raz: moved to the heap
	teamInfo[0] = '\0';

	s = CG_ConfigString( CS_SIEGE_STATE );
	if ( s[0] ) {
		CG_ParseSiegeState( s );
	}

	s = CG_ConfigString( CS_SIEGE_WINTEAM );
	if ( s[0] ) {
		cg_siegeWinTeam = atoi( s );
	}

	if ( cgs.gametype == GT_SIEGE ) {
		CG_ParseSiegeObjectiveStatus( CG_ConfigString( CS_SIEGE_OBJECTIVES ) );
		cg_beatingSiegeTime = atoi( CG_ConfigString( CS_SIEGE_TIMEOVERRIDE ) );
		if ( cg_beatingSiegeTime ) {
			CG_SetSiegeTimerCvar( cg_beatingSiegeTime );
		}
	}

	if ( cgs.gametype != GT_SIEGE ) {
		goto failure;
	}

	// grab the .siege file
	Com_sprintf( levelname, sizeof(levelname), "maps/%s.siege", cgs.mapnameClean );

	len = trap->FS_Open( levelname, &f, FS_READ );
	if ( !f || len <= 0 || len >= MAX_SIEGE_INFO_SIZE ) {
		goto failure;
	}

	trap->FS_Read( siege_info, len, f );
	trap->FS_Close( f );

	siege_valid = qtrue;

	if ( BG_SiegeGetValueGroup( siege_info, "Teams", teams ) ) {
		char buf[1024];

		trap->Cvar_VariableStringBuffer( "cg_siegeTeam1", buf, sizeof(buf) );
		if ( buf[0] && Q_stricmp( buf, "none" ) ) {
			Q_strncpyz( team1, buf, sizeof(team1) );
		}
		else {
			BG_SiegeGetPairedValue( teams, "team1", team1 );
		}

		if ( team1[0] == '@' ) {
			// it's a damn stringed reference.
			char b[256];
			trap->SE_GetStringTextString( team1 + 1, b, sizeof(b) );
			trap->Cvar_Set( "cg_siegeTeam1Name", b );
		}
		else {
			trap->Cvar_Set( "cg_siegeTeam1Name", team1 );
		}

		trap->Cvar_VariableStringBuffer( "cg_siegeTeam2", buf, sizeof(buf) );
		if ( buf[0] && Q_stricmp( buf, "none" ) ) {
			Q_strncpyz( team2, buf, sizeof(team2) );
		}
		else {
			BG_SiegeGetPairedValue( teams, "team2", team2 );
		}

		if ( team2[0] == '@' ) {
			// it's a damn stringed reference.
			char b[256];
			trap->SE_GetStringTextString( team2 + 1, b, sizeof(b) );
			trap->Cvar_Set( "cg_siegeTeam2Name", b );
		}
		else {
			trap->Cvar_Set( "cg_siegeTeam2Name", team2 );
		}
	}
	else {
		trap->Error( ERR_DROP, "Siege teams not defined" );
	}

	if ( BG_SiegeGetValueGroup( siege_info, team1, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "TeamIcon", teamIcon ) )
			trap->Cvar_Set( "team1_icon", teamIcon );

		if ( BG_SiegeGetPairedValue( teamInfo, "Timed", btime ) ) {
			team1Timed = atoi( btime ) * 1000;
			CG_SetSiegeTimerCvar( team1Timed );
		}
		else {
			team1Timed = 0;
		}
	}
	else {
		trap->Error( ERR_DROP, "No team entry for '%s'\n", team1 );
	}

	if ( BG_SiegeGetPairedValue( siege_info, "mapgraphic", teamInfo ) ) {
		trap->Cvar_Set( "siege_mapgraphic", teamInfo );
	}
	else {
		trap->Cvar_Set( "siege_mapgraphic", "gfx/mplevels/siege1_hoth" );
	}

	if ( BG_SiegeGetPairedValue( siege_info, "missionname", teamInfo ) ) {
		trap->Cvar_Set( "siege_missionname", teamInfo );
	}
	else {
		trap->Cvar_Set( "siege_missionname", " " );
	}

	if ( BG_SiegeGetValueGroup( siege_info, team2, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "TeamIcon", teamIcon ) ) {
			trap->Cvar_Set( "team2_icon", teamIcon );
		}

		if ( BG_SiegeGetPairedValue( teamInfo, "Timed", btime ) ) {
			team2Timed = atoi( btime ) * 1000;
			CG_SetSiegeTimerCvar( team2Timed );
		}
		else {
			team2Timed = 0;
		}
	}
	else {
		trap->Error( ERR_DROP, "No team entry for '%s'\n", team2 );
	}

	//Load the player class types
	BG_SiegeLoadClasses( NULL );

	if ( !bgNumSiegeClasses ) {
		trap->Error( ERR_DROP, "Couldn't find any player classes for Siege" );
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if ( !bgNumSiegeTeams ) {
		trap->Error( ERR_DROP, "Couldn't find any player teams for Siege" );
	}

	// Get and set the team themes for each team. This will control which classes can be used on each team.
	if ( BG_SiegeGetValueGroup( siege_info, team1, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "UseTeam", btime ) ) {
			BG_SiegeSetTeamTheme( SIEGETEAM_TEAM1, btime );
		}
		if ( BG_SiegeGetPairedValue( teamInfo, "FriendlyShader", btime ) ) {
			cgSiegeTeam1PlShader = trap->R_RegisterShaderNoMip( btime );
		}
		else {
			cgSiegeTeam1PlShader = 0;
		}
	}
	if ( BG_SiegeGetValueGroup( siege_info, team2, teamInfo ) ) {
		if ( BG_SiegeGetPairedValue( teamInfo, "UseTeam", btime ) ) {
			BG_SiegeSetTeamTheme( SIEGETEAM_TEAM2, btime );
		}
		if ( BG_SiegeGetPairedValue( teamInfo, "FriendlyShader", btime ) ) {
			cgSiegeTeam2PlShader = trap->R_RegisterShaderNoMip( btime );
		}
		else {
			cgSiegeTeam2PlShader = 0;
		}
	}

	//Now go through the classes used by the loaded teams and try to precache any forced models or forced skins.
	for ( i = SIEGETEAM_TEAM1; i <= SIEGETEAM_TEAM2; i++ ) {
		sTeam = BG_SiegeFindThemeForTeam( i );

		if ( !sTeam ) {
			continue;
		}

		//Get custom team shaders while we're at it.
		if ( i == SIEGETEAM_TEAM1 )
			cgSiegeTeam1PlShader = sTeam->friendlyShader;
		else if ( i == SIEGETEAM_TEAM2 )
			cgSiegeTeam2PlShader = sTeam->friendlyShader;

		for ( j = 0; j < sTeam->numClasses; j++ ) {
			cl = sTeam->classes[j];

			if ( cl->forcedModel[0] ) {
				// This class has a forced model, so precache it.
				trap->R_RegisterModel( va( "models/players/%s/model.glm", cl->forcedModel ) );

				if ( cl->forcedSkin[0] ) {
					// also has a forced skin, precache it.
					char useSkinName[MAX_QPATH];

					if ( strchr( cl->forcedSkin, '|' ) ) {
						Com_sprintf( useSkinName, sizeof(useSkinName), "models/players/%s/|%s",
							cl->forcedModel, cl->forcedSkin );
					}
					else {
						Com_sprintf( useSkinName, sizeof(useSkinName), "models/players/%s/model_%s.skin",
							cl->forcedModel, cl->forcedSkin );
					}

					trap->R_RegisterSkin( useSkinName );
				}
			}
		}
	}

	//precache saber data for classes that use sabers on both teams
	BG_PrecacheSabersForSiegeTeam( SIEGETEAM_TEAM1 );
	BG_PrecacheSabersForSiegeTeam( SIEGETEAM_TEAM2 );

	CG_PrecachePlayersForSiegeTeam( SIEGETEAM_TEAM1 );
	CG_PrecachePlayersForSiegeTeam( SIEGETEAM_TEAM2 );

	CG_PrecachePlayersForSiegeTeam( SIEGETEAM_TEAM1 );
	CG_PrecachePlayersForSiegeTeam( SIEGETEAM_TEAM2 );

	CG_PrecacheSiegeObjectiveAssetsForTeam( SIEGETEAM_TEAM1 );
	CG_PrecacheSiegeObjectiveAssetsForTeam( SIEGETEAM_TEAM2 );

	return;
failure:
	siege_valid = qfalse;
}

static char *CG_SiegeObjectiveBuffer( int team, int objective ) {
	static char buf[8192];
	char teamstr[1024];

	if ( team == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		if ( BG_SiegeGetValueGroup( cgParseObjectives, va( "Objective%i", objective ), buf ) )
			return buf;
	}

	return NULL;
}

void CG_ParseSiegeObjectiveStatus( const char *str ) {
	int i = 0, team = SIEGETEAM_TEAM1;
	const char *cvarName, *s;
	int objectiveNum = 0;

	if ( !str || !str[0] )
		return;

	while ( str[i] ) {
		if ( str[i] == '|' ) {
			// switch over to team2, this is the next section
			team = SIEGETEAM_TEAM2;
			objectiveNum = 0;
		}
		else if ( str[i] == '-' ) {
			objectiveNum++;
			i++;

			cvarName = va( "team%i_objective%i", team, objectiveNum );
			if ( str[i] == '1' )
				trap->Cvar_Set( cvarName, "1" );
			else
				trap->Cvar_Set( cvarName, "0" );

			// now set the description and graphic cvars to by read by the menu
			s = CG_SiegeObjectiveBuffer( team, objectiveNum );
			if ( s && s[0] ) {
				char buffer[8192];

				cvarName = va( "team%i_objective%i_longdesc", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "objdesc", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "UNSPECIFIED" );

				cvarName = va( "team%i_objective%i_gfx", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "objgfx", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "UNSPECIFIED" );

				cvarName = va( "team%i_objective%i_mapicon", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "mapicon", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "UNSPECIFIED" );

				cvarName = va( "team%i_objective%i_litmapicon", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "litmapicon", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "UNSPECIFIED" );

				cvarName = va( "team%i_objective%i_donemapicon", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "donemapicon", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "UNSPECIFIED" );

				cvarName = va( "team%i_objective%i_mappos", team, objectiveNum );
				if ( BG_SiegeGetPairedValue( s, "mappos", buffer ) )
					trap->Cvar_Set( cvarName, buffer );
				else
					trap->Cvar_Set( cvarName, "0 0 32 32" );
			}
		}
		i++;
	}

	if ( cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR )
		CG_SiegeBriefingDisplay( cg.predictedPlayerState.persistant[PERS_TEAM], qtrue );
}

void CG_SiegeRoundOver( centity_t *ent, int won ) {
	int				myTeam, success = 0;
	char			teamstr[64], appstring[1024], soundstr[1024];
	playerState_t	*ps = NULL;

	if ( !siege_valid ) {
		trap->Error( ERR_DROP, "ERROR: Siege data does not exist on client!\n" );
		return;
	}

	// this should always be true, if it isn't though use the predicted ps as a fallback
	ps = cg.snap ? &cg.snap->ps : &cg.predictedPlayerState;

	myTeam = ps->persistant[PERS_TEAM];

	if ( myTeam == TEAM_SPECTATOR )
		return;

	if ( myTeam == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		if ( won == myTeam )
			success = BG_SiegeGetPairedValue( cgParseObjectives, "wonround", appstring );
		else
			success = BG_SiegeGetPairedValue( cgParseObjectives, "lostround", appstring );

		if ( success )
			CG_DrawSiegeMessage( appstring, 0 );

		appstring[0] = '\0';
		soundstr[0] = '\0';

		if ( myTeam == won )
			Com_sprintf( teamstr, sizeof(teamstr), "roundover_sound_wewon" );
		else
			Com_sprintf( teamstr, sizeof(teamstr), "roundover_sound_welost" );

		if ( BG_SiegeGetPairedValue( cgParseObjectives, teamstr, appstring ) )
			Com_sprintf( soundstr, sizeof(soundstr), appstring );

		if ( soundstr[0] )
			trap->S_StartLocalSound( trap->S_RegisterSound( soundstr ), CHAN_ANNOUNCER );
	}
}

void CG_SiegeGetObjectiveDescription( int team, int objective, char *buffer ) {
	char teamstr[1024], objectiveStr[8192];

	buffer[0] = '\0'; //set to 0 ahead of time in case we fail to find the objective group/name

	if ( team == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		// found the team group
		if ( BG_SiegeGetValueGroup( cgParseObjectives, va( "Objective%i", objective ), objectiveStr ) )
			BG_SiegeGetPairedValue( objectiveStr, "goalname", buffer );
	}
}

int CG_SiegeGetObjectiveFinal( int team, int objective ) {
	char finalStr[64], teamstr[1024], objectiveStr[8192];

	if ( team == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		if ( BG_SiegeGetValueGroup( cgParseObjectives, va( "Objective%i", objective ), objectiveStr ) ) {
			BG_SiegeGetPairedValue( objectiveStr, "final", finalStr );
			return atoi( finalStr );
		}
	}
	return 0;
}

void CG_SiegeBriefingDisplay( int team, qboolean dontShow ) {
	char teamstr[64] = { '\0' };
	char briefing[8192] = { '\0' };
	char properValue[1024] = { '\0' };
	char objectiveDesc[1024] = { '\0' };
	int i, useTeam = team;
	qboolean primary = qfalse;

	if ( !siege_valid )
		return;

	if ( team == TEAM_SPECTATOR )
		return;

	if ( team == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( useTeam != SIEGETEAM_TEAM1 && useTeam != SIEGETEAM_TEAM2 )
		useTeam = SIEGETEAM_TEAM2;

	trap->Cvar_Set( "siege_primobj_inuse", "0" );

	for ( i = 1; i<16; i++ ) {
		//Get the value for this objective on this team
		//Now set the cvar for the menu to display.

		//primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>-1)?qtrue:qfalse;
		primary = (CG_SiegeGetObjectiveFinal( useTeam, i ) > 0) ? qtrue : qfalse;

		properValue[0] = '\0';
		trap->Cvar_VariableStringBuffer( va( "team%i_objective%i", useTeam, i ), properValue, sizeof(properValue) );
		if ( primary )
			trap->Cvar_Set( "siege_primobj", properValue );
		else
			trap->Cvar_Set( va( "siege_objective%i", i ), properValue );

		//Now set the long desc cvar for the menu to display.
		properValue[0] = '\0';
		trap->Cvar_VariableStringBuffer( va( "team%i_objective%i_longdesc", useTeam, i ), properValue, sizeof(properValue) );
		if ( primary )
			trap->Cvar_Set( "siege_primobj_longdesc", properValue );
		else
			trap->Cvar_Set( va( "siege_objective%i_longdesc", i ), properValue );

		//Now set the gfx cvar for the menu to display.
		properValue[0] = '\0';
		trap->Cvar_VariableStringBuffer( va( "team%i_objective%i_gfx", useTeam, i ), properValue, sizeof(properValue) );
		if ( primary )
			trap->Cvar_Set( "siege_primobj_gfx", properValue );
		else
			trap->Cvar_Set( va( "siege_objective%i_gfx", i ), properValue );

		//Now set the mapicon cvar for the menu to display.
		properValue[0] = '\0';
		trap->Cvar_VariableStringBuffer( va( "team%i_objective%i_mapicon", useTeam, i ), properValue, sizeof(properValue) );
		if ( primary )
			trap->Cvar_Set( "siege_primobj_mapicon", properValue );
		else
			trap->Cvar_Set( va( "siege_objective%i_mapicon", i ), properValue );

		//Now set the mappos cvar for the menu to display.
		properValue[0] = '\0';
		trap->Cvar_VariableStringBuffer( va( "team%i_objective%i_mappos", useTeam, i ), properValue, sizeof(properValue) );
		if ( primary )
			trap->Cvar_Set( "siege_primobj_mappos", properValue );
		else
			trap->Cvar_Set( va( "siege_objective%i_mappos", i ), properValue );

		//Now set the description cvar for the objective
		CG_SiegeGetObjectiveDescription( useTeam, i, objectiveDesc );

		if ( objectiveDesc[0] ) {
			// found a valid objective description
			if ( primary ) {
				trap->Cvar_Set( "siege_primobj_desc", objectiveDesc );
				// this one is marked not in use because it gets primobj
				trap->Cvar_Set( va( "siege_objective%i_inuse", i ), "0" );
				trap->Cvar_Set( "siege_primobj_inuse", "1" );
				trap->Cvar_Set( va( "team%i_objective%i_inuse", useTeam, i ), "1" );
			}
			else {
				trap->Cvar_Set( va( "siege_objective%i_desc", i ), objectiveDesc );
				trap->Cvar_Set( va( "siege_objective%i_inuse", i ), "2" );
				trap->Cvar_Set( va( "team%i_objective%i_inuse", useTeam, i ), "2" );
			}
		}
		else {
			// didn't find one, so set the "inuse" cvar to 0 for the objective and mark it non-complete.
			trap->Cvar_Set( va( "siege_objective%i_inuse", i ), "0" );
			trap->Cvar_Set( va( "siege_objective%i", i ), "0" );
			trap->Cvar_Set( va( "team%i_objective%i_inuse", useTeam, i ), "0" );
			trap->Cvar_Set( va( "team%i_objective%i", useTeam, i ), "0" );
			trap->Cvar_Set( va( "siege_objective%i_mappos", i ), "" );
			trap->Cvar_Set( va( "team%i_objective%i_mappos", useTeam, i ), "" );
			trap->Cvar_Set( va( "siege_objective%i_gfx", i ), "" );
			trap->Cvar_Set( va( "team%i_objective%i_gfx", useTeam, i ), "" );
			trap->Cvar_Set( va( "siege_objective%i_mapicon", i ), "" );
			trap->Cvar_Set( va( "team%i_objective%i_mapicon", useTeam, i ), "" );
		}
	}

	if ( dontShow )
		return;

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		if ( BG_SiegeGetPairedValue( cgParseObjectives, "briefing", briefing ) )
			CG_DrawSiegeMessage( briefing, 1 );
	}
}

void CG_SiegeObjectiveCompleted( centity_t *ent, int won, int objectivenum ) {
	int myTeam, success = 0;
	static char foundobjective[MAX_SIEGE_INFO_SIZE] = { 0 };
	char teamstr[64], objstr[256], appstring[1024], soundstr[1024];
	playerState_t *ps = NULL;

	//Raz: moved to the heap
	foundobjective[0] = '\0';

	if ( !siege_valid ) {
		trap->Error( ERR_DROP, "Siege data does not exist on client!\n" );
		return;
	}

	ps = cg.snap ? &cg.snap->ps : &cg.predictedPlayerState;

	myTeam = ps->persistant[PERS_TEAM];

	if ( myTeam == TEAM_SPECTATOR )
		return;

	if ( won == SIEGETEAM_TEAM1 )
		Com_sprintf( teamstr, sizeof(teamstr), team1 );
	else
		Com_sprintf( teamstr, sizeof(teamstr), team2 );

	if ( BG_SiegeGetValueGroup( siege_info, teamstr, cgParseObjectives ) ) {
		Com_sprintf( objstr, sizeof(objstr), "Objective%i", objectivenum );

		if ( BG_SiegeGetValueGroup( cgParseObjectives, objstr, foundobjective ) ) {
			if ( myTeam == SIEGETEAM_TEAM1 )
				success = BG_SiegeGetPairedValue( foundobjective, "message_team1", appstring );
			else
				success = BG_SiegeGetPairedValue( foundobjective, "message_team2", appstring );

			if ( success )
				CG_DrawSiegeMessageNonMenu( appstring );

			appstring[0] = '\0';
			soundstr[0] = '\0';

			if ( myTeam == SIEGETEAM_TEAM1 )
				Com_sprintf( teamstr, sizeof(teamstr), "sound_team1" );
			else
				Com_sprintf( teamstr, sizeof(teamstr), "sound_team2" );

			if ( BG_SiegeGetPairedValue( foundobjective, teamstr, appstring ) )
				Com_sprintf( soundstr, sizeof(soundstr), appstring );
			if ( soundstr[0] )
				trap->S_StartLocalSound( trap->S_RegisterSound( soundstr ), CHAN_ANNOUNCER );
		}
	}
}

siegeExtended_t cg_siegeExtendedData[MAX_CLIENTS];

//parse a single extended siege data entry
void CG_ParseSiegeExtendedDataEntry( const char *conStr ) {
	char s[MAX_STRING_CHARS], *str = (char *)conStr;
	int argParses = 0, i, maxAmmo = 0, clNum = -1, health = 1, maxhealth = 1, ammo = 1;
	centity_t *cent;

	if ( !conStr || !conStr[0] )
		return;

	while ( *str && argParses < 4 ) {
		i = 0;
		while ( *str && *str != '|' ) {
			s[i] = *str;
			i++;
			//Raz: May need to revert this change, but =o..
			//	*str++;
			str++;
		}
		s[i] = '\0';
		switch ( argParses ) {
		case 0:
			clNum = atoi( s );
			break;
		case 1:
			health = atoi( s );
			break;
		case 2:
			maxhealth = atoi( s );
			break;
		case 3:
			ammo = atoi( s );
			break;
		default:
			break;
		}
		argParses++;
		str++;
	}

	if ( clNum < 0 || clNum >= MAX_CLIENTS )
		return;

	cg_siegeExtendedData[clNum].health = health;
	cg_siegeExtendedData[clNum].maxhealth = maxhealth;
	cg_siegeExtendedData[clNum].ammo = ammo;

	cent = &cg_entities[clNum];

	maxAmmo = ammoMax[weaponData[cent->currentState.weapon].ammoIndex];
	if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
		maxAmmo *= 2.0f;

	// assure the weapon number is valid and not over max
	//	keep the weapon so if it changes before our next ext data update we'll know that the ammo is not applicable.
	if ( ammo >= 0 && ammo <= maxAmmo )
		cg_siegeExtendedData[clNum].weapon = cent->currentState.weapon;
	else
		cg_siegeExtendedData[clNum].weapon = -1;

	cg_siegeExtendedData[clNum].lastUpdated = cg.time;
}

//parse incoming siege data, see counterpart in g_saga.c
void CG_ParseSiegeExtendedData( void ) {
	int i, numEntries = trap->Cmd_Argc();

	if ( numEntries < 1 ) {
		assert( !"Bad numEntries for sxd" );
		return;
	}

	for ( i = 0; i < numEntries; i++ )
		CG_ParseSiegeExtendedDataEntry( CG_Argv( i + 1 ) );
}

void CG_SetSiegeTimerCvar( int msec ) {
	int mins, tens, secs;

	secs = msec / 1000;
	mins = secs / 60;
	secs -= mins * 60;
	tens = secs / 10;
	secs -= tens * 10;

	trap->Cvar_Set( "ui_siegeTimer", va( "%i:%i%i", mins, tens, secs ) );
}
