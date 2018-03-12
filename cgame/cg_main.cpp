// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include <string>
#include <iostream>
#include <sstream>

#include "cg_local.h"
#include "bg_lua.h"
#include "bg_local.h"
#include "ui/ui_shared.h"
#include "cg_lights.h"
#include "cg_media.h"
#include "cg_serverHistory.h"
#include "ui/ui_fonts.h"
#include "cg_notify.h"

displayContextDef_t cgDC;

void UI_CleanupGhoul2( void );

refEntity_t	MiscEnts[MAX_MISC_ENTS]; //statically allocated for now.
float		Radius[MAX_MISC_ENTS];
float		zOffset[MAX_MISC_ENTS]; //some models need a z offset for culling, because of stupid wrong model origins

int			NumMiscEnts = 0;

void CG_MiscEnt( void );
void CG_DoCameraShake( vector3 *origin, float intensity, int radius, int time );

//do we have any force powers that we would normally need to cycle to?
qboolean CG_NoUseableForce( void ) {
	int i = FP_HEAL;
	while ( i < NUM_FORCE_POWERS ) {
		if ( i != FP_SABERTHROW &&
			i != FP_SABER_OFFENSE &&
			i != FP_SABER_DEFENSE &&
			i != FP_LEVITATION ) { //valid selectable power
			if ( cg.predictedPlayerState.fd.forcePowersKnown & (1 << i) ) { //we have it
				return qfalse;
			}
		}
		i++;
	}

	//no useable force powers, I guess.
	return qtrue;
}

static uint32_t C_PointContents( void ) {
	TCGPointContents	*data = (TCGPointContents *)cg.sharedBuffer;

	return CG_PointContents( &data->mPoint, data->mPassEntityNum );
}

static void C_GetLerpOrigin( void ) {
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy( &cg_entities[data->mEntityNum].lerpOrigin, &data->mPoint );
}

static void C_GetLerpData( void ) {//only used by FX system to pass to getboltmat
	TCGGetBoltData		*data = (TCGGetBoltData *)cg.sharedBuffer;

	VectorCopy( &cg_entities[data->mEntityNum].lerpOrigin, &data->mOrigin );
	VectorCopy( &cg_entities[data->mEntityNum].modelScale, &data->mScale );
	VectorCopy( &cg_entities[data->mEntityNum].lerpAngles, &data->mAngles );
	if ( cg_entities[data->mEntityNum].currentState.eType == ET_PLAYER ) { //normal player
		data->mAngles.pitch = 0.0f;
		data->mAngles.roll = 0.0f;
	}
	else if ( cg_entities[data->mEntityNum].currentState.eType == ET_NPC ) { //an NPC
		Vehicle_t *pVeh = cg_entities[data->mEntityNum].m_pVehicle;
		if ( !pVeh ) { //for vehicles, we may or may not want to 0 out pitch and roll
			data->mAngles.pitch = 0.0f;
			data->mAngles.roll = 0.0f;
		}
		else if ( pVeh->m_pVehicleInfo->type == VH_SPEEDER ) { //speeder wants no pitch but a roll
			data->mAngles.pitch = 0.0f;
		}
		else if ( pVeh->m_pVehicleInfo->type != VH_FIGHTER ) { //fighters want all angles
			data->mAngles.pitch = 0.0f;
			data->mAngles.roll = 0.0f;
		}
	}
}

static void C_Trace( void ) {
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_Trace( &td->mResult, &td->mStart, &td->mMins, &td->mMaxs, &td->mEnd, td->mSkipNumber, td->mMask );
}

static void C_G2Trace( void ) {
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_G2Trace( &td->mResult, &td->mStart, &td->mMins, &td->mMaxs, &td->mEnd, td->mSkipNumber, td->mMask );
}

static void C_G2Mark( void ) {
	TCGG2Mark	*td = (TCGG2Mark *)cg.sharedBuffer;
	trace_t		trace;
	vector3		end;

	VectorMA( &td->start, 64, &td->dir, &end );
	CG_G2Trace( &trace, &td->start, NULL, NULL, &end, ENTITYNUM_NONE, MASK_PLAYERSOLID );

	if ( trace.entityNum < ENTITYNUM_WORLD &&
		cg_entities[trace.entityNum].ghoul2 ) { //hit someone with a ghoul2 instance, let's project the decal on them then.
		centity_t *cent = &cg_entities[trace.entityNum];

		//CG_TestLine(tr.endpos, end, 2000, 0xFF0000u, 1);

		CG_AddGhoul2Mark( td->shader, td->size, &trace.endpos, &end, trace.entityNum,
			&cent->lerpOrigin, cent->lerpAngles.yaw, cent->ghoul2, &cent->modelScale,
			Q_irand( 2000, 4000 ) );
		//I'm making fx system decals have a very short lifetime.
	}
}

static void CG_DebugBoxLines( vector3 *mins, vector3 *maxs, int duration ) {
	vector3 start, end;
	vector3 vert;

	float x = maxs->x - mins->x;
	float y = maxs->y - mins->y;

	start.z = maxs->z;
	vert.z = mins->z;

	vert.x = mins->x;
	vert.y = mins->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine( &start, &vert, duration, 0xFF0000u, 1 );

	vert.x = mins->x;
	vert.y = maxs->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine( &start, &vert, duration, 0xFF0000u, 1 );

	vert.x = maxs->x;
	vert.y = mins->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine( &start, &vert, duration, 0xFF0000u, 1 );

	vert.x = maxs->x;
	vert.y = maxs->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine( &start, &vert, duration, 0xFF0000u, 1 );

	// top of box
	VectorCopy( maxs, &start );
	VectorCopy( maxs, &end );
	start.x -= x;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	end.x = start.x;
	end.y -= y;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	start.y = end.y;
	start.x += x;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	CG_TestLine( &start, maxs, duration, 0xFF0000u, 1 );

	// bottom of box
	VectorCopy( mins, &start );
	VectorCopy( mins, &end );
	start.x += x;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	end.x = start.x;
	end.y += y;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	start.y = end.y;
	start.x -= x;
	CG_TestLine( &start, &end, duration, 0xFF0000u, 1 );
	CG_TestLine( &start, mins, duration, 0xFF0000u, 1 );
}

//handle ragdoll callbacks, for events and debugging -rww
static int CG_RagCallback( int callType ) {
	switch ( callType ) {
	case RAG_CALLBACK_DEBUGBOX:
	{
		ragCallbackDebugBox_t *callData = (ragCallbackDebugBox_t *)cg.sharedBuffer;

		CG_DebugBoxLines( &callData->mins, &callData->maxs, callData->duration );
	}
		break;
	case RAG_CALLBACK_DEBUGLINE:
	{
		ragCallbackDebugLine_t *callData = (ragCallbackDebugLine_t *)cg.sharedBuffer;

		CG_TestLine( &callData->start, &callData->end, callData->time, callData->color, callData->radius );
	}
		break;
	case RAG_CALLBACK_BONESNAP:
	{
		ragCallbackBoneSnap_t *callData = (ragCallbackBoneSnap_t *)cg.sharedBuffer;
		centity_t *cent = &cg_entities[callData->entNum];
		int snapSound = trap->S_RegisterSound( va( "sound/player/bodyfall_human%i.wav", Q_irand( 1, 3 ) ) );

		trap->S_StartSound( &cent->lerpOrigin, callData->entNum, CHAN_AUTO, snapSound );
	}
	case RAG_CALLBACK_BONEIMPACT:
		break;
	case RAG_CALLBACK_BONEINSOLID:
#if 0
	{
		ragCallbackBoneInSolid_t *callData = (ragCallbackBoneInSolid_t *)cg.sharedBuffer;

		if (callData->solidCount > 16)
		{ //don't bother if we're just tapping into solidity, we'll probably recover on our own
			centity_t *cent = &cg_entities[callData->entNum];
			vector3 slideDir;

			VectorSubtract(cent->lerpOrigin, callData->bonePos, slideDir);
			VectorAdd(cent->ragOffsets, slideDir, cent->ragOffsets);

			cent->hasRagOffset = qtrue;
		}
	}
#endif
		break;
	case RAG_CALLBACK_TRACELINE:
	{
		ragCallbackTraceLine_t *callData = (ragCallbackTraceLine_t *)cg.sharedBuffer;

		CG_Trace( &callData->tr, &callData->start, &callData->mins, &callData->maxs,
			&callData->end, callData->ignore, callData->mask );
	}
		break;
	default:
		Com_Error( ERR_DROP, "Invalid callType in CG_RagCallback" );
		break;
	}

	return 0;
}

static void C_ImpactMark( void ) {
	TCGImpactMark	*data = (TCGImpactMark *)cg.sharedBuffer;

	/*
	CG_ImpactMark((int)arg0, (const float *)arg1, (const float *)arg2, (float)arg3,
	(float)arg4, (float)arg5, (float)arg6, (float)arg7, qtrue, (float)arg8, qfalse);
	*/
	CG_ImpactMark( data->mHandle, &data->mPoint, &data->mAngle, data->mRotation,
		data->mRed, data->mGreen, data->mBlue, data->mAlphaStart, qtrue, data->mSizeStart, qfalse );
}

