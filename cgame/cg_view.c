;// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"
#include "bg_saga.h"
#include "cg_lights.h"
#include "cg_luaevent.h"

#define CAMERA_SIZE	4

static int GetCameraClip( void ) {
	return (cg.japp.isGhosted) ? (MASK_SOLID) : (MASK_SOLID|CONTENTS_PLAYERCLIP);
}

refdef_t *CG_GetRefdef( void ) {
	return &cg.refdef[cg.currentRefdef];
}




/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like 
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vector3		angles;
	refdef_t *refdef = CG_GetRefdef();

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap->Cmd_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );

	if ( trap->Cmd_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		trap->Print( "Can't register model\n" );
		return;
	}

	VectorMA( &refdef->vieworg, 100, &refdef->viewaxis[0], &cg.testModelEntity.origin );

	angles.pitch = 0;
	angles.yaw = 180 + refdef->viewangles.yaw;
	angles.roll = 0;

	AnglesToAxis( &angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	//cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;

	// rww - 9-13-01 [1-26-01-sof2]
	cg.testModelEntity.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	trap->Print( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	trap->Print( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;
	refdef_t *refdef = CG_GetRefdef();

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap->R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		trap->Print ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( &refdef->vieworg, &cg.testModelEntity.origin );
		VectorCopy( &refdef->viewaxis[0], &cg.testModelEntity.axis[0] );
		VectorCopy( &refdef->viewaxis[1], &cg.testModelEntity.axis[1] );
		VectorCopy( &refdef->viewaxis[2], &cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin.data[i] += refdef->viewaxis[0].data[i] * cg.gunAlign.x;
			cg.testModelEntity.origin.data[i] += refdef->viewaxis[1].data[i] * cg.gunAlign.y;
			cg.testModelEntity.origin.data[i] += refdef->viewaxis[2].data[i] * cg.gunAlign.z;
		}
	}

	SE_R_AddRefEntityToScene( &cg.testModelEntity, MAX_CLIENTS );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;
	refdef_t *refdef = CG_GetRefdef();

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewSize.integer < 30) {
			trap->Cvar_Set ("cg_viewSize","30");
			size = 30;
		} else if (cg_viewSize.integer > 100) {
			trap->Cvar_Set ("cg_viewSize","100");
			size = 100;
		} else {
			size = cg_viewSize.integer;
		}

	}
	refdef->width = cgs.glconfig.vidWidth*size/100;
	refdef->width &= ~1;

	refdef->height = cgs.glconfig.vidHeight*size/100;
	refdef->height &= ~1;

	refdef->x = (cgs.glconfig.vidWidth - refdef->width)/2;
	refdef->y = (cgs.glconfig.vidHeight - refdef->height)/2;
}

//==============================================================================

