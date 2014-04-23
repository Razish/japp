// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "Ghoul2/G2.h"

#ifdef _GAME
#include "g_local.h" //ahahahahhahahaha@$!$!
#endif

#ifdef _CGAME
#include "cg_local.h"
#endif

#include "JAPP/jp_promode.h"

#define MAX_WEAPON_CHARGE_TIME 5000

#ifdef _GAME
extern void G_CheapWeaponFire( int entNum, int ev );
extern qboolean TryGrapple( gentity_t *ent ); //g_cmds.c
#endif

extern qboolean BG_FullBodyTauntAnim( int anim );
extern float PM_WalkableGroundDistance( void );
extern qboolean PM_GroundSlideOkay( float zNormal );
extern saberInfo_t *BG_MySaber( int clientNum, int saberNum );

pmove_t		*pm;
pml_t		pml;

bgEntity_t *pm_entSelf = NULL;
bgEntity_t *pm_entVeh = NULL;

qboolean gPMDoSlowFall = qfalse;

qboolean pm_cancelOutZoom = qfalse;

// movement parameters
float	pm_stopspeed = 100.0f;
static float PM_DuckScale( void ) {
	return GetCInfo( CINFO_VQ3PHYS ) ? 0.25f : 0.50f;
}
float	pm_swimScale = 0.50f;
float	pm_wadeScale = 0.70f;

float	pm_vehicleaccelerate = 36.0f;
float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 8.0f;

float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 3.0f;
float	pm_spectatorfriction = 5.0f;

int		c_pmove = 0;

float forceSpeedLevels[4] =
{
	1, //rank 0?
	1.25f,
	1.5f,
	1.75f
};

int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS] =
{
	{ //nothing should be usable at rank 0..
		999,//FP_HEAL,//instant
		999,//FP_LEVITATION,//hold/duration
		999,//FP_SPEED,//duration
		999,//FP_PUSH,//hold/duration
		999,//FP_PULL,//hold/duration
		999,//FP_TELEPATHY,//instant
		999,//FP_GRIP,//hold/duration
		999,//FP_LIGHTNING,//hold/duration
		999,//FP_RAGE,//duration
		999,//FP_PROTECT,//duration
		999,//FP_ABSORB,//duration
		999,//FP_TEAM_HEAL,//instant
		999,//FP_TEAM_FORCE,//instant
		999,//FP_DRAIN,//hold/duration
		999,//FP_SEE,//duration
		999,//FP_SABER_OFFENSE,
		999,//FP_SABER_DEFENSE,
		999//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		65,//FP_HEAL,//instant //was 25, but that was way too little
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		50,//FP_PROTECT,//duration
		50,//FP_ABSORB,//duration
		50,//FP_TEAM_HEAL,//instant
		50,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABER_OFFENSE,
		2,//FP_SABER_DEFENSE,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		60,//FP_HEAL,//instant
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		30,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		25,//FP_PROTECT,//duration
		25,//FP_ABSORB,//duration
		33,//FP_TEAM_HEAL,//instant
		33,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABER_OFFENSE,
		1,//FP_SABER_DEFENSE,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	},
	{
		50,//FP_HEAL,//instant //You get 5 points of health.. for 50 force points!
		10,//FP_LEVITATION,//hold/duration
		50,//FP_SPEED,//duration
		20,//FP_PUSH,//hold/duration
		20,//FP_PULL,//hold/duration
		20,//FP_TELEPATHY,//instant
		60,//FP_GRIP,//hold/duration
		1,//FP_LIGHTNING,//hold/duration
		50,//FP_RAGE,//duration
		10,//FP_PROTECT,//duration
		10,//FP_ABSORB,//duration
		25,//FP_TEAM_HEAL,//instant
		25,//FP_TEAM_FORCE,//instant
		20,//FP_DRAIN,//hold/duration
		20,//FP_SEE,//duration
		0,//FP_SABER_OFFENSE,
		0,//FP_SABER_DEFENSE,
		20//FP_SABERTHROW,
		//NUM_FORCE_POWERS
	}
};

float forceJumpHeight[NUM_FORCE_POWER_LEVELS] =
{
	32,//normal jump (+stepheight+crouchdiff = 66)
	96,//(+stepheight+crouchdiff = 130)
	192,//(+stepheight+crouchdiff = 226)
	384//(+stepheight+crouchdiff = 418)
};

float forceJumpStrength[NUM_FORCE_POWER_LEVELS] =
{
	JUMP_VELOCITY,//normal jump
	420,
	590,
	840
};

//rww - Get a pointer to the bgEntity by the index
bgEntity_t *PM_BGEntForNum( int num ) {
	bgEntity_t *ent;

	if ( !pm ) {
		assert( !"You cannot call PM_BGEntForNum outside of pm functions!" );
		return NULL;
	}

	if ( !pm->baseEnt ) {
		assert( !"Base entity address not set" );
		return NULL;
	}

	if ( !pm->entSize ) {
		assert( !"sizeof(ent) is 0, impossible (not set?)" );
		return NULL;
	}

	assert( num >= 0 && num < MAX_GENTITIES );

	ent = (bgEntity_t *)((byte *)pm->baseEnt + pm->entSize*(num));

	return ent;
}

qboolean BG_SabersOff( playerState_t *ps ) {
	if ( !ps->saberHolstered ) {
		return qfalse;
	}
	if ( ps->fd.saberAnimLevelBase == SS_DUAL
		|| ps->fd.saberAnimLevelBase == SS_STAFF ) {
		if ( ps->saberHolstered < 2 ) {
			return qfalse;
		}
	}
	return qtrue;
}

qboolean BG_KnockDownable( playerState_t *ps ) {
	if ( !ps ) { //just for safety
		return qfalse;
	}

	if ( ps->m_iVehicleNum ) { //riding a vehicle, don't knock me down
		return qfalse;
	}

	if ( ps->emplacedIndex ) { //using emplaced gun or eweb, can't be knocked down
		return qfalse;
	}

	//ok, I guess?
	return qtrue;
}

//hacky assumption check, assume any client non-humanoid is a rocket trooper
qboolean PM_IsRocketTrooper( void ) {
	/*
	if (pm->ps->clientNum < MAX_CLIENTS &&
	pm->gametype == GT_SIEGE &&
	pm->nonHumanoid)
	{
	return qtrue;
	}
	*/

	return qfalse;
}

int PM_GetSaberStance( void ) {
	int anim = BOTH_STAND2;
	saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
	saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );

	if ( !pm->ps->saberEntityNum ) { //lost it
		return BOTH_STAND1;
	}

	if ( BG_SabersOff( pm->ps ) ) {
		return BOTH_STAND1;
	}

	if ( saber1
		&& saber1->readyAnim != -1 ) {
		return saber1->readyAnim;
	}

	if ( saber2
		&& saber2->readyAnim != -1 ) {
		return saber2->readyAnim;
	}

	if ( saber1
		&& saber2
		&& !pm->ps->saberHolstered ) {//dual sabers, both on
		return BOTH_SABERDUAL_STANCE;
	}

	switch ( pm->ps->fd.saberAnimLevel ) {
	case SS_DUAL:
		anim = BOTH_SABERDUAL_STANCE;
		break;
	case SS_STAFF:
		anim = BOTH_SABERSTAFF_STANCE;
		break;
	case SS_FAST:
	case SS_TAVION:
		anim = BOTH_SABERFAST_STANCE;
		break;
	case SS_STRONG:
		anim = BOTH_SABERSLOW_STANCE;
		break;
	case SS_NONE:
	case SS_MEDIUM:
	case SS_DESANN:
	default:
		anim = BOTH_STAND2;
		break;
	}
	return anim;
}

qboolean PM_DoSlowFall( void ) {
	if ( ((pm->ps->legsAnim) == BOTH_WALL_RUN_RIGHT || (pm->ps->legsAnim) == BOTH_WALL_RUN_LEFT) && pm->ps->legsTimer > 500 ) {
		return qtrue;
	}

	return qfalse;
}

//begin vehicle functions crudely ported from sp -rww
/*
====================================================================
void pitch_roll_for_slope (edict_t *forwhom, vector3 *slope, vector3 *storeAngles )

MG

This will adjust the pitch and roll of a monster to match
a given slope - if a non-'0 0 0' slope is passed, it will
use that value, otherwise it will use the ground underneath
the monster.  If it doesn't find a surface, it does nothinh\g
and returns.
====================================================================
*/

void PM_pitch_roll_for_slope( bgEntity_t *forwhom, vector3 *pass_slope, vector3 *storeAngles ) {
	vector3	slope;
	vector3	nvf, ovf, ovr, startspot, endspot, new_angles = { 0, 0, 0 };
	float	pitch, mod, dot;

	//if we don't have a slope, get one
	if ( !pass_slope || VectorCompare( &vec3_origin, pass_slope ) ) {
		trace_t trace;

		VectorCopy( &pm->ps->origin, &startspot );
		startspot.z += pm->mins.z + 4;
		VectorCopy( &startspot, &endspot );
		endspot.z -= 300;
		pm->trace( &trace, &pm->ps->origin, &vec3_origin, &vec3_origin, &endspot, forwhom->s.number, MASK_SOLID );
		//		if(trace_fraction>0.05f&&forwhom.movetype==MOVETYPE_STEP)
		//			forwhom.flags(-)FL_ONGROUND;

		if ( trace.fraction >= 1.0f )
			return;

		if ( VectorCompare( &vec3_origin, &trace.plane.normal ) )
			return;

		VectorCopy( &trace.plane.normal, &slope );
	}
	else {
		VectorCopy( pass_slope, &slope );
	}

	if ( forwhom->s.NPC_class == CLASS_VEHICLE ) {//special code for vehicles
		Vehicle_t *pVeh = forwhom->m_pVehicle;
		vector3 tempAngles;

		tempAngles.pitch = tempAngles.roll = 0;
		tempAngles.yaw = pVeh->m_vOrientation->yaw;
		AngleVectors( &tempAngles, &ovf, &ovr, NULL );
	}
	else {
		AngleVectors( &pm->ps->viewangles, &ovf, &ovr, NULL );
	}

	vectoangles( &slope, &new_angles );
	pitch = new_angles.pitch + 90;
	new_angles.roll = new_angles.pitch = 0;

	AngleVectors( &new_angles, &nvf, NULL, NULL );

	mod = DotProduct( &nvf, &ovr );

	if ( mod < 0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( &nvf, &ovf );

	if ( storeAngles ) {
		storeAngles->pitch = dot * pitch;
		storeAngles->roll = ((1 - Q_fabs( dot )) * pitch * mod);
	}
	else //if ( forwhom->client )
	{
		float oldmins2;

		pm->ps->viewangles.pitch = dot * pitch;
		pm->ps->viewangles.roll = ((1 - Q_fabs( dot )) * pitch * mod);
		oldmins2 = pm->mins.z;
		pm->mins.z = -24 + 12 * fabsf( pm->ps->viewangles.pitch ) / 180.0f;
		//FIXME: if it gets bigger, move up
		if ( oldmins2 > pm->mins.z ) {//our mins is now lower, need to move up
			//FIXME: trace?
			pm->ps->origin.z += (oldmins2 - pm->mins.z);
			//forwhom->currentorigin.z = forwhom->client->ps.origin.z;
			//gi.linkentity( forwhom );
		}
	}
	/*
	else
	{
	forwhom->currentAngles.pitch = dot * pitch;
	forwhom->currentAngles.roll = ((1-Q_fabsf(dot)) * pitch * mod);
	}
	*/
}

#define		FLY_NONE	0
#define		FLY_NORMAL	1
#define		FLY_VEHICLE	2
#define		FLY_HOVER	3
static int pm_flying = FLY_NONE;

void PM_SetSpecialMoveValues( void ) {
	bgEntity_t *pEnt;

	if ( pm->ps->clientNum < MAX_CLIENTS ) { //we know that real players aren't vehs
		pm_flying = FLY_NONE;
		return;
	}

	//default until we decide otherwise
	pm_flying = FLY_NONE;

	pEnt = pm_entSelf;

	if ( pEnt ) {
		if ( (pm->ps->eFlags2&EF2_FLYING) )// pm->gent->client->moveType == MT_FLYSWIM )
		{
			pm_flying = FLY_NORMAL;
		}
		else if ( pEnt->s.NPC_class == CLASS_VEHICLE ) {
			if ( pEnt->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER ) {
				pm_flying = FLY_VEHICLE;
			}
			else if ( pEnt->m_pVehicle->m_pVehicleInfo->hoverHeight > 0 ) {
				pm_flying = FLY_HOVER;
			}
		}
	}
}

static void PM_SetVehicleAngles( vector3 *normal ) {
	bgEntity_t *pEnt = pm_entSelf;
	Vehicle_t *pVeh;
	vector3	vAngles;
	float vehicleBankingSpeed;
	float pitchBias;
	int i;

	if ( !pEnt || pEnt->s.NPC_class != CLASS_VEHICLE ) {
		return;
	}

	pVeh = pEnt->m_pVehicle;

	//float	curVehicleBankingSpeed;
	vehicleBankingSpeed = (pVeh->m_pVehicleInfo->bankingSpeed*32.0f)*pml.frametime;//0.25f

	if ( vehicleBankingSpeed <= 0
		|| (pVeh->m_pVehicleInfo->pitchLimit == 0 && pVeh->m_pVehicleInfo->rollLimit == 0) ) {//don't bother, this vehicle doesn't bank
		return;
	}
	//FIXME: do 3 traces to define a plane and use that... smoothes it out some, too...
	//pitch_roll_for_slope( pm->gent, normal, vAngles );
	//FIXME: maybe have some pitch control in water and/or air?

	if ( pVeh->m_pVehicleInfo->type == VH_FIGHTER ) {
		pitchBias = 0.0f;
	}
	else {
		//FIXME: gravity does not matter in SPACE!!!
		//center of gravity affects pitch in air/water (FIXME: what about roll?)
		pitchBias = 90.0f*pVeh->m_pVehicleInfo->centerOfGravity.data[0];//if centerOfGravity is all the way back (-1.0f), vehicle pitches up 90 degrees when in air
	}

	VectorClear( &vAngles );
	if ( pm->waterlevel > 0 ) {//in water
		//view pitch has some influence when in water
		//FIXME: take center of gravity into account?
		vAngles.pitch += (pm->ps->viewangles.pitch - vAngles.pitch)*0.75f + (pitchBias*0.5f);
	}
	else if ( normal ) {//have a valid surface below me
		PM_pitch_roll_for_slope( pEnt, normal, &vAngles );
		if ( (pml.groundTrace.contents&(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) ) {//on water
			//view pitch has some influence when on a fluid surface
			//FIXME: take center of gravity into account
			vAngles.pitch += (pm->ps->viewangles.pitch - vAngles.pitch)*0.5f + (pitchBias*0.5f);
		}
	}
	else {//in air, let pitch match view...?
		//FIXME: take center of gravity into account
		vAngles.pitch = pm->ps->viewangles.pitch*0.5f + pitchBias;
		//don't bank so fast when in the air
		vehicleBankingSpeed *= (0.125f*pml.frametime);
	}
	//NOTE: if angles are flat and we're moving through air (not on ground),
	//		then pitch/bank?
	if ( pVeh->m_pVehicleInfo->rollLimit > 0 ) {
		//roll when banking
		vector3	velocity;
		float	speed;
		VectorCopy( &pm->ps->velocity, &velocity );
		velocity.data[2] = 0.0f;
		speed = VectorNormalize( &velocity );
		if ( speed > 32.0f || speed < -32.0f ) {
			vector3	rt, tempVAngles;
			float	side;
			float	dp;

			// Magic number fun!  Speed is used for banking, so modulate the speed by a sine wave
			//FIXME: this banks too early
			speed *= sinf( (150 + pml.frametime) * 0.003f );

			// Clamp to prevent harsh rolling
			if ( speed > 60 )
				speed = 60;

			VectorCopy( pVeh->m_vOrientation, &tempVAngles );
			tempVAngles.roll = 0;
			AngleVectors( &tempVAngles, NULL, &rt, NULL );
			dp = DotProduct( &velocity, &rt );
			side = speed * dp;
			vAngles.roll -= side;
		}
	}

	//cap
	if ( pVeh->m_pVehicleInfo->pitchLimit != -1 ) {
		if ( vAngles.pitch > pVeh->m_pVehicleInfo->pitchLimit )
			vAngles.pitch = pVeh->m_pVehicleInfo->pitchLimit;
		else if ( vAngles.pitch < -pVeh->m_pVehicleInfo->pitchLimit )
			vAngles.pitch = -pVeh->m_pVehicleInfo->pitchLimit;
	}

	if ( vAngles.roll > pVeh->m_pVehicleInfo->rollLimit )
		vAngles.roll = pVeh->m_pVehicleInfo->rollLimit;
	else if ( vAngles.roll < -pVeh->m_pVehicleInfo->rollLimit )
		vAngles.roll = -pVeh->m_pVehicleInfo->rollLimit;

	//do it
	for ( i = 0; i < 3; i++ ) {
		if ( i == 2/*YAW*/ ) {//yawing done elsewhere
			continue;
		}
		//bank faster the higher the difference is
#if 0
		else if ( i == 0/*PITCH*/ )
			curVehicleBankingSpeed = vehicleBankingSpeed*fabsf(AngleNormalize180(AngleSubtract( vAngles.pitch, pVeh->m_vOrientation->pitch )))/(g_vehicleInfo[pm->ps->vehicleIndex].pitchLimit/2.0f);
		else if ( i == 2/*ROLL*/ )
			curVehicleBankingSpeed = vehicleBankingSpeed*fabsf(AngleNormalize180(AngleSubtract( vAngles.roll, pVeh->m_vOrientation->roll )))/(g_vehicleInfo[pm->ps->vehicleIndex].rollLimit/2.0f);

		if ( curVehicleBankingSpeed )
#endif
		{
			if ( pVeh->m_vOrientation->data[i] >= vAngles.data[i] + vehicleBankingSpeed )	pVeh->m_vOrientation->data[i] -= vehicleBankingSpeed;
			else if ( pVeh->m_vOrientation->data[i] <= vAngles.data[i] - vehicleBankingSpeed )	pVeh->m_vOrientation->data[i] += vehicleBankingSpeed;
			else																				pVeh->m_vOrientation->data[i] = vAngles.data[i];
		}
	}
}

void BG_VehicleTurnRateForSpeed( Vehicle_t *pVeh, float speed, float *mPitchOverride, float *mYawOverride ) {
	if ( pVeh && pVeh->m_pVehicleInfo ) {
		float speedFrac = 1.0f;
		if ( pVeh->m_pVehicleInfo->speedDependantTurning ) {
			if ( pVeh->m_LandTrace.fraction >= 1.0f
				|| pVeh->m_LandTrace.plane.normal.z < MIN_LANDING_SLOPE ) {
				speedFrac = (speed / (pVeh->m_pVehicleInfo->speedMax*0.75f));
				if ( speedFrac < 0.25f )
					speedFrac = 0.25f;
				else if ( speedFrac > 1.0f )
					speedFrac = 1.0f;
			}
		}
		if ( pVeh->m_pVehicleInfo->mousePitch )
			*mPitchOverride = pVeh->m_pVehicleInfo->mousePitch*speedFrac;
		if ( pVeh->m_pVehicleInfo->mouseYaw )
			*mYawOverride = pVeh->m_pVehicleInfo->mouseYaw*speedFrac;
	}
}

static void PM_GroundTraceMissed( void );
void PM_HoverTrace( void ) {
	Vehicle_t *pVeh;
	float hoverHeight;
	vector3		point, vAng, fxAxis[3];
	trace_t		*trace;
	float relativeWaterLevel;

	bgEntity_t *pEnt = pm_entSelf;
	if ( !pEnt || pEnt->s.NPC_class != CLASS_VEHICLE ) {
		return;
	}

	pVeh = pEnt->m_pVehicle;
	hoverHeight = pVeh->m_pVehicleInfo->hoverHeight;
	trace = &pml.groundTrace;

	pml.groundPlane = qfalse;

	//relativeWaterLevel = (pm->ps->waterheight - (pm->ps->origin.z+pm->mins.z));
	relativeWaterLevel = pm->waterlevel; //I.. guess this works
	if ( pm->waterlevel && relativeWaterLevel >= 0 ) {//in water
		if ( pVeh->m_pVehicleInfo->bouyancy <= 0.0f ) {//sink like a rock
		}
		else {//rise up
			float floatHeight = (pVeh->m_pVehicleInfo->bouyancy * ((pm->maxs.z - pm->mins.z)*0.5f)) - (hoverHeight*0.5f);//1.0f should make you float half-in, half-out of water
			if ( relativeWaterLevel > floatHeight ) {//too low, should rise up
				pm->ps->velocity.z += (relativeWaterLevel - floatHeight) * pVeh->m_fTimeModifier;
			}
		}
		//if ( pm->ps->waterheight < pm->ps->origin.z+pm->maxs[2] )
		if ( pm->waterlevel <= 1 ) {//part of us is sticking out of water
			if ( fabsf( pm->ps->velocity.x ) + fabsf( pm->ps->velocity.y ) > 100 ) {//moving at a decent speed
				if ( Q_irand( pml.frametime, 100 ) >= 50 ) {//splash
					vector3 wakeOrg;

					vAng.pitch = vAng.roll = 0;
					vAng.yaw = pVeh->m_vOrientation->yaw;
					AngleVectors( &vAng, &fxAxis[2], &fxAxis[1], &fxAxis[0] );
					VectorCopy( &pm->ps->origin, &wakeOrg );
					//wakeOrg[2] = pm->ps->waterheight;
					if ( pm->waterlevel >= 2 ) {
						wakeOrg.z = pm->ps->origin.z + 16;
					}
					else {
						wakeOrg.z = pm->ps->origin.z;
					}
#ifdef _GAME //yeah, this is kind of crappy and makes no use of prediction whatsoever
					if ( pVeh->m_pVehicleInfo->iWakeFX ) {
						//G_PlayEffectID( pVeh->m_pVehicleInfo->iWakeFX, wakeOrg, fxAxis[0] );
						//tempent use bad!
						G_AddEvent( (gentity_t *)pEnt, EV_PLAY_EFFECT_ID, pVeh->m_pVehicleInfo->iWakeFX );
					}
#endif
				}
			}
		}
	}
	else {
		int traceContents;
		float minNormal = pVeh->m_pVehicleInfo->maxSlope;

		point.x = pm->ps->origin.x;
		point.y = pm->ps->origin.y;
		point.z = pm->ps->origin.z - hoverHeight;

		//FIXME: check for water, too?  If over water, go slower and make wave effect
		//		If *in* water, go really slow and use bouyancy stat to determine how far below surface to float

		//NOTE: if bouyancy is 2.0f or higher, you float over water like it's solid ground.
		//		if it's 1.0f, you sink halfway into water.  If it's 0, you sink...
		traceContents = pm->tracemask;
		if ( pVeh->m_pVehicleInfo->bouyancy >= 2.0f ) {//sit on water
			traceContents |= (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA);
		}
		pm->trace( trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, traceContents );
		if ( trace->plane.normal.x > 0.5f || trace->plane.normal.x < -0.5f ||
			trace->plane.normal.y > 0.5f || trace->plane.normal.y < -0.5f ) { //steep slanted hill, don't go up it.
			float d = fabsf( trace->plane.normal.x );
			float e = fabsf( trace->plane.normal.y );
			if ( e > d ) {
				d = e;
			}
			pm->ps->velocity.z = -300.0f*d;
		}
		else if ( trace->plane.normal.z >= minNormal ) {//not a steep slope, so push us up
			if ( trace->fraction < 1.0f ) {//push up off ground
				float hoverForce = pVeh->m_pVehicleInfo->hoverStrength;
				if ( trace->fraction > 0.5f ) {
					pm->ps->velocity.z += (1.0f - trace->fraction)*hoverForce*pVeh->m_fTimeModifier;
				}
				else {
					pm->ps->velocity.z += (0.5f - (trace->fraction*trace->fraction))*hoverForce*2.0f*pVeh->m_fTimeModifier;
				}
				if ( (trace->contents&(CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) ) {//hovering on water, make a spash if moving
					if ( fabsf( pm->ps->velocity.x ) + fabsf( pm->ps->velocity.y ) > 100 ) {//moving at a decent speed
						if ( Q_irand( pml.frametime, 100 ) >= 50 ) {//splash
							vAng.pitch = vAng.roll = 0;
							vAng.yaw = pVeh->m_vOrientation->yaw;
							AngleVectors( &vAng, &fxAxis[2], &fxAxis[1], &fxAxis[0] );
#ifdef _GAME
							if ( pVeh->m_pVehicleInfo->iWakeFX ) {
								G_PlayEffectID( pVeh->m_pVehicleInfo->iWakeFX, &trace->endpos, &fxAxis[0] );
							}
#endif
						}
					}
				}
				pml.groundPlane = qtrue;
			}
		}
	}
	if ( pml.groundPlane ) {
		PM_SetVehicleAngles( &pml.groundTrace.plane.normal );
		// We're on the ground.
		pVeh->m_ulFlags &= ~VEH_FLYING;

		pVeh->m_vAngularVelocity = 0.0f;
	}
	else {
		PM_SetVehicleAngles( NULL );
		// We're flying in the air.
		pVeh->m_ulFlags |= VEH_FLYING;
		//groundTrace

		if ( pVeh->m_vAngularVelocity == 0.0f ) {
			pVeh->m_vAngularVelocity = pVeh->m_vOrientation->yaw - pVeh->m_vPrevOrientation.yaw;
			if ( pVeh->m_vAngularVelocity<-15.0f ) {
				pVeh->m_vAngularVelocity = -15.0f;
			}
			if ( pVeh->m_vAngularVelocity> 15.0f ) {
				pVeh->m_vAngularVelocity = 15.0f;
			}
		}
		//pVeh->m_vAngularVelocity *= 0.95f;		// Angular Velocity Decays Over Time
		if ( pVeh->m_vAngularVelocity > 0.0f ) {
			pVeh->m_vAngularVelocity -= pml.frametime;
			if ( pVeh->m_vAngularVelocity < 0.0f ) {
				pVeh->m_vAngularVelocity = 0.0f;
			}
		}
		else if ( pVeh->m_vAngularVelocity < 0.0f ) {
			pVeh->m_vAngularVelocity += pml.frametime;
			if ( pVeh->m_vAngularVelocity > 0.0f ) {
				pVeh->m_vAngularVelocity = 0.0f;
			}
		}
	}
	PM_GroundTraceMissed();
}
//end vehicle functions crudely ported from sp -rww

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

void PM_AddEventWithParm( int newEvent, int parm ) {
	BG_AddPredictableEventToPlayerstate( newEvent, parm, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int i;

	if ( entityNum == ENTITYNUM_WORLD )
		return;

	if ( pm->numtouch >= MAXTOUCH )
		return;

#ifdef _GAME
	if ( ((gentity_t *)pm_entSelf)->client->pers.adminData.isGhost )
		return;
#endif

	// duel isolation
#if 0 // redundant, handled in g_active and cg_predict before Pmove
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
			return;

		// they're dueling, but not with us
		if ( themDueling && themDuelist != selfNum )
			return;
	}
#endif

	// see if it is already added
	for ( i = 0; i < pm->numtouch; i++ ) {
		if ( pm->touchents[i] == entityNum )
			return;
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vector3 *in, vector3 *normal, vector3 *out, float overbounce ) {
	float	backoff;
	float	change;
	float	oldInZ;
	int		i;

	if ( (pm->ps->pm_flags&PMF_STUCK_TO_WALL) ) {//no sliding!
		VectorCopy( in, out );
		return;
	}
	oldInZ = in->z;

	backoff = DotProduct( in, normal );

	if ( backoff < 0 ) {
		backoff *= overbounce;
	}
	else {
		backoff /= overbounce;
	}

	for ( i = 0; i < 3; i++ ) {
		change = normal->data[i] * backoff;
		out->data[i] = in->data[i] - change;
	}
	if ( pm->stepSlideFix ) {
		if ( pm->ps->clientNum < MAX_CLIENTS//normal player
			&& pm->ps->groundEntityNum != ENTITYNUM_NONE//on the ground
			&& normal->z < MIN_WALK_NORMAL )//sliding against a steep slope
		{//if walking on the ground, don't slide up slopes that are too steep to walk on
			out->z = oldInZ;
		}
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vector3	vec, *vel;
	float	speed, newspeed, control;
	float	drop;
	bgEntity_t *pEnt = NULL;

	vel = &pm->ps->velocity;

	VectorCopy( vel, &vec );
	if ( pml.walking ) {
		vec.z = 0;	// ignore slope movement
	}

	speed = VectorLength( &vec );
	if ( speed < 1 ) {
		vel->x = 0;
		vel->y = 0;		// allow sinking underwater
		if ( pm->ps->pm_type == PM_SPECTATOR ) {
			vel->z = 0;
		}
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	if ( pm->ps->clientNum >= MAX_CLIENTS ) {
		pEnt = pm_entSelf;
	}

	// apply ground friction, even if on ladder
	if ( pm_flying != FLY_VEHICLE &&
		pEnt &&
		pEnt->s.NPC_class == CLASS_VEHICLE &&
		pEnt->m_pVehicle &&
		pEnt->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL &&
		pEnt->m_pVehicle->m_pVehicleInfo->type != VH_WALKER &&
		pEnt->m_pVehicle->m_pVehicleInfo->friction ) {
		float friction = pEnt->m_pVehicle->m_pVehicleInfo->friction;
		if ( !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) /*&& !(pm->ps->pm_flags & PMF_TIME_NOFRICTION)*/ ) {
			control = speed < pm_stopspeed ? pm_stopspeed : speed;
			drop += control*friction*pml.frametime;
			/*
			if ( Flying == FLY_HOVER )
			{
			if ( pm->cmd.rightmove )
			{//if turning, increase friction
			control *= 2.0f;
			}
			if ( pm->ps->groundEntityNum < ENTITYNUM_NONE )
			{//on the ground
			drop += control*friction*pml.frametime;
			}
			else if ( pml.groundPlane )
			{//on a slope
			drop += control*friction*2.0f*pml.frametime;
			}
			else
			{//in air
			drop += control*2.0f*friction*pml.frametime;
			}
			}
			*/
		}
	}
	else if ( pm_flying != FLY_NORMAL && pm_flying != FLY_VEHICLE ) {
		// apply ground friction
		if ( pm->waterlevel <= 1 ) {
			if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
				// if getting knocked back, no friction
				if ( !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
					control = speed < pm_stopspeed ? pm_stopspeed : speed;
					drop += control*pm_friction*pml.frametime;
				}
			}
		}
	}

	if ( pm_flying == FLY_VEHICLE ) {
		if ( !(pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
			control = speed;// < pm_stopspeed ? pm_stopspeed : speed;
			drop += control*pm_friction*pml.frametime;
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}
	// If on a client then there is no friction
	else if ( GetCInfo( CINFO_HEADSLIDE ) && pm->ps->groundEntityNum < MAX_CLIENTS ) {
		drop = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_FLOAT ) {
		if ( pm->ps->pm_type == PM_FLOAT ) { //almost no friction while floating
			drop += speed*0.1f*pml.frametime;
		}
		else {
			drop += speed*pm_spectatorfriction*pml.frametime;
		}
	}

	// scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;

	VectorScale( vel, newspeed, vel );
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vector3 *wishdir, float wishspeed, float accel ) {
	if ( pm->gametype != GT_SIEGE
		|| pm->ps->m_iVehicleNum
		|| pm->ps->clientNum >= MAX_CLIENTS
		|| pm->ps->pm_type != PM_NORMAL ) { //standard method, allows "bunnyhopping" and whatnot
		int			i;
		float		addspeed, accelspeed, currentspeed;

		currentspeed = DotProduct( &pm->ps->velocity, wishdir );
		addspeed = wishspeed - currentspeed;
		if ( addspeed <= 0 && pm->ps->clientNum < MAX_CLIENTS ) {
			return;
		}

		if ( GetCInfo( CINFO_VQ3PHYS ) ) {
			accelspeed = accel*pml.frametime*wishspeed;
			if ( accelspeed > addspeed )
				accelspeed = addspeed;
		}
		else {
			if ( addspeed < 0 ) {
				accelspeed = (-accel)*pml.frametime*wishspeed;
				if ( accelspeed < addspeed )
					accelspeed = addspeed;
			}
			else {
				accelspeed = accel*pml.frametime*wishspeed;
				if ( accelspeed > addspeed )
					accelspeed = addspeed;
			}
		}

		for ( i = 0; i < 3; i++ ) {
			pm->ps->velocity.data[i] += accelspeed * wishdir->data[i];
		}
	}
	else { //use the proper way for siege
		vector3		wishVelocity;
		vector3		pushDir;
		float		pushLen;
		float		canPush;

		VectorScale( wishdir, wishspeed, &wishVelocity );
		VectorSubtract( &wishVelocity, &pm->ps->velocity, &pushDir );
		pushLen = VectorNormalize( &pushDir );

		canPush = accel*pml.frametime*wishspeed;
		if ( canPush > pushLen ) {
			canPush = pushLen;
		}

		VectorMA( &pm->ps->velocity, canPush, &pushDir, &pm->ps->velocity );
	}
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrtf(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total, scale;
	int		umove = GetCInfo( CINFO_VQ3PHYS ) ? cmd->upmove : 0;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max )	max = abs( cmd->rightmove );
	if ( abs( umove ) > max )			max = abs( umove );

	if ( !max )
		return 0;

	if ( GetCInfo( CINFO_VQ3PHYS ) )
		total = sqrtf( cmd->forwardmove*cmd->forwardmove + cmd->rightmove*cmd->rightmove + umove*umove );
	else
		total = sqrtf( (float)(cmd->forwardmove*cmd->forwardmove + cmd->rightmove*cmd->rightmove + umove*umove) );
	scale = (float)pm->ps->speed * max / (127.0f * total);

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		}
		else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	}
	else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		}
		else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		}
	}
}

#define METROID_JUMP 1

qboolean PM_ForceJumpingUp( void ) {
	if ( !(pm->ps->fd.forcePowersActive&(1 << FP_LEVITATION)) && pm->ps->fd.forceJumpCharge ) {//already jumped and let go
		return qfalse;
	}

	if ( BG_InSpecialJump( pm->ps->legsAnim ) ) {
		return qfalse;
	}

	if ( BG_SaberInSpecial( pm->ps->saberMove ) ) {
		return qfalse;
	}

	if ( BG_SaberInSpecialAttack( pm->ps->legsAnim ) ) {
		return qfalse;
	}

	if ( BG_HasYsalamiri( pm->gametype, pm->ps ) ) {
		return qfalse;
	}

	if ( !BG_CanUseFPNow( pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION ) ) {
		return qfalse;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE && //in air
		(pm->ps->pm_flags & PMF_JUMP_HELD) && //jumped
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 && //force-jump capable
		pm->ps->velocity.z > 0 )//going up
	{
		return qtrue;
	}
	return qfalse;
}

static void PM_JumpForDir( void ) {
	int anim = BOTH_JUMP1;
	if ( pm->cmd.forwardmove > 0 ) {
		anim = BOTH_JUMP1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else if ( pm->cmd.forwardmove < 0 ) {
		anim = BOTH_JUMPBACK1;
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
	else if ( pm->cmd.rightmove > 0 ) {
		anim = BOTH_JUMPRIGHT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else if ( pm->cmd.rightmove < 0 ) {
		anim = BOTH_JUMPLEFT1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else {
		anim = BOTH_JUMP1;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	if ( !BG_InDeathAnim( pm->ps->legsAnim ) ) {
		PM_SetAnim( SETANIM_LEGS, anim, SETANIM_FLAG_OVERRIDE, 100 );
	}
}

void PM_SetPMViewAngle( playerState_t *ps, vector3 *angle, usercmd_t *ucmd ) {
	int			i;

	for ( i = 0; i < 3; i++ ) { // set the delta angle
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT( angle->data[i] );
		ps->delta_angles.data[i] = cmdAngle - ucmd->angles.data[i];
	}
	VectorCopy( angle, &ps->viewangles );
}

qboolean PM_AdjustAngleForWallRun( playerState_t *ps, usercmd_t *ucmd, qboolean doMove ) {
	if ( ((ps->legsAnim) == BOTH_WALL_RUN_RIGHT || (ps->legsAnim) == BOTH_WALL_RUN_LEFT) && ps->legsTimer > 500 ) {//wall-running and not at end of anim
		//stick to wall, if there is one
		vector3	fwd, rt, traceTo, mins, maxs, fwdAngles;
		trace_t	trace;
		float	dist, yawAdjust;

		VectorSet( &mins, -15, -15, 0 );
		VectorSet( &maxs, 15, 15, 24 );
		VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

		AngleVectors( &fwdAngles, &fwd, &rt, NULL );
		if ( (ps->legsAnim) == BOTH_WALL_RUN_RIGHT ) {
			dist = 128;
			yawAdjust = -90;
		}
		else {
			dist = -128;
			yawAdjust = 90;
		}
		VectorMA( &ps->origin, dist, &rt, &traceTo );

		pm->trace( &trace, &ps->origin, &mins, &maxs, &traceTo, ps->clientNum, MASK_PLAYERSOLID );

		if ( trace.fraction < 1.0f
			&& (trace.plane.normal.z >= 0.0f && trace.plane.normal.z <= 0.4f) )//&& ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			trace_t	trace2;
			vector3 traceTo2;
			vector3	wallRunFwd, wallRunAngles;

			VectorClear( &wallRunAngles );
			wallRunAngles.yaw = vectoyaw( &trace.plane.normal ) + yawAdjust;
			AngleVectors( &wallRunAngles, &wallRunFwd, NULL, NULL );

			VectorMA( &pm->ps->origin, 32, &wallRunFwd, &traceTo2 );
			pm->trace( &trace2, &pm->ps->origin, &mins, &maxs, &traceTo2, pm->ps->clientNum, MASK_PLAYERSOLID );
			if ( trace2.fraction < 1.0f && DotProduct( &trace2.plane.normal, &wallRunFwd ) <= -0.999f ) {//wall we can't run on in front of us
				trace.fraction = 1.0f;//just a way to get it to kick us off the wall below
			}
		}

		if ( trace.fraction < 1.0f
			&& (trace.plane.normal.z >= 0.0f&&trace.plane.normal.z <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/) ) {//still a wall there
			if ( (ps->legsAnim) == BOTH_WALL_RUN_RIGHT ) {
				ucmd->rightmove = 127;
			}
			else {
				ucmd->rightmove = -127;
			}
			if ( ucmd->upmove < 0 ) {
				ucmd->upmove = 0;
			}
			//make me face perpendicular to the wall
			ps->viewangles.yaw = vectoyaw( &trace.plane.normal ) + yawAdjust;

			PM_SetPMViewAngle( ps, &ps->viewangles, ucmd );

			ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
			if ( doMove ) {
				//push me forward
				float	zVel = ps->velocity.z;
				if ( ps->legsTimer > 500 ) {//not at end of anim yet
					float speed = 175;
					if ( ucmd->forwardmove < 0 ) {//slower
						speed = 100;
					}
					else if ( ucmd->forwardmove > 0 ) {
						speed = 250;//running speed
					}
					VectorScale( &fwd, speed, &ps->velocity );
				}
				ps->velocity.z = zVel;//preserve z velocity
				//pull me toward the wall, too
				VectorMA( &ps->velocity, dist, &rt, &ps->velocity );
			}
			ucmd->forwardmove = 0;
			return qtrue;
		}
		else if ( doMove ) {//stop it
			if ( (ps->legsAnim) == BOTH_WALL_RUN_RIGHT ) {
				PM_SetAnim( SETANIM_BOTH, BOTH_WALL_RUN_RIGHT_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
			else if ( (ps->legsAnim) == BOTH_WALL_RUN_LEFT ) {
				PM_SetAnim( SETANIM_BOTH, BOTH_WALL_RUN_LEFT_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
		}
	}

	return qfalse;
}

qboolean PM_AdjustAnglesForWallRunUpFlipAlt( usercmd_t *ucmd ) {
	//	ucmd->angles.pitch = ANGLE2SHORT( pm->ps->viewangles.pitch ) - pm->ps->delta_angles.pitch;
	//	ucmd->angles.yaw = ANGLE2SHORT( pm->ps->viewangles.yaw ) - pm->ps->delta_angles.yaw;
	PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, ucmd );
	return qtrue;
}

qboolean PM_AdjustAngleForWallRunUp( playerState_t *ps, usercmd_t *ucmd, qboolean doMove ) {
	if ( ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START ) {//wall-running up
		//stick to wall, if there is one
		vector3	fwd, traceTo, mins, maxs, fwdAngles;
		trace_t	trace;
		float	dist = 128;

		VectorSet( &mins, -15, -15, 0 );
		VectorSet( &maxs, 15, 15, 24 );
		VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

		AngleVectors( &fwdAngles, &fwd, NULL, NULL );
		VectorMA( &ps->origin, dist, &fwd, &traceTo );
		pm->trace( &trace, &ps->origin, &mins, &maxs, &traceTo, ps->clientNum, MASK_PLAYERSOLID );
		if ( trace.fraction > 0.5f ) {//hmm, some room, see if there's a floor right here
			trace_t	trace2;
			vector3	top, bottom;

			VectorCopy( &trace.endpos, &top );
			top.z += (pm->mins.z * -1) + 4.0f;
			VectorCopy( &top, &bottom );
			bottom.z -= 64.0f;
			pm->trace( &trace2, &top, &pm->mins, &pm->maxs, &bottom, ps->clientNum, MASK_PLAYERSOLID );
			if ( !trace2.allsolid
				&& !trace2.startsolid
				&& trace2.fraction < 1.0f
				&& trace2.plane.normal.z > 0.7f )//slope we can stand on
			{//cool, do the alt-flip and land on whetever it is we just scaled up
				VectorScale( &fwd, 100, &pm->ps->velocity );
				pm->ps->velocity.z += 400;
				PM_SetAnim( SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_ALT, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
				pm->ps->pm_flags |= PMF_JUMP_HELD;
				//ent->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
				//ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
				//G_AddEvent( ent, EV_JUMP, 0 );
				PM_AddEvent( EV_JUMP );
				ucmd->upmove = 0;
				return qfalse;
			}
		}

		if ( //ucmd->upmove <= 0 &&
			ps->legsTimer > 0 &&
			ucmd->forwardmove > 0 &&
			trace.fraction < 1.0f &&
			(trace.plane.normal.z >= 0.0f&&trace.plane.normal.z <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/) ) {//still a vertical wall there
			//make sure there's not a ceiling above us!
			trace_t	trace2;
			VectorCopy( &ps->origin, &traceTo );
			traceTo.z += 64;
			pm->trace( &trace2, &ps->origin, &mins, &maxs, &traceTo, ps->clientNum, MASK_PLAYERSOLID );
			if ( trace2.fraction < 1.0f ) {//will hit a ceiling, so force jump-off right now
				//NOTE: hits any entity or clip brush in the way, too, not just architecture!
			}
			else {//all clear, keep going
				//FIXME: don't pull around 90 turns
				//FIXME: simulate stepping up steps here, somehow?
				ucmd->forwardmove = 127;
				if ( ucmd->upmove < 0 ) {
					ucmd->upmove = 0;
				}
				//make me face the wall
				ps->viewangles.yaw = vectoyaw( &trace.plane.normal ) + 180;
				PM_SetPMViewAngle( ps, &ps->viewangles, ucmd );
				/*
				if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
				{//don't clamp angles when looking through a viewEntity
				SetClientViewAngle( ent, ent->client->ps.viewangles );
				}
				*/
				ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
				//if ( ent->s.number || !player_locked )
				if ( 1 ) //aslkfhsakf
				{
					if ( doMove ) {
						//pull me toward the wall
						VectorScale( &trace.plane.normal, -dist*trace.fraction, &ps->velocity );
						//push me up
						if ( ps->legsTimer > 200 ) {//not at end of anim yet
							float speed = 300;
							/*
							if ( ucmd->forwardmove < 0 )
							{//slower
							speed = 100;
							}
							else if ( ucmd->forwardmove > 0 )
							{
							speed = 250;//running speed
							}
							*/
							ps->velocity.z = speed;//preserve z velocity
						}
					}
				}
				ucmd->forwardmove = 0;
				return qtrue;
			}
		}
		//failed!
		if ( doMove ) {//stop it
			VectorScale( &fwd, -300.0f, &ps->velocity );
			ps->velocity.z += 200;
			//NPC_SetAnim( ent, SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_END, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//why?!?#?#@!%$R@$KR#F:Hdl;asfm
			PM_SetAnim( SETANIM_BOTH, BOTH_FORCEWALLRUNFLIP_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			ps->pm_flags |= PMF_JUMP_HELD;
			//ent->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;

			//FIXME do I need this in mp?
			//ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
			PM_AddEvent( EV_JUMP );
			ucmd->upmove = 0;
			//return qtrue;
		}
	}
	return qfalse;
}

#define	JUMP_OFF_WALL_SPEED	200.0f
//nice...
static float BG_ForceWallJumpStrength( void ) {
	return (forceJumpStrength[FORCE_LEVEL_3] / 2.5f);
}

qboolean PM_AdjustAngleForWallJump( playerState_t *ps, usercmd_t *ucmd, qboolean doMove ) {
	if ( BG_InLedgeMove( ps->legsAnim ) ) {//Ledge movin'  Let the ledge move function handle it.
		return qfalse;
	}

	if ( ((BG_InReboundJump( ps->legsAnim ) || BG_InReboundHold( ps->legsAnim ))
		&& (BG_InReboundJump( ps->torsoAnim ) || BG_InReboundHold( ps->torsoAnim )))
		|| (pm->ps->pm_flags&PMF_STUCK_TO_WALL) ) {//hugging wall, getting ready to jump off
		//stick to wall, if there is one
		vector3	checkDir, traceTo, mins, maxs, fwdAngles;
		trace_t	trace;
		float	dist = 128.0f, yawAdjust;

		VectorSet( &mins, pm->mins.x, pm->mins.y, 0 );
		VectorSet( &maxs, pm->maxs.x, pm->maxs.y, 24 );
		VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

		switch ( ps->legsAnim ) {
		case BOTH_FORCEWALLREBOUND_RIGHT:
		case BOTH_FORCEWALLHOLD_RIGHT:
			AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
			yawAdjust = -90;
			break;
		case BOTH_FORCEWALLREBOUND_LEFT:
		case BOTH_FORCEWALLHOLD_LEFT:
			AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
			VectorScale( &checkDir, -1, &checkDir );
			yawAdjust = 90;
			break;
		case BOTH_FORCEWALLREBOUND_FORWARD:
		case BOTH_FORCEWALLHOLD_FORWARD:
			AngleVectors( &fwdAngles, &checkDir, NULL, NULL );
			yawAdjust = 180;
			break;
		case BOTH_FORCEWALLREBOUND_BACK:
		case BOTH_FORCEWALLHOLD_BACK:
			AngleVectors( &fwdAngles, &checkDir, NULL, NULL );
			VectorScale( &checkDir, -1, &checkDir );
			yawAdjust = 0;
			break;
		default:
			//WTF???
			pm->ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			return qfalse;
		}
		if ( pm->debugMelee == 2 ) {//uber-skillz
			if ( ucmd->upmove > 0 ) {//hold on until you let go manually
				if ( BG_InReboundHold( ps->legsAnim ) ) {//keep holding
					if ( ps->legsTimer < 150 ) {
						ps->legsTimer = 150;
					}
				}
				else {//if got to hold part of anim, play hold anim
					if ( ps->legsTimer <= 300 ) {
						ps->saberHolstered = 2;
						PM_SetAnim( SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
						ps->legsTimer = ps->torsoTimer = 150;
					}
				}
			}
		}
		VectorMA( &ps->origin, dist, &checkDir, &traceTo );
		pm->trace( &trace, &ps->origin, &mins, &maxs, &traceTo, ps->clientNum, MASK_PLAYERSOLID );
		if ( //ucmd->upmove <= 0 &&
			ps->legsTimer > 100 &&
			trace.fraction < 1.0f &&
			fabsf( trace.plane.normal.z ) <= 0.2f/*MAX_WALL_GRAB_SLOPE*/ ) {//still a vertical wall there
			//FIXME: don't pull around 90 turns
			/*
			if ( ent->s.number || !player_locked )
			{
			ucmd->forwardmove = 127;
			}
			*/
			if ( ucmd->upmove < 0 ) {
				ucmd->upmove = 0;
			}
			//align me to the wall
			ps->viewangles.yaw = vectoyaw( &trace.plane.normal ) + yawAdjust;
			PM_SetPMViewAngle( ps, &ps->viewangles, ucmd );
			/*
			if ( ent->client->ps.viewEntity <= 0 || ent->client->ps.viewEntity >= ENTITYNUM_WORLD )
			{//don't clamp angles when looking through a viewEntity
			SetClientViewAngle( ent, ent->client->ps.viewangles );
			}
			*/
			ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
			//if ( ent->s.number || !player_locked )
			if ( 1 ) {
				if ( doMove ) {
					//pull me toward the wall
					VectorScale( &trace.plane.normal, -128.0f, &ps->velocity );
				}
			}
			ucmd->upmove = 0;
			ps->pm_flags |= PMF_STUCK_TO_WALL;
			return qtrue;
		}
		else if ( doMove
			&& (ps->pm_flags&PMF_STUCK_TO_WALL) ) {//jump off
			//push off of it!
			ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			ps->velocity.x = ps->velocity.y = 0;
			VectorScale( &checkDir, -JUMP_OFF_WALL_SPEED, &ps->velocity );
			ps->velocity.z = BG_ForceWallJumpStrength();
			ps->pm_flags |= PMF_JUMP_HELD;//PMF_JUMPING|PMF_JUMP_HELD;
			//G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
			ps->fd.forceJumpSound = 1; //this is a stupid thing, i should fix it.
			//ent->client->ps.forcePowersActive |= (1<<FP_LEVITATION);
			if ( ps->origin.z < ps->fd.forceJumpZStart ) {
				ps->fd.forceJumpZStart = ps->origin.z;
			}
			//FIXME do I need this?

			BG_ForcePowerDrain( ps, FP_LEVITATION, 10 );
			//no control for half a second
			ps->pm_flags |= PMF_TIME_KNOCKBACK;
			ps->pm_time = 500;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 127;

			if ( BG_InReboundHold( ps->legsAnim ) ) {//if was in hold pose, release now
				PM_SetAnim( SETANIM_BOTH, BOTH_FORCEWALLRELEASE_FORWARD + (ps->legsAnim - BOTH_FORCEWALLHOLD_FORWARD), SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
			else {
				//PM_JumpForDir();
				PM_SetAnim( SETANIM_LEGS, BOTH_FORCEJUMP1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0 );
			}

			//return qtrue;
		}
	}
	ps->pm_flags &= ~PMF_STUCK_TO_WALL;
	return qfalse;
}

//The height level at which you grab ledges.  In terms of player origin
#define LEDGEGRABMAXHEIGHT		70
#define LEDGEGRABHEIGHT			52.4f
#define LEDGEVERTOFFSET			LEDGEGRABHEIGHT
#define LEDGEGRABMINHEIGHT		40

//max distance you can be from the ledge for ledge grabbing to work
#define LEDGEGRABDISTANCE		40

//min distance you can be from the ledge for ledge grab to work
#define LEDGEGRABMINDISTANCE	22

//distance at which the animation grabs the ledge
#define LEDGEHOROFFSET			22.3f

//lets go of a ledge
static void BG_LetGoofLedge( playerState_t *ps ) {
	ps->pm_flags &= ~PMF_STUCK_TO_WALL;
	ps->torsoTimer = 0;
	ps->legsTimer = 0;
}

static void PM_SetVelocityforLedgeMove( playerState_t *ps, int anim ) {
	vector3 fwdAngles, moveDir;
	float animationpoint = BG_GetLegsAnimPoint( ps, pm_entSelf->localAnimIndex );

	switch ( anim ) {
	case BOTH_LEDGE_GRAB:
	case BOTH_LEDGE_HOLD:
		VectorClear( &ps->velocity );
		return;

	case BOTH_LEDGE_LEFT:
		if ( animationpoint > .333f && animationpoint < .666f ) {
			VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );
			AngleVectors( &fwdAngles, NULL, &moveDir, NULL );
			VectorScale( &moveDir, -30, &moveDir );
			VectorCopy( &moveDir, &ps->velocity );
		}
		else
			VectorClear( &ps->velocity );
		break;

	case BOTH_LEDGE_RIGHT:
		if ( animationpoint > .333f && animationpoint < .666f ) {
			VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );
			AngleVectors( &fwdAngles, NULL, &moveDir, NULL );
			VectorScale( &moveDir, 30, &moveDir );
			VectorCopy( &moveDir, &ps->velocity );
		}
		else
			VectorClear( &ps->velocity );
		break;

	case BOTH_LEDGE_MERCPULL:
		if ( animationpoint > .8f && animationpoint < .925f ) {
			ps->velocity.x = 0;
			ps->velocity.y = 0;
			ps->velocity.z = 77;//192;//154;
		}
		else if ( animationpoint > .7f && animationpoint < .75f ) {
			ps->velocity.x = 0;
			ps->velocity.y = 0;
			ps->velocity.z = 16;//128;//26;
		}
		else if ( animationpoint > .375f && animationpoint < .7f ) {
			ps->velocity.x = 0;
			ps->velocity.y = 0;
			ps->velocity.z = 70;//96;//140;
		}
		else if ( animationpoint < .375f ) {
			VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );
			AngleVectors( &fwdAngles, &moveDir, NULL, NULL );
			VectorScale( &moveDir, 70, &moveDir );
			VectorCopy( &moveDir, &ps->velocity );
		}
		else
			VectorClear( &ps->velocity );
		break;

	default:
		VectorClear( &ps->velocity );
		return;
	}
}

//Switch to this animation and keep repeating this animation while updating its timers
#define	AFLAG_PACE (SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS|SETANIM_FLAG_PACE)

static void PM_AdjustAngleForWallGrap( playerState_t *ps, usercmd_t *ucmd ) {
	if ( (ps->pm_flags & PMF_STUCK_TO_WALL) && BG_InLedgeMove( ps->legsAnim ) ) {
		// still holding onto the ledge stick our view to the wall angles
		if ( ps->legsAnim != BOTH_LEDGE_MERCPULL ) {
			vector3 traceTo, traceFrom, fwd, fwdAngles;
			trace_t	trace;

			VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );
			AngleVectors( &fwdAngles, &fwd, NULL, NULL );
			VectorNormalize( &fwd );

			VectorCopy( &ps->origin, &traceFrom );
			traceFrom.z += LEDGEGRABHEIGHT - 1;

			VectorMA( &traceFrom, LEDGEGRABDISTANCE, &fwd, &traceTo );

			pm->trace( &trace, &traceFrom, NULL, NULL, &traceTo, ps->clientNum, MASK_SOLID );

			if ( trace.fraction == 1 ) {
				// that's not good, we lost the ledge so let go.
				BG_LetGoofLedge( ps );
				return;
			}

			// lock the view viewangles
			ps->viewangles.yaw = vectoyaw( &trace.plane.normal ) + 180;
			//	PM_SetPMViewAngle( ps, ps->viewangles, ucmd );
			//	ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
		}
		else {
			// lock viewangles
			//	PM_SetPMViewAngle( ps, ps->viewangles, ucmd );
			//	ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
		}
		ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;

		//	if ( ps->legsTimer < 0 ) {
		if ( ps->legsTimer <= 50 ) {
			// Try switching to idle
			// pull up done, bail.
			if ( ps->legsAnim == BOTH_LEDGE_MERCPULL )
				ps->pm_flags &= ~PMF_STUCK_TO_WALL;
			else {
				PM_SetAnim( SETANIM_BOTH, BOTH_LEDGE_HOLD, SETANIM_FLAG_OVERRIDE, 0 );
				ps->torsoTimer = 100;//500;
				ps->legsTimer = 100;//500;
				//hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
		}
		else if ( ps->legsAnim == BOTH_LEDGE_HOLD ) {
			if ( ucmd->rightmove ) {
				// trying to move left/right
				if ( ucmd->rightmove < 0 ) {
					// shimmy left
					PM_SetAnim( SETANIM_BOTH, BOTH_LEDGE_LEFT, AFLAG_PACE, 0 );
					// hold weapontime so people can't do attacks while in ledgegrab
					ps->weaponTime = ps->legsTimer;
				}
				else {
					// shimmy right
					PM_SetAnim( SETANIM_BOTH, BOTH_LEDGE_RIGHT, AFLAG_PACE, 0 );
					// hold weapontime so people can't do attacks while in ledgegrab
					ps->weaponTime = ps->legsTimer;
				}
			}
			// letting go
			else if ( ucmd->forwardmove < 0 )
				BG_LetGoofLedge( ps );
			else if ( ucmd->forwardmove > 0 ) {
				// Pull up
				PM_SetAnim( SETANIM_BOTH, BOTH_LEDGE_MERCPULL, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_HOLDLESS, 0 );
				// hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
			else {
				// keep holding on
				ps->torsoTimer = 100;//500;
				ps->legsTimer = 100;//500;
				// hold weapontime so people can't do attacks while in ledgegrab
				ps->weaponTime = ps->legsTimer;
			}
		}

		//set movement velocity
		PM_SetVelocityforLedgeMove( ps, ps->legsAnim );

		//clear movement commands to prevent movement
		ucmd->rightmove = ucmd->upmove = ucmd->forwardmove = 0;
	}
}

//Set the height for when a force jump was started. If it's 0, nuge it up (slight hack to prevent holding jump over slopes)
void PM_SetForceJumpZStart( float value ) {
	pm->ps->fd.forceJumpZStart = value;
	if ( !pm->ps->fd.forceJumpZStart )
		pm->ps->fd.forceJumpZStart -= 0.1f;
}

const float forceJumpHeightMax[NUM_FORCE_POWER_LEVELS] = {
	66, //( 32+stepheight(18)+crouchdiff(24) =  74)
	130, //( 96+stepheight(18)+crouchdiff(24) = 138)
	226, //(192+stepheight(18)+crouchdiff(24) = 234)
	418, //(384+stepheight(18)+crouchdiff(24) = 426)
};

//NOTE!!! assumes an appropriate anim is being passed in!!!
void PM_GrabWallForJump( int anim ) {
	PM_SetAnim( SETANIM_BOTH, anim, SETANIM_FLAG_RESTART | SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
	PM_AddEvent( EV_JUMP );//make sound for grab
	pm->ps->pm_flags |= PMF_STUCK_TO_WALL;
}

// scan for for a ledge in the given direction
static qboolean LedgeTrace( trace_t *trace, vector3 *dir, float *lerpup, float *lerpfwd, float *lerpyaw ) {
	vector3 traceTo, traceFrom, wallangles;

	if ( GetCPD( pm_entSelf, CPD_LEDGEGRAB ) || !GetCInfo( CINFO_LEDGEGRAB ) )
		return qfalse;

	VectorMA( &pm->ps->origin, LEDGEGRABDISTANCE, dir, &traceTo );
	VectorCopy( &pm->ps->origin, &traceFrom );

	traceFrom.z += LEDGEGRABMINHEIGHT;
	traceTo.z += LEDGEGRABMINHEIGHT;

	pm->trace( trace, &traceFrom, NULL, NULL, &traceTo, pm->ps->clientNum, MASK_DEADSOLID );

	if ( trace->fraction < 1 ) {//hit a wall, pop into the wall and fire down to find top of wall
		VectorMA( &trace->endpos, 0.5f, dir, &traceTo );

		VectorCopy( &traceTo, &traceFrom );

		traceFrom.z += (LEDGEGRABMAXHEIGHT - LEDGEGRABMINHEIGHT);

		pm->trace( trace, &traceFrom, NULL, NULL, &traceTo, pm->ps->clientNum, MASK_DEADSOLID );

		if ( trace->fraction == 1.0f || trace->startsolid )
			return qfalse;
	}

	else {//ok,
		VectorCopy( &traceTo, &traceFrom );
		traceFrom.z += (LEDGEGRABMAXHEIGHT - LEDGEGRABMINHEIGHT);

		pm->trace( trace, &traceFrom, NULL, NULL, &traceTo, pm->ps->clientNum, MASK_DEADSOLID );

		if ( trace->fraction == 1.0f || trace->startsolid )
			return qfalse;

		//found something, let's try to find the top face.
		VectorCopy( &trace->endpos, &traceFrom );
		traceFrom.z++;

		pm->trace( trace, &traceFrom, NULL, NULL, &traceTo, pm->ps->clientNum, MASK_DEADSOLID );
	}

	//check to make sure we found a good top surface and go from there
	vectoangles( &trace->plane.normal, &wallangles );

	if ( wallangles.pitch > -45 )
		return qfalse; //no ledge or the ledge is too steep
	else {
		VectorCopy( &trace->endpos, &traceTo );
		*lerpup = trace->endpos.z - pm->ps->origin.z - LEDGEVERTOFFSET;

		VectorCopy( &pm->ps->origin, &traceFrom );
		traceTo.z -= 1;

		traceFrom.z = traceTo.z;

		pm->trace( trace, &traceFrom, NULL, NULL, &traceTo, pm->ps->clientNum, MASK_DEADSOLID );

		vectoangles( &trace->plane.normal, &wallangles );
		if ( trace->fraction == 1.0f || wallangles.pitch > 20 || wallangles.pitch < -20 )
			return qfalse;//no ledge or too steep of a ledge

		*lerpfwd = Distance( &trace->endpos, &traceFrom ) - LEDGEHOROFFSET;
		*lerpyaw = vectoyaw( &trace->plane.normal ) + 180;
		return qtrue;
	}
}

//check for ledge grab
qboolean PM_CheckGrab( void ) {
	vector3 checkDir, traceTo, fwdAngles;
	trace_t	trace;
	float lerpup = 0;
	float lerpfwd = 0;
	float lerpyaw = 0;
	qboolean skipcmdtrace = qfalse;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//not in the air don't attempt a ledge grab
		return qfalse;
	}

	if ( pm->ps->pm_type == PM_JETPACK ) {//don't do ledgegrab checks while using the jetpack
		return qfalse;
	}

	if ( GetCPD( pm_entSelf, CPD_LEDGEGRAB ) || !GetCInfo( CINFO_LEDGEGRAB ) )
		return qfalse;

	if ( BG_InLedgeMove( pm->ps->legsAnim ) ||	//already on a ledge
		pm->ps->pm_type == PM_SPECTATOR ||	//a spectator
		BG_InSpecialJump( pm->ps->legsAnim ) )	//in a special jump
	{
		return qfalse;
	}

	//try looking in front of us first
	VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0.0f );
	AngleVectors( &fwdAngles, &checkDir, NULL, NULL );

	if ( !VectorCompare( &pm->ps->velocity, &vec3_origin ) ) {//player is moving
		if ( LedgeTrace( &trace, &checkDir, &lerpup, &lerpfwd, &lerpyaw ) ) {
			skipcmdtrace = qtrue;
		}
	}

	if ( !skipcmdtrace ) {//no luck finding a ledge to grab based on movement.  Try looking for a ledge based on where the player is
		//TRYING to go.
		if ( !pm->cmd.rightmove && !pm->cmd.forwardmove ) {//no dice abort
			return qfalse;
		}
		else {
			if ( pm->cmd.rightmove ) {
				if ( pm->cmd.rightmove > 0 ) {
					AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
					VectorNormalize( &checkDir );
				}
				else if ( pm->cmd.rightmove < 0 ) {
					AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
					VectorScale( &checkDir, -1, &checkDir );
					VectorNormalize( &checkDir );
				}
			}
			else if ( pm->cmd.forwardmove > 0 ) {//already tried this direction.
				return qfalse;
			}
			else if ( pm->cmd.forwardmove < 0 ) {
				AngleVectors( &fwdAngles, &checkDir, NULL, NULL );
				VectorScale( &checkDir, -1, &checkDir );
				VectorNormalize( &checkDir );
			}

			if ( !LedgeTrace( &trace, &checkDir, &lerpup, &lerpfwd, &lerpyaw ) ) {//no dice
				return qfalse;
			}
		}
	}

	VectorCopy( &pm->ps->origin, &traceTo );
	VectorMA( &pm->ps->origin, lerpfwd, &checkDir, &traceTo );
	traceTo.z += lerpup;

	//check to see if we can actually latch to that position.
	pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &traceTo, pm->ps->clientNum, MASK_PLAYERSOLID );
	if ( trace.fraction != 1 || trace.startsolid ) {
		return qfalse;
	}

	//turn to face wall
	pm->ps->viewangles.yaw = lerpyaw;
	PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	pm->cmd.angles.yaw = ANGLE2SHORT( pm->ps->viewangles.yaw ) - pm->ps->delta_angles.yaw;


	//We are clear to latch to the wall
	pm->ps->saberHolstered = 2;
	VectorCopy( &trace.endpos, &pm->ps->origin );
	VectorCopy( &vec3_origin, &pm->ps->velocity );
	PM_GrabWallForJump( BOTH_LEDGE_GRAB );
	pm->ps->weaponTime = pm->ps->legsTimer;

	return qtrue;
}


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	qboolean allowFlips = qtrue;

	if ( pm->ps->clientNum >= MAX_CLIENTS ) {
		bgEntity_t *pEnt = pm_entSelf;

		if ( pEnt->s.eType == ET_NPC &&
			pEnt->s.NPC_class == CLASS_VEHICLE ) { //no!
			return qfalse;
		}
	}

	if ( pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
		pm->ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
		pm->ps->forceHandExtend == HANDEXTEND_POSTTHROWN ) {
		return qfalse;
	}

	if ( pm->ps->pm_type == PM_JETPACK ) { //there's no actual jumping while we jetpack
		return qfalse;
	}

	//Don't allow jump until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;
	}

	if ( PM_InKnockDown( pm->ps ) || BG_InRoll( pm->ps, pm->ps->legsAnim ) ) {//in knockdown
		return qfalse;
	}

	if ( pm->ps->weapon == WP_SABER ) {
		saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
		saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber1
			&& (saber1->saberFlags&SFL_NO_FLIPS) ) {
			allowFlips = qfalse;
		}
		if ( saber2
			&& (saber2->saberFlags&SFL_NO_FLIPS) ) {
			allowFlips = qfalse;
		}
	}

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE || pm->ps->origin.z < pm->ps->fd.forceJumpZStart ) {
		pm->ps->fd.forcePowersActive &= ~(1 << FP_LEVITATION);
	}

	if ( pm->ps->fd.forcePowersActive & (1 << FP_LEVITATION) ) { //Force jump is already active.. continue draining power appropriately until we land.
		if ( pm->ps->fd.forcePowerDebounce[FP_LEVITATION] < pm->cmd.serverTime ) {
			if ( pm->gametype == GT_DUEL
				|| pm->gametype == GT_POWERDUEL ) {//jump takes less power
				BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 1 );
			}
			else {
				BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 5 );
			}
			if ( pm->ps->fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_2 ) {
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 300;
			}
			else {
				pm->ps->fd.forcePowerDebounce[FP_LEVITATION] = pm->cmd.serverTime + 200;
			}
		}
	}

	if ( pm->ps->forceJumpFlip ) { //Forced jump anim
		int anim = BOTH_FORCEINAIR1;
		int	parts = SETANIM_BOTH;
		if ( allowFlips ) {
			if ( pm->cmd.forwardmove > 0 ) {
				anim = BOTH_FLIP_F;
			}
			else if ( pm->cmd.forwardmove < 0 ) {
				anim = BOTH_FLIP_B;
			}
			else if ( pm->cmd.rightmove > 0 ) {
				anim = BOTH_FLIP_R;
			}
			else if ( pm->cmd.rightmove < 0 ) {
				anim = BOTH_FLIP_L;
			}
		}
		else {
			if ( pm->cmd.forwardmove > 0 ) {
				anim = BOTH_FORCEINAIR1;
			}
			else if ( pm->cmd.forwardmove < 0 ) {
				anim = BOTH_FORCEINAIRBACK1;
			}
			else if ( pm->cmd.rightmove > 0 ) {
				anim = BOTH_FORCEINAIRRIGHT1;
			}
			else if ( pm->cmd.rightmove < 0 ) {
				anim = BOTH_FORCEINAIRLEFT1;
			}
		}
		if ( pm->ps->weaponTime ) {//FIXME: really only care if we're in a saber attack anim...
			parts = SETANIM_LEGS;
		}

		PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
		pm->ps->forceJumpFlip = qfalse;
		return qtrue;
	}
#if METROID_JUMP
	if ( pm->waterlevel < 2 ) {
		if ( pm->ps->gravity > 0 ) {//can't do this in zero-G
			if ( PM_ForceJumpingUp() ) {//holding jump in air
				float curHeight = pm->ps->origin.z - pm->ps->fd.forceJumpZStart;
				//check for max force jump level and cap off & cut z vel
				if ( (curHeight <= forceJumpHeight[0] ||//still below minimum jump height
					(pm->ps->fd.forcePower&&pm->cmd.upmove >= 10)) &&////still have force power available and still trying to jump up
					curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] &&
					pm->ps->fd.forceJumpZStart )//still below maximum jump height
				{//can still go up
					if ( curHeight > forceJumpHeight[0] ) {//passed normal jump height  *2?
						if ( !(pm->ps->fd.forcePowersActive&(1 << FP_LEVITATION)) )//haven't started forcejump yet
						{
							//start force jump
							pm->ps->fd.forcePowersActive |= (1 << FP_LEVITATION);
							pm->ps->fd.forceJumpSound = 1;
							//play flip
							if ( (pm->cmd.forwardmove || pm->cmd.rightmove) && //pushing in a dir
								(pm->ps->legsAnim) != BOTH_FLIP_F &&//not already flipping
								(pm->ps->legsAnim) != BOTH_FLIP_B &&
								(pm->ps->legsAnim) != BOTH_FLIP_R &&
								(pm->ps->legsAnim) != BOTH_FLIP_L
								&& allowFlips ) {
								int anim = BOTH_FORCEINAIR1;
								int	parts = SETANIM_BOTH;

								if ( pm->cmd.forwardmove > 0 ) {
									anim = BOTH_FLIP_F;
								}
								else if ( pm->cmd.forwardmove < 0 ) {
									anim = BOTH_FLIP_B;
								}
								else if ( pm->cmd.rightmove > 0 ) {
									anim = BOTH_FLIP_R;
								}
								else if ( pm->cmd.rightmove < 0 ) {
									anim = BOTH_FLIP_L;
								}
								if ( pm->ps->weaponTime ) {
									parts = SETANIM_LEGS;
								}

								PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
							}
							else if ( pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 ) {
								vector3 facingFwd, facingRight, facingAngles;
								int	anim = -1;
								float dotR, dotF;

								VectorSet( &facingAngles, 0, pm->ps->viewangles.yaw, 0 );

								AngleVectors( &facingAngles, &facingFwd, &facingRight, NULL );
								dotR = DotProduct( &facingRight, &pm->ps->velocity );
								dotF = DotProduct( &facingFwd, &pm->ps->velocity );

								if ( fabsf( dotR ) > fabsf( dotF ) * 1.5f ) {
									if ( dotR > 150 ) {
										anim = BOTH_FORCEJUMPRIGHT1;
									}
									else if ( dotR < -150 ) {
										anim = BOTH_FORCEJUMPLEFT1;
									}
								}
								else {
									if ( dotF > 150 ) {
										anim = BOTH_FORCEJUMP1;
									}
									else if ( dotF < -150 ) {
										anim = BOTH_FORCEJUMPBACK1;
									}
								}
								if ( anim != -1 ) {
									int parts = SETANIM_BOTH;
									if ( pm->ps->weaponTime ) {//FIXME: really only care if we're in a saber attack anim...
										parts = SETANIM_LEGS;
									}

									PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
								}
							}
						}
						else { //jump is already active (the anim has started)
							if ( pm->ps->legsTimer < 1 ) {//not in the middle of a legsAnim
								int anim = (pm->ps->legsAnim);
								int newAnim;
								switch ( anim ) {
								case BOTH_FORCEJUMP1:
									newAnim = BOTH_FORCELAND1;//BOTH_FORCEINAIR1;
									break;

								case BOTH_FORCEJUMPBACK1:
									newAnim = BOTH_FORCELANDBACK1;//BOTH_FORCEINAIRBACK1;
									break;

								case BOTH_FORCEJUMPLEFT1:
									newAnim = BOTH_FORCELANDLEFT1;//BOTH_FORCEINAIRLEFT1;
									break;

								case BOTH_FORCEJUMPRIGHT1:
									newAnim = BOTH_FORCELANDRIGHT1;//BOTH_FORCEINAIRRIGHT1;
									break;

								default:
									newAnim = -1;
									break;
								}
								if ( newAnim != -1 ) {
									int parts = SETANIM_BOTH;
									if ( pm->ps->weaponTime ) {
										parts = SETANIM_LEGS;
									}

									PM_SetAnim( parts, newAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
								}
							}
						}
					}

					//need to scale this down, start with height velocity (based on max force jump height) and scale down to regular jump vel
					pm->ps->velocity.z = (forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - curHeight) / forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] * forceJumpStrength[pm->ps->fd.forcePowerLevel[FP_LEVITATION]];//JUMP_VELOCITY;
					pm->ps->velocity.z /= 10;
					pm->ps->velocity.z += JUMP_VELOCITY;
					pm->ps->pm_flags |= PMF_JUMP_HELD;
				}
				else if ( curHeight > forceJumpHeight[0] && curHeight < forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - forceJumpHeight[0] ) {//still have some headroom, don't totally stop it
					if ( pm->ps->velocity.z > JUMP_VELOCITY ) {
						pm->ps->velocity.z = JUMP_VELOCITY;
					}
				}
				else {
					//pm->ps->velocity[2] = 0;
					//rww - changed for the sake of balance in multiplayer

					if ( pm->ps->velocity.z > JUMP_VELOCITY ) {
						pm->ps->velocity.z = JUMP_VELOCITY;
					}
				}
				pm->cmd.upmove = 0;
				return qfalse;
			}
		}
	}

#endif

	//Not jumping
	if ( pm->cmd.upmove < 10 && pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	if ( pm->ps->gravity <= 0 ) {//in low grav, you push in the dir you're facing as long as there is something behind you to shove off of
		vector3	forward, back;
		trace_t	trace;

		AngleVectors( &pm->ps->viewangles, &forward, NULL, NULL );
		VectorMA( &pm->ps->origin, -8, &forward, &back );
		pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &back, pm->ps->clientNum, pm->tracemask );

		if ( trace.fraction <= 1.0f ) {
			VectorMA( &pm->ps->velocity, JUMP_VELOCITY * 2, &forward, &pm->ps->velocity );
			PM_SetAnim( SETANIM_LEGS, BOTH_FORCEJUMP1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 150 );
		}//else no surf close enough to push off of
		pm->cmd.upmove = 0;
	}
	else if ( pm->cmd.upmove > 0 && pm->waterlevel < 2 &&
		pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_0 &&
		!(pm->ps->pm_flags&PMF_JUMP_HELD) &&
		(pm->ps->weapon == WP_SABER || pm->ps->weapon == WP_MELEE) &&
		!PM_IsRocketTrooper() &&
		!BG_HasYsalamiri( pm->gametype, pm->ps ) &&
		BG_CanUseFPNow( pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION ) ) {
		qboolean allowWallRuns = GetCInfo( CINFO_VQ3PHYS ) ? qfalse : qtrue;
		qboolean allowWallFlips = GetCInfo( CINFO_VQ3PHYS ) ? qfalse : qtrue;
		//	qboolean allowFlips = qtrue;
		qboolean allowWallGrabs = GetCInfo( CINFO_VQ3PHYS ) ? qfalse : qtrue;
		if ( pm->ps->weapon == WP_SABER ) {
			saberInfo_t *saber1 = BG_MySaber( pm->ps->clientNum, 0 );
			saberInfo_t *saber2 = BG_MySaber( pm->ps->clientNum, 1 );
			if ( saber1
				&& (saber1->saberFlags&SFL_NO_WALL_RUNS) ) {
				allowWallRuns = qfalse;
			}
			if ( saber2
				&& (saber2->saberFlags&SFL_NO_WALL_RUNS) ) {
				allowWallRuns = qfalse;
			}
			if ( saber1
				&& (saber1->saberFlags&SFL_NO_WALL_FLIPS) ) {
				allowWallFlips = qfalse;
			}
			if ( saber2
				&& (saber2->saberFlags&SFL_NO_WALL_FLIPS) ) {
				allowWallFlips = qfalse;
			}
			if ( saber1
				&& (saber1->saberFlags&SFL_NO_FLIPS) ) {
				allowFlips = qfalse;
			}
			if ( saber2
				&& (saber2->saberFlags&SFL_NO_FLIPS) ) {
				allowFlips = qfalse;
			}
			if ( saber1
				&& (saber1->saberFlags&SFL_NO_WALL_GRAB) ) {
				allowWallGrabs = qfalse;
			}
			if ( saber2
				&& (saber2->saberFlags&SFL_NO_WALL_GRAB) ) {
				allowWallGrabs = qfalse;
			}
		}

		if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//on the ground
			//check for left-wall and right-wall special jumps
			int anim = -1;
			float	vertPush = 0;
			if ( pm->cmd.rightmove > 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 ) {//strafing right
				if ( pm->cmd.forwardmove > 0 ) {//wall-run
					if ( allowWallRuns ) {
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.0f;
						anim = BOTH_WALL_RUN_RIGHT;
					}
				}
				else if ( pm->cmd.forwardmove == 0 ) {//wall-flip
					if ( allowWallFlips ) {
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						anim = BOTH_WALL_FLIP_RIGHT;
					}
				}
			}
			else if ( pm->cmd.rightmove < 0 && pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1 ) {//strafing left
				if ( pm->cmd.forwardmove > 0 ) {//wall-run
					if ( allowWallRuns ) {
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.0f;
						anim = BOTH_WALL_RUN_LEFT;
					}
				}
				else if ( pm->cmd.forwardmove == 0 ) {//wall-flip
					if ( allowWallFlips ) {
						vertPush = forceJumpStrength[FORCE_LEVEL_2] / 2.25f;
						anim = BOTH_WALL_FLIP_LEFT;
					}
				}
			}
			else if ( pm->cmd.forwardmove < 0 && !(pm->cmd.buttons&BUTTON_ATTACK) ) {//backflip
				if ( allowFlips ) {
					vertPush = JUMP_VELOCITY;
					anim = BOTH_FLIP_BACK1;//BG_PickAnim( BOTH_FLIP_BACK1, BOTH_FLIP_BACK3 );
				}
			}

			vertPush += 128; //give them an extra shove

			if ( anim != -1 ) {
				vector3 fwd, right, traceto, mins, maxs, fwdAngles;
				vector3	idealNormal = { 0 }, wallNormal = { 0 };
				trace_t	trace;
				qboolean doTrace = qfalse;
				int contents = MASK_PLAYERSOLID;

				VectorSet( &mins, pm->mins.x, pm->mins.y, 0 );
				VectorSet( &maxs, pm->maxs.x, pm->maxs.y, 24 );
				VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

				memset( &trace, 0, sizeof(trace) ); //to shut the compiler up

				AngleVectors( &fwdAngles, &fwd, &right, NULL );

				//trace-check for a wall, if necc.
				switch ( anim ) {
				case BOTH_WALL_FLIP_LEFT:
				case BOTH_WALL_RUN_LEFT:
					doTrace = qtrue;
					VectorMA( &pm->ps->origin, -16, &right, &traceto );
					break;

				case BOTH_WALL_FLIP_RIGHT:
				case BOTH_WALL_RUN_RIGHT:
					doTrace = qtrue;
					VectorMA( &pm->ps->origin, 16, &right, &traceto );
					break;

				case BOTH_WALL_FLIP_BACK1:
					doTrace = qtrue;
					VectorMA( &pm->ps->origin, 16, &fwd, &traceto );
					break;
				default:
					break;
				}

				if ( doTrace ) {
					pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, contents );
					VectorCopy( &trace.plane.normal, &wallNormal );
					VectorNormalize( &wallNormal );
					VectorSubtract( &pm->ps->origin, &traceto, &idealNormal );
					VectorNormalize( &idealNormal );
				}

				if ( !doTrace || (trace.fraction < 1.0f && (trace.entityNum < MAX_CLIENTS || DotProduct( &wallNormal, &idealNormal ) > 0.7f)) ) {//there is a wall there.. or hit a client
					if ( (anim != BOTH_WALL_RUN_LEFT
						&& anim != BOTH_WALL_RUN_RIGHT
						&& anim != BOTH_FORCEWALLRUNFLIP_START)
						|| (wallNormal.z >= 0.0f&&wallNormal.z <= 0.4f/*MAX_WALL_RUN_Z_NORMAL*/) ) {//wall-runs can only run on perfectly flat walls, sorry.
						int parts;
						//move me to side
						if ( anim == BOTH_WALL_FLIP_LEFT ) {
							pm->ps->velocity.x = pm->ps->velocity.y = 0;
							VectorMA( &pm->ps->velocity, 150, &right, &pm->ps->velocity );
						}
						else if ( anim == BOTH_WALL_FLIP_RIGHT ) {
							pm->ps->velocity.x = pm->ps->velocity.y = 0;
							VectorMA( &pm->ps->velocity, -150, &right, &pm->ps->velocity );
						}
						else if ( anim == BOTH_FLIP_BACK1
							|| anim == BOTH_FLIP_BACK2
							|| anim == BOTH_FLIP_BACK3
							|| anim == BOTH_WALL_FLIP_BACK1 ) {
							pm->ps->velocity.x = pm->ps->velocity.y = 0;
							VectorMA( &pm->ps->velocity, -150, &fwd, &pm->ps->velocity );
						}

						if ( GetCInfo( CINFO_FLIPKICK ) ) {
							if ( doTrace && anim != BOTH_WALL_RUN_LEFT && anim != BOTH_WALL_RUN_RIGHT ) {
								bgEntity_t *kickedEnt = PM_BGEntForNum( trace.entityNum );
								if ( trace.entityNum < MAX_CLIENTS || kickedEnt->s.eType == ET_NPC )
									pm->ps->forceKickFlip = trace.entityNum + 1; //let the server know that this person gets kicked by this client
							}
						}

						//up
						if ( vertPush ) {
							pm->ps->velocity.z = vertPush;
							pm->ps->fd.forcePowersActive |= (1 << FP_LEVITATION);
						}
						//animate me
						parts = SETANIM_LEGS;
						if ( anim == BOTH_BUTTERFLY_LEFT ) {
							parts = SETANIM_BOTH;
							pm->cmd.buttons &= ~BUTTON_ATTACK;
							pm->ps->saberMove = LS_NONE;
						}
						else if ( !pm->ps->weaponTime ) {
							parts = SETANIM_BOTH;
						}
						PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
						if ( anim == BOTH_BUTTERFLY_LEFT ) {
							pm->ps->weaponTime = pm->ps->torsoTimer;
						}
						PM_SetForceJumpZStart( pm->ps->origin.z );//so we don't take damage if we land at same height
						pm->ps->pm_flags |= PMF_JUMP_HELD;
						pm->cmd.upmove = 0;
						pm->ps->fd.forceJumpSound = 1;
					}
				}
			}
		}
		else {//in the air
			animNumber_t legsAnim = pm->ps->legsAnim;

			if ( legsAnim == BOTH_WALL_RUN_LEFT || legsAnim == BOTH_WALL_RUN_RIGHT ) {//running on a wall
				vector3 right, traceto, mins, maxs, fwdAngles;
				trace_t	trace;
				int		anim = -1;

				VectorSet( &mins, pm->mins.x, pm->mins.x, 0 );
				VectorSet( &maxs, pm->maxs.x, pm->maxs.x, 24 );
				VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

				AngleVectors( &fwdAngles, NULL, &right, NULL );

				if ( legsAnim == BOTH_WALL_RUN_LEFT ) {
					if ( pm->ps->legsTimer > 400 ) {//not at the end of the anim
						float animLen = PM_AnimLength( 0, (animNumber_t)BOTH_WALL_RUN_LEFT );
						if ( pm->ps->legsTimer < animLen - 400 ) {//not at start of anim
							VectorMA( &pm->ps->origin, -16, &right, &traceto );
							anim = BOTH_WALL_RUN_LEFT_FLIP;
						}
					}
				}
				else if ( legsAnim == BOTH_WALL_RUN_RIGHT ) {
					if ( pm->ps->legsTimer > 400 ) {//not at the end of the anim
						float animLen = PM_AnimLength( 0, (animNumber_t)BOTH_WALL_RUN_RIGHT );
						if ( pm->ps->legsTimer < animLen - 400 ) {//not at start of anim
							VectorMA( &pm->ps->origin, 16, &right, &traceto );
							anim = BOTH_WALL_RUN_RIGHT_FLIP;
						}
					}
				}
				if ( anim != -1 ) {
					pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, CONTENTS_SOLID | CONTENTS_BODY );
					if ( trace.fraction < 1.0f ) {//flip off wall
						int parts = 0;

						if ( anim == BOTH_WALL_RUN_LEFT_FLIP ) {
							pm->ps->velocity.x *= 0.5f;
							pm->ps->velocity.y *= 0.5f;
							VectorMA( &pm->ps->velocity, 150, &right, &pm->ps->velocity );
						}
						else if ( anim == BOTH_WALL_RUN_RIGHT_FLIP ) {
							pm->ps->velocity.x *= 0.5f;
							pm->ps->velocity.y *= 0.5f;
							VectorMA( &pm->ps->velocity, -150, &right, &pm->ps->velocity );
						}
						parts = SETANIM_LEGS;
						if ( !pm->ps->weaponTime ) {
							parts = SETANIM_BOTH;
						}
						PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
						pm->cmd.upmove = 0;
					}
				}
				if ( pm->cmd.upmove != 0 ) {//jump failed, so don't try to do normal jump code, just return
					return qfalse;
				}
			}
			else if ( GetCInfo( CINFO_FLIPKICK )
				&& pm->cmd.forwardmove > 0 //pushing forward
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1
				&& pm->ps->velocity.z > 200
				&& PM_GroundDistance() <= 80 //unfortunately we do not have a happy ground timer like SP (this would use up more bandwidth if we wanted prediction workign right), so we'll just use the actual ground distance.
				&& !BG_InSpecialJump( pm->ps->legsAnim ) ) {//run up wall, flip backwards
				vector3 fwd, traceto, mins, maxs, fwdAngles;
				trace_t	trace;
				vector3	idealNormal;
				bgEntity_t *kickedEnt = NULL;

				VectorSet( &mins, pm->mins.x, pm->mins.y, pm->mins.z );
				VectorSet( &maxs, pm->maxs.x, pm->maxs.y, pm->maxs.z );
				VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

				AngleVectors( &fwdAngles, &fwd, NULL, NULL );
				VectorMA( &pm->ps->origin, 32, &fwd, &traceto );

				pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, MASK_PLAYERSOLID );//FIXME: clip brushes too?
				VectorSubtract( &pm->ps->origin, &traceto, &idealNormal );
				VectorNormalize( &idealNormal );

				kickedEnt = PM_BGEntForNum( trace.entityNum );

				if ( trace.fraction < 1.0f && (trace.entityNum < MAX_CLIENTS || kickedEnt->s.eType == ET_NPC) ) {//there is a wall there
					int parts = SETANIM_LEGS;

					pm->ps->velocity.x = pm->ps->velocity.y = 0;
					VectorMA( &pm->ps->velocity, -150, &fwd, &pm->ps->velocity );
					pm->ps->velocity.z += 128;

					if ( !pm->ps->weaponTime )
						parts = SETANIM_BOTH;
					PM_SetAnim( parts, BOTH_WALL_FLIP_BACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );

					pm->ps->legsTimer -= 600; //I force this anim to play to the end to prevent landing on your head and suddenly flipping over.
					//It is a bit too long at the end though, so I'll just shorten it.

					PM_SetForceJumpZStart( pm->ps->origin.z );//so we don't take damage if we land at same height
					pm->cmd.upmove = 0;
					pm->ps->fd.forceJumpSound = 1;
					BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 5 );

					pm->ps->forceKickFlip = trace.entityNum + 1; //let the server know that this person gets kicked by this client
				}
			}
			//NEW JKA
			else if ( pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_START ) {
				vector3 fwd, traceto, mins, maxs, fwdAngles;
				trace_t	trace;
				int		anim = -1;
				float animLen;

				VectorSet( &mins, pm->mins.x, pm->mins.x, 0.0f );
				VectorSet( &maxs, pm->maxs.x, pm->maxs.x, 24.0f );
				//hmm, did you mean [1] and [1]?
				VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0.0f );
				AngleVectors( &fwdAngles, &fwd, NULL, NULL );

				assert( pm_entSelf ); //null pm_entSelf would be a Bad Thing<tm>
				animLen = BG_AnimLength( pm_entSelf->localAnimIndex, BOTH_FORCEWALLRUNFLIP_START );
				if ( pm->ps->legsTimer < animLen - 400 ) {//not at start of anim
					VectorMA( &pm->ps->origin, 16, &fwd, &traceto );
					anim = BOTH_FORCEWALLRUNFLIP_END;
				}
				if ( anim != -1 ) {
					pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, CONTENTS_SOLID | CONTENTS_BODY );
					if ( trace.fraction < 1.0f ) {//flip off wall
						int parts = SETANIM_LEGS;

						pm->ps->velocity.x *= 0.5f;
						pm->ps->velocity.y *= 0.5f;
						VectorMA( &pm->ps->velocity, -300, &fwd, &pm->ps->velocity );
						pm->ps->velocity.z += 200;
						if ( !pm->ps->weaponTime ) {//not attacking, set anim on both
							parts = SETANIM_BOTH;
						}
						PM_SetAnim( parts, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
						//FIXME: do damage to traceEnt, like above?
						//pm->ps->pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
						//ha ha, so silly with your silly jumpy fally flags.
						pm->cmd.upmove = 0;
						PM_AddEvent( EV_JUMP );
					}
				}
				if ( pm->cmd.upmove != 0 ) {//jump failed, so don't try to do normal jump code, just return
					return qfalse;
				}
			}
			else if ( pm->cmd.forwardmove > 0 //pushing forward
				//	&& pm->ps->fd.forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_1
				&& PM_WalkableGroundDistance() <= 80 //unfortunately we do not have a happy ground timer like SP (this would use up more bandwidth if we wanted prediction workign right), so we'll just use the actual ground distance.
				&& (pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_INAIR1) )//not in a flip or spin or anything
			{//run up wall, flip backwards
				if ( allowWallRuns ) {
					//FIXME: have to be moving... make sure it's opposite the wall... or at least forward?
					int wallWalkAnim = BOTH_WALL_FLIP_BACK1;
					int parts = SETANIM_LEGS;
					int contents = MASK_PLAYERSOLID;
					qboolean kick = qtrue;
					if ( pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2 ) {
						wallWalkAnim = BOTH_FORCEWALLRUNFLIP_START;
						parts = SETANIM_BOTH;
						kick = qfalse;
					}
					else {
						if ( !pm->ps->weaponTime ) {
							parts = SETANIM_BOTH;
						}
					}
					//if ( PM_HasAnimation( pm->gent, wallWalkAnim ) )
					if ( 1 ) //sure, we have it! Because I SAID SO.
					{
						vector3 fwd, traceto, mins, maxs, fwdAngles;
						trace_t	trace;
						vector3	idealNormal;
						bgEntity_t *traceEnt;

						VectorSet( &mins, pm->mins.x, pm->mins.y, 0.0f );
						VectorSet( &maxs, pm->maxs.x, pm->maxs.y, 24.0f );
						VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0.0f );

						AngleVectors( &fwdAngles, &fwd, NULL, NULL );
						VectorMA( &pm->ps->origin, 32, &fwd, &traceto );

						pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, contents );//FIXME: clip brushes too?
						VectorSubtract( &pm->ps->origin, &traceto, &idealNormal );
						VectorNormalize( &idealNormal );
						traceEnt = PM_BGEntForNum( trace.entityNum );

						if ( trace.fraction < 1.0f
							&& ((trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid != SOLID_BMODEL) || DotProduct( &trace.plane.normal, &idealNormal )>0.7f) ) {//there is a wall there
							pm->ps->velocity.x = pm->ps->velocity.y = 0;
							if ( wallWalkAnim == BOTH_FORCEWALLRUNFLIP_START ) {
								pm->ps->velocity.z = forceJumpStrength[FORCE_LEVEL_3] / 2.0f;
							}
							else {
								VectorMA( &pm->ps->velocity, -150, &fwd, &pm->ps->velocity );
								pm->ps->velocity.z += 150.0f;
							}
							//animate me
							PM_SetAnim( parts, wallWalkAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
							//						pm->ps->pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
							//again with the flags!
							//G_SoundOnEnt( pm->gent, CHAN_BODY, "sound/weapons/force/jump.wav" );
							//yucky!
							PM_SetForceJumpZStart( pm->ps->origin.z );//so we don't take damage if we land at same height
							pm->cmd.upmove = 0;
							pm->ps->fd.forceJumpSound = 1;
							BG_ForcePowerDrain( pm->ps, FP_LEVITATION, 5 );

							//kick if jumping off an ent
							if ( GetCInfo( CINFO_FLIPKICK ) && kick && traceEnt && (traceEnt->s.eType == ET_PLAYER || traceEnt->s.eType == ET_NPC) ) { //kick that thang!
								pm->ps->forceKickFlip = traceEnt->s.number + 1;
							}
							pm->cmd.rightmove = pm->cmd.forwardmove = 0;
						}
					}
				}
			}
			else if ( (!BG_InSpecialJump( legsAnim )//not in a special jump anim
				|| BG_InReboundJump( legsAnim )//we're already in a rebound
				|| BG_InBackFlip( legsAnim ))//a backflip (needed so you can jump off a wall behind you)
				//&& pm->ps->velocity[2] <= 0
				&& pm->ps->velocity.z > -1200 //not falling down very fast
				&& !(pm->ps->pm_flags&PMF_JUMP_HELD)//have to have released jump since last press
				&& (pm->cmd.forwardmove || pm->cmd.rightmove)//pushing in a direction
				//&& pm->ps->forceRageRecoveryTime < pm->cmd.serverTime	//not in a force Rage recovery period
				&& pm->ps->fd.forcePowerLevel[FP_LEVITATION] > FORCE_LEVEL_2//level 3 jump or better
				//&& WP_ForcePowerAvailable( pm->gent, FP_LEVITATION, 10 )//have enough force power to do another one
				&& BG_CanUseFPNow( pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION )
				&& (pm->ps->origin.z - pm->ps->fd.forceJumpZStart) < (forceJumpHeightMax[FORCE_LEVEL_3] - (BG_ForceWallJumpStrength() / 2.0f)) //can fit at least one more wall jump in (yes, using "magic numbers"... for now)
				//&& (pm->ps->legsAnim == BOTH_JUMP1 || pm->ps->legsAnim == BOTH_INAIR1 ) )//not in a flip or spin or anything
				&& !PM_CheckGrab() ) {//see if we're pushing at a wall and jump off it if so
				if ( allowWallGrabs ) {
					//FIXME: make sure we have enough force power
					//FIXME: check  to see if we can go any higher
					//FIXME: limit to a certain number of these in a row?
					//FIXME: maybe don't require a ucmd direction, just check all 4?
					//FIXME: should stick to the wall for a second, then push off...
					vector3 checkDir, traceto, mins, maxs, fwdAngles;
					trace_t	trace;
					vector3	idealNormal;
					int		anim = -1;

					VectorSet( &mins, pm->mins.x, pm->mins.y, 0.0f );
					VectorSet( &maxs, pm->maxs.x, pm->maxs.y, 24.0f );
					VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0.0f );

					if ( pm->cmd.rightmove ) {
						if ( pm->cmd.rightmove > 0 ) {
							anim = BOTH_FORCEWALLREBOUND_RIGHT;
							AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
						}
						else if ( pm->cmd.rightmove < 0 ) {
							anim = BOTH_FORCEWALLREBOUND_LEFT;
							AngleVectors( &fwdAngles, NULL, &checkDir, NULL );
							VectorScale( &checkDir, -1, &checkDir );
						}
					}
					else if ( pm->cmd.forwardmove > 0 ) {
						anim = BOTH_FORCEWALLREBOUND_FORWARD;
						AngleVectors( &fwdAngles, &checkDir, NULL, NULL );
					}
					else if ( pm->cmd.forwardmove < 0 ) {
						anim = BOTH_FORCEWALLREBOUND_BACK;
						AngleVectors( &fwdAngles, &checkDir, NULL, NULL );
						VectorScale( &checkDir, -1, &checkDir );
					}
					if ( anim != -1 ) {//trace in the dir we're pushing in and see if there's a vertical wall there
						bgEntity_t *traceEnt;

						VectorMA( &pm->ps->origin, 8, &checkDir, &traceto );
						pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, CONTENTS_SOLID );//FIXME: clip brushes too?
						VectorSubtract( &pm->ps->origin, &traceto, &idealNormal );
						VectorNormalize( &idealNormal );
						traceEnt = PM_BGEntForNum( trace.entityNum );
						if ( trace.fraction < 1.0f
							&&fabsf( trace.plane.normal.z ) <= 0.2f/*MAX_WALL_GRAB_SLOPE*/
							&& ((trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid != SOLID_BMODEL) || DotProduct( &trace.plane.normal, &idealNormal )>0.7f) ) {//there is a wall there
							float dot = DotProduct( &pm->ps->velocity, &trace.plane.normal );
							if ( dot < 1.0f ) {//can't be heading *away* from the wall!
								//grab it!
								PM_GrabWallForJump( anim );
							}
						}
					}
				}
			}
			else {
				//FIXME: if in a butterfly, kick people away?
			}
			//END NEW JKA
		}
	}

	/*
	if ( pm->cmd.upmove > 0
	&& (pm->ps->weapon == WP_SABER || pm->ps->weapon == WP_MELEE)
	&& !PM_IsRocketTrooper()
	&& (pm->ps->weaponTime > 0||pm->cmd.buttons&BUTTON_ATTACK) )
	{//okay, we just jumped and we're in an attack
	if ( !BG_InRoll( pm->ps, pm->ps->legsAnim )
	&& !PM_InKnockDown( pm->ps )
	&& !BG_InDeathAnim(pm->ps->legsAnim)
	&& !BG_FlippingAnim( pm->ps->legsAnim )
	&& !PM_SpinningAnim( pm->ps->legsAnim )
	&& !BG_SaberInSpecialAttack( pm->ps->torsoAnim )
	&& ( BG_SaberInAttack( pm->ps->saberMove ) ) )
	{//not in an anim we shouldn't interrupt
	//see if it's not too late to start a special jump-attack
	float animLength = PM_AnimLength( 0, (animNumber_t)pm->ps->torsoAnim );
	if ( animLength - pm->ps->torsoTimer < 500 )
	{//just started the saberMove
	//check for special-case jump attacks
	if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_2 )
	{//using medium attacks
	if (PM_GroundDistance() < 32 &&
	!BG_InSpecialJump(pm->ps->legsAnim))
	{ //FLIP AND DOWNWARD ATTACK
	//trace_t tr;

	//if (PM_SomeoneInFront(&tr))
	{
	PM_SetSaberMove(PM_SaberFlipOverAttackMove());
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	VectorClear(pml.groundTrace.plane.normal);

	pm->ps->weaponTime = pm->ps->torsoTimer;
	}
	}
	}
	else if ( pm->ps->fd.saberAnimLevel == FORCE_LEVEL_3 )
	{//using strong attacks
	if ( pm->cmd.forwardmove > 0 && //going forward
	(pm->cmd.buttons & BUTTON_ATTACK) && //must be holding attack still
	PM_GroundDistance() < 32 &&
	!BG_InSpecialJump(pm->ps->legsAnim))
	{//strong attack: jump-hack
	PM_SetSaberMove( PM_SaberJumpAttackMove() );
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	VectorClear(pml.groundTrace.plane.normal);

	pm->ps->weaponTime = pm->ps->torsoTimer;
	}
	}
	}
	}
	}
	*/
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		return qfalse;
	}
	if ( pm->cmd.upmove > 0 ) {//no special jumps
		const float jumpVelocity = GetCInfo( CINFO_VQ3PHYS ) ? 270.0f : JUMP_VELOCITY;
		pm->ps->velocity.z = jumpVelocity;

		PM_SetForceJumpZStart( pm->ps->origin.z );//so we don't take damage if we land at same height
		pm->ps->pm_flags |= PMF_JUMP_HELD;

		if ( GetCInfo( CINFO_CPMPHYSICS ) ) {
			float dot = 0.0f, speed = 0.0f;
			const float jumpVel = jumpVelocity;//270.0f;
			vector3 dir;

			// first the double jump
			if ( pm->ps->stats[STAT_JUMPTIME] > 0 ) {
				pm->ps->velocity.z = jumpVel*2.0f;
				pm->ps->pm_flags &= ~PMF_JUMP_HELD;
			}

			pm->ps->stats[STAT_JUMPTIME] = 400;

			// now the ramp jump
			speed = VectorNormalize2( &pm->ps->velocity, &dir );
			dir.z = 0.0f;

			dot = DotProduct( &dir, &pml.groundTrace.plane.normal ) * -1.0f;
			if ( pml.groundPlane && dot > 0.0f ) {
				pm->ps->velocity.z += dot*speed;
				pm->ps->pm_flags &= ~PMF_JUMP_HELD;
			}
		}
	}

	//Jumping
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	//	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	//	PM_SetForceJumpZStart(pm->ps->origin.z);

	PM_AddEvent( EV_JUMP );

	//Set the animations
	if ( pm->ps->gravity > 0 && !BG_InSpecialJump( pm->ps->legsAnim ) ) {
		PM_JumpForDir();
	}

	return qtrue;
}


/*
=============
PM_CheckWaterJump
=============
*/
static qboolean	PM_CheckWaterJump( void ) {
	vector3	spot;
	int		cont;
	vector3	flatforward;

	if ( pm->ps->pm_time ) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward.x = pml.forward.x;
	flatforward.y = pml.forward.y;
	flatforward.z = 0;
	VectorNormalize( &flatforward );

	VectorMA( &pm->ps->origin, 30, &flatforward, &spot );
	spot.z += 4;
	cont = pm->pointcontents( &spot, pm->ps->clientNum );
	if ( !(cont & CONTENTS_SOLID) ) {
		return qfalse;
	}

	spot.z += 16;
	cont = pm->pointcontents( &spot, pm->ps->clientNum );
	if ( cont ) {
		return qfalse;
	}

	// jump out of water
	VectorScale( &pml.forward, 200, &pm->ps->velocity );
	pm->ps->velocity.z = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity.z -= pm->ps->gravity * pml.frametime;
	if ( pm->ps->velocity.z < 0 ) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	vector3	wishvel;
	float	wishspeed;
	vector3	wishdir;
	float	scale;
	float	vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if (pm->ps->velocity.z > -300) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity.z = 100;
			} else if (pm->watertype == CONTENTS_SLIME) {
				pm->ps->velocity.z = 80;
			} else {
				pm->ps->velocity.z = 50;
			}
		}
	}
#endif
	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel.x = 0;
		wishvel.y = 0;
		wishvel.z = -60;		// sink towards bottom
	}
	else {
		wishvel.x = scale * pml.forward.x*pm->cmd.forwardmove + scale * pml.right.x*pm->cmd.rightmove;
		wishvel.y = scale * pml.forward.y*pm->cmd.forwardmove + scale * pml.right.y*pm->cmd.rightmove;
		wishvel.z = scale * pml.forward.z*pm->cmd.forwardmove + scale * pml.right.z*pm->cmd.rightmove;

		wishvel.z += scale * pm->cmd.upmove;
	}

	VectorCopy( &wishvel, &wishdir );
	wishspeed = VectorNormalize( &wishdir );

	if ( wishspeed > pm->ps->speed * pm_swimScale ) {
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate( &wishdir, wishspeed, pm_wateraccelerate );

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( &pm->ps->velocity, &pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength( &pm->ps->velocity );
		// slide along the ground plane
		PM_ClipVelocity( &pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );

		//Raz: overbounce
		if ( pm->overbounce ) {
			VectorNormalize( &pm->ps->velocity );
			VectorScale( &pm->ps->velocity, vel, &pm->ps->velocity );
		}
	}

	PM_SlideMove( qfalse );
}

/*
===================
PM_FlyVehicleMove

===================
*/
static void PM_FlyVehicleMove( void ) {
	int		i;
	vector3	wishvel;
	float	wishspeed;
	vector3	wishdir;
	float	scale;
	float	zVel;
	float	fmove = 0.0f, smove = 0.0f;

	// We don't use these here because we pre-calculate the movedir in the vehicle update anyways, and if
	// you leave this, you get strange motion during boarding (the player can move the vehicle).
	//fmove = pm->cmd.forwardmove;
	//smove = pm->cmd.rightmove;

	// normal slowdown
	if ( pm->ps->gravity && pm->ps->velocity.z < 0 && pm->ps->groundEntityNum == ENTITYNUM_NONE ) {//falling
		zVel = pm->ps->velocity.z;
		PM_Friction();
		pm->ps->velocity.z = zVel;
	}
	else {
		PM_Friction();
		if ( pm->ps->velocity.z < 0 && pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
			pm->ps->velocity.z = 0;	// ignore slope movement
		}
	}

	scale = PM_CmdScale( &pm->cmd );

	// Get The WishVel And WishSpeed
	//-------------------------------
	if ( pm->ps->clientNum >= MAX_CLIENTS ) {//NPC

		// If The UCmds Were Set, But Never Converted Into A MoveDir, Then Make The WishDir From UCmds
		//--------------------------------------------------------------------------------------------
		if ( (fmove != 0.0f || smove != 0.0f) && VectorCompare( &pm->ps->moveDir, &vec3_origin ) ) {
			//gi.Printf("Generating MoveDir\n");
			for ( i = 0; i < 3; i++ ) {
				wishvel.data[i] = pml.forward.data[i] * fmove + pml.right.data[i] * smove;
			}

			VectorCopy( &wishvel, &wishdir );
			wishspeed = VectorNormalize( &wishdir );
			wishspeed *= scale;
		}
		// Otherwise, Use The Move Dir
		//-----------------------------
		else {
			wishspeed = pm->ps->speed;
			VectorScale( &pm->ps->moveDir, pm->ps->speed, &wishvel );
			VectorCopy( &pm->ps->moveDir, &wishdir );
		}
	}
	else {
		for ( i = 0; i < 3; i++ ) {
			wishvel.data[i] = pml.forward.data[i] * fmove + pml.right.data[i] * smove;
		}
		// when going up or down slopes the wish velocity should Not be zero
		//	wishvel[2] = 0;

		VectorCopy( &wishvel, &wishdir );
		wishspeed = VectorNormalize( &wishdir );
		wishspeed *= scale;
	}

	// Handle negative speed.
	if ( wishspeed < 0 ) {
		VectorScale( &wishvel, -1.0f, &wishvel );
		VectorScale( &wishdir, -1.0f, &wishdir );
	}

	VectorCopy( &wishvel, &wishdir );
	wishspeed = VectorNormalize( &wishdir );

	PM_Accelerate( &wishdir, wishspeed, 100 );

	PM_StepSlideMove( qtrue );
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	vector3	wishvel;
	float	wishspeed;
	vector3	wishdir;
	float	scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );

	if ( pm->ps->pm_type == PM_SPECTATOR && pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
		//turbo boost
		scale *= 10;
	}

	//
	// user intentions
	//
	if ( !scale ) {
		wishvel.x = 0;
		wishvel.y = 0;
		wishvel.z = pm->ps->speed * (pm->cmd.upmove / 127.0f);
	}
	else {
		wishvel.x = scale * pml.forward.x*pm->cmd.forwardmove + scale * pml.right.x*pm->cmd.rightmove;
		wishvel.y = scale * pml.forward.y*pm->cmd.forwardmove + scale * pml.right.y*pm->cmd.rightmove;
		wishvel.z = scale * pml.forward.z*pm->cmd.forwardmove + scale * pml.right.z*pm->cmd.rightmove;

		wishvel.z += scale * pm->cmd.upmove;
	}

	VectorCopy( &wishvel, &wishdir );
	wishspeed = VectorNormalize( &wishdir );

	PM_Accelerate( &wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/

static void PM_AirMove( void ) {
	vector3		wishVel, wishDir;
	float		fmove, smove, wishSpeed, wishSpeed2;
	float		scale, accelerate;
	usercmd_t	cmd;
	Vehicle_t	*pVeh = NULL;
	qboolean	promode = GetCInfo( CINFO_CPMPHYSICS );

	if ( pm->ps->clientNum >= MAX_CLIENTS ) {
		bgEntity_t	*pEnt = pm_entSelf;
		if ( pEnt && pEnt->s.NPC_class == CLASS_VEHICLE )
			pVeh = pEnt->m_pVehicle;
	}

	if ( pm->ps->pm_type != PM_SPECTATOR && !GetCInfo( CINFO_VQ3PHYS ) ) {
#if METROID_JUMP
		PM_CheckJump();
#else
		if ( pm->ps->fd.forceJumpZStart && pm->ps->forceJumpFlip )
			PM_CheckJump();
#endif
		//	PM_CheckGrab();
	}
	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward.z = 0;
	pml.right.z = 0;
	VectorNormalize( &pml.forward );
	VectorNormalize( &pml.right );

	if ( pVeh && pVeh->m_pVehicleInfo->hoverHeight > 0 ) {
		// in a hovering vehicle, have air control
		if ( 1 ) {
			VectorScale( &pm->ps->moveDir, pm->ps->speed, &wishVel );
			VectorCopy( &pm->ps->moveDir, &wishDir );
			scale = 1.0f;
		}
	}
	else if ( gPMDoSlowFall ) {
		// no air-control
		VectorClear( &wishVel );
	}
	else {
		// reduced air control while not jetting
		wishVel.x = pml.forward.x*fmove + pml.right.x*smove;
		wishVel.y = pml.forward.y*fmove + pml.right.y*smove;
		wishVel.z = 0;

		if ( pm->ps->pm_type == PM_JETPACK ) {
			// if we are jetting then we have more control than usual
			if ( pm->cmd.upmove <= 0 )
				VectorScale( &wishVel, 0.8f, &wishVel );
			else
				VectorScale( &wishVel, 2.0f, &wishVel );
		}
	}

	VectorCopy( &wishVel, &wishDir );
	wishSpeed = VectorNormalize( &wishDir );
	wishSpeed *= scale;
	wishSpeed2 = wishSpeed;

	if ( promode ) {
		if ( DotProduct( &pm->ps->velocity, &wishDir ) < 0.0f )
			accelerate = cpm_pm_airstopaccelerate;
		else
			accelerate = pm_airaccelerate;
		if ( pm->ps->movementDir == 2 || pm->ps->movementDir == 6 ) {
			if ( wishSpeed > cpm_pm_wishspeed )
				wishSpeed = cpm_pm_wishspeed;
			accelerate = cpm_pm_strafeaccelerate;
		}
	}
	else
		accelerate = pm_airaccelerate;

	// speeders have more control in air
	if ( pVeh && pVeh->m_pVehicleInfo->type == VH_SPEEDER ) {
		//in mid-air
		accelerate = pVeh->m_pVehicleInfo->traction;
		// on a slope of some kind, shouldn't have much control and should slide a lot
		if ( pml.groundPlane )
			accelerate *= 0.5f;
	}
	// not on ground, so little effect on velocity
	PM_Accelerate( &wishDir, wishSpeed, accelerate );

	//Raz: CPM Physics
	if ( promode )
		CPM_PM_Aircontrol( pm, &wishDir, wishSpeed2 );

	// we may have a ground plane that is very steep, even though we don't have a groundentity slide along the steep plane
	if ( pml.groundPlane ) {
		if ( !(pm->ps->pm_flags & PMF_STUCK_TO_WALL) ) {
			// don't slide when stuck to a wall
			if ( PM_GroundSlideOkay( pml.groundTrace.plane.normal.z ) )
				PM_ClipVelocity( &pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );
		}
	}

	// no grav when stuck to wall
	if ( (pm->ps->pm_flags & PMF_STUCK_TO_WALL) )
		PM_StepSlideMove( qfalse );
	else
		PM_StepSlideMove( qtrue );
}

static void PM_GrappleMove( void ) {
	vector3 vel = { 0.0f };
	vector3 v = { 0.0f };
	float vLen = 0.0f;

	VectorScale( &pml.forward, -16, &v );
	VectorAdd( &pm->ps->lastHitLoc, &v, &v );
	VectorSubtract( &v, &pm->ps->origin, &vel );
	vLen = VectorLength( &vel );
	VectorNormalize( &vel );

	if ( vLen <= 100.0f )
		VectorScale( &vel, 10 * vLen, &vel );
	else
		VectorScale( &vel, 800, &vel );

	VectorCopy( &vel, &pm->ps->velocity );

	pml.groundPlane = qfalse;
}

#if 0
static void PM_GrappleSwing( void )
{
	vector3	grapplePos	= { 0.0f };
	vector3	grappleDir	= { 0.0f };
	float	dist		= 0.0f;
#ifdef _CGAME
	VectorCopy( cg_entities[cg_entities[pm_entSelf->s.number].bolt1].currentState.pos.trBase, grapplePos );
#else
	VectorCopy( g_entities[pm_entSelf->s.number].client->hook->s.pos.trBase, grapplePos );
#endif

	dist = Distance( pm->ps->origin, grapplePos );
	VectorSubtract( grapplePos, pm->ps->origin, grappleDir );
	VectorNormalize( grappleDir );
	VectorMA( pm->ps->origin, dist, grappleDir, pm->ps->origin );
	pml.groundPlane = false;

}
#else
void PM_GrappleSwing( void ) {
	vector3 dist;
	float length, length2;

	VectorSubtract( &pm->ps->lastHitLoc, &pml.previous_origin, &dist );

	length = VectorLength( &dist );

	if ( length > 0.0f ) {
		float	Unknown1, Unknown2;
		vector3	UnknownVec;

		VectorSubtract( &pm->ps->lastHitLoc, &pm->ps->origin, &dist );
		length2 = VectorLength( &dist );
		VectorNormalize( &dist );

		Unknown1 = pm->ps->gravity * length2 / length * pml.frametime;

		VectorScale( &dist, Unknown1, &UnknownVec );
		VectorAdd( &UnknownVec, &pm->ps->velocity, &UnknownVec );

		Unknown2 = UnknownVec.z*dist.z + UnknownVec.y*dist.y + UnknownVec.x*dist.x;
		pm->ps->velocity.x = -Unknown2 * dist.x + UnknownVec.x;
		pm->ps->velocity.y = -Unknown2 * dist.y + UnknownVec.y;
		pm->ps->velocity.z = -Unknown2 * dist.z + UnknownVec.z;
	}

	pml.groundPlane = qfalse;

	//RAZTODO: Play animation, similar to force jump animation playing
	//	sub_2001AB20();
}
#endif

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vector3		wishvel;
	float		fmove, smove;
	vector3		wishdir;
	float		wishspeed = 0.0f;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;
	qboolean	npcMovement = qfalse;

	if ( pm->waterlevel > 2 && DotProduct( &pml.forward, &pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( pm->ps->pm_type != PM_SPECTATOR ) {
		if ( PM_CheckJump() ) {
			// jumped away
			if ( pm->waterlevel > 1 ) {
				PM_WaterMove();
			}
			else {
				PM_AirMove();
			}
			return;
		}
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward.z = 0;
	pml.right.z = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( &pml.forward, &pml.groundTrace.plane.normal, &pml.forward, OVERCLIP );
	PM_ClipVelocity( &pml.right, &pml.groundTrace.plane.normal, &pml.right, OVERCLIP );
	//
	VectorNormalize( &pml.forward );
	VectorNormalize( &pml.right );

	// Get The WishVel And WishSpeed
	//-------------------------------
	if ( pm->ps->clientNum >= MAX_CLIENTS && !VectorCompare( &pm->ps->moveDir, &vec3_origin ) ) {//NPC
		bgEntity_t *pEnt = pm_entSelf;

		if ( pEnt && pEnt->s.NPC_class == CLASS_VEHICLE ) {
			// If The UCmds Were Set, But Never Converted Into A MoveDir, Then Make The WishDir From UCmds
			//--------------------------------------------------------------------------------------------
			if ( (fmove != 0.0f || smove != 0.0f) && VectorCompare( &pm->ps->moveDir, &vec3_origin ) ) {
				//gi.Printf("Generating MoveDir\n");
				for ( i = 0; i < 3; i++ ) {
					wishvel.data[i] = pml.forward.data[i] * fmove + pml.right.data[i] * smove;
				}

				VectorCopy( &wishvel, &wishdir );
				wishspeed = VectorNormalize( &wishdir );
				wishspeed *= scale;
			}
			// Otherwise, Use The Move Dir
			//-----------------------------
			else {
				//wishspeed = pm->ps->speed;
				VectorScale( &pm->ps->moveDir, pm->ps->speed, &wishvel );
				VectorCopy( &wishvel, &wishdir );
				wishspeed = VectorNormalize( &wishdir );
			}

			npcMovement = qtrue;
		}
	}

	if ( !npcMovement ) {
		for ( i = 0; i < 3; i++ ) {
			wishvel.data[i] = pml.forward.data[i] * fmove + pml.right.data[i] * smove;
		}
		// when going up or down slopes the wish velocity should Not be zero

		VectorCopy( &wishvel, &wishdir );
		wishspeed = VectorNormalize( &wishdir );
		wishspeed *= scale;
	}

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( wishspeed > pm->ps->speed * PM_DuckScale() ) {
			wishspeed = pm->ps->speed * PM_DuckScale();
		}
	}
	else if ( (pm->ps->pm_flags & PMF_ROLLING) && !BG_InRoll( pm->ps, pm->ps->legsAnim ) &&
		!PM_InRollComplete( pm->ps, pm->ps->legsAnim ) ) {
		if ( wishspeed > pm->ps->speed * PM_DuckScale() ) {
			wishspeed = pm->ps->speed * PM_DuckScale();
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float	waterScale;

		waterScale = pm->waterlevel / 3.0f;
		waterScale = 1.0f - (1.0f - pm_swimScale) * waterScale;
		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( pm_flying == FLY_HOVER ) {
		accelerate = pm_vehicleaccelerate;
	}
	else if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	}
	else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate( &wishdir, wishspeed, accelerate );
	/*
	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
	#ifdef _GAME
	Com_Printf(S_COLOR_RED"S: %f, %f\n", wishspeed, pm->ps->speed);
	#else
	Com_Printf(S_COLOR_GREEN"C: %f, %f\n", wishspeed, pm->ps->speed);
	#endif
	}
	*/

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity.x, pm->ps->velocity.y, pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( (pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity.z -= pm->ps->gravity * pml.frametime;
	}

	vel = VectorLength( &pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity( &pm->ps->velocity, &pml.groundTrace.plane.normal, &pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize( &pm->ps->velocity );
	VectorScale( &pm->ps->velocity, vel, &pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity.x && !pm->ps->velocity.y ) {
		return;
	}

	PM_StepSlideMove( qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength( &pm->ps->velocity );
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear( &pm->ps->velocity );
	}
	else {
		VectorNormalize( &pm->ps->velocity );
		VectorScale( &pm->ps->velocity, forward, &pm->ps->velocity );
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vector3		wishvel;
	float		fmove, smove;
	vector3		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength( &pm->ps->velocity );
	if ( speed < 1 ) {
		VectorCopy( &vec3_origin, &pm->ps->velocity );
	}
	else {
		drop = 0;

		friction = pm_friction*1.5f;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 )
			newspeed = 0;
		newspeed /= speed;

		VectorScale( &pm->ps->velocity, newspeed, &pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );
	if ( pm->cmd.buttons & BUTTON_ATTACK ) {	//turbo boost
		scale *= 10;
	}
	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {	//turbo boost
		scale *= 10;
	}

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for ( i = 0; i < 3; i++ )
		wishvel.data[i] = pml.forward.data[i] * fmove + pml.right.data[i] * smove;
	wishvel.z += pm->cmd.upmove;

	VectorCopy( &wishvel, &wishdir );
	wishspeed = VectorNormalize( &wishdir );
	wishspeed *= scale;

	PM_Accelerate( &wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA( &pm->ps->origin, pml.frametime, &pm->ps->velocity, &pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	return (pml.groundTrace.surfaceFlags & MATERIAL_MASK);
}

extern qboolean PM_CanRollFromSoulCal( playerState_t *ps );
static int PM_TryRoll( void ) {
	trace_t	trace;
	int		anim = -1;
	vector3 fwd, right, traceto, mins, maxs, fwdAngles;
	qboolean jpRollAllowed = GetCInfo( CINFO_WEAPONROLL ) ? qtrue : !!(pm->ps->weapon == WP_SABER || pm->ps->weapon == WP_MELEE);

	if ( BG_SaberInAttack( pm->ps->saberMove ) || BG_SaberInSpecialAttack( pm->ps->torsoAnim )
		|| BG_SpinningSaberAnim( pm->ps->legsAnim )
		|| PM_SaberInStart( pm->ps->saberMove ) ) {//attacking or spinning (or, if player, starting an attack)
		if ( PM_CanRollFromSoulCal( pm->ps ) ) {//hehe
		}
		else {
			return 0;
		}
	}

	if ( !jpRollAllowed ||
		PM_IsRocketTrooper() ||
		BG_HasYsalamiri( pm->gametype, pm->ps ) ||
		!BG_CanUseFPNow( pm->gametype, pm->ps, pm->cmd.serverTime, FP_LEVITATION ) ) { //Not using saber, or can't use jump
		return 0;
	}

	if ( pm->ps->weapon == WP_SABER ) {
		saberInfo_t *saber = BG_MySaber( pm->ps->clientNum, 0 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_ROLLS) ) {
			return 0;
		}
		saber = BG_MySaber( pm->ps->clientNum, 1 );
		if ( saber
			&& (saber->saberFlags&SFL_NO_ROLLS) ) {
			return 0;
		}
	}

	VectorSet( &mins, pm->mins.x, pm->mins.y, pm->mins.z + STEPSIZE );
	VectorSet( &maxs, pm->maxs.x, pm->maxs.y, pm->ps->crouchheight );

	VectorSet( &fwdAngles, 0, pm->ps->viewangles.yaw, 0 );

	AngleVectors( &fwdAngles, &fwd, &right, NULL );

	if ( pm->cmd.forwardmove ) { //check forward/backward rolls
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			anim = BOTH_ROLL_B;
			VectorMA( &pm->ps->origin, -64, &fwd, &traceto );
		}
		else {
			anim = BOTH_ROLL_F;
			VectorMA( &pm->ps->origin, 64, &fwd, &traceto );
		}
	}
	else if ( pm->cmd.rightmove > 0 ) { //right
		anim = BOTH_ROLL_R;
		VectorMA( &pm->ps->origin, 64, &right, &traceto );
	}
	else if ( pm->cmd.rightmove < 0 ) { //left
		anim = BOTH_ROLL_L;
		VectorMA( &pm->ps->origin, -64, &right, &traceto );
	}

	if ( anim != -1 ) { //We want to roll. Perform a trace to see if we can, and if so, send us into one.
		pm->trace( &trace, &pm->ps->origin, &mins, &maxs, &traceto, pm->ps->clientNum, CONTENTS_SOLID );
		if ( trace.fraction >= 1.0f ) {
			pm->ps->saberMove = LS_NONE;
			return anim;
		}
	}
	return 0;
}

#ifdef _GAME
static void PM_CrashLandEffect( void ) {
	float delta;
	if ( pm->waterlevel ) {
		return;
	}
	delta = fabsf( pml.previous_velocity.z ) / 10;//VectorLength( pml.previous_velocity );?
	if ( delta >= 30 ) {
		vector3 bottom;
		int	effectID;
		int material = (pml.groundTrace.surfaceFlags&MATERIAL_MASK);
		VectorSet( &bottom, pm->ps->origin.x, pm->ps->origin.y, pm->ps->origin.z + pm->mins.z + 1 );
		switch ( material ) {
		case MATERIAL_MUD:
			effectID = EFFECT_LANDING_MUD;
			break;

		case MATERIAL_SAND:
			effectID = EFFECT_LANDING_SAND;
			break;

		case MATERIAL_DIRT:
			effectID = EFFECT_LANDING_DIRT;
			break;

		case MATERIAL_SNOW:
			effectID = EFFECT_LANDING_SNOW;
			break;

		case MATERIAL_GRAVEL:
			effectID = EFFECT_LANDING_GRAVEL;
			break;

		default:
			effectID = -1;
			break;
		}

		if ( effectID != -1 ) {
			G_PlayEffect( effectID, &bottom, &pml.groundTrace.plane.normal );
		}
	}
}
#endif
/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;
	qboolean	didRoll = qfalse;

	// calculate the exact velocity on landing
	dist = pm->ps->origin.z - pml.previous_origin.z;
	vel = pml.previous_velocity.z;
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if ( den < 0 ) {
		pm->ps->inAirAnim = qfalse;
		return;
	}
	t = (-b - sqrtf( den )) / (2 * a);

	delta = vel + t * acc;
	delta = delta*delta * 0.0001f;

#ifdef _GAME
	PM_CrashLandEffect();
#endif
	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	if ( pm->ps->legsAnim == BOTH_A7_KICK_F_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_B_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_R_AIR ||
		pm->ps->legsAnim == BOTH_A7_KICK_L_AIR ) {
		int landAnim;
		switch ( pm->ps->legsAnim ) {
		case BOTH_A7_KICK_F_AIR:
			landAnim = BOTH_FORCELAND1;
			break;
		case BOTH_A7_KICK_B_AIR:
			landAnim = BOTH_FORCELANDBACK1;
			break;
		case BOTH_A7_KICK_R_AIR:
			landAnim = BOTH_FORCELANDRIGHT1;
			break;
		case BOTH_A7_KICK_L_AIR:
			landAnim = BOTH_FORCELANDLEFT1;
			break;
		default:
			landAnim = -1;
			break;
		}
		if ( landAnim != -1 ) {
			if ( pm->ps->torsoAnim == pm->ps->legsAnim ) {
				PM_SetAnim( SETANIM_BOTH, landAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
			else {
				PM_SetAnim( SETANIM_LEGS, landAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
		}
	}
	else if ( pm->ps->legsAnim == BOTH_FORCEJUMPLEFT1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMPRIGHT1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMPBACK1 ||
		pm->ps->legsAnim == BOTH_FORCEJUMP1 ) {
		int fjAnim;
		switch ( pm->ps->legsAnim ) {
		case BOTH_FORCEJUMPLEFT1:
			fjAnim = BOTH_LANDLEFT1;
			break;
		case BOTH_FORCEJUMPRIGHT1:
			fjAnim = BOTH_LANDRIGHT1;
			break;
		case BOTH_FORCEJUMPBACK1:
			fjAnim = BOTH_LANDBACK1;
			break;
		default:
			fjAnim = BOTH_LAND1;
			break;
		}
		PM_SetAnim( SETANIM_BOTH, fjAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
	}
	// decide which landing animation to use
	else if ( !BG_InRoll( pm->ps, pm->ps->legsAnim ) && pm->ps->inAirAnim && !pm->ps->m_iVehicleNum ) { //only play a land animation if we transitioned into an in-air animation while off the ground
		if ( !BG_SaberInSpecial( pm->ps->saberMove ) ) {
			if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
				PM_ForceLegsAnim( BOTH_LANDBACK1 );
			}
			else {
				PM_ForceLegsAnim( BOTH_LAND1 );
			}
		}
	}

	if ( pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE && !PM_IsRocketTrooper() ) { //saber handles its own anims
		//This will push us back into our weaponready stance from the land anim.
		if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) {
			PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
		}
		else {
			if ( pm->ps->weapon == WP_EMPLACED_GUN ) {
				PM_StartTorsoAnim( BOTH_GUNSIT1 );
			}
			else {
				PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
			}
		}
	}

	if ( !BG_InSpecialJump( pm->ps->legsAnim ) ||
		pm->ps->legsTimer < 1 ||
		(pm->ps->legsAnim) == BOTH_WALL_RUN_LEFT ||
		(pm->ps->legsAnim) == BOTH_WALL_RUN_RIGHT ) { //Only set the timer if we're in an anim that can be interrupted (this would not be, say, a flip)
		if ( !BG_InRoll( pm->ps, pm->ps->legsAnim ) && pm->ps->inAirAnim ) {
			if ( !BG_SaberInSpecial( pm->ps->saberMove ) || pm->ps->weapon != WP_SABER ) {
				if ( pm->ps->legsAnim != BOTH_FORCELAND1			&&	pm->ps->legsAnim != BOTH_FORCELANDBACK1 &&
					pm->ps->legsAnim != BOTH_FORCELANDRIGHT1	&&	pm->ps->legsAnim != BOTH_FORCELANDLEFT1 ) { //don't override if we have started a force land
					pm->ps->legsTimer = TIMER_LAND;
				}
			}
		}
	}

	pm->ps->inAirAnim = qfalse;

	if ( pm->ps->m_iVehicleNum ) { //don't do fall stuff while on a vehicle
		return;
	}

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25f;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5f;
	}

	if ( delta < 1 ) {
		return;
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		if ( delta >= 2 && !PM_InOnGroundAnim( pm->ps->legsAnim ) && !PM_InKnockDown( pm->ps ) && !BG_InRoll( pm->ps, pm->ps->legsAnim ) &&
			pm->ps->forceHandExtend == HANDEXTEND_NONE ) {//roll!
			int anim = PM_TryRoll();

			if ( PM_InRollComplete( pm->ps, pm->ps->legsAnim ) ) {
				anim = 0;
				pm->ps->legsTimer = 0;
				pm->ps->legsAnim = 0;
				PM_SetAnim( SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
				pm->ps->legsTimer = TIMER_LAND;
			}

			if ( anim ) {//absorb some impact
				pm->ps->legsTimer = 0;
				delta /= 3; // /= 2 just cancels out the above delta *= 2 when landing while crouched, the roll itself should absorb a little damage
				pm->ps->legsAnim = 0;
				if ( pm->ps->torsoAnim == BOTH_A7_SOULCAL ) { //get out of it on torso
					pm->ps->torsoTimer = 0;
				}
				PM_SetAnim( SETANIM_BOTH, anim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
				didRoll = qtrue;
			}
		}
	}

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) ) {
		if ( delta > 7 ) {
			int delta_send = (int)delta;

			if ( delta_send > 600 ) { //will never need to know any value above this
				delta_send = 600;
			}

			if ( pm->ps->fd.forceJumpZStart ) {
				if ( (int)pm->ps->origin.z >= (int)pm->ps->fd.forceJumpZStart ) { //was force jumping, landed on higher or same level as when force jump was started
					if ( delta_send > 8 ) {
						delta_send = 8;
					}
				}
				else {
					if ( delta_send > 8 ) {
						int dif = ((int)pm->ps->fd.forceJumpZStart - (int)pm->ps->origin.z);
						int dmgLess = (forceJumpHeight[pm->ps->fd.forcePowerLevel[FP_LEVITATION]] - dif);

						if ( dmgLess < 0 ) {
							dmgLess = 0;
						}

						delta_send -= (dmgLess*0.3f);

						if ( delta_send < 8 ) {
							delta_send = 8;
						}

						//Com_Printf("Damage sub: %i\n", (int)((dmgLess*0.1f)));
					}
				}
			}

			if ( didRoll ) { //Add the appropriate event..
				PM_AddEventWithParm( EV_ROLL, delta_send );
			}
			else {
				PM_AddEventWithParm( EV_FALL, delta_send );
			}
		}
		else {
			if ( didRoll ) {
				PM_AddEventWithParm( EV_ROLL, 0 );
			}
			else {
				PM_AddEventWithParm( EV_FOOTSTEP, PM_FootstepForSurface() );
			}
		}
	}

	// make sure velocity resets so we don't bounce back up again in case we miss the clear elsewhere
	pm->ps->velocity.z = 0;

	// start footstep cycle over
	pm->ps->bobCycle = 0;

	if ( pm->overbounce == 2 || (pm->overbounce == 3 && pm->ps->fd.forceJumpZStart > pm->ps->origin.z) ) {
		if ( VectorLength( &pm->ps->velocity ) < 1 )
			pm->ps->velocity.z = -vel;
	}
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vector3		point;

	if ( pm->debugLevel ) {
		Com_Printf( "%i:allsolid\n", c_pmove );
	}

	// jitter around
	for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
			for ( k = -1; k <= 1; k++ ) {
				VectorCopy( &pm->ps->origin, &point );
				point.x += (float)i;
				point.y += (float)j;
				point.z += (float)k;
				pm->trace( trace, &point, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask );
				if ( !trace->allsolid ) {
					point.x = pm->ps->origin.x;
					point.y = pm->ps->origin.y;
					point.z = pm->ps->origin.z - 0.25f;

					pm->trace( trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask );
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vector3		point;

	//rww - don't want to do this when handextend_choke, because you can be standing on the ground
	//while still holding your throat.
	if ( pm->ps->pm_type == PM_FLOAT ) {
		//we're assuming this is because you're being choked
		int parts = SETANIM_LEGS;

		//rww - also don't use SETANIM_FLAG_HOLD, it will cause the legs to float around a bit before going into
		//a proper anim even when on the ground.
		PM_SetAnim( parts, BOTH_CHOKE3, SETANIM_FLAG_OVERRIDE, 100 );
	}
	else if ( pm->ps->pm_type == PM_JETPACK ) {//jetpacking
		//rww - also don't use SETANIM_FLAG_HOLD, it will cause the legs to float around a bit before going into
		//a proper anim even when on the ground.
		//PM_SetAnim(SETANIM_LEGS,BOTH_FORCEJUMP1,SETANIM_FLAG_OVERRIDE, 100);
	}
	//If the anim is choke3, act like we just went into the air because we aren't in a float
	else if ( pm->ps->groundEntityNum != ENTITYNUM_NONE || (pm->ps->legsAnim) == BOTH_CHOKE3 ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( &pm->ps->origin, &point );
		point.z -= 64;

		pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask );
		if ( trace.fraction == 1.0f || pm->ps->pm_type == PM_FLOAT ) {
			if ( pm->ps->velocity.z <= 0 && !(pm->ps->pm_flags&PMF_JUMP_HELD) ) {
				//PM_SetAnim(SETANIM_LEGS,BOTH_INAIR1,SETANIM_FLAG_OVERRIDE, 100);
				PM_SetAnim( SETANIM_LEGS, BOTH_INAIR1, 0, 100 );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else if ( pm->cmd.forwardmove >= 0 ) {
				PM_SetAnim( SETANIM_LEGS, BOTH_JUMP1, SETANIM_FLAG_OVERRIDE, 100 );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else {
				PM_SetAnim( SETANIM_LEGS, BOTH_JUMPBACK1, SETANIM_FLAG_OVERRIDE, 100 );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}

			pm->ps->inAirAnim = qtrue;
		}
	}
	else if ( !pm->ps->inAirAnim ) {
		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( &pm->ps->origin, &point );
		point.z -= 64;

		pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask );
		if ( trace.fraction == 1.0f || pm->ps->pm_type == PM_FLOAT ) {
			pm->ps->inAirAnim = qtrue;
		}
	}

	if ( PM_InRollComplete( pm->ps, pm->ps->legsAnim ) ) { //Client won't catch an animation restart because it only checks frame against incoming frame, so if you roll when you land after rolling
		//off of something it won't replay the roll anim unless we switch it off in the air. This fixes that.
		PM_SetAnim( SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
		pm->ps->inAirAnim = qtrue;
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vector3		point;
	trace_t		trace;
	float minNormal = (float)MIN_WALK_NORMAL;

	if ( pm->ps->clientNum >= MAX_CLIENTS ) {
		bgEntity_t *pEnt = pm_entSelf;

		if ( pEnt && pEnt->s.NPC_class == CLASS_VEHICLE ) {
			minNormal = pEnt->m_pVehicle->m_pVehicleInfo->maxSlope;
		}
	}

	point.x = pm->ps->origin.x;
	point.y = pm->ps->origin.y;
	point.z = pm->ps->origin.z - 0.25f;

	pm->trace( &trace, &pm->ps->origin, &pm->mins, &pm->maxs, &point, pm->ps->clientNum, pm->tracemask );
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid( &trace ) )
			return;
	}

	if ( pm->ps->pm_type == PM_FLOAT || pm->ps->pm_type == PM_JETPACK ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0f ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity.z > 0 && DotProduct( &pm->ps->velocity, &trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:kickoff\n", c_pmove );
		}
		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 ) {
			PM_ForceLegsAnim( BOTH_JUMP1 );
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else {
			PM_ForceLegsAnim( BOTH_JUMPBACK1 );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal.z < minNormal ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:steep\n", c_pmove );
		}
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf( "%i:Land\n", c_pmove );
		}

		PM_CrashLand();

#ifdef _GAME
		if ( pm->ps->clientNum < MAX_CLIENTS &&
			!pm->ps->m_iVehicleNum &&
			trace.entityNum < ENTITYNUM_WORLD &&
			trace.entityNum >= MAX_CLIENTS &&
			!pm->ps->zoomMode &&
			pm_entSelf ) { //check if we landed on a vehicle
			gentity_t *trEnt = &g_entities[trace.entityNum];
			if ( trEnt->inuse && trEnt->client && trEnt->s.eType == ET_NPC && trEnt->s.NPC_class == CLASS_VEHICLE &&
				!trEnt->client->ps.m_iVehicleNum &&
				trEnt->m_pVehicle &&
				trEnt->m_pVehicle->m_pVehicleInfo->type != VH_WALKER &&
				trEnt->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER ) { //it's a vehicle alright, let's board it.. if it's not an atst or ship
				if ( !BG_SaberInSpecial( pm->ps->saberMove ) &&
					pm->ps->forceHandExtend == HANDEXTEND_NONE &&
					pm->ps->weaponTime <= 0 ) {
					gentity_t *servEnt = (gentity_t *)pm_entSelf;
					if ( level.gametype < GT_TEAM ||
						!trEnt->alliedTeam ||
						(trEnt->alliedTeam == servEnt->client->sess.sessionTeam) ) { //not belonging to a team, or client is on same team
						trEnt->m_pVehicle->m_pVehicleInfo->Board( trEnt->m_pVehicle, pm_entSelf );
					}
				}
			}
		}
#endif

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity.z < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;
	pm->ps->lastOnGround = pm->cmd.serverTime;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel
=============
*/
static void PM_SetWaterLevel( void ) {
	vector3		point;
	int			sample1;
	int			sample2;
	int			cont;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	point.x = pm->ps->origin.x;
	point.y = pm->ps->origin.y;
	point.z = pm->ps->origin.z + MINS_Z + 1;
	cont = pm->pointcontents( &point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point.z = pm->ps->origin.z + MINS_Z + sample1;
		cont = pm->pointcontents( &point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point.z = pm->ps->origin.z + MINS_Z + sample2;
			cont = pm->pointcontents( &point, pm->ps->clientNum );
			if ( cont & MASK_WATER )
				pm->waterlevel = 3;
		}
	}

}

qboolean PM_CheckDualForwardJumpDuck( void ) {
	qboolean resized = qfalse;
	if ( pm->ps->legsAnim == BOTH_JUMPATTACK6 ) {
		//dynamically reduce bounding box to let character sail over heads of enemies
		if ( (pm->ps->legsTimer >= 1450
			&& PM_AnimLength( 0, BOTH_JUMPATTACK6 ) - pm->ps->legsTimer >= 400)
			|| (pm->ps->legsTimer >= 400
			&& PM_AnimLength( 0, BOTH_JUMPATTACK6 ) - pm->ps->legsTimer >= 1100) ) {//in a part of the anim that we're pretty much sideways in, raise up the mins
			pm->mins.z = 0;
			pm->ps->pm_flags |= PMF_FIX_MINS;
			resized = qtrue;
		}
	}
	return resized;
}

void PM_CheckFixMins( void ) {
	if ( (pm->ps->pm_flags&PMF_FIX_MINS) )// pm->mins[2] > DEFAULT_MINS_2 )
	{//drop the mins back down
		//do a trace to make sure it's okay
		trace_t	trace;
		vector3 end, curMins, curMaxs;

		VectorSet( &end, pm->ps->origin.x, pm->ps->origin.y, pm->ps->origin.z + MINS_Z );
		VectorSet( &curMins, pm->mins.x, pm->mins.y, 0 );
		VectorSet( &curMaxs, pm->maxs.x, pm->maxs.y, pm->ps->standheight );

		pm->trace( &trace, &pm->ps->origin, &curMins, &curMaxs, &end, pm->ps->clientNum, pm->tracemask );
		if ( !trace.allsolid && !trace.startsolid ) {//should never start in solid
			if ( trace.fraction >= 1.0f ) {//all clear
				//drop the bottom of my bbox back down
				pm->mins.z = MINS_Z;
				pm->ps->pm_flags &= ~PMF_FIX_MINS;
			}
			else {//move me up so the bottom of my bbox will be where the trace ended, at least
				//need to trace up, too
				float updist = ((1.0f - trace.fraction) * -MINS_Z);
				end.z = pm->ps->origin.z + updist;
				pm->trace( &trace, &pm->ps->origin, &curMins, &curMaxs, &end, pm->ps->clientNum, pm->tracemask );
				if ( !trace.allsolid && !trace.startsolid ) {//should never start in solid
					if ( trace.fraction >= 1.0f ) {//all clear
						//move me up
						pm->ps->origin.z += updist;
						//drop the bottom of my bbox back down
						pm->mins.z = MINS_Z;
						pm->ps->pm_flags &= ~PMF_FIX_MINS;
					}
					else {//crap, no room to expand, so just crouch us
						if ( pm->ps->legsAnim != BOTH_JUMPATTACK6
							|| pm->ps->legsTimer <= 200 ) {//at the end of the anim, and we can't leave ourselves like this
							//so drop the maxs, put the mins back and move us up
							pm->maxs.z += MINS_Z;
							pm->ps->origin.z -= MINS_Z;
							pm->mins.z = MINS_Z;
							//this way we'll be in a crouch when we're done
							if ( pm->ps->legsAnim == BOTH_JUMPATTACK6 ) {
								pm->ps->legsTimer = pm->ps->torsoTimer = 0;
							}
							pm->ps->pm_flags |= PMF_DUCKED;
							//FIXME: do we need to set a crouch anim here?
							pm->ps->pm_flags &= ~PMF_FIX_MINS;
						}
					}
				}//crap, stuck
			}
		}//crap, stuck!
	}
}

static qboolean PM_CanStand( void ) {
	qboolean canStand = qtrue;
	float x, y;
	trace_t trace;

	const vector3 lineMins = { -5.0f, -5.0f, -2.5f };
	const vector3 lineMaxs = { 5.0f, 5.0f, 0.0f };

	for ( x = pm->mins.x + 5.0f; canStand && x <= (pm->maxs.x - 5.0f); x += 10.0f ) {
		for ( y = pm->mins.y + 5.0f; y <= (pm->maxs.y - 5.0f); y += 10.0f ) {
			vector3 start, end;
			VectorSet( &start, x, y, pm->maxs.z );
			VectorSet( &end, x, y, pm->ps->standheight );

			VectorAdd( &start, &pm->ps->origin, &start );
			VectorAdd( &end, &pm->ps->origin, &end );

			pm->trace( &trace, &start, &lineMins, &lineMaxs, &end, pm->ps->clientNum, pm->tracemask );
			if ( trace.allsolid || trace.fraction < 1.0f ) {
				canStand = qfalse;
				break;
			}
		}
	}

	return canStand;
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck( void ) {
	if ( pm->ps->m_iVehicleNum > 0 && pm->ps->m_iVehicleNum < ENTITYNUM_NONE ) {//riding a vehicle or are a vehicle
		//no ducking or rolling when on a vehicle
		//right?  not even on ones that you just ride on top of?
		pm->ps->pm_flags &= ~PMF_DUCKED;
		pm->ps->pm_flags &= ~PMF_ROLLING;
		//NOTE: we don't clear the pm->cmd.upmove here because
		//the vehicle code may need it later... but, for riders,
		//it should have already been copied over to the vehicle, right?

		if ( pm->ps->clientNum >= MAX_CLIENTS ) {
			return;
		}
		if ( pm_entVeh && pm_entVeh->m_pVehicle &&
			(pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER ||
			pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL) ) {
			trace_t solidTr;

			pm->mins.x = -16;
			pm->mins.y = -16;
			pm->mins.z = MINS_Z;

			pm->maxs.x = 16;
			pm->maxs.y = 16;
			pm->maxs.z = pm->ps->standheight;//DEFAULT_MAXS_2;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

			pm->trace( &solidTr, &pm->ps->origin, &pm->mins, &pm->maxs, &pm->ps->origin, pm->ps->m_iVehicleNum, pm->tracemask );
			if ( solidTr.startsolid || solidTr.allsolid || solidTr.fraction != 1.0f ) { //whoops, can't fit here. Down to 0!
				VectorClear( &pm->mins );
				VectorClear( &pm->maxs );
#ifdef _GAME
				{
					gentity_t *me = &g_entities[pm->ps->clientNum];
					if ( me->inuse && me->client ) { //yeah, this is a really terrible hack.
						me->client->solidHack = level.time + 200;
					}
				}
#endif
			}
		}
	}
	else {
		if ( pm->ps->clientNum < MAX_CLIENTS ) {
			pm->mins.x = -15;
			pm->mins.y = -15;

			pm->maxs.x = 15;
			pm->maxs.y = 15;
		}

		if ( PM_CheckDualForwardJumpDuck() ) {//special anim resizing us
		}
		else {
			PM_CheckFixMins();

			if ( !pm->mins.z ) {
				pm->mins.z = MINS_Z;
			}
		}

		if ( pm->ps->pm_type == PM_DEAD && pm->ps->clientNum < MAX_CLIENTS ) {
			pm->maxs.z = -8;
			pm->ps->viewheight = DEAD_VIEWHEIGHT;
			return;
		}

		if ( BG_InRoll( pm->ps, pm->ps->legsAnim ) && !BG_KickingAnim( pm->ps->legsAnim ) ) {
			pm->maxs.z = pm->ps->crouchheight; //CROUCH_MAXS_2;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
			pm->ps->pm_flags &= ~PMF_DUCKED;
			pm->ps->pm_flags |= PMF_ROLLING;
			return;
		}
		else if ( pm->ps->pm_flags & PMF_ROLLING ) {
			//Raz: fix for crouchjumping through roof
			if ( PM_CanStand() ) {
				pm->maxs.z = pm->ps->standheight;
				pm->ps->pm_flags &= ~PMF_ROLLING;
			}
		}
		else if ( pm->cmd.upmove < 0 ||
			pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			pm->ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
			pm->ps->forceHandExtend == HANDEXTEND_POSTTHROWN ) {	// duck
			pm->ps->pm_flags |= PMF_DUCKED;
		}
		else {	// stand up if possible
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				//Raz: fix for crouchjumping through roof
				if ( PM_CanStand() ) {
					pm->maxs.z = pm->ps->standheight;
					pm->ps->pm_flags &= ~PMF_DUCKED;
				}
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		pm->maxs.z = pm->ps->crouchheight;//CROUCH_MAXS_2;
		pm->ps->viewheight = CROUCH_VIEWHEIGHT;
	}
	else if ( pm->ps->pm_flags & PMF_ROLLING ) {
		pm->maxs.z = pm->ps->crouchheight;//CROUCH_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
	else {
		pm->maxs.z = pm->ps->standheight;//DEFAULT_MAXS_2;
		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
	}
}



//===================================================================



/*
==============
PM_Use

Generates a use event
==============
*/
#define USE_DELAY 2000

void PM_Use( void ) {
	if ( pm->ps->useTime > 0 )
		pm->ps->useTime -= 100;//pm->cmd.msec;

	if ( pm->ps->useTime > 0 ) {
		return;
	}

	if ( !(pm->cmd.buttons & BUTTON_USE) ) {
		pm->useEvent = 0;
		pm->ps->useTime = 0;
		return;
	}

	pm->useEvent = EV_USE;
	pm->ps->useTime = USE_DELAY;
}

qboolean PM_WalkingAnim( int anim ) {
	switch ( anim ) {
	case BOTH_WALK1:				//# Normal walk
	case BOTH_WALK2:				//# Normal walk with saber
	case BOTH_WALK_STAFF:			//# Normal walk with staff
	case BOTH_WALK_DUAL:			//# Normal walk with staff
	case BOTH_WALK5:				//# Tavion taunting Kyle (cin 22)
	case BOTH_WALK6:				//# Slow walk for Luke (cin 12)
	case BOTH_WALK7:				//# Fast walk
	case BOTH_WALKBACK1:			//# Walk1 backwards
	case BOTH_WALKBACK2:			//# Walk2 backwards
	case BOTH_WALKBACK_STAFF:		//# Walk backwards with staff
	case BOTH_WALKBACK_DUAL:		//# Walk backwards with dual
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_RunningAnim( int anim ) {
	switch ( anim ) {
	case BOTH_RUN1:
	case BOTH_RUN2:
	case BOTH_RUN_STAFF:
	case BOTH_RUN_DUAL:
	case BOTH_RUNBACK1:
	case BOTH_RUNBACK2:
	case BOTH_RUNBACK_STAFF:
	case BOTH_RUNBACK_DUAL:
	case BOTH_RUN1START:			//# Start into full run1
	case BOTH_RUN1STOP:			//# Stop from full run1
	case BOTH_RUNSTRAFE_LEFT1:	//# Sidestep left: should loop
	case BOTH_RUNSTRAFE_RIGHT1:	//# Sidestep right: should loop
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_SwimmingAnim( int anim ) {
	switch ( anim ) {
	case BOTH_SWIM_IDLE1:		//# Swimming Idle 1
	case BOTH_SWIMFORWARD:		//# Swim forward loop
	case BOTH_SWIMBACKWARD:		//# Swim backward loop
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean PM_RollingAnim( int anim ) {
	switch ( anim ) {
	case BOTH_ROLL_F:			//# Roll forward
	case BOTH_ROLL_B:			//# Roll backward
	case BOTH_ROLL_L:			//# Roll left
	case BOTH_ROLL_R:			//# Roll right
		return qtrue;
	default:
		return qfalse;
	}
}

void PM_AnglesForSlope( const float yaw, const vector3 *slope, vector3 *angles ) {
	vector3	nvf, ovf, ovr, new_angles;
	float	pitch, mod, dot;

	VectorSet( angles, 0, yaw, 0 );
	AngleVectors( angles, &ovf, &ovr, NULL );

	vectoangles( slope, &new_angles );
	pitch = new_angles.pitch + 90;
	new_angles.roll = new_angles.pitch = 0;

	AngleVectors( &new_angles, &nvf, NULL, NULL );

	mod = DotProduct( &nvf, &ovr );

	if ( mod < 0 )
		mod = -1;
	else
		mod = 1;

	dot = DotProduct( &nvf, &ovf );

	angles->yaw = 0;
	angles->pitch = dot * pitch;
	angles->roll = ((1 - Q_fabs( dot )) * pitch * mod);
}

void PM_FootSlopeTrace( float *pDiff, float *pInterval ) {
	vector3	footLOrg, footROrg, footLBot, footRBot;
	vector3 footLPoint, footRPoint;
	vector3 footMins, footMaxs;
	vector3 footLSlope, footRSlope;

	trace_t	trace;
	float	diff, interval;

	mdxaBone_t	boltMatrix;
	vector3		G2Angles;

	VectorSet( &G2Angles, 0, pm->ps->viewangles.yaw, 0 );

	interval = 4;//?

	trap->G2API_GetBoltMatrix( pm->ghoul2, 0, pm->g2Bolts_LFoot, &boltMatrix, &G2Angles, &pm->ps->origin, pm->cmd.serverTime, NULL, &pm->modelScale );
	footLPoint.x = boltMatrix.matrix[0][3];
	footLPoint.y = boltMatrix.matrix[1][3];
	footLPoint.z = boltMatrix.matrix[2][3];

	trap->G2API_GetBoltMatrix( pm->ghoul2, 0, pm->g2Bolts_RFoot, &boltMatrix, &G2Angles, &pm->ps->origin, pm->cmd.serverTime, NULL, &pm->modelScale );
	footRPoint.x = boltMatrix.matrix[0][3];
	footRPoint.y = boltMatrix.matrix[1][3];
	footRPoint.z = boltMatrix.matrix[2][3];

	//get these on the cgame and store it, save ourselves a ghoul2 construct skel call
	VectorCopy( &footLPoint, &footLOrg );
	VectorCopy( &footRPoint, &footROrg );

	//step 2: adjust foot tag z height to bottom of bbox+1
	footLOrg.z = pm->ps->origin.z + pm->mins.z + 1;
	footROrg.z = pm->ps->origin.z + pm->mins.z + 1;
	VectorSet( &footLBot, footLOrg.x, footLOrg.y, footLOrg.z - interval * 10 );
	VectorSet( &footRBot, footROrg.x, footROrg.y, footROrg.z - interval * 10 );

	//step 3: trace down from each, find difference
	VectorSet( &footMins, -3, -3, 0 );
	VectorSet( &footMaxs, 3, 3, 1 );

	pm->trace( &trace, &footLOrg, &footMins, &footMaxs, &footLBot, pm->ps->clientNum, pm->tracemask );
	VectorCopy( &trace.endpos, &footLBot );
	VectorCopy( &trace.plane.normal, &footLSlope );

	pm->trace( &trace, &footROrg, &footMins, &footMaxs, &footRBot, pm->ps->clientNum, pm->tracemask );
	VectorCopy( &trace.endpos, &footRBot );
	VectorCopy( &trace.plane.normal, &footRSlope );

	diff = footLBot.z - footRBot.z;

	if ( pDiff != NULL ) {
		*pDiff = diff;
	}
	if ( pInterval != NULL ) {
		*pInterval = interval;
	}
}

qboolean BG_InSlopeAnim( int anim ) {
	switch ( anim ) {
	case LEGS_LEFTUP1:			//# On a slope with left foot 4 higher than right
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP1:			//# On a slope with RIGHT foot 4 higher than left
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP1:
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP1:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP1:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP1:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP1:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP1:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP1:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP1:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		return qtrue;
	default:
		return qfalse;
	}
}

#define	SLOPE_RECALC_INT 100

qboolean PM_AdjustStandAnimForSlope( void ) {
	float	diff;
	float	interval;
	int		destAnim;
	int		legsAnim;
#define SLOPERECALCVAR pm->ps->slopeRecalcTime //this is purely convenience

	if ( !pm->ghoul2 ) { //probably just changed models and not quite in sync yet
		return qfalse;
	}

	if ( pm->g2Bolts_LFoot == -1 || pm->g2Bolts_RFoot == -1 ) {//need these bolts!
		return qfalse;
	}

	//step 1: find the 2 foot tags
	PM_FootSlopeTrace( &diff, &interval );

	//step 4: based on difference, choose one of the left/right slope-match intervals
	if ( diff >= interval * 5 ) {
		destAnim = LEGS_LEFTUP5;
	}
	else if ( diff >= interval * 4 ) {
		destAnim = LEGS_LEFTUP4;
	}
	else if ( diff >= interval * 3 ) {
		destAnim = LEGS_LEFTUP3;
	}
	else if ( diff >= interval * 2 ) {
		destAnim = LEGS_LEFTUP2;
	}
	else if ( diff >= interval ) {
		destAnim = LEGS_LEFTUP1;
	}
	else if ( diff <= interval*-5 ) {
		destAnim = LEGS_RIGHTUP5;
	}
	else if ( diff <= interval*-4 ) {
		destAnim = LEGS_RIGHTUP4;
	}
	else if ( diff <= interval*-3 ) {
		destAnim = LEGS_RIGHTUP3;
	}
	else if ( diff <= interval*-2 ) {
		destAnim = LEGS_RIGHTUP2;
	}
	else if ( diff <= interval*-1 ) {
		destAnim = LEGS_RIGHTUP1;
	}
	else {
		return qfalse;
	}

	legsAnim = pm->ps->legsAnim;
	//adjust for current legs anim
	switch ( legsAnim ) {
	case BOTH_STAND1:

	case LEGS_S1_LUP1:
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP1:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
		destAnim = LEGS_S1_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND2:
	case BOTH_SABERFAST_STANCE:
	case BOTH_SABERSLOW_STANCE:
	case BOTH_CROUCH1IDLE:
	case BOTH_CROUCH1:
	case LEGS_LEFTUP1:			//# On a slope with left foot 4 higher than right
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP1:			//# On a slope with RIGHT foot 4 higher than left
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
		//fine
		break;
	case BOTH_STAND3:
	case LEGS_S3_LUP1:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP1:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
		destAnim = LEGS_S3_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND4:
	case LEGS_S4_LUP1:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP1:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
		destAnim = LEGS_S4_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND5:
	case LEGS_S5_LUP1:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP1:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		destAnim = LEGS_S5_LUP1 + (destAnim - LEGS_LEFTUP1);
		break;
	case BOTH_STAND6:
	default:
		return qfalse;
	}

	//step 5: based on the chosen interval and the current legsAnim, pick the correct anim
	//step 6: increment/decrement to the dest anim, not instant
	if ( (legsAnim >= LEGS_LEFTUP1 && legsAnim <= LEGS_LEFTUP5)
		|| (legsAnim >= LEGS_S1_LUP1 && legsAnim <= LEGS_S1_LUP5)
		|| (legsAnim >= LEGS_S3_LUP1 && legsAnim <= LEGS_S3_LUP5)
		|| (legsAnim >= LEGS_S4_LUP1 && legsAnim <= LEGS_S4_LUP5)
		|| (legsAnim >= LEGS_S5_LUP1 && legsAnim <= LEGS_S5_LUP5) ) {//already in left-side up
		if ( destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime ) {
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if ( destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime ) {
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else //if (SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else if ( (legsAnim >= LEGS_RIGHTUP1 && legsAnim <= LEGS_RIGHTUP5)
		|| (legsAnim >= LEGS_S1_RUP1 && legsAnim <= LEGS_S1_RUP5)
		|| (legsAnim >= LEGS_S3_RUP1 && legsAnim <= LEGS_S3_RUP5)
		|| (legsAnim >= LEGS_S4_RUP1 && legsAnim <= LEGS_S4_RUP5)
		|| (legsAnim >= LEGS_S5_RUP1 && legsAnim <= LEGS_S5_RUP5) ) {//already in right-side up
		if ( destAnim > legsAnim && SLOPERECALCVAR < pm->cmd.serverTime ) {
			legsAnim++;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else if ( destAnim < legsAnim && SLOPERECALCVAR < pm->cmd.serverTime ) {
			legsAnim--;
			SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
		}
		else //if (SLOPERECALCVAR < pm->cmd.serverTime)
		{
			legsAnim = destAnim;
		}

		destAnim = legsAnim;
	}
	else {//in a stand of some sort?
		switch ( legsAnim ) {
		case BOTH_STAND1:
		case TORSO_WEAPONREADY1:
		case TORSO_WEAPONREADY2:
		case TORSO_WEAPONREADY3:
		case TORSO_WEAPONREADY10:

			if ( destAnim >= LEGS_S1_LUP1 && destAnim <= LEGS_S1_LUP5 ) {//going into left side up
				destAnim = LEGS_S1_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if ( destAnim >= LEGS_S1_RUP1 && destAnim <= LEGS_S1_RUP5 ) {//going into right side up
				destAnim = LEGS_S1_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else {//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND2:
		case BOTH_SABERFAST_STANCE:
		case BOTH_SABERSLOW_STANCE:
		case BOTH_CROUCH1IDLE:
			if ( destAnim >= LEGS_LEFTUP1 && destAnim <= LEGS_LEFTUP5 ) {//going into left side up
				destAnim = LEGS_LEFTUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if ( destAnim >= LEGS_RIGHTUP1 && destAnim <= LEGS_RIGHTUP5 ) {//going into right side up
				destAnim = LEGS_RIGHTUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else {//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND3:
			if ( destAnim >= LEGS_S3_LUP1 && destAnim <= LEGS_S3_LUP5 ) {//going into left side up
				destAnim = LEGS_S3_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if ( destAnim >= LEGS_S3_RUP1 && destAnim <= LEGS_S3_RUP5 ) {//going into right side up
				destAnim = LEGS_S3_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else {//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND4:
			if ( destAnim >= LEGS_S4_LUP1 && destAnim <= LEGS_S4_LUP5 ) {//going into left side up
				destAnim = LEGS_S4_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if ( destAnim >= LEGS_S4_RUP1 && destAnim <= LEGS_S4_RUP5 ) {//going into right side up
				destAnim = LEGS_S4_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else {//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND5:
			if ( destAnim >= LEGS_S5_LUP1 && destAnim <= LEGS_S5_LUP5 ) {//going into left side up
				destAnim = LEGS_S5_LUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else if ( destAnim >= LEGS_S5_RUP1 && destAnim <= LEGS_S5_RUP5 ) {//going into right side up
				destAnim = LEGS_S5_RUP1;
				SLOPERECALCVAR = pm->cmd.serverTime + SLOPE_RECALC_INT;
			}
			else {//will never get here
				return qfalse;
			}
			break;
		case BOTH_STAND6:
		default:
			return qfalse;
		}
	}
	//step 7: set the anim
	//PM_SetAnim( SETANIM_LEGS, destAnim, SETANIM_FLAG_NORMAL, 100 );
	PM_ContinueLegsAnim( destAnim );

	return qtrue;
}

extern int WeaponReadyLegsAnim[WP_NUM_WEAPONS];

//rww - slowly back out of slope leg anims, to prevent skipping between slope anims and general jittering
int PM_LegsSlopeBackTransition( int desiredAnim ) {
	int anim = pm->ps->legsAnim;

	switch ( anim ) {
	case LEGS_LEFTUP2:			//# On a slope with left foot 8 higher than right
	case LEGS_LEFTUP3:			//# On a slope with left foot 12 higher than right
	case LEGS_LEFTUP4:			//# On a slope with left foot 16 higher than right
	case LEGS_LEFTUP5:			//# On a slope with left foot 20 higher than right
	case LEGS_RIGHTUP2:			//# On a slope with RIGHT foot 8 higher than left
	case LEGS_RIGHTUP3:			//# On a slope with RIGHT foot 12 higher than left
	case LEGS_RIGHTUP4:			//# On a slope with RIGHT foot 16 higher than left
	case LEGS_RIGHTUP5:			//# On a slope with RIGHT foot 20 higher than left
	case LEGS_S1_LUP2:
	case LEGS_S1_LUP3:
	case LEGS_S1_LUP4:
	case LEGS_S1_LUP5:
	case LEGS_S1_RUP2:
	case LEGS_S1_RUP3:
	case LEGS_S1_RUP4:
	case LEGS_S1_RUP5:
	case LEGS_S3_LUP2:
	case LEGS_S3_LUP3:
	case LEGS_S3_LUP4:
	case LEGS_S3_LUP5:
	case LEGS_S3_RUP2:
	case LEGS_S3_RUP3:
	case LEGS_S3_RUP4:
	case LEGS_S3_RUP5:
	case LEGS_S4_LUP2:
	case LEGS_S4_LUP3:
	case LEGS_S4_LUP4:
	case LEGS_S4_LUP5:
	case LEGS_S4_RUP2:
	case LEGS_S4_RUP3:
	case LEGS_S4_RUP4:
	case LEGS_S4_RUP5:
	case LEGS_S5_LUP2:
	case LEGS_S5_LUP3:
	case LEGS_S5_LUP4:
	case LEGS_S5_LUP5:
	case LEGS_S5_RUP2:
	case LEGS_S5_RUP3:
	case LEGS_S5_RUP4:
	case LEGS_S5_RUP5:
		VectorClear( &pm->ps->velocity );
		if ( pm->ps->slopeRecalcTime < pm->cmd.serverTime ) {
			pm->ps->slopeRecalcTime = pm->cmd.serverTime + 8;//SLOPE_RECALC_INT;
			return anim - 1;
		}
		else
			return anim;

	default:
		return desiredAnim;
	}
}

static int JP_GetJPFixRoll( void ) {
	int level = 0;
#ifdef _GAME
	uint32_t cinfo = jp_cinfo.integer;
#else
	uint32_t cinfo = cgs.japp.jp_cinfo;
#endif

	if ( cinfo & CINFO_JK2ROLL1 )
		level++;
	if ( cinfo & CINFO_JK2ROLL2 )
		level++;
	if ( cinfo & CINFO_JK2ROLL3 )
		level++;

	return level;
}

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	uint32_t			setAnimFlags = 0;

	if ( (PM_InSaberAnim( (pm->ps->legsAnim) ) && !BG_SpinningSaberAnim( (pm->ps->legsAnim) ))
		|| (pm->ps->legsAnim) == BOTH_STAND1
		|| (pm->ps->legsAnim) == BOTH_STAND1TO2
		|| (pm->ps->legsAnim) == BOTH_STAND2TO1
		|| (pm->ps->legsAnim) == BOTH_STAND2
		|| (pm->ps->legsAnim) == BOTH_SABERFAST_STANCE
		|| (pm->ps->legsAnim) == BOTH_SABERSLOW_STANCE
		|| (pm->ps->legsAnim) == BOTH_BUTTON_HOLD
		|| (pm->ps->legsAnim) == BOTH_BUTTON_RELEASE
		|| PM_LandingAnim( (pm->ps->legsAnim) )
		|| PM_PainAnim( (pm->ps->legsAnim) ) ) {//legs are in a saber anim, and not spinning, be sure to override it
		setAnimFlags |= SETANIM_FLAG_OVERRIDE;
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrtf( pm->ps->velocity.x*pm->ps->velocity.x + pm->ps->velocity.y*pm->ps->velocity.y );

	if ( pm->ps->saberMove == LS_SPINATTACK ) {
		PM_ContinueLegsAnim( pm->ps->torsoAnim );
	}
	else if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			if ( pm->xyspeed > 60 ) {
				PM_ContinueLegsAnim( BOTH_SWIMFORWARD );
			}
			else {
				PM_ContinueLegsAnim( BOTH_SWIM_IDLE1 );
			}
		}
		return;
	}
	// if not trying to move
	else if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if ( pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_RANCOR ) {
				if ( (pm->ps->eFlags2&EF2_USE_ALT_ANIM) ) {//holding someone
					PM_ContinueLegsAnim( BOTH_STAND4 );
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_STAND4,SETANIM_FLAG_NORMAL);
				}
				else if ( (pm->ps->eFlags2&EF2_ALERTED) ) {//have an enemy or have had one since we spawned
					PM_ContinueLegsAnim( BOTH_STAND2 );
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_STAND2,SETANIM_FLAG_NORMAL);
				}
				else {//just stand there
					PM_ContinueLegsAnim( BOTH_STAND1 );
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_STAND1,SETANIM_FLAG_NORMAL);
				}
			}
			else if ( pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_WAMPA ) {
				if ( (pm->ps->eFlags2&EF2_USE_ALT_ANIM) ) {//holding a victim
					PM_ContinueLegsAnim( BOTH_STAND2 );
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_STAND2,SETANIM_FLAG_NORMAL);
				}
				else {//not holding a victim
					PM_ContinueLegsAnim( BOTH_STAND1 );
					//PM_SetAnim(pm,SETANIM_LEGS,BOTH_STAND1,SETANIM_FLAG_NORMAL);
				}
			}
			else if ( (pm->ps->pm_flags & PMF_DUCKED) || (pm->ps->pm_flags & PMF_ROLLING) ) {
				if ( (pm->ps->legsAnim) != BOTH_CROUCH1IDLE ) {
					PM_SetAnim( SETANIM_LEGS, BOTH_CROUCH1IDLE, setAnimFlags, 100 );
				}
				else {
					PM_ContinueLegsAnim( BOTH_CROUCH1IDLE );
				}
			}
			else {
				if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) {
					///????  continue legs anim on a torso anim...??!!!
					//yeah.. the anim has a valid pose for the legs, it uses it (you can't move while using disruptor)
					PM_ContinueLegsAnim( TORSO_WEAPONREADY4 );
				}
				else {
					if ( pm->ps->weapon == WP_SABER && BG_SabersOff( pm->ps ) ) {
						if ( !PM_AdjustStandAnimForSlope() ) {
							//	PM_ContinueLegsAnim( BOTH_STAND1 );
							PM_ContinueLegsAnim( PM_LegsSlopeBackTransition( BOTH_STAND1 ) );

						}
					}
					else {
						if ( pm->ps->weapon != WP_SABER || !PM_AdjustStandAnimForSlope() ) {
							if ( pm->ps->weapon == WP_SABER ) {
								PM_ContinueLegsAnim( PM_LegsSlopeBackTransition( PM_GetSaberStance() ) );
							}
							else {
								PM_ContinueLegsAnim( PM_LegsSlopeBackTransition( WeaponReadyLegsAnim[pm->ps->weapon] ) );
							}
						}
					}
				}
			}
		}
		return;
	}


	if ( pm->ps->saberMove == LS_SPINATTACK ) {
		bobmove = 0.2f;
		PM_ContinueLegsAnim( pm->ps->torsoAnim );
	}
	else if ( pm->ps->pm_flags & PMF_DUCKED ) {
		int rolled = 0;
		qboolean canRoll = qfalse;
		bobmove = 0.5f;	// ducked characters bob much faster

#if 0
		if ( ( (PM_RunningAnim( pm->ps->legsAnim ) && VectorLengthSquared( &pm->ps->velocity ) >= 40000/*200*200*/)
			|| PM_CanRollFromSoulCal( pm->ps ) )
			&& !BG_InRoll(pm->ps, pm->ps->legsAnim) )
#else
		if ( PM_RunningAnim( pm->ps->legsAnim ) ) {
			if ( ((JP_GetJPFixRoll() > 2) || VectorLengthSquared( &pm->ps->velocity ) >= 30000 || (JP_GetJPFixRoll() > 1 && !(pm->cmd.buttons & BUTTON_FORCEGRIP))) ) {
				if ( !BG_InRoll( pm->ps, pm->ps->legsAnim ) ) {
					canRoll = qtrue;
				}
			}
		}
		//		if ( ( ((PM_RunningAnim( pm->ps->legsAnim ) && ((JP_GetJPFixRoll() >= 2) || (JP_GetJPFixRoll() >= 1 && !(pm->cmd.buttons & BUTTON_FORCEGRIP)) )) && VectorLengthSquared( pm->ps->velocity ) >= 30000/*200*200*/)
		//				|| PM_CanRollFromSoulCal( pm->ps ) )
		//			 && !BG_InRoll(pm->ps, pm->ps->legsAnim) )
		if ( canRoll )
#endif
		{//roll!
			rolled = PM_TryRoll();
		}
		if ( !rolled ) { //if the roll failed or didn't attempt, do standard crouching anim stuff.
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				if ( (pm->ps->legsAnim) != BOTH_CROUCH1WALKBACK ) {
					PM_SetAnim( SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags, 100 );
				}
				else {
					PM_ContinueLegsAnim( BOTH_CROUCH1WALKBACK );
				}
			}
			else {
				if ( (pm->ps->legsAnim) != BOTH_CROUCH1WALK ) {
					PM_SetAnim( SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags, 100 );
				}
				else {
					PM_ContinueLegsAnim( BOTH_CROUCH1WALK );
				}
			}
		}
		else { //otherwise send us into the roll
			pm->ps->legsTimer = 0;
			pm->ps->legsAnim = 0;
			PM_SetAnim( SETANIM_BOTH, rolled, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 150 );
			PM_AddEventWithParm( EV_ROLL, 0 );
			pm->maxs.z = pm->ps->crouchheight;//CROUCH_MAXS_2;
			pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
			pm->ps->pm_flags &= ~PMF_DUCKED;
			pm->ps->pm_flags |= PMF_ROLLING;
		}
	}
	else if ( (pm->ps->pm_flags & PMF_ROLLING) && !BG_InRoll( pm->ps, pm->ps->legsAnim ) &&
		!PM_InRollComplete( pm->ps, pm->ps->legsAnim ) ) {
		bobmove = 0.5f;	// ducked characters bob much faster

		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			if ( (pm->ps->legsAnim) != BOTH_CROUCH1WALKBACK ) {
				PM_SetAnim( SETANIM_LEGS, BOTH_CROUCH1WALKBACK, setAnimFlags, 100 );
			}
			else {
				PM_ContinueLegsAnim( BOTH_CROUCH1WALKBACK );
			}
		}
		else {
			if ( (pm->ps->legsAnim) != BOTH_CROUCH1WALK ) {
				PM_SetAnim( SETANIM_LEGS, BOTH_CROUCH1WALK, setAnimFlags, 100 );
			}
			else {
				PM_ContinueLegsAnim( BOTH_CROUCH1WALK );
			}
		}
	}
	else {
		int desiredAnim = -1;

		if ( (pm->ps->legsAnim == BOTH_FORCELAND1 ||
			pm->ps->legsAnim == BOTH_FORCELANDBACK1 ||
			pm->ps->legsAnim == BOTH_FORCELANDRIGHT1 ||
			pm->ps->legsAnim == BOTH_FORCELANDLEFT1) &&
			pm->ps->legsTimer > 0 ) { //let it finish first
			bobmove = 0.2f;
		}
		else if ( !(pm->cmd.buttons & BUTTON_WALKING) ) {//running
			bobmove = 0.4f;	// faster speeds bob faster
			if ( pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_WAMPA ) {
				if ( (pm->ps->eFlags2&EF2_USE_ALT_ANIM) ) {//full on run, on all fours
					desiredAnim = BOTH_RUN1;
				}
				else {//regular, upright run
					desiredAnim = BOTH_RUN2;
				}
			}
			else if ( pm->ps->clientNum >= MAX_CLIENTS &&
				pm_entSelf &&
				pm_entSelf->s.NPC_class == CLASS_RANCOR ) {//no run anims
				if ( (pm->ps->pm_flags&PMF_BACKWARDS_RUN) ) {
					desiredAnim = BOTH_WALKBACK1;
				}
				else {
					desiredAnim = BOTH_WALK1;
				}
			}
			else if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				switch ( pm->ps->fd.saberAnimLevel ) {
				case SS_STAFF:
					if ( pm->ps->saberHolstered > 1 ) {//saber off
						desiredAnim = BOTH_RUNBACK1;
					}
					else {
						//desiredAnim = BOTH_RUNBACK_STAFF;
						//hmm.. stuff runback anim is pretty messed up for some reason.
						desiredAnim = BOTH_RUNBACK2;
					}
					break;
				case SS_DUAL:
					if ( pm->ps->saberHolstered > 1 ) {//sabers off
						desiredAnim = BOTH_RUNBACK1;
					}
					else {
						//desiredAnim = BOTH_RUNBACK_DUAL;
						//and so is the dual
						desiredAnim = BOTH_RUNBACK2;
					}
					break;
				default:
					if ( pm->ps->saberHolstered ) {//saber off
						desiredAnim = BOTH_RUNBACK1;
					}
					else {
						desiredAnim = BOTH_RUNBACK2;
					}
					break;
				}
			}
			else {
				switch ( pm->ps->fd.saberAnimLevel ) {
				case SS_STAFF:
					if ( pm->ps->saberHolstered > 1 ) {//blades off
						desiredAnim = BOTH_RUN1;
					}
					else if ( pm->ps->saberHolstered == 1 ) {//1 blade on
						desiredAnim = BOTH_RUN2;
					}
					else {
						if ( pm->ps->fd.forcePowersActive & (1 << FP_SPEED) ) {
							desiredAnim = BOTH_RUN1;
						}
						else {
							desiredAnim = BOTH_RUN_STAFF;
						}
					}
					break;
				case SS_DUAL:
					if ( pm->ps->saberHolstered > 1 ) {//blades off
						desiredAnim = BOTH_RUN1;
					}
					else if ( pm->ps->saberHolstered == 1 ) {//1 saber on
						desiredAnim = BOTH_RUN2;
					}
					else {
						desiredAnim = BOTH_RUN_DUAL;
					}
					break;
				default:
					if ( pm->ps->saberHolstered ) {//saber off
						desiredAnim = BOTH_RUN1;
					}
					else {
						desiredAnim = BOTH_RUN2;
					}
					break;
				}
			}
		}
		else {
			bobmove = 0.2f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				switch ( pm->ps->fd.saberAnimLevel ) {
				case SS_STAFF:
					if ( pm->ps->saberHolstered > 1 ) {
						desiredAnim = BOTH_WALKBACK1;
					}
					else if ( pm->ps->saberHolstered ) {
						desiredAnim = BOTH_WALKBACK2;
					}
					else {
						desiredAnim = BOTH_WALKBACK_STAFF;
					}
					break;
				case SS_DUAL:
					if ( pm->ps->saberHolstered > 1 ) {
						desiredAnim = BOTH_WALKBACK1;
					}
					else if ( pm->ps->saberHolstered ) {
						desiredAnim = BOTH_WALKBACK2;
					}
					else {
						desiredAnim = BOTH_WALKBACK_DUAL;
					}
					break;
				default:
					if ( pm->ps->saberHolstered ) {
						desiredAnim = BOTH_WALKBACK1;
					}
					else {
						desiredAnim = BOTH_WALKBACK2;
					}
					break;
				}
			}
			else {
				if ( pm->ps->weapon == WP_MELEE ) {
					desiredAnim = BOTH_WALK1;
				}
				else if ( BG_SabersOff( pm->ps ) ) {
					desiredAnim = BOTH_WALK1;
				}
				else {
					switch ( pm->ps->fd.saberAnimLevel ) {
					case SS_STAFF:
						if ( pm->ps->saberHolstered > 1 ) {
							desiredAnim = BOTH_WALK1;
						}
						else if ( pm->ps->saberHolstered ) {
							desiredAnim = BOTH_WALK2;
						}
						else {
							desiredAnim = BOTH_WALK_STAFF;
						}
						break;
					case SS_DUAL:
						if ( pm->ps->saberHolstered > 1 ) {
							desiredAnim = BOTH_WALK1;
						}
						else if ( pm->ps->saberHolstered ) {
							desiredAnim = BOTH_WALK2;
						}
						else {
							desiredAnim = BOTH_WALK_DUAL;
						}
						break;
					default:
						if ( pm->ps->saberHolstered ) {
							desiredAnim = BOTH_WALK1;
						}
						else {
							desiredAnim = BOTH_WALK2;
						}
						break;
					}
				}
			}
		}

		if ( desiredAnim != -1 ) {
			int ires = PM_LegsSlopeBackTransition( desiredAnim );

			if ( (pm->ps->legsAnim) != desiredAnim && ires == desiredAnim ) {
				PM_SetAnim( SETANIM_LEGS, desiredAnim, setAnimFlags, 100 );
			}
			else {
				PM_ContinueLegsAnim( ires );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ( ((old + 64) ^ (pm->ps->bobCycle + 64)) & 128 ) {
		pm->ps->footstepTime = pm->cmd.serverTime + 300;
		if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		}
		else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		}
		else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
#ifdef _GAME
	qboolean impact_splash = qfalse;
#endif
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel ) {
#ifdef _GAME
		if ( VectorLengthSquared( &pm->ps->velocity ) > 40000 )
			impact_splash = qtrue;
#endif
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel ) {
#ifdef _GAME
		if ( VectorLengthSquared( &pm->ps->velocity ) > 40000 )
			impact_splash = qtrue;
#endif
		PM_AddEvent( EV_WATER_LEAVE );
	}

#ifdef _GAME
	if ( impact_splash ) {
		//play the splash effect
		trace_t	tr;
		vector3	start, end;


		VectorCopy( &pm->ps->origin, &start );
		VectorCopy( &pm->ps->origin, &end );

		// FIXME: set start and end better
		start.z += 10;
		end.z -= 40;

		pm->trace( &tr, &start, &vec3_origin, &vec3_origin, &end, pm->ps->clientNum, MASK_WATER );

		if ( tr.fraction < 1.0f ) {
			if ( (tr.contents&CONTENTS_LAVA) )		G_PlayEffect( EFFECT_LAVA_SPLASH, &tr.endpos, &tr.plane.normal );
			else if ( (tr.contents&CONTENTS_SLIME) )	G_PlayEffect( EFFECT_ACID_SPLASH, &tr.endpos, &tr.plane.normal );
			else										G_PlayEffect( EFFECT_WATER_SPLASH, &tr.endpos, &tr.plane.normal );
		}
	}
#endif

	//
	// check for head just going under water
	//
	if ( pml.previous_waterlevel != 3 && pm->waterlevel == 3 ) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( pml.previous_waterlevel == 3 && pm->waterlevel != 3 ) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}

void BG_ClearRocketLock( playerState_t *ps ) {
	if ( ps ) {
		ps->rocketLockIndex = ENTITYNUM_NONE;
		ps->rocketLastValidTime = 0;
		ps->rocketLockTime = -1;
		ps->rocketTargetTime = 0;
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	// turn of any kind of zooming when weapon switching.
	if ( pm->ps->zoomMode ) {
		pm->ps->zoomMode = 0;
		pm->ps->zoomTime = pm->ps->commandTime;
	}

	PM_AddEventWithParm( EV_CHANGE_WEAPON, weapon );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	//PM_StartTorsoAnim( TORSO_DROPWEAP1 );
	PM_SetAnim( SETANIM_TORSO, TORSO_DROPWEAP1, SETANIM_FLAG_OVERRIDE, 0 );

	BG_ClearRocketLock( pm->ps );
}


/*
===============
PM_FinishWeaponChange
===============
*/
void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !(pm->ps->stats[STAT_WEAPONS] & (1 << weapon)) ) {
		weapon = WP_NONE;
	}

	if ( weapon == WP_SABER ) {
		PM_SetSaberMove( LS_DRAW );
	}
	else {
		//PM_StartTorsoAnim( TORSO_RAISEWEAP1);
		PM_SetAnim( SETANIM_TORSO, TORSO_RAISEWEAP1, SETANIM_FLAG_OVERRIDE, 0 );
	}
	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;
}

#ifdef _GAME
extern void WP_GetVehicleCamPos( gentity_t *ent, gentity_t *pilot, vector3 *camPos );
#else
extern void CG_GetVehicleCamPos( vector3 *camPos );
#endif
#define MAX_XHAIR_DIST_ACCURACY	20000.0f
int BG_VehTraceFromCamPos( trace_t *camTrace, bgEntity_t *bgEnt, const vector3 *entOrg, const vector3 *shotStart, const vector3 *end, vector3 *newEnd, vector3 *shotDir, float bestDist ) {
	//NOTE: this MUST stay up to date with the method used in CG_ScanForCrosshairEntity (where it checks the doExtraVehTraceFromViewPos bool)
	vector3	viewDir2End, extraEnd, camPos;
	float	minAutoAimDist;

#ifdef _GAME
	WP_GetVehicleCamPos( (gentity_t *)bgEnt, (gentity_t *)bgEnt->m_pVehicle->m_pPilot, &camPos );
#else
	CG_GetVehicleCamPos( &camPos );
#endif

	minAutoAimDist = Distance( entOrg, &camPos ) + (bgEnt->m_pVehicle->m_pVehicleInfo->length / 2.0f) + 200.0f;

	VectorCopy( end, newEnd );
	VectorSubtract( end, &camPos, &viewDir2End );
	VectorNormalize( &viewDir2End );
	VectorMA( &camPos, MAX_XHAIR_DIST_ACCURACY, &viewDir2End, &extraEnd );

	pm->trace( camTrace, &camPos, &vec3_origin, &vec3_origin, &extraEnd, bgEnt->s.number, CONTENTS_SOLID | CONTENTS_BODY );

	if ( !camTrace->allsolid
		&& !camTrace->startsolid
		&& camTrace->fraction < 1.0f
		&& (camTrace->fraction*MAX_XHAIR_DIST_ACCURACY) > minAutoAimDist
		&& ((camTrace->fraction*MAX_XHAIR_DIST_ACCURACY) - Distance( entOrg, &camPos )) < bestDist ) {//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
		VectorCopy( &camTrace->endpos, newEnd );
		VectorSubtract( newEnd, shotStart, shotDir );
		VectorNormalize( shotDir );
		return (camTrace->entityNum + 1);
	}
	return 0;
}

void PM_RocketLock( float lockDist, qboolean vehicleLock ) {
	// Not really a charge weapon, but we still want to delay fire until the button comes up so that we can
	//	implement our alt-fire locking stuff
	vector3		ang;
	trace_t		tr;

	vector3 muzzleOffPoint, muzzlePoint, forward, right, up;

	if ( vehicleLock ) {
		AngleVectors( &pm->ps->viewangles, &forward, &right, &up );
		VectorCopy( &pm->ps->origin, &muzzlePoint );
		VectorMA( &muzzlePoint, lockDist, &forward, &ang );
	}
	else {
		AngleVectors( &pm->ps->viewangles, &forward, &right, &up );

		AngleVectors( &pm->ps->viewangles, &ang, NULL, NULL );

		VectorCopy( &pm->ps->origin, &muzzlePoint );
		VectorCopy( &WP_MuzzlePoint[WP_ROCKET_LAUNCHER], &muzzleOffPoint );

		VectorMA( &muzzlePoint, muzzleOffPoint.x, &forward, &muzzlePoint );
		VectorMA( &muzzlePoint, muzzleOffPoint.y, &right, &muzzlePoint );
		muzzlePoint.z += pm->ps->viewheight + muzzleOffPoint.z;
		ang.x = muzzlePoint.x + ang.x*lockDist;
		ang.y = muzzlePoint.y + ang.y*lockDist;
		ang.z = muzzlePoint.z + ang.z*lockDist;
	}


	pm->trace( &tr, &muzzlePoint, NULL, NULL, &ang, pm->ps->clientNum, MASK_PLAYERSOLID );

	if ( vehicleLock ) {//vehicles also do a trace from the camera point if the main one misses
		if ( tr.fraction >= 1.0f ) {
			trace_t camTrace;
			vector3 newEnd, shotDir;
			if ( BG_VehTraceFromCamPos( &camTrace, PM_BGEntForNum( pm->ps->clientNum ), &pm->ps->origin, &muzzlePoint, &tr.endpos, &newEnd, &shotDir, (tr.fraction*lockDist) ) ) {
				memcpy( &tr, &camTrace, sizeof(tr) );
			}
		}
	}

	if ( tr.fraction != 1 && tr.entityNum < ENTITYNUM_NONE && tr.entityNum != pm->ps->clientNum ) {
		bgEntity_t *bgEnt = PM_BGEntForNum( tr.entityNum );
		if ( bgEnt && (bgEnt->s.powerups&PW_CLOAKED) ) {
			pm->ps->rocketLockIndex = ENTITYNUM_NONE;
			pm->ps->rocketLockTime = 0;
		}
		else if ( bgEnt && (bgEnt->s.eType == ET_PLAYER || bgEnt->s.eType == ET_NPC) ) {
			if ( pm->ps->rocketLockIndex == ENTITYNUM_NONE ) {
				pm->ps->rocketLockIndex = tr.entityNum;
				pm->ps->rocketLockTime = pm->cmd.serverTime;
			}
			else if ( pm->ps->rocketLockIndex != tr.entityNum && pm->ps->rocketTargetTime < pm->cmd.serverTime ) {
				pm->ps->rocketLockIndex = tr.entityNum;
				pm->ps->rocketLockTime = pm->cmd.serverTime;
			}
			else if ( pm->ps->rocketLockIndex == tr.entityNum ) {
				if ( pm->ps->rocketLockTime == -1 ) {
					pm->ps->rocketLockTime = pm->ps->rocketLastValidTime;
				}
			}

			if ( pm->ps->rocketLockIndex == tr.entityNum ) {
				pm->ps->rocketTargetTime = pm->cmd.serverTime + 500;
			}
		}
		else if ( !vehicleLock ) {
			if ( pm->ps->rocketTargetTime < pm->cmd.serverTime ) {
				pm->ps->rocketLockIndex = ENTITYNUM_NONE;
				pm->ps->rocketLockTime = 0;
			}
		}
	}
	else if ( pm->ps->rocketTargetTime < pm->cmd.serverTime ) {
		pm->ps->rocketLockIndex = ENTITYNUM_NONE;
		pm->ps->rocketLockTime = 0;
	}
	else {
		if ( pm->ps->rocketLockTime != -1 ) {
			pm->ps->rocketLastValidTime = pm->ps->rocketLockTime;
		}
		pm->ps->rocketLockTime = -1;
	}
}

//---------------------------------------
static qboolean PM_DoChargedWeapons( qboolean vehicleRocketLock, bgEntity_t *veh )
//---------------------------------------
{
	qboolean	charging = qfalse,
		altFire = qfalse;

	if ( vehicleRocketLock ) {
		if ( (pm->cmd.buttons&(BUTTON_ATTACK | BUTTON_ALT_ATTACK)) ) {//actually charging
			if ( veh
				&& veh->m_pVehicle ) {//just make sure we have this veh info
				if ( ((pm->cmd.buttons&BUTTON_ATTACK)
					&& g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
					&&pm->ps->ammo[0] >= g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].iAmmoPerShot)
					||
					((pm->cmd.buttons&BUTTON_ALT_ATTACK)
					&& g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming
					&&pm->ps->ammo[1] >= g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].iAmmoPerShot) ) {//pressing the appropriate fire button for the lock-on/charging weapon
					PM_RocketLock( 16384, qtrue );
					charging = qtrue;
				}
				if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
					altFire = qtrue;
				}
			}
		}
		//else, let go and should fire now
	}
	else {
		// If you want your weapon to be a charging weapon, just set this bit up
		switch ( pm->ps->weapon ) {
		case WP_BRYAR_PISTOL:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
				charging = qtrue;
				altFire = qtrue;
			}
			break;

		case WP_CONCUSSION:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
				altFire = qtrue;
			break;

		case WP_BRYAR_OLD:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
				charging = qtrue;
				altFire = qtrue;
			}
			break;

		case WP_BOWCASTER:
			if ( pm->cmd.buttons & BUTTON_ATTACK )
				charging = qtrue;
			break;

		case WP_ROCKET_LAUNCHER:
			if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK)
				&& pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] >= weaponData[pm->ps->weapon].altEnergyPerShot ) {
				PM_RocketLock( 2048, qfalse );
				charging = qtrue;
				altFire = qtrue;
			}
			break;

		case WP_THERMAL:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
				altFire = qtrue; // override default of not being an alt-fire
				charging = qtrue;
			}
			else if ( pm->cmd.buttons & BUTTON_ATTACK )
				charging = qtrue;
			break;

		case WP_DEMP2:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
				altFire = qtrue; // override default of not being an alt-fire
				charging = qtrue;
			}
			break;

		case WP_DISRUPTOR:
			if ( (pm->cmd.buttons & BUTTON_ATTACK) && pm->ps->zoomMode == 1 && pm->ps->zoomLocked ) {
				if ( !pm->cmd.forwardmove && !pm->cmd.rightmove && pm->cmd.upmove <= 0 ) {
					charging = qtrue;
					altFire = qtrue;
				}
				else {
					charging = qfalse;
					altFire = qfalse;
				}
			}

			if ( pm->ps->zoomMode != 1 && pm->ps->weaponstate == WEAPON_CHARGING_ALT ) {
				pm->ps->weaponstate = WEAPON_READY;
				charging = qfalse;
				altFire = qfalse;
			}
			break;

		default:
			break;

		} // end switch
	}

	// set up the appropriate weapon state based on the button that's down.
	//	Note that we ALWAYS return if charging is set ( meaning the buttons are still down )
	if ( charging ) {
		if ( altFire ) {
			if ( pm->ps->weaponstate != WEAPON_CHARGING_ALT ) {
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;
				assert( pm->ps->weapon > WP_NONE );
				BG_AddPredictableEventToPlayerstate( EV_WEAPON_CHARGE_ALT, pm->ps->weapon, pm->ps );
			}

			if ( vehicleRocketLock ) {//check vehicle ammo
				if ( veh && pm->ps->ammo[1] < g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].iAmmoPerShot ) {
					pm->ps->weaponstate = WEAPON_CHARGING_ALT;
					goto rest;
				}
			}
			else if ( pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < (weaponData[pm->ps->weapon].altChargeSub + weaponData[pm->ps->weapon].altEnergyPerShot) ) {
				pm->ps->weaponstate = WEAPON_CHARGING_ALT;

				goto rest;
			}
			else if ( (pm->cmd.serverTime - pm->ps->weaponChargeTime) < weaponData[pm->ps->weapon].altMaxCharge ) {
				if ( pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime ) {
#ifdef _GAME
					if ( !((gentity_t *)pm_entSelf)->client->pers.adminData.merc || !japp_mercInfiniteAmmo.integer )
#endif
						pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].altChargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].altChargeSubTime;
				}
			}
		}
		else {
			if ( pm->ps->weaponstate != WEAPON_CHARGING ) {
				// charge isn't started, so do it now
				pm->ps->weaponstate = WEAPON_CHARGING;
				pm->ps->weaponChargeTime = pm->cmd.serverTime;
				pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;

#ifdef _DEBUG
				//	Com_Printf("Starting charge\n");
#endif
				BG_AddPredictableEventToPlayerstate( EV_WEAPON_CHARGE, pm->ps->weapon, pm->ps );
			}

			if ( vehicleRocketLock ) {
				if ( veh && pm->ps->ammo[0] < g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].iAmmoPerShot ) {//check vehicle ammo
					pm->ps->weaponstate = WEAPON_CHARGING;
					goto rest;
				}
			}
			else if ( pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < (weaponData[pm->ps->weapon].chargeSub + weaponData[pm->ps->weapon].energyPerShot) ) {
				pm->ps->weaponstate = WEAPON_CHARGING;

				goto rest;
			}
			else if ( (pm->cmd.serverTime - pm->ps->weaponChargeTime) < weaponData[pm->ps->weapon].maxCharge ) {
				if ( pm->ps->weaponChargeSubtractTime < pm->cmd.serverTime ) {
#ifdef _GAME
					if ( !((gentity_t *)pm_entSelf)->client->pers.adminData.merc || !japp_mercInfiniteAmmo.integer )
#endif
						pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= weaponData[pm->ps->weapon].chargeSub;
					pm->ps->weaponChargeSubtractTime = pm->cmd.serverTime + weaponData[pm->ps->weapon].chargeSubTime;
				}
			}
		}

		return qtrue; // short-circuit rest of weapon code
	}
rest:
	// Only charging weapons should be able to set these states...so....
	//	let's see which fire mode we need to set up now that the buttons are up
	if ( pm->ps->weaponstate == WEAPON_CHARGING ) {
		// weapon has a charge, so let us do an attack
#ifdef _DEBUG
		//	Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif

		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ATTACK;
		pm->ps->eFlags |= EF_FIRING;
	}
	else if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT ) {
		// weapon has a charge, so let us do an alt-attack
#ifdef _DEBUG
		//	Com_Printf("Firing.  Charge time=%d\n", pm->cmd.serverTime - pm->ps->weaponChargeTime);
#endif

		// dumb, but since we shoot a charged weapon on button-up, we need to repress this button for now
		pm->cmd.buttons |= BUTTON_ALT_ATTACK;
		pm->ps->eFlags |= (EF_FIRING | EF_ALT_FIRING);
	}

	return qfalse; // continue with the rest of the weapon code
}


#define BOWCASTER_CHARGE_UNIT	200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in g_weapon
#define BRYAR_CHARGE_UNIT		200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in g_weapon

int PM_ItemUsable( playerState_t *ps, int forcedUse ) {
	vector3 fwd, fwdorg, dest, pos;
	vector3 yawonly;
	vector3 mins, maxs;
	vector3 trtest;
	trace_t tr;

	if ( ps->m_iVehicleNum ) {
		return 0;
	}

	if ( ps->pm_flags & PMF_USE_ITEM_HELD ) { //force to let go first
		return 0;
	}

	if ( ps->duelInProgress ) { //not allowed to use holdables while in a private duel.
		return 0;
	}

	if ( !forcedUse ) {
		forcedUse = bg_itemlist[ps->stats[STAT_HOLDABLE_ITEM]].giTag;
	}

	if ( !BG_IsItemSelectable( ps, forcedUse ) ) {
		return 0;
	}

	switch ( forcedUse ) {
	case HI_MEDPAC:
	case HI_MEDPAC_BIG:
		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return 0;
		}
		if ( ps->stats[STAT_HEALTH] <= 0 ||
			(ps->eFlags & EF_DEAD) ) {
			return 0;
		}

		return 1;
	case HI_SEEKER:
		if ( ps->eFlags & EF_SEEKERDRONE ) {
			PM_AddEventWithParm( EV_ITEMUSEFAIL, SEEKER_ALREADYDEPLOYED );
			return 0;
		}

		return 1;
	case HI_SENTRY_GUN:
		if ( ps->fd.sentryDeployed ) {
			PM_AddEventWithParm( EV_ITEMUSEFAIL, SENTRY_ALREADYPLACED );
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

		pm->trace( &tr, &ps->origin, &mins, &maxs, &trtest, ps->clientNum, MASK_PLAYERSOLID );

		if ( (tr.fraction != 1 && tr.entityNum != ps->clientNum) || tr.startsolid || tr.allsolid ) {
			PM_AddEventWithParm( EV_ITEMUSEFAIL, SENTRY_NOROOM );
			return 0;
		}

		return 1;
	case HI_SHIELD:
		mins.x = -8;
		mins.y = -8;
		mins.z = 0;

		maxs.x = 8;
		maxs.y = 8;
		maxs.z = 8;

		AngleVectors( &ps->viewangles, &fwd, NULL, NULL );
		fwd.z = 0;
		VectorMA( &ps->origin, 64, &fwd, &dest );
		pm->trace( &tr, &ps->origin, &mins, &maxs, &dest, ps->clientNum, MASK_SHOT );
		if ( tr.fraction > 0.9f && !tr.startsolid && !tr.allsolid ) {
			VectorCopy( &tr.endpos, &pos );
			VectorSet( &dest, pos.x, pos.y, pos.z - 4096 );
			pm->trace( &tr, &pos, &mins, &maxs, &dest, ps->clientNum, MASK_SOLID );
			if ( !tr.startsolid && !tr.allsolid ) {
				return 1;
			}
		}
		PM_AddEventWithParm( EV_ITEMUSEFAIL, SHIELD_NOROOM );
		return 0;
	case HI_JETPACK: //check for stuff here?
		return 1;
	case HI_HEALTHDISP:
		return 1;
	case HI_AMMODISP:
		return 1;
	case HI_EWEB:
		return 1;
	case HI_CLOAK: //check for stuff here?
		return 1;
	default:
		return 1;
	}
}

//cheesy vehicle weapon hackery
qboolean PM_CanSetWeaponAnims( void ) {
	if ( pm->ps->m_iVehicleNum ) {
		return qfalse;
	}

	return qtrue;
}

//perform player anim overrides while on vehicle.
extern int PM_irand_timesync( int val1, int val2 );
void PM_VehicleWeaponAnimate( void ) {
	bgEntity_t *veh = pm_entVeh;
	Vehicle_t *pVeh;
	int iFlags = 0, iBlend = 0, Anim = -1;

	if ( !veh ||
		!veh->m_pVehicle ||
		!veh->m_pVehicle->m_pPilot ||
		!veh->m_pVehicle->m_pPilot->playerState ||
		pm->ps->clientNum != veh->m_pVehicle->m_pPilot->playerState->clientNum ) { //make sure the vehicle exists, and its pilot is this player
		return;
	}

	pVeh = veh->m_pVehicle;

	if ( pVeh->m_pVehicleInfo->type == VH_WALKER ||
		pVeh->m_pVehicleInfo->type == VH_FIGHTER ) { //slightly hacky I guess, but whatever.
		return;
	}
backAgain:
	// If they're firing, play the right fire animation.
	if ( pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK) ) {
		iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
		iBlend = 200;

		switch ( pm->ps->weapon ) {
		case WP_SABER:
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) { //don't do anything.. I guess.
				pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
				goto backAgain;
			}
			// If we're already in an attack animation, leave (let it continue).
			if ( pm->ps->torsoTimer <= 0 ) { //we'll be starting a new attack
				PM_AddEvent( EV_SABER_ATTACK );
			}

			//just set it to something so we have a proper trail. This is a stupid
			//hack (much like the rest of this function)
			pm->ps->saberMove = LS_R_TL2BR;

			if ( pm->ps->torsoTimer > 0 && (pm->ps->torsoAnim == BOTH_VS_ATR_S ||
				pm->ps->torsoAnim == BOTH_VS_ATL_S) ) {
				/*
				//FIXME: no need to even call the PM_SetAnim at all in this case
				Anim = (animNumber_t)pm->ps->torsoAnim;
				iFlags = SETANIM_FLAG_NORMAL;
				break;
				*/
				return;
			}

			// Start the attack.
			if ( pm->cmd.rightmove > 0 )	//right side attack
			{
				Anim = BOTH_VS_ATR_S;
			}
			else if ( pm->cmd.rightmove < 0 )	//left-side attack
			{
				Anim = BOTH_VS_ATL_S;
			}
			else	//random
			{
				//FIXME: alternate back and forth or auto-aim?
				//if ( !Q_irand( 0, 1 ) )
				if ( !PM_irand_timesync( 0, 1 ) ) {
					Anim = BOTH_VS_ATR_S;
				}
				else {
					Anim = BOTH_VS_ATL_S;
				}
			}

			if ( pm->ps->torsoTimer <= 0 ) { //restart the anim if we are already in it (and finished)
				iFlags |= SETANIM_FLAG_RESTART;
			}
			break;

		case WP_BLASTER:
			// Override the shoot anim.
			if ( pm->ps->torsoAnim == BOTH_ATTACK3 ) {
				if ( pm->cmd.rightmove > 0 )			//right side attack
				{
					Anim = BOTH_VS_ATR_G;
				}
				else if ( pm->cmd.rightmove < 0 )	//left side
				{
					Anim = BOTH_VS_ATL_G;
				}
				else	//frontal
				{
					Anim = BOTH_VS_ATF_G;
				}
			}
			break;

		default:
			Anim = BOTH_VS_IDLE;
			break;
		}
	}
	else if ( veh->playerState && veh->playerState->speed < 0 &&
		pVeh->m_pVehicleInfo->type == VH_ANIMAL ) { //tauntaun is going backwards
		Anim = BOTH_VT_WALK_REV;
		iBlend = 600;
	}
	else if ( veh->playerState && veh->playerState->speed < 0 &&
		pVeh->m_pVehicleInfo->type == VH_SPEEDER ) { //speeder is going backwards
		Anim = BOTH_VS_REV;
		iBlend = 600;
	}
	// They're not firing so play the Idle for the weapon.
	else {
		iFlags = SETANIM_FLAG_NORMAL;

		switch ( pm->ps->weapon ) {
		case WP_SABER:
			if ( BG_SabersOff( pm->ps ) ) { //saber holstered, normal idle
				Anim = BOTH_VS_IDLE;
			}
#if 0
			// In the Air.
			else if ( pVeh->m_ulFlags & VEH_FLYING )
				iBlend = 800;
				Anim = BOTH_VS_AIR_G;
				iFlags = SETANIM_FLAG_OVERRIDE;
			}
#endif
#if 0
			// Crashing.
			else if ( pVeh->m_ulFlags & VEH_CRASHING ) {
				pVeh->m_ulFlags &= ~VEH_CRASHING;	// Remove the flag, we are doing the animation.
				iBlend = 800;
				Anim = BOTH_VS_LAND_SR;
				iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			}
#endif
			else {
				Anim = BOTH_VS_IDLE_SR;
			}
			break;

		case WP_BLASTER:
#if 0
			// In the Air.
			if ( pVeh->m_ulFlags & VEH_FLYING ) {
				iBlend = 800;
				Anim = BOTH_VS_AIR_G;
				iFlags = SETANIM_FLAG_OVERRIDE;
			}
#endif
#if 0
			// Crashing.
			else if ( pVeh->m_ulFlags & VEH_CRASHING ) {
				pVeh->m_ulFlags &= ~VEH_CRASHING;	// Remove the flag, we are doing the animation.
				iBlend = 800;
				Anim = BOTH_VS_LAND_G;
				iFlags = SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD;
			}
#endif
			/*else*/ {
				Anim = BOTH_VS_IDLE_G;
			}
			break;

		default:
			Anim = BOTH_VS_IDLE;
			break;
		}
	}

	if ( Anim != -1 ) { //override it
		if ( pVeh->m_pVehicleInfo->type == VH_ANIMAL ) { //agh.. remap anims for the tauntaun
			switch ( Anim ) {
			case BOTH_VS_IDLE:
				if ( veh->playerState && veh->playerState->speed > 0 ) {
					if ( veh->playerState->speed > pVeh->m_pVehicleInfo->speedMax ) { //turbo
						Anim = BOTH_VT_TURBO;
					}
					else {
						Anim = BOTH_VT_RUN_FWD;
					}
				}
				else {
					Anim = BOTH_VT_IDLE;
				}
				break;
			case BOTH_VS_ATR_S:
				Anim = BOTH_VT_ATR_S;
				break;
			case BOTH_VS_ATL_S:
				Anim = BOTH_VT_ATL_S;
				break;
			case BOTH_VS_ATR_G:
				Anim = BOTH_VT_ATR_G;
				break;
			case BOTH_VS_ATL_G:
				Anim = BOTH_VT_ATL_G;
				break;
			case BOTH_VS_ATF_G:
				Anim = BOTH_VT_ATF_G;
				break;
			case BOTH_VS_IDLE_SL:
				Anim = BOTH_VT_IDLE_S;
				break;
			case BOTH_VS_IDLE_SR:
				Anim = BOTH_VT_IDLE_S;
				break;
			case BOTH_VS_IDLE_G:
				Anim = BOTH_VT_IDLE_G;
				break;

				//should not happen for tauntaun:
			case BOTH_VS_AIR_G:
			case BOTH_VS_LAND_SL:
			case BOTH_VS_LAND_SR:
			case BOTH_VS_LAND_G:
				return;
			default:
				break;
			}
		}

		PM_SetAnim( SETANIM_BOTH, Anim, iFlags, iBlend );
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
extern int PM_KickMoveForConditions( void );
static void PM_Weapon( void ) {
	int		addTime;
	int amount;
	int		killAfterItem = 0;
	bgEntity_t *veh = NULL;
	qboolean vehicleRocketLock = qfalse;

#ifdef _GAME
	if ( pm->ps->clientNum >= MAX_CLIENTS &&
		pm->ps->weapon == WP_NONE &&
		pm->cmd.weapon == WP_NONE &&
		pm_entSelf ) { //npc with no weapon
		gentity_t *gent = (gentity_t *)pm_entSelf;
		if ( gent->inuse && gent->client &&
			!gent->localAnimIndex ) { //humanoid
			pm->ps->torsoAnim = pm->ps->legsAnim;
			pm->ps->torsoTimer = pm->ps->legsTimer;
			return;
		}
	}
#endif

	if ( !pm->ps->emplacedIndex &&
		pm->ps->weapon == WP_EMPLACED_GUN ) { //oh no!
		int i = 0;
		int weap = -1;

		while ( i < WP_NUM_WEAPONS ) {
			if ( (pm->ps->stats[STAT_WEAPONS] & (1 << i)) && i != WP_NONE ) { //this one's good
				weap = i;
				break;
			}
			i++;
		}

		if ( weap != -1 ) {
			pm->cmd.weapon = weap;
			pm->ps->weapon = weap;
			return;
		}
	}

	if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&&pm->ps->m_iVehicleNum ) { //riding a vehicle
		veh = pm_entVeh;
		if ( veh && veh->m_pVehicle && (veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER) ) {//riding a walker/fighter
			//keep saber off, do no weapon stuff at all!
			pm->ps->saberHolstered = 2;
#ifdef _GAME
			pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
#else
			if ( g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
				|| g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming ) {//our vehicle uses a rocket launcher, so do the normal checks
				vehicleRocketLock = qtrue;
				pm->cmd.buttons &= ~BUTTON_ATTACK;
			}
			else {
				pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
			}
#endif
		}
	}

	if ( pm->ps->weapon != WP_DISRUPTOR //not using disruptor
		&& pm->ps->weapon != WP_ROCKET_LAUNCHER//not using rocket launcher
		&& pm->ps->weapon != WP_THERMAL//not using thermals
		&& !pm->ps->m_iVehicleNum )//not a vehicle or in a vehicle
	{ //check for exceeding max charge time if not using disruptor or rocket launcher or thermals
		if ( pm->ps->weaponstate == WEAPON_CHARGING_ALT ) {
			int timeDif = (pm->cmd.serverTime - pm->ps->weaponChargeTime);

			if ( timeDif > MAX_WEAPON_CHARGE_TIME ) {
				pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			}
		}

		if ( pm->ps->weaponstate == WEAPON_CHARGING ) {
			int timeDif = (pm->cmd.serverTime - pm->ps->weaponChargeTime);

			if ( timeDif > MAX_WEAPON_CHARGE_TIME ) {
				pm->cmd.buttons &= ~BUTTON_ATTACK;
			}
		}
	}

	if ( pm->ps->forceHandExtend == HANDEXTEND_WEAPONREADY &&
		PM_CanSetWeaponAnims() ) { //reset into weapon stance
		if ( pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE && !PM_IsRocketTrooper() ) { //saber handles its own anims
			if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) {
				//PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
				PM_StartTorsoAnim( TORSO_RAISEWEAP1 );
			}
			else {
				if ( pm->ps->weapon == WP_EMPLACED_GUN ) {
					PM_StartTorsoAnim( BOTH_GUNSIT1 );
				}
				else {
					//PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
					PM_StartTorsoAnim( TORSO_RAISEWEAP1 );
				}
			}
		}

		//we now go into a weapon raise anim after every force hand extend.
		//this is so that my holster-view-weapon-when-hand-extend stuff works.
		pm->ps->weaponstate = WEAPON_RAISING;
		pm->ps->weaponTime += 250;

		pm->ps->forceHandExtend = HANDEXTEND_NONE;
	}
	else if ( pm->ps->forceHandExtend != HANDEXTEND_NONE ) { //nothing else should be allowed to happen during this time, including weapon fire
		int desiredAnim = 0;
		qboolean seperateOnTorso = qfalse;
		qboolean playFullBody = qfalse;
		int desiredOnTorso = 0;

		switch ( pm->ps->forceHandExtend ) {
		case HANDEXTEND_FORCEPUSH:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		case HANDEXTEND_FORCEPULL:
			desiredAnim = BOTH_FORCEPULL;
			break;
		case HANDEXTEND_FORCE_HOLD:
			if ( (pm->ps->fd.forcePowersActive&(1 << FP_GRIP)) ) {//gripping
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			else if ( (pm->ps->fd.forcePowersActive&(1 << FP_LIGHTNING)) ) {//lightning
				if ( pm->ps->weapon == WP_MELEE
					&& pm->ps->activeForcePass > FORCE_LEVEL_2 ) {//2-handed lightning
					desiredAnim = BOTH_FORCE_2HANDEDLIGHTNING_HOLD;
				}
				else {
					desiredAnim = BOTH_FORCELIGHTNING_HOLD;
				}
			}
			else if ( (pm->ps->fd.forcePowersActive&(1 << FP_DRAIN)) ) {//draining
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			else {//???
				desiredAnim = BOTH_FORCEGRIP_HOLD;
			}
			break;
		case HANDEXTEND_SABERPULL:
			desiredAnim = BOTH_SABERPULL;
			break;
		case HANDEXTEND_CHOKE:
			desiredAnim = BOTH_CHOKE3; //left-handed choke
			break;
		case HANDEXTEND_DODGE:
			desiredAnim = pm->ps->forceDodgeAnim;
			break;
		case HANDEXTEND_KNOCKDOWN:
			if ( pm->ps->forceDodgeAnim ) {
				if ( pm->ps->forceDodgeAnim > 4 ) { //this means that we want to play a sepereate anim on the torso
					int originalDAnim = pm->ps->forceDodgeAnim - 8; //-8 is the original legs anim
					if ( originalDAnim == 2 ) {
						desiredAnim = BOTH_FORCE_GETUP_B1;
					}
					else if ( originalDAnim == 3 ) {
						desiredAnim = BOTH_FORCE_GETUP_B3;
					}
					else {
						desiredAnim = BOTH_GETUP1;
					}

					//now specify the torso anim
					seperateOnTorso = qtrue;
					desiredOnTorso = BOTH_FORCEPUSH;
				}
				else if ( pm->ps->forceDodgeAnim == 2 ) {
					desiredAnim = BOTH_FORCE_GETUP_B1;
				}
				else if ( pm->ps->forceDodgeAnim == 3 ) {
					desiredAnim = BOTH_FORCE_GETUP_B3;
				}
				else {
					desiredAnim = BOTH_GETUP1;
				}
			}
			else {
				//Raz: Melee grapple additions
				if ( pm->ps->torsoAnim == BOTH_PLAYER_PA_3_FLY && pm->ps->legsAnim == BOTH_PLAYER_PA_3_FLY )
					desiredAnim = BOTH_PLAYER_PA_3_FLY;
				else
					desiredAnim = BOTH_KNOCKDOWN1;
			}
			break;
		case HANDEXTEND_DUELCHALLENGE:
			desiredAnim = BOTH_ENGAGETAUNT;
			break;
		case HANDEXTEND_TAUNT:
			desiredAnim = pm->ps->forceDodgeAnim;
			if ( desiredAnim != BOTH_ENGAGETAUNT
				&& VectorCompare( &pm->ps->velocity, &vec3_origin )
				&& pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
				playFullBody = qtrue;
			}
			break;
		case HANDEXTEND_PRETHROW:
			desiredAnim = BOTH_A3_TL_BR;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROW:
			desiredAnim = BOTH_D3_TL___;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_PRETHROWN:
			desiredAnim = BOTH_KNEES1;
			playFullBody = qtrue;
			break;
		case HANDEXTEND_POSTTHROWN:
			if ( pm->ps->forceDodgeAnim ) {
				desiredAnim = BOTH_FORCE_GETUP_F2;
			}
			else {
				desiredAnim = BOTH_KNOCKDOWN5;
			}
			playFullBody = qtrue;

			break;
		case HANDEXTEND_DRAGGING:
			desiredAnim = BOTH_B1_BL___;
			break;
		case HANDEXTEND_JEDITAUNT:
			desiredAnim = BOTH_GESTURE1;
			//playFullBody = qtrue;
			break;
			//Hmm... maybe use these, too?
			//BOTH_FORCEHEAL_QUICK //quick heal (SP level 2 & 3)
			//BOTH_MINDTRICK1 // wave (maybe for mind trick 2 & 3 - whole area, and for force seeing)
			//BOTH_MINDTRICK2 // tap (maybe for mind trick 1 - one person)
			//BOTH_FORCEGRIP_START //start grip
			//BOTH_FORCEGRIP_HOLD //hold grip
			//BOTH_FORCEGRIP_RELEASE //release grip
			//BOTH_FORCELIGHTNING //quick lightning burst (level 1)
			//BOTH_FORCELIGHTNING_START //start lightning
			//BOTH_FORCELIGHTNING_HOLD //hold lightning
			//BOTH_FORCELIGHTNING_RELEASE //release lightning
		default:
			desiredAnim = BOTH_FORCEPUSH;
			break;
		}

		if ( !seperateOnTorso ) { //of seperateOnTorso, handle it after setting the legs
			PM_SetAnim( SETANIM_TORSO, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100 );
			pm->ps->torsoTimer = 1;
		}

		if ( playFullBody ) { //sorry if all these exceptions are getting confusing. This one just means play on both legs and torso.
			PM_SetAnim( SETANIM_BOTH, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100 );
			pm->ps->legsTimer = pm->ps->torsoTimer = 1;
		}
		else if ( pm->ps->forceHandExtend == HANDEXTEND_DODGE || pm->ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
			(pm->ps->forceHandExtend == HANDEXTEND_CHOKE && pm->ps->groundEntityNum == ENTITYNUM_NONE) ) { //special case, play dodge anim on whole body, choke anim too if off ground
			if ( seperateOnTorso ) {
				PM_SetAnim( SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100 );
				pm->ps->legsTimer = 1;

				PM_SetAnim( SETANIM_TORSO, desiredOnTorso, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100 );
				pm->ps->torsoTimer = 1;
			}
			else {
				PM_SetAnim( SETANIM_LEGS, desiredAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 100 );
				pm->ps->legsTimer = 1;
			}
		}

		return;
	}

	if ( BG_InSpecialJump( pm->ps->legsAnim ) ||
		BG_InRoll( pm->ps, pm->ps->legsAnim ) ||
		PM_InRollComplete( pm->ps, pm->ps->legsAnim ) ) {
		/*
		if (pm->cmd.weapon != WP_MELEE &&
		pm->ps->weapon != WP_MELEE &&
		(pm->ps->stats[STAT_WEAPONS] & (1<<WP_SABER)))
		{ //it's alright also if we are melee
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;
		}
		*/
		if ( pm->ps->weaponTime < pm->ps->legsTimer ) {
			pm->ps->weaponTime = pm->ps->legsTimer;
		}
	}

	if ( pm->ps->duelInProgress ) {
		if ( GetCInfo( CINFO_PRIVDUELWEAP ) )
			pm->cmd.weapon = pm->ps->weapon;
		else
			pm->cmd.weapon = pm->ps->weapon = WP_SABER;

		if ( pm->ps->duelTime >= pm->cmd.serverTime ) {
			pm->cmd.upmove = 0;
			pm->cmd.forwardmove = 0;
			pm->cmd.rightmove = 0;
		}
	}

	if ( pm->ps->weapon == WP_SABER && pm->ps->saberMove != LS_READY && pm->ps->saberMove != LS_NONE ) {
		pm->cmd.weapon = WP_SABER; //don't allow switching out mid-attack
	}

	if ( pm->ps->weapon == WP_SABER ) {
		//rww - we still need the item stuff, so we won't return immediately
		PM_WeaponLightsaber();
		killAfterItem = 1;
	}
	else if ( pm->ps->weapon != WP_EMPLACED_GUN ) {
		pm->ps->saberHolstered = 0;
	}

	if ( PM_CanSetWeaponAnims() ) {
		if ( pm->ps->weapon == WP_THERMAL ||
			pm->ps->weapon == WP_TRIP_MINE ||
			pm->ps->weapon == WP_DET_PACK ) {
			if ( pm->ps->weapon == WP_THERMAL ) {
				if ( (pm->ps->torsoAnim) == WeaponAttackAnim[pm->ps->weapon] &&
					(pm->ps->weaponTime - 200) <= 0 ) {
					PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
				}
			}
			else {
				if ( (pm->ps->torsoAnim) == WeaponAttackAnim[pm->ps->weapon] &&
					(pm->ps->weaponTime - 700) <= 0 ) {
					PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
				}
			}
		}
	}

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->clientNum < MAX_CLIENTS && pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {

		//Raz: rocket lock bug, one of many...
		BG_ClearRocketLock( pm->ps );

		if ( !(pm->ps->pm_flags & PMF_USE_ITEM_HELD) ) {

			if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
				&& pm->ps->m_iVehicleNum ) {//riding a vehicle, can't use holdable items, this button operates as the weapon link/unlink toggle
				return;
			}

			if ( !pm->ps->stats[STAT_HOLDABLE_ITEM] ) {
				return;
			}

			if ( !PM_ItemUsable( pm->ps, 0 ) ) {
				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				return;
			}
			else {
				if ( pm->ps->stats[STAT_HOLDABLE_ITEMS] & (1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag) ) {
					if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS &&
						bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_JETPACK &&
						bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_HEALTHDISP &&
						bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_AMMODISP &&
						bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_CLOAK &&
						bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_EWEB ) { //never use up the binoculars or jetpack or dispensers or cloak or ...
						pm->ps->stats[STAT_HOLDABLE_ITEMS] -= (1 << bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag);
					}
				}
				else {
					return; //this should not happen...
				}

				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
				PM_AddEvent( EV_USE_ITEM0 + bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag );

				if ( bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_BINOCULARS &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_JETPACK &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_HEALTHDISP &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_AMMODISP &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_CLOAK &&
					bg_itemlist[pm->ps->stats[STAT_HOLDABLE_ITEM]].giTag != HI_EWEB ) {
					pm->ps->stats[STAT_HOLDABLE_ITEM] = 0;
					BG_CycleInven( pm->ps, 1 );
				}
			}
			return;
		}
	}
	else {
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}

	/*
	if (pm->ps->weapon == WP_SABER || pm->ps->weapon == WP_MELEE)
	{ //we can't toggle zoom while using saber (for obvious reasons) so make sure it's always off
	pm->ps->zoomMode = 0;
	pm->ps->zoomFov = 0;
	pm->ps->zoomLocked = qfalse;
	pm->ps->zoomLockTime = 0;
	}
	*/

	if ( killAfterItem ) {
		return;
	}

	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
	}

	if ( pm->ps->isJediMaster && pm->ps->emplacedIndex ) {
		pm->ps->emplacedIndex = 0;
		pm->ps->saberHolstered = 0;
	}

	if ( pm->ps->duelInProgress && pm->ps->emplacedIndex ) {
		pm->ps->emplacedIndex = 0;
		pm->ps->saberHolstered = 0;
	}

	if ( pm->ps->weapon == WP_EMPLACED_GUN && pm->ps->emplacedIndex ) {
		pm->cmd.weapon = WP_EMPLACED_GUN; //No switch for you!
		PM_StartTorsoAnim( BOTH_GUNSIT1 );
	}

	if ( pm->ps->isJediMaster
		|| (pm->ps->duelInProgress && !GetCInfo( CINFO_PRIVDUELWEAP ))
		|| pm->ps->trueJedi ) {
		pm->cmd.weapon = WP_SABER;
		pm->ps->weapon = WP_SABER;

		if ( pm->ps->isJediMaster || pm->ps->trueJedi ) {
			pm->ps->stats[STAT_WEAPONS] = (1 << WP_SABER);
		}
	}

	// take an ammo away if not infinite
	if ( pm->ps->weapon != WP_NONE &&
		pm->ps->weapon == pm->cmd.weapon &&
		(pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING) ) {
		if ( pm->ps->clientNum < MAX_CLIENTS && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] != -1 ) {
			// enough energy to fire this weapon?
			if ( pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].energyPerShot &&
				pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < weaponData[pm->ps->weapon].altEnergyPerShot ) { //the weapon is out of ammo essentially because it cannot fire primary or secondary, so do the switch
				//regardless of if the player is attacking or not
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon );

				if ( pm->ps->weaponTime < 500 ) {
					pm->ps->weaponTime += 500;
				}
				return;
			}

			if ( pm->ps->weapon == WP_DET_PACK && !pm->ps->hasDetPackPlanted && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] < 1 ) {
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon );

				if ( pm->ps->weaponTime < 500 ) {
					pm->ps->weaponTime += 500;
				}
				return;
			}
		}
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->cmd.weapon );
		}
	}

	if ( pm->ps->weaponTime > 0 ) {
		return;
	}

	if ( pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1 ) {
		if ( pm_cancelOutZoom ) {
			pm->ps->zoomMode = 0;
			pm->ps->zoomFov = 0;
			pm->ps->zoomLocked = qfalse;
			pm->ps->zoomLockTime = 0;
			PM_AddEvent( EV_DISRUPTOR_ZOOMSOUND );
			return;
		}

		if ( pm->cmd.forwardmove ||
			pm->cmd.rightmove ||
			pm->cmd.upmove > 0 ) {
			return;
		}
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;
		if ( PM_CanSetWeaponAnims() ) {
			if ( pm->ps->weapon == WP_SABER ) {
				PM_StartTorsoAnim( PM_GetSaberStance() );
			}
			else if ( pm->ps->weapon == WP_MELEE || PM_IsRocketTrooper() ) {
				PM_StartTorsoAnim( pm->ps->legsAnim );
			}
			else {
				if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) {
					PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
				}
				else {
					if ( pm->ps->weapon == WP_EMPLACED_GUN ) {
						PM_StartTorsoAnim( BOTH_GUNSIT1 );
					}
					else {
						PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
					}
				}
			}
		}
		return;
	}

	if ( PM_CanSetWeaponAnims() &&
		!PM_IsRocketTrooper() &&
		pm->ps->weaponstate == WEAPON_READY && pm->ps->weaponTime <= 0 &&
		(pm->ps->weapon >= WP_BRYAR_PISTOL || pm->ps->weapon == WP_STUN_BATON) &&
		pm->ps->torsoTimer <= 0 &&
		(pm->ps->torsoAnim) != WeaponReadyAnim[pm->ps->weapon] &&
		pm->ps->torsoAnim != TORSO_WEAPONIDLE3 &&
		pm->ps->weapon != WP_EMPLACED_GUN ) {
		PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
	}
	else if ( PM_CanSetWeaponAnims() &&
		pm->ps->weapon == WP_MELEE ) {
		if ( pm->ps->weaponTime <= 0 &&
			pm->ps->forceHandExtend == HANDEXTEND_NONE ) {
			int desTAnim = pm->ps->legsAnim;

			if ( desTAnim == BOTH_STAND1 ||
				desTAnim == BOTH_STAND2 ) { //remap the standard standing anims for melee stance
				desTAnim = BOTH_STAND6;
			}

			if ( !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) ) { //don't do this while holding attack
				if ( pm->ps->torsoAnim != desTAnim ) {
					PM_StartTorsoAnim( desTAnim );
				}
			}
		}
	}
	else if ( PM_CanSetWeaponAnims() && PM_IsRocketTrooper() ) {
		int desTAnim = pm->ps->legsAnim;

		if ( !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) ) { //don't do this while holding attack
			if ( pm->ps->torsoAnim != desTAnim ) {
				PM_StartTorsoAnim( desTAnim );
			}
		}
	}

	if ( ((pm->ps->torsoAnim) == TORSO_WEAPONREADY4 ||
		(pm->ps->torsoAnim) == BOTH_ATTACK4) &&
		(pm->ps->weapon != WP_DISRUPTOR || pm->ps->zoomMode != 1) ) {
		if ( pm->ps->weapon == WP_EMPLACED_GUN ) {
			PM_StartTorsoAnim( BOTH_GUNSIT1 );
		}
		else if ( PM_CanSetWeaponAnims() ) {
			PM_StartTorsoAnim( WeaponReadyAnim[pm->ps->weapon] );
		}
	}
	else if ( ((pm->ps->torsoAnim) != TORSO_WEAPONREADY4 &&
		(pm->ps->torsoAnim) != BOTH_ATTACK4) &&
		PM_CanSetWeaponAnims() &&
		(pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1) ) {
		PM_StartTorsoAnim( TORSO_WEAPONREADY4 );
	}

	if ( pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE ) {//we are a vehicle
		veh = pm_entSelf;
	}
	if ( veh
		&& veh->m_pVehicle ) {
		if ( g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID].fHoming
			|| g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID].fHoming ) {//don't clear the rocket locking ever?
			vehicleRocketLock = qtrue;
		}
	}

	if ( !vehicleRocketLock ) {
		if ( pm->ps->weapon != WP_ROCKET_LAUNCHER ) {
			if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
				&&pm->ps->m_iVehicleNum ) {//riding a vehicle, the vehicle will tell me my rocketlock stuff...
			}
			else {
				pm->ps->rocketLockIndex = ENTITYNUM_NONE;
				pm->ps->rocketLockTime = 0;
				pm->ps->rocketTargetTime = 0;
			}
		}
	}

	if ( PM_DoChargedWeapons( vehicleRocketLock, veh ) ) {
		// In some cases the charged weapon code may want us to short circuit the rest of the firing code
		return;
	}

	// check for fire
	if ( !(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) ) {
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if ( pm->ps->weapon == WP_EMPLACED_GUN ) {
		addTime = weaponData[pm->ps->weapon].fireTime;
		pm->ps->weaponTime += addTime;
		if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) ) {
			PM_AddEvent( EV_ALT_FIRE );
		}
		else {
			PM_AddEvent( EV_FIRE_WEAPON );
		}
		return;
	}
	else if ( pm->ps->m_iVehicleNum
		&& pm_entSelf->s.NPC_class == CLASS_VEHICLE ) { //a vehicle NPC that has a pilot
		pm->ps->weaponstate = WEAPON_FIRING;
		pm->ps->weaponTime += 100;
#ifdef _GAME //hack, only do it game-side. vehicle weapons don't really need predicting I suppose.
		if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) ) {
			G_CheapWeaponFire( pm->ps->clientNum, EV_ALT_FIRE );
		}
		else {
			G_CheapWeaponFire( pm->ps->clientNum, EV_FIRE_WEAPON );
		}
#endif
		/*
		addTime = weaponData[WP_EMPLACED_GUN].fireTime;
		pm->ps->weaponTime += addTime;
		if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) )
		{
		PM_AddEvent( EV_ALT_FIRE );
		}
		else
		{
		PM_AddEvent( EV_FIRE_WEAPON );
		}
		*/
		return;
	}

	if ( pm->ps->weapon == WP_DISRUPTOR &&
		(pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
		!pm->ps->zoomLocked ) {
		return;
	}

	if ( pm->ps->weapon == WP_DISRUPTOR &&
		(pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
		pm->ps->zoomMode == 2 ) { //can't use disruptor secondary while zoomed binoculars
		return;
	}

	if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) {
		PM_StartTorsoAnim( BOTH_ATTACK4 );
	}
	else if ( pm->ps->weapon == WP_MELEE ) { //special anims for standard melee attacks
		//Alternate between punches and use the anim length as weapon time.
		if ( !pm->ps->m_iVehicleNum ) { //if riding a vehicle don't do this stuff at all
			if ( pm->debugMelee &&
				(pm->cmd.buttons & BUTTON_ATTACK) &&
				(pm->cmd.buttons & BUTTON_ALT_ATTACK) ) { //ok, grapple time
#if 0 //eh, I want to try turning the saber off, but can't do that reliably for prediction..
				qboolean icandoit = qtrue;
				if (pm->ps->weaponTime > 0)
				{ //weapon busy
					icandoit = qfalse;
				}
				if (pm->ps->forceHandExtend != HANDEXTEND_NONE)
				{ //force power or knockdown or something
					icandoit = qfalse;
				}
				if (pm->ps->weapon != WP_SABER && pm->ps->weapon != WP_MELEE)
				{
					icandoit = qfalse;
				}

				if (icandoit)
				{
					//G_SetAnim(ent, &ent->client->pers.cmd, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
					PM_SetAnim(SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0);
					if (pm->ps->torsoAnim == BOTH_KYLE_GRAB)
					{ //providing the anim set succeeded..
						pm->ps->torsoTimer += 500; //make the hand stick out a little longer than it normally would
						if (pm->ps->legsAnim == pm->ps->torsoAnim)
						{
							pm->ps->legsTimer = pm->ps->torsoTimer;
						}
						pm->ps->weaponTime = pm->ps->torsoTimer;
						return;
					}
				}
#else
#ifdef _GAME
				if ( pm_entSelf ) {
					if ( TryGrapple( (gentity_t *)pm_entSelf ) ) {
						return;
					}
				}
#else
				return;
#endif
#endif
			}
			else if ( pm->debugMelee &&
				(pm->cmd.buttons & BUTTON_ALT_ATTACK) ) { //kicks
				if ( !BG_KickingAnim( pm->ps->torsoAnim ) &&
					!BG_KickingAnim( pm->ps->legsAnim ) ) {
					int kickMove = PM_KickMoveForConditions();
					if ( kickMove == LS_HILT_BASH ) { //yeah.. no hilt to bash with!
						kickMove = LS_KICK_F;
					}

					if ( kickMove != -1 ) {
						if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {//if in air, convert kick to an in-air kick
							float gDist = PM_GroundDistance();
							//let's only allow air kicks if a certain distance from the ground
							//it's silly to be able to do them right as you land.
							//also looks wrong to transition from a non-complete flip anim...
							if ( (!BG_FlippingAnim( pm->ps->legsAnim ) || pm->ps->legsTimer <= 0) &&
								gDist > 64.0f && //strict minimum
								gDist > (-pm->ps->velocity.z) - 64.0f //make sure we are high to ground relative to downward velocity as well
								) {
								switch ( kickMove ) {
								case LS_KICK_F:
									kickMove = LS_KICK_F_AIR;
									break;
								case LS_KICK_B:
									kickMove = LS_KICK_B_AIR;
									break;
								case LS_KICK_R:
									kickMove = LS_KICK_R_AIR;
									break;
								case LS_KICK_L:
									kickMove = LS_KICK_L_AIR;
									break;
								default: //oh well, can't do any other kick move while in-air
									kickMove = -1;
									break;
								}
							}
							else { //off ground, but too close to ground
								kickMove = -1;
							}
						}
					}

					if ( kickMove != -1 ) {
						int kickAnim = saberMoveData[kickMove].animToUse;

						if ( kickAnim != -1 ) {
							PM_SetAnim( SETANIM_BOTH, kickAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
							if ( pm->ps->legsAnim == kickAnim ) {
								pm->ps->weaponTime = pm->ps->legsTimer;
								return;
							}
						}
					}
				}

				//if got here then no move to do so put torso into leg idle or whatever
				if ( pm->ps->torsoAnim != pm->ps->legsAnim ) {
					PM_SetAnim( SETANIM_BOTH, pm->ps->legsAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
				}
				pm->ps->weaponTime = 0;
				return;
			}
			else { //just punch
				int desTAnim = BOTH_MELEE1;
				if ( pm->ps->torsoAnim == BOTH_MELEE1 ) {
					desTAnim = BOTH_MELEE2;
				}
				PM_StartTorsoAnim( desTAnim );

				if ( pm->ps->torsoAnim == desTAnim ) {
					pm->ps->weaponTime = pm->ps->torsoTimer;
				}
			}
		}
	}
	else {
		//Raz: Hacky fix here
		int weapon = pm->ps->weapon;
#ifdef _GAME
		if ( !Client_Supports( (gentity_t *)pm_entSelf, CSF_FIXED_WEAPON_ANIMS ) && (weapon == WP_CONCUSSION || weapon == WP_BRYAR_OLD) )
#else
		if ( !Server_Supports( SSF_FIXED_WEAP_ANIMS ) && (weapon == WP_CONCUSSION || weapon == WP_BRYAR_OLD) )
#endif
			weapon++;
		PM_StartTorsoAnim( WeaponAttackAnim[weapon] );
	}

#ifdef _GAME
	if ( ((gentity_t *)pm_entSelf)->client->pers.adminData.merc && japp_mercInfiniteAmmo.integer )
		amount = 0;
	else
#endif
	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK )
		amount = weaponData[pm->ps->weapon].altEnergyPerShot;
	else
		amount = weaponData[pm->ps->weapon].energyPerShot;

	pm->ps->weaponstate = WEAPON_FIRING;

	// take an ammo away if not infinite
	if ( pm->ps->clientNum < MAX_CLIENTS && pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] != -1 ) {
		// enough energy to fire this weapon?
		if ( (pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] - amount) >= 0 ) {
			pm->ps->ammo[weaponData[pm->ps->weapon].ammoIndex] -= amount;
		}
		else	// Not enough energy
		{
			// Switch weapons
			if ( pm->ps->weapon != WP_DET_PACK || !pm->ps->hasDetPackPlanted ) {
				PM_AddEventWithParm( EV_NOAMMO, WP_NUM_WEAPONS + pm->ps->weapon );
				if ( pm->ps->weaponTime < 500 ) {
					pm->ps->weaponTime += 500;
				}
			}
			return;
		}
	}

	if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
#if 0
		if ( pm->ps->weapon == WP_BRYAR_PISTOL && pm->gametype != GT_SIEGE )
			PM_AddEvent( EV_FIRE_WEAPON );
			addTime = weaponData[pm->ps->weapon].fireTime;
		}
		else if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode != 1 ) {
#else
		if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode != 1 ) {
#endif
			PM_AddEvent( EV_FIRE_WEAPON );
			addTime = weaponData[pm->ps->weapon].fireTime;
		}
		else {
			if ( pm->ps->weapon != WP_MELEE ||
				!pm->ps->m_iVehicleNum ) { //do not fire melee events at all when on vehicle
				PM_AddEvent( EV_ALT_FIRE );
			}
			addTime = weaponData[pm->ps->weapon].altFireTime;
		}
	}
	else {
		if ( pm->ps->weapon != WP_MELEE ||
			!pm->ps->m_iVehicleNum ) { //do not fire melee events at all when on vehicle
			PM_AddEvent( EV_FIRE_WEAPON );
		}
		addTime = weaponData[pm->ps->weapon].fireTime;
		if ( pm->gametype == GT_SIEGE && pm->ps->weapon == WP_DET_PACK ) {	// were far too spammy before?  So says Rick.
			addTime *= 2;
		}
	}

	/*
	if ( pm->ps->powerups[PW_HASTE] ) {
	addTime /= 1.3f;
	}
	*/

	if ( pm->ps->fd.forcePowersActive & (1 << FP_RAGE) ) {
		addTime *= 0.75f;
	}
	else if ( pm->ps->fd.forceRageRecoveryTime > pm->cmd.serverTime ) {
		addTime *= 1.5f;
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->m_iVehicleNum ) { //eh, fine, clear it
			if ( pm->ps->forceHandExtendTime < pm->cmd.serverTime ) {
				pm->ps->forceHandExtend = HANDEXTEND_NONE;
			}
		}

		if ( pm->ps->torsoTimer < 1 && pm->ps->forceHandExtend == HANDEXTEND_NONE &&
			pm->ps->legsTimer < 1 && pm->ps->weaponTime < 1 && pm->ps->saberLockTime < pm->cmd.serverTime ) {

			pm->ps->forceHandExtend = HANDEXTEND_TAUNT;

			//FIXME: random taunt anims?
			pm->ps->forceDodgeAnim = BOTH_ENGAGETAUNT;

			pm->ps->forceHandExtendTime = pm->cmd.serverTime + 1000;

			//pm->ps->weaponTime = 100;

			PM_AddEvent( EV_TAUNT );
		}
#if 0
		// Here's an interesting bit.  The bots in TA used buttons to do additional gestures.
		// I ripped them out because I didn't want too many buttons given the fact that I was already adding some for JK2.
		// We can always add some back in if we want though.
	} else if ( pm->cmd.buttons & BUTTON_GETFLAG ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GETFLAG );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_GUARDBASE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_GUARDBASE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_PATROL ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_PATROL );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_FOLLOWME ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_FOLLOWME );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_AFFIRMATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_AFFIRMATIVE);
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
	} else if ( pm->cmd.buttons & BUTTON_NEGATIVE ) {
		if ( pm->ps->torsoTimer == 0 ) {
			PM_StartTorsoAnim( TORSO_NEGATIVE );
			pm->ps->torsoTimer = 600;	//TIMER_GESTURE;
		}
#endif //
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}
		else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}

	if ( GetCInfo( CINFO_CPMPHYSICS ) ) {
		if ( pm->ps->stats[STAT_JUMPTIME] > 0 )
			pm->ps->stats[STAT_JUMPTIME] -= pml.msec;
	}
}

// Following function is stateless (at the moment). And hoisting it out of the namespace here is easier than fixing all
//	the places it's used, which includes files that are also compiled in SP. We do need to make sure we only get one copy
//	in the linker, though.
qboolean BG_UnrestrainedPitchRoll( playerState_t *ps, Vehicle_t *pVeh ) {
	//FIXME: specify per vehicle instead of assuming true for all fighters
	//FIXME: map/server setting?
	if ( bg_fighterAltControl.integer && ps->clientNum < MAX_CLIENTS && ps->m_iVehicleNum && pVeh && pVeh->m_pVehicleInfo
		&& pVeh->m_pVehicleInfo->type == VH_FIGHTER ) {
		// can roll and pitch without limitation!
		return qtrue;
	}
	return qfalse;
}

// This can be used as another entry point when only the viewangles are being updated instead of a full move
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short temp;
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION )
		return;

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 )
		return;		// no view changes at all

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ ) {
		temp = cmd->angles.data[i] + ps->delta_angles.data[i];
#ifdef VEH_CONTROL_SCHEME_4
		if ( pm_entVeh && pm_entVeh->m_pVehicle && pm_entVeh->m_pVehicle->m_pVehicleInfo
			&& pm_entVeh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
			&& (cmd->serverTime-pm_entVeh->playerState->hyperSpaceTime) >= HYPERSPACE_TIME )
		{//in a vehicle and not hyperspacing
			if ( i == 0/*PITCH*/ ) {
				int pitchClamp = ANGLE2SHORT( AngleNormalize180( pm_entVeh->m_pVehicle->m_vPrevRiderViewAngles.pitch+10.0f ) );
				// don't let the player look up or down more than 22.5f degrees
				if ( temp > pitchClamp ) {
					ps->delta_angles.data[i] = pitchClamp - cmd->angles.data[i];
					temp = pitchClamp;
				}
				else if ( temp < -pitchClamp ) {
					ps->delta_angles.data[i] = -pitchClamp - cmd->angles.data[i];
					temp = -pitchClamp;
				}
			}
			if ( i == 1/*YAW*/ ) {
				int yawClamp = ANGLE2SHORT( AngleNormalize180( pm_entVeh->m_pVehicle->m_vPrevRiderViewAngles.yaw+10.0f ) );
				// don't let the player look left or right more than 22.5f degrees
				if ( temp > yawClamp ) {
					ps->delta_angles.data[i] = yawClamp - cmd->angles.data[i];
					temp = yawClamp;
				}
				else if ( temp < -yawClamp ) {
					ps->delta_angles.data[i] = -yawClamp - cmd->angles.data[i];
					temp = -yawClamp;
				}
			}
		}
#else //VEH_CONTROL_SCHEME_4
		if ( pm_entVeh && BG_UnrestrainedPitchRoll( ps, pm_entVeh->m_pVehicle ) ) {
			/*
			if ( i == ROLL ) {
			// get roll from vehicle
			ps->viewangles.roll = pm_entVeh->playerState->viewangles.roll;//->m_pVehicle->m_vOrientation->roll;
			continue;

			}
			*/
		}
#endif // VEH_CONTROL_SCHEME_4
		else {
			if ( i == 0/*PITCH*/ ) {
				// don't let the player look up or down more than 90 degrees
				if ( temp > 16000 ) {
					ps->delta_angles.data[i] = 16000 - cmd->angles.data[i];
					temp = 16000;
				}
				else if ( temp < -16000 ) {
					ps->delta_angles.data[i] = -16000 - cmd->angles.data[i];
					temp = -16000;
				}
			}
		}
		ps->viewangles.data[i] = SHORT2ANGLE( temp );
	}
}

//-------------------------------------------
void PM_AdjustAttackStates( pmove_t *pmove )
//-------------------------------------------
{
	int amount;

	if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&&pmove->ps->m_iVehicleNum ) { //riding a vehicle
		bgEntity_t *veh = pm_entVeh;
		if ( veh && veh->m_pVehicle && (veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER) ) {//riding a walker/fighter
			//not firing, ever
			pmove->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
			return;
		}
	}
	// get ammo usage
	if ( pmove->cmd.buttons & BUTTON_ALT_ATTACK ) {
		amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] - weaponData[pmove->ps->weapon].altEnergyPerShot;
	}
	else {
		amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] - weaponData[pmove->ps->weapon].energyPerShot;
	}

	// disruptor alt-fire should toggle the zoom mode, but only bother doing this for the player?
	if ( pmove->ps->weapon == WP_DISRUPTOR && pmove->ps->weaponstate == WEAPON_READY ) {

		if ( !(pmove->ps->eFlags & EF_ALT_FIRING) && (pmove->cmd.buttons & BUTTON_ALT_ATTACK) /*&&
			pm->cmd.upmove <= 0 && !pm->cmd.forwardmove && !pm->cmd.rightmove*/ ) {
			// We just pressed the alt-fire key
			if ( !pmove->ps->zoomMode && pmove->ps->pm_type != PM_DEAD ) {
				// not already zooming, so do it now
				pmove->ps->zoomMode = 1;
				pmove->ps->zoomLocked = qfalse;
				pmove->ps->zoomFov = 80.0f;//cg_fov.value;
				pmove->ps->zoomLockTime = pmove->cmd.serverTime + 50;
				PM_AddEvent( EV_DISRUPTOR_ZOOMSOUND );
			}
			else if ( pmove->ps->zoomMode == 1 && pmove->ps->zoomLockTime < pmove->cmd.serverTime ) { //check for == 1 so we can't turn binoculars off with disruptor alt fire
				// already zooming, so must be wanting to turn it off
				pmove->ps->zoomMode = 0;
				pmove->ps->zoomTime = pmove->ps->commandTime;
				pmove->ps->zoomLocked = qfalse;
				PM_AddEvent( EV_DISRUPTOR_ZOOMSOUND );
				pmove->ps->weaponTime = 1000;
			}
		}
		else if ( !(pmove->cmd.buttons & BUTTON_ALT_ATTACK) && pmove->ps->zoomLockTime < pmove->cmd.serverTime ) {
			// Not pressing zoom any more
			if ( pmove->ps->zoomMode ) {
				if ( pmove->ps->zoomMode == 1 && !pmove->ps->zoomLocked ) { //approximate what level the client should be zoomed at based on how long zoom was held
					pmove->ps->zoomFov = ((pmove->cmd.serverTime + 50) - pmove->ps->zoomLockTime) * 0.035f;
					if ( pmove->ps->zoomFov > 50 ) {
						pmove->ps->zoomFov = 50;
					}
					if ( pmove->ps->zoomFov < 1 ) {
						pmove->ps->zoomFov = 1;
					}
				}
				// were zooming in, so now lock the zoom
				pmove->ps->zoomLocked = qtrue;
			}
		}
		//This seemed like a good idea, but apparently it confuses people. So disabled for now.
		/*
		else if (!(pm->ps->eFlags & EF_ALT_FIRING) && (pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
		(pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove))
		{ //if you try to zoom while moving, just convert it into a primary attack
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
		pm->cmd.buttons |= BUTTON_ATTACK;
		}
		*/

		/*
		if (pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove)
		{
		if (pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
		{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
		pm->ps->zoomMode = 0;
		pm->ps->zoomTime = pm->ps->commandTime;
		pm->ps->zoomLocked = qfalse;
		PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
		}
		}
		*/

		if ( pmove->cmd.buttons & BUTTON_ATTACK ) {
			// If we are zoomed, we should switch the ammo usage to the alt-fire, otherwise, we'll
			//	just use whatever ammo was selected from above
			if ( pmove->ps->zoomMode ) {
				amount = pmove->ps->ammo[weaponData[pmove->ps->weapon].ammoIndex] -
					weaponData[pmove->ps->weapon].altEnergyPerShot;
			}
		}
		else {
			// alt-fire button pressing doesn't use any ammo
			amount = 0;
		}
	}
	/*
	else if (pm->ps->weapon == WP_DISRUPTOR) //still perform certain checks, even if the weapon is not ready
	{
	if (pm->cmd.upmove > 0 || pm->cmd.forwardmove || pm->cmd.rightmove)
	{
	if (pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime)
	{ //check for == 1 so we can't turn binoculars off with disruptor alt fire
	pm->ps->zoomMode = 0;
	pm->ps->zoomTime = pm->ps->commandTime;
	pm->ps->zoomLocked = qfalse;
	PM_AddEvent(EV_DISRUPTOR_ZOOMSOUND);
	}
	}
	}
	*/

	// set the firing flag for continuous beam weapons, saber will fire even if out of ammo
	if ( !(pmove->ps->pm_flags & PMF_RESPAWNED) &&
		pmove->ps->pm_type != PM_INTERMISSION &&
		pmove->ps->pm_type != PM_NOCLIP &&
		(pmove->cmd.buttons & (BUTTON_ATTACK | BUTTON_ALT_ATTACK)) &&
		(amount >= 0 || pmove->ps->weapon == WP_SABER) ) {
		if ( pmove->cmd.buttons & BUTTON_ALT_ATTACK ) {
			pmove->ps->eFlags |= EF_ALT_FIRING;
		}
		else {
			pmove->ps->eFlags &= ~EF_ALT_FIRING;
		}

		// This flag should always get set, even when alt-firing
		pmove->ps->eFlags |= EF_FIRING;
	}
	else {
		// Clear 'em out
		pmove->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
	}

	// disruptor should convert a main fire to an alt-fire if the gun is currently zoomed
	if ( pmove->ps->weapon == WP_DISRUPTOR ) {
		if ( pmove->cmd.buttons & BUTTON_ATTACK && pmove->ps->zoomMode == 1 && pmove->ps->zoomLocked ) {
			// converting the main fire to an alt-fire
			pmove->cmd.buttons |= BUTTON_ALT_ATTACK;
			pmove->ps->eFlags |= EF_ALT_FIRING;
		}
		else if ( pmove->cmd.buttons & BUTTON_ALT_ATTACK && pmove->ps->zoomMode == 1 && pmove->ps->zoomLocked ) {
			pmove->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pmove->ps->eFlags &= ~EF_ALT_FIRING;
		}
	}
}

void BG_CmdForRoll( playerState_t *ps, int anim, usercmd_t *pCmd ) {
	switch ( anim ) {
	case BOTH_ROLL_F:
		if ( !(JP_GetJPFixRoll() == 3 && (pCmd->forwardmove < 0)) )
			pCmd->forwardmove = 127;
		pCmd->rightmove = 0;
		break;

	case BOTH_ROLL_B:
		pCmd->forwardmove = -127;
		pCmd->rightmove = 0;
		break;

	case BOTH_ROLL_R:
		pCmd->forwardmove = 0;
		pCmd->rightmove = 127;
		break;

	case BOTH_ROLL_L:
		pCmd->forwardmove = 0;
		pCmd->rightmove = -127;
		break;

	case BOTH_GETUP_BROLL_R:
		pCmd->forwardmove = 0;
		pCmd->rightmove = 48;
		//NOTE: speed is 400
		break;

	case BOTH_GETUP_FROLL_R:
		// end of anim
		if ( ps->legsTimer <= 250 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			pCmd->forwardmove = 0;
			pCmd->rightmove = 48;
			//NOTE: speed is 400
		}
		break;

	case BOTH_GETUP_BROLL_L:
		pCmd->forwardmove = 0;
		pCmd->rightmove = -48;
		//NOTE: speed is 400
		break;

	case BOTH_GETUP_FROLL_L:
		// end of anim
		if ( ps->legsTimer <= 250 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			pCmd->forwardmove = 0;
			pCmd->rightmove = -48;
			//NOTE: speed is 400
		}
		break;

	case BOTH_GETUP_BROLL_B:
		// end of anim
		if ( ps->torsoTimer <= 250 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		// beginning of anim
		else if ( PM_AnimLength( 0, (animNumber_t)ps->legsAnim ) - ps->torsoTimer < 350 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			//FIXME: ramp down over length of anim
			pCmd->forwardmove = -64;
			pCmd->rightmove = 0;
			//NOTE: speed is 400
		}
		break;

	case BOTH_GETUP_FROLL_B:
		// end of anim
		if ( ps->torsoTimer <= 100 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		// beginning of anim
		else if ( PM_AnimLength( 0, (animNumber_t)ps->legsAnim ) - ps->torsoTimer < 200 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			//FIXME: ramp down over length of anim
			pCmd->forwardmove = -64;
			pCmd->rightmove = 0;
			//NOTE: speed is 400
		}
		break;

	case BOTH_GETUP_BROLL_F:
		// end of anim
		if ( ps->torsoTimer <= 550 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		// beginning of anim
		else if ( PM_AnimLength( 0, (animNumber_t)ps->legsAnim ) - ps->torsoTimer < 150 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			pCmd->forwardmove = 64;
			pCmd->rightmove = 0;
			//NOTE: speed is 400
		}
		break;

	case BOTH_GETUP_FROLL_F:
		// end of anim
		if ( ps->torsoTimer <= 100 )
			pCmd->forwardmove = pCmd->rightmove = 0;
		else {
			//FIXME: ramp down over length of anim
			pCmd->forwardmove = 64;
			pCmd->rightmove = 0;
			//NOTE: speed is 400
		}
		break;

	default:
		break;
	}
	pCmd->upmove = 0;
}

qboolean PM_SaberInTransition( int move );

void BG_AdjustClientSpeed( playerState_t *ps, usercmd_t *cmd, int svTime ) {
	saberInfo_t	*saber;

	if ( ps->clientNum >= MAX_CLIENTS ) {
		bgEntity_t *bgEnt = pm_entSelf;

		if ( bgEnt && bgEnt->s.NPC_class == CLASS_VEHICLE ) { //vehicles manage their own speed
			return;
		}
	}

	//For prediction, always reset speed back to the last known server base speed
	//If we didn't do this, under lag we'd eventually dwindle speed down to 0 even though
	//that would not be the correct predicted value.
	ps->speed = ps->basespeed;

	if ( ps->forceHandExtend == HANDEXTEND_DODGE ) {
		ps->speed = 0;
	}

	if ( ps->forceHandExtend == HANDEXTEND_KNOCKDOWN ||
		ps->forceHandExtend == HANDEXTEND_PRETHROWN ||
		ps->forceHandExtend == HANDEXTEND_POSTTHROWN ) {
		ps->speed = 0;
	}


	if ( cmd->forwardmove < 0 && !(cmd->buttons&BUTTON_WALKING) && pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//running backwards is slower than running forwards (like SP)
		ps->speed *= 0.75f;
	}

	if ( ps->fd.forcePowersActive & (1 << FP_GRIP) ) {
		ps->speed *= 0.4f;
	}

	if ( ps->fd.forcePowersActive & (1 << FP_SPEED) ) {
		ps->speed *= 1.7f;
	}
	else if ( ps->fd.forcePowersActive & (1 << FP_RAGE) ) {
		ps->speed *= 1.3f;
	}
	else if ( ps->fd.forceRageRecoveryTime > svTime ) {
		ps->speed *= 0.75f;
	}

	if ( pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1 && pm->ps->zoomLockTime < pm->cmd.serverTime ) {
		ps->speed *= 0.5f;
	}

	if ( ps->fd.forceGripCripple && pm->ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		if ( ps->fd.forcePowersActive & (1 << FP_RAGE) )
			ps->speed *= 0.9f;
		else if ( ps->fd.forcePowersActive & (1 << FP_SPEED) )
			ps->speed *= 0.8f;
		else
			ps->speed *= 0.2f;
	}

	if ( BG_SaberInAttack( ps->saberMove ) && cmd->forwardmove < 0 ) {//if running backwards while attacking, don't run as fast.
		switch ( ps->fd.saberAnimLevel ) {
		case FORCE_LEVEL_1:
			ps->speed *= 0.75f;
			break;
		case FORCE_LEVEL_2:
		case SS_DUAL:
		case SS_STAFF:
			ps->speed *= 0.60f;
			break;
		case FORCE_LEVEL_3:
			ps->speed *= 0.45f;
			break;
		default:
			break;
		}
	}

	else if ( BG_SpinningSaberAnim( ps->legsAnim ) ) {
		if ( ps->fd.saberAnimLevel == FORCE_LEVEL_3 ) {
			ps->speed *= 0.3f;
		}
		else {
			ps->speed *= 0.5f;
		}
	}
	else if ( ps->weapon == WP_SABER && BG_SaberInAttack( ps->saberMove ) ) {//if attacking with saber while running, drop your speed
		switch ( ps->fd.saberAnimLevel ) {
		case FORCE_LEVEL_2:
		case SS_DUAL:
		case SS_STAFF:
			ps->speed *= 0.85f;
			break;
		case FORCE_LEVEL_3:
			ps->speed *= 0.55f;
			break;
		default:
			break;
		}
	}
	else if ( ps->weapon == WP_SABER && ps->fd.saberAnimLevel == FORCE_LEVEL_3 &&
		PM_SaberInTransition( ps->saberMove ) ) { //Now, we want to even slow down in transitions for level 3 (since it has chains and stuff now)
		if ( cmd->forwardmove < 0 ) {
			ps->speed *= 0.4f;
		}
		else {
			ps->speed *= 0.6f;
		}
	}

	//Raz: JK2 rolls
#if 0
#ifdef _GAME
	if ( 0 )//( dmflags.integer & DF_JK2_ROLL )
#else
	if ( 0 )//( cgs.dmflags & DF_JK2_ROLL )
#endif
	{
		if ( BG_InRoll( ps, ps->legsAnim )  )
		{
			BG_CmdForRoll( ps, ps->legsAnim, cmd );
			if ( ps->speed > 50 )
			{ //can't roll unless you're able to move normally
				if ((ps->legsAnim) == BOTH_ROLL_B)
					ps->speed = ps->legsTimer/2.5f;//450;
				else
					ps->speed = ps->legsTimer/1.5f;//450;
				if (ps->speed > 600)
					ps->speed = 600;
				//Automatically slow down as the roll ends.
			}
		}
	}
	else if ( BG_InRoll( ps, ps->legsAnim ) && ps->speed > 50 )
#endif
		if ( BG_InRoll( ps, ps->legsAnim ) && ps->speed > 50.0f ) { //can't roll unless you're able to move normally
			if ( (ps->legsAnim) == BOTH_ROLL_B ) { //backwards roll is pretty fast, should also be slower
				if ( ps->legsTimer > 800 || (JP_GetJPFixRoll() == 3 && ps->legsTimer > 100) )
					ps->speed = ps->legsTimer / 2.5f;
				else
					ps->speed = ps->legsTimer / 6.0f;//450;
			}
			else {
				if ( ps->legsTimer <= 800 ) {
					if ( JP_GetJPFixRoll() == 3 ) {
						if ( ps->legsTimer <= 100 ) {
							if ( (pm->cmd.forwardmove < 0) ) {
								ps->speed = ps->legsTimer / 20.0f;
							}
							else {
								ps->speed = ps->legsTimer / 5.0f;//450;
							}
						}
						else {
							if ( pm->cmd.forwardmove < 0 )
								ps->speed = ps->legsTimer / 20.0f;
							else
								ps->speed = ps->legsTimer / 1.5f;
						}
					}
					else {
						ps->speed = ps->legsTimer / 5.0f;
					}
				}
				else {
					ps->speed = ps->legsTimer / 1.5f;
				}
				//	ps->speed = ps->legsTimer/5.0f;
			}
			if ( JP_GetJPFixRoll() < 3 && ps->speed > 600.0f )
				ps->speed = 600.0f;
			//Automatically slow down as the roll ends.
		}

	saber = BG_MySaber( ps->clientNum, 0 );
	if ( saber && saber->moveSpeedScale != 1.0f )
		ps->speed *= saber->moveSpeedScale;
	saber = BG_MySaber( ps->clientNum, 1 );
	if ( saber && saber->moveSpeedScale != 1.0f )
		ps->speed *= saber->moveSpeedScale;
}

qboolean BG_InRollAnim( entityState_t *cent ) {
	switch ( cent->legsAnim ) {
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean BG_InKnockDown( int anim ) {
	switch ( anim ) {
	case BOTH_KNOCKDOWN1:
	case BOTH_KNOCKDOWN2:
	case BOTH_KNOCKDOWN3:
	case BOTH_KNOCKDOWN4:
	case BOTH_KNOCKDOWN5:
		return qtrue;

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
		return qtrue;

	default:
		return qfalse;
	}
}

qboolean BG_InRollES( entityState_t *ps, int anim ) {
	switch ( anim ) {
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:
		return qfalse;
	}
}

void BG_IK_MoveArm( void *ghoul2, int lHandBolt, int time, entityState_t *ent, int basePose, vector3 *desiredPos, qboolean *ikInProgress,
	vector3 *origin, vector3 *angles, vector3 *scale, int blendTime, qboolean forceHalt ) {
	mdxaBone_t lHandMatrix;
	vector3 lHand;
	vector3 torg;
	float distToDest;

	if ( !ghoul2 ) {
		return;
	}

	assert( bgHumanoidAnimations[basePose].firstFrame > 0 );

	if ( !*ikInProgress && !forceHalt ) {
		int baseposeAnim = basePose;
		sharedSetBoneIKStateParams_t ikP;

		//restrict the shoulder joint
		//VectorSet(ikP.pcjMins,-50.0f,-80.0f,-15.0f);
		//VectorSet(ikP.pcjMaxs,15.0f,40.0f,15.0f);

		//for now, leaving it unrestricted, but restricting elbow joint.
		//This lets us break the arm however we want in order to fling people
		//in throws, and doesn't look bad.
		VectorSet( &ikP.pcjMins, 0, 0, 0 );
		VectorSet( &ikP.pcjMaxs, 0, 0, 0 );

		//give the info on our entity.
		ikP.blendTime = blendTime;
		VectorCopy( origin, &ikP.origin );
		VectorCopy( angles, &ikP.angles );
		ikP.angles.pitch = 0;
		ikP.pcjOverrides = 0;
		ikP.radius = 10.0f;
		VectorCopy( scale, &ikP.scale );

		//base pose frames for the limb
		ikP.startFrame = bgHumanoidAnimations[baseposeAnim].firstFrame + bgHumanoidAnimations[baseposeAnim].numFrames;
		ikP.endFrame = bgHumanoidAnimations[baseposeAnim].firstFrame + bgHumanoidAnimations[baseposeAnim].numFrames;

		ikP.forceAnimOnBone = qfalse; //let it use existing anim if it's the same as this one.

		//we want to call with a null bone name first. This will init all of the
		//ik system stuff on the g2 instance, because we need ragdoll effectors
		//in order for our pcj's to know how to angle properly.
		if ( !trap->G2API_SetBoneIKState( ghoul2, time, NULL, IKS_DYNAMIC, &ikP ) ) {
			assert( !"Failed to init IK system for g2 instance!" );
		}

		//Now, create our IK bone state.
		if ( trap->G2API_SetBoneIKState( ghoul2, time, "lhumerus", IKS_DYNAMIC, &ikP ) ) {
			//restrict the elbow joint
			VectorSet( &ikP.pcjMins, -90.0f, -20.0f, -20.0f );
			VectorSet( &ikP.pcjMaxs, 30.0f, 20.0f, -20.0f );

			if ( trap->G2API_SetBoneIKState( ghoul2, time, "lradius", IKS_DYNAMIC, &ikP ) ) { //everything went alright.
				*ikInProgress = qtrue;
			}
		}
	}

	if ( *ikInProgress && !forceHalt ) { //actively update our ik state.
		sharedIKMoveParams_t ikM;
		sharedRagDollUpdateParams_t tuParms;
		vector3 tAngles;

		//set the argument struct up
		VectorCopy( desiredPos, &ikM.desiredOrigin ); //we want the bone to move here.. if possible

		VectorCopy( angles, &tAngles );
		tAngles.pitch = tAngles.roll = 0;

		trap->G2API_GetBoltMatrix( ghoul2, 0, lHandBolt, &lHandMatrix, &tAngles, origin, time, 0, scale );
		//Get the point position from the matrix.
		lHand.x = lHandMatrix.matrix[0][3];
		lHand.y = lHandMatrix.matrix[1][3];
		lHand.z = lHandMatrix.matrix[2][3];

		VectorSubtract( &lHand, desiredPos, &torg );
		distToDest = VectorLength( &torg );

		//closer we are, more we want to keep updated.
		//if we're far away we don't want to be too fast or we'll start twitching all over.
		if ( distToDest < 2 ) { //however if we're this close we want very precise movement
			ikM.movementSpeed = 0.4f;
		}
		else if ( distToDest < 16 ) {
			ikM.movementSpeed = 0.9f;//8.0f;
		}
		else if ( distToDest < 32 ) {
			ikM.movementSpeed = 0.8f;//4.0f;
		}
		else if ( distToDest < 64 ) {
			ikM.movementSpeed = 0.7f;//2.0f;
		}
		else {
			ikM.movementSpeed = 0.6f;
		}
		VectorCopy( origin, &ikM.origin ); //our position in the world.

		ikM.boneName[0] = 0;
		if ( trap->G2API_IKMove( ghoul2, time, &ikM ) ) {
			//now do the standard model animate stuff with ragdoll update params.
			VectorCopy( angles, &tuParms.angles );
			tuParms.angles.pitch = 0;

			VectorCopy( origin, &tuParms.position );
			VectorCopy( scale, &tuParms.scale );

			tuParms.me = ent->number;
			VectorClear( &tuParms.velocity );

			trap->G2API_AnimateG2Models( ghoul2, time, &tuParms );
		}
		else {
			*ikInProgress = qfalse;
		}
	}
	else if ( *ikInProgress ) { //kill it
		float cFrame, animSpeed;
		int sFrame, eFrame;
		uint32_t flags;

		trap->G2API_SetBoneIKState( ghoul2, time, "lhumerus", IKS_NONE, NULL );
		trap->G2API_SetBoneIKState( ghoul2, time, "lradius", IKS_NONE, NULL );

		//then reset the angles/anims on these PCJs
		trap->G2API_SetBoneAngles( ghoul2, 0, "lhumerus", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );
		trap->G2API_SetBoneAngles( ghoul2, 0, "lradius", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, time );

		//Get the anim/frames that the pelvis is on exactly, and match the left arm back up with them again.
		trap->G2API_GetBoneAnim( ghoul2, "pelvis", (const int)time, &cFrame, &sFrame, &eFrame, &flags, &animSpeed, 0, 0 );
		trap->G2API_SetBoneAnim( ghoul2, 0, "lhumerus", sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );
		trap->G2API_SetBoneAnim( ghoul2, 0, "lradius", sFrame, eFrame, flags, animSpeed, time, sFrame, 300 );

		//And finally, get rid of all the ik state effector data by calling with null bone name (similar to how we init it).
		trap->G2API_SetBoneIKState( ghoul2, time, NULL, IKS_NONE, NULL );

		*ikInProgress = qfalse;
	}
}

//Adjust the head/neck desired angles
void BG_UpdateLookAngles( int lookingDebounceTime, vector3 *lastHeadAngles, int time, vector3 *lookAngles, float lookSpeed, float minPitch, float maxPitch, float minYaw, float maxYaw, float minRoll, float maxRoll ) {
	static const float fFrameInter = 0.1f;
	static vector3 oldLookAngles;
	static vector3 lookAnglesDiff;
	static int ang;

	if ( lookingDebounceTime > time ) {
		//clamp so don't get "Exorcist" effect
		if ( lookAngles->pitch > maxPitch ) {
			lookAngles->pitch = maxPitch;
		}
		else if ( lookAngles->pitch < minPitch ) {
			lookAngles->pitch = minPitch;
		}
		if ( lookAngles->yaw > maxYaw ) {
			lookAngles->yaw = maxYaw;
		}
		else if ( lookAngles->yaw < minYaw ) {
			lookAngles->yaw = minYaw;
		}
		if ( lookAngles->roll > maxRoll ) {
			lookAngles->roll = maxRoll;
		}
		else if ( lookAngles->roll < minRoll ) {
			lookAngles->roll = minRoll;
		}

		//slowly lerp to this new value
		//Remember last headAngles
		VectorCopy( lastHeadAngles, &oldLookAngles );
		VectorSubtract( lookAngles, &oldLookAngles, &lookAnglesDiff );

		for ( ang = 0; ang < 3; ang++ ) {
			lookAnglesDiff.data[ang] = AngleNormalize180( lookAnglesDiff.data[ang] );
		}

		if ( VectorLengthSquared( &lookAnglesDiff ) ) {
			lookAngles->pitch = AngleNormalize180( oldLookAngles.pitch + (lookAnglesDiff.pitch * fFrameInter*lookSpeed) );
			lookAngles->yaw = AngleNormalize180( oldLookAngles.yaw + (lookAnglesDiff.yaw * fFrameInter*lookSpeed) );
			lookAngles->roll = AngleNormalize180( oldLookAngles.roll + (lookAnglesDiff.roll * fFrameInter*lookSpeed) );
		}
	}
	//Remember current lookAngles next time
	VectorCopy( lookAngles, lastHeadAngles );
}

//for setting visual look (headturn) angles
static void BG_G2ClientNeckAngles( void *ghoul2, int time, const vector3 *lookAngles, vector3 *headAngles, vector3 *neckAngles, vector3 *thoracicAngles, vector3 *headClampMinAngles, vector3 *headClampMaxAngles, entityState_t *cent ) {
	vector3	lA;
	VectorCopy( lookAngles, &lA );
	//clamp the headangles (which should now be relative to the cervical (neck) angles
	if ( lA.pitch < headClampMinAngles->pitch ) {
		lA.pitch = headClampMinAngles->pitch;
	}
	else if ( lA.pitch > headClampMaxAngles->pitch ) {
		lA.pitch = headClampMaxAngles->pitch;
	}

	if ( lA.yaw < headClampMinAngles->yaw ) {
		lA.yaw = headClampMinAngles->yaw;
	}
	else if ( lA.yaw > headClampMaxAngles->yaw ) {
		lA.yaw = headClampMaxAngles->yaw;
	}

	if ( lA.roll < headClampMinAngles->roll ) {
		lA.roll = headClampMinAngles->roll;
	}
	else if ( lA.roll > headClampMaxAngles->roll ) {
		lA.roll = headClampMaxAngles->roll;
	}

	//split it up between the neck and cranium
	if ( BG_InLedgeMove( cent->legsAnim ) ) {//lock arm parent bone to animation
		thoracicAngles->pitch = 0;
	}
	else if ( thoracicAngles->pitch ) {//already been set above, blend them
		thoracicAngles->pitch = (thoracicAngles->pitch + (lA.pitch * 0.4f)) * 0.5f;
	}
	else {
		thoracicAngles->pitch = lA.pitch * 0.4f;
	}
	if ( thoracicAngles->yaw ) {//already been set above, blend them
		thoracicAngles->yaw = (thoracicAngles->yaw + (lA.yaw * 0.1f)) * 0.5f;
	}
	else {
		thoracicAngles->yaw = lA.yaw * 0.1f;
	}
	if ( thoracicAngles->roll ) {//already been set above, blend them
		thoracicAngles->roll = (thoracicAngles->roll + (lA.roll * 0.1f)) * 0.5f;
	}
	else {
		thoracicAngles->roll = lA.roll * 0.1f;
	}

	if ( BG_InLedgeMove( cent->legsAnim ) ) {//lock the neckAngles to prevent the head from acting weird
		VectorClear( neckAngles );
		VectorClear( headAngles );
	}
	else {
		neckAngles->pitch = lA.pitch * 0.2f;
		neckAngles->yaw = lA.yaw * 0.3f;
		neckAngles->roll = lA.roll * 0.3f;

		headAngles->pitch = lA.pitch * 0.4f;
		headAngles->yaw = lA.yaw * 0.6f;
		headAngles->roll = lA.roll * 0.6f;
	}

	/* //non-applicable SP code
	if ( G_RidingVehicle( cent->gent ) )// && type == VH_SPEEDER ?
	{//aim torso forward too
	headAngles->yaw = neckAngles->yaw = thoracicAngles->yaw = 0;
	}
	*/

	trap->G2API_SetBoneAngles( ghoul2, 0, "cranium", headAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
	trap->G2API_SetBoneAngles( ghoul2, 0, "cervical", neckAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
	trap->G2API_SetBoneAngles( ghoul2, 0, "thoracic", thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
}

//rww - Finally decided to convert all this stuff to BG form.
static void BG_G2ClientSpineAngles( void *ghoul2, int motionBolt, vector3 *cent_lerpOrigin, vector3 *cent_lerpAngles, entityState_t *cent,
	int time, vector3 *viewAngles, int ciLegs, int ciTorso, const vector3 *angles, vector3 *thoracicAngles,
	vector3 *ulAngles, vector3 *llAngles, vector3 *modelScale, float *tPitchAngle, float *tYawAngle, int *corrTime ) {
	qboolean doCorr = qfalse;

	//*tPitchAngle = viewAngles->pitch;
	viewAngles->yaw = AngleDelta( cent_lerpAngles->yaw, angles->yaw );
	//*tYawAngle = viewAngles->yaw;

#if 1
	if ( !BG_FlippingAnim( cent->legsAnim ) &&
		!BG_SpinningSaberAnim( cent->legsAnim ) &&
		!BG_SpinningSaberAnim( cent->torsoAnim ) &&
		!BG_InSpecialJump( cent->legsAnim ) &&
		!BG_InSpecialJump( cent->torsoAnim ) &&
		!BG_InDeathAnim( cent->legsAnim ) &&
		!BG_InDeathAnim( cent->torsoAnim ) &&
		!BG_InRollES( cent, cent->legsAnim ) &&
		!BG_InRollAnim( cent ) &&
		!BG_SaberInSpecial( cent->saberMove ) &&
		!BG_SaberInSpecialAttack( cent->torsoAnim ) &&
		!BG_SaberInSpecialAttack( cent->legsAnim ) &&

		!BG_InKnockDown( cent->torsoAnim ) &&
		!BG_InKnockDown( cent->legsAnim ) &&
		!BG_InKnockDown( ciTorso ) &&
		!BG_InKnockDown( ciLegs ) &&

		!BG_FlippingAnim( ciLegs ) &&
		!BG_SpinningSaberAnim( ciLegs ) &&
		!BG_SpinningSaberAnim( ciTorso ) &&
		!BG_InSpecialJump( ciLegs ) &&
		!BG_InSpecialJump( ciTorso ) &&
		!BG_InDeathAnim( ciLegs ) &&
		!BG_InDeathAnim( ciTorso ) &&
		!BG_SaberInSpecialAttack( ciTorso ) &&
		!BG_SaberInSpecialAttack( ciLegs ) &&

		!(cent->eFlags & EF_DEAD) &&
		(cent->legsAnim) != (cent->torsoAnim) &&
		(ciLegs) != (ciTorso) &&
		!cent->m_iVehicleNum ) {
		doCorr = qtrue;
	}
#else
	if ( ((!BG_FlippingAnim( cent->legsAnim )
		&& !BG_SpinningSaberAnim( cent->legsAnim )
		&& !BG_SpinningSaberAnim( cent->torsoAnim )
		&& (cent->legsAnim) != (cent->torsoAnim)) //NOTE: presumes your legs & torso are on the same frame, though they *should* be because PM_SetAnimFinal tries to keep them in synch
		||
		(!BG_FlippingAnim( ciLegs )
		&& !BG_SpinningSaberAnim( ciLegs )
		&& !BG_SpinningSaberAnim( ciTorso )
		&& (ciLegs) != (ciTorso)))
		||
		ciLegs != cent->legsAnim
		||
		ciTorso != cent->torsoAnim)
	{
		doCorr = qtrue;
		*corrTime = time + 1000; //continue correcting for a second after to smooth things out. SP doesn't need this for whatever reason but I can't find a way around it.
	}
	else if (*corrTime >= time)
	{
		if (!BG_FlippingAnim( cent->legsAnim )
			&& !BG_SpinningSaberAnim( cent->legsAnim )
			&& !BG_SpinningSaberAnim( cent->torsoAnim )
			&& !BG_FlippingAnim( ciLegs )
			&& !BG_SpinningSaberAnim( ciLegs )
			&& !BG_SpinningSaberAnim( ciTorso ))
		{
			doCorr = qtrue;
		}
	}
#endif

	if ( doCorr ) {//FIXME: no need to do this if legs and torso on are same frame
		//adjust for motion offset
		mdxaBone_t	boltMatrix;
		vector3		motionFwd, motionAngles;
		vector3		motionRt, tempAng;
		int			ang;

		trap->G2API_GetBoltMatrix_NoRecNoRot( ghoul2, 0, motionBolt, &boltMatrix, &vec3_origin, cent_lerpOrigin, time, 0, modelScale );
		//BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, motionFwd );
		motionFwd.x = -boltMatrix.matrix[0][1];
		motionFwd.y = -boltMatrix.matrix[1][1];
		motionFwd.z = -boltMatrix.matrix[2][1];

		vectoangles( &motionFwd, &motionAngles );

		//BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, motionRt );
		motionRt.x = -boltMatrix.matrix[0][0];
		motionRt.y = -boltMatrix.matrix[1][0];
		motionRt.z = -boltMatrix.matrix[2][0];

		vectoangles( &motionRt, &tempAng );
		motionAngles.roll = -tempAng.pitch;

		for ( ang = 0; ang < 3; ang++ ) {
			viewAngles->data[ang] = AngleNormalize180( viewAngles->data[ang] - AngleNormalize180( motionAngles.data[ang] ) );
		}
	}

	//distribute the angles differently up the spine
	//NOTE: each of these distributions must add up to 1.0f
	if ( BG_InLedgeMove( cent->legsAnim ) ) {//lock spine to animation
		thoracicAngles->pitch = 0;
		llAngles->pitch = 0;
		ulAngles->pitch = 0;
	}
	else {
		thoracicAngles->pitch = viewAngles->pitch*0.20f;
		llAngles->pitch = viewAngles->pitch*0.40f;
		ulAngles->pitch = viewAngles->pitch*0.40f;
	}

	thoracicAngles->yaw = viewAngles->yaw*0.20f;
	ulAngles->yaw = viewAngles->yaw*0.35f;
	llAngles->yaw = viewAngles->yaw*0.45f;

	thoracicAngles->roll = viewAngles->roll*0.20f;
	ulAngles->roll = viewAngles->roll*0.35f;
	llAngles->roll = viewAngles->roll*0.45f;
}

/*
==================
CG_SwingAngles
==================
*/
static float BG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
	float speed, float *angle, qboolean *swinging, int frametime ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return 0;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabsf( swing );
	if ( scale < swingTolerance * 0.5f ) {
		scale = 0.5f;
	}
	else if ( scale < swingTolerance ) {
		scale = 1.0f;
	}
	else {
		scale = 2.0f;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}
	else if ( swing < 0 ) {
		move = frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	}
	else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}

	return swing;
}

//#define BONE_BASED_LEG_ANGLES

//I apologize for this function
qboolean BG_InRoll2( entityState_t *es ) {
	switch ( es->legsAnim ) {
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	default:
		return qfalse;
	}
}


extern qboolean BG_SaberLockBreakAnim( int anim ); //bg_panimate.c
void BG_G2PlayerAngles( void *ghoul2, int motionBolt, entityState_t *cent, int time, vector3 *cent_lerpOrigin,
	vector3 *cent_lerpAngles, vector3 legs[3], vector3 *legsAngles, qboolean *tYawing, qboolean *tPitching,
	qboolean *lYawing, float *tYawAngle, float *tPitchAngle, float *lYawAngle, int frametime, vector3 *turAngles,
	vector3 *modelScale, int ciLegs, int ciTorso, int *corrTime, vector3 *lookAngles, vector3 *lastHeadAngles,
	int lookTime, entityState_t *emplaced, int *crazySmoothFactor ) {
	int adddir = 0;
	float degrees_negative = 0, degrees_positive = 0;
	static int dir, i;
	static float dif, dest, speed;
	static const float lookSpeed = 1.5f;
#ifdef BONE_BASED_LEG_ANGLES
	static float legBoneYaw;
#endif
	static vector3 eyeAngles, neckAngles, velocity, torsoAngles, headAngles, velPos, velAng, ulAngles, llAngles,
		viewAngles, angles, thoracicAngles = { 0, 0, 0 }, headClampMinAngles = { -25, -55, -10 },
		headClampMaxAngles = { 50, 50, 10 };

	if ( cent->m_iVehicleNum || cent->forceFrame || BG_SaberLockBreakAnim( cent->legsAnim ) || BG_SaberLockBreakAnim( cent->torsoAnim ) ) { //a vehicle or riding a vehicle - in either case we don't need to be in here
		vector3 forcedAngles;

		VectorClear( &forcedAngles );
		forcedAngles.yaw = cent_lerpAngles->yaw;
		forcedAngles.roll = cent_lerpAngles->roll;
		AnglesToAxis( &forcedAngles, legs );
		VectorCopy( &forcedAngles, legsAngles );
		VectorCopy( legsAngles, turAngles );

		if ( cent->number < MAX_CLIENTS ) {
			trap->G2API_SetBoneAngles( ghoul2, 0, "lower_lumbar", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			trap->G2API_SetBoneAngles( ghoul2, 0, "upper_lumbar", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			trap->G2API_SetBoneAngles( ghoul2, 0, "cranium", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			trap->G2API_SetBoneAngles( ghoul2, 0, "thoracic", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			trap->G2API_SetBoneAngles( ghoul2, 0, "cervical", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
		}
		return;
	}

	if ( (time + 2000) < *corrTime ) {
		*corrTime = 0;
	}

	VectorCopy( cent_lerpAngles, &headAngles );
	headAngles.yaw = AngleMod( headAngles.yaw );
	VectorClear( legsAngles );
	VectorClear( &torsoAngles );
	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ((cent->legsAnim) != BOTH_STAND1) ||
		(cent->torsoAnim) != WeaponReadyAnim[cent->weapon] ) {
		// if not standing still, always point all in the same direction
		//cent->pe.torso.yawing = qtrue;	// always center
		*tYawing = qtrue;
		//cent->pe.torso.pitching = qtrue;	// always center
		*tPitching = qtrue;
		//cent->pe.legs.yawing = qtrue;	// always center
		*lYawing = qtrue;
	}

	// adjust legs for movement dir
	if ( cent->eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	}
	else {
		dir = cent->angles2.yaw;
		if ( dir < 0 || dir > 7 ) {
			Com_Error( ERR_DROP, "Bad player movement angle (%i)", dir );
		}
	}

	torsoAngles.yaw = headAngles.yaw;

	//for now, turn torso instantly and let the legs swing to follow
	*tYawAngle = torsoAngles.yaw;

	// --------- pitch -------------

	VectorCopy( &cent->pos.trDelta, &velocity );

	if ( BG_InRoll2( cent ) ) { //don't affect angles based on vel then
		VectorClear( &velocity );
	}
	else if ( cent->weapon == WP_SABER &&
		BG_SaberInSpecial( cent->saberMove ) ) {
		VectorClear( &velocity );
	}

	speed = VectorNormalize( &velocity );

	if ( !speed ) {
		torsoAngles.yaw = headAngles.yaw;
	}

	// only show a fraction of the pitch angle in the torso
	if ( headAngles.pitch > 180 ) {
		dest = (-360 + headAngles.pitch) * 0.75f;
	}
	else {
		dest = headAngles.pitch * 0.75f;
	}

	if ( cent->m_iVehicleNum ) { //swing instantly on vehicles
		*tPitchAngle = dest;
	}
	else {
		BG_SwingAngles( dest, 15, 30, 0.1f, tPitchAngle, tPitching, frametime );
	}
	torsoAngles.pitch = *tPitchAngle;

	// --------- roll -------------

	if ( speed ) {
		vector3	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( &velocity, &axis[1] );
		legsAngles->roll -= side;

		side = speed * DotProduct( &velocity, &axis[0] );
		legsAngles->pitch += side;
	}

	//legsAngles->yaw = headAngles.yaw + (movementOffsets[ dir ]*speed_dif);

	//rww - crazy velocity-based leg angle calculation
	legsAngles->yaw = headAngles.yaw;
	velPos.x = cent_lerpOrigin->x + velocity.x;
	velPos.y = cent_lerpOrigin->y + velocity.y;
	velPos.z = cent_lerpOrigin->z;// + velocity[2];

	if ( cent->groundEntityNum == ENTITYNUM_NONE ||
		cent->forceFrame ||
		(cent->weapon == WP_EMPLACED_GUN && emplaced) ) { //off the ground, no direction-based leg angles (same if in saberlock)
		VectorCopy( cent_lerpOrigin, &velPos );
	}

	VectorSubtract( cent_lerpOrigin, &velPos, &velAng );

	if ( !VectorCompare( &velAng, &vec3_origin ) ) {
		vectoangles( &velAng, &velAng );

		if ( velAng.yaw <= legsAngles->yaw ) {
			degrees_negative = (legsAngles->yaw - velAng.yaw);
			degrees_positive = (360 - legsAngles->yaw) + velAng.yaw;
		}
		else {
			degrees_negative = legsAngles->yaw + (360 - velAng.yaw);
			degrees_positive = (velAng.yaw - legsAngles->yaw);
		}

		if ( degrees_negative < degrees_positive ) {
			dif = degrees_negative;
			adddir = 0;
		}
		else {
			dif = degrees_positive;
			adddir = 1;
		}

		if ( dif > 90 ) {
			dif = (180 - dif);
		}

		if ( dif > 60 ) {
			dif = 60;
		}

		//Slight hack for when playing is running backward
		if ( dir == 3 || dir == 5 ) {
			dif = -dif;
		}

		if ( adddir ) {
			legsAngles->yaw -= dif;
		}
		else {
			legsAngles->yaw += dif;
		}
	}

	if ( cent->m_iVehicleNum ) { //swing instantly on vehicles
		*lYawAngle = legsAngles->yaw;
	}
	else {
		BG_SwingAngles( legsAngles->yaw, /*40*/0, 90, 0.65f, lYawAngle, lYawing, frametime );
	}
	legsAngles->yaw = *lYawAngle;

	/*
	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );
	*/

	legsAngles->roll = 0;
	torsoAngles.roll = 0;

	//	VectorCopy(legsAngles, turAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( &headAngles, &torsoAngles, &headAngles );
	AnglesSubtract( &torsoAngles, legsAngles, &torsoAngles );

	legsAngles->pitch = 0;

	if ( cent->heldByClient ) { //keep the base angles clear when doing the IK stuff, it doesn't compensate for it.
		//rwwFIXMEFIXME: Store leg angles off and add them to all the fed in angles for G2 functions?
		VectorClear( legsAngles );
		legsAngles->yaw = cent_lerpAngles->yaw;
	}

#ifdef BONE_BASED_LEG_ANGLES
	legBoneYaw = legsAngles.yaw;
	VectorClear(legsAngles);
	legsAngles.yaw = cent_lerpAngles.yaw;
#endif

	VectorCopy( legsAngles, turAngles );

	AnglesToAxis( legsAngles, legs );

	VectorCopy( cent_lerpAngles, &viewAngles );
	viewAngles.yaw = viewAngles.roll = 0;
	viewAngles.pitch *= 0.5f;

	VectorSet( &angles, 0, legsAngles->yaw, 0 );

	angles.pitch = legsAngles->pitch;
	if ( angles.pitch > 30 ) {
		angles.pitch = 30;
	}
	else if ( angles.pitch < -30 ) {
		angles.pitch = -30;
	}

	if ( cent->weapon == WP_EMPLACED_GUN &&
		emplaced ) { //if using an emplaced gun, then we want to make sure we're angled to "hold" it right
		vector3 facingAngles;

		VectorSubtract( &emplaced->pos.trBase, cent_lerpOrigin, &facingAngles );
		vectoangles( &facingAngles, &facingAngles );

		if ( emplaced->weapon == WP_NONE ) { //e-web
			VectorCopy( &facingAngles, legsAngles );
			AnglesToAxis( legsAngles, legs );
		}
		else { //misc emplaced
			float emplacedDif = AngleSubtract( cent_lerpAngles->yaw, facingAngles.yaw );

			/*
			if (emplaced->weapon == WP_NONE)
			{ //offset is a little bit different for the e-web
			emplacedDif -= 16.0f;
			}
			*/

			VectorSet( &facingAngles, -16.0f, -emplacedDif, 0.0f );

			if ( cent->legsAnim == BOTH_STRAFE_LEFT1 || cent->legsAnim == BOTH_STRAFE_RIGHT1 ) { //try to adjust so it doesn't look wrong
				if ( crazySmoothFactor ) { //want to smooth a lot during this because it chops around and looks like ass
					*crazySmoothFactor = time + 1000;
				}

				BG_G2ClientSpineAngles( ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time, &viewAngles, ciLegs, ciTorso, &angles, &thoracicAngles, &ulAngles, &llAngles, modelScale, tPitchAngle, tYawAngle, corrTime );
				trap->G2API_SetBoneAngles( ghoul2, 0, "lower_lumbar", &llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
				trap->G2API_SetBoneAngles( ghoul2, 0, "upper_lumbar", &ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
				trap->G2API_SetBoneAngles( ghoul2, 0, "cranium", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );

				VectorAdd( &facingAngles, &thoracicAngles, &facingAngles );

				if ( cent->legsAnim == BOTH_STRAFE_LEFT1 ) { //this one needs some further correction
					facingAngles.yaw -= 32.0f;
				}
			}
			else {
				//trap->G2API_SetBoneAngles(ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				//trap->G2API_SetBoneAngles(ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
				trap->G2API_SetBoneAngles( ghoul2, 0, "cranium", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			}

			VectorScale( &facingAngles, 0.6f, &facingAngles );	trap->G2API_SetBoneAngles( ghoul2, 0, "lower_lumbar", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			VectorScale( &facingAngles, 0.8f, &facingAngles );	trap->G2API_SetBoneAngles( ghoul2, 0, "upper_lumbar", &facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
			VectorScale( &facingAngles, 0.8f, &facingAngles );	trap->G2API_SetBoneAngles( ghoul2, 0, "thoracic", &facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );

			//Now we want the head angled toward where we are facing
			VectorSet( &facingAngles, 0.0f, emplacedDif, 0.0f );
			VectorScale( &facingAngles, 0.6f, &facingAngles );
			trap->G2API_SetBoneAngles( ghoul2, 0, "cervical", &facingAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );

			return; //don't have to bother with the rest then
		}
	}

	BG_G2ClientSpineAngles( ghoul2, motionBolt, cent_lerpOrigin, cent_lerpAngles, cent, time,
		&viewAngles, ciLegs, ciTorso, &angles, &thoracicAngles, &ulAngles, &llAngles, modelScale,
		tPitchAngle, tYawAngle, corrTime );

	VectorCopy( cent_lerpAngles, &eyeAngles );

	for ( i = 0; i < 3; i++ ) {
		lookAngles->data[i] = AngleNormalize180( lookAngles->data[i] );
		eyeAngles.data[i] = AngleNormalize180( eyeAngles.data[i] );
	}
	AnglesSubtract( lookAngles, &eyeAngles, lookAngles );

	BG_UpdateLookAngles( lookTime, lastHeadAngles, time, lookAngles, lookSpeed, -50.0f, 50.0f, -70.0f, 70.0f, -30.0f, 30.0f );

	//added NPC_class input to allow for different spine handling for some of the NPCs
	BG_G2ClientNeckAngles( ghoul2, time, lookAngles, &headAngles, &neckAngles, &thoracicAngles, &headClampMinAngles, &headClampMaxAngles, cent );

#ifdef BONE_BASED_LEG_ANGLES
	{
		vector3 bLAngles;
		VectorClear(bLAngles);
		bLAngles.roll = AngleNormalize180((legBoneYaw - cent_lerpAngles.yaw));
		trap->G2API_SetBoneAngles(ghoul2, 0, "model_root", bLAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);

		if (!llAngles.yaw)
		{
			llAngles.yaw -= bLAngles.roll;
		}
	}
#endif
	trap->G2API_SetBoneAngles( ghoul2, 0, "lower_lumbar", &llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
	trap->G2API_SetBoneAngles( ghoul2, 0, "upper_lumbar", &ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
	trap->G2API_SetBoneAngles( ghoul2, 0, "thoracic", &thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
	//trap->G2API_SetBoneAngles(ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time);
}

void BG_G2ATSTAngles( void *ghoul2, int time, vector3 *cent_lerpAngles ) {//																							up			right		fwd
	trap->G2API_SetBoneAngles( ghoul2, 0, "thoracic", cent_lerpAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, 0, 0, time );
}

static qboolean PM_AdjustAnglesForDualJumpAttack( playerState_t *ps, usercmd_t *ucmd ) {
	//ucmd->angles.pitch = ANGLE2SHORT( ps->viewangles.pitch ) - ps->delta_angles.pitch;
	//ucmd->angles.yaw = ANGLE2SHORT( ps->viewangles.yaw ) - ps->delta_angles.yaw;
	return qtrue;
}

static void PM_CmdForSaberMoves( usercmd_t *ucmd ) {
	//DUAL FORWARD+JUMP+ATTACK
	if ( (pm->ps->legsAnim == BOTH_JUMPATTACK6		&& pm->ps->saberMove == LS_JUMPATTACK_DUAL) ||
		(pm->ps->legsAnim == BOTH_BUTTERFLY_FL1	&& pm->ps->saberMove == LS_JUMPATTACK_STAFF_LEFT) ||
		(pm->ps->legsAnim == BOTH_BUTTERFLY_FR1	&& pm->ps->saberMove == LS_JUMPATTACK_STAFF_RIGHT) ||
		(pm->ps->legsAnim == BOTH_BUTTERFLY_RIGHT	&& pm->ps->saberMove == LS_BUTTERFLY_RIGHT) ||
		(pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT	&& pm->ps->saberMove == LS_BUTTERFLY_LEFT) ) {
		int aLen = PM_AnimLength( 0, BOTH_JUMPATTACK6 );

		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;

		if ( pm->ps->legsAnim == BOTH_JUMPATTACK6 ) { //dual stance attack
			if ( pm->ps->legsTimer >= 100 //not at end
				&& (aLen - pm->ps->legsTimer) >= 250 ) //not in beginning
			{ //middle of anim
				//push forward
				ucmd->forwardmove = 127;
			}

			if ( (pm->ps->legsTimer >= 900 //not at end
				&& aLen - pm->ps->legsTimer >= 950) //not in beginning
				|| (pm->ps->legsTimer >= 1600
				&& aLen - pm->ps->legsTimer >= 400) ) //not in beginning
			{ //one of the two jumps
				if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) { //still on ground?
					if ( pm->ps->groundEntityNum >= MAX_CLIENTS ) {
						//jump!
						pm->ps->velocity.z = 250;//400;
						pm->ps->fd.forceJumpZStart = pm->ps->origin.z;//so we don't take damage if we land at same height
						//pm->ps->pm_flags |= PMF_JUMPING;
						//FIXME: NPCs yell?
						PM_AddEvent( EV_JUMP );
						//G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
					}
				}
				else { //FIXME: if this is the second jump, maybe we should just stop the anim?
				}
			}
		}
		else { //saberstaff attacks
			int aLen = PM_AnimLength( 0, (animNumber_t)pm->ps->legsAnim );
			float lenMin = 1700.0f;
			float lenMax = 1800.0f;

			if ( pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT ) {
				lenMin = 1200.0f;
				lenMax = 1400.0f;
			}

			//FIXME: don't slide off people/obstacles?
			if ( pm->ps->legsAnim == BOTH_BUTTERFLY_RIGHT
				|| pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT ) {
				if ( pm->ps->legsTimer > 450 ) {
					switch ( pm->ps->legsAnim ) {
					case BOTH_BUTTERFLY_LEFT:
						ucmd->rightmove = -127;
						break;
					case BOTH_BUTTERFLY_RIGHT:
						ucmd->rightmove = 127;
						break;
					default:
						break;
					}
				}
			}
			else {
				if ( pm->ps->legsTimer >= 100 //not at end
					&& aLen - pm->ps->legsTimer >= 250 )//not in beginning
				{//middle of anim
					//push forward
					ucmd->forwardmove = 127;
				}
			}

			if ( pm->ps->legsTimer >= lenMin && pm->ps->legsTimer < lenMax ) {//one of the two jumps
				if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//still on ground?
					//jump!
					if ( pm->ps->legsAnim == BOTH_BUTTERFLY_LEFT ) {
						pm->ps->velocity.z = 350;
					}
					else {
						pm->ps->velocity.z = 250;
					}
					pm->ps->fd.forceJumpZStart = pm->ps->origin.z;//so we don't take damage if we land at same height
					//pm->ps->pm_flags |= PMF_JUMPING;//|PMF_SLOW_MO_FALL;
					//FIXME: NPCs yell?
					PM_AddEvent( EV_JUMP );
					//G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
				}
				else {//FIXME: if this is the second jump, maybe we should just stop the anim?
				}
			}
		}

		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {//can only turn when your feet hit the ground
			if ( PM_AdjustAnglesForDualJumpAttack( pm->ps, ucmd ) ) {
				PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, ucmd );
			}
		}
		//rwwFIXMEFIXME: Bother with bbox resizing like sp?
	}
	//STAFF BACK+JUMP+ATTACK
	else if ( pm->ps->saberMove == LS_A_BACKFLIP_ATK &&
		pm->ps->legsAnim == BOTH_JUMPATTACK7 ) {
		int aLen = PM_AnimLength( 0, BOTH_JUMPATTACK7 );

		if ( pm->ps->legsTimer > 800 //not at end
			&& aLen - pm->ps->legsTimer >= 400 )//not in beginning
		{//middle of anim
			if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//still on ground?
				vector3 yawAngles, backDir;

				//push backwards some?
				VectorSet( &yawAngles, 0, pm->ps->viewangles.yaw + 180, 0 );
				AngleVectors( &yawAngles, &backDir, 0, 0 );
				VectorScale( &backDir, 100, &pm->ps->velocity );

				//jump!
				pm->ps->velocity.z = 300;
				pm->ps->fd.forceJumpZStart = pm->ps->origin.z; //so we don't take damage if we land at same height
				//pm->ps->pm_flags |= PMF_JUMPING;//|PMF_SLOW_MO_FALL;

				//FIXME: NPCs yell?
				PM_AddEvent( EV_JUMP );
				//G_SoundOnEnt( ent, CHAN_BODY, "sound/weapons/force/jump.wav" );
				ucmd->upmove = 0; //clear any actual jump command
			}
		}
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
	}
	//STAFF/DUAL SPIN ATTACK
	else if ( pm->ps->saberMove == LS_SPINATTACK ||
		pm->ps->saberMove == LS_SPINATTACK_DUAL ) {
		ucmd->forwardmove = ucmd->rightmove = ucmd->upmove = 0;
		//lock their viewangles during these attacks.
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, ucmd );
	}
}

//constrain him based on the angles of his vehicle and the caps
void PM_VehicleViewAngles( playerState_t *ps, bgEntity_t *veh, usercmd_t *ucmd ) {
	Vehicle_t *pVeh = veh->m_pVehicle;
	qboolean setAngles = qtrue;
	vector3 clampMin, clampMax;
	int i;

	VectorClear( &clampMin );
	VectorClear( &clampMax );

	if ( veh->m_pVehicle->m_pPilot
		&& veh->m_pVehicle->m_pPilot->s.number == ps->clientNum ) {//set the pilot's viewangles to the vehicle's viewangles
#ifdef VEH_CONTROL_SCHEME_4
		if ( 1 )
#else //VEH_CONTROL_SCHEME_4
		if ( !BG_UnrestrainedPitchRoll( ps, veh->m_pVehicle ) )
#endif //VEH_CONTROL_SCHEME_4
		{//only if not if doing special free-roll/pitch control
			setAngles = qtrue;
			clampMax.pitch = pVeh->m_pVehicleInfo->lookPitch;
			clampMin.yaw = clampMax.yaw = 0;
			clampMin.roll = clampMax.roll = -1;
		}
	}
	else {
		//NOTE: passengers can look around freely, UNLESS they're controlling a turret!
		for ( i = 0; i < MAX_VEHICLE_TURRETS; i++ ) {
			if ( veh->m_pVehicle->m_pVehicleInfo->turret[i].passengerNum == ps->generic1 ) {//this turret is my station
				setAngles = qtrue;
				clampMin.pitch = veh->m_pVehicle->m_pVehicleInfo->turret[i].pitchClampUp;
				clampMax.pitch = veh->m_pVehicle->m_pVehicleInfo->turret[i].pitchClampDown;
				clampMin.yaw = veh->m_pVehicle->m_pVehicleInfo->turret[i].yawClampRight;
				clampMax.yaw = veh->m_pVehicle->m_pVehicleInfo->turret[i].yawClampLeft;
				clampMin.roll = clampMax.roll = 0;
				break;
			}
		}
	}
	if ( setAngles ) {
		for ( i = 0; i < 3; i++ ) {//clamp viewangles
			if ( clampMin.data[i] == -1 || clampMax.data[i] == -1 ) {//no clamp
			}
			else if ( !clampMin.data[i] && !clampMax.data[i] ) {//no allowance
				//ps->viewangles.data[i] = veh->playerState->viewangles.data[i];
			}
			else {//allowance
				if ( ps->viewangles.data[i] > clampMax.data[i] )
					ps->viewangles.data[i] = clampMax.data[i];
				else if ( ps->viewangles.data[i] < clampMin.data[i] )
					ps->viewangles.data[i] = clampMin.data[i];
			}
		}

		PM_SetPMViewAngle( ps, &ps->viewangles, ucmd );
	}
}

//see if a weapon is ok to use on a vehicle
qboolean PM_WeaponOkOnVehicle( int weapon ) {
	//FIXME: check g_vehicleInfo for our vehicle?
	switch ( weapon ) {
	case WP_MELEE:
	case WP_SABER:
	case WP_BLASTER:
		//	case WP_THERMAL:
		return qtrue;
	default:
		return qfalse;
	}
}

//do we have a weapon that's ok for using on the vehicle?
int PM_GetOkWeaponForVehicle( void ) {
	int i = 0;

	while ( i < WP_NUM_WEAPONS ) {
		if ( (pm->ps->stats[STAT_WEAPONS] & (1 << i)) &&
			PM_WeaponOkOnVehicle( i ) ) { //this one's good
			return i;
		}

		i++;
	}

	//oh dear!
	//assert(!"No valid veh weaps");
	return -1;
}

//force the vehicle to turn and travel to its forced destination point
void PM_VehForcedTurning( bgEntity_t *veh ) {
	bgEntity_t *dst = PM_BGEntForNum( veh->playerState->vehTurnaroundIndex );
	float pitchD, yawD;
	vector3 dir;

	if ( !veh || !veh->m_pVehicle ) {
		return;
	}

	if ( !dst ) { //can't find dest ent?
		return;
	}

	pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
	pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
	pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

	VectorSubtract( &dst->s.origin, &veh->playerState->origin, &dir );
	vectoangles( &dir, &dir );

	yawD = AngleSubtract( pm->ps->viewangles.yaw, dir.yaw );
	pitchD = AngleSubtract( pm->ps->viewangles.pitch, dir.pitch );

	yawD *= 0.6f*pml.frametime;
	pitchD *= 0.6f*pml.frametime;

#ifdef VEH_CONTROL_SCHEME_4
	veh->playerState->viewangles.yaw = AngleSubtract(veh->playerState->viewangles.yaw, yawD);
	veh->playerState->viewangles.pitch = AngleSubtract(veh->playerState->viewangles.pitch, pitchD);
	pm->ps->viewangles.yaw = veh->playerState->viewangles.yaw;
	pm->ps->viewangles.pitch = 0;

	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
	PM_SetPMViewAngle(veh->playerState, veh->playerState->viewangles, &pm->cmd);
	VectorClear( veh->m_pVehicle->m_vPrevRiderViewAngles );
	veh->m_pVehicle->m_vPrevRiderViewAngles.yaw = AngleNormalize180(pm->ps->viewangles.yaw);

#else //VEH_CONTROL_SCHEME_4

	pm->ps->viewangles.yaw = AngleSubtract( pm->ps->viewangles.yaw, yawD );
	pm->ps->viewangles.pitch = AngleSubtract( pm->ps->viewangles.pitch, pitchD );

	PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
#endif //VEH_CONTROL_SCHEME_4
}

#ifdef VEH_CONTROL_SCHEME_4
void PM_VehFaceHyperspacePoint(bgEntity_t *veh)
{

	if (!veh || !veh->m_pVehicle)
	{
		return;
	}
	else
	{
		float timeFrac = ((float)(pm->cmd.serverTime-veh->playerState->hyperSpaceTime))/HYPERSPACE_TIME;
		float	turnRate, aDelta;
		int		i, matchedAxes = 0;

		pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
		pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
		pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

		turnRate = (90.0f*pml.frametime);
		for ( i = 0; i < 3; i++ )
		{
			aDelta = AngleSubtract(veh->playerState->hyperSpaceangles.data[i], veh->m_pVehicle->m_vOrientation[i]);
			if ( fabsf( aDelta ) < turnRate )
			{//all is good
				veh->playerState->viewangles.data[i] = veh->playerState->hyperSpaceangles.data[i];
				matchedAxes++;
			}
			else
			{
				aDelta = AngleSubtract(veh->playerState->hyperSpaceangles.data[i], veh->playerState->viewangles.data[i]);
				if ( fabsf( aDelta ) < turnRate )
				{
					veh->playerState->viewangles.data[i] = veh->playerState->hyperSpaceangles.data[i];
				}
				else if ( aDelta > 0 )
				{
					if ( i == YAW )
					{
						veh->playerState->viewangles.data[i] = AngleNormalize360( veh->playerState->viewangles.data[i]+turnRate );
					}
					else
					{
						veh->playerState->viewangles.data[i] = AngleNormalize180( veh->playerState->viewangles.data[i]+turnRate );
					}
				}
				else
				{
					if ( i == YAW )
					{
						veh->playerState->viewangles.data[i] = AngleNormalize360( veh->playerState->viewangles.data[i]-turnRate );
					}
					else
					{
						veh->playerState->viewangles.data[i] = AngleNormalize180( veh->playerState->viewangles.data[i]-turnRate );
					}
				}
			}
		}

		pm->ps->viewangles.yaw = veh->playerState->viewangles.yaw;
		pm->ps->viewangles.pitch = 0.0f;

		PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
		PM_SetPMViewAngle(veh->playerState, veh->playerState->viewangles, &pm->cmd);
		VectorClear( veh->m_pVehicle->m_vPrevRiderViewAngles );
		veh->m_pVehicle->m_vPrevRiderViewAngles.yaw = AngleNormalize180(pm->ps->viewangles.yaw);

		if ( timeFrac < HYPERSPACE_TELEPORT_FRAC )
		{//haven't gone through yet
			if ( matchedAxes < 3 )
			{//not facing the right dir yet
				//keep hyperspace time up to date
				veh->playerState->hyperSpaceTime += pml.msec;
			}
			else if ( !(veh->playerState->eFlags2&EF2_HYPERSPACE))
			{//flag us as ready to hyperspace!
				veh->playerState->eFlags2 |= EF2_HYPERSPACE;
			}
		}
	}
}

#else //VEH_CONTROL_SCHEME_4

void PM_VehFaceHyperspacePoint( bgEntity_t *veh ) {

	if ( !veh || !veh->m_pVehicle ) {
		return;
	}
	else {
		float timeFrac = ((float)(pm->cmd.serverTime - veh->playerState->hyperSpaceTime)) / HYPERSPACE_TIME;
		float	turnRate, aDelta;
		int		i, matchedAxes = 0;

		pm->cmd.upmove = veh->m_pVehicle->m_ucmd.upmove = 127;
		pm->cmd.forwardmove = veh->m_pVehicle->m_ucmd.forwardmove = 0;
		pm->cmd.rightmove = veh->m_pVehicle->m_ucmd.rightmove = 0;

		turnRate = (90.0f*pml.frametime);
		for ( i = 0; i < 3; i++ ) {
			aDelta = AngleSubtract( veh->playerState->hyperSpaceAngles.data[i], veh->m_pVehicle->m_vOrientation->data[i] );
			if ( fabsf( aDelta ) < turnRate ) {//all is good
				pm->ps->viewangles.data[i] = veh->playerState->hyperSpaceAngles.data[i];
				matchedAxes++;
			}
			else {
				aDelta = AngleSubtract( veh->playerState->hyperSpaceAngles.data[i], pm->ps->viewangles.data[i] );
				if ( fabsf( aDelta ) < turnRate ) {
					pm->ps->viewangles.data[i] = veh->playerState->hyperSpaceAngles.data[i];
				}
				else if ( aDelta > 0 ) {
					if ( i == 1/*YAW*/ )	pm->ps->viewangles.data[i] = AngleNormalize360( pm->ps->viewangles.data[i] + turnRate );
					else					pm->ps->viewangles.data[i] = AngleNormalize180( pm->ps->viewangles.data[i] + turnRate );
				}
				else {
					if ( i == 1/*YAW*/ )	pm->ps->viewangles.data[i] = AngleNormalize360( pm->ps->viewangles.data[i] - turnRate );
					else					pm->ps->viewangles.data[i] = AngleNormalize180( pm->ps->viewangles.data[i] - turnRate );
				}
			}
		}

		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );

		if ( timeFrac < HYPERSPACE_TELEPORT_FRAC ) {//haven't gone through yet
			if ( matchedAxes < 3 ) {//not facing the right dir yet
				//keep hyperspace time up to date
				veh->playerState->hyperSpaceTime += pml.msec;
			}
			else if ( !(veh->playerState->eFlags2&EF2_HYPERSPACE) ) {//flag us as ready to hyperspace!
				veh->playerState->eFlags2 |= EF2_HYPERSPACE;
			}
		}
	}
}

#endif //VEH_CONTROL_SCHEME_4

void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vector3 *origin, vector3 *mins, vector3 *maxs,
	int clientNum, int tracemask,
	void( *localTrace )(trace_t *results, const vector3 *start, const vector3 *mins, const vector3 *maxs, const vector3 *end, int passEntityNum, int contentMask) ) {
	if ( !veh
		|| !veh->m_pVehicleInfo->length
		|| !veh->m_pVehicleInfo->width
		|| !veh->m_pVehicleInfo->height )
		//|| veh->m_LandTrace.fraction < 1.0f )
	{
		return;
	}
	else if ( veh->m_pVehicleInfo->type != VH_FIGHTER
		//&& veh->m_pVehicleInfo->type != VH_SPEEDER
		&& veh->m_pVehicleInfo->type != VH_FLIER ) {//only those types of vehicles have dynamic bboxes, the rest just use a static bbox
		VectorSet( maxs, veh->m_pVehicleInfo->width / 2.0f, veh->m_pVehicleInfo->width / 2.0f, veh->m_pVehicleInfo->height + DEFAULT_MINS_2 );
		VectorSet( mins, veh->m_pVehicleInfo->width / -2.0f, veh->m_pVehicleInfo->width / -2.0f, DEFAULT_MINS_2 );
		return;
	}
	else {
		vector3	axis[3], point[8];
		vector3	newMins, newMaxs;
		int		curAxis = 0, i;
		trace_t trace;

		AnglesToAxis( veh->m_vOrientation, axis );
		VectorMA( origin, veh->m_pVehicleInfo->length / 2.0f, &axis[0], &point[0] );
		VectorMA( origin, -veh->m_pVehicleInfo->length / 2.0f, &axis[0], &point[1] );
		//extrapolate each side up and down
		VectorMA( &point[0], veh->m_pVehicleInfo->height / 2.0f, &axis[2], &point[0] );
		VectorMA( &point[0], -veh->m_pVehicleInfo->height, &axis[2], &point[2] );
		VectorMA( &point[1], veh->m_pVehicleInfo->height / 2.0f, &axis[2], &point[1] );
		VectorMA( &point[1], -veh->m_pVehicleInfo->height, &axis[2], &point[3] );

		VectorMA( origin, veh->m_pVehicleInfo->width / 2.0f, &axis[1], &point[4] );
		VectorMA( origin, -veh->m_pVehicleInfo->width / 2.0f, &axis[1], &point[5] );
		//extrapolate each side up and down
		VectorMA( &point[4], veh->m_pVehicleInfo->height / 2.0f, &axis[2], &point[4] );
		VectorMA( &point[4], -veh->m_pVehicleInfo->height, &axis[2], &point[6] );
		VectorMA( &point[5], veh->m_pVehicleInfo->height / 2.0f, &axis[2], &point[5] );
		VectorMA( &point[5], -veh->m_pVehicleInfo->height, &axis[2], &point[7] );
		/*
		VectorMA( origin, veh->m_pVehicleInfo->height/2.0f, axis[2], point[4] );
		VectorMA( origin, -veh->m_pVehicleInfo->height/2.0f, axis[2], point[5] );
		*/
		//Now inflate a bbox around these points
		VectorCopy( origin, &newMins );
		VectorCopy( origin, &newMaxs );
		for ( curAxis = 0; curAxis < 3; curAxis++ ) {
			for ( i = 0; i<8; i++ ) {
				if ( point[i].data[curAxis] > newMaxs.data[curAxis] )
					newMaxs.data[curAxis] = point[i].data[curAxis];
				else if ( point[i].data[curAxis] < newMins.data[curAxis] )
					newMins.data[curAxis] = point[i].data[curAxis];
			}
		}
		VectorSubtract( &newMins, origin, &newMins );
		VectorSubtract( &newMaxs, origin, &newMaxs );
		//now see if that's a valid way to be
		if ( localTrace ) {
			localTrace( &trace, origin, &newMins, &newMaxs, origin, clientNum, tracemask );
		}
		else { //don't care about solid stuff then
			trace.startsolid = trace.allsolid = 0;
		}
		if ( !trace.startsolid && !trace.allsolid ) {//let's use it!
			VectorCopy( &newMins, mins );
			VectorCopy( &newMaxs, maxs );
		}
		//else: just use the last one, I guess...?
		//FIXME: make it as close as possible?  Or actually prevent the change in m_vOrientation?  Or push away from anything we hit?
	}
}
/*
================
PmoveSingle

================
*/
extern int BG_EmplacedView( vector3 *baseAngles, vector3 *angles, float *newYaw, float constraint );
extern qboolean BG_FighterUpdate( Vehicle_t *pVeh, const usercmd_t *pUcmd, vector3 *trMins, vector3 *trMaxs, float gravity,
	void( *traceFunc )(trace_t *results, const vector3 *start, const vector3 *lmins, const vector3 *lmaxs, const vector3 *end, int passEntityNum, int contentMask) ); //FighterNPC.c

#define JETPACK_HOVER_HEIGHT	64

//#define _TESTING_VEH_PREDICTION

void PM_MoveForKata( usercmd_t *ucmd ) {
	if ( pm->ps->legsAnim == BOTH_A7_SOULCAL
		&& pm->ps->saberMove == LS_STAFF_SOULCAL ) {//forward spinning staff attack
		ucmd->upmove = 0;

		if ( PM_CanRollFromSoulCal( pm->ps ) ) {
			ucmd->upmove = -127;
			ucmd->rightmove = 0;
			if ( ucmd->forwardmove < 0 ) {
				ucmd->forwardmove = 0;
			}
		}
		else {
			ucmd->rightmove = 0;
			//FIXME: don't slide off people/obstacles?
			if ( pm->ps->legsTimer >= 2750 ) {//not at end
				//push forward
				ucmd->forwardmove = 64;
			}
			else {
				ucmd->forwardmove = 0;
			}
		}
		if ( pm->ps->legsTimer >= 2650
			&& pm->ps->legsTimer < 2850 ) {//the jump
			if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//still on ground?
				//jump!
				pm->ps->velocity.z = 250;
				pm->ps->fd.forceJumpZStart = pm->ps->origin.z;//so we don't take damage if we land at same height
				//	pm->ps->pm_flags |= PMF_JUMPING;//|PMF_SLOW_MO_FALL;
				//FIXME: NPCs yell?
				PM_AddEvent( EV_JUMP );
			}
		}
	}
	else if ( pm->ps->legsAnim == BOTH_A2_SPECIAL ) { //medium kata
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;

		if ( pm->ps->legsTimer < 2700 && pm->ps->legsTimer > 2300 )
			pm->cmd.forwardmove = 127;

		else if ( pm->ps->legsTimer < 900 && pm->ps->legsTimer > 500 )
			pm->cmd.forwardmove = 127;

		else
			pm->cmd.forwardmove = 0;
	}
	else if ( pm->ps->legsAnim == BOTH_A3_SPECIAL ) { //strong kata
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;

		if ( pm->ps->legsTimer < 1700 && pm->ps->legsTimer > 1000 )
			pm->cmd.forwardmove = 127;
		else
			pm->cmd.forwardmove = 0;
	}
	else {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}
}

void PmoveSingle( pmove_t *pmove ) {
	qboolean stiffenedUp = qfalse;
	float gDist = 0;
	qboolean noAnimate = qfalse;
	int savedGravity = 0;

	pm = pmove;

#ifdef _GAME
	if ( japp_fixWeaponCharge.integer ) {
		if ( pm->cmd.buttons & BUTTON_ATTACK && pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
			pm->cmd.buttons &= ~BUTTON_ATTACK;
			pm->cmd.buttons &= ~BUTTON_USE_HOLDABLE;
		}
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK && pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons &= ~BUTTON_USE_HOLDABLE;
		}
	}
#endif

	if ( pm->ps->emplacedIndex ) {
		if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) { //hackerrific.
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	}

	//set up these "global" bg ents
	pm_entSelf = PM_BGEntForNum( pm->ps->clientNum );
	if ( pm->ps->m_iVehicleNum ) {
		if ( pm->ps->clientNum < MAX_CLIENTS ) { //player riding vehicle
			pm_entVeh = PM_BGEntForNum( pm->ps->m_iVehicleNum );
		}
		else { //vehicle with player pilot
			pm_entVeh = PM_BGEntForNum( pm->ps->m_iVehicleNum - 1 );
		}
	}
	else { //no vehicle ent
		pm_entVeh = NULL;
	}

	gPMDoSlowFall = PM_DoSlowFall();

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( PM_IsRocketTrooper() ) { //kind of nasty, don't let them crouch or anything if nonhumanoid (probably a rockettrooper)
		if ( pm->cmd.upmove < 0 ) {
			pm->cmd.upmove = 0;
		}
	}

	if ( pm->ps->pm_type == PM_FLOAT ) { //You get no control over where you go in grip movement
		stiffenedUp = qtrue;
	}
	else if ( pm->ps->eFlags & EF_DISINTEGRATION ) {
		stiffenedUp = qtrue;
	}
	else if ( BG_SaberLockBreakAnim( pm->ps->legsAnim )
		|| BG_SaberLockBreakAnim( pm->ps->torsoAnim )
		|| pm->ps->saberLockTime >= pm->cmd.serverTime ) {//can't move or turn
		stiffenedUp = qtrue;
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	}

	// DFA
	else if ( pm->ps->saberMove == LS_A_BACK || pm->ps->saberMove == LS_A_BACK_CR ||
		pm->ps->saberMove == LS_A_BACKSTAB || pm->ps->saberMove == LS_A_FLIP_STAB ||
		pm->ps->saberMove == LS_A_FLIP_SLASH || pm->ps->saberMove == LS_A_JUMP_T__B_ ||
		pm->ps->saberMove == LS_DUAL_LR || pm->ps->saberMove == LS_DUAL_FB ) {
		if ( !GetCInfo( CINFO_YELLOWDFA )
			&& (pm->ps->legsAnim == BOTH_JUMPFLIPSTABDOWN || pm->ps->legsAnim == BOTH_JUMPFLIPSLASHDOWN1) ) {
			// flipover medium stance attack
			if ( pm->ps->legsTimer < 1600 && pm->ps->legsTimer > 900 ) {
				pm->ps->viewangles.yaw += pml.frametime*240.0f;
				PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
			}
		}
		stiffenedUp = qtrue;
	}
	else if (
		(pm->ps->legsAnim) == (BOTH_A2_STABBACK1) ||
		(pm->ps->legsAnim) == (BOTH_ATTACK_BACK) ||
		(pm->ps->legsAnim) == (BOTH_CROUCHATTACKBACK1) ||
		(pm->ps->legsAnim) == (BOTH_FORCELEAP2_T__B_) ||
		(pm->ps->legsAnim) == (BOTH_JUMPFLIPSTABDOWN) ||
		(pm->ps->legsAnim) == (BOTH_JUMPFLIPSLASHDOWN1) ) {
		stiffenedUp = qtrue;
	}
	else if ( pm->ps->legsAnim == BOTH_ROLL_STAB ) {
		stiffenedUp = qtrue;
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	}
	else if ( pm->ps->heldByClient ) {
		stiffenedUp = qtrue;
	}
	else if ( BG_KickMove( pm->ps->saberMove ) || BG_KickingAnim( pm->ps->legsAnim ) ) {
		stiffenedUp = qtrue;
	}
	//Raz: Bug fix http://forum.cosmosgaming.net/threads/1510/
	else if ( BG_InGrappleMove( pm->ps->torsoAnim ) || BG_InGrappleMove( pm->ps->legsAnim ) ) {
		stiffenedUp = qtrue;
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	}
	else if ( pm->ps->saberMove == LS_STABDOWN_DUAL ||
		pm->ps->saberMove == LS_STABDOWN_STAFF ||
		pm->ps->saberMove == LS_STABDOWN ) {//FIXME: need to only move forward until we bump into our target...?
		if ( pm->ps->legsTimer < 800 ) { //freeze movement near end of anim
			stiffenedUp = qtrue;
			PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
		}
		else { //force forward til then
			pm->cmd.rightmove = 0;
			pm->cmd.upmove = 0;
			pm->cmd.forwardmove = 64;
		}
	}
	else if ( pm->ps->saberMove == LS_PULL_ATTACK_STAB ||
		pm->ps->saberMove == LS_PULL_ATTACK_SWING ) {
		stiffenedUp = qtrue;
	}

	//Raz: Added
	if ( pm->ps->legsAnim == BOTH_CROUCH3 || //amkneel
		pm->ps->legsAnim == BOTH_HIT1 || //amdie
		pm->ps->legsAnim == BOTH_DEATH15 || //amdie2
		pm->ps->legsAnim == BOTH_SLEEP1 || //sleep
		pm->ps->legsAnim == TORSO_SURRENDER_START || //amsurrender
		pm->ps->legsAnim == BOTH_SLEEP6START || //amsit
		pm->ps->legsAnim == BOTH_STAND5TOSIT3 || //amsit2
		pm->ps->legsAnim == BOTH_SIT6 || //amsit3
		pm->ps->legsAnim == BOTH_SIT7 || //amsit4
		pm->ps->legsAnim == BOTH_KNEES2 || //ambeg
		pm->ps->legsAnim == BOTH_STAND10 || //amwait
		pm->ps->legsAnim == BOTH_STAND5TOSTAND8 || //amhips
		pm->ps->legsAnim == BOTH_FLIP_STAB || //Red special move
		pm->ps->legsAnim == BOTH_JUMP_BACKKICK_SPIN || //Dual special move
		pm->ps->legsAnim == BOTH_FORCE_GETUP_F2 || //amdie get up
		pm->ps->legsAnim == BOTH_SABERKILLER1 || //amfinishinghim
		pm->ps->legsAnim == BOTH_FORCE_GETUP_F2 //ampower
		)
		stiffenedUp = qtrue;

	else if ( BG_SaberInKata( pm->ps->saberMove ) ||
		BG_InKataAnim( pm->ps->torsoAnim ) ||
		BG_InKataAnim( pm->ps->legsAnim ) ) {
		PM_MoveForKata( &pm->cmd );
	}
	else if ( BG_FullBodyTauntAnim( pm->ps->legsAnim )
		&& BG_FullBodyTauntAnim( pm->ps->torsoAnim ) ) {
		if ( (pm->cmd.buttons&BUTTON_ATTACK)
			|| (pm->cmd.buttons&BUTTON_ALT_ATTACK)
			|| (pm->cmd.buttons&BUTTON_FORCEPOWER)
			|| (pm->cmd.buttons&BUTTON_FORCEGRIP)
			|| (pm->cmd.buttons&BUTTON_FORCE_LIGHTNING)
			|| (pm->cmd.buttons&BUTTON_FORCE_DRAIN)
			|| pm->cmd.upmove ) {//stop the anim
			if ( pm->ps->legsAnim == BOTH_MEDITATE
				&& pm->ps->torsoAnim == BOTH_MEDITATE ) {
				PM_SetAnim( SETANIM_BOTH, BOTH_MEDITATE_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
			}
			else {
				pm->ps->legsTimer = pm->ps->torsoTimer = 0;
			}
			if ( pm->ps->forceHandExtend == HANDEXTEND_TAUNT ) {
				pm->ps->forceHandExtend = 0;
			}
		}
		//Raz: Prediction hacks to match JA+ server (gloat, bow, etc)
		//	else
		else if ( pm->ps->torsoAnim != BOTH_BOW && (pm->ps->torsoAnim < BOTH_SHOWOFF_FAST || pm->ps->torsoAnim > BOTH_VICTORY_STAFF) ) {
			if ( pm->ps->legsAnim == BOTH_MEDITATE ) {
				if ( pm->ps->legsTimer < 100 ) {
					pm->ps->legsTimer = 100;
				}
			}
			if ( pm->ps->torsoAnim == BOTH_MEDITATE ) {
				if ( pm->ps->torsoTimer < 100 ) {
					pm->ps->legsTimer = 100;
				}
				pm->ps->forceHandExtend = HANDEXTEND_TAUNT;
				pm->ps->forceHandExtendTime = pm->cmd.serverTime + 100;
			}
			if ( pm->ps->legsTimer > 0 || pm->ps->torsoTimer > 0 ) {
				stiffenedUp = qtrue;
				//	PM_SetPMViewAngle(pm->ps, pm->ps->viewangles, &pm->cmd);
				pm->cmd.rightmove = 0;
				pm->cmd.upmove = 0;
				pm->cmd.forwardmove = 0;
				//	pm->cmd.buttons = 0;
			}
		}
	}

	else if ( pm->ps->legsAnim == BOTH_MEDITATE_END
		&& pm->ps->legsTimer > 0 ) {
		stiffenedUp = qtrue;
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;
		pm->cmd.buttons = 0;
	}

	else if ( pm->ps->legsAnim == BOTH_FORCELAND1 ||
		pm->ps->legsAnim == BOTH_FORCELANDBACK1 ||
		pm->ps->legsAnim == BOTH_FORCELANDRIGHT1 ||
		pm->ps->legsAnim == BOTH_FORCELANDLEFT1 ) { //can't move while in a force land
		stiffenedUp = qtrue;
	}

	if ( pm->ps->saberMove == LS_A_LUNGE ) {//can't move during lunge
		pm->cmd.rightmove = pm->cmd.upmove = 0;
		if ( pm->ps->legsTimer > 500 ) {
			pm->cmd.forwardmove = 127;
		}
		else {
			pm->cmd.forwardmove = 0;
		}
	}

	if ( pm->ps->saberMove == LS_A_JUMP_T__B_ ) {//can't move during leap
		if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//hit the ground
			pm->cmd.forwardmove = 0;
		}
		pm->cmd.rightmove = pm->cmd.upmove = 0;
	}

	if ( (pm->ps->legsAnim) == BOTH_KISSER || (pm->ps->legsAnim) == BOTH_KISSEE ) {
		stiffenedUp = qtrue;
	}

	if ( pm->ps->emplacedIndex ) {
		if ( pm->cmd.forwardmove < 0 || PM_GroundDistance() > 32.0f ) {
			pm->ps->emplacedIndex = 0;
			pm->ps->saberHolstered = 0;
		}
		else {
			stiffenedUp = qtrue;
		}
	}

	/*
	if (pm->ps->weapon == WP_DISRUPTOR && pm->ps->weaponstate == WEAPON_CHARGING_ALT)
	{ //not allowed to move while charging the disruptor
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	if (pm->cmd.upmove > 0)
	{
	pm->cmd.upmove = 0;
	}
	}
	*/

	if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->weaponstate == WEAPON_CHARGING_ALT ) { //not allowed to move while charging the disruptor
		if ( pm->cmd.forwardmove ||
			pm->cmd.rightmove ||
			pm->cmd.upmove > 0 ) { //get out
			pm->ps->weaponstate = WEAPON_READY;
			pm->ps->weaponTime = 1000;
			PM_AddEventWithParm( EV_WEAPON_CHARGE, WP_DISRUPTOR ); //cut the weapon charge sound
			pm->cmd.upmove = 0;
		}
	}
	else if ( pm->ps->weapon == WP_DISRUPTOR && pm->ps->zoomMode == 1 ) { //can't jump
		if ( pm->cmd.upmove > 0 ) {
			pm->cmd.upmove = 0;
		}
	}

	if ( stiffenedUp ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->fd.forceGripCripple ) { //don't let attack or alt attack if being gripped I guess
		pm->cmd.buttons &= ~BUTTON_ATTACK;
		pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;
	}

	if ( BG_InRoll( pm->ps, pm->ps->legsAnim ) ) { //can't roll unless you're able to move normally
		BG_CmdForRoll( pm->ps, pm->ps->legsAnim, &pm->cmd );
	}

	PM_CmdForSaberMoves( &pm->cmd );

	BG_AdjustClientSpeed( pm->ps, &pm->cmd, pm->cmd.serverTime );

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	}
	else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	pm_cancelOutZoom = qfalse;
	if ( pm->ps->weapon == WP_DISRUPTOR &&
		pm->ps->zoomMode == 1 ) {
		if ( (pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
			!(pm->cmd.buttons & BUTTON_ATTACK) &&
			pm->ps->zoomLocked ) {
			pm_cancelOutZoom = qtrue;
		}
	}
	// In certain situations, we may want to control which attack buttons are pressed and what kind of functionality
	//	is attached to them
	PM_AdjustAttackStates( pm );

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 &&
		!(pm->cmd.buttons & (BUTTON_ATTACK | BUTTON_USE_HOLDABLE)) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset( &pml, 0, sizeof(pml) );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	}
	else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}

	/*
	if (pm->ps->clientNum >= MAX_CLIENTS)
	{
	#ifdef _GAME
	Com_Printf( S_C0LOR_RED" SERVER N%i msec %d\n", pm->ps->clientNum, pml.msec );
	#else
	Com_Printf( S_COLOR_GREEN" CLIENT N%i msec %d\n", pm->ps->clientNum, pml.msec );
	#endif
	}
	*/

	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy( &pm->ps->origin, &pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy( &pm->ps->velocity, &pml.previous_velocity );

	pml.frametime = pml.msec * 0.001f;

	if ( pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE ) { //we are a vehicle
		bgEntity_t *veh = pm_entSelf;
		assert( veh && veh->m_pVehicle );
		if ( veh && veh->m_pVehicle ) {
			veh->m_pVehicle->m_fTimeModifier = pml.frametime*60.0f;
		}
	}
	else if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&&pm->ps->m_iVehicleNum ) {
		bgEntity_t *veh = pm_entVeh;

		if ( veh && veh->playerState &&
			(pm->cmd.serverTime - veh->playerState->hyperSpaceTime) < HYPERSPACE_TIME ) { //going into hyperspace, turn to face the right angles
			PM_VehFaceHyperspacePoint( veh );
		}
		else if ( veh && veh->playerState &&
			veh->playerState->vehTurnaroundIndex &&
			veh->playerState->vehTurnaroundTime > pm->cmd.serverTime ) { //riding this vehicle, turn my view too
			PM_VehForcedTurning( veh );
		}
	}

	if ( pm->ps->legsAnim == BOTH_FORCEWALLRUNFLIP_ALT &&
		pm->ps->legsTimer > 0 ) {
		vector3 vFwd, fwdAng;
		VectorSet( &fwdAng, 0.0f, pm->ps->viewangles.yaw, 0.0f );

		AngleVectors( &fwdAng, &vFwd, NULL, NULL );
		if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
			float savZ = pm->ps->velocity.z;
			VectorScale( &vFwd, 100, &pm->ps->velocity );
			pm->ps->velocity.z = savZ;
		}
		pm->cmd.forwardmove = pm->cmd.rightmove = pm->cmd.upmove = 0;
		PM_AdjustAnglesForWallRunUpFlipAlt( &pm->cmd );
	}

	//	PM_AdjustAngleForWallRun(pm->ps, &pm->cmd, qtrue);
	//	PM_AdjustAnglesForStabDown( pm->ps, &pm->cmd );
	PM_AdjustAngleForWallJump( pm->ps, &pm->cmd, qtrue );
	PM_AdjustAngleForWallRunUp( pm->ps, &pm->cmd, qtrue );
	PM_AdjustAngleForWallRun( pm->ps, &pm->cmd, qtrue );
	PM_AdjustAngleForWallGrap( pm->ps, &pm->cmd );

	if ( pm->ps->saberMove == LS_A_JUMP_T__B_ || pm->ps->saberMove == LS_A_LUNGE ||
		pm->ps->saberMove == LS_A_BACK_CR || pm->ps->saberMove == LS_A_BACK ||
		pm->ps->saberMove == LS_A_BACKSTAB ) {
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	}

	if ( (pm->ps->legsAnim) == BOTH_KISSER ||
		(pm->ps->legsAnim) == BOTH_KISSEE ) {
		pm->ps->viewangles.pitch = 0;
		PM_SetPMViewAngle( pm->ps, &pm->ps->viewangles, &pm->cmd );
	}

	PM_SetSpecialMoveValues();

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	AngleVectors( &pm->ps->viewangles, &pml.forward, &pml.right, &pml.up );

	if ( pm->cmd.upmove < 10 && !(pm->ps->pm_flags & PMF_STUCK_TO_WALL) ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if ( pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	/*
	if (pm->ps->fd.saberAnimLevel == SS_STAFF &&
	(pm->cmd.buttons & BUTTON_ALT_ATTACK) &&
	pm->cmd.upmove > 0)
	{ //this is how you do kick-for-condition
	pm->cmd.upmove = 0;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	}
	*/

	if ( pm->ps->saberLockTime >= pm->cmd.serverTime ) {
		pm->cmd.upmove = 0;
		pm->cmd.forwardmove = 0;//50;
		pm->cmd.rightmove = 0;//*= 0.1f;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck();
		if ( !pm->noSpecMove ) {
			PM_FlyMove();
		}
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		if ( pm->ps->clientNum < MAX_CLIENTS ) {
			PM_NoclipMove();
			PM_DropTimers();
			return;
		}
	}

	if ( pm->ps->pm_type == PM_FREEZE ) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION ) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	if ( pm->ps->pm_type == PM_JETPACK ) {
		gDist = PM_GroundDistance();
		savedGravity = pm->ps->gravity;

		if ( gDist < JETPACK_HOVER_HEIGHT + 64 ) {
			pm->ps->gravity *= 0.1f;
		}
		else {
			pm->ps->gravity *= 0.25f;
		}
	}
	else if ( gPMDoSlowFall ) {
		savedGravity = pm->ps->gravity;
		pm->ps->gravity *= 0.5f;
	}

	//if we're in jetpack mode then see if we should be jetting around
	if ( pm->ps->pm_type == PM_JETPACK ) {
		if ( pm->cmd.rightmove > 0 ) {
			PM_ContinueLegsAnim( BOTH_INAIRRIGHT1 );
		}
		else if ( pm->cmd.rightmove < 0 ) {
			PM_ContinueLegsAnim( BOTH_INAIRLEFT1 );
		}
		else if ( pm->cmd.forwardmove > 0 ) {
			PM_ContinueLegsAnim( BOTH_INAIR1 );
		}
		else if ( pm->cmd.forwardmove < 0 ) {
			PM_ContinueLegsAnim( BOTH_INAIRBACK1 );
		}
		else {
			PM_ContinueLegsAnim( BOTH_INAIR1 );
		}

		if ( pm->ps->weapon == WP_SABER &&
			BG_SpinningSaberAnim( pm->ps->legsAnim ) ) { //make him stir around since he shouldn't have any real control when spinning
			pm->ps->velocity.x += Q_irand( -100, 100 );
			pm->ps->velocity.y += Q_irand( -100, 100 );
		}

		if ( pm->cmd.upmove > 0 && pm->ps->velocity.z < 256 ) { //cap upward velocity off at 256. Seems reasonable.
			float addIn = 12.0f;

			/*
						//Add based on our distance to the ground if we're already travelling upward
						if (pm->ps->velocity[2] > 0)
						{
						while (gDist > 64)
						{ //subtract 1 for every 64 units off the ground we get
						addIn--;

						gDist -= 64;

						if (addIn <= 0)
						{ //break out if we're not even going to add anything
						break;
						}
						}
						}
						*/
			if ( pm->ps->velocity.z > 0 ) {
				addIn = 12.0f - (gDist / 64.0f);
			}

			if ( addIn > 0.0f ) {
				pm->ps->velocity.z += addIn;
			}

			pm->ps->eFlags |= EF_JETPACK_FLAMING; //going up
		}
		else {
			pm->ps->eFlags &= ~EF_JETPACK_FLAMING; //idling

			if ( pm->ps->velocity.z < 256 ) {
				if ( pm->ps->velocity.z < -100 )
					pm->ps->velocity.z = -100;
				if ( gDist < JETPACK_HOVER_HEIGHT ) { //make sure we're always hovering off the ground somewhat while jetpack is active
					pm->ps->velocity.z += 2;
				}
			}
		}
	}

	if ( pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf && pm_entSelf->m_pVehicle ) { //Now update our mins/maxs to match our m_vOrientation based on our length, width & height
		BG_VehicleAdjustBBoxForOrientation( pm_entSelf->m_pVehicle, &pm->ps->origin, &pm->mins, &pm->maxs, pm->ps->clientNum, pm->tracemask, pm->trace );
	}

	// set groundentity
	PM_GroundTrace();
	if ( pm_flying == FLY_HOVER ) {//never stick to the ground
		PM_HoverTrace();
	}

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {//on ground
		pm->ps->fd.forceJumpZStart = 0;
	}

	if ( pm->ps->pm_type == PM_DEAD ) {
		if ( pm->ps->clientNum >= MAX_CLIENTS &&
			pm_entSelf &&
			pm_entSelf->s.NPC_class == CLASS_VEHICLE &&
			pm_entSelf->m_pVehicle->m_pVehicleInfo->type != VH_ANIMAL ) {//vehicles don't use deadmove
		}
		else {
			PM_DeadMove();
		}
	}

	PM_DropTimers();

#ifdef _TESTING_VEH_PREDICTION
#ifndef _GAME
	{
		vector3 blah;
		VectorMA(pm->ps->origin, 128.0f, pm->ps->moveDir, blah);
		CG_TestLine(pm->ps->origin, blah, 1, 0x0000ff, 1);

		VectorMA(pm->ps->origin, 1.0f, pm->ps->velocity, blah);
		CG_TestLine(pm->ps->origin, blah, 1, 0xff0000, 1);
	}
#endif
#endif

	if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&&pm->ps->m_iVehicleNum ) { //a player riding a vehicle
		bgEntity_t *veh = pm_entVeh;

		if ( veh && veh->m_pVehicle &&
			(veh->m_pVehicle->m_pVehicleInfo->type == VH_WALKER || veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER) ) {//*sigh*, until we get forced weapon-switching working?
			pm->cmd.buttons &= ~(BUTTON_ATTACK | BUTTON_ALT_ATTACK);
			pm->ps->eFlags &= ~(EF_FIRING | EF_ALT_FIRING);
			//pm->cmd.weapon = pm->ps->weapon;
		}
	}

	if ( !pm->ps->m_iVehicleNum &&
		pm_entSelf->s.NPC_class != CLASS_VEHICLE&&
		pm_entSelf->s.NPC_class != CLASS_RANCOR&&
		pm->ps->groundEntityNum < ENTITYNUM_WORLD &&
		pm->ps->groundEntityNum >= MAX_CLIENTS ) { //I am a player client, not riding on a vehicle, and potentially standing on an NPC
		bgEntity_t *pEnt = PM_BGEntForNum( pm->ps->groundEntityNum );

		if ( pEnt && pEnt->s.eType == ET_NPC &&
			pEnt->s.NPC_class != CLASS_VEHICLE ) //don't bounce on vehicles
		{ //this is actually an NPC, let's try to bounce of its head to make sure we can't just stand around on top of it.
			if ( pm->ps->velocity.z < 270 ) { //try forcing velocity up and also force him to jump
				pm->ps->velocity.z = 270; //seems reasonable
				pm->cmd.upmove = 127;
			}
		}
#ifdef _GAME
		else if ( !pm->ps->zoomMode &&
			pm_entSelf //I exist
			&& pEnt->m_pVehicle )//ent has a vehicle
		{
			gentity_t *gEnt = (gentity_t*)pEnt;
			if ( gEnt->client
				&& !gEnt->client->ps.m_iVehicleNum //vehicle is empty
				&& (gEnt->spawnflags & 2) )//SUSPENDED
			{//it's a vehicle, see if we should get in it
				//if land on an empty, suspended vehicle, get in it
				pEnt->m_pVehicle->m_pVehicleInfo->Board( pEnt->m_pVehicle, (bgEntity_t *)pm_entSelf );
			}
		}
#endif
	}

	if ( pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE ) { //we are a vehicle
		bgEntity_t *veh = pm_entSelf;

		assert( veh && veh->playerState && veh->m_pVehicle && veh->s.number >= MAX_CLIENTS );

		if ( veh->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER ) { //kind of hacky, don't want to do this for flying vehicles
			veh->m_pVehicle->m_vOrientation->pitch = pm->ps->viewangles.pitch;
		}

		if ( !pm->ps->m_iVehicleNum ) { //no one is driving, just update and get out
#ifdef _GAME
			veh->m_pVehicle->m_pVehicleInfo->Update( veh->m_pVehicle, &pm->cmd );
			veh->m_pVehicle->m_pVehicleInfo->Animate( veh->m_pVehicle );
#endif
		}
		else {
			bgEntity_t *self = pm_entVeh;
#ifdef _GAME
			int i = 0;
#endif

			assert( self && self->playerState && self->s.number < MAX_CLIENTS );

			if ( pm->ps->pm_type == PM_DEAD &&
				(veh->m_pVehicle->m_ulFlags & VEH_CRASHING) ) {
				veh->m_pVehicle->m_ulFlags &= ~VEH_CRASHING;
			}

			if ( self->playerState->m_iVehicleNum ) { //only do it if they still have a vehicle (didn't get ejected this update or something)
				PM_VehicleViewAngles( self->playerState, veh, &veh->m_pVehicle->m_ucmd );
			}

#ifdef _GAME
			veh->m_pVehicle->m_pVehicleInfo->Update( veh->m_pVehicle, &veh->m_pVehicle->m_ucmd );
			veh->m_pVehicle->m_pVehicleInfo->Animate( veh->m_pVehicle );

			veh->m_pVehicle->m_pVehicleInfo->UpdateRider( veh->m_pVehicle, self, &veh->m_pVehicle->m_ucmd );
			//update the passengers
			while ( i < veh->m_pVehicle->m_iNumPassengers ) {
				if ( veh->m_pVehicle->m_ppPassengers[i] ) {
					gentity_t *thePassenger = (gentity_t *)veh->m_pVehicle->m_ppPassengers[i]; //yes, this is, in fact, ass.
					if ( thePassenger->inuse && thePassenger->client ) {
						veh->m_pVehicle->m_pVehicleInfo->UpdateRider( veh->m_pVehicle, veh->m_pVehicle->m_ppPassengers[i], &thePassenger->client->pers.cmd );
					}
				}
				i++;
			}
#else
			if ( !veh->playerState->vehBoarding )//|| veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER ) { //client must explicitly call this for prediction
					BG_FighterUpdate( veh->m_pVehicle, &veh->m_pVehicle->m_ucmd, &pm->mins, &pm->maxs, self->playerState->gravity, pm->trace );
				}

				if ( veh->m_pVehicle->m_iBoarding == 0 ) {
					vector3 vRollAng;

					//make sure we are set as its pilot cgame side
					veh->m_pVehicle->m_pPilot = self;

					// Keep track of the old orientation.
					VectorCopy( veh->m_pVehicle->m_vOrientation, &veh->m_pVehicle->m_vPrevOrientation );

					veh->m_pVehicle->m_pVehicleInfo->ProcessOrientCommands( veh->m_pVehicle );
					PM_SetPMViewAngle( veh->playerState, veh->m_pVehicle->m_vOrientation, &veh->m_pVehicle->m_ucmd );
					veh->m_pVehicle->m_pVehicleInfo->ProcessMoveCommands( veh->m_pVehicle );

					vRollAng.yaw = self->playerState->viewangles.yaw;
					vRollAng.pitch = self->playerState->viewangles.pitch;
					vRollAng.roll = veh->m_pVehicle->m_vOrientation->roll;
					PM_SetPMViewAngle( self->playerState, &vRollAng, &pm->cmd );

					// Setup the move direction.
					if ( veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER ) {
						AngleVectors( veh->m_pVehicle->m_vOrientation, &veh->playerState->moveDir, NULL, NULL );
					}
					else {
						vector3 vVehAngles;

						VectorSet( &vVehAngles, 0, veh->m_pVehicle->m_vOrientation->yaw, 0 );
						AngleVectors( &vVehAngles, &veh->playerState->moveDir, NULL, NULL );
					}
				}
			}
			/*
			else
			{
			veh->playerState->speed = 0.0f;
			PM_SetPMViewAngle(self->playerState, veh->playerState->viewangles, &veh->m_pVehicle->m_ucmd);
			}
			*/
			else if ( veh->playerState ) {
				veh->playerState->speed = 0.0f;
				if ( veh->m_pVehicle ) {
					PM_SetPMViewAngle( self->playerState, veh->m_pVehicle->m_vOrientation, &pm->cmd );
					PM_SetPMViewAngle( veh->playerState, veh->m_pVehicle->m_vOrientation, &pm->cmd );
				}
			}
#endif
		}
		noAnimate = qtrue;
	}

	if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&&pm->ps->m_iVehicleNum ) {//don't even run physics on a player if he's on a vehicle - he goes where the vehicle goes
	}
	else { //don't even run physics on a player if he's on a vehicle - he goes where the vehicle goes
		if ( pm->ps->pm_type == PM_FLOAT
			|| pm_flying == FLY_NORMAL ) {
			PM_FlyMove();
		}
		else if ( pm_flying == FLY_VEHICLE ) {
			PM_FlyVehicleMove();
		}
		else {
			if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
				PM_WaterJumpMove();

			else if ( (pm->ps->pm_flags & PMF_GRAPPLE_PULL) && (pm->cmd.buttons & BUTTON_GRAPPLE) ) {
				PM_GrappleMove();
				PM_AirMove();
			}
			else if ( (pm->ps->eFlags & EF_GRAPPLE_SWING) && !(pm->cmd.buttons & BUTTON_WALKING) ) {
				PM_AirMove();
				PM_GrappleSwing();
			}

			// swimming
			else if ( pm->waterlevel > 1 )
				PM_WaterMove();

			// walking on ground
			else if ( pml.walking )
				PM_WalkMove();

			// airborne
			else
				PM_AirMove();
		}
	}

	if ( !noAnimate ) {
		PM_Animate();
	}

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	if ( pm_flying == FLY_HOVER ) {//never stick to the ground
		PM_HoverTrace();
	}
	PM_SetWaterLevel();
	if ( pm->cmd.forcesel != (byte)-1 && (pm->ps->fd.forcePowersKnown & (1 << pm->cmd.forcesel)) ) {
		pm->ps->fd.forcePowerSelected = pm->cmd.forcesel;
	}
	if ( pm->cmd.invensel != (byte)-1 && (pm->ps->stats[STAT_HOLDABLE_ITEMS] & (1 << pm->cmd.invensel)) ) {
		pm->ps->stats[STAT_HOLDABLE_ITEM] = BG_GetItemIndexByTag( pm->cmd.invensel, IT_HOLDABLE );
	}

	if ( pm->ps->m_iVehicleNum
		/*&&pm_entSelf->s.NPC_class!=CLASS_VEHICLE*/
		&& pm->ps->clientNum < MAX_CLIENTS ) {//a client riding a vehicle
		if ( (pm->ps->eFlags&EF_NODRAW) ) {//inside the vehicle, do nothing
		}
		else if ( !PM_WeaponOkOnVehicle( pm->cmd.weapon ) || !PM_WeaponOkOnVehicle( pm->ps->weapon ) ) { //this weapon is not legal for the vehicle, force to our current one
			if ( !PM_WeaponOkOnVehicle( pm->ps->weapon ) ) { //uh-oh!
				int weap = PM_GetOkWeaponForVehicle();

				if ( weap != -1 ) {
					pm->cmd.weapon = weap;
					pm->ps->weapon = weap;
				}
			}
			else {
				pm->cmd.weapon = pm->ps->weapon;
			}
		}
	}

	if ( !pm->ps->m_iVehicleNum //not a vehicle and not riding one
		|| pm_entSelf->s.NPC_class == CLASS_VEHICLE //you are a vehicle NPC
		|| (!(pm->ps->eFlags&EF_NODRAW) && PM_WeaponOkOnVehicle( pm->cmd.weapon )) )//you're not inside the vehicle and the weapon you're holding can be used when riding this vehicle
	{ //only run weapons if a valid weapon is selected
		// weapons
		PM_Weapon();
	}

	PM_Use();

	if ( !pm->ps->m_iVehicleNum &&
		(pm->ps->clientNum < MAX_CLIENTS ||
		!pm_entSelf ||
		pm_entSelf->s.NPC_class != CLASS_VEHICLE) ) { //don't do this if we're on a vehicle, or we are one
		// footstep events / legs animations
		PM_Footsteps();
	}

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap velocity to integer coordinates to save network bandwidth
	if ( !pm->pmove_float )
		VectorSnap( &pm->ps->velocity );

	if ( pm->ps->pm_type == PM_JETPACK || gPMDoSlowFall ) {
		//	if ( (pm->ps->eFlags & EF_NOT_USED_2) )
		pm->ps->gravity = savedGravity;
	}

	if (//pm->ps->m_iVehicleNum &&
		pm->ps->clientNum >= MAX_CLIENTS &&
		pm_entSelf &&
		pm_entSelf->s.NPC_class == CLASS_VEHICLE ) { //a vehicle with passengers
		bgEntity_t *veh;

		veh = pm_entSelf;

		assert( veh->m_pVehicle );

		//this could be kind of "inefficient" because it's called after every passenger pmove too.
		//Maybe instead of AttachRiders we should have each rider call attach for himself?
		if ( veh->m_pVehicle && veh->ghoul2 ) {
			veh->m_pVehicle->m_pVehicleInfo->AttachRiders( veh->m_pVehicle );
		}
	}

	if ( pm_entSelf->s.NPC_class != CLASS_VEHICLE
		&& pm->ps->m_iVehicleNum ) { //riding a vehicle, see if we should do some anim overrides
		PM_VehicleWeaponAnimate();
	}
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t *pmove ) {
	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	//Raz: Pmove smoothing
	if ( pmove->pmove_fixed && finalTime < pmove->ps->commandTime + pmove->pmove_msec )
		return;

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	if ( pmove->ps->fallingToDeath ) {
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
		pmove->cmd.buttons = 0;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec )
				msec = pmove->pmove_msec;
		}
		else {
			if ( msec > 66 )
				msec = 66;
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;

		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD )
			pmove->cmd.upmove = 20;
	}
}