void CG_MiscEnt( void ) {
	int			modelIndex;
	refEntity_t	*RefEnt;
	TCGMiscEnt	*data = (TCGMiscEnt *)cg.sharedBuffer;
	vector3		mins, maxs;
	float		*radius, *zOff;

	if ( NumMiscEnts >= MAX_MISC_ENTS ) {
		return;
	}

	radius = &Radius[NumMiscEnts];
	zOff = &zOffset[NumMiscEnts];
	RefEnt = &MiscEnts[NumMiscEnts++];

	modelIndex = trap->R_RegisterModel( data->mModel );
	if ( modelIndex == 0 ) {
		Com_Error( ERR_DROP, "client_model has invalid model definition" );
		return;
	}

	*zOff = 0;

	memset( RefEnt, 0, sizeof(refEntity_t) );
	RefEnt->reType = RT_MODEL;
	RefEnt->hModel = modelIndex;
	RefEnt->frame = 0;
	trap->R_ModelBounds( modelIndex, &mins, &maxs );
	VectorCopy( &data->mScale, &RefEnt->modelScale );
	VectorCopy( &data->mOrigin, &RefEnt->origin );

	VectorScaleVector( &mins, &data->mScale, &mins );
	VectorScaleVector( &maxs, &data->mScale, &maxs );
	*radius = Distance( &mins, &maxs );

	AnglesToAxis( &data->mAngles, RefEnt->axis );
	ScaleModelAxis( RefEnt );
}

void CG_DrawMiscEnts( void ) {
	int			i;
	refEntity_t	*RefEnt;
	float		*radius, *zOff;
	vector3		difference;
	vector3		cullOrigin;
	refdef_t *refdef = CG_GetRefdef();

	RefEnt = MiscEnts;
	radius = Radius;
	zOff = zOffset;
	for ( i = 0; i < NumMiscEnts; i++ ) {
		VectorCopy( &RefEnt->origin, &cullOrigin );
		cullOrigin.z += 1.0f;

		if ( *zOff ) {
			cullOrigin.z += *zOff;
		}

		if ( cg.snap && trap->R_InPVS( &refdef->vieworg, &cullOrigin, cg.snap->areamask ) ) {
			VectorSubtract( &RefEnt->origin, &refdef->vieworg, &difference );
			if ( VectorLength( &difference ) - (*radius) <= cg.distanceCull ) {
				SE_R_AddRefEntityToScene( RefEnt, MAX_CLIENTS );
			}
		}
		RefEnt++;
		radius++;
		zOff++;
	}
}

cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];

centity_t			*cg_permanents[MAX_GENTITIES]; //rwwRMG - added
int					cg_numpermanents = 0;

weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];

static void CVU_SVRunning( void ) {
	cgs.localServer = sv_running.integer;
}

static void CVU_ForceColour( void ) {
	ivector3 *v = &cg.enemyColour;
	if ( sscanf( cg_forceEnemyColour.string, "%i %i %i", &v->r, &v->g, &v->b ) != 3 ) {
		v->r = 255;
		v->g = 255;
		v->b = 255;
	}

	v = &cg.allyColour;
	if ( sscanf( cg_forceAllyColour.string, "%i %i %i", &v->r, &v->g, &v->b ) != 3 ) {
		v->r = 255;
		v->g = 255;
		v->b = 255;
	}
}

static void CVU_ForceModel( void ) {
	for ( int i = 0; i < cgs.maxclients; i++ ) {
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !VALIDSTRING( clientInfo ) ) {
			continue;
		}

		CG_NewClientInfo( i, qtrue );
	}
}

static void CVU_ForceOwnSaber( void ) {
	std::stringstream ss( cg_forceOwnSaber.string );
	for ( int i = 0; i < MAX_SABERS; i++ ) {
		ss >> cg.forceOwnSaber[i];
	}
}

static void CVU_ForceEnemySaber( void ) {
	std::stringstream ss( cg_forceEnemySaber.string );
	for ( int i = 0; i < MAX_SABERS; i++ ) {
		ss >> cg.forceEnemySaber[i];
	}
}

static void CVU_TeamOverlay( void ) {
	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( cg_drawTeamOverlay.integer > 0 && cgs.gametype >= GT_SINGLE_PLAYER )
		trap->Cvar_Set( "teamoverlay", "1" );
	else
		trap->Cvar_Set( "teamoverlay", "0" );
}

static void CVU_BubbleColour( void ) {
	ivector4 *v = &cg.bubbleColour;
	if ( sscanf( cg_bubbleColour.string, "%i %i %i %i", &v->r, &v->g, &v->b, &v->a ) != 4 ) {
		v->r = 0;
		v->g = 255;
		v->b = 0;
		v->a = 100;
	}
}

static void CVU_ChatboxPos( void ) {
	ivector2 *v = &cg.chatbox.pos;
	if ( sscanf( cg_chatboxPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 128;
		v->y = 395;
	}
}

static void CVU_ChatboxSize( void ) {
	if ( sscanf( cg_chatboxSize.string, "%f %i", &cg.chatbox.size.scale, &cg.chatbox.size.width ) != 2 ) {
		cg.chatbox.size.scale = 0.5f;
		cg.chatbox.size.width = 497;
	}
}

static void CVU_ChatboxTabs( void ) {
	CG_ChatboxSelect( "normal" );
}

static void CVU_CrosshairColour( void ) {
	ivector4 *v = &cg.crosshair.colour;
	if ( sscanf( cg_crosshairColour.string, "%i %i %i %i", &v->r, &v->g, &v->b, &v->a ) != 4 ) {
		v->r = 255;
		v->g = 255;
		v->b = 255;
		v->a = 255;
	}
}

static void CVU_ChatboxBG( void ) {
	vector4 *v = &cg.chatbox.background;
	if ( sscanf( cg_chatboxBackground.string, "%f %f %f %f", &v->r, &v->g, &v->b, &v->a ) != 4 ) {
		v->r = 0.0f;
		v->g = 0.0f;
		v->b = 0.0f;
		v->a = 0.5f;
	}
}

static void CVU_DuelColour( void ) {
	ivector4 *v = &cg.duelColour.rgba;
	int tmp;
	if ( sscanf( cg_duelColour.string, "%i %i %i %i %i", &v->r, &v->g, &v->b, &v->a, &tmp ) != 5 ) {
		v->r = 75;
		v->g = 75;
		v->b = 224;
		v->a = 128;
		cg.duelColour.forceAlpha = qfalse;
	}
	else
		cg.duelColour.forceAlpha = tmp;
}

static void CVU_GunAlign( void ) {
	vector3 *v = &cg.gunAlign;
	if ( sscanf( cg_gunAlign.string, "%f %f %f", &v->x, &v->y, &v->z ) != 3 ) {
		v->x = 0.0f;
		v->y = 0.0f;
		v->z = 0.0f;
	}
}

static void CVU_GunBob( void ) {
	vector3 *v = &cg.gunBob;
	if ( sscanf( cg_gunBob.string, "%f %f %f", &v->pitch, &v->yaw, &v->roll ) != 3 ) {
		v->pitch = 0.005f;
		v->yaw = 0.01f;
		v->roll = 0.005f;
	}
}

static void CVU_GunDrift( void ) {
	vector3 *v = &cg.gunIdleDrift.amount;
	if ( sscanf( cg_gunIdleDrift.string, "%f %f %f %f", &v->pitch, &v->yaw, &v->roll, &cg.gunIdleDrift.speed ) != 4 ) {
		v->pitch = 0.01f;
		v->yaw = 0.01f;
		v->roll = 0.01f;
		cg.gunIdleDrift.speed = 0.001f;
	}
}

static void CVU_LagPos( void ) {
	ivector2 *v = &cg.lagometerPos;
	if ( sscanf( cg_lagometerPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 48;
		v->y = 160;
	}
}

static void CVU_MoveKeysPos( void ) {
	ivector2 *v = &cg.moveKeysPos;
	if ( sscanf( cg_movementKeysPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = (SCREEN_WIDTH / 2);
		v->y = (SCREEN_HEIGHT / 2);
	}
}

static void CVU_StatsPos( void ) {
	ivector2 *v = &cg.statsPos;
	if ( sscanf( cg_hudStatsPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 2;
		v->y = (SCREEN_HEIGHT / 2);
	}
}

static void CVU_ShieldColour( void ) {
	ivector4 *v = &cg.shieldColour.rgba;
	int tmp;
	if ( sscanf( cg_shieldColour.string, "%i %i %i %i %i", &v->r, &v->g, &v->b, &v->a, &tmp ) != 5 ) {
		v->r = 75;
		v->g = 128;
		v->b = 224;
		v->a = 255;
		cg.shieldColour.forceAlpha = qfalse;
	}
	else
		cg.shieldColour.forceAlpha = tmp;
}

static void CVU_StrafeHelpColour( void ) {
	ivector4 *v = &cg.strafeHelperColour;
	if ( sscanf( cg_strafeHelperColor.string, "%i %i %i %i", &v->r, &v->g, &v->b, &v->a ) != 4 ) {
		v->r = 0;
		v->g = 255;
		v->b = 255;
		v->a = 255;
	}
}

static void CVU_StrafeTrailWeights( void ) {
	ivector2 *v = &cg.strafeTrailWeights;
	if ( sscanf( cg_strafeTrailWeights.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 300;
		v->y = 1500;
	}
}

static void CVU_ViewBob( void ) {
	int tmp;
	if ( sscanf( cg_viewBob.string, "%f %f %f %i", &cg.viewBob.pitch, &cg.viewBob.roll, &cg.viewBob.up, &tmp ) != 4 ) {
		cg.viewBob.pitch = 0.002f;
		cg.viewBob.roll = 0.002f;
		cg.viewBob.up = 0.005f;
		cg.viewBob.fall = qtrue;
	}
	else
		cg.viewBob.fall = tmp;
}

static void CVU_AutomapAngle( void ) {
	vector3 *v = &cg.automapAngle;
	if ( sscanf( r_autoMapAngle.string, "%f %f %f", &v->pitch, &v->yaw, &v->roll ) != 3 ) {
		v->pitch = 90.0f;
		v->yaw = 0.0f;
		v->roll = 0.0f;
	}
}

static void CVU_AccelPos( void ) {
	ivector2 *v = &cg.accelerometer.position;
	if ( sscanf( cg_accelerometerPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = (SCREEN_WIDTH / 2);
		v->y = 360;
	}
}

static void CVU_AccelSize( void ) {
	ivector2 *v = &cg.accelerometer.size;
	if ( sscanf( cg_accelerometerSize.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 128;
		v->y = 20;
	}
}

typedef struct cvarTable_s {
	vmCvar_t	*vmCvar;
	const char	*cvarName, *defaultString;
	void( *update )(void);
	uint32_t	cvarFlags;
} cvarTable_t;

#define XCVAR_DECL
#include "cg_xcvar.h"
#undef XCVAR_DECL

void CG_Set2DRatio(void) {
	if (japp_ratioFix.integer)
		cgs.widthRatioCoef = (float)(SCREEN_WIDTH * cgs.glconfig.vidHeight) / (float)(SCREEN_HEIGHT * cgs.glconfig.vidWidth);
	else
		cgs.widthRatioCoef = 1.0f;
}

static cvarTable_t cvarTable[] = {
#define XCVAR_LIST
#include "cg_xcvar.h"
#undef XCVAR_LIST
};

static int cvarTableSize = ARRAY_LEN( cvarTable );

void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount > modCount ) {
				if ( cv->update ) {
					cv->update();
				}
				JPLua::Cvar_Update( cv->cvarName );
			}
		}
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time >= (cg.crosshairClientTime + 1000) )
		return -1;

	if ( cg.crosshairClientNum >= MAX_CLIENTS )
		return -1;

	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime )
		return -1;

	return cg.snap->ps.persistant[PERS_ATTACKER];
}

const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cmd_Argv( arg, buffer, sizeof(buffer) );

	return buffer;
}

const char *CG_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cvar_VariableStringBuffer( var_name, buffer, sizeof(buffer) );

	return buffer;
}

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime( void ) {
	return cg.time;
}