//==============================================================================
//==============================================================================
// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;
	refdef_t *refdef = CG_GetRefdef();
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		refdef->vieworg.z -= cg.stepChange * (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

#define CAMERA_DAMP_INTERVAL	50

static vector3	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vector3	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vector3	camerafwd, cameraup;

vector3	cameraFocusAngles[MAX_CLIENTS],			cameraFocusLoc[MAX_CLIENTS];
vector3	cameraIdealTarget[MAX_CLIENTS],			cameraIdealLoc[MAX_CLIENTS];
vector3	cameraCurTarget[MAX_CLIENTS]={0,0,0},	cameraCurLoc[MAX_CLIENTS]={0,0,0};
vector3	cameraOldLoc[MAX_CLIENTS]={0,0,0},		cameraNewLoc[MAX_CLIENTS]={0,0,0};
int		cameraLastFrame[MAX_CLIENTS]={0};

float	cameraLastYaw[MAX_CLIENTS]={0};
float	cameraStiffFactor[MAX_CLIENTS]= {0.0f};

/*
===============
Notes on the camera viewpoint in and out...

refdef->vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
refdef->viewangles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/
  
extern qboolean gCGHasFallVector;
extern vector3 gCGFallVector;

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(int clientNum)
{
	refdef_t *refdef = CG_GetRefdef();
	// Initialize IdealTarget
	if (gCGHasFallVector)
	{
		VectorCopy(&gCGFallVector, &cameraFocusLoc[clientNum]);
	}
	else
	{
		VectorCopy(&refdef->vieworg, &cameraFocusLoc[clientNum]);
	}

	//Raz: Use predicted playerstate!
	// Add in the new viewheight
	cameraFocusLoc[clientNum].z += cg.predictedPlayerState.viewheight; //cg.snap->ps.viewheight

	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
//	VectorMA(cameraFocusLoc, thirdPersonVertOffset, cameraup, cameraIdealTarget);
	
	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
	VectorCopy( &cameraFocusLoc[clientNum], &cameraIdealTarget[clientNum] );
	
	{
		float vertOffset = cg_thirdPersonVertOffset.value;

		if (cg.snap && cg.snap->ps.m_iVehicleNum)
		{
			centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
			if (veh->m_pVehicle &&
				veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
			{ //override the range with what the vehicle wants it to be
				if ( veh->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset )
				{
					if ( cg.predictedPlayerState.viewangles.pitch > 0 )
					{
						vertOffset = 130+cg.predictedPlayerState.viewangles.pitch*-10;
						if ( vertOffset < -170 )
						{
							vertOffset = -170;
						}
					}
					else if ( cg.predictedPlayerState.viewangles.pitch < 0 )
					{
						vertOffset = 130+cg.predictedPlayerState.viewangles.pitch *-5;
						if ( vertOffset > 130 )
						{
							vertOffset = 130;
						}
					}
					else
					{
						vertOffset = 30;
					}
				}
				else 
				{
					vertOffset = veh->m_pVehicle->m_pVehicleInfo->cameraVertOffset;
				}
			}
			else if ( veh->m_pVehicle
				&& veh->m_pVehicle->m_pVehicleInfo
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL )
			{
				vertOffset = 0;
			}
		}
		cameraIdealTarget[clientNum].z += vertOffset;
	}
	//VectorMA(cameraFocusLoc, cg_thirdPersonVertOffset.value, cameraup, cameraIdealTarget);
}

	

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(int clientNum)
{
	float thirdPersonRange = cg_thirdPersonRange.value;

	if (cg.snap && cg.snap->ps.m_iVehicleNum)
	{
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if (veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
		{ //override the range with what the vehicle wants it to be
			thirdPersonRange = veh->m_pVehicle->m_pVehicleInfo->cameraRange;
			if ( veh->playerState->hackingTime )
			{
				thirdPersonRange += fabs(((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * 100.0f;
			}
		}
	}

	if ( cg.snap
		&& (cg.snap->ps.eFlags2&EF2_HELD_BY_MONSTER) 
		&& cg.snap->ps.hasLookTarget
		&& cg_entities[cg.snap->ps.lookTarget].currentState.NPC_class == CLASS_RANCOR )//only possibility for now, may add Wampa and sand creature later
	{//stay back
		//thirdPersonRange = 180.0f;
		thirdPersonRange = 120.0f;
	}

	VectorMA(&cameraIdealTarget[clientNum], -(thirdPersonRange), &camerafwd, &cameraIdealLoc[clientNum]);
}



static void CG_ResetThirdPersonViewDamp( int clientNum )
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	if (cameraFocusAngles[clientNum].pitch > 80.0)
		cameraFocusAngles[clientNum].pitch = 80.0;
	else if (cameraFocusAngles[clientNum].pitch < -80.0)
		cameraFocusAngles[clientNum].pitch = -80.0;

	AngleVectors(&cameraFocusAngles[clientNum], &camerafwd, NULL, &cameraup);

	// Set the cameraIdealTarget
	CG_CalcIdealThirdPersonViewTarget( clientNum );

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation( clientNum );

	// Now, we just set everything to the new positions.
	VectorCopy(&cameraIdealLoc[clientNum], &cameraCurLoc[clientNum]);
	VectorCopy(&cameraIdealTarget[clientNum], &cameraCurTarget[clientNum]);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, &cameraFocusLoc[clientNum], &cameramins, &cameramaxs, &cameraCurTarget[clientNum], clientNum, GetCameraClip());
	if (trace.fraction <= 1.0)
	{
		VectorCopy(&trace.endpos, &cameraCurTarget[clientNum]);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, &cameraCurTarget[clientNum], &cameramins, &cameramaxs, &cameraCurLoc[clientNum], clientNum, GetCameraClip());
	if (trace.fraction <= 1.0)
	{
		VectorCopy(&trace.endpos, &cameraCurLoc[clientNum]);
	}

	cameraLastFrame[clientNum] = cg.time;
	cameraLastYaw[clientNum] = cameraFocusAngles[clientNum].yaw;
	cameraStiffFactor[clientNum] = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp( int clientNum )
{
	trace_t trace;
	vector3	targetdiff;
	float	dampfactor, dtime, ratio;

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget( clientNum );

	if ( cg.predictedVehicleState.hyperSpaceTime
		&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
	{//hyperspacing, no damp
		VectorCopy(&cameraIdealTarget[clientNum], &cameraCurTarget[clientNum]);
	}
	else if (cg_thirdPersonTargetDamp.value>=1.0||cg.thisFrameTeleport||cg.predictedPlayerState.m_iVehicleNum)
	{	// No damping.
		VectorCopy(&cameraIdealTarget[clientNum], &cameraCurTarget[clientNum]);
	}
	else if (cg_thirdPersonTargetDamp.value>=0.0)
	{	
		// Calculate the difference from the current position to the new one.
		VectorSubtract(&cameraIdealTarget[clientNum], &cameraCurTarget[clientNum], &targetdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-cg_thirdPersonTargetDamp.value;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame[clientNum]) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible, 
		// the ratio should be initialized into a chart ultimately.
		ratio = cg_smoothCamera.integer ? powf( dampfactor, dtime ) : Q_powf( dampfactor, dtime );
		
		// This value is how much distance is "left" from the ideal.
		VectorMA(&cameraIdealTarget[clientNum], -ratio, &targetdiff, &cameraCurTarget[clientNum]);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, &cameraFocusLoc[clientNum], &cameramins, &cameramaxs, &cameraCurTarget[clientNum], clientNum, GetCameraClip());
	if (trace.fraction < 1.0)
	{
		VectorCopy(&trace.endpos, &cameraCurTarget[clientNum]);
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}

// This can be called every interval, at the user's discretion.
extern void CG_CalcEntityLerpPositions( centity_t *cent ); //cg_ents.c
static void CG_UpdateThirdPersonCameraDamp( int clientNum )
{
	trace_t trace;
	vector3	locdiff;
	float dampfactor, dtime, ratio;

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation( clientNum );
	
	
	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor=0.0;
	if ( cg.predictedVehicleState.hyperSpaceTime
		&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
	{//hyperspacing - don't damp camera
		dampfactor = 1.0f;
	}
	else if (cg_thirdPersonCameraDamp.value != 0.0)
	{
		float pitch;
		float dFactor;

		if (!cg.predictedPlayerState.m_iVehicleNum)
		{
			dFactor = cg_thirdPersonCameraDamp.value;
		}
		else
		{
			dFactor = 1.0f;
		}

		// Note that the camera pitch has already been capped off to 89.
		pitch = Q_fabs(cameraFocusAngles[clientNum].pitch);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0;	
		dampfactor = (1.0-dFactor)*(pitch*pitch);

		dampfactor += dFactor;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor[clientNum] > 0.0f)
		{	// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0-dampfactor)*cameraStiffFactor[clientNum];
		}
	}

	if (dampfactor>=1.0||cg.thisFrameTeleport)
	{	// No damping.
		VectorCopy(&cameraIdealLoc[clientNum], &cameraCurLoc[clientNum]);
	}
	else if (dampfactor>=0.0)
	{	
		// Calculate the difference from the current position to the new one.
		VectorSubtract(&cameraIdealLoc[clientNum], &cameraCurLoc[clientNum], &locdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-dampfactor;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame[clientNum]) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible, 
		// the ratio should be initialized into a chart ultimately.
		ratio = cg_smoothCamera.integer ? powf( dampfactor, dtime ) : Q_powf( dampfactor, dtime );
		
		// This value is how much distance is "left" from the ideal.
		VectorMA(&cameraIdealLoc[clientNum], -ratio, &locdiff, &cameraCurLoc[clientNum]);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, &cameraCurTarget[clientNum], &cameramins, &cameramaxs, &cameraCurLoc[clientNum], clientNum, GetCameraClip());

	if (trace.fraction < 1.0)
	{
		if (trace.entityNum < ENTITYNUM_WORLD &&
			cg_entities[trace.entityNum].currentState.solid == SOLID_BMODEL &&
			cg_entities[trace.entityNum].currentState.eType == ET_MOVER)
		{ //get a different position for movers -rww
			centity_t *mover = &cg_entities[trace.entityNum];

			//this is absolutely hackiful, since we calc view values before we add packet ents and lerp,
			//if we hit a mover we want to update its lerp pos and force it when we do the trace against
			//it.
			if (mover->currentState.pos.trType != TR_STATIONARY &&
				mover->currentState.pos.trType != TR_LINEAR)
			{
				int curTr = mover->currentState.pos.trType;
				vector3 curTrB;

				VectorCopy(&mover->currentState.pos.trBase, &curTrB);

				//calc lerporigin for this client frame
				CG_CalcEntityLerpPositions(mover);

				//force the calc'd lerp to be the base and say we are stationary so we don't try to extrapolate
				//out further.
				mover->currentState.pos.trType = TR_STATIONARY;
				VectorCopy(&mover->lerpOrigin, &mover->currentState.pos.trBase);
				
				//retrace
				CG_Trace(&trace, &cameraCurTarget[clientNum], &cameramins, &cameramaxs, &cameraCurLoc[clientNum], clientNum, GetCameraClip());

				//copy old data back in
				mover->currentState.pos.trType = (trType_t) curTr;
				VectorCopy(&curTrB, &mover->currentState.pos.trBase);
			}
			if (trace.fraction < 1.0f)
			{ //still hit it, so take the proper trace endpos and use that.
				VectorCopy(&trace.endpos, &cameraCurLoc[clientNum]);
			}
		}
		else
		{
			VectorCopy( &trace.endpos, &cameraCurLoc[clientNum] );
		}
	}

	// Note that previously there was an upper limit to the number of physics traces that are done through the world
	// for the sake of camera collision, since it wasn't calced per frame.  Now it is calculated every frame.
	// This has the benefit that the camera is a lot smoother now (before it lerped between tested points),
	// however two full volume traces each frame is a bit scary to think about.
}




/*
===============
CG_OffsetThirdPersonView

===============
*/
extern vmCvar_t cg_thirdPersonHorzOffset;
extern qboolean BG_UnrestrainedPitchRoll( playerState_t *ps, Vehicle_t *pVeh );
static void CG_OffsetThirdPersonView( int clientNum ) 
{
	vector3 diff;
	float thirdPersonHorzOffset = cg_thirdPersonHorzOffset.value;
	float deltayaw;
	refdef_t *refdef = CG_GetRefdef();

	if (cg.snap && cg.snap->ps.m_iVehicleNum)
	{
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if (veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
		{ //override the range with what the vehicle wants it to be
			thirdPersonHorzOffset = veh->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
			if ( veh->playerState->hackingTime )
			{
				thirdPersonHorzOffset += (((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * -80.0f;
			}
		}
	}

	cameraStiffFactor[clientNum] = 0.0;

	// Set camera viewing direction.
	VectorCopy( &refdef->viewangles, &cameraFocusAngles[clientNum] );

	// if dead, look at killer
	if ( cg.snap
		&& (cg.snap->ps.eFlags2&EF2_HELD_BY_MONSTER) 
		&& cg.snap->ps.hasLookTarget
		&& cg_entities[cg.snap->ps.lookTarget].currentState.NPC_class == CLASS_RANCOR )//only possibility for now, may add Wampa and sand creature later
	{//being held
		//vector3 monsterPos, dir2Me;
		centity_t	*monster = &cg_entities[cg.snap->ps.lookTarget];
		VectorSet( &cameraFocusAngles[clientNum], 0, AngleNormalize180(monster->lerpAngles.yaw+180), 0 );
		//make the look angle the vector from his mouth to me
		/*
		VectorCopy( monster->lerpOrigin, monsterPos );
		monsterPos[2] = cg.snap->ps.origin[2];
		VectorSubtract( monsterPos, cg.snap->ps.origin, dir2Me );
		vectoangles( dir2Me, cameraFocusAngles );
		*/
	}
	else if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) 
	{
		cameraFocusAngles[clientNum].yaw = cg.snap->ps.stats[STAT_DEAD_YAW];
	}
	else
	{	// Add in the third Person Angle.
		cameraFocusAngles[clientNum].yaw += cg_thirdPersonAngle.value;
		{
			float pitchOffset = cg_thirdPersonPitchOffset.value;
			if (cg.snap && cg.snap->ps.m_iVehicleNum)
			{
				centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
				if (veh->m_pVehicle &&
					veh->m_pVehicle->m_pVehicleInfo->cameraOverride)
				{ //override the range with what the vehicle wants it to be
					if ( veh->m_pVehicle->m_pVehicleInfo->cameraPitchDependantVertOffset )
					{
						if ( cg.predictedPlayerState.viewangles.pitch > 0 )
							pitchOffset = cg.predictedPlayerState.viewangles.pitch *-0.75;
						else if ( cg.predictedPlayerState.viewangles.pitch < 0 )
							pitchOffset = cg.predictedPlayerState.viewangles.pitch *-0.75;
						else
							pitchOffset = 0;
					}
					else
						pitchOffset = veh->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
				}
			}
			//RAZTEST: swoop
			if ( /*0 &&*/ cg.predictedPlayerState.m_iVehicleNum //in a vehicle
				&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
			{
				float pitchPerc = ((90.0f-fabs(cameraFocusAngles[clientNum].roll))/90.0f);
				cameraFocusAngles[clientNum].pitch += pitchOffset*pitchPerc;
				if ( cameraFocusAngles[clientNum].roll > 0 )
					cameraFocusAngles[clientNum].yaw -= pitchOffset-(pitchOffset*pitchPerc);
				else
					cameraFocusAngles[clientNum].yaw += pitchOffset-(pitchOffset*pitchPerc);
			}
			else
			{
				cameraFocusAngles[clientNum].pitch += pitchOffset;
			}
		}
	}

	// The next thing to do is to see if we need to calculate a new camera target location.

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cameraLastFrame[clientNum] == 0 || cameraLastFrame[clientNum] > cg.time)
	{
		CG_ResetThirdPersonViewDamp( clientNum );
	}
	else
	{
		// Cap the pitch within reasonable limits
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//no clamp on pitch
			//FIXME: when pitch >= 90 or <= -90, camera rotates oddly... need to CrossProduct not just vectoangles
		}
		else
		{
#if 1
				 if (cameraFocusAngles[clientNum].pitch > 80.0)		cameraFocusAngles[clientNum].pitch = 80.0;
			else if (cameraFocusAngles[clientNum].pitch < -80.0)	cameraFocusAngles[clientNum].pitch = -80.0;
#endif
		}

		AngleVectors(&cameraFocusAngles[clientNum], &camerafwd, NULL, &cameraup);

		deltayaw = fabs(cameraFocusAngles[clientNum].yaw - cameraLastYaw[clientNum]);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabs(deltayaw - 360.0f);
		}
		cameraStiffFactor[clientNum] = deltayaw / (float)(cg.time-cameraLastFrame[clientNum]);
		if (cameraStiffFactor[clientNum] < 1.0)
			cameraStiffFactor[clientNum] = 0.0;
		else if (cameraStiffFactor[clientNum] > 2.5)
			cameraStiffFactor[clientNum] = 0.75;
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			cameraStiffFactor[clientNum] = (cameraStiffFactor[clientNum]-1.0f)*0.5f;
		}
		cameraLastYaw[clientNum] = cameraFocusAngles[clientNum].yaw;

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp( clientNum );
		CG_UpdateThirdPersonCameraDamp( clientNum );
	}

	// Now interestingly, the Quake method is to calculate a target focus point above the player, and point the camera at it.
	// We won't do that for now.

	// We must now take the angle taken from the camera target and location.
	/*VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	VectorNormalize(diff);
	vectoangles(diff, refdef->viewangles);*/
	VectorSubtract(&cameraCurTarget[clientNum], &cameraCurLoc[clientNum], &diff);
	{
		float dist = VectorNormalize(&diff);
		//under normal circumstances, should never be 0.00000 and so on.
		if ( !dist || (diff.x == 0 || diff.y == 0) )
		{//must be hitting something, need some value to calc angles, so use cam forward
			VectorCopy( &camerafwd, &diff );
		}
	}
	//RAZTEST: swoop
	if ( /*0 &&*/ cg.predictedPlayerState.m_iVehicleNum //in a vehicle
		&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
	{//FIXME: this causes camera jerkiness, need to blend the roll?
		float sav_Roll = refdef->viewangles.roll;
		vectoangles(&diff, &refdef->viewangles);
		refdef->viewangles.roll = sav_Roll;
	}
	else
	{
		vectoangles(&diff, &refdef->viewangles);
	}

	// Temp: just move the camera to the side a bit
	if ( thirdPersonHorzOffset != 0.0f )
	{
		AnglesToAxis( &refdef->viewangles, refdef->viewaxis );
		VectorMA( &cameraCurLoc[clientNum], thirdPersonHorzOffset, &refdef->viewaxis[1], &cameraCurLoc[clientNum] );
	}

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(&cameraCurLoc[clientNum], &refdef->vieworg);

	cameraLastFrame[clientNum]=cg.time;
}

void CG_GetVehicleCamPos( vector3 *camPos )
{
	refdef_t *refdef = CG_GetRefdef();
	VectorCopy( &refdef->vieworg, camPos );
}

/*
===============
CG_OffsetThirdPersonView

===============
*//*
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vector3		forward, right, up;
	vector3		view;
	vector3		focusAngles;
	trace_t		trace;
	static vector3	mins = { -4, -4, -4 };
	static vector3	maxs = { 4, 4, 4 };
	vector3		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;

	refdef->vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( refdef->viewangles, focusAngles );

	// if dead, look at killer
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		focusAngles.yaw = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		refdef->viewangles.yaw = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles.pitch > 45 ) {
		focusAngles.pitch = 45;		// don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( refdef->vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( refdef->vieworg, view );

	view[2] += 8;

	refdef->viewangles.pitch *= 0.5;

	AngleVectors( refdef->viewangles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, refdef->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, GetCameraClip());

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enogh that this is poking out

			CG_Trace( &trace, refdef->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, GetCameraClip());
			VectorCopy( trace.endpos, view );
		}
	}


	VectorCopy( view, refdef->vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, refdef->vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}
	refdef->viewangles.pitch = -180 / M_PI * atan2( focusPoint[2], focusDist );
	refdef->viewangles.yaw -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		refdef->vieworg[2] -= cg.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}*/

/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
	vector3			*origin, *angles;
	float			bob, ratio, delta, speed, f;
	vector3			predictedVelocity;
	int				timeDelta, kickTime;
	refdef_t *refdef = CG_GetRefdef();
	
	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
		return;

	origin = &refdef->vieworg;
	angles = &refdef->viewangles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles->roll = 40;
		angles->pitch = -15;
		angles->yaw = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin->z += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	kickTime = (cg.time - cg.kick_time);
	if ( kickTime < 800 )
	{//kicks are always 1 second long.  Deal with it.
		float kickPerc = 0.0f;
		if ( kickTime <= 200 )
		{//winding up
			kickPerc = kickTime/200.0f;
		}
		else
		{//returning to normal
			kickTime = 800 - kickTime;
			kickPerc = kickTime/600.0f;
		}
		VectorMA( angles, kickPerc, &cg.kick_angles, angles );
	}

	// add angles based on damage kick
	if ( cg.damageTime && cg_viewKickDamage.integer ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles->pitch += ratio * cg.v_dmg_pitch;
			angles->roll += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles->pitch += ratio * cg.v_dmg_pitch;
				angles->roll += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles.pitch += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( &cg.predictedPlayerState.velocity, &predictedVelocity );

	delta = DotProduct ( &predictedVelocity, &refdef->viewaxis[0]);
	angles->pitch += delta * cg_runPitch.value;
	
	delta = DotProduct ( &predictedVelocity, &refdef->viewaxis[1]);
	angles->roll -= delta * cg_runRoll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg.viewBob.pitch * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching
	angles->pitch += delta;
	delta = cg.bobfracsin * cg.viewBob.roll * speed;
	if (cg.predictedPlayerState.pm_flags & PMF_DUCKED)
		delta *= 3;		// crouching accentuates roll
	if (cg.bobcycle & 1)
		delta = -delta;
	angles->roll += delta;

//===================================

	// add view height
	origin->z += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		refdef->vieworg.z -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg.viewBob.up;
	if (bob > 6)
		bob = 6;
	origin->z += bob;


	// add fall height
	if ( cg.viewBob.fall )
	{
		delta = cg.time - cg.landTime;
		if ( delta < LAND_DEFLECT_TIME ) {
			f = delta / LAND_DEFLECT_TIME;
			refdef->vieworg.z += cg.landChange * f;
		} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
			delta -= LAND_DEFLECT_TIME;
			f = 1.0 - ( delta / LAND_RETURN_TIME );
			refdef->vieworg.z += cg.landChange * f;
		}
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, &cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vector3			forward, up;
 
	refdef->vieworg[2] -= NECK_LENGTH;
	AngleVectors( refdef->viewangles, forward, NULL, up );
	VectorMA( refdef->vieworg, 3, forward, refdef->vieworg );
	VectorMA( refdef->vieworg, NECK_LENGTH, up, refdef->vieworg );
	}
#endif
}

//RAZTODO: implement CG_OffsetFighterView?
#if 0
static void CG_OffsetFighterView( void )
{
	vector3 vehFwd, vehRight, vehUp, backDir;
	vector3	camOrg, camBackOrg;
	float horzOffset = cg_thirdPersonHorzOffset.value;
	float vertOffset = cg_thirdPersonVertOffset.value;
	float pitchOffset = cg_thirdPersonPitchOffset.value;
	float yawOffset = cg_thirdPersonAngle.value;
	float range = cg_thirdPersonRange.value;
	trace_t	trace;
	centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	refdef_t *refdef = CG_GetRefdef();

	AngleVectors( &refdef->viewangles, &vehFwd, &vehRight, &vehUp );

	if ( veh->m_pVehicle &&
		veh->m_pVehicle->m_pVehicleInfo->cameraOverride )
	{ //override the horizontal offset with what the vehicle wants it to be
		horzOffset = veh->m_pVehicle->m_pVehicleInfo->cameraHorzOffset;
		vertOffset = veh->m_pVehicle->m_pVehicleInfo->cameraVertOffset;
		//NOTE: no yaw offset?
		pitchOffset = veh->m_pVehicle->m_pVehicleInfo->cameraPitchOffset;
		range = veh->m_pVehicle->m_pVehicleInfo->cameraRange;
		if ( veh->playerState->hackingTime )
		{
			horzOffset += (((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * -80.0f;
			range += fabs(((float)veh->playerState->hackingTime)/MAX_STRAFE_TIME) * 100.0f;
		}
	}

	//Set camera viewing position
	VectorMA( &refdef->vieworg, horzOffset, &vehRight, &camOrg );
	VectorMA( &camOrg, vertOffset, &vehUp, &camOrg );

	//trace to that pos
	CG_Trace(&trace, &refdef->vieworg, &cameramins, &cameramaxs, &camOrg, cg.snap->ps.clientNum, GetCameraClip());
	if ( trace.fraction < 1.0 )
	{
		VectorCopy( &trace.endpos, &camOrg );
	}

	// Set camera viewing direction.
	refdef->viewangles.yaw += yawOffset;
	refdef->viewangles.pitch += pitchOffset;

	//Now bring the cam back from that pos and angles at range
	AngleVectors( &refdef->viewangles, &backDir, NULL, NULL );
	VectorScale( &backDir, -1, &backDir );

	VectorMA( &camOrg, range, &backDir, &camBackOrg );

	//trace to that pos
	CG_Trace(&trace, &camOrg, &cameramins, &cameramaxs, &camBackOrg, cg.snap->ps.clientNum, GetCameraClip());
	VectorCopy( &trace.endpos, &camOrg );

	//FIXME: do we need to smooth the org?
	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(&camOrg, &refdef->vieworg);
}
#endif

void CG_ZoomDown_f( void ) { 
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void CG_ZoomUp_f( void ) { 
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}



/*
====================
CG_CalcFovFromX

Calcs Y FOV from given X FOV
====================
*/
qboolean CG_CalcFOVFromX( float fov_x ) 
{
	float	x;
//	float	phase;
//	float	v;
//	int		contents;
	float	fov_y;
	qboolean	inwater;
	refdef_t *refdef = CG_GetRefdef();

	if ( cg_fovAspectAdjust.integer ) {
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight;
		const float desiredFov = fov_x;

		fov_x = atan( tan( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
	}

	x = refdef->width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( refdef->height, x );
	fov_y = fov_y * 360 / M_PI;

	// there's a problem with this, it only takes the leafbrushes into account, not the entity brushes,
	//	so if you give slime/water etc properties to a func_door area brush in order to move the whole water 
	//	level up/down this doesn't take into account the door position, so warps the view the whole time
	//	whether the water is up or not. Fortunately there's only one slime area in Trek that you can be under,
	//	so lose it...
#if 0
/*
	// warp if underwater
	contents = CG_PointContents( refdef->vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}
*/
#else
	inwater = qfalse;
#endif

	// set it
	refdef->fov_x = fov_x;
	refdef->fov_y = fov_y;

	return (inwater);
}

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float zoomFov; //this has to be global client-side

static int CG_CalcFov( void ) {
	float	x;
	float	phase;
	float	v;
	float	fov_x, fov_y;
	float	f;
	int		inwater;
	//[TrueView]
	float cgFov;
	//float	cgFov = cg_fov.value;
	refdef_t *refdef = CG_GetRefdef();

	if(!cg.renderingThirdPerson && (cg_trueGuns.integer || cg.predictedPlayerState.weapon == WP_SABER
		|| cg.predictedPlayerState.weapon == WP_MELEE) 
		&& cg_trueFOV.value && (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		&& (cg.predictedPlayerState.pm_type != PM_INTERMISSION))
	{
		cgFov = cg_trueFOV.value;
	}
	else
	{
		cgFov = cg_fov.value;
	}
	//[/TrueView]

	if (cgFov < 1)
	{
		cgFov = 1;
	}
	//[TrueView]
	//Allow larger Fields of View
	if (cgFov > 150)
	{
		cgFov = 150;
	}
	/*
	if (cgFov > 97)
	{
		cgFov = 97;
	}
	*/
	//[/TrueView]

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 80;//90;
	} else {
		// user selectable
		/* Woops....
		if ( cgs.dmflags & DF_FIXED_FOV ) {
			// dmflag to prevent wide fov for all clients
			fov_x = 80;//90;
		} else */{
			fov_x = cgFov;
			if ( fov_x < 1 ) {
				fov_x = 1;
			} else if ( fov_x > 160 ) {
				fov_x = 160;
			}
		}

		if (cg.predictedPlayerState.zoomMode == 2)
		{ //binoculars
			if (zoomFov > 40.0f)
			{
				zoomFov -= cg.frametime * 0.075f;

				if (zoomFov < 40.0f)
				{
					zoomFov = 40.0f;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
			}

			fov_x = zoomFov;
		}
		else if (cg.predictedPlayerState.zoomMode)
		{
			if (!cg.predictedPlayerState.zoomLocked)
			{
				if (zoomFov > 50)
				{ //Now starting out at nearly half zoomed in
					zoomFov = 50;
				}
				zoomFov -= cg.frametime * 0.035f;//0.075f;

				if (zoomFov < MAX_ZOOM_FOV)
				{
					zoomFov = MAX_ZOOM_FOV;
				}
				else if (zoomFov > cgFov)
				{
					zoomFov = cgFov;
				}
				else
				{	// Still zooming
					static int zoomSoundTime = 0;

					if (zoomSoundTime < cg.time || zoomSoundTime > cg.time + 10000)
					{
						trap->S_StartSound(&refdef->vieworg, ENTITYNUM_WORLD, CHAN_LOCAL, cgs.media.disruptorZoomLoop);
						zoomSoundTime = cg.time + 300;
					}
				}
			}

			if (zoomFov < MAX_ZOOM_FOV)
			{
				zoomFov = 50;		// hack to fix zoom during vid restart
			}
			fov_x = zoomFov;
		}
		else 
		{
			zoomFov = 80;

			f = ( cg.time - cg.predictedPlayerState.zoomTime ) / ZOOM_OUT_TIME;
			if ( f > 1.0 ) 
			{
				fov_x = fov_x;
			} 
			else 
			{
				fov_x = cg.predictedPlayerState.zoomFov + f * ( fov_x - cg.predictedPlayerState.zoomFov );
			}
		}
	}

	if ( cg_fovAspectAdjust.integer ) {
		// Based on LordHavoc's code for Darkplaces
		// http://www.quakeworld.nu/forum/topic/53/what-does-your-qw-look-like/page/30
		const float baseAspect = 0.75f; // 3/4
		const float aspect = (float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight;
		const float desiredFov = fov_x;

		fov_x = atan( tan( desiredFov*M_PI / 360.0f ) * baseAspect*aspect )*360.0f / M_PI;
	}

	x = refdef->width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( refdef->height, x );
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	refdef->viewContents = CG_PointContents( &refdef->vieworg, -1 );
	if ( refdef->viewContents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}

	// set it
	refdef->fov_x = fov_x;
	refdef->fov_y = fov_y;

	if (cg.predictedPlayerState.zoomMode)
	{
		cg.zoomSensitivity = zoomFov/cgFov;
	}
	else if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = refdef->fov_y / 75.0;
	}

	return inwater;
}


/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void ) 
{
	int			t;
	int			maxTime;
	refEntity_t		ent;
	refdef_t *refdef = CG_GetRefdef();

	if ( !cg.damageValue ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( &refdef->vieworg, 8, &refdef->viewaxis[0], &ent.origin );
	VectorMA( &ent.origin, cg.damageX * -8, &refdef->viewaxis[1], &ent.origin );
	VectorMA( &ent.origin, cg.damageY * 8, &refdef->viewaxis[2], &ent.origin );

	ent.radius = cg.damageValue * 3 * ( 1.0 - ((float)t / maxTime) );

	if (cg.snap->ps.damageType == 0)
	{ //pure health
		ent.customShader = cgs.media.viewPainShader;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else if (cg.snap->ps.damageType == 1)
	{ //pure shields
		ent.customShader = cgs.media.viewPainShader_Shields;
		ent.shaderRGBA[0] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	else
	{ //shields and health
		ent.customShader = cgs.media.viewPainShader_ShieldsAndHealth;
		ent.shaderRGBA[0] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[1] = 180 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[2] = 50 * ( 1.0 - ((float)t / maxTime) );
		ent.shaderRGBA[3] = 255;
	}
	SE_R_AddRefEntityToScene( &ent, MAX_CLIENTS );
}

int cg_actionCamLastTime = 0;
vector3 cg_actionCamLastPos[MAX_CLIENTS];

//action cam routine -rww
static qboolean CG_ThirdPersonActionCam(int clientNum)
{
    centity_t *cent = &cg_entities[clientNum];
	clientInfo_t *ci = &cgs.clientinfo[clientNum];
	trace_t tr;
	vector3 positionDir;
	vector3 desiredAngles;
	vector3 desiredPos;
	vector3 v;
	const float smoothFactor = 0.1f*timescale.value;
	int i;
	refdef_t *refdef = CG_GetRefdef();

	if (!cent->ghoul2)
	{ //if we don't have a g2 instance this frame for whatever reason then do nothing
		return qfalse;
	}

	if (cent->currentState.weapon != WP_SABER)
	{ //just being safe, should not ever happen
		return qfalse;
	}

	if ((cg.time - ci->saber[0].blade[0].trail.lastTime) > 300)
	{ //too long since we last got the blade position
		return qfalse;
	}

	//get direction from base to ent origin
	VectorSubtract(&ci->saber[0].blade[0].trail.base, &cent->lerpOrigin, &positionDir);
	VectorNormalize(&positionDir);

	//position the cam based on the direction and saber position
	VectorMA(&cent->lerpOrigin, cg_thirdPersonRange.value*2, &positionDir, &desiredPos);

	//trace to the desired pos to see how far that way we can actually go before we hit something
	//the endpos will be valid for our desiredpos no matter what
	CG_Trace(&tr, &cent->lerpOrigin, NULL, NULL, &desiredPos, cent->currentState.number, MASK_SOLID);
	VectorCopy(&tr.endpos, &desiredPos);

	if ((cg.time - cg_actionCamLastTime) > 300)
	{
		//do a third person offset first and grab the initial point from that
		CG_OffsetThirdPersonView( clientNum );
		VectorCopy(&refdef->vieworg, &cg_actionCamLastPos[clientNum]);
	}

	cg_actionCamLastTime = cg.time;

	//lerp the vieworg to the desired pos from the last valid
	VectorSubtract(&desiredPos, &cg_actionCamLastPos[clientNum], &v);
	
	if (VectorLength(&v) > 64.0f)
	{ //don't bother moving yet if not far from the last pos
		for (i = 0; i < 3; i++)
		{
			cg_actionCamLastPos[clientNum].data[i] = (cg_actionCamLastPos[clientNum].data[i] + (v.data[i]*smoothFactor));
			refdef->vieworg.data[i] = cg_actionCamLastPos[clientNum].data[i];
		}
	}
	else
	{
		VectorCopy(&cg_actionCamLastPos[clientNum], &refdef->vieworg);
	}

	//Make sure the point is alright
	CG_Trace(&tr, &cent->lerpOrigin, NULL, NULL, &refdef->vieworg, cent->currentState.number, MASK_SOLID);
	VectorCopy(&tr.endpos, &refdef->vieworg);

	VectorSubtract(&cent->lerpOrigin, &refdef->vieworg, &positionDir);
	vectoangles(&positionDir, &desiredAngles);

	//just set the angles for now
	VectorCopy(&desiredAngles, &refdef->viewangles);
	return qtrue;
}

vector3	cg_lastTurretViewAngles={0};
qboolean CG_CheckPassengerTurretView( void )
{
	refdef_t *refdef = CG_GetRefdef();
	if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
		&& cg.predictedPlayerState.generic1 )//as a passenger
	{//passenger in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if ( vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo 
			&& vehCent->m_pVehicle->m_pVehicleInfo->maxPassengers )
		{//a vehicle capable of carrying passengers
			int turretNum;
			for ( turretNum = 0; turretNum < MAX_VEHICLE_TURRETS; turretNum++ )
			{
				if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iAmmoMax )
				{// valid turret
					if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].passengerNum == cg.predictedPlayerState.generic1 )
					{//I control this turret
						int boltIndex = -1;
						qboolean hackPosAndAngle = qfalse;
						if ( vehCent->m_pVehicle->m_iGunnerViewTag[turretNum] != -1 )
						{
							boltIndex = vehCent->m_pVehicle->m_iGunnerViewTag[turretNum];
						}
						else
						{//crap... guess?
							hackPosAndAngle = qtrue;
							if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].yawBone )
							{
								boltIndex = trap->G2API_AddBolt( vehCent->ghoul2, 0, vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].yawBone );
							}
							else if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].pitchBone )
							{
								boltIndex = trap->G2API_AddBolt( vehCent->ghoul2, 0, vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].pitchBone );
							}
							else
							{//well, no way of knowing, so screw it
								return qfalse;
							}
						}
						if ( boltIndex != -1 )
						{
							mdxaBone_t boltMatrix;
							vector3 fwd, up;
							trap->G2API_GetBoltMatrix_NoRecNoRot(vehCent->ghoul2, 0, boltIndex, &boltMatrix, &vehCent->lerpAngles,
								&vehCent->lerpOrigin, cg.time, NULL, &vehCent->modelScale);
							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, &refdef->vieworg);
							if ( hackPosAndAngle )
							{
								//FIXME: these are assumptions, externalize?  BETTER YET: give me a controller view bolt/tag for each turret
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, &fwd);
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, &up);
								VectorMA( &refdef->vieworg, 8.0f, &fwd, &refdef->vieworg );
								VectorMA( &refdef->vieworg, 4.0f, &up, &refdef->vieworg );
							}
							else
							{
								BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, &fwd);
							}
							{
								vector3	newAngles, deltaAngles;
								vectoangles( &fwd, &newAngles );
								AnglesSubtract( &newAngles, &cg_lastTurretViewAngles, &deltaAngles );
								VectorMA( &cg_lastTurretViewAngles, 0.5f*(float)cg.frametime/100.0f, &deltaAngles, &refdef->viewangles );
							}
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}
/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
void CG_EmplacedView(vector3 *angles);
static int CG_CalcViewValues( int clientNum ) {
	qboolean manningTurret = qfalse;
	playerState_t	*ps;
	refdef_t *refdef = CG_GetRefdef();

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( refdef->text[0], "Park Ranger", sizeof(refdef->text[0]) );
	// Q_strncpyz( refdef->text[1], "19", sizeof(refdef->text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vector3 origin, angles;
		if (trap->getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, refdef->vieworg);
			angles.roll = 0;
			VectorCopy(angles, refdef->viewangles);
			AnglesToAxis( refdef->viewangles, refdef->viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( &ps->origin, &refdef->vieworg );
		VectorCopy( &ps->viewangles, &refdef->viewangles );
		AnglesToAxis( &refdef->viewangles, refdef->viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabsf( sinf( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrtf( ps->velocity.x*ps->velocity.x + ps->velocity.y*ps->velocity.y );

	if (cg.xyspeed > 270)
	{
		cg.xyspeed = 270;
	}

	manningTurret = CG_CheckPassengerTurretView();
	if ( !manningTurret )
	{//not manning a turret on a vehicle
		VectorCopy( &ps->origin, &refdef->vieworg );
#ifdef VEH_CONTROL_SCHEME_4
		if ( cg.predictedPlayerState.m_iVehicleNum )//in a vehicle
		{
			Vehicle_t *pVeh = cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle;
			if ( BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, pVeh ) )//can roll/pitch without restriction
			{//use the vehicle's viewangles to render view!
				VectorCopy( cg.predictedVehicleState.viewangles, refdef->viewangles );
			}
			else if ( pVeh //valid vehicle data pointer
				&& pVeh->m_pVehicleInfo//valid vehicle info
				&& pVeh->m_pVehicleInfo->type == VH_FIGHTER )//fighter
			{
				VectorCopy( cg.predictedVehicleState.viewangles, refdef->viewangles );
				refdef->viewangles.pitch = AngleNormalize180( refdef->viewangles.pitch );
			}
			else
			{
				VectorCopy( ps->viewangles, refdef->viewangles );
			}
		}
#else// VEH_CONTROL_SCHEME_4
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//use the vehicle's viewangles to render view!
			VectorCopy( &cg.predictedVehicleState.viewangles, &refdef->viewangles );
		}
#endif// VEH_CONTROL_SCHEME_4
		else
		{
			VectorCopy( &ps->viewangles, &refdef->viewangles );
		}
	}
	VectorCopy( &refdef->viewangles, &cg_lastTurretViewAngles );

	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}

	//Raz: Pmove smoothing
	if ( cg_smoothClients.integer ) {
		int			cmdNum;
		usercmd_t	cmd;

		cmdNum = trap->GetCurrentCmdNumber() - CMD_BACKUP + 1;
		trap->GetUserCmd( cmdNum, &cmd );
		if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) && !cg.demoPlayback && cmd.serverTime <= cg.snap->ps.commandTime )
			VectorMA( &refdef->vieworg, (cg.time - ps->commandTime) * 0.001, &ps->velocity, &refdef->vieworg );
	}

	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( &refdef->vieworg, f, &cg.predictedError, &refdef->vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if (cg.snap->ps.weapon == WP_EMPLACED_GUN &&
		cg.snap->ps.emplacedIndex)
	{ //constrain the view properly for emplaced guns
		CG_EmplacedView(&cg_entities[cg.snap->ps.emplacedIndex].currentState.angles);
	}

	if ( !manningTurret )
	{
#ifndef RAZTEST
		if ( cg.predictedPlayerState.m_iVehicleNum //in a vehicle
			&& BG_UnrestrainedPitchRoll( &cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle ) )//can roll/pitch without restriction
		{//use the vehicle's viewangles to render view!
			CG_OffsetFighterView();
		}
		else if ( cg.renderingThirdPerson ) {
#else
		if ( cg.renderingThirdPerson ) {
#endif
			// back away from character
			if (cg_thirdPersonSpecialCam.integer &&
				BG_SaberInSpecial(cg.predictedPlayerState.saberMove/*cg.snap->ps.saberMove*/))
			{ //the action cam
				if (!CG_ThirdPersonActionCam( clientNum ))
				{ //couldn't do it for whatever reason, resort back to third person then
					CG_OffsetThirdPersonView( clientNum );
				}
			}
			else
			{
				CG_OffsetThirdPersonView( clientNum );
			}
		} else {
			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();
		}
	}

	// position eye relative to origin
	AnglesToAxis( &refdef->viewangles, refdef->viewaxis );

	if ( cg.hyperspace ) {
		refdef->rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			//trap->S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
==============
CG_DrawSkyBoxPortal
==============
*/
extern qboolean cg_skyOri;
extern vector3 cg_skyOriPos;
extern float cg_skyOriScale;
extern qboolean cg_noFogOutsidePortal;
void CG_DrawSkyBoxPortal(const char *cstr)
{
	refdef_t backuprefdef;
	float fov_x;
	float fov_y;
	float x;
	char *token;
	float f = 0;
	refdef_t *refdef = CG_GetRefdef();

	//backuprefdef = cg.refdef;
	memcpy( &backuprefdef, refdef, sizeof( backuprefdef ) );

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
		return;
	}
	refdef->vieworg.x = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
		return;
	}
	refdef->vieworg.y = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
		return;
	}
	refdef->vieworg.z = atof(token);

	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0]) 
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring\n");
		return;
	}
	fov_x = atoi(token);

	if (!fov_x)
	{
		//[TrueView]
		if(!cg.renderingThirdPerson && (cg_trueGuns.integer || cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE)
			&& cg_trueFOV.value
			&& (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
			&& (cg.predictedPlayerState.pm_type != PM_INTERMISSION))
		{
			fov_x = cg_trueFOV.value;
		}
		else
		{
			fov_x = cg_fov.value;
		}
		//fov_x = cg_fov.value;
		//[/TrueView]
	}

	// setup fog the first time, ignore this part of the configstring after that
	token = COM_ParseExt(&cstr, qfalse);
	if (!token || !token[0])
	{
		trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog state\n" );
	}
	else 
	{
		if(atoi(token))
		{	// this camera has fog
			token = COM_ParseExt( &cstr, qfalse );
			if ( !VALIDSTRING( token ) )
				trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[0]\n" );

			token = COM_ParseExt( &cstr, qfalse );
			if ( !VALIDSTRING( token ) )
				trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[1]\n" );

			token = COM_ParseExt( &cstr, qfalse );
			if ( !VALIDSTRING( token ) )
				trap->Error( ERR_DROP, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[2]\n" );

			token = COM_ParseExt( &cstr, qfalse );
			token = COM_ParseExt( &cstr, qfalse );
		}
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		// if in intermission, use a fixed value
		//[TrueView]
		if(!cg.renderingThirdPerson && (cg_trueGuns.integer || cg.predictedPlayerState.weapon == WP_SABER
		|| cg.predictedPlayerState.weapon == WP_MELEE) && cg_trueFOV.value)
		{
			fov_x = cg_trueFOV.value;
		}
		else
		{
			fov_x = cg_fov.value;
		}
		//fov_x = cg_fov.value;
		//[/TrueView]
	}
	else
	{
		//[TrueView]
		if(!cg.renderingThirdPerson && (cg_trueGuns.integer || cg.predictedPlayerState.weapon == WP_SABER
		|| cg.predictedPlayerState.weapon == WP_MELEE) && cg_trueFOV.value 
		&& (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		&& (cg.predictedPlayerState.pm_type != PM_INTERMISSION))
		{
			fov_x = cg_trueFOV.value;
		}
		else
		{
			fov_x = cg_fov.value;
		}
		//fov_x = cg_fov.value;
		//[/TrueView]
		if ( fov_x < 1 ) 
		{
			fov_x = 1;
		}
		else if ( fov_x > 160 )
		{
			fov_x = 160;
		}

		if (cg.predictedPlayerState.zoomMode)
		{
			fov_x = zoomFov;
		}

		// do smooth transitions for zooming
		if (cg.predictedPlayerState.zoomMode)
		{ //zoomed/zooming in
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f > 1.0 ) {
				fov_x = zoomFov;
			} else {
				fov_x = fov_x + f * ( zoomFov - fov_x );
			}
		}
		else
		{ //zooming out
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_OUT_TIME;
			if ( f > 1.0 ) {
				fov_x = fov_x;
			} else {
				fov_x = zoomFov + f * ( fov_x - zoomFov);
			}
		}
	}

	//RAZFIXME: skyportal FOV
	x = refdef->width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( refdef->height, x );
	fov_y = fov_y * 360 / M_PI;

	refdef->fov_x = fov_x;
	refdef->fov_y = fov_y;
	
	refdef->rdflags |= RDF_SKYBOXPORTAL;
	refdef->rdflags |= RDF_DRAWSKYBOX;

	refdef->time = cg.time;

	if ( !cg.hyperspace) 
	{ //rww - also had to add this to add effects being rendered in portal sky areas properly.
		trap->FX_AddScheduledEffects(qtrue);
	}

	CG_AddPacketEntities(qtrue); //rww - There was no proper way to put real entities inside the portal view before.
									//This will put specially flagged entities in the render.

	if (cg_skyOri)
	{ //ok, we want to orient the sky refdef vieworg based on the normal vieworg's relation to the ori pos
		vector3 dif;

		VectorSubtract(&backuprefdef.vieworg, &cg_skyOriPos, &dif);
		VectorScale(&dif, cg_skyOriScale, &dif);
		VectorAdd(&refdef->vieworg, &dif, &refdef->vieworg);
	}

	if (cg_noFogOutsidePortal)
	{ //make sure no fog flag is stripped first, and make sure it is set on the normal refdef
		refdef->rdflags &= ~RDF_NOFOG;
		backuprefdef.rdflags |= RDF_NOFOG;
	}

	// draw the skybox
	trap->R_RenderScene( refdef );

	//cg.refdef = backuprefdef;
	memcpy( refdef, &backuprefdef, sizeof( backuprefdef ) );
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap->S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}

void CG_UpdateSoundTrackers()
{
	int num;
	centity_t *cent;
	refdef_t *refdef = CG_GetRefdef();

	for ( num = 0 ; num < ENTITYNUM_NONE ; num++ )
	{
		cent = &cg_entities[num];

		if (cent && (cent->currentState.eFlags & EF_SOUNDTRACKER) && cent->currentState.number == num)
			//make sure the thing is valid at least.
		{ //keep sound for this entity updated in accordance with its attached entity at all times
			if (cg.snap && cent->currentState.trickedentindex == cg.snap->ps.clientNum)
			{ //this is actually the player, so center the sound origin right on top of us
				VectorCopy(&refdef->vieworg, &cent->lerpOrigin);
				trap->S_UpdateEntityPosition( cent->currentState.number, &cent->lerpOrigin );
			}
			else
			{
				trap->S_UpdateEntityPosition( cent->currentState.number, &cg_entities[cent->currentState.trickedentindex].lerpOrigin );
			}
		}

		if (cent->currentState.number == num)
		{
			//update all looping sounds..
			CG_S_UpdateLoopingSounds(num);
		}
	}
}

//=========================================================================

/*
================================
Screen Effect stuff starts here
================================
*/
#define	CAMERA_DEFAULT_FOV			90.0f
#define MAX_SHAKE_INTENSITY			16.0f

cgscreffects_t cgScreenEffects;

void CG_SE_UpdateShake( vector3 *origin, vector3 *angles )
{
	vector3	moveDir;
	float	intensity_scale, intensity;
	int		i;

	if ( cgScreenEffects.shake_duration <= 0 )
		return;

	if ( cg.time > ( cgScreenEffects.shake_start + cgScreenEffects.shake_duration ) )
	{
		cgScreenEffects.shake_intensity = 0;
		cgScreenEffects.shake_duration = 0;
		cgScreenEffects.shake_start = 0;
		return;
	}

	cgScreenEffects.FOV = CAMERA_DEFAULT_FOV;
	cgScreenEffects.FOV2 = CAMERA_DEFAULT_FOV;

	//intensity_scale now also takes into account FOV with 90.0 as normal
	intensity_scale = 1.0f - ( (float) ( cg.time - cgScreenEffects.shake_start ) / (float) cgScreenEffects.shake_duration ) * (((cgScreenEffects.FOV+cgScreenEffects.FOV2)/2.0f)/90.0f);

	intensity = cgScreenEffects.shake_intensity * intensity_scale;

	for ( i = 0; i < 3; i++ )
	{
		moveDir.data[i] = ( crandom() * intensity );
	}

	//Move the camera
	VectorAdd( origin, &moveDir, origin );

	for ( i=0; i < 2; i++ ) // Don't do ROLL
		moveDir.data[i] = ( crandom() * intensity );

	//Move the angles
	VectorAdd( angles, &moveDir, angles );
}

void CG_SE_UpdateMusic(void)
{
	if (cgScreenEffects.music_volume_multiplier < 0.1)
	{
		cgScreenEffects.music_volume_multiplier = 1.0;
		return;
	}

	if (cgScreenEffects.music_volume_time < cg.time)
	{
		if (cgScreenEffects.music_volume_multiplier != 1.0 || cgScreenEffects.music_volume_set)
		{
			char musMultStr[512];

			cgScreenEffects.music_volume_multiplier += 0.1f;
			if (cgScreenEffects.music_volume_multiplier > 1.0)
			{
				cgScreenEffects.music_volume_multiplier = 1.0;
			}

			Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
			trap->Cvar_Set("s_musicMult", musMultStr);

			if (cgScreenEffects.music_volume_multiplier == 1.0)
			{
				cgScreenEffects.music_volume_set = qfalse;
			}
			else
			{
				cgScreenEffects.music_volume_time = cg.time + 200;
			}
		}

		return;
	}

	if (!cgScreenEffects.music_volume_set)
	{ //if the volume_time is >= cg.time, we should have a volume multiplier set
		char musMultStr[512];

		Com_sprintf(musMultStr, sizeof(musMultStr), "%f", cgScreenEffects.music_volume_multiplier);
		trap->Cvar_Set("s_musicMult", musMultStr);
		cgScreenEffects.music_volume_set = qtrue;
	}
}

/*
=================
CG_CalcScreenEffects

Currently just for screen shaking (and music volume management)
=================
*/
void CG_CalcScreenEffects(void)
{
	refdef_t *refdef = CG_GetRefdef();
	if ( cg_viewShake.integer )
		CG_SE_UpdateShake( &refdef->vieworg, &refdef->viewangles );
	CG_SE_UpdateMusic();
}

void CGCam_Shake( float intensity, int duration )
{
	if ( intensity > MAX_SHAKE_INTENSITY )
		intensity = MAX_SHAKE_INTENSITY;

	cgScreenEffects.shake_intensity = intensity;
	cgScreenEffects.shake_duration = duration;
	

	cgScreenEffects.shake_start = cg.time;
}

void CG_DoCameraShake( vector3 *origin, float intensity, int radius, int time )
{
	//FIXME: When exactly is the vieworg calculated in relation to the rest of the frame?s

	vector3	dir;
	float	dist, intensityScale;
	float	realIntensity;
	refdef_t *refdef = CG_GetRefdef();

	VectorSubtract( &refdef->vieworg, origin, &dir );
	dist = VectorNormalize( &dir );

	//Use the dir to add kick to the explosion

	if ( dist > radius )
		return;

	intensityScale = 1 - ( dist / (float) radius );
	realIntensity = intensity * intensityScale;

	CGCam_Shake( realIntensity, time );
}

void CGCam_SetMusicMult( float multiplier, int duration )
{
	if (multiplier < 0.1f)
	{
		multiplier = 0.1f;
	}

	if (multiplier > 1.0f)
	{
		multiplier = 1.0f;
	}

	cgScreenEffects.music_volume_multiplier = multiplier;
	cgScreenEffects.music_volume_time = cg.time + duration;
	cgScreenEffects.music_volume_set = qfalse;
}

/*
================================
Screen Effect stuff ends here
================================
*/

/*
=================
CG_EmplacedView

Keep view reasonably constrained in relation to gun -rww
=================
*/
int BG_EmplacedView(vector3 *baseAngles, vector3 *angles, float *newYaw, float constraint);

void CG_EmplacedView(vector3 *angles)
{
	float yaw;
	int doOverride;
	refdef_t *refdef = CG_GetRefdef();
	
	doOverride = BG_EmplacedView(&refdef->viewangles, angles, &yaw, cg_entities[cg.snap->ps.emplacedIndex].currentState.origin2.x);
	
	if (doOverride)
	{
		refdef->viewangles.yaw = yaw;
		AnglesToAxis(&refdef->viewangles, refdef->viewaxis);

	//	if (doOverride == 2)
	//		trap->SetClientForceAngle(cg.time + 5000, refdef->viewangles);
	}

	//we want to constrain the predicted player state viewangles as well
	doOverride = BG_EmplacedView(&cg.predictedPlayerState.viewangles, angles, &yaw, cg_entities[cg.snap->ps.emplacedIndex].currentState.origin2.x);
	if (doOverride)
        cg.predictedPlayerState.viewangles.yaw = yaw;
}

//specially add cent's for automap
static void CG_AddRefentForAutoMap(centity_t *cent)
{
	refEntity_t ent;
	vector3 flat;

	if (cent->currentState.eFlags & EF_NODRAW)
	{
		return;
	}

	memset(&ent, 0, sizeof(refEntity_t));
	ent.reType = RT_MODEL;

	VectorCopy(&cent->lerpAngles, &flat);
	flat.pitch = flat.roll = 0.0f;

	VectorCopy(&cent->lerpOrigin, &ent.origin);
	VectorCopy(&flat, &ent.angles);
	AnglesToAxis(&flat, ent.axis);

	if (cent->ghoul2 &&
		(cent->currentState.eType == ET_PLAYER ||
		cent->currentState.eType == ET_NPC ||
		cent->currentState.modelGhoul2))
	{ //using a ghoul2 model
		ent.ghoul2 = cent->ghoul2;
		ent.radius = cent->currentState.g2radius;

		if (!ent.radius)
		{
			ent.radius = 64.0f;
		}
	}
	else
	{ //then assume a standard indexed model
		ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	}

	SE_R_AddRefEntityToScene(&ent, MAX_CLIENTS);
}

//add all entities that would be on the radar
void CG_AddRadarAutomapEnts(void)
{
	int i = 0;

	//first add yourself
//	CG_AddRefentForAutoMap(&cg_entities[cg.predictedPlayerState.clientNum]);

	for ( i=0; i<cgs.maxclients; i++ )
		CG_AddRefentForAutoMap( &cg_entities[i] );

	for ( i=0; i<cg.radarEntityCount; i++ )
		CG_AddRefentForAutoMap(&cg_entities[cg.radarEntities[i]]);
}

/*
================
CG_DrawAutoMap

Draws the automap scene. -rww
================
*/
float cg_autoMapZoom = 1024.0f;
float cg_autoMapZoomMainOffset = 0.0f;
//vector3 cg_autoMapAngle = {90.0f, 0.0f, 0.0f};
autoMapInput_t cg_autoMapInput;
int cg_autoMapInputTime = 0;
#define	SIDEFRAME_WIDTH			16
#define	SIDEFRAME_HEIGHT		32
void CG_DrawAutoMap(void)
{
	clientInfo_t	*local;
	refdef_t		refdef;
	trace_t			tr;
	vector3			fwd;
	vector3			playerMins, playerMaxs;
	int				vWidth, vHeight;
	float			hScale, vScale;
	float			x, y, w, h;

	if (!r_autoMap.integer)
	{ //don't do anything then
		return;
	}

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{ //don't show when dead
		return;
	}

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR )
	{ //don't show when spec
		return;
	}

	local = &cgs.clientinfo[ cg.predictedPlayerState.clientNum ];
	if ( !local->infoValid )
	{ //don't show if bad ci
		return;
	}

	if (cgs.gametype < GT_TEAM)
	{ //don't show in non-team gametypes
	//	return;
	}

#if 0
	if (cg_autoMapInputTime >= cg.time)
	{
		if (cg_autoMapInput.up)
		{
			cg_autoMapZoom -= cg_autoMapInput.up;
			if (cg_autoMapZoom < cg_autoMapZoomMainOffset+64.0f)
			{
				cg_autoMapZoom = cg_autoMapZoomMainOffset+64.0f;
			}
		}

		if (cg_autoMapInput.down)
		{
			cg_autoMapZoom += cg_autoMapInput.down;
			if (cg_autoMapZoom > cg_autoMapZoomMainOffset+4096.0f)
			{
				cg_autoMapZoom = cg_autoMapZoomMainOffset+4096.0f;
			}
		}

		if (cg_autoMapInput.yaw)
		{
			cg_autoMapAngle.yaw += cg_autoMapInput.yaw;
		}

		if (cg_autoMapInput.pitch)
		{
			cg_autoMapAngle.pitch += cg_autoMapInput.pitch;
		}

		if (cg_autoMapInput.goToDefaults)
		{
			cg_autoMapZoom = 512.0f;
			VectorSet(cg_autoMapAngle, 90.0f, 0.0f, 0.0f);
		}
	}
#endif

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = (RDF_NOWORLDMODEL|RDF_AUTOMAP);

	VectorCopy(&cg.predictedPlayerState.origin, &refdef.vieworg);
	VectorCopy(&cg.automapAngle, &refdef.viewangles);
	
	//scale out in the direction of the view angles base on the zoom factor
	AngleVectors(&refdef.viewangles, &fwd, 0, 0);
	VectorMA(&refdef.vieworg, -r_autoMapZoom.value, &fwd, &refdef.vieworg);

	AnglesToAxis(&refdef.viewangles, refdef.viewaxis);

	refdef.fov_x = r_autoMapFov.value;
	refdef.fov_y = r_autoMapFov.value;

	//guess this doesn't need to be done every frame, but eh
	trap->R_GetRealRes(&vWidth, &vHeight);

	//set scaling values so that the 640x480 will result at 1.0/1.0
	hScale = vWidth/640.0f;
	vScale = vHeight/480.0f;

	x = r_autoMapX.value;
	y = r_autoMapY.value;
	w = r_autoMapW.value;
	h = r_autoMapH.value;

	refdef.x = x*hScale;
	refdef.y = y*vScale;
	refdef.width = w*hScale;
	refdef.height = h*vScale;

	CG_DrawPic(x-SIDEFRAME_WIDTH, y, SIDEFRAME_WIDTH, h, cgs.media.wireframeAutomapFrame_left);
	CG_DrawPic(x+w, y, SIDEFRAME_WIDTH, h, cgs.media.wireframeAutomapFrame_right);
	CG_DrawPic(x-SIDEFRAME_WIDTH, y-SIDEFRAME_HEIGHT, w+(SIDEFRAME_WIDTH*2), SIDEFRAME_HEIGHT, cgs.media.wireframeAutomapFrame_top);
	CG_DrawPic(x-SIDEFRAME_WIDTH, y+h, w+(SIDEFRAME_WIDTH*2), SIDEFRAME_HEIGHT, cgs.media.wireframeAutomapFrame_bottom);

	refdef.time = cg.time;

	trap->R_ClearScene();
	CG_AddRadarAutomapEnts();

	if ( (cg.predictedPlayerState.m_iVehicleNum &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].currentState.eType == ET_NPC &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].currentState.NPC_class == CLASS_VEHICLE &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle &&
		cg_entities[cg.predictedPlayerState.m_iVehicleNum].m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		|| r_autoMapAdjustHeight.integer )
	{ //constantly adjust to current height
		trap->R_AutomapElevationAdjustment(cg.predictedPlayerState.origin.z);
	}
	else
	{
		//Trace down and set the ground elevation as the main automap elevation point
		VectorSet(&playerMins, -15, -15, DEFAULT_MINS_2);
		VectorSet(&playerMaxs, 15, 15, DEFAULT_MAXS_2);

		VectorCopy(&cg.predictedPlayerState.origin, &fwd);
		fwd.z -= 4096.0f;
		CG_Trace(&tr, &cg.predictedPlayerState.origin, &playerMins, &playerMaxs, &fwd, cg.predictedPlayerState.clientNum, MASK_SOLID);

		if (!tr.startsolid && !tr.allsolid)
		{
			trap->R_AutomapElevationAdjustment(tr.endpos.z);
		}
	}
	trap->R_RenderScene( &refdef );
}

