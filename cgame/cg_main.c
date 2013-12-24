// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

#include "cg_lua.h"

#include "ui/ui_shared.h"
// display context for new ui stuff
displayContextDef_t cgDC;

#include "cg_lights.h"

extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;
/*
Ghoul2 Insert Start
*/
void CG_InitItems(void);
/*
Ghoul2 Insert End
*/

void CG_InitJetpackGhoul2(void);
void CG_CleanJetpackGhoul2(void);

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback );
void CG_Shutdown( void );

void CG_CalcEntityLerpPositions( centity_t *cent );
void CG_ROFF_NotetrackCallback( centity_t *cent, const char *notetrack);

void UI_CleanupGhoul2(void);

#define MAX_MISC_ENTS	4000

//static refEntity_t	*MiscEnts = 0;
//static float		*Radius = 0;
static refEntity_t	MiscEnts[MAX_MISC_ENTS]; //statically allocated for now.
static float		Radius[MAX_MISC_ENTS];
static float		zOffset[MAX_MISC_ENTS]; //some models need a z offset for culling, because of stupid wrong model origins

static int			NumMiscEnts = 0;

extern autoMapInput_t cg_autoMapInput; //cg_view.c
extern int cg_autoMapInputTime;
//extern vector3 cg_autoMapAngle;

void CG_MiscEnt(void);
void CG_DoCameraShake( vector3 *origin, float intensity, int radius, int time );

//do we have any force powers that we would normally need to cycle to?
qboolean CG_NoUseableForce(void)
{
	int i = FP_HEAL;
	while (i < NUM_FORCE_POWERS)
	{
		if (i != FP_SABERTHROW &&
			i != FP_SABER_OFFENSE &&
			i != FP_SABER_DEFENSE &&
			i != FP_LEVITATION)
		{ //valid selectable power
			if (cg.predictedPlayerState.fd.forcePowersKnown & (1 << i))
			{ //we have it
				return qfalse;
			}
		}
		i++;
	}

	//no useable force powers, I guess.
	return qtrue;
}

static int C_PointContents(void)
{
	TCGPointContents	*data = (TCGPointContents *)cg.sharedBuffer;

	return CG_PointContents( &data->mPoint, data->mPassEntityNum );
}

static void C_GetLerpOrigin(void)
{
	TCGVectorData		*data = (TCGVectorData *)cg.sharedBuffer;

	VectorCopy(&cg_entities[data->mEntityNum].lerpOrigin, &data->mPoint);
}

static void C_GetLerpData(void)
{//only used by FX system to pass to getboltmat
	TCGGetBoltData		*data = (TCGGetBoltData *)cg.sharedBuffer;

	VectorCopy(&cg_entities[data->mEntityNum].lerpOrigin, &data->mOrigin);
	VectorCopy(&cg_entities[data->mEntityNum].modelScale, &data->mScale);
	VectorCopy(&cg_entities[data->mEntityNum].lerpAngles, &data->mAngles);
	if (cg_entities[data->mEntityNum].currentState.eType == ET_PLAYER)
	{ //normal player
		data->mAngles.pitch = 0.0f;
		data->mAngles.roll = 0.0f;
	}
	else if (cg_entities[data->mEntityNum].currentState.eType == ET_NPC)
	{ //an NPC
		Vehicle_t *pVeh = cg_entities[data->mEntityNum].m_pVehicle;
		if (!pVeh)
		{ //for vehicles, we may or may not want to 0 out pitch and roll
			data->mAngles.pitch = 0.0f;
			data->mAngles.roll = 0.0f;
		}
		else if (pVeh->m_pVehicleInfo->type == VH_SPEEDER)
		{ //speeder wants no pitch but a roll
			data->mAngles.pitch = 0.0f;
		}
		else if (pVeh->m_pVehicleInfo->type != VH_FIGHTER)
		{ //fighters want all angles
			data->mAngles.pitch = 0.0f;
			data->mAngles.roll = 0.0f;
		}
	}
}

static void C_Trace(void)
{
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_Trace(&td->mResult, &td->mStart, &td->mMins, &td->mMaxs, &td->mEnd, td->mSkipNumber, td->mMask);
}

static void C_G2Trace(void)
{
	TCGTrace	*td = (TCGTrace *)cg.sharedBuffer;

	CG_G2Trace(&td->mResult, &td->mStart, &td->mMins, &td->mMaxs, &td->mEnd, td->mSkipNumber, td->mMask);
}

static void C_G2Mark(void)
{
	TCGG2Mark	*td = (TCGG2Mark *)cg.sharedBuffer;
	trace_t		trace;
	vector3		end;

	VectorMA(&td->start, 64, &td->dir, &end);
	CG_G2Trace(&trace, &td->start, NULL, NULL, &end, ENTITYNUM_NONE, MASK_PLAYERSOLID);

	if (trace.entityNum < ENTITYNUM_WORLD &&
		cg_entities[trace.entityNum].ghoul2)
	{ //hit someone with a ghoul2 instance, let's project the decal on them then.
		centity_t *cent = &cg_entities[trace.entityNum];

		//CG_TestLine(tr.endpos, end, 2000, 0x0000ff, 1);

		CG_AddGhoul2Mark(td->shader, td->size, &trace.endpos, &end, trace.entityNum,
			&cent->lerpOrigin, cent->lerpAngles.yaw, cent->ghoul2, &cent->modelScale,
			Q_irand(2000, 4000));
		//I'm making fx system decals have a very short lifetime.
	}
}

static void CG_DebugBoxLines(vector3 *mins, vector3 *maxs, int duration)
{
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
	CG_TestLine(&start, &vert, duration, 0x00000ff, 1);

	vert.x = mins->x;
	vert.y = maxs->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine(&start, &vert, duration, 0x00000ff, 1);

	vert.x = maxs->x;
	vert.y = mins->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine(&start, &vert, duration, 0x00000ff, 1);

	vert.x = maxs->x;
	vert.y = maxs->y;
	start.x = vert.x;
	start.y = vert.y;
	CG_TestLine(&start, &vert, duration, 0x00000ff, 1);

	// top of box
	VectorCopy(maxs, &start);
	VectorCopy(maxs, &end);
	start.x -= x;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	end.x = start.x;
	end.y -= y;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	start.y = end.y;
	start.x += x;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	CG_TestLine(&start, maxs, duration, 0x00000ff, 1);

	// bottom of box
	VectorCopy(mins, &start);
	VectorCopy(mins, &end);
	start.x += x;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	end.x = start.x;
	end.y += y;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	start.y = end.y;
	start.x -= x;
	CG_TestLine(&start, &end, duration, 0x00000ff, 1);
	CG_TestLine(&start, mins, duration, 0x00000ff, 1);
}

//handle ragdoll callbacks, for events and debugging -rww
static int CG_RagCallback(int callType)
{
	switch(callType)
	{
	case RAG_CALLBACK_DEBUGBOX:
		{
			ragCallbackDebugBox_t *callData = (ragCallbackDebugBox_t *)cg.sharedBuffer;

			CG_DebugBoxLines(&callData->mins, &callData->maxs, callData->duration);
		}
		break;
	case RAG_CALLBACK_DEBUGLINE:
		{
			ragCallbackDebugLine_t *callData = (ragCallbackDebugLine_t *)cg.sharedBuffer;

			CG_TestLine(&callData->start, &callData->end, callData->time, callData->color, callData->radius);
		}
		break;
	case RAG_CALLBACK_BONESNAP:
		{
			ragCallbackBoneSnap_t *callData = (ragCallbackBoneSnap_t *)cg.sharedBuffer;
			centity_t *cent = &cg_entities[callData->entNum];
			int snapSound = trap->S_RegisterSound(va("sound/player/bodyfall_human%i.wav", Q_irand(1, 3)));

			trap->S_StartSound(&cent->lerpOrigin, callData->entNum, CHAN_AUTO, snapSound);
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

			CG_Trace(&callData->tr, &callData->start, &callData->mins, &callData->maxs,
				&callData->end, callData->ignore, callData->mask);
		}
		break;
	default:
		Com_Error(ERR_DROP, "Invalid callType in CG_RagCallback");
		break;
	}

	return 0;
}

static void C_ImpactMark(void)
{
	TCGImpactMark	*data = (TCGImpactMark *)cg.sharedBuffer;

	/*
	CG_ImpactMark((int)arg0, (const float *)arg1, (const float *)arg2, (float)arg3,
		(float)arg4, (float)arg5, (float)arg6, (float)arg7, qtrue, (float)arg8, qfalse);
	*/
	CG_ImpactMark(data->mHandle, &data->mPoint, &data->mAngle, data->mRotation,
		data->mRed, data->mGreen, data->mBlue, data->mAlphaStart, qtrue, data->mSizeStart, qfalse);
}

void CG_MiscEnt(void)
{
	int			modelIndex;
	refEntity_t	*RefEnt;
	TCGMiscEnt	*data = (TCGMiscEnt *)cg.sharedBuffer;
	vector3		mins, maxs;
	float		*radius, *zOff;

	if (NumMiscEnts >= MAX_MISC_ENTS)
	{
		return;
	}
	
	radius = &Radius[NumMiscEnts];
	zOff = &zOffset[NumMiscEnts];
	RefEnt = &MiscEnts[NumMiscEnts++];

	modelIndex = trap->R_RegisterModel(data->mModel);
	if (modelIndex == 0)
	{
		Com_Error(ERR_DROP, "client_model has invalid model definition");
		return;
	}

	*zOff = 0;

	memset(RefEnt, 0, sizeof(refEntity_t));
	RefEnt->reType = RT_MODEL;
	RefEnt->hModel = modelIndex;
	RefEnt->frame = 0;
	trap->R_ModelBounds(modelIndex, &mins, &maxs);
	VectorCopy(&data->mScale, &RefEnt->modelScale);
	VectorCopy(&data->mOrigin, &RefEnt->origin);

	VectorScaleVector(&mins, &data->mScale, &mins);
	VectorScaleVector(&maxs, &data->mScale, &maxs);
	*radius = Distance(&mins, &maxs);

	AnglesToAxis( &data->mAngles, RefEnt->axis );
	ScaleModelAxis(RefEnt);
}

void CG_DrawMiscEnts(void)
{
	int			i;
	refEntity_t	*RefEnt;
	float		*radius, *zOff;
	vector3		difference;
	vector3		cullOrigin;
	refdef_t *refdef = CG_GetRefdef();

	RefEnt = MiscEnts;
	radius = Radius;
	zOff = zOffset;
	for(i=0;i<NumMiscEnts;i++)
	{
		VectorCopy(&RefEnt->origin, &cullOrigin);
		cullOrigin.z += 1.0f;

		if (*zOff)
		{
			cullOrigin.z += *zOff;
		}

		if (cg.snap && trap->R_InPVS(&refdef->vieworg, &cullOrigin, cg.snap->areamask))
		{
			VectorSubtract(&RefEnt->origin, &refdef->vieworg, &difference);
			if (VectorLength(&difference)-(*radius) <= cg.distanceCull)
			{
				SE_R_AddRefEntityToScene(RefEnt, MAX_CLIENTS);
			}
		}
		RefEnt++;
		radius++;
		zOff++;
	}
}

/*
Ghoul2 Insert Start
*/
/*
void CG_ResizeG2Bolt(boltInfo_v *bolt, int newCount)
{
	bolt->resize(newCount);
}

void CG_ResizeG2Surface(surfaceInfo_v *surface, int newCount)
{
	surface->resize(newCount);
}

void CG_ResizeG2Bone(boneInfo_v *bone, int newCount)
{
	bone->resize(newCount);
}

void CG_ResizeG2(CGhoul2Info_v *ghoul2, int newCount)
{
	ghoul2->resize(newCount);
}

void CG_ResizeG2TempBone(mdxaBone_v *tempBone, int newCount)
{
	tempBone->resize(newCount);
}
*/
/*
Ghoul2 Insert End
*/
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
	int i;

	for ( i=0; i<MAX_CLIENTS; i++ ) {
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !VALIDSTRING( clientInfo ) )
			continue;

		CG_NewClientInfo( i, qtrue );
	}
}

static void CVU_TeamOverlay( void ) {
	// If team overlay is on, ask for updates from the server.  If its off,
	// let the server know so we don't receive it
	if ( cg_drawTeamOverlay.integer > 0 && cgs.gametype >= GT_SINGLE_PLAYER)
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
		v->pitch	= 0.005f;
		v->yaw		= 0.01f;
		v->roll		= 0.005f;
	}
}

static void CVU_GunDrift( void ) {
	vector3 *v = &cg.gunIdleDrift.amount;
	if ( sscanf( cg_gunIdleDrift.string, "%f %f %f %f", &v->pitch, &v->yaw, &v->roll, &cg.gunIdleDrift.speed ) != 4 ) {
		v->pitch				= 0.01f;
		v->yaw					= 0.01f;
		v->roll					= 0.01f;
		cg.gunIdleDrift.speed	= 0.001f;
	}
}