// a global weather effect (rain, snow, etc)
void CG_ParseWeatherEffect( const char *str ) {
	trap->R_WorldEffectCommand( str + 1 );
}

void CG_ParseSiegeState( const char *str ) {
	int i = 0, j = 0;
	char b[1024];

	while ( str[i] && str[i] != '|' ) {
		b[j] = str[i];
		i++, j++;
	}
	b[j] = '\0';
	cgSiegeRoundState = atoi( b );

	if ( str[i] == '|' ) {
		j = 0;
		i++;
		while ( str[i] ) {
			b[j] = str[i];
			i++, j++;
		}
		b[j] = '\0';
		cgSiegeRoundTime = atoi( b );
		if ( cgSiegeRoundState == 0 || cgSiegeRoundState == 2 )
			cgSiegeRoundBeganTime = cgSiegeRoundTime;
	}
	else
		cgSiegeRoundTime = cg.time;
}

const char *CG_GetStringEdString( const char *refSection, const char *refName ) {
	static char text[2][1024] = { { 0 } };
	static int index = 0;

	index ^= 1;
	trap->SE_GetStringTextString( va( "%s_%s", refSection, refName ), text[index], sizeof(text[0]) );

	return text[index];
}

void CG_SiegeCountCvars( void ) {
	int classGfx[6];

	trap->Cvar_Set( "ui_tm1_cnt", va( "%d", CG_GetTeamNonScoreCount( TEAM_RED ) ) );
	trap->Cvar_Set( "ui_tm2_cnt", va( "%d", CG_GetTeamNonScoreCount( TEAM_BLUE ) ) );
	trap->Cvar_Set( "ui_tm3_cnt", va( "%d", CG_GetTeamNonScoreCount( TEAM_SPECTATOR ) ) );

	// This is because the only way we can match up classes is by the gfx handle.
	classGfx[0] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_infantry" );
	classGfx[1] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_heavy_weapons" );
	classGfx[2] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_demolitionist" );
	classGfx[3] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_vanguard" );
	classGfx[4] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_support" );
	classGfx[5] = trap->R_RegisterShaderNoMip( "gfx/mp/c_icon_jedi_general" );

	trap->Cvar_Set( "ui_tm1_c0_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[0] ) ) );
	trap->Cvar_Set( "ui_tm1_c1_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[1] ) ) );
	trap->Cvar_Set( "ui_tm1_c2_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[2] ) ) );
	trap->Cvar_Set( "ui_tm1_c3_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[3] ) ) );
	trap->Cvar_Set( "ui_tm1_c4_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[4] ) ) );
	trap->Cvar_Set( "ui_tm1_c5_cnt", va( "%d", CG_GetClassCount( TEAM_RED, classGfx[5] ) ) );

	trap->Cvar_Set( "ui_tm2_c0_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[0] ) ) );
	trap->Cvar_Set( "ui_tm2_c1_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[1] ) ) );
	trap->Cvar_Set( "ui_tm2_c2_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[2] ) ) );
	trap->Cvar_Set( "ui_tm2_c3_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[3] ) ) );
	trap->Cvar_Set( "ui_tm2_c4_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[4] ) ) );
	trap->Cvar_Set( "ui_tm2_c5_cnt", va( "%d", CG_GetClassCount( TEAM_BLUE, classGfx[5] ) ) );

}

void CG_BuildSpectatorString( void ) {
#if 0
	int i;
	cg.spectatorList[0] = 0;

	// Count up the number of players per team and per class
	CG_SiegeCountCvars();

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}

	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
#else
	int i = 0;

	cg.scoreboard.spectatorList[0] = '\0';

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].team == TEAM_SPECTATOR )
			Q_strcat( cg.scoreboard.spectatorList, sizeof(cg.scoreboard.spectatorList), va( S_COLOR_WHITE"%s  ", cgs.clientinfo[cg.scores[i].client].name ) );
	}

	i = strlen( cg.scoreboard.spectatorList );
	if ( i != cg.scoreboard.spectatorLen ) {//new spectator info
		cg.scoreboard.spectatorLen = i;
		cg.scoreboard.spectatorX = SCREEN_WIDTH;
		cg.scoreboard.spectatorResetTime = cg.time;
	}
#endif
}

static void CG_RegisterClients( void ) {
	cg.loading = qtrue;
	CG_LoadingClient( cg.clientNum );
	CG_NewClientInfo( cg.clientNum, qfalse );

	for ( int i = 0; i < cgs.maxclients; i++ ) {
		if ( i == cg.clientNum ) {
			continue;
		}

		const char *clientInfo = CG_ConfigString( CS_PLAYERS + i );
		if ( !clientInfo[0] )
			continue;
		CG_LoadingClient( i );
		CG_NewClientInfo( i, qfalse );
	}

	CG_BuildSpectatorString();
	cg.loading = qfalse;
}

const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		trap->Error( ERR_DROP, "CG_ConfigString: bad index: %i", index );
		return NULL;
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

void CG_StartMusic( qboolean bForceStart ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( (const char **)&s ), sizeof(parm1) );
	Q_strncpyz( parm2, COM_Parse( (const char **)&s ), sizeof(parm2) );

	trap->S_StartBackgroundTrack( parm1, parm2, !bForceStart );
}

char *CG_GetMenuBuffer( const char *filename ) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap->FS_Open( filename, &f, FS_READ );
	if ( !f ) {
		trap->Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return NULL;
	}
	if ( len >= MAX_MENUFILE ) {
		trap->Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap->FS_Close( f );
		return NULL;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );

	return buf;
}

qboolean CG_Asset_Parse( int handle ) {
	pc_token_t token;

	if ( !trap->PC_ReadToken( handle, &token ) )
		return qfalse;
	if ( Q_stricmp( token.string, "{" ) )
		return qfalse;

	while ( 1 ) {
		if ( !trap->PC_ReadToken( handle, &token ) )
			return qfalse;

		if ( !Q_stricmp( token.string, "}" ) ) {
			return qtrue;
		}

#if 0
		// font
		else if ( !Q_stricmp( token.string, "font" ) ) {
			int pointSize;
			if ( !trap->PC_ReadToken( handle, &token ) || !PC_Int_Parse( handle, &pointSize ) )
				break;
			cgDC.Assets.qhMediumFont = cgDC.RegisterFont( token.string );
		}

		// smallFont
		else if ( !Q_stricmp( token.string, "smallFont" ) ) {
			int pointSize;
			if ( !trap->PC_ReadToken( handle, &token ) || !PC_Int_Parse( handle, &pointSize ) )
				break;
			cgDC.Assets.qhSmallFont = cgDC.RegisterFont( token.string );
		}

		// smallFont
		else if ( !Q_stricmp( token.string, "small2Font" ) ) {
			int pointSize;
			if ( !trap->PC_ReadToken( handle, &token ) || !PC_Int_Parse( handle, &pointSize ) )
				break;
			cgDC.Assets.qhSmall2Font = cgDC.RegisterFont( token.string );
		}

		// font
		else if ( !Q_stricmp( token.string, "bigfont" ) ) {
			int pointSize;
			if ( !trap->PC_ReadToken( handle, &token ) || !PC_Int_Parse( handle, &pointSize ) )
				break;
			cgDC.Assets.qhBigFont = cgDC.RegisterFont( token.string );
		}

		// font
		else if ( !Q_stricmp( token.string, "monoFont" ) ) {
			int pointSize;
			if ( !trap->PC_ReadToken( handle, &token ) || !PC_Int_Parse( handle, &pointSize ) )
				break;
			cgDC.Assets.japp.fontMono = cgDC.RegisterFont(token.string);
		}
#endif

		// gradientbar
		else if ( !Q_stricmp( token.string, "gradientbar" ) ) {
			if ( !trap->PC_ReadToken( handle, &token ) )
				break;
			cgDC.Assets.gradientBar = trap->R_RegisterShaderNoMip( token.string );
		}

		// enterMenuSound
		else if ( !Q_stricmp( token.string, "menuEnterSound" ) ) {
			if ( !trap->PC_ReadToken( handle, &token ) )
				break;
			cgDC.Assets.menuEnterSound = trap->S_RegisterSound( token.string );
		}

		// exitMenuSound
		else if ( !Q_stricmp( token.string, "menuExitSound" ) ) {
			if ( !trap->PC_ReadToken( handle, &token ) )
				break;
			cgDC.Assets.menuExitSound = trap->S_RegisterSound( token.string );
		}

		// itemFocusSound
		else if ( !Q_stricmp( token.string, "itemFocusSound" ) ) {
			if ( !trap->PC_ReadToken( handle, &token ) )
				break;
			cgDC.Assets.itemFocusSound = trap->S_RegisterSound( token.string );
		}

		// menuBuzzSound
		else if ( !Q_stricmp( token.string, "menuBuzzSound" ) ) {
			if ( !trap->PC_ReadToken( handle, &token ) )
				break;
			cgDC.Assets.menuBuzzSound = trap->S_RegisterSound( token.string );
		}

		else if ( !Q_stricmp( token.string, "cursor" ) ) {
			if ( !PC_String_Parse( handle, &cgDC.Assets.cursorStr ) )
				break;
			cgDC.Assets.cursor = trap->R_RegisterShaderNoMip( cgDC.Assets.cursorStr );
		}

		else if ( !Q_stricmp( token.string, "fadeClamp" ) ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeClamp ) )
				break;
		}

		else if ( !Q_stricmp( token.string, "fadeCycle" ) ) {
			if ( !PC_Int_Parse( handle, &cgDC.Assets.fadeCycle ) )
				break;
		}

		else if ( !Q_stricmp( token.string, "fadeAmount" ) ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.fadeAmount ) )
				break;
		}

		else if ( !Q_stricmp( token.string, "shadowX" ) ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowX ) )
				break;
		}

		else if ( !Q_stricmp( token.string, "shadowY" ) ) {
			if ( !PC_Float_Parse( handle, &cgDC.Assets.shadowY ) )
				break;
		}

		else if ( !Q_stricmp( token.string, "shadowColor" ) ) {
			if ( !PC_Color_Parse( handle, &cgDC.Assets.shadowColor ) )
				break;
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor.a;
		}
	}
	return qfalse;
}