float altViewX=0.0f, altViewY=0.0f;
void CG_DrawAltView(int clientNum)
{
	clientInfo_t	*local;
	refdef_t		*refdef = CG_GetRefdef();
	//vector3			fwd;
	int				vWidth, vHeight;
	float			hScale, vScale;
	float			x, y, w, h;
	qboolean valid = qfalse;

	if ( cg_entities[clientNum].currentValid )
		valid = qtrue;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{ //don't show when dead
		return;
	}

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR )
	{ //don't show when spec
	//	return;
	}

	local = &cgs.clientinfo[ clientNum ];
	if ( !local->infoValid )
	{ //don't show if bad ci
		return;
	}

	if ( valid )
	{
		memcpy( refdef, &cg.refdef[REFDEF_DEFAULT], sizeof( refdef_t ) );

		//	refdef->rdflags = (RDF_NOWORLDMODEL|RDF_AUTOMAP);

		//VectorCopy(cg.predictedPlayerState.origin, refdef->vieworg);
		//	VectorCopy(cg_entities[clientNum].lerpOrigin, refdef->vieworg);
		//	CG_OffsetThirdPersonView( clientNum );
		//	VectorCopy(cg_entities[clientNum].lerpAngles, refdef->viewangles);
		VectorCopy(&cg_entities[clientNum].lerpOrigin, &refdef->vieworg);
		CG_OffsetThirdPersonView( clientNum );
		VectorCopy(&cg_entities[clientNum].lerpAngles, &refdef->viewangles);
		//	CG_CalcViewValues( clientNum );

		//scale out in the direction of the view angles base on the zoom factor
		//	AngleVectors(refdef->viewangles, fwd, 0, 0);
		//	VectorMA(refdef->vieworg, -r_autoMapZoom.value, fwd, refdef->vieworg);

		AnglesToAxis(&refdef->viewangles, refdef->viewaxis);

		refdef->fov_x = cg_fov.value;
		refdef->fov_y = cg_fov.value;

		//guess this doesn't need to be done every frame, but eh
		trap->R_GetRealRes(&vWidth, &vHeight);

		//set scaling values so that the 640x480 will result at 1.0/1.0
		hScale = vWidth/640.0f;
		vScale = vHeight/480.0f;

		x = altViewX;
		y = altViewY;
		w = 160;
		h = 160;

		refdef->x = x*hScale;
		refdef->y = y*vScale;

		refdef->width = w*hScale;
		refdef->height = h*vScale;

		refdef->time = cg.time;

		trap->R_ClearScene();
		//CG_AddRefentForAutoMap( &cg_entities[cg.clientNum] );
		{
			//	int saved = cg.renderingThirdPerson;
			//	cg.renderingThirdPerson = true;
			CG_Player( &cg_entities[clientNum] );
			//	cg.renderingThirdPerson = saved;
		}

		trap->R_RenderScene( refdef );
	}
	else
	{
		CG_DrawPic( altViewX, altViewY, 160, 160, cgs.media.backTileShader );
	}

	altViewX += 160;
	if ( altViewX > 640 )
	{
		altViewX = 0;
		altViewY += 160;
		if ( altViewY > 480 )
		{
			altViewY = 0;
		}
	}
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
static qboolean cg_rangedFogging = qfalse; //so we know if we should go back to normal fog
float cg_linearFogOverride = 0.0f; //designer-specified override for linear fogging style

