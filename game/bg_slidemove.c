// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_slidemove.c -- part of bg_pmove functionality

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

#if defined( _GAME )
	#include "g_local.h"
#elif defined( _CGAME )
	#include "cgame/cg_local.h"
#endif

/*

input: origin, velocity, bounds, groundPlane, trace function

output: origin, velocity, impacts, stairup boolean

*/


//do vehicle impact stuff
// slight rearrangement by BTO (VV) so that we only have one namespace include
#ifdef _GAME
	extern void G_FlyVehicleSurfaceDestruction(gentity_t *veh, trace_t *trace, int magnitude, qboolean force ); //g_vehicle.c
	extern qboolean G_CanBeEnemy(gentity_t *self, gentity_t *enemy); //w_saber.c
#endif

extern qboolean BG_UnrestrainedPitchRoll( playerState_t *ps, Vehicle_t *pVeh );

extern bgEntity_t *pm_entSelf;
extern bgEntity_t *pm_entVeh;

//vehicle impact stuff continued...
#ifdef _GAME
	extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );
#endif

extern void PM_SetPMViewAngle(playerState_t *ps, vector3 *angle, usercmd_t *ucmd);

#define MAX_IMPACT_TURN_ANGLE 45.0f
void PM_VehicleImpact( bgEntity_t *pEnt, trace_t *trace ) {
	// See if the vehicle has crashed into the ground.
	Vehicle_t *pSelfVeh = pEnt->m_pVehicle;
	float magnitude = VectorLength( &pm->ps->velocity ) * pSelfVeh->m_pVehicleInfo->mass / 50.0f;
	qboolean forceSurfDestruction = qfalse;
#ifdef _GAME
	gentity_t *hitEnt = trace ? &g_entities[trace->entityNum] : NULL;

	if ( !hitEnt || (pSelfVeh && pSelfVeh->m_pPilot && hitEnt && hitEnt->s.eType == ET_MISSILE && hitEnt->inuse
		&& hitEnt->r.ownerNum == pSelfVeh->m_pPilot->s.number) )
	{
		return;
	}

	if ( pSelfVeh && pSelfVeh->m_iRemovedSurfaces ) {
		// spiralling to our deaths, explode on any solid impact
		if ( hitEnt->s.NPC_class == CLASS_VEHICLE ) {
			// hit another vehicle, explode!
			// Give credit to whoever got me into this death spiral state
			gentity_t *parent = (gentity_t *)pSelfVeh->m_pParentEntity;
			gentity_t *killer = NULL;
			if ( parent->client->ps.otherKiller < ENTITYNUM_WORLD && parent->client->ps.otherKillerTime > level.time ) {
				gentity_t *potentialKiller = &g_entities[parent->client->ps.otherKiller];

				if ( potentialKiller->inuse && potentialKiller->client )
					killer = potentialKiller;
			}
			//FIXME: damage hitEnt, some, too?  Our explosion should hurt them some, but...
			G_Damage( (gentity_t *)pEnt, killer, killer, NULL, &pm->ps->origin, 999999, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
			return;
		}
		else if ( !VectorCompare( &trace->plane.normal, &vec3_origin )
			&& (trace->entityNum == ENTITYNUM_WORLD || hitEnt->r.bmodel) )
		{// have a valid hit plane and we hit a solid brush
			vector3	moveDir;
			float impactDot;

			VectorCopy( &pm->ps->velocity, &moveDir );
			VectorNormalize( &moveDir );
			impactDot = DotProduct( &moveDir, &trace->plane.normal );
			if ( impactDot <= -0.7f ) {
				// hit rather head-on and hard
				// Give credit to whoever got me into this death spiral state
				gentity_t *parent = (gentity_t *)pSelfVeh->m_pParentEntity;
				gentity_t *killer = NULL;
				if ( parent->client->ps.otherKiller < ENTITYNUM_WORLD && parent->client->ps.otherKillerTime > level.time ) {
					gentity_t *potentialKiller = &g_entities[parent->client->ps.otherKiller];

					if ( potentialKiller->inuse && potentialKiller->client )
						killer = potentialKiller;
				}
				G_Damage( (gentity_t *)pEnt, killer, killer, NULL, &pm->ps->origin, 999999, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT
				return;
			}
		}
	}
	
	if ( trace->entityNum < ENTITYNUM_WORLD && hitEnt->s.eType == ET_MOVER && hitEnt->s.apos.trType != TR_STATIONARY
		&& (hitEnt->spawnflags & 16) /*IMPACT*/ && !Q_stricmp( "func_rotating", hitEnt->classname ) )
	{
		// hit a func_rotating that is supposed to destroy anything it touches!
		// guarantee the hit will happen, thereby taking off a piece of the ship
		forceSurfDestruction = qtrue;
	}
	else if ( (fabsf( pm->ps->velocity.x ) + fabsf( pm->ps->velocity.y )) < 100.0f && pm->ps->velocity.z > -100.0f )
#else
	if ( (fabsf( pm->ps->velocity.x ) + fabs( pm->ps->velocity.y )) < 100.0f && pm->ps->velocity.z > -100.0f )
#endif
	{// we're landing, we're cool
		// this was annoying me -rww
		//FIXME: this shouldn't even be getting called when the vehicle is at rest!
#ifdef _GAME
		if ( hitEnt && (hitEnt->s.eType == ET_PLAYER || hitEnt->s.eType == ET_NPC) && pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER ) {
		}
		else
#endif
			return;
	}
	if ( pSelfVeh && (pSelfVeh->m_pVehicleInfo->type == VH_SPEEDER || pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER)
		&& (magnitude >= 100 || forceSurfDestruction) )
	{
		if ( pEnt->m_pVehicle->m_iHitDebounce < pm->cmd.serverTime || forceSurfDestruction ) {
			// a bit of a hack, may conflict with getting shot, but...
			//FIXME: impact sound and effect should be gotten from g_vehicleInfo...?
			//FIXME: should pass in trace.endpos and trace.plane.normal
			vector3	vehUp;
#ifndef _GAME
			bgEntity_t *hitEnt;
#endif

			if ( trace && !pSelfVeh->m_iRemovedSurfaces && !forceSurfDestruction ) {
				qboolean turnFromImpact = qfalse, turnHitEnt = qfalse;
				float l = pm->ps->speed*0.5f;
				vector3	bounceDir;
#ifndef _GAME
				hitEnt = PM_BGEntForNum( trace->entityNum );
#endif
				if ( (trace->entityNum == ENTITYNUM_WORLD || hitEnt->s.solid == SOLID_BMODEL)
					 && !VectorCompare( &trace->plane.normal, &vec3_origin ) )
				{ //bounce off in the opposite direction of the impact
					if ( pSelfVeh->m_pVehicleInfo->type == VH_SPEEDER ) {
						pm->ps->speed *= pml.frametime;
						VectorCopy( &trace->plane.normal, &bounceDir );
					}
					else if ( trace->plane.normal.z >= MIN_LANDING_SLOPE && pSelfVeh->m_LandTrace.fraction < 1.0f
						&& pm->ps->speed <= MIN_LANDING_SPEED )
					{
						// could land here, don't bounce off, in fact, return altogether!
						return;
					}
					else {
						if ( pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER )
							turnFromImpact = qtrue;
						VectorCopy( &trace->plane.normal, &bounceDir );
					}
				}
				else if ( pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER ) {
					// check for impact with another fighter
#ifndef _GAME
					hitEnt = PM_BGEntForNum( trace->entityNum );
#endif
					if ( hitEnt->s.NPC_class == CLASS_VEHICLE && hitEnt->m_pVehicle && hitEnt->m_pVehicle->m_pVehicleInfo
						&& hitEnt->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
					{// two vehicles hit each other, turn away from the impact
						turnFromImpact = qtrue;
						turnHitEnt = qtrue;
#ifndef _GAME
						VectorSubtract( &pm->ps->origin, &hitEnt->s.origin, &bounceDir );
#else
						VectorSubtract( &pm->ps->origin, &hitEnt->r.currentOrigin, &bounceDir );
#endif
						VectorNormalize( &bounceDir );
					}
				}
				if ( turnFromImpact ) {
					// bounce off impact surf and turn away
					vector3	pushDir, turnAwayAngles, turnDelta;
					float turnStrength, pitchTurnStrength, yawTurnStrength;
					vector3	moveDir;
					float bounceDot, turnDivider;

					//bounce
					if ( !turnHitEnt )
						VectorScale( &bounceDir, (pm->ps->speed*0.25f/pSelfVeh->m_pVehicleInfo->mass), &pushDir );
					else
					{//hit another fighter
#ifdef _GAME
						if ( hitEnt->client )
							VectorScale( &bounceDir, (pm->ps->speed+hitEnt->client->ps.speed)*0.5f, &pushDir );
						else
							VectorScale( &bounceDir, (pm->ps->speed+hitEnt->s.speed)*0.5f, &pushDir );
#else
						VectorScale( &bounceDir, (pm->ps->speed+hitEnt->s.speed)*0.5f, &bounceDir );
#endif
						VectorScale( &pushDir, (l/pSelfVeh->m_pVehicleInfo->mass), &pushDir );
						VectorScale( &pushDir, 0.1f, &pushDir );
					}
					VectorNormalize2( &pm->ps->velocity, &moveDir );
					bounceDot = DotProduct( &moveDir, &bounceDir )*-1;
					if ( bounceDot < 0.1f )
						bounceDot = 0.1f;
					VectorScale( &pushDir, bounceDot, &pushDir );
					VectorAdd( &pm->ps->velocity, &pushDir, &pm->ps->velocity );

					//turn
					turnDivider = (pSelfVeh->m_pVehicleInfo->mass/400.0f);
					if ( turnHitEnt ) //don't turn as much when hit another ship
						turnDivider *= 4.0f;
					if ( turnDivider < 0.5f )
						turnDivider = 0.5f;

					turnStrength = (magnitude/2000.0f);
					if ( turnStrength < 0.1f )
						turnStrength = 0.1f;
					else if ( turnStrength > 2.0f )
						turnStrength = 2.0f;

					//get the angles we are going to turn towards
					vectoangles( &bounceDir, &turnAwayAngles );
					//get the delta from our current angles to those new angles
					AnglesSubtract( &turnAwayAngles, pSelfVeh->m_vOrientation, &turnDelta );
					//now do pitch
					if ( !bounceDir.z ) {
						// shouldn't be any pitch
					}
					else {
						pitchTurnStrength = turnStrength*turnDelta.pitch;
						if ( pitchTurnStrength > MAX_IMPACT_TURN_ANGLE )
							pitchTurnStrength = MAX_IMPACT_TURN_ANGLE;
						else if ( pitchTurnStrength < -MAX_IMPACT_TURN_ANGLE )
							pitchTurnStrength = -MAX_IMPACT_TURN_ANGLE;
					//	pSelfVeh->m_vOrientation->pitch = AngleNormalize180( pSelfVeh->m_vOrientation->pitch+pitchTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
						pSelfVeh->m_vFullAngleVelocity.pitch = AngleNormalize180( pSelfVeh->m_vOrientation->pitch + pitchTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
					}
					//now do yaw
					if ( !bounceDir.x && !bounceDir.y ) {
						// shouldn't be any yaw
					}
					else {
						yawTurnStrength = turnStrength*turnDelta.yaw;
						if ( yawTurnStrength > MAX_IMPACT_TURN_ANGLE )
							yawTurnStrength = MAX_IMPACT_TURN_ANGLE;
						else if ( yawTurnStrength < -MAX_IMPACT_TURN_ANGLE )
							yawTurnStrength = -MAX_IMPACT_TURN_ANGLE;
					//	pSelfVeh->m_vOrientation->roll = AngleNormalize180( pSelfVeh->m_vOrientation->roll-yawTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
						pSelfVeh->m_vFullAngleVelocity.roll = AngleNormalize180( pSelfVeh->m_vOrientation->roll-yawTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
					}
					/*
					PM_SetPMViewAngle( pm->ps, pSelfVeh->m_vOrientation, &pSelfVeh->m_ucmd );
					if ( pm_entVeh ) {
						// I'm a vehicle, so pm_entVeh is actually my pilot
						bgEntity_t *pilot = pm_entVeh;
						if ( !BG_UnrestrainedPitchRoll( pilot->playerState, pSelfVeh ) ) {
							// set the rider's viewangles to the vehicle's viewangles
							PM_SetPMViewAngle( pilot->playerState, pSelfVeh->m_vOrientation, &pSelfVeh->m_ucmd );
						}
					}
					*/
#ifdef _GAME//server-side, turn the guy we hit away from us, too
					if ( turnHitEnt && hitEnt->client && !FighterIsLanded( hitEnt->m_pVehicle, &hitEnt->client->ps )
						&& !(hitEnt->spawnflags & 2) /*SUSPENDED*/ )
					{
						l = hitEnt->client->ps.speed;
						//now bounce *them* away and turn them
						//flip the bounceDir
						VectorScale( &bounceDir, -1, &bounceDir );
						//do bounce
						VectorScale( &bounceDir, (pm->ps->speed+l)*0.5f, &pushDir );
						VectorScale( &pushDir, (l*0.5f/hitEnt->m_pVehicle->m_pVehicleInfo->mass), &pushDir );
						VectorNormalize2( &hitEnt->client->ps.velocity, &moveDir );
						bounceDot = DotProduct( &moveDir, &bounceDir )*-1;
						if ( bounceDot < 0.1f )
							bounceDot = 0.1f;
						VectorScale( &pushDir, bounceDot, &pushDir );
						VectorAdd( &hitEnt->client->ps.velocity, &pushDir, &hitEnt->client->ps.velocity );
						//turn
						turnDivider = (hitEnt->m_pVehicle->m_pVehicleInfo->mass/400.0f);
						// don't turn as much when hit another ship
						if ( turnHitEnt )
							turnDivider *= 4.0f;
						if ( turnDivider < 0.5f )
							turnDivider = 0.5f;
						//get the angles we are going to turn towards
						vectoangles( &bounceDir, &turnAwayAngles );
						//get the delta from our current angles to those new angles
						AnglesSubtract( &turnAwayAngles, hitEnt->m_pVehicle->m_vOrientation, &turnDelta );
						//now do pitch
						if ( !bounceDir.z ) {
							// shouldn't be any pitch
						}
						else {
							pitchTurnStrength = turnStrength*turnDelta.pitch;
							if ( pitchTurnStrength > MAX_IMPACT_TURN_ANGLE )
								pitchTurnStrength = MAX_IMPACT_TURN_ANGLE;
							else if ( pitchTurnStrength < -MAX_IMPACT_TURN_ANGLE )
								pitchTurnStrength = -MAX_IMPACT_TURN_ANGLE;
						//	hitEnt->m_pVehicle->m_vOrientation->pitch = AngleNormalize180( hitEnt->m_pVehicle->m_vOrientation->pitch+pitchTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
							hitEnt->m_pVehicle->m_vFullAngleVelocity.pitch = AngleNormalize180( hitEnt->m_pVehicle->m_vOrientation->pitch+pitchTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
						}
						//now do yaw
						if ( !bounceDir.x && !bounceDir.y ) {
							// shouldn't be any yaw
						}
						else {
							yawTurnStrength = turnStrength*turnDelta.yaw;
								 if ( yawTurnStrength >  MAX_IMPACT_TURN_ANGLE )	yawTurnStrength =  MAX_IMPACT_TURN_ANGLE;
							else if ( yawTurnStrength < -MAX_IMPACT_TURN_ANGLE )	yawTurnStrength = -MAX_IMPACT_TURN_ANGLE;
						//	hitEnt->m_pVehicle->m_vOrientation->roll = AngleNormalize180( hitEnt->m_pVehicle->m_vOrientation->roll-yawTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
							hitEnt->m_pVehicle->m_vFullAngleVelocity.roll = AngleNormalize180( hitEnt->m_pVehicle->m_vOrientation->roll-yawTurnStrength/turnDivider*pSelfVeh->m_fTimeModifier );
						}
						//NOTE: will these angle changes stick or will they be stomped 
						//		when the vehicle goes through its own update and re-grabs 
						//		its angles from its pilot...?  Should we do a 
						//		SetClientViewAngles on the pilot?
						/*
						SetClientViewAngle( hitEnt, hitEnt->m_pVehicle->m_vOrientation );
						if ( hitEnt->m_pVehicle->m_pPilot && ((gentity_t *)hitEnt->m_pVehicle->m_pPilot)->client )
							SetClientViewAngle( (gentity_t *)hitEnt->m_pVehicle->m_pPilot, hitEnt->m_pVehicle->m_vOrientation );
						*/
					}
#endif
				}
			}

#ifdef _GAME
			if ( !hitEnt )
				return;

			AngleVectors( pSelfVeh->m_vOrientation, NULL, NULL, &vehUp );
			if ( pSelfVeh->m_pVehicleInfo->iImpactFX ) {
			//	G_PlayEffectID( pSelfVeh->m_pVehicleInfo->iImpactFX, pm->ps->origin, vehUp );
				//tempent use bad!
				G_AddEvent( (gentity_t *)pEnt, EV_PLAY_EFFECT_ID, pSelfVeh->m_pVehicleInfo->iImpactFX );
			}
			pEnt->m_pVehicle->m_iHitDebounce = pm->cmd.serverTime + 200;
			magnitude /= pSelfVeh->m_pVehicleInfo->toughness * 50.0f; 

			if ( hitEnt && (hitEnt->s.eType != ET_TERRAIN || !(hitEnt->spawnflags & 1) || pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER) ) {
				// don't damage the vehicle from terrain that doesn't want to damage vehicles
				if ( pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER ) {
					// increase the damage...
					float mult = (pSelfVeh->m_vOrientation->pitch*0.1f);
					if ( mult < 1.0f )
						mult = 1.0f;
					if ( hitEnt->inuse && hitEnt->takedamage ) {
						// if the other guy takes damage, don't hurt us a lot for ramming him
						//unless it's a vehicle, then we get 1.5 times damage
						if ( hitEnt->s.eType == ET_NPC && hitEnt->s.NPC_class == CLASS_VEHICLE && hitEnt->m_pVehicle )
							mult = 1.5f;
						else
							mult = 0.5f;
					}

					magnitude *= mult;
				}
				pSelfVeh->m_iLastImpactDmg = magnitude;
				//FIXME: what about proper death credit to the guy who shot you down?
				//FIXME: actually damage part of the ship that impacted?
				G_Damage( (gentity_t *)pEnt, NULL, NULL, NULL, &pm->ps->origin, magnitude*5, DAMAGE_NO_ARMOR, MOD_FALLING );//FIXME: MOD_IMPACT

				if ( pSelfVeh->m_pVehicleInfo->surfDestruction )
					G_FlyVehicleSurfaceDestruction( (gentity_t *)pEnt, trace, magnitude, forceSurfDestruction );

				pSelfVeh->m_ulFlags |= VEH_CRASHING;
			}

			if ( hitEnt && hitEnt->inuse && hitEnt->takedamage ) {
				// damage this guy because we hit him
				float pmult = 1.0f;
				int finalD;
				gentity_t *attackEnt;

				if ( (hitEnt->s.eType == ET_PLAYER && hitEnt->s.number < MAX_CLIENTS) ||
					 (hitEnt->s.eType == ET_NPC && hitEnt->s.NPC_class != CLASS_VEHICLE) )
				{// probably a humanoid, or something
					if ( pSelfVeh->m_pVehicleInfo->type == VH_FIGHTER )
						pmult = 2000.0f;
					else
						pmult = 40.0f;

					if ( hitEnt->client && BG_KnockDownable( &hitEnt->client->ps ) && G_CanBeEnemy( (gentity_t *)pEnt, hitEnt) ) {
						// smash!
						if ( hitEnt->client->ps.forceHandExtend != HANDEXTEND_KNOCKDOWN ) {
							hitEnt->client->ps.forceHandExtend = HANDEXTEND_KNOCKDOWN;
							hitEnt->client->ps.forceHandExtendTime = pm->cmd.serverTime + 1100;
							hitEnt->client->ps.forceDodgeAnim = 0; //this toggles between 1 and 0, when it's 1 we should play the get up anim
						}

						hitEnt->client->ps.otherKiller = pEnt->s.number;
						hitEnt->client->ps.otherKillerTime = pm->cmd.serverTime + 5000;
						hitEnt->client->ps.otherKillerDebounceTime = pm->cmd.serverTime + 100;

						//add my velocity into his to force him along in the correct direction from impact
						VectorAdd( &hitEnt->client->ps.velocity, &pm->ps->velocity, &hitEnt->client->ps.velocity );
						//upward thrust
						hitEnt->client->ps.velocity.z += 200.0f;
					}
				}

				if ( pSelfVeh->m_pPilot )
					attackEnt = (gentity_t *)pSelfVeh->m_pPilot;
				else
					attackEnt = (gentity_t *)pEnt;

				finalD = magnitude*pmult;
				if ( finalD < 1 )
					finalD = 1;
				G_Damage( hitEnt, attackEnt, attackEnt, NULL, &pm->ps->origin, finalD, 0, MOD_MELEE );//FIXME: MOD_IMPACT
			}
#else	//this is gonna result in "double effects" for the client doing the prediction.
		//it doesn't look bad though. could just use predicted events, but I'm too lazy.
			hitEnt = PM_BGEntForNum( trace->entityNum );

			if ( !hitEnt || hitEnt->s.owner != pEnt->s.number ) {
				// don't hit your own missiles!
				AngleVectors( pSelfVeh->m_vOrientation, NULL, NULL, &vehUp );
				pEnt->m_pVehicle->m_iHitDebounce = pm->cmd.serverTime + 200;
				trap->FX_PlayEffectID( pSelfVeh->m_pVehicleInfo->iImpactFX, &pm->ps->origin, &vehUp, -1, -1, qfalse );

				pSelfVeh->m_ulFlags |= VEH_CRASHING;
			}
#endif
		}
	}
}

qboolean PM_GroundSlideOkay( float zNormal ) {
	if ( zNormal > 0 && pm->ps->velocity.z > 0 ) {
		if ( pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT
			|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT 
			|| pm->ps->legsAnim == BOTH_WALL_RUN_RIGHT_STOP
			|| pm->ps->legsAnim == BOTH_WALL_RUN_LEFT_STOP 
			|| pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START 
			|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_START
			|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_ATTACK
			|| pm->ps->legsAnim == BOTH_FORCELONGLEAP_LAND
			|| BG_InReboundJump( pm->ps->legsAnim ) )
		{
			return qfalse;
		}
	}

	return qtrue;
}

#ifdef _GAME
	extern void Client_CheckImpactBBrush( gentity_t *self, gentity_t *other );
#endif

// returns qtrue if the entity should be collided against
qboolean PM_ClientImpact( trace_t *trace ) {
	//don't try to predict this
#ifdef _GAME
	gentity_t *traceEnt;
#endif
	int entityNum = trace->entityNum;

	if ( !pm_entSelf )
		return qfalse;

	if ( entityNum >= ENTITYNUM_WORLD )
		return qfalse;

#ifdef _GAME
	traceEnt = &g_entities[entityNum];
	if ( VectorLength( &pm->ps->velocity ) >= 100 && pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->lastOnGround+100 < level.time
		/*&& pm->ps->groundEntityNum == ENTITYNUM_NONE*/ )
	{
		Client_CheckImpactBBrush( (gentity_t *)pm_entSelf, traceEnt );
	}

	// it's dead or not in my way anymore, don't clip against it
	if ( !traceEnt || !(traceEnt->r.contents & pm->tracemask) )
		return qtrue;
#endif

#ifdef _GAME
	//Raz: Ghosts
	if ( ((gentity_t *)pm_entSelf)->client->pers.adminData.isGhost )
		return qtrue;
#endif

	if ( entityNum <= MAX_CLIENTS ) {
		qboolean selfDueling = pm->ps->duelInProgress;
		int selfDuelist = pm->ps->duelIndex;
		int selfNum = pm_entSelf->s.number;
#ifdef _GAME
		gentity_t *other = &g_entities[entityNum];
		qboolean themDueling = other->client->ps.duelInProgress;
		int themDuelist = other->client->ps.duelIndex;
#else // _CGAME
		qboolean themDueling = cg_entities[entityNum].currentState.bolt1;
		int themDuelist = 9001; // pretend they're never dueling with us
#endif

		// we're dueling, but not with them
		if ( selfDueling && entityNum != selfDuelist )
			return qtrue;

		// they're dueling, but not with us
		if ( themDueling && themDuelist != selfNum )
			return qtrue;
	}

	return qfalse;
}

/*
==================
PM_SlideMove

Returns qtrue if the velocity was clipped in some way
==================
*/
#define	MAX_CLIP_PLANES	5
qboolean PM_SlideMove( qboolean gravity ) {
	int bumpcount, numbumps;
	vector3 dir;
	float d;
	int numplanes;
	vector3 normal, planes[MAX_CLIP_PLANES];
	vector3 primal_velocity, clipVelocity;
	int i, j, k;
	trace_t trace;
	vector3 end;
	float time_left, into;
	vector3 endVelocity, endClipVelocity;
//	qboolean damageSelf = qtrue;
	
	numbumps = 4;

	VectorCopy( &pm->ps->velocity, &primal_velocity );
	VectorCopy( &pm->ps->velocity, &endVelocity );

	if ( gravity ) {
		endVelocity.z -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity.z = ( pm->ps->velocity.z + endVelocity.z ) * 0.5;
		primal_velocity.z = endVelocity.z;
		if ( pml.groundPlane ) {
			// slide along the ground plane
			if ( PM_GroundSlideOkay( pml.groundTrace.plane.normal.z ) )
				PM_ClipVelocity (&pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane ) {
		numplanes = 1;
		VectorCopy( &pml.groundTrace.plane.normal, &planes[0] );
		if ( !PM_GroundSlideOkay( planes[0].z ) ) {
			planes[0].z = 0;
			VectorNormalize( &planes[0] );
		}
	}
	else
		numplanes = 0;

	// never turn against original velocity
	VectorNormalize2( &pm->ps->velocity, &planes[numplanes] );
	numplanes++;

	for ( bumpcount=0; bumpcount<numbumps; bumpcount++ ) {
		// calculate position we are trying to move to
		VectorMA( &pm->ps->origin, time_left, &pm->ps->velocity, &end );

		// see if we can make it there
		pm->trace ( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &end, pm->ps->clientNum, pm->tracemask);

		if ( trace.allsolid ) {
			// entity is completely trapped in another solid
			pm->ps->velocity.z = 0;	// don't build up falling damage, but allow sideways acceleration
			return qtrue;
		}

		// actually covered some distance
		if ( trace.fraction > 0 )
			VectorCopy( &trace.endpos, &pm->ps->origin );

		// moved the entire distance
		if ( trace.fraction == 1 )
			 break;

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		if ( pm->ps->clientNum >= MAX_CLIENTS ) {
			bgEntity_t *pEnt = pm_entSelf;

			// do vehicle impact stuff then
			if ( pEnt && pEnt->s.eType == ET_NPC && pEnt->s.NPC_class == CLASS_VEHICLE && pEnt->m_pVehicle )
				PM_VehicleImpact( pEnt, &trace );
		}
		else if ( PM_ClientImpact( &trace ) )
			continue;

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			VectorClear( &pm->ps->velocity );
			return qtrue;
		}

		VectorCopy( &trace.plane.normal, &normal );

		// wall-running
		if ( !PM_GroundSlideOkay( normal.z ) ) {
			//never push up off a sloped wall
			normal.z = 0;
			VectorNormalize( &normal );
		}

		// if this is the same plane we hit before, nudge velocity out along it, which fixes some epsilon issues with
		//	non-axial planes
		// no sliding if stuck to wall!
		if ( !(pm->ps->pm_flags&PMF_STUCK_TO_WALL) ) {
			for ( i=0; i<numplanes; i++ ) {
				if ( VectorCompare( &normal, &planes[i] ) ) {
					VectorAdd( &normal, &pm->ps->velocity, &pm->ps->velocity );
					break;
				}
			}
			if ( i < numplanes )
				continue;
		}
		VectorCopy( &normal, &planes[numplanes] );
		numplanes++;

		// modify velocity so it parallels all of the clip planes

		// find a plane that it enters
		for ( i=0; i<numplanes; i++ ) {
			into = DotProduct( &pm->ps->velocity, &planes[i] );

			// move doesn't interact with the plane
			if ( into >= 0.1f )
				continue;

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed )
				pml.impactSpeed = -into;

			// slide along the plane
			PM_ClipVelocity( &pm->ps->velocity, &planes[i], &clipVelocity, OVERCLIP );

			// slide along the plane
			PM_ClipVelocity( &endVelocity, &planes[i], &endClipVelocity, OVERCLIP );

			// see if there is a second plane that the new move enters
			for ( j=0; j<numplanes; j++ ) {
				if ( j == i )
					continue;

				// move doesn't interact with the plane
				if ( DotProduct( &clipVelocity, &planes[j] ) >= 0.1f )
					continue;

				// try clipping the move to the plane
				PM_ClipVelocity( &clipVelocity, &planes[j], &clipVelocity, OVERCLIP );
				PM_ClipVelocity( &endClipVelocity, &planes[j], &endClipVelocity, OVERCLIP );

				// see if it goes back into the first clip plane
				if ( DotProduct( &clipVelocity, &planes[i] ) >= 0 )
					continue;

				// slide the original velocity along the crease
				CrossProduct( &planes[i], &planes[j], &dir );
				VectorNormalize( &dir );
				d = DotProduct( &dir, &pm->ps->velocity );
				VectorScale( &dir, d, &clipVelocity );

				CrossProduct( &planes[i], &planes[j], &dir );
				VectorNormalize( &dir );
				d = DotProduct( &dir, &endVelocity );
				VectorScale( &dir, d, &endClipVelocity );

				// see if there is a third plane the the new move enters
				for ( k=0; k<numplanes; k++ ) {
					if ( k == i || k == j )
						continue;

					// move doesn't interact with the plane
					if ( DotProduct( &clipVelocity, &planes[k] ) >= 0.1f )
						continue;

					// stop dead at a triple plane interaction
					VectorClear( &pm->ps->velocity );
					return qtrue;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( &clipVelocity, &pm->ps->velocity );
			VectorCopy( &endClipVelocity, &endVelocity );
			break;
		}
	}

	if ( gravity )
		VectorCopy( &endVelocity, &pm->ps->velocity );

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time )
		VectorCopy( &primal_velocity, &pm->ps->velocity );

	return ( bumpcount != 0 );
}

void PM_StepSlideMove( qboolean gravity ) { 
	vector3 start_o, start_v, down_o, down_v;
	trace_t trace;
	vector3 up, down;
	float stepSize, delta;
	qboolean isGiant = qfalse, skipStep = qfalse;
	bgEntity_t *pEnt;

	VectorCopy( &pm->ps->origin, &start_o );
	VectorCopy( &pm->ps->velocity, &start_v );

	if ( BG_InReboundHold( pm->ps->legsAnim ) )
		gravity = qfalse;

	// we got exactly where we wanted to go first try
	if ( !PM_SlideMove( gravity ) )
		return;

	pEnt = pm_entSelf;

	if ( pm->ps->clientNum >= MAX_CLIENTS && pEnt && pEnt->s.NPC_class == CLASS_VEHICLE && pEnt->m_pVehicle
		&& pEnt->m_pVehicle->m_pVehicleInfo->hoverHeight > 0 )
	{
			return;
	}

	VectorCopy( &start_o, &down );
	down.z -= STEPSIZE;
	pm->trace( &trace, &start_o, &pm->mins, &pm->maxs, &down, pm->ps->clientNum, pm->tracemask );
	VectorSet( &up, 0, 0, 1 );
	// never step up when you still have up velocity
	if ( pm->ps->velocity.z > 0 && (trace.fraction == 1.0 || DotProduct( &trace.plane.normal, &up ) < 0.7f) )
		return;

	VectorCopy( &pm->ps->origin, &down_o );
	VectorCopy( &pm->ps->velocity, &down_v );

	VectorCopy( &start_o, &up );

	if ( pm->ps->clientNum >= MAX_CLIENTS ) {
		// apply ground friction, even if on ladder
		if ( pEnt && (pEnt->s.NPC_class == CLASS_ATST || (pEnt->s.NPC_class == CLASS_VEHICLE && pEnt->m_pVehicle && pEnt->m_pVehicle->m_pVehicleInfo->type == VH_WALKER)) )
		{// AT-STs can step high
			up.z += 66.0f;
			isGiant = qtrue;
		}
		// also can step up high
		else if ( pEnt && pEnt->s.NPC_class == CLASS_RANCOR ) {
			up.z += 64.0f;
			isGiant = qtrue;
		}
		else
			up.z += STEPSIZE;
	}
	else
		up.z += STEPSIZE;

	// test the player position if they were a stepheight higher
	pm->trace( &trace, &start_o, &pm->mins, &pm->maxs, &up, pm->ps->clientNum, pm->tracemask );
	if ( trace.allsolid ) {
		// can't step up
		if ( pm->debugLevel )
			Com_Printf( "%i:bend can't step\n", c_pmove );
		return;
	}

	stepSize = trace.endpos.z - start_o.z;
	// try slidemove from this position
	VectorCopy( &trace.endpos, &pm->ps->origin );
	VectorCopy( &start_v, &pm->ps->velocity );

	PM_SlideMove( gravity );

	// push down the final amount
	VectorCopy( &pm->ps->origin, &down );
	down.z -= stepSize;
	pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &down, pm->ps->clientNum, pm->tracemask );

	if ( pm->stepSlideFix ) {
		if ( pm->ps->clientNum < MAX_CLIENTS && trace.plane.normal.z < MIN_WALK_NORMAL ) {
			// normal players cannot step up slopes that are too steep to walk on!
			vector3 stepVec;
			//okay, the step up ends on a slope that it too steep to step up onto,
			//BUT:
			//If the step looks like this:
			//  (B)\__
			//        \_____(A)
			//Then it might still be okay, so we figure out the slope of the entire move
			//from (A) to (B) and if that slope is walk-upabble, then it's okay
			VectorSubtract( &trace.endpos, &down_o, &stepVec );
			VectorNormalize( &stepVec ); 
			if ( stepVec.z > (1.0f-MIN_WALK_NORMAL) )
				skipStep = qtrue;
		}
	}

	// normal players cannot step up slopes that are too steep to walk on!
	if ( !trace.allsolid && !skipStep ) {
		// Rancor don't step on clients
		if ( pm->ps->clientNum >= MAX_CLIENTS && isGiant && trace.entityNum < MAX_CLIENTS && pEnt
			&& pEnt->s.NPC_class == CLASS_RANCOR )
		{
			if ( pm->stepSlideFix ) {
				VectorCopy( &down_o, &pm->ps->origin );
				VectorCopy( &down_v, &pm->ps->velocity );
			}
			else {
				VectorCopy( &start_o, &pm->ps->origin );
				VectorCopy( &start_v, &pm->ps->velocity );
			}
		}
		else {
			VectorCopy( &trace.endpos, &pm->ps->origin );
			if ( pm->stepSlideFix && trace.fraction < 1.0 )
				PM_ClipVelocity( &pm->ps->velocity, &trace.plane.normal, &pm->ps->velocity, OVERCLIP );
		}
	}
	else {
		if ( pm->stepSlideFix ) {
			VectorCopy( &down_o, &pm->ps->origin );
			VectorCopy( &down_v, &pm->ps->velocity );
		}
	}
	if ( !pm->stepSlideFix && trace.fraction < 1.0 )
		PM_ClipVelocity( &pm->ps->velocity, &trace.plane.normal, &pm->ps->velocity, OVERCLIP );

	// use the step move
	delta = pm->ps->origin.z - start_o.z;
	if ( delta > 2 ) {
			 if ( delta < 7 )	PM_AddEvent( EV_STEP_4 );
		else if ( delta < 11 )	PM_AddEvent( EV_STEP_8 );
		else if ( delta < 15 )	PM_AddEvent( EV_STEP_12 );
		else					PM_AddEvent( EV_STEP_16 );
	}
	if ( pm->debugLevel )
		Com_Printf( "%i:stepped\n", c_pmove );
}