void CG_ParseMenu( const char *menuFile ) {
	pc_token_t token;
	int handle;

	handle = trap->PC_LoadSource( menuFile );
	if ( !handle )
		handle = trap->PC_LoadSource( "ui/testhud.menu" );
	if ( !handle )
		return;

	while ( 1 ) {
		if ( !trap->PC_ReadToken( handle, &token ) )
			break;

		if ( token.string[0] == '}' )
			break;

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 ) {
			if ( CG_Asset_Parse( handle ) )
				continue;
			else
				break;
		}


		if ( Q_stricmp( token.string, "menudef" ) == 0 ) {
			// start a new menu
			Menu_New( handle );
		}
	}
	trap->PC_FreeSource( handle );
}


qboolean CG_Load_Menu( const char **p ) {

	char *token;

	token = COM_ParseExt( (const char **)p, qtrue );

	if ( token[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt( (const char **)p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 ) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			break;
		}

		CG_ParseMenu( token );
	}
	return qfalse;
}


static qboolean CG_OwnerDrawHandleKey( int ownerDraw, uint32_t flags, float *special, int key ) {
	return qfalse;
}


static int CG_FeederCount( int feederID ) {
	int i, count = 0;

	switch ( feederID ) {
	case FEEDER_REDTEAM_LIST:
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == TEAM_RED )
				count++;
		}
		break;

	case FEEDER_BLUETEAM_LIST:
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == TEAM_BLUE )
				count++;
		}
		break;

	case FEEDER_SCOREBOARD:
		return cg.numScores;

	default:
		break;
	}

	return count;
}


void CG_SetScoreSelection( void *p ) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].team == TEAM_RED ) {
			red++;
		}
		else if ( cg.scores[i].team == TEAM_BLUE ) {
			blue++;
		}
		if ( ps->clientNum == cg.scores[i].client ) {
			cg.selectedScore = i;
		}
	}

	if ( menu == NULL ) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if ( cg.scores[cg.selectedScore].team == TEAM_BLUE ) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection( menu, feeder, i, NULL );
	}
	else {
		Menu_SetFeederSelection( menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL );
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex( int index, int team, int *scoreIndex ) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == team ) {
				if ( count == index ) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[cg.scores[index].client];
}

static const char *CG_FeederItemText( int feederID, int index, int column, qhandle_t *handle1, qhandle_t *handle2,
	qhandle_t *handle3 ) {
	const gitem_t *item;
	int scoreIndex = 0, team = -1;
	clientInfo_t *info = NULL;
	score_t *sp = NULL;

	*handle1 = *handle2 = *handle3 = -1;

	if ( feederID == FEEDER_REDTEAM_LIST )
		team = TEAM_RED;
	else if ( feederID == FEEDER_BLUETEAM_LIST )
		team = TEAM_BLUE;

	info = CG_InfoFromScoreIndex( index, team, &scoreIndex );
	sp = &cg.scores[scoreIndex];

	if ( info && info->infoValid ) {
		switch ( column ) {
		case 0:
			if ( info->powerups & (1 << PW_NEUTRALFLAG) ) {
				item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
				*handle1 = cg_items[ITEM_INDEX( item )].icon;
			}
			else if ( info->powerups & (1 << PW_REDFLAG) ) {
				item = BG_FindItemForPowerup( PW_REDFLAG );
				*handle1 = cg_items[ITEM_INDEX( item )].icon;
			}
			else if ( info->powerups & (1 << PW_BLUEFLAG) ) {
				item = BG_FindItemForPowerup( PW_BLUEFLAG );
				*handle1 = cg_items[ITEM_INDEX( item )].icon;
			}
			break;
		case 1:
			if ( team == -1 ) {
				return "";
			}
			else {
				*handle1 = CG_StatusHandle( info->teamTask );
			}
			break;
		case 2:
			if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << sp->client) ) {
				return "Ready";
			}
			if ( team == -1 ) {
				if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) {
					return va( "%i/%i", info->wins, info->losses );
				}
				else if ( info->infoValid && info->team == TEAM_SPECTATOR ) {
					return "Spectator";
				}
				else {
					return "";
				}
			}
			break;
		case 3:
			return info->name;
		case 4:
			return va( "%i", info->score );
		case 5:
			return va( "%4i", sp->time );
		case 6:
			if ( sp->ping == -1 ) {
				return "connecting";
			}
			return va( "%4i", sp->ping );
		default:
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage( int feederID, int index ) {
	return NULL_HANDLE;
}

static qboolean CG_FeederSelection( int feederID, int index, itemDef_t *item ) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count = 0, team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;

		for ( i = 0; i < cg.numScores; i++ ) {
			if ( cg.scores[i].team == team ) {
				if ( index == count )
					cg.selectedScore = i;
				count++;
			}
		}
	}
	else
		cg.selectedScore = index;

	return qtrue;
}

float CG_Cvar_Get( const char *cvar ) {
	char buff[128];

	trap->Cvar_VariableStringBuffer( cvar, buff, sizeof(buff) );

	return (float)atof( buff );
}

void CG_Text_PaintWithCursor( float x, float y, float scale, const vector4 *color, const char *text, int cursorPos, char cursor, int limit, int style, int iMenuFont, bool customFont ) {
	const Font font( iMenuFont, scale, customFont );
	font.Paint( x, y, text, color, style, limit );
	//FIXME: add cursor code
}

static int CG_OwnerDrawWidth( int ownerDraw, float scale ) {
	const Font font( FONT_MEDIUM, scale, false );
	switch ( ownerDraw ) {
	case CG_GAME_TYPE:
		return font.Width( BG_GetGametypeString( cgs.gametype ) );
	case CG_GAME_STATUS:
		return font.Width( CG_GetGameStatusText() );
	case CG_KILLER:
		return font.Width( CG_GetKillerText() );
	case CG_RED_NAME:
		return font.Width( DEFAULT_REDTEAM_NAME/*cg_redTeamName.string*/ );
	case CG_BLUE_NAME:
		return font.Width( DEFAULT_BLUETEAM_NAME/*cg_blueTeamName.string*/ );
	default:
		break;
	}
	return 0;
}

static int CG_PlayCinematic( const char *name, float x, float y, float w, float h ) {
	return trap->CIN_PlayCinematic( name, x, y, w, h, CIN_loop );
}

static void CG_StopCinematic( int handle ) {
	trap->CIN_StopCinematic( handle );
}

static void CG_DrawCinematic( int handle, float x, float y, float w, float h ) {
	trap->CIN_SetExtents( handle, x, y, w, h );
	trap->CIN_DrawCinematic( handle );
}

static void CG_RunCinematicFrame( int handle ) {
	trap->CIN_RunCinematic( handle );
}