extern void BG_VehicleTurnRateForSpeed( Vehicle_t *pVeh, float speed, float *mPitchOverride, float *mYawOverride );
extern qboolean PM_InKnockDown( playerState_t *ps );

extern qboolean cgQueueLoad;
extern void CG_ActualLoadDeferredPlayers( void );

static int cg_siegeClassIndex = -2;

#include "ui/ui_shared.h"

static QINLINE void normalizeToLength(vector3 *vec, float length){
	float len, ilen;
	len = vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
	len = sqrt (len);

	if ( len > 0) {
		ilen = length/len;
		vec->x *= ilen;
		vec->y *= ilen;
		vec->z *= ilen;
	}
}

static QINLINE float angleBetween(vector3 *vec1, vector3 *vec2){
	float angle;

	angle = DotProduct(vec1,vec2);
	angle /= VectorLength(vec1);
	angle /= VectorLength(vec2);
	if (angle < 0)
		angle = -angle;
	angle = acos(angle);

	return angle;
}

static void addVelocityVector(){
	refEntity_t*	ref;
	vector3	velocity;

	//static settings, probably better to put elsewhere

	ref = &cg.japp.velocityVect;

	ref->shaderRGBA[0] = ref->shaderRGBA[1] = ref->shaderRGBA[2] = ref->shaderRGBA[3] = 0xff;

	//dynamic settings
	VectorCopy(&cg.predictedPlayerState.origin, &ref->origin);
	ref->origin.z += cg.predictedPlayerState.viewheight / 2;

	VectorCopy( &cg.predictedPlayerState.velocity, &velocity );
	velocity.z = 0;
	VectorScale(&velocity, 0.1, &velocity);
	normalizeToLength(&velocity, cg_strafeHelperLength.value);
	VectorAdd(&ref->origin, &velocity, &ref->oldorigin);

	SE_R_AddRefEntityToScene( ref, MAX_CLIENTS );
}

