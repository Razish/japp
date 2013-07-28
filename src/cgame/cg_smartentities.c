//	==============================
//	MultiPlugin 1.0 by Deathspike
//	==============================
//	Concept, Proof-of-Concept and development of "Smart Entities" by Deathspike.
//	Based and tested on MultiPlugin framework for testing and development interests.
//	Later ported to MovieBattles and went into live testing.
//	
//	Adapted by Scooper
//	
//	Haxxored by Raz0r
//	==============================

#include "cg_local.h"

float VectorAngle( const vector3 *a, const vector3 *b )
{
    float lA	= VectorLength( a);
	float lB	= VectorLength( b );
	float lAB	= lA * lB;

	if( lAB == 0.0 )
		return 0.0;
	else
		return (float)(acosf(DotProduct(a,b)/lAB) * (180.f/M_PI));
}

void MakeVector(const vector3 *ain, vector3 *vout)
{
	float pitch, yaw, tmp;		
	
	pitch =	(float) (ain->pitch * M_PI/180.0f);
	yaw =	(float) (ain->yaw * M_PI/180.0f);
	tmp =	(float) cosf(pitch);
	
	vout->x = (float) (-tmp * -cosf(yaw));
	vout->y = (float) (sinf(yaw)*tmp);
	vout->z = (float) -sinf(pitch);
}

// ==============================
// SE_PerformTrace

void SE_PerformTrace(trace_t *results, vector3 *start, vector3 *end, int mask)
{
	trap_CM_BoxTrace(results, start, end, NULL, NULL, 0, mask);
}

// ==============================
// SE_IsPlayerCrouching

qboolean SE_IsPlayerCrouching( int entitiy )
{
	// FIXME: This is no proper way to determine if a client is actually in a crouch position, we want to do this
	// in order to properly hide a client from the enemy when he is crouching behind an obstacle and could not possibly
	// be seen.

	centity_t		*cent	= &cg_entities[entitiy];
	playerState_t	*ps		= cent->playerState;

	if (!cent || !ps)
		return qfalse;

	if (ps->forceHandExtend == HANDEXTEND_KNOCKDOWN)
		return qtrue;

	if (ps->pm_flags & PMF_DUCKED)
		return qtrue;

	switch((ps->legsAnim))
	{
		case BOTH_GETUP1:
		case BOTH_GETUP2:
		case BOTH_GETUP3:
		case BOTH_GETUP4:
		case BOTH_GETUP5:
		case BOTH_FORCE_GETUP_F1:
		case BOTH_FORCE_GETUP_F2:
		case BOTH_FORCE_GETUP_B1:
		case BOTH_FORCE_GETUP_B2:
		case BOTH_FORCE_GETUP_B3:
		case BOTH_FORCE_GETUP_B4:
		case BOTH_FORCE_GETUP_B5:
		case BOTH_GETUP_BROLL_B:
		case BOTH_GETUP_BROLL_F:
		case BOTH_GETUP_BROLL_L:
		case BOTH_GETUP_BROLL_R:
		case BOTH_GETUP_FROLL_B:
		case BOTH_GETUP_FROLL_F:
		case BOTH_GETUP_FROLL_L:
		case BOTH_GETUP_FROLL_R:
		{
			return qtrue;
		}
	}

	switch((ps->torsoAnim))
	{
		case BOTH_GETUP1:
		case BOTH_GETUP2:
		case BOTH_GETUP3:
		case BOTH_GETUP4:
		case BOTH_GETUP5:
		case BOTH_FORCE_GETUP_F1:
		case BOTH_FORCE_GETUP_F2:
		case BOTH_FORCE_GETUP_B1:
		case BOTH_FORCE_GETUP_B2:
		case BOTH_FORCE_GETUP_B3:
		case BOTH_FORCE_GETUP_B4:
		case BOTH_FORCE_GETUP_B5:
		case BOTH_GETUP_BROLL_B:
		case BOTH_GETUP_BROLL_F:
		case BOTH_GETUP_BROLL_L:
		case BOTH_GETUP_BROLL_R:
		case BOTH_GETUP_FROLL_B:
		case BOTH_GETUP_FROLL_F:
		case BOTH_GETUP_FROLL_L:
		case BOTH_GETUP_FROLL_R:
		{
			return qtrue;
		}
	}

	return qfalse;
}