void CG_LoadMenus( const char *menuFile ) {
	const char *token, *p;
	int len;
	fileHandle_t f;
	static char buf[MAX_MENUDEFFILE];

	len = trap->FS_Open( menuFile, &f, FS_READ );

	if ( !f ) {
		if ( Q_StringIsInteger( menuFile ) )
			trap->Print( S_COLOR_GREEN "hud menu file skipped, using default\n" );
		else
			trap->Print( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );

		len = trap->FS_Open( "ui/jahud.txt", &f, FS_READ );
		if ( !f ) {
			trap->Error( ERR_DROP, S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n", menuFile );
			return;
		}
	}

	if ( len >= MAX_MENUDEFFILE ) {
		trap->FS_Close( f );
		trap->Error( ERR_DROP, S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = '\0';
	trap->FS_Close( f );

	p = buf;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token || token[0] == '\0' || token[0] == '}' )
			break;

		if ( Q_stricmp( token, "}" ) == 0 )
			break;

		if ( !Q_stricmp( token, "loadmenu" ) ) {
			if ( CG_Load_Menu( &p ) )
				continue;
			else
				break;
		}
	}

	//	trap->Print( "UI menu load time = %d milli seconds\n", trap->Milliseconds() - start );
}

void CG_LoadHudMenu( void ) {
	const char *hudSet;

	cgDC.registerShaderNoMip = trap->R_RegisterShaderNoMip;
	cgDC.setColor = trap->R_SetColor;
	cgDC.drawHandlePic = CG_DrawPic;
	cgDC.drawStretchPic = trap->R_DrawStretchPic;
	cgDC.registerModel = trap->R_RegisterModel;
	cgDC.modelBounds = trap->R_ModelBounds;
	cgDC.fillRect = CG_FillRect;
	cgDC.drawRect = CG_DrawRect;
	cgDC.drawSides = CG_DrawSides;
	cgDC.drawTopBottom = CG_DrawTopBottom;
	cgDC.clearScene = trap->R_ClearScene;
	cgDC.addRefEntityToScene = SE_R_AddRefEntityToScene;
	cgDC.renderScene = trap->R_RenderScene;
	cgDC.RegisterFont = trap->R_RegisterFont;
	cgDC.Font_StrLenPixels = trap->R_Font_StrLenPixels;
	cgDC.Font_StrLenChars = trap->R_Font_StrLenChars;
	cgDC.Font_HeightPixels = trap->R_Font_HeightPixels;
	cgDC.Font_DrawString = trap->R_Font_DrawString;
	cgDC.Language_IsAsian = trap->R_Language_IsAsian;
	cgDC.Language_UsesSpaces = trap->R_Language_UsesSpaces;
	cgDC.AnyLanguage_ReadCharFromString = trap->R_AnyLanguage_ReadCharFromString;
	cgDC.ownerDrawItem = CG_OwnerDraw;
	cgDC.getValue = CG_GetValue;
	cgDC.ownerDrawVisible = CG_OwnerDrawVisible;
	cgDC.runScript = CG_RunMenuScript;
	cgDC.deferScript = CG_DeferMenuScript;
	cgDC.getTeamColor = CG_GetTeamColor;
	cgDC.setCVar = trap->Cvar_Set;
	cgDC.getCVarString = trap->Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = CG_Text_PaintWithCursor;
	cgDC.startLocalSound = trap->S_StartLocalSound;
	cgDC.ownerDrawHandleKey = CG_OwnerDrawHandleKey;
	cgDC.feederCount = CG_FeederCount;
	cgDC.feederItemImage = CG_FeederItemImage;
	cgDC.feederItemText = CG_FeederItemText;
	cgDC.feederSelection = CG_FeederSelection;
	cgDC.Error = trap->Error;
	cgDC.Print = trap->Print;
	cgDC.ownerDrawWidth = CG_OwnerDrawWidth;
	cgDC.registerSound = trap->S_RegisterSound;
	cgDC.startBackgroundTrack = trap->S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = trap->S_StopBackgroundTrack;
	cgDC.playCinematic = CG_PlayCinematic;
	cgDC.stopCinematic = CG_StopCinematic;
	cgDC.drawCinematic = CG_DrawCinematic;
	cgDC.runCinematicFrame = CG_RunCinematicFrame;

	cgDC.ext.Font_StrLenPixels = trap->ext.R_Font_StrLenPixels;

	Init_Display( &cgDC );

	Menu_Reset();

	hudSet = cg_hudFiles.string;
	if ( hudSet[0] == '\0' ) {
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus( hudSet );

}

void CG_AssetCache( void ) {
	//	Com_Printf( "Menu Size: %i bytes\n", sizeof( Menus ) );
	cgDC.Assets.gradientBar = trap->R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	cgDC.Assets.fxBasePic = trap->R_RegisterShaderNoMip( ART_FX_BASE );
	cgDC.Assets.fxPic[0] = trap->R_RegisterShaderNoMip( ART_FX_RED );
	cgDC.Assets.fxPic[1] = trap->R_RegisterShaderNoMip( ART_FX_YELLOW );
	cgDC.Assets.fxPic[2] = trap->R_RegisterShaderNoMip( ART_FX_GREEN );
	cgDC.Assets.fxPic[3] = trap->R_RegisterShaderNoMip( ART_FX_TEAL );
	cgDC.Assets.fxPic[4] = trap->R_RegisterShaderNoMip( ART_FX_BLUE );
	cgDC.Assets.fxPic[5] = trap->R_RegisterShaderNoMip( ART_FX_CYAN );
	cgDC.Assets.fxPic[6] = trap->R_RegisterShaderNoMip( ART_FX_WHITE );
	cgDC.Assets.scrollBar = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	cgDC.Assets.scrollBarArrowDown = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	cgDC.Assets.scrollBarArrowUp = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	cgDC.Assets.scrollBarArrowLeft = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	cgDC.Assets.scrollBarArrowRight = trap->R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	cgDC.Assets.scrollBarThumb = trap->R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	cgDC.Assets.sliderBar = trap->R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	cgDC.Assets.sliderThumb = trap->R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );
	cgDC.Assets.sliderThumbDefault = trap->R_RegisterShaderNoMip( ASSET_SLIDER_THUMB_DEFAULT );
}

void CG_TransitionPermanent( void ) {
	cg_numpermanents = 0;
	for ( int i = 0; i < MAX_GENTITIES; i++ ) {
		centity_t &cent = cg_entities[i];
		if ( trap->GetDefaultState( i, &cent.currentState ) ) {
			cent.nextState = cent.currentState;
			VectorCopy( &cent.currentState.origin, &cent.lerpOrigin );
			VectorCopy( &cent.currentState.angles, &cent.lerpAngles );
			cent.currentValid = qtrue;

			cg_permanents[cg_numpermanents++] = &cent;
		}
	}
}


// this is a 32k custom pool for parsing ents, it can get reset between ent parsing so we don't need a whole lot of memory
#define MAX_CGSTRPOOL_SIZE (32768)
static int cg_strPoolSize = 0;
static byte cg_strPool[MAX_CGSTRPOOL_SIZE];

char *CG_StrPool_Alloc( int size ) {
	char *giveThemThis;

	if ( cg_strPoolSize + size >= MAX_CGSTRPOOL_SIZE )
		Com_Error( ERR_DROP, "You exceeded the cgame string pool size. Bad programmer!\n" );

	giveThemThis = (char *)&cg_strPool[cg_strPoolSize];
	cg_strPoolSize += size;

	//memset it for them, just to be nice.
	memset( giveThemThis, 0, size );

	return giveThemThis;
}

void CG_StrPool_Reset( void ) {
	cg_strPoolSize = 0;
}

// Builds a copy of the string, translating \n to real linefeeds so message texts can be multi-line
char *CG_NewString( const char *string ) {
	char *newb, *new_p;
	int i, l;

	l = strlen( string ) + 1;

	//	newb = CG_StrPool_Alloc( l );
	newb = (char *)malloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0; i < l; i++ ) {
		if ( string[i] == '\\' && i < l - 1 ) {
			i++;
			if ( string[i] == 'n' )
				*new_p++ = '\n';
			else {
				if ( string[i] == 'r' )
					*new_p++ = '\r';
				else
					*new_p++ = '\\';
			}
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

//data to grab our spawn info into
typedef struct cgSpawnEnt_s {
	float		angle;
	vector3		angles;
	char		*classname;
	float		fogstart;
	vector3		maxs, mins;
	char		*model;
	float		fScale;
	vector3		scale;
	int			onlyFogHere;
	vector3		origin;
	float		radarrange;
	float		zoffset;
} cgSpawnEnt_t;

#define	CGFOFS( x ) offsetof( cgSpawnEnt_t, x )

//spawn fields for our cgame "entity"
static const BG_field_t cg_spawnFields[] = {
	{ "angle", CGFOFS( angle ), F_FLOAT },
	{ "angles", CGFOFS( angles ), F_VECTOR },
	{ "classname", CGFOFS( classname ), F_LSTRING },
	{ "fogstart", CGFOFS( fogstart ), F_FLOAT },
	{ "maxs", CGFOFS( maxs ), F_VECTOR },
	{ "mins", CGFOFS( mins ), F_VECTOR },
	{ "model", CGFOFS( model ), F_LSTRING },
	{ "modelscale", CGFOFS( fScale ), F_FLOAT },
	{ "modelscale_vec", CGFOFS( scale ), F_VECTOR },
	{ "onlyfoghere", CGFOFS( onlyFogHere ), F_INT },
	{ "origin", CGFOFS( origin ), F_VECTOR },
	{ "radarrange", CGFOFS( radarrange ), F_FLOAT },
	{ "zoffset", CGFOFS( zoffset ), F_FLOAT },
};

static int cg_numSpawnVars;
static int cg_numSpawnVarChars;
static char *cg_spawnVars[MAX_SPAWN_VARS][2];
static char cg_spawnVarChars[MAX_SPAWN_VARS_CHARS];

//get some info from the skyportal ent on the map
qboolean cg_noFogOutsidePortal = qfalse;
void CG_CreateSkyPortalFromSpawnEnt( cgSpawnEnt_t *ent ) {
	// only globally fog INSIDE the sky portal
	if ( ent->onlyFogHere )
		cg_noFogOutsidePortal = qtrue;
}

//create a skybox portal orientation entity. there -should- only
//be one of these things per level. if there's more than one the
//next will just stomp over the last. -rww
qboolean cg_skyOri = qfalse;
vector3 cg_skyOriPos;
float cg_skyOriScale = 0.0f;
void CG_CreateSkyOriFromSpawnEnt( cgSpawnEnt_t *ent ) {
	cg_skyOri = qtrue;
	VectorCopy( &ent->origin, &cg_skyOriPos );
	cg_skyOriScale = ent->fScale;
}

//get brush box extents, note this does not care about bsp instances.
void CG_CreateBrushEntData( cgSpawnEnt_t *ent ) {
	trap->R_ModelBounds( trap->R_RegisterModel( ent->model ), &ent->mins, &ent->maxs );
}

void CG_CreateWeatherZoneFromSpawnEnt( cgSpawnEnt_t *ent ) {
	CG_CreateBrushEntData( ent );
	trap->WE_AddWeatherZone( &ent->mins, &ent->maxs );
}

//create a new cgame-only model
void CG_CreateModelFromSpawnEnt( cgSpawnEnt_t *ent ) {
	int modelIndex;
	refEntity_t *RefEnt;
	vector3 mins, maxs;
	float *radius, *zOff;

	if ( NumMiscEnts >= MAX_MISC_ENTS ) {
		Com_Error( ERR_DROP, "Too many misc_model_static's on level, ask a programmer to raise the limit (currently "
			XSTRING( MAX_MISC_ENTS ) "), or take some out." );
		return;
	}

	if ( !ent || !ent->model || !ent->model[0] ) {
		Com_Error( ERR_DROP, "misc_model_static with no model." );
		return;
	}

	radius = &Radius[NumMiscEnts];
	zOff = &zOffset[NumMiscEnts];
	RefEnt = &MiscEnts[NumMiscEnts++];

	modelIndex = trap->R_RegisterModel( ent->model );
	if ( modelIndex == 0 ) {
		Com_Error( ERR_DROP, "misc_model_static failed to load model '%s'", ent->model );
		return;
	}

	memset( RefEnt, 0, sizeof(refEntity_t) );
	RefEnt->reType = RT_MODEL;
	RefEnt->hModel = modelIndex;
	RefEnt->frame = 0;
	trap->R_ModelBounds( modelIndex, &mins, &maxs );
	VectorCopy( &ent->scale, &RefEnt->modelScale );
	//use same scale on each axis then
	if ( ent->fScale )
		RefEnt->modelScale.x = RefEnt->modelScale.y = RefEnt->modelScale.z = ent->fScale;
	VectorCopy( &ent->origin, &RefEnt->origin );
	VectorCopy( &ent->origin, &RefEnt->lightingOrigin );

	VectorScaleVector( &mins, &ent->scale, &mins );
	VectorScaleVector( &maxs, &ent->scale, &maxs );
	*radius = Distance( &mins, &maxs );
	*zOff = ent->zoffset;

	// only yaw supplied...
	if ( ent->angle )
		ent->angles.yaw = ent->angle;

	AnglesToAxis( &ent->angles, RefEnt->axis );
	ScaleModelAxis( RefEnt );
}

char *CG_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( cg_numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		trap->Error( ERR_DROP, "CG_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = cg_spawnVarChars + cg_numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	cg_numSpawnVarChars += l + 1;

	return dest;
}

// cgame version of G_ParseSpawnVars, for ents that don't really need to take up an entity slot (e.g. static models)
qboolean CG_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	cg_numSpawnVars = 0;
	cg_numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap->R_GetEntityToken( com_token, sizeof(com_token) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		trap->Error( ERR_DROP, "CG_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap->R_GetEntityToken( keyname, sizeof(keyname) ) ) {
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap->R_GetEntityToken( com_token, sizeof(com_token) ) ) { //this happens on mike's test level, I don't know why. Fixme?
			//trap->Error( ERR_DROP, "CG_ParseSpawnVars: EOF without closing brace" );
			break;
		}

		if ( com_token[0] == '}' ) {
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: closing brace without data" );
		}
		if ( cg_numSpawnVars == MAX_SPAWN_VARS ) {
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		cg_spawnVars[cg_numSpawnVars][0] = CG_AddSpawnVarToken( keyname );
		cg_spawnVars[cg_numSpawnVars][1] = CG_AddSpawnVarToken( com_token );
		cg_numSpawnVars++;
	}

	return qtrue;
}

void BG_ParseField( const BG_field_t *l_fields, int numFields, const char *key, const char *value, byte *ent );

extern float cg_linearFogOverride; //cg_view.c
extern float cg_radarRange;//cg_draw.c

// See if we should do something for this ent cgame-side
void CG_SpawnCGameEntFromVars( void ) {
	int i;
	cgSpawnEnt_t ent;

	memset( &ent, 0, sizeof(ent) );

	for ( i = 0; i < cg_numSpawnVars; i++ ) { //shove all this stuff into our data structure used specifically for getting spawn info
		BG_ParseField( cg_spawnFields, ARRAY_LEN( cg_spawnFields ), cg_spawnVars[i][0], cg_spawnVars[i][1], (byte *)&ent );
	}

	if ( ent.classname && ent.classname[0] ) { //we'll just stricmp this bastard, since there aren't all that many cgame-only things, and they all have special handling
		if ( !Q_stricmp( ent.classname, "worldspawn" ) ) { //I'd like some info off this guy
			if ( ent.fogstart ) { //linear fog method
				cg_linearFogOverride = ent.fogstart;
			}
			//get radarRange off of worldspawn
			if ( ent.radarrange ) { //linear fog method
				cg_radarRange = ent.radarrange;
			}
		}
		else if ( !Q_stricmp( ent.classname, "misc_model_static" ) ) { //we've got us a static model
			CG_CreateModelFromSpawnEnt( &ent );
		}
		else if ( !Q_stricmp( ent.classname, "misc_skyportal_orient" ) ) { //a sky portal orientation point
			CG_CreateSkyOriFromSpawnEnt( &ent );
		}
		else if ( !Q_stricmp( ent.classname, "misc_skyportal" ) ) { //might as well parse this thing cgame side for the extra info I want out of it
			CG_CreateSkyPortalFromSpawnEnt( &ent );
		}
		else if ( !Q_stricmp( ent.classname, "misc_weather_zone" ) ) { //might as well parse this thing cgame side for the extra info I want out of it
			CG_CreateWeatherZoneFromSpawnEnt( &ent );
		}
	}

	//reset the string pool for the next entity, if there is one
	CG_StrPool_Reset();
}

// Parses entity string data for cgame-only entities, that we can throw away on the server and never even bother sending
void CG_SpawnCGameOnlyEnts( void ) {
	//make sure it is reset
	trap->R_GetEntityToken( NULL, -1 );

	if ( !CG_ParseSpawnVars() ) { //first one is gonna be the world spawn
		trap->Error( ERR_DROP, "no entities for cgame parse" );
	}
	else { //parse the world spawn info we want
		CG_SpawnCGameEntFromVars();
	}

	while ( CG_ParseSpawnVars() ) { //now run through the whole list, and look for things we care about cgame-side
		CG_SpawnCGameEntFromVars();
	}
}

/*
Ghoul2 Insert End
*/

extern playerState_t *cgSendPS[MAX_GENTITIES]; //is not MAX_CLIENTS because NPCs exceed MAX_CLIENTS
void CG_PmoveClientPointerUpdate( void );

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );

#define LOG_DIRECTORY "logs/cl/"
static void CG_OpenLog( const char *filename, fileHandle_t *f, qboolean sync ) {
	trap->FS_Open( filename, f, sync ? FS_APPEND_SYNC : FS_APPEND );
	if ( *f )
		trap->Print( "Logging to %s\n", filename );
	else
		trap->Print( "WARNING: Couldn't open logfile: %s\n", filename );
}

static void CG_CloseLog( fileHandle_t *f ) {
	if ( !*f )
		return;

	trap->FS_Close( *f );
	*f = NULL_FILE;
}

// Called after every level change or subsystem restart
// Will perform callbacks to make the loading info screen update.
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback ) {
	char buf[64];
	const char	*s;

	// clear out globals
	memset( cg_entities, 0, sizeof(cg_entities) );

	cgs.~cgs_t();
	new( &cgs ) cgs_t{}; // Using {} instead of () to work around MSVC bug
	cg.~cg_t();
	new( &cg ) cg_t{};

	memset( cg_items, 0, sizeof(cg_items) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );

	trap->RegisterSharedMemory( cg.sharedBuffer );

	CG_RegisterCvars();
	CG_InitConsoleCommands();

	// logging
	if ( cg_logConsole.integer )	CG_OpenLog( LOG_DIRECTORY "console.log", &cg.log.console, cg_logConsole.integer == 2 );
	else							trap->Print( "Not logging console to disk.\n" );
	if ( cg_logChat.integer )		CG_OpenLog( LOG_DIRECTORY "chat.log", &cg.log.chat, cg_logChat.integer == 2 );
	else							trap->Print( "Not logging chat to disk.\n" );
	if ( cg_logDebug.integer )		CG_OpenLog( LOG_DIRECTORY "debug.log", &cg.log.debug, cg_logDebug.integer == 2 );
	else							trap->Print( "Not logging debug messages to disk.\n" );
	if ( cg_logSecurity.integer )	CG_OpenLog( LOG_DIRECTORY "security.log", &cg.log.security, cg_logSecurity.integer == 2 );
	else							trap->Print( "Not logging security events to disk.\n" );

	// load some permanent stuff
	BG_InitAnimsets();
	BG_VehicleLoadParms();
	CG_InitJetpackGhoul2();
	CG_PmoveClientPointerUpdate();

	CG_PreloadMedia();

	cg.clientNum = clientNum;
	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;
	cg.itemSelect = -1;
	cg.forceSelect = 0xFFFFFFFFu;
	cg.forceHUDActive = qtrue;
	cg.forceHUDTotalFlashTime = 0;
	cg.forceHUDNextFlashTime = 0;
	cg.renderingThirdPerson = cg_thirdPerson.integer;
	cg.weaponSelect = WP_BRYAR_PISTOL;
	cgs.redflag = cgs.blueflag = FLAG_ATBASE;
	cg.demoPlayback = demoPlayback;
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );

	trap->GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / SCREEN_WIDTH;
	cgs.screenYScale = cgs.glconfig.vidHeight / SCREEN_HEIGHT;
	CG_Set2DRatio();

	trap->GetGameState( &cgs.gameState );
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		trap->Error( ERR_DROP, "Client/Server game mismatch: " GAME_VERSION "/%s", s );
		return;
	}

	CG_TransitionPermanent();

	CG_LoadingString( "Notifications" );
	CG_NotifyInit();

	CG_LoadingString( "Server info" );
	CG_ParseServerinfo();

	CG_LoadingString( "String pool" );
	String_Init();

	CG_LoadingString( "Saber hilts" );
	WP_SaberLoadParms();

	//CG_LoadingString( "Media" );
	CG_LoadMedia();
	cgs.activeCursor = media.gfx.interface.cursor;

	CG_LoadingString( "Siege data" );
	CG_InitSiegeMode();

	CG_LoadingString( "Trueview" );
	CG_TrueViewInit();

	CG_LoadingString( "Asset cache" );
	CG_AssetCache();

	CG_LoadingString( "HUD" );
	CG_LoadHudMenu(); // load new hud stuff

	CG_LoadingString( "Players" );
	CG_RegisterClients();

	CG_LoadingString( "Local entities" );
	CG_InitLocalEntities();

	CG_LoadingString( "Marks" );
	CG_InitMarkPolys();

	CG_LoadingString( "JPLua" );
	JPLua::Init();

	CG_LoadingString( "Chatbox" );
	CG_ChatboxInit();

	CG_SetConfigValues(); // Make sure we have update values (scores)
	CG_StartMusic( qfalse );
	CG_ClearLightStyles();
	CG_ShaderStateChanged();
	trap->S_ClearLoopingSounds();
	cg.distanceCull = trap->R_GetDistanceCull();
	CG_SpawnCGameOnlyEnts();

#ifdef _WIN32
	{//Detect the timestamp format via the registry
#define JP_TIMESTAMP_REGISTRY_KEY "Control Panel\\International"
#define JP_TIMESTAMP_REGISTRY_NAME "sTimeFormat"
		char registryValue[256] = { 0 };
		HKEY hkey;
		unsigned long datalen = sizeof(registryValue);  // data field length(in), data returned length(out)
		unsigned long datatype; // #defined in winnt.h (predefined types 0-11)
		LONG error;
		if ( (error = RegOpenKeyExA( (HKEY)HKEY_CURRENT_USER, (LPCSTR)JP_TIMESTAMP_REGISTRY_KEY, (DWORD)0,
			(REGSAM)KEY_QUERY_VALUE, &hkey )) == ERROR_SUCCESS )
		{
			if ( (error = RegQueryValueExA( (HKEY)hkey, (LPCSTR)JP_TIMESTAMP_REGISTRY_NAME, (LPDWORD)NULL,
				(LPDWORD)&datatype, (LPBYTE)registryValue, (LPDWORD)&datalen )) == ERROR_SUCCESS )
			{
				if ( registryValue[0] == 'H' ) {
					cg.japp.timestamp24Hour = qtrue;
				}
				RegCloseKey( hkey );
			}
			else {
				Com_Printf( S_COLOR_RED "Error, couldn't query registry string " JP_TIMESTAMP_REGISTRY_NAME ", error code %i\n", error );
			}
		}
		else {
			Com_Printf( S_COLOR_RED "Error, couldn't open registry key " JP_TIMESTAMP_REGISTRY_KEY ", error code %i\n", error );
		}
	}
#endif

	CG_LoadingString( "" );

	// post-init stuff
	trap->Cvar_VariableStringBuffer( "rate", buf, sizeof(buf) );
	if ( atoi( buf ) == 4000 ) {
		trap->Print( S_COLOR_YELLOW "WARNING: Default /rate value detected. Suggest typing /rate 25000 for a smoother "
			"connection!\n" );
	}

	CG_UpdateServerHistory();
}