static qboolean addIdealVectors(){
	refEntity_t	*left, *right;
	float  idealAngle, /*cosAngle, sinAngle,*/ delta;
	float  cosLeftAngle, sinLeftAngle, cosRightAngle, sinRightAngle;
	float  idealAngleToLeft, idealAngleToRight;
	float fps, speedLimit;
	vector3 *vel;
	qboolean drawLeft = qtrue, drawRight = qtrue;

	if (cg.xyspeed == 0.0f) //no speed, we dont care
		return qfalse;
	
	vel = &cg.predictedPlayerState.velocity;

	left = &cg.japp.leftIdeal;
	right = &cg.japp.rightIdeal;

	cg.japp.leftIdeal.shaderRGBA[0] = cg.japp.rightIdeal.shaderRGBA[0] = cg.strafeHelperColour.r;
	cg.japp.leftIdeal.shaderRGBA[1] = cg.japp.rightIdeal.shaderRGBA[1] = cg.strafeHelperColour.g;
	cg.japp.leftIdeal.shaderRGBA[2] = cg.japp.rightIdeal.shaderRGBA[2] = cg.strafeHelperColour.b;
	cg.japp.leftIdeal.shaderRGBA[3] = cg.japp.rightIdeal.shaderRGBA[3] = cg.strafeHelperColour.a;

	//dynamic settings
	//computing ideal angle
	fps = cg.japp.fps;//cg_strafehelper_fps.integer;
	speedLimit = (cg.snap && (cg.snap->ps.fd.forcePowersActive & (1<<FP_SPEED))) ? 425 : 250;

	idealAngle = speedLimit - speedLimit/fps;
	idealAngle /= cg.xyspeed;

	if (idealAngle > 1.0f)
		return qfalse;

	idealAngle = acos(idealAngle);

	delta = idealAngle;

	//increase ideal angle a bit, so people will aim behind it more => better acceleration
	//idealAngle += cg_testcvar1.value;

	
	//for inverted strafe we need to subtract Pi/4 more and get absolute value
	if (cg_strafeHelper.integer == 2){
		idealAngleToLeft = idealAngleToRight = M_PI/2 - idealAngle;
	} else if (cg_strafeHelper.integer == 4){
		idealAngleToLeft = idealAngle + M_PI/2;
		idealAngleToRight = idealAngle - M_PI/2;
	}else {
		idealAngleToLeft = idealAngleToRight = idealAngle - M_PI/4;
	}

	//if (idealAngle <= 0 /*|| idealAngle >= M_PI/2*/){
	//	return qfalse;
	//}

	cosLeftAngle = cos(idealAngleToLeft);
	sinLeftAngle = sin(idealAngleToLeft);

	cosRightAngle = cos(idealAngleToRight);
	sinRightAngle = sin(idealAngleToRight);

    //beginning of the ideal vectors
	VectorCopy(&cg.predictedPlayerState.origin, &left->origin);
	VectorCopy(&cg.predictedPlayerState.origin, &right->origin);

	//move both vectors up a bit (dont wanna have it on ground)
	left->origin.z  += cg.predictedPlayerState.viewheight / 2;
	right->origin.z += cg.predictedPlayerState.viewheight / 2;

	//ends of the ideal vectors
	left->oldorigin.z = right->oldorigin.z = 0;

	//turning velocity to left 
	left->oldorigin.x = vel->x*cosLeftAngle - vel->y*sinLeftAngle;
	left->oldorigin.y = vel->x*sinLeftAngle + vel->y*cosLeftAngle;

	//turning velocity to right 
	right->oldorigin.x =  vel->x*cosRightAngle + vel->y*sinRightAngle;
	right->oldorigin.y = -vel->x*sinRightAngle + vel->y*cosRightAngle;

	normalizeToLength(&left->oldorigin, cg_strafeHelperLength.integer);
	normalizeToLength(&right->oldorigin, cg_strafeHelperLength.integer);

	if (cg.japp.isfixedVector){
		//compute angle between fixed direction and left ideal direction
		//if angle is greater then limit (for example 5 degrees, dont draw left)
		float angle, angleToLeft, angleToRight;
		vector3 fw;

		//calculate forward direction
		AngleVectors(&cg.predictedPlayerState.viewangles, &fw, NULL, NULL);
		fw.z = 0;

		//calculate angle distance between left and right
		angleToLeft = angleBetween(&fw, &left->oldorigin);
		angleToRight = angleBetween(&fw, &right->oldorigin);

		if (angleToLeft < angleToRight){
			//calculate angle distance from left to fixed 
			angle = angleBetween(&left->oldorigin, &cg.japp.fixedVector);
			angle += M_PI/4 - delta;

			//trap->Print("DEBUG: angleToLeft=%.3f angleToRight=%.3f delta=%.3f angleFromLeft2Fixed=%.3f\n",
			//	angleToLeft,angleToRight,delta,angle);

			if (angle < cg_strafeHelperAngle.value*M_PI/180.0f){
				drawLeft = qtrue;
				drawRight = qfalse;
			} else {
				drawLeft = qfalse;
				drawRight = qtrue;
			}
		} else {
			//calculate angle distance from right to fixed 
			angle = angleBetween(&right->oldorigin, &cg.japp.fixedVector);
			angle += M_PI/4 - delta;

			//trap->Print("DEBUG: angleToLeft=%.3f angleToRight=%.3f delta=%.3f angleFromRight2Fixed=%.3f\n",
			//	angleToLeft,angleToRight,delta,angle);

			if (angle < cg_strafeHelperAngle.value*M_PI/180.0f){
				drawLeft = qfalse;
				drawRight = qtrue;
			} else {
				drawLeft = qtrue;
				drawRight = qfalse;
			}
		}

		//TO DO: proper fix for invert strafe
		//if (cg_strafehelper.integer == 2){
		//	qboolean tmp = drawLeft;
		//	drawLeft = drawRight;
		//	drawRight = tmp;
		//}

	}



	if (drawLeft){
		VectorAdd(&left->origin, &left->oldorigin, &left->oldorigin);
		SE_R_AddRefEntityToScene( left, MAX_CLIENTS );
	}

	if (drawRight){
		VectorAdd(&right->origin, &right->oldorigin, &right->oldorigin);
		SE_R_AddRefEntityToScene( right, MAX_CLIENTS );
	}

	return qtrue;
}