static void CVU_LagPos( void ) {
	ivector2 *v = &cg.lagometerPos;
	if ( sscanf( cg_hudLagometerPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 48;
		v->y = 160;
	}
}

static void CVU_MoveKeysPos( void ) {
	ivector2 *v = &cg.moveKeysPos;
	if ( sscanf( cg_hudMovementKeysPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 320;
		v->y = 240;
	}
}

static void CVU_StatsPos( void ) {
	ivector2 *v = &cg.statsPos;
	if ( sscanf( cg_hudStatsPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 2;
		v->y = 240;
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
		cg.viewBob.pitch	= 0.002f;
		cg.viewBob.roll		= 0.002f;
		cg.viewBob.up		= 0.005f;
		cg.viewBob.fall		= qtrue;
	}
	else
		cg.viewBob.fall = tmp;
}

static void CVU_AutomapAngle( void ) {
	vector3 *v = &cg.automapAngle;
	if ( sscanf( r_autoMapAngle.string, "%f %f %f", &v->pitch, &v->yaw, &v->roll ) != 3 ) {
		v->pitch	= 90.0f;
		v->yaw		= 0.0f;
		v->roll		= 0.0f;
	}
}

static void CVU_AccelPos( void ) {
	ivector2 *v = &cg.accelerometer.position;
	if ( sscanf( cg_accelerometerPos.string, "%i %i", &v->x, &v->y ) != 2 ) {
		v->x = 320;
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

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
} cvarTable_t;

#define XCVAR_DECL
	#include "cg_xcvar.h"
#undef XCVAR_DECL

static cvarTable_t cvarTable[] = {
	#define XCVAR_LIST
		#include "cg_xcvar.h"
	#undef XCVAR_LIST
};

static int cvarTableSize = ARRAY_LEN( cvarTable );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		trap->Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i=0, cv=cvarTable; i<cvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap->Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount > modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS)
	{
		return -1;
	}

	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
//	Com_Printf( "%i %i\n", cg.snap->ps.persistant[PERS_ATTACKER], cg.attackerTime );
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cmd_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

const char *CG_Cvar_VariableString( const char *var_name ) {
	static char	buffer[MAX_STRING_CHARS];

	trap->Cvar_VariableStringBuffer( var_name, buffer, sizeof( buffer ) );

	return buffer;
}

//========================================================================

//so shared code can get the local time depending on the side it's executed on
int BG_GetTime(void)
{
	return cg.time;
}

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap->S_RegisterSound( item->pickup_sound );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			trap->Error( ERR_DROP, "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		trap->S_RegisterSound( data );
	}

	// parse the space seperated precache string for other media
	s = item->precaches;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			trap->Error( ERR_DROP, "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "efx" )) {
			trap->FX_RegisterEffect( data );
		}
	}
}

static void CG_AS_Register(void)
{
	const char *soundName;
	int i;

//	CG_LoadingString( "ambient sound sets" );

	//Load the ambient sets
#if 0 //as_preCacheMap was game-side.. that is evil.
	trap->AS_AddPrecacheEntry( "#clear" );	// ;-)
	//FIXME: Don't ask... I had to get around a really nasty MS error in the templates with this...
	namePrecache_m::iterator	pi;
	STL_ITERATE( pi, as_preCacheMap )
	{
		cgi_AS_AddPrecacheEntry( ((*pi).first).c_str() );
	}
#else
	trap->AS_AddPrecacheEntry( "#clear" );

	for ( i = 1 ; i < MAX_AMBIENT_SETS ; i++ ) {
		soundName = CG_ConfigString( CS_AMBIENT_SET+i );
		if ( !soundName || !soundName[0] )
		{
			break;
		}

		trap->AS_AddPrecacheEntry(soundName);
	}
	soundName = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );
	if (soundName && soundName[0] && Q_stricmp(soundName, "default"))
	{ //global soundset
		trap->AS_AddPrecacheEntry(soundName);
	}
#endif

	trap->AS_ParseSets();
}

//a global weather effect (rain, snow, etc)
void CG_ParseWeatherEffect(const char *str)
{
	char *sptr = (char *)str;
	sptr++; //pass the '*'
	trap->R_WorldEffectCommand(sptr);
}

extern int cgSiegeRoundBeganTime;
void CG_ParseSiegeState(const char *str)
{
	int i = 0;
	int j = 0;
//	int prevState = cgSiegeRoundState;
	char b[1024];

	while (str[i] && str[i] != '|')
	{
		b[j] = str[i];
		i++;
		j++;
	}
	b[j] = 0;
	cgSiegeRoundState = atoi(b);

	if (str[i] == '|')
	{
		j = 0;
		i++;
		while (str[i])
		{
			b[j] = str[i];
			i++;
			j++;
		}
		b[j] = 0;
//		if (cgSiegeRoundState != prevState)
		{ //it changed
			cgSiegeRoundTime = atoi(b);
			if (cgSiegeRoundState == 0 || cgSiegeRoundState == 2)
			{
				cgSiegeRoundBeganTime = cgSiegeRoundTime;
			}
		}
	}
	else
	{
		cgSiegeRoundTime = cg.time;
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
void CG_PrecacheNPCSounds(const char *str);
void CG_ParseSiegeObjectiveStatus(const char *str);
extern int cg_beatingSiegeTime;
extern int cg_siegeWinTeam;
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	const char	*soundName;

	CG_AS_Register();

//	CG_LoadingString( "sounds" );

	trap->S_RegisterSound( "sound/weapons/melee/punch1.mp3" );
	trap->S_RegisterSound( "sound/weapons/melee/punch2.mp3" );
	trap->S_RegisterSound( "sound/weapons/melee/punch3.mp3" );
	trap->S_RegisterSound( "sound/weapons/melee/punch4.mp3" );
	trap->S_RegisterSound("sound/movers/objects/saber_slam");

	trap->S_RegisterSound("sound/player/bodyfall_human1.wav");
	trap->S_RegisterSound("sound/player/bodyfall_human2.wav");
	trap->S_RegisterSound("sound/player/bodyfall_human3.wav");

	//test effects
	trap->FX_RegisterEffect("effects/mp/test_sparks.efx");
	trap->FX_RegisterEffect("effects/mp/test_wall_impact.efx");

	cgs.media.oneMinuteSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM004" );
	cgs.media.fiveMinuteSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM005" );
	cgs.media.oneFragSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM001" );
	cgs.media.twoFragSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM002" );
	cgs.media.threeFragSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM003");
	cgs.media.count3Sound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM035" );
	cgs.media.count2Sound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM036" );
	cgs.media.count1Sound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM037" );
	cgs.media.countFightSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM038" );

	cgs.media.hackerIconShader			= trap->R_RegisterShaderNoMip("gfx/mp/c_icon_tech");

	cgs.media.redSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_glow" );
	cgs.media.redSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_line" );
	cgs.media.orangeSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_glow" );
	cgs.media.orangeSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_line" );
	cgs.media.yellowSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_glow" );
	cgs.media.yellowSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_line" );
	cgs.media.greenSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_glow" );
	cgs.media.greenSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_line" );
	cgs.media.blueSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_glow" );
	cgs.media.blueSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_line" );
	cgs.media.purpleSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_glow" );
	cgs.media.purpleSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_line" );
	//[RGBSabers]
	cgs.media.rgbSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow1" );
	cgs.media.rgbSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore1" );

	//Flame 1
	cgs.media.rgbSaberGlow2Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow2" );
	cgs.media.rgbSaberCore2Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore2" );
	cgs.media.rgbSaberTrail2Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail2" );

	//Electric 1
	cgs.media.rgbSaberGlow3Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow3" );
	cgs.media.rgbSaberCore3Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore3" );
	cgs.media.rgbSaberTrail3Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail3" );

	//Flame 2
	cgs.media.rgbSaberGlow4Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow4" );
	cgs.media.rgbSaberCore4Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore4" );
	cgs.media.rgbSaberTrail4Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail4" );

	//Electric 2
	cgs.media.rgbSaberGlow5Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow5" );
	cgs.media.rgbSaberCore5Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore5" );
	cgs.media.rgbSaberTrail5Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/swordTrail" );

	//Black
	cgs.media.blackSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackglow" );
	cgs.media.blackSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackcore" );
	cgs.media.blackBlurShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blacktrail" );
	//[/RGBSabers]

	//[SFXSabers]
	cgs.media.sfxSaberTrailShader		= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_trail" );
	cgs.media.sfxSaberBladeShader		= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade" );
	cgs.media.sfxSaberBlade2Shader		= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade_rgb" );
	cgs.media.sfxSaberEndShader			= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end" );
	cgs.media.sfxSaberEnd2Shader		= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end_rgb" );
	//[/SFXSabers]

	//[Movie Sabers]
#if 0
	//Original Trilogy Sabers
	cgs.media.otSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_saberCore" );
	cgs.media.redOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_redGlow" );
	cgs.media.orangeOTGlowShader		= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_orangeGlow" );
	cgs.media.yellowOTGlowShader		= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_yellowGlow" );
	cgs.media.greenOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_greenGlow" );
	cgs.media.blueOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_blueGlow" );
	cgs.media.purpleOTGlowShader		= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_purpleGlow" );

	//Episode I Sabers
	cgs.media.ep1SaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/saber_core" );
	cgs.media.redEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/red_glowa" );
	cgs.media.orangeEp1GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/orange_glowa" );
	cgs.media.yellowEp1GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/yellow_glowa" );
	cgs.media.greenEp1GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/green_glowa" );
	cgs.media.blueEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/blue_glowa" );
	cgs.media.purpleEp1GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/purple_glowa" );

	//Episode II Sabers
	cgs.media.ep2SaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/saber_core" );
	cgs.media.whiteIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/white_ignite_flare" );
	cgs.media.blackIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/black_ignite_flare" );
	cgs.media.redEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/red_glowa" );
	cgs.media.orangeEp2GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/orange_glowa" );
	cgs.media.yellowEp2GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/yellow_glowa" );
	cgs.media.greenEp2GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/green_glowa" );
	cgs.media.blueEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/blue_glowa" );
	cgs.media.purpleEp2GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/purple_glowa" );

	//Episode III Sabers
	cgs.media.ep3SaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/saber_core" );
	cgs.media.whiteIgniteFlare02		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/white_ignite_flare" );
	cgs.media.blackIgniteFlare02		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/black_ignite_flare" );
	cgs.media.redIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/red_ignite_flare" );
	cgs.media.greenIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/green_ignite_flare" );
	cgs.media.purpleIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/purple_ignite_flare" );
	cgs.media.blueIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/blue_ignite_flare" );
	cgs.media.orangeIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/orange_ignite_flare" );
	cgs.media.yellowIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/yellow_ignite_flare" );
	cgs.media.redEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/red_glowa" );
	cgs.media.orangeEp3GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/orange_glowa" );
	cgs.media.yellowEp3GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/yellow_glowa" );
	cgs.media.greenEp3GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/green_glowa" );
	cgs.media.blueEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/blue_glowa" );
	cgs.media.purpleEp3GlowShader		= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/purple_glowa" );
#endif
	//[/Movie Sabers]


	cgs.media.saberBlurShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/saberBlur" );
	cgs.media.swordTrailShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/swordTrail" );

	cgs.media.forceCoronaShader			= trap->R_RegisterShaderNoMip( "gfx/hud/force_swirl" );

	cgs.media.yellowDroppedSaberShader	= trap->R_RegisterShaderNoMip("gfx/effects/yellow_glow");

	cgs.media.rivetMarkShader			= trap->R_RegisterShaderNoMip( "gfx/damage/rivetmark" );

	trap->R_RegisterShader( "gfx/effects/saberFlare" );

	trap->R_RegisterShader( "powerups/ysalimarishell" );
	
	trap->R_RegisterShaderNoMip( "gfx/effects/forcePush" );

	trap->R_RegisterShader( "gfx/misc/red_dmgshield" );
	trap->R_RegisterShader( "gfx/misc/red_portashield" );
	trap->R_RegisterShader( "gfx/misc/blue_dmgshield" );
	trap->R_RegisterShader( "gfx/misc/blue_portashield" );

	trap->R_RegisterShader( "models/map_objects/imp_mine/turret_chair_dmg.tga" );

	for ( i=1; i<9; i++ )
		trap->S_RegisterSound( va( "sound/weapons/saber/saberhup%i.wav", i ) );

	for ( i=1; i<10; i++ )
		trap->S_RegisterSound( va( "sound/weapons/saber/saberblock%i.wav", i ) );

	for ( i=1; i<4; i++ )
		trap->S_RegisterSound( va( "sound/weapons/saber/bounce%i.wav", i ) );

	trap->S_RegisterSound( "sound/weapons/saber/enemy_saber_on.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/enemy_saber_off.wav" );

	trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhitwall1" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhitwall2" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhitwall3" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhit.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhit1.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhit2.wav" );
	trap->S_RegisterSound( "sound/weapons/saber/saberhit3.wav" );

	trap->S_RegisterSound( "sound/weapons/saber/saber_catch.wav" );

	cgs.media.teamHealSound		= trap->S_RegisterSound( "sound/weapons/force/teamheal.wav" );
	cgs.media.teamRegenSound	= trap->S_RegisterSound( "sound/weapons/force/teamforce.wav" );

	trap->S_RegisterSound( "sound/weapons/force/heal.wav" );
	trap->S_RegisterSound( "sound/weapons/force/speed.wav" );
	trap->S_RegisterSound( "sound/weapons/force/see.wav" );
	trap->S_RegisterSound( "sound/weapons/force/rage.wav" );
	trap->S_RegisterSound( "sound/weapons/force/lightning" );
	trap->S_RegisterSound( "sound/weapons/force/lightninghit1" );
	trap->S_RegisterSound( "sound/weapons/force/lightninghit2" );
	trap->S_RegisterSound( "sound/weapons/force/lightninghit3" );
	trap->S_RegisterSound( "sound/weapons/force/drain.wav" );
	trap->S_RegisterSound( "sound/weapons/force/jumpbuild.wav" );
	trap->S_RegisterSound( "sound/weapons/force/distract.wav" );
	trap->S_RegisterSound( "sound/weapons/force/distractstop.wav" );

	trap->S_RegisterSound( "sound/weapons/force/pull.wav" );
	trap->S_RegisterSound( "sound/weapons/force/push.wav" );

	for ( i=1; i<3; i++ )
		trap->S_RegisterSound( va( "sound/weapons/thermal/bounce%i.wav", i ) );

	trap->S_RegisterSound( "sound/movers/switches/switch2.wav" );
	trap->S_RegisterSound( "sound/movers/switches/switch3.wav" );
	trap->S_RegisterSound( "sound/ambience/spark5.wav" );
	trap->S_RegisterSound( "sound/chars/turret/ping.wav" );
	trap->S_RegisterSound( "sound/chars/turret/startup.wav" );
	trap->S_RegisterSound( "sound/chars/turret/shutdown.wav" );
	trap->S_RegisterSound( "sound/chars/turret/move.wav" );
	trap->S_RegisterSound( "sound/player/pickuphealth.wav" );
	trap->S_RegisterSound( "sound/player/pickupshield.wav" );

	trap->S_RegisterSound("sound/effects/glassbreak1.wav");

	trap->S_RegisterSound( "sound/weapons/rocket/tick.wav" );
	trap->S_RegisterSound( "sound/weapons/rocket/lock.wav" );

	trap->S_RegisterSound( "sound/weapons/force/speedloop.wav" );

	trap->S_RegisterSound( "sound/weapons/force/protecthit.mp3" ); //PDSOUND_PROTECTHIT
	trap->S_RegisterSound( "sound/weapons/force/protect.mp3" ); //PDSOUND_PROTECT
	trap->S_RegisterSound( "sound/weapons/force/absorbhit.mp3" ); //PDSOUND_ABSORBHIT
	trap->S_RegisterSound( "sound/weapons/force/absorb.mp3" ); //PDSOUND_ABSORB
	trap->S_RegisterSound( "sound/weapons/force/jump.mp3" ); //PDSOUND_FORCEJUMP
	trap->S_RegisterSound( "sound/weapons/force/grip.mp3" ); //PDSOUND_FORCEGRIP

	if ( cgs.gametype >= GT_TEAM ) {

#ifdef JK2AWARDS
		cgs.media.captureAwardSound = trap->S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav" );
#endif
		cgs.media.redLeadsSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM046");
		cgs.media.blueLeadsSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM045");
		cgs.media.teamsTiedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM032" );

		cgs.media.redScoredSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM044");
		cgs.media.blueScoredSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM043" );

		if ( cgs.gametype == GT_CTF ) {
			cgs.media.redFlagReturnedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM042" );
			cgs.media.blueFlagReturnedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM041" );
			cgs.media.redTookFlagSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM040" );
			cgs.media.blueTookFlagSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM039" );
		}

		if ( cgs.gametype == GT_CTY ) {
			cgs.media.redYsalReturnedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM050" );
			cgs.media.blueYsalReturnedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM049" );
			cgs.media.redTookYsalSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM048" );
			cgs.media.blueTookYsalSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM047" );
		}
	}

	cgs.media.drainSound = trap->S_RegisterSound("sound/weapons/force/drained.mp3");

	cgs.media.happyMusic = trap->S_RegisterSound("music/goodsmall.mp3");
	cgs.media.dramaticFailure = trap->S_RegisterSound("music/badsmall.mp3");

	//PRECACHE ALL MUSIC HERE (don't need to precache normally because it's streamed off the disk)
	cg.loadLCARSStage = 1;

	cgs.media.selectSound = trap->S_RegisterSound( "sound/weapons/change.wav" );

	cgs.media.teleInSound = trap->S_RegisterSound( "sound/player/telein.wav" );
	cgs.media.teleOutSound = trap->S_RegisterSound( "sound/player/teleout.wav" );
	cgs.media.respawnSound = trap->S_RegisterSound( "sound/items/respawn1.wav" );

	trap->S_RegisterSound( "sound/movers/objects/objectHit.wav" );

	cgs.media.talkSound = trap->S_RegisterSound( "sound/player/talk.wav" );
	cgs.media.landSound = trap->S_RegisterSound( "sound/player/land1.wav");
	cgs.media.fallSound = trap->S_RegisterSound( "sound/player/fallsplat.wav");

	cgs.media.crackleSound = trap->S_RegisterSound( "sound/effects/energy_crackle.wav" );
#ifdef JK2AWARDS
	cgs.media.impressiveSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM025" );
	cgs.media.excellentSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM053" );
	cgs.media.deniedSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM017" );
	cgs.media.humiliationSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM019" );
	cgs.media.defendSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM024" );
	cgs.media.holyShitSound = trap->S_RegisterSound( "sound/chars/protocol/misc/holyshit" );
#endif

	/*
	cgs.media.takenLeadSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM051");
	cgs.media.tiedLeadSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM032");
	cgs.media.lostLeadSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM052");
	*/

	cgs.media.rollSound					= trap->S_RegisterSound( "sound/player/roll1.wav");

	cgs.media.noforceSound				= trap->S_RegisterSound( "sound/weapons/force/noforce" );

	cgs.media.watrInSound				= trap->S_RegisterSound( "sound/player/watr_in.wav");
	cgs.media.watrOutSound				= trap->S_RegisterSound( "sound/player/watr_out.wav");
	cgs.media.watrUnSound				= trap->S_RegisterSound( "sound/player/watr_un.wav");

	cgs.media.explosionModel			= trap->R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	cgs.media.surfaceExplosionShader	= trap->R_RegisterShader( "surfaceExplosion" );

	cgs.media.disruptorShader			= trap->R_RegisterShader( "gfx/effects/burn");

	cgs.media.solidWhite = trap->R_RegisterShader( "gfx/effects/solidWhite_cull" );

	trap->R_RegisterShader("gfx/misc/mp_light_enlight_disable");
	trap->R_RegisterShader("gfx/misc/mp_dark_enlight_disable");

	trap->R_RegisterModel ( "models/map_objects/mp/sphere.md3" );
	trap->R_RegisterModel("models/items/remote.md3");

	cgs.media.holocronPickup = trap->S_RegisterSound( "sound/player/holocron.wav" );

	// Zoom
	cgs.media.zoomStart = trap->S_RegisterSound( "sound/interface/zoomstart.wav" );
	cgs.media.zoomLoop	= trap->S_RegisterSound( "sound/interface/zoomloop.wav" );
	cgs.media.zoomEnd	= trap->S_RegisterSound( "sound/interface/zoomend.wav" );

	for ( i=0; i<4; i++ )
	{
		cgs.media.footsteps[FOOTSTEP_STONEWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/stone_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_STONERUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/stone_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_METALWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/metal_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_METALRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/metal_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_PIPEWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/pipe_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_PIPERUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/pipe_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SPLASH][i]		= trap->S_RegisterSound( va( "sound/player/footsteps/water_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_WADE][i]		= trap->S_RegisterSound( va( "sound/player/footsteps/water_walk%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SWIM][i]		= trap->S_RegisterSound( va( "sound/player/footsteps/water_wade_0%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SNOWWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/snow_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SNOWRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/snow_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SANDWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/sand_walk%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_SANDRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/sand_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_GRASSWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/grass_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_GRASSRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/grass_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_DIRTWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/dirt_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_DIRTRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/dirt_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_MUDWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/mud_walk%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_MUDRUN][i]		= trap->S_RegisterSound( va( "sound/player/footsteps/mud_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_GRAVELWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/gravel_walk%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_GRAVELRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/gravel_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_RUGWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/rug_step%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_RUGRUN][i]		= trap->S_RegisterSound( va( "sound/player/footsteps/rug_run%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_WOODWALK][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/wood_walk%i.wav", i+1 ) );
		cgs.media.footsteps[FOOTSTEP_WOODRUN][i]	= trap->S_RegisterSound( va( "sound/player/footsteps/wood_run%i.wav", i+1 ) );
	}

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i=1; i<bg_numItems; i++ ) {
		if ( items[i] == '1' )
			CG_RegisterItemSounds( i );
	}

	for ( i=1; i<MAX_SOUNDS; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );

		if ( !soundName[0] )
			break;

		if ( soundName[0] == '*' )
		{
			//an NPC soundset
			if ( soundName[1] == '$' )
				CG_PrecacheNPCSounds( soundName );

			continue; // custom sound
		}
		cgs.gameSounds[i] = trap->S_RegisterSound( soundName );
	}

	for ( i=1; i<MAX_FX; i++ ) {
		soundName = CG_ConfigString( CS_EFFECTS+i );
		if ( !soundName[0] )
			break;

		if ( soundName[0] == '*' )
		{// it's a special global weather effect
			CG_ParseWeatherEffect( soundName );
			cgs.gameEffects[i] = 0;
		}
		else
			cgs.gameEffects[i] = trap->FX_RegisterEffect( soundName );
	}

	// register all the server specified icons
	for ( i=1; i<MAX_ICONS; i++ )
	{
		const char *iconName = NULL;

		iconName = CG_ConfigString( CS_ICONS + i );
		if ( !iconName[0] )
			break;

		cgs.gameIcons[i] = trap->R_RegisterShaderNoMip( iconName );
	}

	soundName = CG_ConfigString( CS_SIEGE_STATE );

	if ( soundName[0] )
		CG_ParseSiegeState( soundName );

	soundName = CG_ConfigString( CS_SIEGE_WINTEAM );

	if ( soundName[0] )
		cg_siegeWinTeam = atoi( soundName );

	if ( cgs.gametype == GT_SIEGE )
	{
		CG_ParseSiegeObjectiveStatus( CG_ConfigString( CS_SIEGE_OBJECTIVES ) );
		cg_beatingSiegeTime = atoi( CG_ConfigString( CS_SIEGE_TIMEOVERRIDE ) );
		if ( cg_beatingSiegeTime )
			CG_SetSiegeTimerCvar( cg_beatingSiegeTime );
	}

	cg.loadLCARSStage = 2;

	// FIXME: only needed with item
	cgs.media.deploySeeker = trap->S_RegisterSound ("sound/chars/seeker/misc/hiss");
	cgs.media.medkitSound = trap->S_RegisterSound ("sound/items/use_bacta.wav");
	
	cgs.media.winnerSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM006" );
	cgs.media.loserSound = trap->S_RegisterSound( "sound/chars/protocol/misc/40MOM010" );
}