//makes sure returned string is in localized format
const char *CG_GetLocationString( const char *loc ) {
	static char text[1024] = { 0 };

	if ( !loc || loc[0] != '@' ) { //just a raw string
		return loc;
	}

	trap->SE_GetStringTextString( loc + 1, text, sizeof(text) );
	return text;
}

//clean up all the ghoul2 allocations, the nice and non-hackly way -rww
void CG_KillCEntityG2( int entNum );
void CG_DestroyAllGhoul2( void ) {
	int i = 0;
	int j;

	//	Com_Printf("... CGameside GHOUL2 Cleanup\n");
	while ( i < MAX_GENTITIES ) { //free all dynamically allocated npc client info structs and ghoul2 instances
		CG_KillCEntityG2( i );
		i++;
	}

	//Clean the weapon instances
	CG_ShutDownG2Weapons();

	i = 0;
	while ( i < MAX_ITEMS ) { //and now for items
		j = 0;
		while ( j < MAX_ITEM_MODELS ) {
			if ( cg_items[i].g2Models[j] && trap->G2_HaveWeGhoul2Models( cg_items[i].g2Models[j] ) ) {
				trap->G2API_CleanGhoul2Models( &cg_items[i].g2Models[j] );
				cg_items[i].g2Models[j] = NULL;
			}
			j++;
		}
		i++;
	}

	//Clean the global jetpack instance
	CG_CleanJetpackGhoul2();
}