// ==============================
// SE_RenderInFOV
// ==============================

qboolean SE_RenderInFOV( vector3 *testOrigin )
{
	vector3	tmp, aim, view;
	refdef_t *refdef = CG_GetRefdef();

	VectorCopy(&refdef->vieworg, &tmp);
	VectorSubtract(testOrigin, &tmp, &aim);	
	MakeVector(&refdef->viewangles, &view);

	if (VectorAngle(&view,&aim) > (refdef->fov_x/1.2))
		return qfalse;

	return qtrue;
}

// ==============================
// SE_RenderIsVisible
// ==============================

qboolean SE_RenderIsVisible( vector3 *startPos, vector3 *testOrigin, qboolean reversedCheck )
{
    trace_t results;

	//SE_PerformTrace(&results, startPos, testOrigin, MASK_SOLID);
	trap_CM_BoxTrace(&results, startPos, testOrigin, NULL, NULL, 0, MASK_SOLID);

	if ( results.fraction < 1.0f )
	{
		if (( results.surfaceFlags & SURF_FORCEFIELD )
			/*|| ( results.surfaceFlags & SURF_NODRAW )*/
			|| (( results.surfaceFlags & MATERIAL_MASK ) == MATERIAL_GLASS )
			|| (( results.surfaceFlags & MATERIAL_MASK ) == MATERIAL_SHATTERGLASS ))
		{//	FIXME: This is a quick hack to render people and things through glass and force fields,
		//	but will also take effect even if there is another wall between them (and double glass). 
		//	Which is bad, of course - but nothing i can prevent right now.
			if ( reversedCheck || SE_RenderIsVisible( testOrigin, startPos, qtrue ))
				return qtrue;
		}

		return qfalse;
	}

    return qtrue;
}

// ==============================
// SE_RenderPlayerPoints
// ==============================
// [0]	= playerOrigin
// [1]	= playerOrigin							+ height
// [2]	= playerOrigin	+ forward
// [3]	= playerOrigin	+ forward				+ height
// [4]	= playerOrigin	+ right		- forward
// [5]	= playerOrigin	+ right		- forward	+ height
// [6]	= playerOrigin	- right		- forward
// [7]	= playerOrigin	- right		- forward	+ height
// ==============================

qboolean SE_RenderPlayerPoints( qboolean isCrouching, vector3 *playerAngles, vector3 *playerOrigin, vector3 playerPoints[9] )
{
	int		isHeight			= (( isCrouching ) ? 32 : 56 );
	vector3	forward, right, up;
	
	AngleVectors( playerAngles, &forward, &right, &up );

	VectorMA( playerOrigin,		 32,	&up,		&playerPoints[0] );
	VectorMA( playerOrigin,		 64,	&forward,	&playerPoints[1] );
	VectorMA( &playerPoints[1],	 64,	&right,		&playerPoints[1] );
	VectorMA( playerOrigin,		 64,	&forward,	&playerPoints[2] );
	VectorMA( &playerPoints[2],	-64,	&right,		&playerPoints[2] );
	VectorMA( playerOrigin,		-64,	&forward,	&playerPoints[3] );
	VectorMA( &playerPoints[3],	 64,	&right,		&playerPoints[3] );
	VectorMA( playerOrigin,		-64,	&forward,	&playerPoints[4] );
	VectorMA( &playerPoints[4],	-64,	&right,		&playerPoints[4] );

	VectorMA( &playerPoints[1], isHeight, &up, &playerPoints[5] );
	VectorMA( &playerPoints[2], isHeight, &up, &playerPoints[6] );
	VectorMA( &playerPoints[3], isHeight, &up, &playerPoints[7] );
	VectorMA( &playerPoints[4], isHeight, &up, &playerPoints[8] );

	return qtrue;
}

// ==============================
// SE_RenderPlayerChecks
// ==============================