//-------------------------------------
// CG_RegisterEffects
// 
// Handles precaching all effect files
//	and any shader, model, or sound
//	files an effect may use.
//-------------------------------------
static void CG_RegisterEffects( void )
{
	/*
	const char	*effectName;
	int			i;

	for ( i = 1 ; i < MAX_FX ; i++ ) 
	{
		effectName = CG_ConfigString( CS_EFFECTS + i );

		if ( !effectName[0] ) 
		{
			break;
		}

		trap->FX_RegisterEffect( effectName );
	}
	*/
	//the above was redundant as it's being done in CG_RegisterSounds

	// Set up the glass effects mini-system.
	CG_InitGlass();

	//footstep effects
	cgs.effects.footstepMud = trap->FX_RegisterEffect( "materials/mud" );
	cgs.effects.footstepSand = trap->FX_RegisterEffect( "materials/sand" );
	cgs.effects.footstepSnow = trap->FX_RegisterEffect( "materials/snow" );
	cgs.effects.footstepGravel = trap->FX_RegisterEffect( "materials/gravel" );
	//landing effects
	cgs.effects.landingMud = trap->FX_RegisterEffect( "materials/mud_large" );
	cgs.effects.landingSand = trap->FX_RegisterEffect( "materials/sand_large" );
	cgs.effects.landingDirt = trap->FX_RegisterEffect( "materials/dirt_large" );
	cgs.effects.landingSnow = trap->FX_RegisterEffect( "materials/snow_large" );
	cgs.effects.landingGravel = trap->FX_RegisterEffect( "materials/gravel_large" );
	//splashes
	cgs.effects.waterSplash = trap->FX_RegisterEffect( "env/water_impact" );
	cgs.effects.lavaSplash = trap->FX_RegisterEffect( "env/lava_splash" );
	cgs.effects.acidSplash = trap->FX_RegisterEffect( "env/acid_splash" );

	//Raz: Portals
	cgs.effects.portalBlue = trap->FX_RegisterEffect( "effects/env/portal1" );
	cgs.effects.portalOrange = trap->FX_RegisterEffect( "effects/env/portal2" );

	//Raz: JA+ flamethrower
	cgs.effects.flamethrower = trap->FX_RegisterEffect( "effects/bespin/flamejet" );
}

//===================================================================================