void CG_AddMovementVectors(){
	static int modCount = 0;
	if (modCount != cg_strafeHelperRadius.modificationCount){
		cg.japp.leftIdeal.radius = cg.japp.rightIdeal.radius = cg.japp.velocityVect.radius = cg_strafeHelperRadius.value;
		modCount = cg_strafeHelperRadius.modificationCount;
	}

	cg.japp.leftIdeal.reType		= cg.japp.rightIdeal.reType			= cg.japp.velocityVect.reType			= RT_LINE;
	cg.japp.leftIdeal.customShader	= cg.japp.rightIdeal.customShader	= cg.japp.velocityVect.customShader		= cgs.media.rgbSaberCoreShader;

	if (addIdealVectors() && cg_strafeHelperVelocity.integer)
		addVelocityVector();
}

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;
	const char *cstr;
	float mSensitivity = cg.zoomSensitivity;
	float mPitchOverride = 0.0f;
	float mYawOverride = 0.0f;
	static centity_t *veh = NULL;
#ifdef VEH_CONTROL_SCHEME_4
	float mSensitivityOverride = 0.0f;
	qboolean bUseFighterPitch = qfalse;
	qboolean	isFighter = qfalse;
#endif
//	int i;
//	int savedSolid[MAX_CLIENTS];
	static int pwSet = 0;
	refdef_t *refdef = CG_GetRefdef();

	if ( !pwSet && cg.time > 5000 )
	{//Raz: Ugly hax
		trap->Cvar_Set( "cp_login", "" );
		pwSet = 1;
	}

	if (cgQueueLoad)
	{ //do this before you start messing around with adding ghoul2 refents and crap
		CG_ActualLoadDeferredPlayers();
		cgQueueLoad = qfalse;
	}

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	if (cg.snap && ui_myteam.integer != cg.snap->ps.persistant[PERS_TEAM])
	{
		trap->Cvar_Set ( "ui_myteam", va("%i", cg.snap->ps.persistant[PERS_TEAM]) );
	}
	if (cgs.gametype == GT_SIEGE &&
		cg.snap &&
		cg_siegeClassIndex != cgs.clientinfo[cg.snap->ps.clientNum].siegeIndex)
	{
		cg_siegeClassIndex = cgs.clientinfo[cg.snap->ps.clientNum].siegeIndex;
		if (cg_siegeClassIndex == -1)
		{
			trap->Cvar_Set("ui_mySiegeClass", "<none>");
		}
		else
		{
			trap->Cvar_Set("ui_mySiegeClass", bgSiegeClasses[cg_siegeClassIndex].name);
		}
	}

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	trap->FX_AdjustTime( cg.time );

	CG_RunLightStyles();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap->S_ClearLoopingSounds();

	// clear all the render lists
	trap->R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	trap->ROFF_UpdateEntities();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) )
	{
#if 0	
		// Transition from zero to negative one on the snapshot timeout.
		// The reason we do this is because the first client frame is responsible for
		// some farily slow processing (such as weather) and we dont want to include
		// that processing time into our calculations
		if ( !cg.snapshotTimeoutTime )
		{
			cg.snapshotTimeoutTime = -1;
		}
		// Transition the snapshot timeout time from -1 to the current time in 
		// milliseconds which will start the timeout.
		else if ( cg.snapshotTimeoutTime == -1 )
		{		
			cg.snapshotTimeoutTime = trap->Milliseconds ( );
		}

		// If we have been waiting too long then just error out
		if ( cg.snapshotTimeoutTime > 0 && (trap->Milliseconds ( ) - cg.snapshotTimeoutTime > cg_snapshotTimeout.integer * 1000) )
		{
			Com_Error ( ERR_DROP, CG_GetStringEdString("MP_SVGAME", "SNAPSHOT_TIMEOUT"));
			return;
		}
#endif	
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	if (cg.snap && cg.snap->ps.saberLockTime > cg.time)
	{
		mSensitivity = 0.01f;
	}
	else if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN)
	{ //lower sens for emplaced guns and vehicles
		mSensitivity = 0.2f;
	}