qboolean SE_RenderPlayerChecks( vector3 *playerOrigin, vector3 playerPoints[9] )
{
	trace_t		results;
	int			i;

	for (i=0; i<9; i++)
	{
		if (trap_CM_PointContents(&playerPoints[i], 0 ) == CONTENTS_SOLID)
		{
			//SE_PerformTrace(&results, playerOrigin, playerPoints[i], MASK_SOLID);
			trap_CM_BoxTrace(&results, playerOrigin, &playerPoints[i], NULL, NULL, 0, MASK_SOLID);
			VectorCopy(&results.endpos, &playerPoints[i]);
		}
	}

	return qtrue;
}

// ==============================
// SE_RenderPlayer
// ==============================

qboolean SE_RenderPlayer( int targIndex )
{
	int			i;
	int			selfIndex	= cg.snap->ps.clientNum;
	//centity_t	*self		= &cg_entities[selfIndex];
	centity_t	*targ		= &cg_entities[targIndex];
	vector3		startPos;
	vector3		targPos[9];
	refdef_t *refdef = CG_GetRefdef();

//	(cg_entities[selfIndex].playerState->zoomMode) ? VectorCopy(cg_entities[selfIndex].lerpOrigin, startPos) : VectorCopy(refdef->vieworg, startPos);
	VectorCopy( (cg_entities[selfIndex].playerState->zoomMode ? &cg_entities[selfIndex].lerpOrigin : &refdef->vieworg), &startPos );

	//Raz: Added 'fix' so people are visible if you're in water.
	if (( trap_CM_PointContents(&startPos, 0) & CONTENTS_WATER ) ||
		( trap_CM_PointContents(&startPos, 0) & CONTENTS_LAVA ) ||
		( trap_CM_PointContents(&startPos, 0) & CONTENTS_SLIME ) )
		return qtrue;
//	if ( refdef->viewContents&CONTENTS_WATER )
//		return true;

	if (( trap_CM_PointContents(&startPos, 0) & CONTENTS_SOLID ) ||
		( trap_CM_PointContents(&startPos, 0) & CONTENTS_TERRAIN ) ||
		( trap_CM_PointContents(&startPos, 0) & CONTENTS_OPAQUE ))
		return qfalse;

	SE_RenderPlayerPoints( SE_IsPlayerCrouching( targIndex ), &targ->lerpAngles, &targ->lerpOrigin, targPos );
	SE_RenderPlayerChecks( &targ->lerpOrigin, targPos );

	for (i=0; i<9; i++)
		if ( SE_RenderIsVisible( &startPos, &targPos[i], qfalse ) )
			return qtrue;

	return qfalse;
}

// ==============================
// SE_RenderThisEntity
// ==============================
// Tracing non-players seems to have a bad effect, we know players are limited to 32 per frame,
// however other gentities that are being added are not! It's stupid to actually add traces for it,
// even with a limited form i used before of 2 traces per object. There are to many to track and
// simply drawing them takes less FPS either way.
// ==============================

qboolean SE_RenderThisEntity( vector3 *testOrigin, int gameEntity )
{
	// If we do not have a snapshot, we cannot calculate anything.
	if ( !cg.snap )
		return qtrue;

	// This is a non-player object, just render it (see above).
	if ( gameEntity < 0 || gameEntity >= MAX_CLIENTS )
		return qtrue;

	// If this player is me, or my spectee, we will always draw and don't trace.
	if ( gameEntity == cg.snap->ps.clientNum )
		return qtrue;

	if ( cg.predictedPlayerState.zoomMode )
		return qtrue;

	// Force seeing is one of those things that wants everything tracked as well.
	// FIXME: Cheating check for setting this flag, that would be madness.
//	if ( cg.snap->ps.fd.forcePowersActive & ( 1 << FP_SEE ))
//		return true;

	// Not rendering; this player is not in our FOV.
	if ( !SE_RenderInFOV( testOrigin ) )
		return qfalse;

	// Not rendering; this player's traces did not appear in my screen.
	if ( !SE_RenderPlayer( gameEntity ) )
		return qfalse;

	return qtrue;
}