extern char *forceHolocronModels[];
int CG_HandleAppendedSkin(char *modelName);
void CG_CacheG2AnimInfo(char *modelName);
/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	int			breakPoint;
	char		items[MAX_ITEMS+1];
	const char	*terrainInfo;
	int			terrainID;
	refdef_t *refdef = CG_GetRefdef();

	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero",
		"gfx/2d/numbers/one",
		"gfx/2d/numbers/two",
		"gfx/2d/numbers/three",
		"gfx/2d/numbers/four",
		"gfx/2d/numbers/five",
		"gfx/2d/numbers/six",
		"gfx/2d/numbers/seven",
		"gfx/2d/numbers/eight",
		"gfx/2d/numbers/nine",
		"gfx/2d/numbers/minus",
	};

	static char		*sb_t_nums[11] = {
		"gfx/2d/numbers/t_zero",
		"gfx/2d/numbers/t_one",
		"gfx/2d/numbers/t_two",
		"gfx/2d/numbers/t_three",
		"gfx/2d/numbers/t_four",
		"gfx/2d/numbers/t_five",
		"gfx/2d/numbers/t_six",
		"gfx/2d/numbers/t_seven",
		"gfx/2d/numbers/t_eight",
		"gfx/2d/numbers/t_nine",
		"gfx/2d/numbers/t_minus",
	};

	static char		*sb_c_nums[11] = {
		"gfx/2d/numbers/c_zero",
		"gfx/2d/numbers/c_one",
		"gfx/2d/numbers/c_two",
		"gfx/2d/numbers/c_three",
		"gfx/2d/numbers/c_four",
		"gfx/2d/numbers/c_five",
		"gfx/2d/numbers/c_six",
		"gfx/2d/numbers/c_seven",
		"gfx/2d/numbers/c_eight",
		"gfx/2d/numbers/c_nine",
		"gfx/2d/numbers/t_minus", //?????
	};

	// clear any references to old media
	memset( &cg.refdef[0], 0, sizeof( cg.refdef ) );
	trap->R_ClearScene();

	CG_LoadingString( cgs.mapname );        

	trap->R_LoadWorld( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap->R_RegisterShaderNoMip( sb_nums[i] );
	}

	cg.loadLCARSStage = 3;

	for ( i=0; i < 11; i++ )
	{
		cgs.media.numberShaders[i]			= trap->R_RegisterShaderNoMip( sb_nums[i] );
		cgs.media.smallnumberShaders[i]		= trap->R_RegisterShaderNoMip( sb_t_nums[i] );
		cgs.media.chunkyNumberShaders[i]	= trap->R_RegisterShaderNoMip( sb_c_nums[i] );
	}

	trap->R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_lone" );
	trap->R_RegisterShaderNoMip ( "gfx/mp/pduel_icon_double" );

	cgs.media.balloonShader = trap->R_RegisterShaderNoMip( "gfx/mp/chat_icon" );
	cgs.media.vchatShader = trap->R_RegisterShaderNoMip( "gfx/mp/vchat_icon" );

	cgs.media.deferShader = trap->R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.radarShader			= trap->R_RegisterShaderNoMip ( "gfx/menus/radar/radar.png" );
	cgs.media.siegeItemShader		= trap->R_RegisterShaderNoMip ( "gfx/menus/radar/goalitem" );
	cgs.media.mAutomapPlayerIcon	= trap->R_RegisterShader( "gfx/menus/radar/arrow_w" );
	cgs.media.mAutomapRocketIcon	= trap->R_RegisterShader( "gfx/menus/radar/rocket" );

	cgs.media.wireframeAutomapFrame_left = trap->R_RegisterShader( "gfx/mp_automap/mpauto_frame_left" );
	cgs.media.wireframeAutomapFrame_right = trap->R_RegisterShader( "gfx/mp_automap/mpauto_frame_right" );
	cgs.media.wireframeAutomapFrame_top = trap->R_RegisterShader( "gfx/mp_automap/mpauto_frame_top" );
	cgs.media.wireframeAutomapFrame_bottom = trap->R_RegisterShader( "gfx/mp_automap/mpauto_frame_bottom" );

	cgs.media.lagometerShader = trap->R_RegisterShaderNoMip("gfx/2d/lag" );
	cgs.media.connectionShader = trap->R_RegisterShaderNoMip( "gfx/2d/net" );

	cgs.media.japp.scoreboardLine	= trap->R_RegisterShaderNoMip( "gfx/menus/scoreboard" );

	trap->FX_InitSystem(refdef);
	CG_RegisterEffects();

	cgs.media.boltShader = trap->R_RegisterShader( "gfx/misc/blueLine" );

	cgs.effects.turretShotEffect = trap->FX_RegisterEffect( "turret/shot" );
	cgs.effects.mEmplacedDeadSmoke = trap->FX_RegisterEffect("emplaced/dead_smoke.efx");
	cgs.effects.mEmplacedExplode = trap->FX_RegisterEffect("emplaced/explode.efx");
	cgs.effects.mTurretExplode = trap->FX_RegisterEffect("turret/explode.efx");
	cgs.effects.mSparkExplosion = trap->FX_RegisterEffect("sparks/spark_explosion.efx");
	cgs.effects.mTripmineExplosion = trap->FX_RegisterEffect("tripMine/explosion.efx");
	cgs.effects.mDetpackExplosion = trap->FX_RegisterEffect("detpack/explosion.efx");
	cgs.effects.mFlechetteAltBlow = trap->FX_RegisterEffect("flechette/alt_blow.efx");
	cgs.effects.mStunBatonFleshImpact = trap->FX_RegisterEffect("stunBaton/flesh_impact.efx");
	cgs.effects.mAltDetonate = trap->FX_RegisterEffect("demp2/altDetonate.efx");
	cgs.effects.mSparksExplodeNoSound = trap->FX_RegisterEffect("sparks/spark_exp_nosnd");
	cgs.effects.mTripMineLaster = trap->FX_RegisterEffect("tripMine/laser.efx");
	cgs.effects.mEmplacedMuzzleFlash = trap->FX_RegisterEffect( "effects/emplaced/muzzle_flash" );
	cgs.effects.mConcussionAltRing = trap->FX_RegisterEffect("concussion/alt_ring");

	cgs.effects.mHyperspaceStars = trap->FX_RegisterEffect("ships/hyperspace_stars");
	cgs.effects.mBlackSmoke = trap->FX_RegisterEffect( "volumetric/black_smoke" );
	cgs.effects.mShipDestDestroyed = trap->FX_RegisterEffect("effects/ships/dest_destroyed.efx");
	cgs.effects.mShipDestBurning = trap->FX_RegisterEffect("effects/ships/dest_burning.efx");
	cgs.effects.mBobaJet = trap->FX_RegisterEffect("effects/boba/jet.efx");


	cgs.effects.itemCone = trap->FX_RegisterEffect("mp/itemcone.efx");
	cgs.effects.mTurretMuzzleFlash = trap->FX_RegisterEffect("effects/turret/muzzle_flash.efx");
	cgs.effects.mSparks = trap->FX_RegisterEffect("sparks/spark_nosnd.efx"); //sparks/spark.efx
	cgs.effects.mSaberCut = trap->FX_RegisterEffect("saber/saber_cut.efx");
	cgs.effects.mSaberBlock = trap->FX_RegisterEffect("saber/saber_block.efx");
	cgs.effects.mSaberBloodSparks = trap->FX_RegisterEffect("saber/blood_sparks_mp.efx");
	cgs.effects.mSaberBloodSparksSmall = trap->FX_RegisterEffect("saber/blood_sparks_25_mp.efx");
	cgs.effects.mSaberBloodSparksMid = trap->FX_RegisterEffect("saber/blood_sparks_50_mp.efx");
	cgs.effects.mSpawn = trap->FX_RegisterEffect("mp/spawn.efx");
	cgs.effects.mJediSpawn = trap->FX_RegisterEffect("mp/jedispawn.efx");
	cgs.effects.mBlasterDeflect = trap->FX_RegisterEffect("blaster/deflect.efx");
	cgs.effects.mBlasterSmoke = trap->FX_RegisterEffect("blaster/smoke_bolton");
	cgs.effects.mForceConfustionOld = trap->FX_RegisterEffect("force/confusion_old.efx");

	cgs.effects.forceLightning		= trap->FX_RegisterEffect( "effects/force/lightning.efx" );
	cgs.effects.forceLightningWide	= trap->FX_RegisterEffect( "effects/force/lightningwide.efx" );
	cgs.effects.forceDrain		= trap->FX_RegisterEffect( "effects/mp/drain.efx" );
	cgs.effects.forceDrainWide	= trap->FX_RegisterEffect( "effects/mp/drainwide.efx" );
	cgs.effects.forceDrained	= trap->FX_RegisterEffect( "effects/mp/drainhit.efx");

	cgs.effects.mDisruptorDeathSmoke = trap->FX_RegisterEffect("disruptor/death_smoke");

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap->R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cg.loadLCARSStage = 4;

	cgs.media.backTileShader = trap->R_RegisterShader( "gfx/2d/backtile" );

	//precache the fpls skin
	//trap->R_RegisterSkin("models/players/kyle/model_fpls2.skin");

	cgs.media.itemRespawningPlaceholder = trap->R_RegisterShader("powerups/placeholder");
	cgs.media.itemRespawningRezOut = trap->R_RegisterShader("powerups/rezout");

	cgs.media.playerShieldDamage = trap->R_RegisterShader("gfx/misc/personalshield");
	cgs.media.protectShader = trap->R_RegisterShader("gfx/misc/forceprotect");
	cgs.media.forceSightBubble = trap->R_RegisterShader("gfx/misc/sightbubble");
	cgs.media.forceShell = trap->R_RegisterShader("powerups/forceshell");
	cgs.media.sightShell = trap->R_RegisterShader("powerups/sightshell");

	cgs.media.itemHoloModel = trap->R_RegisterModel("models/map_objects/mp/holo.md3");

	if ( cgs.gametype == GT_HOLOCRON )
	{
		for ( i=0; i<NUM_FORCE_POWERS; i++ )
		{
			if ( forceHolocronModels[i] && forceHolocronModels[i][0] )
				trap->R_RegisterModel( forceHolocronModels[i] );
		}
	}

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY ) {
		if (cgs.gametype == GT_CTF)
		{
			cgs.media.redFlagModel = trap->R_RegisterModel( "models/flags/r_flag.md3" );
			cgs.media.blueFlagModel = trap->R_RegisterModel( "models/flags/b_flag.md3" );
		}
		else if ( cgs.gametype == GT_CTY )
		{
			cgs.media.redFlagModel = trap->R_RegisterModel( "models/flags/r_flag_ysal.md3" );
			cgs.media.blueFlagModel = trap->R_RegisterModel( "models/flags/b_flag_ysal.md3" );
		}

		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_x" );
		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_x" );

		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );

		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag" );
		trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag" );

		trap->R_RegisterShaderNoMip("gfx/2d/net.tga");
	}

	if ( cgs.gametype >= GT_TEAM ) {
		cgs.media.teamRedShader = trap->R_RegisterShader( "sprites/team_red" );
		cgs.media.teamBlueShader = trap->R_RegisterShader( "sprites/team_blue" );
		//cgs.media.redQuadShader = trap->R_RegisterShader("powerups/blueflag" );
		cgs.media.teamStatusBar = trap->R_RegisterShader( "gfx/2d/colorbar.tga" );
	}
	else if ( cgs.gametype == GT_JEDIMASTER )
	{
		cgs.media.teamRedShader = trap->R_RegisterShader( "sprites/team_red" );
	}

	if (cgs.gametype == GT_POWERDUEL)
	{
		cgs.media.powerDuelAllyShader = trap->R_RegisterShader("gfx/mp/pduel_icon_double");//trap->R_RegisterShader("gfx/mp/pduel_gameicon_ally");
	}

	cgs.media.heartShader			= trap->R_RegisterShaderNoMip( "ui/assets/statusbar/selectedhealth.tga" );

	cgs.media.ysaliredShader		= trap->R_RegisterShader( "powerups/ysaliredshell");
	cgs.media.ysaliblueShader		= trap->R_RegisterShader( "powerups/ysaliblueshell");
	cgs.media.ysalimariShader		= trap->R_RegisterShader( "powerups/ysalimarishell");
	cgs.media.boonShader			= trap->R_RegisterShader( "powerups/boonshell");
	cgs.media.endarkenmentShader	= trap->R_RegisterShader( "powerups/endarkenmentshell");
	cgs.media.enlightenmentShader	= trap->R_RegisterShader( "powerups/enlightenmentshell");
	cgs.media.invulnerabilityShader = trap->R_RegisterShader( "powerups/invulnerabilityshell");

#ifdef JK2AWARDS
	cgs.media.medalImpressive		= trap->R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent		= trap->R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet			= trap->R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend			= trap->R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist			= trap->R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture			= trap->R_RegisterShaderNoMip( "medal_capture" );