#ifdef VEH_CONTROL_SCHEME_4
	else if (cg.predictedPlayerState.m_iVehicleNum//in a vehicle
		&& !cg.predictedPlayerState.generic1 )//not as a passenger
	{
		centity_t *cent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		if ( cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{
			BG_VehicleTurnRateForSpeed( cent->m_pVehicle, cent->currentState.speed, &mPitchOverride, &mYawOverride );
			//mSensitivityOverride = 5.0f;//old default value
			mSensitivityOverride = 0.0f;
			bUseFighterPitch = qtrue;
			trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, mSensitivityOverride, cg.forceSelect, cg.itemSelect, bUseFighterPitch );
			isFighter = qtrue;
		}
	} 

	if ( !isFighter )
#endif //VEH_CONTROL_SCHEME_4
	{
		if (cg.predictedPlayerState.m_iVehicleNum)
		{
			veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
		}
		if (veh &&
			veh->currentState.eType == ET_NPC &&
			veh->currentState.NPC_class == CLASS_VEHICLE &&
			veh->m_pVehicle &&
			veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
			bg_fighterAltControl.integer)
		{
			trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qtrue );
			veh = NULL; //this is done because I don't want an extra assign each frame because I am so perfect and super efficient.
		}
		else
		{
			trap->SetUserCmdValue( cg.weaponSelect, mSensitivity, mPitchOverride, mYawOverride, 0.0f, cg.forceSelect, cg.itemSelect, qfalse );
		}
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	//Raz: Don't collide against those we're not dueling against
//	for ( i=0; i<MAX_CLIENTS; i++ )
//		if ( cg.snap->ps.duelIndex != i )
//		{
//			savedSolid[i] = cg_entities[i].currentState.solid;
//			if ( cg.snap->ps.duelInProgress )
//				cg_entities[i].currentState.solid = 0;
//		}

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