// Called before every level change or subsystem restart
void CG_Shutdown( void ) {
	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

	CG_DestroyAllGhoul2();

	//	Com_Printf("... FX System Cleanup\n");
	trap->FX_FreeSystem();
	trap->ROFF_Clean();

	//reset weather
	trap->R_WorldEffectCommand( "die" );

	UI_CleanupGhoul2();
	//If there was any ghoul2 stuff in our side of the shared ui code, then remove it now.

	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

#ifdef JPLUA
	//Raz: Lua!
	JPLua::Shutdown( qfalse );
#endif // JPLUA

	CG_NotifyShutdown();

	// close log files
	CG_LogPrintf( cg.log.chat, "End logging\n------------------------------------------------------------\n\n" );
	CG_LogPrintf( cg.log.console, "End logging\n------------------------------------------------------------\n\n" );
	CG_CloseLog( &cg.log.chat );
	CG_CloseLog( &cg.log.console );
	CG_CloseLog( &cg.log.security );
}

void CG_NextForcePower_f( void ) {
	int current;
	usercmd_t cmd;
	if ( !cg.snap ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ) {
		return;
	}

	current = trap->GetCurrentCmdNumber();
	trap->GetUserCmd( current, &cmd );
	if ( (cmd.buttons & BUTTON_USE) || CG_NoUseableForce() ) {
		CG_NextInventory_f();
		return;
	}

	if ( CG_IsSpectating() ) {
		return;
	}

	//	BG_CycleForce(&cg.snap->ps, 1);
	if ( cg.forceSelect != -1 ) {
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce( &cg.snap->ps, 1 );

	if ( cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected) ) {
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

void CG_PrevForcePower_f( void ) {
	int current;
	usercmd_t cmd;
	if ( !cg.snap ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ) {
		return;
	}

	current = trap->GetCurrentCmdNumber();
	trap->GetUserCmd( current, &cmd );
	if ( (cmd.buttons & BUTTON_USE) || CG_NoUseableForce() ) {
		CG_PrevInventory_f();
		return;
	}

	if ( CG_IsSpectating() ) {
		return;
	}

	//	BG_CycleForce(&cg.snap->ps, -1);
	if ( cg.forceSelect != -1 ) {
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce( &cg.snap->ps, -1 );

	if ( cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected) ) {
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

void CG_NextInventory_f( void ) {
	if ( !cg.snap ) {
		return;
	}

	if ( CG_IsSpectating() ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ) {
		return;
	}

	if ( cg.itemSelect != -1 ) {
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag( cg.itemSelect, IT_HOLDABLE );
	}
	BG_CycleInven( &cg.snap->ps, 1 );

	if ( cg.snap->ps.stats[STAT_HOLDABLE_ITEM] ) {
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}

void CG_PrevInventory_f( void ) {
	if ( !cg.snap ) {
		return;
	}

	if ( CG_IsSpectating() ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ) {
		return;
	}

	if ( cg.itemSelect != -1 ) {
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag( cg.itemSelect, IT_HOLDABLE );
	}
	BG_CycleInven( &cg.snap->ps, -1 );

	if ( cg.snap->ps.stats[STAT_HOLDABLE_ITEM] ) {
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}

qboolean CG_IsFollowing( void ) {
	if ( !cg.snap )
		return qfalse;

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
		return qtrue;

	return qfalse;
}

qboolean CG_IsSpectating( void ) {
	if ( !cg.snap )
		return qfalse;

	if ( CG_IsFollowing() || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
		|| cg.predictedPlayerState.pm_type == PM_SPECTATOR ) {
		return qtrue;
	}

	return qfalse;
}

char *ConcatArgs( int start ) {
	int i, c, tlen, len;
	static char line[MAX_STRING_CHARS];
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap->Cmd_Argc();
	for ( i = start; i < c; i++ ) {
		trap->Cmd_Argv( i, arg, sizeof(arg) );
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

	line[len] = '\0';

	return line;
}

// Print to the logfile with a time stamp if it is open
void CG_LogPrintf( fileHandle_t fileHandle, const char *fmt, ... ) {
	va_list argptr;
	char string[1024] = { 0 };
	size_t len;

	if ( cg_logFormat.integer == 0 ) {
		int msec = cg.time - cgs.levelStartTime;
		int secs = msec / 1000;
		int mins = secs / 60;
		secs %= 60;
		msec %= 1000;

		Com_sprintf( string, sizeof(string), "%i:%02i ", mins, secs );
	}
	else {
		time_t rawtime;
		time( &rawtime );
		strftime( string, sizeof(string), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &rawtime ) );
	}

	len = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + len, sizeof(string)-len, fmt, argptr );
	va_end( argptr );

	if ( !fileHandle )
		return;

	trap->FS_Write( string, strlen( string ), fileHandle );
}

static qboolean CG_IncomingConsoleCommand( void ) {
	//rww - let mod authors filter client console messages so they can cut them off if they want.
	//return qtrue if the command is ok. Otherwise, you can set char 0 on the command str to 0 and return
	//qfalse to not execute anything, or you can fill conCommand in with something valid and return 0
	//in order to have that string executed in place. Some example code:
#if 0
	TCGIncomingConsoleCommand *icc = (TCGIncomingConsoleCommand *)cg.sharedBuffer;
	if ( strstr( icc->conCommand, "wait" ) )
	{ //filter out commands contaning wait
		Com_Printf( "You can't use commands containing the string wait with MyMod v1.0\n" );
		icc->conCommand[0] = '\0';
		return qfalse;
	}
	else if ( strstr( icc->conCommand, "blah" ) )
	{ //any command containing the string "blah" is redirected to "quit"
		strcpy( icc->conCommand, "quit" );
		return qfalse;
	}
#endif
	return qtrue;
}

static void CG_GetOrigin( int entID, vector3 *out ) {
	VectorCopy( &cg_entities[entID].currentState.pos.trBase, out );
}

static void CG_GetAngles( int entID, vector3 *out ) {
	VectorCopy( &cg_entities[entID].currentState.apos.trBase, out );
}

static trajectory_t *CG_GetOriginTrajectory( int entID ) {
	return &cg_entities[entID].nextState.pos;
}

static trajectory_t *CG_GetAngleTrajectory( int entID ) {
	return &cg_entities[entID].nextState.apos;
}

static void _CG_ROFF_NotetrackCallback( int entID, const char *notetrack ) {
	CG_ROFF_NotetrackCallback( &cg_entities[entID], notetrack );
}

static void CG_MapChange( void ) {
	// this may be called more than once for a given map change, as the server is going to attempt to send out
	// multiple broadcasts in hopes that the client will receive one of them
	cg.mMapChange = qtrue;
}

static void CG_AutomapInput( void ) {
	autoMapInput_t *autoInput = (autoMapInput_t *)cg.sharedBuffer;

	memcpy( &cg_autoMapInput, autoInput, sizeof(autoMapInput_t) );

#if 0
	if ( !arg0 ) //if this is non-0, it's actually a one-frame mouse event
		cg_autoMapInputTime = cg.time + 1000;
	else
#endif
	{
		if ( cg_autoMapInput.yaw )		cg.automapAngle.yaw += cg_autoMapInput.yaw;
		if ( cg_autoMapInput.pitch )	cg.automapAngle.pitch += cg_autoMapInput.pitch;
		cg_autoMapInput.yaw = 0.0f;
		cg_autoMapInput.pitch = 0.0f;
	}
}

static void CG_FX_CameraShake( void ) {
	TCGCameraShake *data = (TCGCameraShake *)cg.sharedBuffer;
	CG_DoCameraShake( &data->mOrigin, data->mIntensity, data->mRadius, data->mTime );
}

cgameImport_t *trap = NULL;

Q_CABI {
Q_EXPORT cgameExport_t *GetModuleAPI( int apiVersion, cgameImport_t *import ) {
	static cgameExport_t cge = { 0 };

	assert( import );
	trap = import;
	Com_Printf = trap->Print;
	Com_Error = trap->Error;

	memset( &cge, 0, sizeof(cge) );

	if ( apiVersion != CGAME_API_VERSION ) {
		trap->Print( "Mismatched CGAME_API_VERSION: expected %i, got %i\n", CGAME_API_VERSION, apiVersion );
		return NULL;
	}

	cge.Init = CG_Init;
	cge.Shutdown = CG_Shutdown;
	cge.ConsoleCommand = CG_ConsoleCommand;
	cge.DrawActiveFrame = CG_DrawActiveFrame;
	cge.CrosshairPlayer = CG_CrosshairPlayer;
	cge.LastAttacker = CG_LastAttacker;
	cge.KeyEvent = CG_KeyEvent;
	cge.MouseEvent = CG_MouseEvent;
	cge.EventHandling = CG_EventHandling;
	cge.PointContents = C_PointContents;
	cge.GetLerpOrigin = C_GetLerpOrigin;
	cge.GetLerpData = C_GetLerpData;
	cge.Trace = C_Trace;
	cge.G2Trace = C_G2Trace;
	cge.G2Mark = C_G2Mark;
	cge.RagCallback = CG_RagCallback;
	cge.IncomingConsoleCommand = CG_IncomingConsoleCommand;
	cge.NoUseableForce = CG_NoUseableForce;
	cge.GetOrigin = CG_GetOrigin;
	cge.GetAngles = CG_GetAngles;
	cge.GetOriginTrajectory = CG_GetOriginTrajectory;
	cge.GetAngleTrajectory = CG_GetAngleTrajectory;
	cge.ROFF_NotetrackCallback = _CG_ROFF_NotetrackCallback;
	cge.MapChange = CG_MapChange;
	cge.AutomapInput = CG_AutomapInput;
	cge.MiscEnt = CG_MiscEnt;
	cge.CameraShake = CG_FX_CameraShake;

	return &cge;
}

// This is the only way control passes into the module.
// This must be the very first function compiled into the .q3vm file
Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 ) {
	switch ( command ) {

	case CG_INIT:
		CG_Init( arg0, arg1, arg2, qfalse );
		return 0;

	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;

	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();

	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame( arg0, (stereoFrame_t)arg1, (qboolean)arg2 );
		return 0;

	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();

	case CG_LAST_ATTACKER:
		return CG_LastAttacker();

	case CG_KEY_EVENT:
		CG_KeyEvent( arg0, arg1 );
		return 0;

	case CG_MOUSE_EVENT:
		CG_MouseEvent( arg0, arg1 );
		return 0;

	case CG_EVENT_HANDLING:
		CG_EventHandling( arg0 );
		return 0;

	case CG_POINT_CONTENTS:
		return C_PointContents();

	case CG_GET_LERP_ORIGIN:
		C_GetLerpOrigin();
		return 0;

	case CG_GET_LERP_DATA:
		C_GetLerpData();
		return 0;

	case CG_GET_GHOUL2:
		return (intptr_t)cg_entities[arg0].ghoul2; //NOTE: This is used by the effect bolting which is actually not used at all.
		//I'm fairly sure if you try to use it with vm's it will just give you total
		//garbage. In other words, use at your own risk.

	case CG_GET_MODEL_LIST:
		return (intptr_t)cgs.gameModels;

	case CG_CALC_LERP_POSITIONS:
		CG_CalcEntityLerpPositions( &cg_entities[arg0] );
		return 0;

	case CG_TRACE:
		C_Trace();
		return 0;
	case CG_GET_SORTED_FORCE_POWER:
		return forcePowerSorted[arg0];
	case CG_G2TRACE:
		C_G2Trace();
		return 0;

	case CG_G2MARK:
		C_G2Mark();
		return 0;

	case CG_RAG_CALLBACK:
		return CG_RagCallback( arg0 );

	case CG_INCOMING_CONSOLE_COMMAND:
		return CG_IncomingConsoleCommand();

	case CG_GET_USEABLE_FORCE:
		return CG_NoUseableForce();

	case CG_GET_ORIGIN:
		CG_GetOrigin( arg0, (vector3 *)arg1 );
		return 0;

	case CG_GET_ANGLES:
		CG_GetAngles( arg0, (vector3 *)arg1 );
		return 0;

	case CG_GET_ORIGIN_TRAJECTORY:
		return (intptr_t)CG_GetOriginTrajectory( arg0 );

	case CG_GET_ANGLE_TRAJECTORY:
		return (intptr_t)CG_GetAngleTrajectory( arg0 );

	case CG_ROFF_NOTETRACK_CALLBACK:
		_CG_ROFF_NotetrackCallback( arg0, (const char *)arg1 );
		return 0;

	case CG_IMPACT_MARK:
		C_ImpactMark();
		return 0;

	case CG_MAP_CHANGE:
		CG_MapChange();
		return 0;

	case CG_AUTOMAP_INPUT:
		CG_AutomapInput();
		return 0;

	case CG_MISC_ENT:
		CG_MiscEnt();
		return 0;

	case CG_FX_CAMERASHAKE:
		CG_FX_CameraShake();
		return 0;

	default:
		trap->Error( ERR_DROP, "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}
}