#endif

	// Binocular interface
	cgs.media.binocularCircle		= trap->R_RegisterShader( "gfx/2d/binCircle" );
	cgs.media.binocularMask			= trap->R_RegisterShader( "gfx/2d/binMask" );
	cgs.media.binocularArrow		= trap->R_RegisterShader( "gfx/2d/binSideArrow" );
	cgs.media.binocularTri			= trap->R_RegisterShader( "gfx/2d/binTopTri" );
	cgs.media.binocularStatic		= trap->R_RegisterShader( "gfx/2d/binocularWindow" );
	cgs.media.binocularOverlay		= trap->R_RegisterShader( "gfx/2d/binocularNumOverlay" );

	cg.loadLCARSStage = 5;

	// Chunk models
	//FIXME: jfm:? bother to conditionally load these if an ent has this material type?
	for ( i = 0; i < NUM_CHUNK_MODELS; i++ )
	{
		cgs.media.chunkModels[CHUNK_METAL2][i]	= trap->R_RegisterModel( va( "models/chunks/metal/metal1_%i.md3", i+1 ) ); //_ /switched\ _
		cgs.media.chunkModels[CHUNK_METAL1][i]	= trap->R_RegisterModel( va( "models/chunks/metal/metal2_%i.md3", i+1 ) ); //  \switched/
		cgs.media.chunkModels[CHUNK_ROCK1][i]	= trap->R_RegisterModel( va( "models/chunks/rock/rock1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK2][i]	= trap->R_RegisterModel( va( "models/chunks/rock/rock2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_ROCK3][i]	= trap->R_RegisterModel( va( "models/chunks/rock/rock3_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE1][i]	= trap->R_RegisterModel( va( "models/chunks/crate/crate1_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_CRATE2][i]	= trap->R_RegisterModel( va( "models/chunks/crate/crate2_%i.md3", i+1 ) );
		cgs.media.chunkModels[CHUNK_WHITE_METAL][i]	= trap->R_RegisterModel( va( "models/chunks/metal/wmetal1_%i.md3", i+1 ) );
	}

	cgs.media.chunkSound			= trap->S_RegisterSound("sound/weapons/explosions/glasslcar");
	cgs.media.grateSound			= trap->S_RegisterSound( "sound/effects/grate_destroy" );
	cgs.media.rockBreakSound		= trap->S_RegisterSound("sound/effects/wall_smash");
	cgs.media.rockBounceSound[0]	= trap->S_RegisterSound("sound/effects/stone_bounce");
	cgs.media.rockBounceSound[1]	= trap->S_RegisterSound("sound/effects/stone_bounce2");
	cgs.media.metalBounceSound[0]	= trap->S_RegisterSound("sound/effects/metal_bounce");
	cgs.media.metalBounceSound[1]	= trap->S_RegisterSound("sound/effects/metal_bounce2");
	cgs.media.glassChunkSound		= trap->S_RegisterSound("sound/weapons/explosions/glassbreak1");
	cgs.media.crateBreakSound[0]	= trap->S_RegisterSound("sound/weapons/explosions/crateBust1" );
	cgs.media.crateBreakSound[1]	= trap->S_RegisterSound("sound/weapons/explosions/crateBust2" );

/*
Ghoul2 Insert Start
*/
	CG_InitItems();
/*
Ghoul2 Insert End
*/
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	Q_strncpyz(items, CG_ConfigString(CS_ITEMS), sizeof(items));

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( items[i] == '1' ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	cg.loadLCARSStage = 6;

	cgs.media.glassShardShader	= trap->R_RegisterShader( "gfx/misc/test_crackle" );

	// doing one shader just makes it look like a shell.  By using two shaders with different bulge offsets and different texture scales, it has a much more chaotic look
	cgs.media.electricBodyShader			= trap->R_RegisterShader( "gfx/misc/electric" );
	cgs.media.electricBody2Shader			= trap->R_RegisterShader( "gfx/misc/fullbodyelectric2" );

	cgs.media.fsrMarkShader					= trap->R_RegisterShader( "footstep_r" );
	cgs.media.fslMarkShader					= trap->R_RegisterShader( "footstep_l" );
	cgs.media.fshrMarkShader				= trap->R_RegisterShader( "footstep_heavy_r" );
	cgs.media.fshlMarkShader				= trap->R_RegisterShader( "footstep_heavy_l" );

	cgs.media.refractionShader				= trap->R_RegisterShader("effects/refraction");

	cgs.media.cloakedShader					= trap->R_RegisterShader( "gfx/effects/cloakedShader" );

	// wall marks
	cgs.media.shadowMarkShader	= trap->R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader	= trap->R_RegisterShader( "wake" );

	cgs.media.viewPainShader					= trap->R_RegisterShader( "gfx/misc/borgeyeflare" );
	cgs.media.viewPainShader_Shields			= trap->R_RegisterShader( "gfx/mp/dmgshader_shields" );
	cgs.media.viewPainShader_ShieldsAndHealth	= trap->R_RegisterShader( "gfx/mp/dmgshader_shieldsandhealth" );

	// register the inline models
	breakPoint = cgs.numInlineModels = trap->CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vector3			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap->R_RegisterModel( name );
		if (!cgs.inlineDrawModel[i])
		{
			breakPoint = i;
			break;
		}

		trap->R_ModelBounds( cgs.inlineDrawModel[i], &mins, &maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i].data[j] = mins.data[j] + 0.5 * ( maxs.data[j] - mins.data[j] );
		}
	}

	cg.loadLCARSStage = 7;

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*cModelName;
		char modelName[MAX_QPATH];

		cModelName = CG_ConfigString( CS_MODELS+i );
		if ( !cModelName[0] ) {
			break;
		}

		strcpy(modelName, cModelName);
		if (strstr(modelName, ".glm") || modelName[0] == '$')
		{ //Check to see if it has a custom skin attached.
			CG_HandleAppendedSkin(modelName);
			CG_CacheG2AnimInfo(modelName);
		}

		if (modelName[0] != '$' && modelName[0] != '@')
		{ //don't register vehicle names and saber names as models.
			cgs.gameModels[i] = trap->R_RegisterModel( modelName );
		}
		else
		{//FIXME: register here so that stuff gets precached!!!
			cgs.gameModels[i] = 0;
		}
	}
	cg.loadLCARSStage = 8;
/*
Ghoul2 Insert Start
*/


	CG_LoadingString( "BSP instances" );

	for(i = 1; i < MAX_SUB_BSP; i++)
	{
		const char		*bspName = 0;
		vector3			mins, maxs;
		int				j;
		int				sub = 0;
		char			temp[MAX_QPATH];

		bspName = CG_ConfigString( CS_BSP_MODELS+i );
		if ( !bspName[0] ) 
		{
			break;
		}

		trap->CM_LoadMap( bspName, qtrue );
		cgs.inlineDrawModel[breakPoint] = trap->R_RegisterModel( bspName );
		trap->R_ModelBounds( cgs.inlineDrawModel[breakPoint], &mins, &maxs );
		for ( j = 0 ; j < 3 ; j++ ) 
		{
			cgs.inlineModelMidpoints[breakPoint].data[j] = mins.data[j] + 0.5 * ( maxs.data[j] - mins.data[j] );
		}
		breakPoint++;
		for(sub=1;sub<MAX_MODELS;sub++)
		{
			Com_sprintf(temp, MAX_QPATH, "*%d-%d", i, sub);
			cgs.inlineDrawModel[breakPoint] = trap->R_RegisterModel( temp );
			if (!cgs.inlineDrawModel[breakPoint])
			{
				break;
			}
			trap->R_ModelBounds( cgs.inlineDrawModel[breakPoint], &mins, &maxs );
			for ( j = 0 ; j < 3 ; j++ ) 
			{
				cgs.inlineModelMidpoints[breakPoint].data[j] = mins.data[j] + 0.5 * ( maxs.data[j] - mins.data[j] );
			}
			breakPoint++;
		}
	}

	CG_LoadingString( "Creating terrain" );
	for(i = 0; i < MAX_TERRAINS; i++)
	{
		terrainInfo = CG_ConfigString( CS_TERRAINS + i );
		if ( !terrainInfo[0] )
		{
			break;
		}

		terrainID = trap->CM_RegisterTerrain(terrainInfo);

		trap->RMG_Init(terrainID, terrainInfo);

		// Send off the terrainInfo to the renderer
		trap->RE_InitRendererTerrain( terrainInfo );
	}

	/*
	CG_LoadingString("skins");
	// register all the server specified models
	for (i=1 ; i<MAX_CHARSKINS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_CHARSKINS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.skins[i] = trap->R_RegisterSkin( modelName );
	}
	*/
	//rww - removed and replaced with CS_G2BONES. For custom skins
	//the new method is to append a * after an indexed model name and
	//then append the skin name after that (note that this is only
	//used for NPCs)

	CG_LoadingString("weapons");

	CG_InitG2Weapons();

/*
Ghoul2 Insert End
*/
	cg.loadLCARSStage = 9;


	// new stuff
	cgs.media.patrolShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/patrol.tga");
	cgs.media.assaultShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/assault.tga");
	cgs.media.campShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/camp.tga");
	cgs.media.followShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/follow.tga");
	cgs.media.defendShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/defend.tga");
	cgs.media.teamLeaderShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/team_leader.tga");
	cgs.media.retrieveShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/retrieve.tga");
	cgs.media.escortShader = trap->R_RegisterShaderNoMip("ui/assets/statusbar/escort.tga");
	cgs.media.cursor = trap->R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	cgs.media.sizeCursor = trap->R_RegisterShaderNoMip( "ui/assets/sizecursor.tga" );
	cgs.media.selectCursor = trap->R_RegisterShaderNoMip( "ui/assets/selectcursor.tga" );
	cgs.media.flagShaders[0] = trap->R_RegisterShaderNoMip("ui/assets/statusbar/flag_in_base.tga");
	cgs.media.flagShaders[1] = trap->R_RegisterShaderNoMip("ui/assets/statusbar/flag_capture.tga");
	cgs.media.flagShaders[2] = trap->R_RegisterShaderNoMip("ui/assets/statusbar/flag_missing.tga");

	cgs.media.halfShieldModel	= trap->R_RegisterModel ( "models/weaphits/testboom.md3" );
	cgs.media.halfShieldShader	= trap->R_RegisterShader( "halfShieldShell" );

	trap->FX_RegisterEffect("force/force_touch");
}


const char *CG_GetStringEdString(char *refSection, char *refName)
{
	static char text[2][1024]={{0}};	//just incase it's nested
	static int		index = 0;

	index ^= 1;
	trap->SE_GetStringTextString(va("%s_%s", refSection, refName), text[index], sizeof(text[0]));
	return text[index];
}

int	CG_GetClassCount(team_t team,int siegeClass );
int CG_GetTeamNonScoreCount(team_t team);

void CG_SiegeCountCvars( void )
{
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

/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
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
	int i=0;

	cg.scoreboard.spectatorList[0] = '\0';

	for ( i=0; i<cg.numScores; i++ )
	{
		if ( cg.scores[i].team == TEAM_SPECTATOR )
			Q_strcat( cg.scoreboard.spectatorList, sizeof( cg.scoreboard.spectatorList ), va( S_COLOR_WHITE"%s  ", cgs.clientinfo[cg.scores[i].client].name ) );
	}

	i = strlen( cg.scoreboard.spectatorList );
	if ( i != cg.scoreboard.spectatorLen )
	{//new spectator info
		cg.scoreboard.spectatorLen = i;
		cg.scoreboard.spectatorX = SCREEN_WIDTH;
		cg.scoreboard.spectatorResetTime = cg.time;
	}
#endif
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum, qfalse);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

		if (cg.clientNum == i) {
			continue;
		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i, qfalse);
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		trap->Error( ERR_DROP, "CG_ConfigString: bad index: %i", index );
		return NULL;
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( qboolean bForceStart ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( (const char **)&s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( (const char **)&s ), sizeof( parm2 ) );

	trap->S_StartBackgroundTrack( parm1, parm2, !bForceStart );
}

char *CG_GetMenuBuffer(const char *filename) {
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

//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle) {
	pc_token_t token;

	if (!trap->PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
	
	while ( 1 ) {
		if (!trap->PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

#if 0
		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}

//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.textFont);
			cgDC.Assets.qhMediumFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.smallFont);
			cgDC.Assets.qhSmallFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// smallFont
		if (Q_stricmp(token.string, "small2Font") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.smallFont);
			cgDC.Assets.qhSmall2Font = cgDC.RegisterFont(token.string);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "bigfont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.bigFont);
			cgDC.Assets.qhBigFont = cgDC.RegisterFont(token.string);
			continue;
		}

		// font
		if (Q_stricmp(token.string, "monoFont") == 0) {
			int pointSize;
			if (!trap->PC_ReadToken(handle, &token) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
//			cgDC.registerFont(token.string, pointSize, &cgDC.Assets.bigFont);
			cgDC.Assets.japp.fontMono = cgDC.RegisterFont(token.string);
			continue;
		}
#endif

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap->R_RegisterShaderNoMip(token.string);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap->S_RegisterSound( token.string );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!trap->PC_ReadToken(handle, &token)) {
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap->S_RegisterSound( token.string );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &cgDC.Assets.cursorStr)) {
				return qfalse;
			}
			cgDC.Assets.cursor = trap->R_RegisterShaderNoMip( cgDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &cgDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &cgDC.Assets.shadowColor)) {
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor.a;
			continue;
		}
	}
	return qfalse; // bk001204 - why not?
}

void CG_ParseMenu(const char *menuFile) {
	pc_token_t token;
	int handle;

	handle = trap->PC_LoadSource(menuFile);
	if (!handle)
		handle = trap->PC_LoadSource("ui/testhud.menu");
	if (!handle)
		return;

	while ( 1 ) {
		if (!trap->PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (CG_Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}


		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap->PC_FreeSource(handle);
}


qboolean CG_Load_Menu(const char **p) 
{

	char *token;

	token = COM_ParseExt((const char **)p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt((const char **)p, qtrue);
	
		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		CG_ParseMenu(token); 
	}
	return qfalse;
}


static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	return qfalse;
}


static int CG_FeederCount(float feederID) {
	int i, count;
	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_RED) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == TEAM_BLUE) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}


void CG_SetScoreSelection(void *p) {
	menuDef_t *menu = (menuDef_t*)p;
	playerState_t *ps = &cg.snap->ps;
	int i, red, blue;
	red = blue = 0;
	for (i = 0; i < cg.numScores; i++) {
		if (cg.scores[i].team == TEAM_RED) {
			red++;
		} else if (cg.scores[i].team == TEAM_BLUE) {
			blue++;
		}
		if (ps->clientNum == cg.scores[i].client) {
			cg.selectedScore = i;
		}
	}

	if (menu == NULL) {
		// just interested in setting the selected score
		return;
	}

	if ( cgs.gametype >= GT_TEAM ) {
		int feeder = FEEDER_REDTEAM_LIST;
		i = red;
		if (cg.scores[cg.selectedScore].team == TEAM_BLUE) {
			feeder = FEEDER_BLUETEAM_LIST;
			i = blue;
		}
		Menu_SetFeederSelection(menu, feeder, i, NULL);
	} else {
		Menu_SetFeederSelection(menu, FEEDER_SCOREBOARD, cg.selectedScore, NULL);
	}
}

// FIXME: might need to cache this info
static clientInfo_t * CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;
	if ( cgs.gametype >= GT_TEAM ) {
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (count == index) {
					*scoreIndex = i;
					return &cgs.clientinfo[cg.scores[i].client];
				}
				count++;
			}
		}
	}
	*scoreIndex = index;
	return &cgs.clientinfo[ cg.scores[index].client ];
}