//	for ( i=0; i<MAX_CLIENTS; i++ )
//		if ( cg.snap->ps.duelIndex != i )
//			cg_entities[i].currentState.solid = savedSolid[i];

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);

	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
#ifdef FP_EWEB
		if (cg.predictedPlayerState.weapon == WP_EMPLACED_GUN && cg.predictedPlayerState.emplacedIndex /*&&
			cg_entities[cg.predictedPlayerState.emplacedIndex].currentState.weapon == WP_NONE*/)
		{ //force third person for e-web and emplaced use
			cg.renderingThirdPerson = 1;
		}
		//[TrueView]
		/*
		else if (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE ||
			BG_InGrappleMove(cg.predictedPlayerState.torsoAnim) || BG_InGrappleMove(cg.predictedPlayerState.legsAnim) ||
			cg.predictedPlayerState.forceHandExtend == HANDEXTEND_KNOCKDOWN || cg.predictedPlayerState.fallingToDeath ||
			cg.predictedPlayerState.m_iVehicleNum || PM_InKnockDown(&cg.predictedPlayerState))
		{
			if (cg_fpls.integer && cg.predictedPlayerState.weapon == WP_SABER)
			{ //force to first person for fpls
				cg.renderingThirdPerson = 0;
			}
			else
			{
				cg.renderingThirdPerson = 1;
			}
		}
		*/
		else if (cg_trueInvertSaber.integer == 2 && (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE))
#else
		if (cg_trueInvertSaber.integer == 2 && (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE))
#endif
		{//force thirdperson for sabers/melee if in cg_trueInvertSaber.integer == 2
			cg.renderingThirdPerson = qtrue;
		}
#ifdef RAZTEST
		else if (cg.predictedPlayerState.fallingToDeath
#else
		else if (cg.predictedPlayerState.fallingToDeath || cg.predictedPlayerState.m_iVehicleNum
#endif
			|| (cg_trueInvertSaber.integer == 1 && !cg_thirdPerson.integer && (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE)))
		{
			cg.renderingThirdPerson = qtrue;
		}
		else if (cg_trueInvertSaber.integer == 1 && cg_thirdPerson.integer && (cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE))
		{
			cg.renderingThirdPerson = qfalse;
		}
		//[/TrueView]
		else if (cg.predictedPlayerState.zoomMode)
		{ //always force first person when zoomed
			cg.renderingThirdPerson = 0;
		}
		if ( cg.japp.fakeGun )
			cg.renderingThirdPerson = 0;
	}
	
	if (cg.predictedPlayerState.pm_type == PM_SPECTATOR)
	{ //always first person for spec
		cg.renderingThirdPerson = 0;
	}


	if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		cg.renderingThirdPerson = 0;
	}

	// build cg.refdef
	inwater = CG_CalcViewValues( cg.clientNum );

	if (cg_linearFogOverride)
	{
		trap->R_SetRangedFog(-cg_linearFogOverride);
	}
	else if (cg.predictedPlayerState.zoomMode)
	{ //zooming with binoculars or sniper, set the fog range based on the zoom level -rww
		cg_rangedFogging = qtrue;
		//smaller the fov the less fog we have between the view and cull dist
		trap->R_SetRangedFog(refdef->fov_x*64.0f);
	}
	else if (cg_rangedFogging)
	{ //disable it
		cg_rangedFogging = qfalse;
		trap->R_SetRangedFog(0.0f);
	}

	cstr = CG_ConfigString(CS_SKYBOXORG);

	if (cstr && cstr[0])
	{ //we have a skyportal
		CG_DrawSkyBoxPortal(cstr);
	}

	CG_CalcScreenEffects();

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson && cg.predictedPlayerState.pm_type != PM_SPECTATOR ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities(qfalse);			// adter calcViewValues, so predicted player state is correct
		CG_AddMarks();
		CG_AddLocalEntities();
		CG_DrawMiscEnts();
	}
	CG_AddViewWeapon( &cg.predictedPlayerState );

	if ( !cg.hyperspace) 
	{
		trap->FX_AddScheduledEffects(qfalse);
	}

	// add buffered sounds
	CG_PlayBufferedSounds();

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}

	//SMod - draws movement vectors
	if ( cg_strafeHelper.integer && cg.predictedPlayerState.stats[STAT_HEALTH] > 0
		&& cg.predictedPlayerState.pm_type != PM_SPECTATOR && cg.predictedPlayerState.pm_type != PM_INTERMISSION )
		CG_AddMovementVectors();

	refdef->time = cg.time;
	memcpy( refdef->areamask, cg.snap->areamask, sizeof( refdef->areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// if there are any entities flagged as sound trackers and attached to other entities, update their sound pos
	CG_UpdateSoundTrackers();

	if (gCGHasFallVector)
	{
		vector3 lookAng;

		VectorSubtract(&cg.snap->ps.origin, &refdef->vieworg, &lookAng);
		VectorNormalize(&lookAng);
		vectoangles(&lookAng, &lookAng);

		VectorCopy(&gCGFallVector, &refdef->vieworg);
		AnglesToAxis(&lookAng, refdef->viewaxis);
	}

	//This is done from the vieworg to get origin for non-attenuated sounds
	cstr = CG_ConfigString( CS_GLOBAL_AMBIENT_SET );

	if (cstr && cstr[0])
	{
		trap->S_UpdateAmbientSet( cstr, &refdef->vieworg );
	}

	// update audio positions
	trap->S_Respatialize( cg.snap->ps.clientNum, &refdef->vieworg, refdef->viewaxis, inwater );

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}
	if (timescale.value != cg_timescaleFadeEnd.value) {
		if (timescale.value < cg_timescaleFadeEnd.value) {
			timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (timescale.value > cg_timescaleFadeEnd.value)
				timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (timescale.value < cg_timescaleFadeEnd.value)
				timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap->Cvar_Set("timescale", va("%f", timescale.value));
		}
	}

	//Raz: JPLua!
	JPLua_Event_RunFrame();

	// actually issue the rendering calls
	CG_DrawActive( stereoView );

	cg.currentRefdef = REFDEF_AUTOMAP;
	CG_DrawAutoMap();

	cg.currentRefdef = REFDEF_ALTVIEW;
	{
		int i = 0;
		altViewX = 0;
		altViewY = 0;
		for ( i=0; i<4; i++ )
		{
		//	CG_DrawAltView( i );
		}
		altViewX = 0;
		altViewY = 0;
	}

	//Reset the refdef for future frames
	cg.currentRefdef = REFDEF_DEFAULT;

	if ( trap->Key_GetCatcher() & KEYCATCH_CGAME && !CG_ChatboxActive() ) {
		displayContextDef_t *dc = Display_GetContext();
		CG_DrawPic( (float)dc->cursorx, (float)dc->cursory, 40.0f, 40.0f, cgs.media.cursor);
	}

	trap->R_SetColor( NULL );

	if ( cg_stats.integer )
		trap->Print( "cg.clientFrame:%i\n", cg.clientFrame );
}

//[TrueView]
//Checks to see if the current camera position is valid based on the last known safe location.  If it's not safe, place
//the camera at the last position safe location
void CheckCameraLocation( vector3 *OldeyeOrigin )  
{
	trace_t			trace;
	refdef_t *refdef = CG_GetRefdef();

	CG_Trace(&trace, OldeyeOrigin, &cameramins, &cameramaxs, &refdef->vieworg, cg.snap->ps.clientNum, GetCameraClip());
	if (trace.fraction <= 1.0)
	{
		VectorCopy(&trace.endpos, &refdef->vieworg);
	}
}
//[/TrueView]