static const char *CG_FeederItemText(float feederID, int index, int column,
									 qhandle_t *handle1, qhandle_t *handle2, qhandle_t *handle3) {
	gitem_t *item;
	int scoreIndex = 0;
	clientInfo_t *info = NULL;
	int team = -1;
	score_t *sp = NULL;

	*handle1 = *handle2 = *handle3 = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_RED;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_BLUE;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
			case 0:
				if ( info->powerups & ( 1 << PW_NEUTRALFLAG ) ) {
					item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_REDFLAG ) ) {
					item = BG_FindItemForPowerup( PW_REDFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else if ( info->powerups & ( 1 << PW_BLUEFLAG ) ) {
					item = BG_FindItemForPowerup( PW_BLUEFLAG );
					*handle1 = cg_items[ ITEM_INDEX(item) ].icon;
				} else {
					/*	
					if ( info->botSkill > 0 && info->botSkill <= 5 ) {
						*handle1 = cgs.media.botSkillShaders[ info->botSkill - 1 ];
					} else if ( info->handicap < 100 ) {
					return va("%i", info->handicap );
					}
					*/
				}
			break;
			case 1:
				if (team == -1) {
					return "";
				} else {
					*handle1 = CG_StatusHandle(info->teamTask);
				}
		  break;
			case 2:
				if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << sp->client ) ) {
					return "Ready";
				}
				if (team == -1) {
					if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) {
						return va("%i/%i", info->wins, info->losses);
					} else if (info->infoValid && info->team == TEAM_SPECTATOR ) {
						return "Spectator";
					} else {
						return "";
					}
				} else {
					if (info->teamLeader) {
						return "Leader";
					}
				}
			break;
			case 3:
				return info->name;
			break;
			case 4:
				return va("%i", info->score);
			break;
			case 5:
				return va("%4i", sp->time);
			break;
			case 6:
				if ( sp->ping == -1 ) {
					return "connecting";
				} 
				return va("%4i", sp->ping);
			break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index) {
	return 0;
}

static qboolean CG_FeederSelection(float feederID, int index, itemDef_t *item) {
	if ( cgs.gametype >= GT_TEAM ) {
		int i, count;
		int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_RED : TEAM_BLUE;
		count = 0;
		for (i = 0; i < cg.numScores; i++) {
			if (cg.scores[i].team == team) {
				if (index == count) {
					cg.selectedScore = i;
				}
				count++;
			}
		}
	} else {
		cg.selectedScore = index;
	}

	return qtrue;
}

static float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap->Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vector4 *color, const char *text, int cursorPos, char cursor, int limit, int style, int iMenuFont) {
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style, iMenuFont);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale) {
	switch (ownerDraw) {
	  case CG_GAME_TYPE:
			return CG_Text_Width( BG_GetGametypeString( cgs.gametype ), scale, FONT_MEDIUM );
	  case CG_GAME_STATUS:
			return CG_Text_Width(CG_GetGameStatusText(), scale, FONT_MEDIUM);
			break;
	  case CG_KILLER:
			return CG_Text_Width(CG_GetKillerText(), scale, FONT_MEDIUM);
			break;
	  case CG_RED_NAME:
			return CG_Text_Width(DEFAULT_REDTEAM_NAME/*cg_redTeamName.string*/, scale, FONT_MEDIUM);
			break;
	  case CG_BLUE_NAME:
			return CG_Text_Width(DEFAULT_BLUETEAM_NAME/*cg_blueTeamName.string*/, scale, FONT_MEDIUM);
			break;


	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap->CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle) {
  trap->CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h) {
  trap->CIN_SetExtents(handle, x, y, w, h);
  trap->CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle) {
  trap->CIN_RunCinematic(handle);
}

/*
=================
CG_LoadMenus();

=================
*/
void CG_LoadMenus(const char *menuFile) 
{
	const char	*token;
	const char	*p;
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUDEFFILE];

	len = trap->FS_Open( menuFile, &f, FS_READ );

	if ( !f ) 
	{
		trap->Print( va( S_COLOR_RED "menu file not found: %s, using default\n", menuFile ) );

		len = trap->FS_Open( "ui/jahud.txt", &f, FS_READ );
		if (!f) 
		{
			trap->Print( va( S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n", menuFile ) );
		}
	}

	if ( len >= MAX_MENUDEFFILE ) 
	{
		trap->Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE ) );
		trap->FS_Close( f );
		return;
	}

	trap->FS_Read( buf, len, f );
	buf[len] = 0;
	trap->FS_Close( f );
	
	p = buf;

	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') 
		{
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) 
		{
			break;
		}

		if (Q_stricmp(token, "loadmenu") == 0) 
		{
			if (CG_Load_Menu(&p)) 
			{
				continue;
			} 
			else 
			{
				break;
			}
		}
	}

	//Com_Printf("UI menu load time = %d milli seconds\n", cgi_Milliseconds() - start);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu() 
{
	const char *hudSet;

	cgDC.registerShaderNoMip			= trap->R_RegisterShaderNoMip;
	cgDC.setColor						= trap->R_SetColor;
	cgDC.drawHandlePic					= &CG_DrawPic;
	cgDC.drawStretchPic					= trap->R_DrawStretchPic;
	cgDC.drawText						= &CG_Text_Paint;
	cgDC.textWidth						= &CG_Text_Width;
	cgDC.textHeight						= &CG_Text_Height;
	cgDC.registerModel					= trap->R_RegisterModel;
	cgDC.modelBounds					= trap->R_ModelBounds;
	cgDC.fillRect						= &CG_FillRect;
	cgDC.drawRect						= &CG_DrawRect;   
	cgDC.drawSides						= &CG_DrawSides;
	cgDC.drawTopBottom					= &CG_DrawTopBottom;
	cgDC.clearScene						= trap->R_ClearScene;
	cgDC.addRefEntityToScene			= &SE_R_AddRefEntityToScene;
	cgDC.renderScene					= trap->R_RenderScene;
	cgDC.RegisterFont					= trap->R_RegisterFont;
	cgDC.Font_StrLenPixels				= trap->R_Font_StrLenPixels;
	cgDC.Font_StrLenChars				= trap->R_Font_StrLenChars;
	cgDC.Font_HeightPixels				= trap->R_Font_HeightPixels;
	cgDC.Font_DrawString				= trap->R_Font_DrawString;
	cgDC.Language_IsAsian				= trap->R_Language_IsAsian;
	cgDC.Language_UsesSpaces			= trap->R_Language_UsesSpaces;
	cgDC.AnyLanguage_ReadCharFromString	= trap->R_AnyLanguage_ReadCharFromString;
	cgDC.ownerDrawItem					= &CG_OwnerDraw;
	cgDC.getValue						= &CG_GetValue;
	cgDC.ownerDrawVisible				= &CG_OwnerDrawVisible;
	cgDC.runScript						= &CG_RunMenuScript;
	cgDC.deferScript					= &CG_DeferMenuScript;
	cgDC.getTeamColor					= &CG_GetTeamColor;
	cgDC.setCVar						= trap->Cvar_Set;
	cgDC.getCVarString					= trap->Cvar_VariableStringBuffer;
	cgDC.getCVarValue					= CG_Cvar_Get;
	cgDC.drawTextWithCursor				= &CG_Text_PaintWithCursor;
	//cgDC.setOverstrikeMode			= trap->Key_SetOverstrikeMode;
	//cgDC.getOverstrikeMode			= trap->Key_GetOverstrikeMode;
	cgDC.startLocalSound				= trap->S_StartLocalSound;
	cgDC.ownerDrawHandleKey				= &CG_OwnerDrawHandleKey;
	cgDC.feederCount					= &CG_FeederCount;
	cgDC.feederItemImage				= &CG_FeederItemImage;
	cgDC.feederItemText					= &CG_FeederItemText;
	cgDC.feederSelection				= &CG_FeederSelection;
	//cgDC.setBinding					= trap->Key_SetBinding;
	//cgDC.getBindingBuf				= trap->Key_GetBindingBuf;
	//cgDC.keynumToStringBuf			= trap->Key_KeynumToStringBuf;
	//cgDC.executeText					= trap->Cmd_ExecuteText;
	cgDC.Error							= trap->Error; 
	cgDC.Print							= trap->Print; 
	cgDC.ownerDrawWidth					= &CG_OwnerDrawWidth;
	//cgDC.Pause						= &CG_Pause;
	cgDC.registerSound					= trap->S_RegisterSound;
	cgDC.startBackgroundTrack			= trap->S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack			= trap->S_StopBackgroundTrack;
	cgDC.playCinematic					= &CG_PlayCinematic;
	cgDC.stopCinematic					= &CG_StopCinematic;
	cgDC.drawCinematic					= &CG_DrawCinematic;
	cgDC.runCinematicFrame				= &CG_RunCinematicFrame;
	
	Init_Display(&cgDC);

	Menu_Reset();

	hudSet = cg_hudFiles.string;
	if (hudSet[0] == '\0') 
	{
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus(hudSet);

}

void CG_AssetCache() {
	//if (Assets.textFont == NULL) {
	//  trap->R_RegisterFont("fonts/arial.ttf", 72, &Assets.textFont);
	//}
	//Assets.background = trap->R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
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

/*


*//*
Ghoul2 Insert Start
*/

// initialise the cg_entities structure - take into account the ghoul2 stl stuff in the active snap shots
void CG_Init_CG(void)
{
	memset( &cg, 0, sizeof(cg));
}

// initialise the cg_entities structure - take into account the ghoul2 stl stuff
void CG_Init_CGents(void)
{
	memset(&cg_entities, 0, sizeof(cg_entities));
}


void CG_InitItems(void)
{
	memset( cg_items, 0, sizeof( cg_items ) );
}

void CG_TransitionPermanent(void)
{
	centity_t	*cent = cg_entities;
	int			i;

	cg_numpermanents = 0;
	for(i=0;i<MAX_GENTITIES;i++,cent++)
	{
		if (trap->GetDefaultState(i, &cent->currentState))
		{
			cent->nextState = cent->currentState;
			VectorCopy (&cent->currentState.origin, &cent->lerpOrigin);
			VectorCopy (&cent->currentState.angles, &cent->lerpAngles);
			cent->currentValid = qtrue;

			cg_permanents[cg_numpermanents++] = cent;
		}
	}
}


//this is a 32k custom pool for parsing ents, it can get reset between ent parsing
//so we don't need a whole lot of memory -rww
#define MAX_CGSTRPOOL_SIZE		32768
static int cg_strPoolSize = 0;
static byte cg_strPool[MAX_CGSTRPOOL_SIZE];

char *CG_StrPool_Alloc(int size)
{
	char *giveThemThis;

	if (cg_strPoolSize+size >= MAX_CGSTRPOOL_SIZE)
	{
		Com_Error(ERR_DROP, "You exceeded the cgame string pool size. Bad programmer!\n");
	}

	giveThemThis = (char *) &cg_strPool[cg_strPoolSize];
	cg_strPoolSize += size;

	//memset it for them, just to be nice.
	memset(giveThemThis, 0, size);

	return giveThemThis;
}

void CG_StrPool_Reset(void)
{
	cg_strPoolSize = 0;
}

/*
=============
CG_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *CG_NewString( const char *string )
{
	char	*newb, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;

//	newb = CG_StrPool_Alloc( l );
	newb = malloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ )
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
			{
				if (string[i] == 'r')
					*new_p++ = '\r';
				else
					*new_p++ = '\\';
			}
		//	if (string[i] == 'r')
		//		*new_p++ = '\r';
		//	else
		//		*new_p++ = '\\';
		}
		else
		{
			*new_p++ = string[i];
		}
	}
	
	return newb;
}

//data to grab our spawn info into
typedef struct cgSpawnEnt_s
{
	float		angle;
	vector3		angles;
	char		*classname;
	float		fogstart;
	vector3		maxs;
	vector3		mins;
	char		*model;
	float		fScale;
	vector3		scale;
	int			onlyFogHere;
	vector3		origin;
	float		radarrange;
	float		zoffset;
} cgSpawnEnt_t;

#define	CGFOFS(x) offsetof(cgSpawnEnt_t, x)

//spawn fields for our cgame "entity"
BG_field_t cg_spawnFields[] =
{
	{ "angle",			CGFOFS( angle ),		F_FLOAT },
	{ "angles",			CGFOFS( angles ),		F_VECTOR },
	{ "classname",		CGFOFS( classname ),	F_LSTRING },
	{ "fogstart",		CGFOFS( fogstart ),		F_FLOAT },
	{ "maxs",			CGFOFS( maxs ),			F_VECTOR },
	{ "mins",			CGFOFS( mins ),			F_VECTOR },
	{ "model",			CGFOFS( model ),		F_LSTRING },
	{ "modelscale",		CGFOFS( fScale ),		F_FLOAT },
	{ "modelscale_vec",	CGFOFS( scale ),		F_VECTOR },
	{ "onlyfoghere",	CGFOFS( onlyFogHere ),	F_INT },
	{ "origin",			CGFOFS( origin ),		F_VECTOR },
	{ "radarrange",		CGFOFS( radarrange ),	F_FLOAT },
	{ "zoffset",		CGFOFS( zoffset ),		F_FLOAT },
};

static int cg_numSpawnVars;
static int cg_numSpawnVarChars;
static char *cg_spawnVars[MAX_SPAWN_VARS][2];
static char cg_spawnVarChars[MAX_SPAWN_VARS_CHARS];

//get some info from the skyportal ent on the map
qboolean cg_noFogOutsidePortal = qfalse;
void CG_CreateSkyPortalFromSpawnEnt(cgSpawnEnt_t *ent)
{
	if (ent->onlyFogHere)
	{ //only globally fog INSIDE the sky portal
		cg_noFogOutsidePortal = qtrue;
	}
}

//create a skybox portal orientation entity. there -should- only
//be one of these things per level. if there's more than one the
//next will just stomp over the last. -rww
qboolean cg_skyOri = qfalse;
vector3 cg_skyOriPos;
float cg_skyOriScale = 0.0f;
void CG_CreateSkyOriFromSpawnEnt(cgSpawnEnt_t *ent)
{
	cg_skyOri = qtrue;
	VectorCopy(&ent->origin, &cg_skyOriPos);
	cg_skyOriScale = ent->fScale;
}

//get brush box extents, note this does not care about bsp instances.
void CG_CreateBrushEntData(cgSpawnEnt_t *ent)
{
	trap->R_ModelBounds(trap->R_RegisterModel(ent->model), &ent->mins, &ent->maxs);
}

void CG_CreateWeatherZoneFromSpawnEnt(cgSpawnEnt_t *ent)
{
	CG_CreateBrushEntData(ent);
	trap->WE_AddWeatherZone(&ent->mins, &ent->maxs);
}

//create a new cgame-only model
void CG_CreateModelFromSpawnEnt(cgSpawnEnt_t *ent)
{
	int			modelIndex;
	refEntity_t	*RefEnt;
	vector3		mins, maxs;
	float		*radius;
	float		*zOff;

	if (NumMiscEnts >= MAX_MISC_ENTS)
	{
		Com_Error(ERR_DROP, "Too many misc_model_static's on level, ask a programmer to raise the limit (currently %i), or take some out.", MAX_MISC_ENTS);
		return;
	}
	
	if (!ent || !ent->model || !ent->model[0])
	{
		Com_Error(ERR_DROP, "misc_model_static with no model.");
		return;
	}

	radius = &Radius[NumMiscEnts];
	zOff = &zOffset[NumMiscEnts];
	RefEnt = &MiscEnts[NumMiscEnts++];

	modelIndex = trap->R_RegisterModel(ent->model);
	if (modelIndex == 0)
	{
		Com_Error(ERR_DROP, "misc_model_static failed to load model '%s'",ent->model);
		return;
	}

	memset(RefEnt, 0, sizeof(refEntity_t));
	RefEnt->reType = RT_MODEL;
	RefEnt->hModel = modelIndex;
	RefEnt->frame = 0;
	trap->R_ModelBounds(modelIndex, &mins, &maxs);
	VectorCopy(&ent->scale, &RefEnt->modelScale);
	if (ent->fScale)
	{ //use same scale on each axis then
		RefEnt->modelScale.x = RefEnt->modelScale.y = RefEnt->modelScale.z = ent->fScale;
	}
	VectorCopy(&ent->origin, &RefEnt->origin);
	VectorCopy(&ent->origin, &RefEnt->lightingOrigin);

	VectorScaleVector(&mins, &ent->scale, &mins);
	VectorScaleVector(&maxs, &ent->scale, &maxs);
	*radius = Distance(&mins, &maxs);
	*zOff = ent->zoffset;

	if (ent->angle)
	{ //only yaw supplied...
		ent->angles.yaw = ent->angle;
	}

	AnglesToAxis( &ent->angles, RefEnt->axis );
	ScaleModelAxis(RefEnt);
}

/*
====================
CG_AddSpawnVarToken
====================
*/
char *CG_AddSpawnVarToken( const char *string )
{
	int		l;
	char	*dest;

	l = strlen( string );
	if ( cg_numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		trap->Error( ERR_DROP, "CG_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = cg_spawnVarChars + cg_numSpawnVarChars;
	memcpy( dest, string, l+1 );

	cg_numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
CG_ParseSpawnVars

cgame version of G_ParseSpawnVars, for ents that don't really
need to take up an entity slot (e.g. static models) -rww
====================
*/
qboolean CG_ParseSpawnVars( void )
{
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	cg_numSpawnVars = 0;
	cg_numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap->R_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		trap->Error( ERR_DROP, "CG_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 )
	{	
		// parse key
		if ( !trap->R_GetEntityToken( keyname, sizeof( keyname ) ) )
		{
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' )
		{
			break;
		}
		
		// parse value	
		if ( !trap->R_GetEntityToken( com_token, sizeof( com_token ) ) )
		{ //this happens on mike's test level, I don't know why. Fixme?
			//trap->Error( ERR_DROP, "CG_ParseSpawnVars: EOF without closing brace" );
			break;
		}

		if ( com_token[0] == '}' )
		{
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: closing brace without data" );
		}
		if ( cg_numSpawnVars == MAX_SPAWN_VARS )
		{
			trap->Error( ERR_DROP, "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		cg_spawnVars[ cg_numSpawnVars ][0] = CG_AddSpawnVarToken( keyname );
		cg_spawnVars[ cg_numSpawnVars ][1] = CG_AddSpawnVarToken( com_token );
		cg_numSpawnVars++;
	}

	return qtrue;
}

/*
==============
CG_SpawnCGameEntFromVars

See if we should do something for this ent cgame-side -rww
==============
*/
void BG_ParseField( BG_field_t *l_fields, int numFields, const char *key, const char *value, byte *ent );

extern float cg_linearFogOverride; //cg_view.c
extern float cg_radarRange;//cg_draw.c
void CG_SpawnCGameEntFromVars(void)
{
	int i;
	cgSpawnEnt_t ent;

	memset(&ent, 0, sizeof(cgSpawnEnt_t));

	for (i = 0; i < cg_numSpawnVars; i++)
	{ //shove all this stuff into our data structure used specifically for getting spawn info
		BG_ParseField( cg_spawnFields, ARRAY_LEN( cg_spawnFields ), cg_spawnVars[i][0], cg_spawnVars[i][1], (byte *)&ent );
	}

	if (ent.classname && ent.classname[0])
	{ //we'll just stricmp this bastard, since there aren't all that many cgame-only things, and they all have special handling
		if (!Q_stricmp(ent.classname, "worldspawn"))
		{ //I'd like some info off this guy
			if (ent.fogstart)
			{ //linear fog method
				cg_linearFogOverride = ent.fogstart;
			}
			//get radarRange off of worldspawn
			if (ent.radarrange)
			{ //linear fog method
				cg_radarRange = ent.radarrange;
			}
		}
		else if (!Q_stricmp(ent.classname, "misc_model_static"))
		{ //we've got us a static model
			CG_CreateModelFromSpawnEnt(&ent);			
		}
		else if (!Q_stricmp(ent.classname, "misc_skyportal_orient"))
		{ //a sky portal orientation point
			CG_CreateSkyOriFromSpawnEnt(&ent);            
		}
		else if (!Q_stricmp(ent.classname, "misc_skyportal"))
		{ //might as well parse this thing cgame side for the extra info I want out of it
			CG_CreateSkyPortalFromSpawnEnt(&ent);            
		}
		else if (!Q_stricmp(ent.classname, "misc_weather_zone"))
		{ //might as well parse this thing cgame side for the extra info I want out of it
			CG_CreateWeatherZoneFromSpawnEnt(&ent);
		}
	}

	//reset the string pool for the next entity, if there is one
	CG_StrPool_Reset();
}

/*
==============
CG_SpawnCGameOnlyEnts

Parses entity string data for cgame-only entities, that we can throw away on
the server and never even bother sending. -rww
==============
*/
void CG_SpawnCGameOnlyEnts(void)
{
	//make sure it is reset
	trap->R_GetEntityToken(NULL, -1);

	if (!CG_ParseSpawnVars())
	{ //first one is gonna be the world spawn
		trap->Error(ERR_DROP, "no entities for cgame parse");
	}
	else
	{ //parse the world spawn info we want
		CG_SpawnCGameEntFromVars();
	}

	while(CG_ParseSpawnVars())
	{ //now run through the whole list, and look for things we care about cgame-side
		CG_SpawnCGameEntFromVars();
	}		
}

/*
Ghoul2 Insert End
*/

extern playerState_t *cgSendPS[MAX_GENTITIES]; //is not MAX_CLIENTS because NPCs exceed MAX_CLIENTS
void CG_PmoveClientPointerUpdate();

void WP_SaberLoadParms( void );
void BG_VehicleLoadParms( void );

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback )
{
	static gitem_t *item;
	char buf[64];
	const char	*s;
	int i = 0;

	BG_InitAnimsets(); //clear it out

	trap->RegisterSharedMemory(cg.sharedBuffer);

	//Load external vehicle data
	BG_VehicleLoadParms();

	// clear everything
/*
Ghoul2 Insert Start
*/

//	memset( cg_entities, 0, sizeof( cg_entities ) );
	CG_Init_CGents();
// this is a No-No now we have stl vector classes in here.
//	memset( &cg, 0, sizeof( cg ) );
	CG_Init_CG();
	CG_InitItems();

	//create the global jetpack instance
	CG_InitJetpackGhoul2();

	CG_PmoveClientPointerUpdate();

/*
Ghoul2 Insert End
*/

	//Load sabers.cfg data
	WP_SaberLoadParms();

	// this is kinda dumb as well, but I need to pre-load some fonts in order to have the text available
	//	to say I'm loading the assets.... which includes loading the fonts. So I'll set these up as reasonable
	//	defaults, then let the menu asset parser (which actually specifies the ingame fonts) load over them
	//	if desired during parse.  Dunno how legal it is to store in these cgDC things, but it causes no harm
	//	and even if/when they get overwritten they'll be legalised by the menu asset parser :-)
//	CG_LoadFonts();
	cgDC.Assets.qhSmallFont = CG_Cvar_Get( "font_small" );//trap->R_RegisterFont("ocr_a");
	cgDC.Assets.qhSmall2Font = CG_Cvar_Get( "font_small2" ); //arialnb
	cgDC.Assets.qhMediumFont = CG_Cvar_Get( "font_medium" );//trap->R_RegisterFont("ergoec");
	//Raz: fonts
	cgDC.Assets.japp.fontLarge = CG_Cvar_Get( "font_japplarge" );//trap->R_RegisterFont( "japplarge" );
	cgDC.Assets.japp.fontSmall = CG_Cvar_Get( "font_jappsmall" );//trap->R_RegisterFont( "jappsmall" );
	cgDC.Assets.japp.fontMono = CG_Cvar_Get( "font_jappmono" );//trap->R_RegisterFont( "jappmono" );
	cgDC.Assets.qhBigFont = CG_Cvar_Get( "font_big" );//cgDC.Assets.qhMediumFont;

	memset( &cgs, 0, sizeof( cgs ) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cg.loadLCARSStage		= 0;

	cg.itemSelect = -1;
	cg.forceSelect = -1;
	
	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap->R_RegisterShaderNoMip( "gfx/2d/charsgrid_med" );
	cgs.media.whiteShader		= trap->R_RegisterShader( "white" );

	cgs.media.loadBarLED		= trap->R_RegisterShaderNoMip( "gfx/hud/load_tick" );
	cgs.media.loadBarLEDCap		= trap->R_RegisterShaderNoMip( "gfx/hud/load_tick_cap" );
	cgs.media.loadBarLEDSurround= trap->R_RegisterShaderNoMip( "gfx/hud/mp_levelload" );

	// Force HUD set up
	cg.forceHUDActive = qtrue;
	cg.forceHUDTotalFlashTime = 0;
	cg.forceHUDNextFlashTime = 0;

	i = WP_NONE+1;
	while (i <= LAST_USEABLE_WEAPON)
	{
		item = BG_FindItemForWeapon(i);

		if (item && item->icon && item->icon[0])
		{
			cgs.media.weaponIcons[i] = trap->R_RegisterShaderNoMip(item->icon);
			cgs.media.weaponIcons_NA[i] = trap->R_RegisterShaderNoMip(va("%s_na", item->icon));
		}
		else
		{ //make sure it is zero'd (default shader)
			cgs.media.weaponIcons[i] = 0;
			cgs.media.weaponIcons_NA[i] = 0;
		}
		i++;
	}
	trap->Cvar_VariableStringBuffer("com_buildscript", buf, sizeof(buf));
	if (atoi(buf))
	{
		trap->R_RegisterShaderNoMip("gfx/hud/w_icon_saberstaff");
		trap->R_RegisterShaderNoMip("gfx/hud/w_icon_duallightsaber");
	}
	i = 0;

	// HUD artwork for cycling inventory,weapons and force powers 
	cgs.media.weaponIconBackground		= trap->R_RegisterShaderNoMip( "gfx/hud/background");
	cgs.media.forceIconBackground		= trap->R_RegisterShaderNoMip( "gfx/hud/background_f");
	cgs.media.inventoryIconBackground	= trap->R_RegisterShaderNoMip( "gfx/hud/background_i");

	//rww - precache holdable item icons here
	while (i < bg_numItems)
	{
		if (bg_itemlist[i].giType == IT_HOLDABLE)
		{
			if (bg_itemlist[i].icon)
			{
				cgs.media.invenIcons[bg_itemlist[i].giTag] = trap->R_RegisterShaderNoMip(bg_itemlist[i].icon);
			}
			else
			{
				cgs.media.invenIcons[bg_itemlist[i].giTag] = 0;
			}
		}

		i++;
	}

	//rww - precache force power icons here
	i = 0;

	while (i < NUM_FORCE_POWERS)
	{
		cgs.media.forcePowerIcons[i] = trap->R_RegisterShaderNoMip(HolocronIcons[i]);

		i++;
	}
	cgs.media.rageRecShader = trap->R_RegisterShaderNoMip("gfx/mp/f_icon_ragerec");


	//body decal shaders -rww
	cgs.media.bdecal_bodyburn1 = trap->R_RegisterShader("gfx/damage/bodyburnmark1");
	cgs.media.bdecal_saberglow = trap->R_RegisterShaderNoMip("gfx/damage/saberglowmark");
	cgs.media.bdecal_burn1 = trap->R_RegisterShader("gfx/damage/bodybigburnmark1");
	cgs.media.mSaberDamageGlow = trap->R_RegisterShaderNoMip("gfx/effects/saberDamageGlow");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	//Raz: Initialise third person setting
	cg.renderingThirdPerson = cg_thirdPerson.integer;

	cg.weaponSelect = WP_BRYAR_PISTOL;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap->GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap->GetGameState( &cgs.gameState );

	CG_TransitionPermanent(); //rwwRMG - added

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	if ( strcmp( s, GAME_VERSION ) ) {
		trap->Error( ERR_DROP, "Client/Server game mismatch: %s/%s", GAME_VERSION, s );
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	// load the new map
//	CG_LoadingString( "collision map" );

	trap->CM_LoadMap( cgs.mapname, qfalse );

	String_Init();

	cg.demoPlayback = demoPlayback;

	cg.loading = qtrue;		// force players to load instead of defer

	//make sure saber data is loaded before this! (so we can precache the appropriate hilts)
	CG_InitSiegeMode();

	//[TrueView]
	CG_TrueViewInit();
	//[/TrueView]

	CG_RegisterSounds();

//	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

//	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	CG_AssetCache();
	CG_LoadHudMenu();      // load new hud stuff

	cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic(qfalse);

//	CG_LoadingString( "Clearing light styles" );
	CG_ClearLightStyles();

//	CG_LoadingString( "Creating automap data" );
	//init automap
	trap->R_InitializeWireframeAutomap();

	CG_LoadingString( "" );

	CG_ShaderStateChanged();

	trap->S_ClearLoopingSounds();

	cg.distanceCull = trap->R_GetDistanceCull();

	//now get all the cgame only cents
	CG_SpawnCGameOnlyEnts();

	// console logging
	if ( cg_logConsole.string[0] ) {
		trap->FS_Open( cg_logConsole.string, &cg.log.console, FS_APPEND );
		if ( cg.log.console )
			trap->Print( "Logging to %s\n", cg_logConsole.string );
		else
			trap->Print( "WARNING: Couldn't open logfile: %s\n", cg_logConsole.string );
	}
	else
		trap->Print( "Not logging console to disk.\n" );

	// chat logging
	if ( cg_logChat.string[0] ) {
		trap->FS_Open( cg_logChat.string, &cg.log.chat, FS_APPEND );
		if ( cg.log.chat )
			trap->Print( "Logging to %s\n", cg_logChat.string );
		else
			trap->Print( "WARNING: Couldn't open logfile: %s\n", cg_logChat.string );
	}
	else
		trap->Print( "Not logging chat to disk.\n" );

	// security logging
	if ( cg_logSecurity.string[0] ) {
		trap->FS_Open( cg_logSecurity.string, &cg.log.security, FS_APPEND );
		if ( cg.log.security )
			trap->Print( "Logging to %s\n", cg_logSecurity.string );
		else
			trap->Print( "WARNING: Couldn't open logfile: %s\n", cg_logSecurity.string );
	}
	else
		trap->Print( "Not logging security to disk.\n" );

	#ifdef JPLUA
		//Raz: Lua!
		JPLua_Init();
	#endif // JPLUA

	JP_ChatboxInit();

	#ifdef _WIN32
		{//Detect the timestamp format via the registry
			#define JP_TIMESTAMP_REGISTRY_KEY "Control Panel\\International"
			#define JP_TIMESTAMP_REGISTRY_NAME "sTimeFormat"
			char registryValue[256] = { 0 };
			HKEY hkey;
			unsigned long datalen = sizeof( registryValue );  // data field length(in), data returned length(out) 
			unsigned long datatype; // #defined in winnt.h (predefined types 0-11)
			LSTATUS error;
			if ( (error = RegOpenKeyExA( (HKEY)HKEY_CURRENT_USER, (LPCSTR)JP_TIMESTAMP_REGISTRY_KEY, (DWORD)0, (REGSAM)KEY_QUERY_VALUE, &hkey )) == ERROR_SUCCESS )
			{
				if ( (error = RegQueryValueExA( (HKEY)hkey, (LPCSTR)JP_TIMESTAMP_REGISTRY_NAME, (LPDWORD)NULL, (LPDWORD)&datatype, (LPBYTE)registryValue, (LPDWORD)&datalen)) == ERROR_SUCCESS )
				{
					if ( registryValue[0] == 'H' )
						cg.japp.timestamp24Hour = qtrue;
					RegCloseKey( hkey );
				}
				else
					Com_Printf( S_COLOR_RED"Error, couldn't query registry string %s, error code %i\n", "sTimeFormat", error );
			}
			else
				Com_Printf( S_COLOR_RED"Error, couldn't open registry key %s, error code %i\n", JP_TIMESTAMP_REGISTRY_KEY, error );
		}
	#endif

	trap->Cvar_VariableStringBuffer( "rate", buf, sizeof( buf ) );
	if ( atoi( buf ) == 4000 )
		trap->Print( S_COLOR_YELLOW"WARNING: Default /rate value detected. Suggest typing /rate 25000 for a smoother connection!\n" );
}

//makes sure returned string is in localized format
const char *CG_GetLocationString(const char *loc)
{
	static char text[1024]={0};

	if (!loc || loc[0] != '@')
	{ //just a raw string
		return loc;
	}

	trap->SE_GetStringTextString(loc+1, text, sizeof(text));
	return text;
}

//clean up all the ghoul2 allocations, the nice and non-hackly way -rww
void CG_KillCEntityG2(int entNum);
void CG_DestroyAllGhoul2(void)
{
	int i = 0;
	int j;

//	Com_Printf("... CGameside GHOUL2 Cleanup\n");
	while (i < MAX_GENTITIES)
	{ //free all dynamically allocated npc client info structs and ghoul2 instances
		CG_KillCEntityG2(i);	
		i++;
	}
	
	//Clean the weapon instances
	CG_ShutDownG2Weapons();

	i = 0;
	while (i < MAX_ITEMS)
	{ //and now for items
		j = 0;
		while (j < MAX_ITEM_MODELS)
		{
			if (cg_items[i].g2Models[j] && trap->G2_HaveWeGhoul2Models(cg_items[i].g2Models[j]))
			{
				trap->G2API_CleanGhoul2Models(&cg_items[i].g2Models[j]);
				cg_items[i].g2Models[j] = NULL;
			}
			j++;
		}
		i++;
	}

	//Clean the global jetpack instance
	CG_CleanJetpackGhoul2();
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) 
{
	BG_ClearAnimsets(); //free all dynamic allocations made through the engine

	CG_DestroyAllGhoul2();

//	Com_Printf("... FX System Cleanup\n");
	trap->FX_FreeSystem();
	trap->ROFF_Clean();

	//reset weather
	trap->R_WorldEffectCommand("die");

	UI_CleanupGhoul2();
	//If there was any ghoul2 stuff in our side of the shared ui code, then remove it now.

	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	#ifdef JPLUA
		//Raz: Lua!
		JPLua_Shutdown();
	#endif // JPLUA


	//Raz: Close log file!
	CG_LogPrintf( cg.log.console, "End logging\n------------------------------------------------------------\n\n" );
	CG_LogPrintf( cg.log.chat, "End logging\n------------------------------------------------------------\n\n" );
	trap->FS_Close( cg.log.console );
	trap->FS_Close( cg.log.chat );
	trap->FS_Close( cg.log.security );
}

/*
===============
CG_NextForcePower_f
===============
*/
void CG_NextForcePower_f( void ) 
{
	int current;
	usercmd_t cmd;
	if ( !cg.snap )
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	current = trap->GetCurrentCmdNumber();
	trap->GetUserCmd(current, &cmd);
	if ((cmd.buttons & BUTTON_USE) || CG_NoUseableForce())
	{
		CG_NextInventory_f();
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

//	BG_CycleForce(&cg.snap->ps, 1);
	if (cg.forceSelect != -1)
	{
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce(&cg.snap->ps, 1);

	if (cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected))
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

/*
===============
CG_PrevForcePower_f
===============
*/
void CG_PrevForcePower_f( void ) 
{
	int current;
	usercmd_t cmd;
	if ( !cg.snap )
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	current = trap->GetCurrentCmdNumber();
	trap->GetUserCmd(current, &cmd);
	if ((cmd.buttons & BUTTON_USE) || CG_NoUseableForce())
	{
		CG_PrevInventory_f();
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

//	BG_CycleForce(&cg.snap->ps, -1);
	if (cg.forceSelect != -1)
	{
		cg.snap->ps.fd.forcePowerSelected = cg.forceSelect;
	}

	BG_CycleForce(&cg.snap->ps, -1);

	if (cg.snap->ps.fd.forcePowersKnown & (1 << cg.snap->ps.fd.forcePowerSelected))
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		cg.forceSelectTime = cg.time;
	}
}

void CG_NextInventory_f(void)
{
	if ( !cg.snap )
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.itemSelect != -1)
	{
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
	}
	BG_CycleInven(&cg.snap->ps, 1);

	if (cg.snap->ps.stats[STAT_HOLDABLE_ITEM])
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}

void CG_PrevInventory_f(void)
{
	if ( !cg.snap )
	{
		return;
	}

	if (cg.snap->ps.pm_flags & PMF_FOLLOW)
	{
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{
		return;
	}

	if (cg.itemSelect != -1)
	{
		cg.snap->ps.stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
	}
	BG_CycleInven(&cg.snap->ps, -1);

	if (cg.snap->ps.stats[STAT_HOLDABLE_ITEM])
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
		cg.invenSelectTime = cg.time;
	}
}


/*
=================
CG_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/

void QDECL CG_LogPrintf( fileHandle_t fileHandle, const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	int			mins, seconds, msec, l;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds %= 60;
	msec %= 1000;

	Com_sprintf( string, sizeof( string ), "%i:%02i ", mins, seconds );
	l = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string + l, sizeof( string ) - l, fmt, argptr );
	va_end( argptr );

	if ( !fileHandle )
		return;

	trap->FS_Write( string, strlen( string ), fileHandle );
}

/*
=================
CG_SecurityLogPrintf

Print to the security logfile with a time stamp if it is open
=================
*/
void QDECL CG_SecurityLogPrintf( const char *fmt, ... ) {
	va_list		argptr;
	char		string[1024] = {0};
	time_t		rawtime;
	int			timeLen=0;

	time( &rawtime );
	localtime( &rawtime );
	strftime( string, sizeof( string ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &rawtime ) );
	timeLen = strlen( string );

	va_start( argptr, fmt );
	Q_vsnprintf( string+timeLen, sizeof( string ) - timeLen, fmt, argptr );
	va_end( argptr );

	trap->Print( "%s", string + timeLen );

	if ( !cg.log.security )
		return;

	trap->FS_Write( string, strlen( string ), cg.log.security );
}

static void _CG_MouseEvent( int x, int y ) {
	cgDC.cursorx = cgs.cursorX;
	cgDC.cursory = cgs.cursorY;
	CG_MouseEvent( x, y );
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
		icc->conCommand[0] = 0;
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

	memcpy( &cg_autoMapInput, autoInput, sizeof( autoMapInput_t ) );

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

/*
============
GetModuleAPI
============
*/

cgameImport_t *trap = NULL;

typedef int (*R_Font_StrLenPixels_t)( const char *text, const int iFontIndex, const float scale );
int (*R_Font_StrLenPixels)( const char *text, const int iFontIndex, const float scale );
float CG_Font_StrLenPixels( const char *text, const int iFontIndex, const float scale ) {
	float width = (float)R_Font_StrLenPixels( text, iFontIndex, 4.0f );
	return (width/4.0f) * scale;
}

int (*R_Font_HeightPixels)( const int iFontIndex, const float scale );
typedef int (*R_Font_HeightPixels_t)( const int iFontIndex, const float scale );
float CG_Font_HeightPixels( const int iFontIndex, const float scale ) {
	float height = (float)R_Font_HeightPixels( iFontIndex, 4.0f );
	return (height/4.0f) * scale;
}

Q_EXPORT cgameExport_t* QDECL GetModuleAPI( int apiVersion, cgameImport_t *import )
{
	static cgameExport_t cge = {0};
	
	assert( import );
	trap = import;
	Com_Printf	= trap->Print;
	Com_Error	= trap->Error;

	//HACK: work-around for JA's crappy width calculation
	R_Font_StrLenPixels = (R_Font_StrLenPixels_t)trap->R_Font_StrLenPixels;
	R_Font_HeightPixels = (R_Font_HeightPixels_t)trap->R_Font_HeightPixels;
	trap->R_Font_StrLenPixels = CG_Font_StrLenPixels;
	trap->R_Font_HeightPixels = CG_Font_HeightPixels;

	memset( &cge, 0, sizeof( cge ) );

	if ( apiVersion != CGAME_API_VERSION ) {
		trap->Print( "Mismatched CGAME_API_VERSION: expected %i, got %i\n", CGAME_API_VERSION, apiVersion );
		return NULL;
	}

	cge.Init					= CG_Init;
	cge.Shutdown				= CG_Shutdown;
	cge.ConsoleCommand			= CG_ConsoleCommand;
	cge.DrawActiveFrame			= CG_DrawActiveFrame;
	cge.CrosshairPlayer			= CG_CrosshairPlayer;
	cge.LastAttacker			= CG_LastAttacker;
	cge.KeyEvent				= CG_KeyEvent;
	cge.MouseEvent				= _CG_MouseEvent;
	cge.EventHandling			= CG_EventHandling;
	cge.PointContents			= C_PointContents;
	cge.GetLerpOrigin			= C_GetLerpOrigin;
	cge.GetLerpData				= C_GetLerpData;
	cge.Trace					= C_Trace;
	cge.G2Trace					= C_G2Trace;
	cge.G2Mark					= C_G2Mark;
	cge.RagCallback				= CG_RagCallback;
	cge.IncomingConsoleCommand	= CG_IncomingConsoleCommand;
	cge.NoUseableForce			= CG_NoUseableForce;
	cge.GetOrigin				= CG_GetOrigin;
	cge.GetAngles				= CG_GetAngles;
	cge.GetOriginTrajectory		= CG_GetOriginTrajectory;
	cge.GetAngleTrajectory		= CG_GetAngleTrajectory;
	cge.ROFF_NotetrackCallback	= _CG_ROFF_NotetrackCallback;
	cge.MapChange				= CG_MapChange;
	cge.AutomapInput			= CG_AutomapInput;
	cge.MiscEnt					= CG_MiscEnt;
	cge.CameraShake				= CG_FX_CameraShake;


	return &cge;
}

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT intptr_t vmMain( int command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 )
{
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
		CG_DrawActiveFrame( arg0, arg1, arg2 );
		return 0;

	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();

	case CG_LAST_ATTACKER:
		return CG_LastAttacker();

	case CG_KEY_EVENT:
		CG_KeyEvent( arg0, arg1 );
		return 0;

	case CG_MOUSE_EVENT:
		_CG_MouseEvent( arg0, arg1 );
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
