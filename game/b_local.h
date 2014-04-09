//B_local.h
//re-added by MCG
#ifndef __B_LOCAL_H__
#define __B_LOCAL_H__

#include "g_local.h"
#include "b_public.h"
#include "say.h"

#include "ai.h"

#define	AI_TIMERS 0//turn on to see print-outs of AI/nav timing
//
// Navigation susbsystem
//

#define NAVF_DUCK			(0x0001u)
#define NAVF_JUMP			(0x0002u)
#define NAVF_HOLD			(0x0004u)
#define NAVF_SLOW			(0x0008u)

#define DEBUG_LEVEL_DETAIL	4
#define DEBUG_LEVEL_INFO	3
#define DEBUG_LEVEL_WARNING	2
#define DEBUG_LEVEL_ERROR	1
#define DEBUG_LEVEL_NONE	0

#define MAX_GOAL_REACHED_DIST_SQUARED	256//16 squared
#define MIN_ANGLE_ERROR 0.01f

#define MIN_ROCKET_DIST_SQUARED 16384//128*128
//
// NPC.cpp
//
// ai debug cvars
void SetNPCGlobals( gentity_t *ent );
void SaveNPCGlobals( void );
void RestoreNPCGlobals( void );

void NPC_Think( gentity_t *self );

//NPC_reactions.cpp
typedef void (PAIN_FUNC)( gentity_t *self, gentity_t *attacker, int damage );
extern PAIN_FUNC funcBBrushPain, misc_model_breakable_pain, NPC_Pain, station_pain, func_usable_pain, NPC_ATST_Pain,
NPC_ST_Pain, NPC_Jedi_Pain, NPC_Droid_Pain, NPC_Probe_Pain, NPC_MineMonster_Pain, NPC_Howler_Pain, NPC_Seeker_Pain,
NPC_Remote_Pain, emplaced_gun_pain, NPC_Mark1_Pain, NPC_GM_Pain, NPC_Sentry_Pain, NPC_Mark2_Pain, PlayerPain, GasBurst,
CrystalCratePain, TurretPain, NPC_Wampa_Pain, NPC_Rancor_Pain;

void NPC_Touch( gentity_t *self, gentity_t *other, trace_t *trace );
void NPC_Use( gentity_t *self, gentity_t *other, gentity_t *activator );
float NPC_GetPainChance( gentity_t *self, int damage );

//
// NPC_misc.cpp
//
void Debug_Printf( vmCvar_t *cv, int level, const char *fmt, ... );
void Debug_NPCPrintf( gentity_t *printNPC, vmCvar_t *cv, int debugLevel, const char *fmt, ... );

//MCG - Begin============================================================
//NPC_ai variables - shared by NPC.cpp andf the following modules
extern gentity_t	*NPC;
extern gNPC_t		*NPCInfo;
extern gclient_t	*client;
extern usercmd_t	ucmd;
extern visibility_t	enemyVisibility;

//AI_Default
qboolean NPC_CheckInvestigate( int alertEventNum );
qboolean NPC_StandTrackAndShoot( gentity_t *NPC, qboolean canDuck );
void NPC_BSIdle( void );
void NPC_BSPointShoot( qboolean shoot );
void NPC_BSStandGuard( void );
void NPC_BSPatrol( void );
void NPC_BSHuntAndKill( void );
void NPC_BSStandAndShoot( void );
void NPC_BSRunAndShoot( void );
void NPC_BSWait( void );
void NPC_BSDefault( void );

//NPC_behavior
void NPC_BSAdvanceFight( void );
void NPC_BSInvestigate( void );
void NPC_BSSleep( void );
void NPC_BSFlee( void );
void NPC_BSFollowLeader( void );
void NPC_BSJump( void );
void NPC_BSRemove( void );
void NPC_BSSearch( void );
void NPC_BSSearchStart( int	homeWp, bState_t bState );
void NPC_BSWander( void );
void NPC_StartFlee( gentity_t *enemy, vector3 *dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );
void G_StartFlee( gentity_t *self, gentity_t *enemy, vector3 *dangerPoint, int dangerLevel, int fleeTimeMin, int fleeTimeMax );

//NPC_combat
int ChooseBestWeapon( void );
void NPC_ChangeWeapon( int newWeapon );
void ShootThink( void );
void WeaponThink( qboolean inCombat );
qboolean HaveWeapon( int weapon );
qboolean CanShoot( gentity_t *ent, gentity_t *shooter );
void NPC_CheckPossibleEnemy( gentity_t *other, visibility_t vis );
gentity_t *NPC_PickEnemy( gentity_t *closestTo, int enemyTeam, qboolean checkVis, qboolean findPlayersFirst, qboolean findClosest );
gentity_t *NPC_CheckEnemy( qboolean findNew, qboolean tooFarOk, qboolean setEnemy ); //setEnemy = qtrue
qboolean NPC_CheckAttack( float scale );
qboolean NPC_CheckDefend( float scale );
qboolean NPC_CheckCanAttack( float attack_scale, qboolean stationary );
int NPC_AttackDebounceForWeapon( void );
qboolean EntIsGlass( gentity_t *check );
qboolean ShotThroughGlass( trace_t *tr, gentity_t *target, vector3 *spot, int mask );
qboolean ValidEnemy( gentity_t *ent );
void G_ClearEnemy( gentity_t *self );
gentity_t *NPC_PickAlly( qboolean facingEachOther, float range, qboolean ignoreGroup, qboolean movingOnly );
void NPC_LostEnemyDecideChase( void );
float NPC_MaxDistSquaredForWeapon( void );
qboolean NPC_EvaluateShot( int hit, qboolean glassOK );
int NPC_ShotEntity( gentity_t *ent, vector3 *impactPos ); //impactedPos = NULL

//NPC_formation
qboolean NPC_SlideMoveToGoal( void );
float NPC_FindClosestTeammate( gentity_t *self );
void NPC_CalcClosestFormationSpot( gentity_t *self );
void G_MaintainFormations( gentity_t *self );
void NPC_BSFormation( void );
void NPC_CreateFormation( gentity_t *self );
void NPC_DropFormation( gentity_t *self );
void NPC_ReorderFormation( gentity_t *self );
void NPC_InsertIntoFormation( gentity_t *self );
void NPC_DeleteFromFormation( gentity_t *self );

#define COLLISION_RADIUS 32
#define NUM_POSITIONS 30

//NPC spawnflags
#define SFB_SMALLHULL	1

#define SFB_RIFLEMAN	2
#define SFB_OLDBORG		2//Borg
#define SFB_PHASER		4
#define SFB_GUN			4//Borg
#define	SFB_TRICORDER	8
#define	SFB_TASER		8//Borg
#define	SFB_DRILL		16//Borg

#define	SFB_CINEMATIC	32
#define	SFB_NOTSOLID	64
#define	SFB_STARTINSOLID 128

//NPC_goal
void SetGoal( gentity_t *goal, float rating );
void NPC_SetGoal( gentity_t *goal, float rating );
void NPC_ClearGoal( void );
void NPC_ReachedGoal( void );
qboolean ReachedGoal( gentity_t *goal );
gentity_t *UpdateGoal( void );
qboolean NPC_ClearPathToGoal( vector3 *dir, gentity_t *goal );
qboolean NPC_MoveToGoal( qboolean tryStraight );

//NPC_reactions

//NPC_senses
#define	ALERT_CLEAR_TIME	200
#define CHECK_PVS		(0x0001u)
#define CHECK_360		(0x0002u)
#define CHECK_FOV		(0x0004u)
#define CHECK_SHOOT		(0x0008u)
#define CHECK_VISRANGE	(0x0010u)
qboolean CanSee( gentity_t *ent );
qboolean InFOV( gentity_t *ent, gentity_t *from, int hFOV, int vFOV );
qboolean InFOV2( vector3 *origin, gentity_t *from, int hFOV, int vFOV );
qboolean InFOV3( vector3 *spot, vector3 *from, vector3 *fromAngles, int hFOV, int vFOV );
visibility_t NPC_CheckVisibility( gentity_t *ent, uint32_t flags );
qboolean InVisrange( gentity_t *ent );

//NPC_spawn
void NPC_Spawn( gentity_t *ent, gentity_t *other, gentity_t *activator );

//NPC_stats
int NPC_ReactionTime( void );
qboolean NPC_ParseParms( const char *NPCName, gentity_t *NPC );
void NPC_LoadParms( void );

//NPC_utils
extern int teamNumbers[TEAM_NUM_TEAMS];
extern int teamStrength[TEAM_NUM_TEAMS];
extern int teamCounter[TEAM_NUM_TEAMS];

void CalcEntitySpot( const gentity_t *ent, const spot_t spot, vector3 *point );
qboolean NPC_UpdateAngles( qboolean doPitch, qboolean doYaw );
void NPC_UpdateShootAngles( vector3 *angles, qboolean doPitch, qboolean doYaw );
qboolean NPC_UpdateFiringAngles( qboolean doPitch, qboolean doYaw );
void SetTeamNumbers( void );
void NPC_AimWiggle( vector3 *enemy_org );
void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );

//g_nav.cpp
int NAV_FindClosestWaypointForEnt( gentity_t *ent, int targWp );
qboolean NAV_CheckAhead( gentity_t *self, vector3 *end, trace_t *trace, int clipmask );

//NPC_combat
float IdealDistance( gentity_t *self );

//g_squad
void NPC_SetSayState( gentity_t *self, gentity_t *to, int saying );

//MCG - End============================================================

// NPC.cpp
void NPC_SetAnim( gentity_t *ent, int type, int anim, uint32_t setAnimFlags );
qboolean NPC_EnemyTooFar( gentity_t *enemy, float dist, qboolean toShoot );

// ==================================================================

//rww - special system for sync'ing bone angles between client and server.
void NPC_SetBoneAngles( gentity_t *ent, const char *bone, vector3 *angles );

//rww - and another method of automatically managing surface status for the client and server at once
void NPC_SetSurfaceOnOff( gentity_t *ent, const char *surfaceName, uint32_t surfaceFlags );

qboolean NPC_ClearLOS( const vector3 *start, const vector3 *end );
qboolean NPC_ClearLOS5( const vector3 *end );
qboolean NPC_ClearLOS4( gentity_t *ent );
qboolean NPC_ClearLOS3( const vector3 *start, gentity_t *ent );
qboolean NPC_ClearLOS2( gentity_t *ent, const vector3 *end );

qboolean NPC_ClearShot( gentity_t *ent );

int NPC_FindCombatPoint( const vector3 *position, const vector3 *avoidPosition, vector3 *enemyPosition, const uint32_t flags, const float avoidDist, const int ignorePoint ); //ignorePoint = -1


qboolean NPC_ReserveCombatPoint( int combatPointID );
qboolean NPC_FreeCombatPoint( int combatPointID, qboolean failed ); //failed = qfalse
qboolean NPC_SetCombatPoint( int combatPointID );

#define	CP_ANY				(0x00000000u) // No flags
#define	CP_COVER			(0x00000001u) // The enemy cannot currently shoot this position
#define CP_CLEAR			(0x00000002u) // This cover point has a clear shot to the enemy
#define CP_FLEE				(0x00000004u) // This cover point is marked as a flee point
#define CP_DUCK				(0x00000008u) // This cover point is marked as a duck point
#define CP_NEAREST			(0x00000010u) // Find the nearest combat point
#define CP_AVOID_ENEMY		(0x00000020u) // Avoid our enemy
#define CP_INVESTIGATE		(0x00000040u) // A special point worth enemy investigation if searching
#define	CP_SQUAD			(0x00000080u) // Squad path
#define	CP_AVOID			(0x00000100u) // Avoid supplied position
#define	CP_APPROACH_ENEMY	(0x00000200u) // Try to get closer to enemy
#define	CP_CLOSEST			(0x00000400u) // Take the closest combatPoint to the enemy that's available
#define	CP_FLANK			(0x00000800u) // Pick a combatPoint behind the enemy
#define	CP_HAS_ROUTE		(0x00001000u) // Pick a combatPoint that we have a route to
#define	CP_SNIPE			(0x00002000u) // Pick a combatPoint that is marked as a sniper spot
#define	CP_SAFE				(0x00004000u) // Pick a combatPoint that is not have dangerTime
#define	CP_HORZ_DIST_COLL	(0x00008000u) // Collect combat points within *horizontal* dist
#define	CP_NO_PVS			(0x00010000u) // A combat point out of the PVS of enemy pos
#define	CP_RETREAT			(0x00020000u) // Try to get farther from enemy

#define CPF_NONE		(0x0000u)
#define	CPF_DUCK		(0x0001u)
#define	CPF_FLEE		(0x0002u)
#define	CPF_INVESTIGATE	(0x0004u)
#define	CPF_SQUAD		(0x0008u)
#define	CPF_LEAN		(0x0010u)
#define	CPF_SNIPE		(0x0020u)

#define	MAX_COMBAT_POINT_CHECK	32

qboolean NPC_ValidEnemy( gentity_t *ent );
qboolean NPC_CheckEnemyExt( qboolean checkAlerts ); //checkAlerts = qfalse
qboolean NPC_FindPlayer( void );
qboolean NPC_CheckCanAttackExt( void );

int NPC_CheckAlertEvents( qboolean checkSight, qboolean checkSound, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel ); //ignoreAlert = -1, mustHaveOwner = qfalse, minAlertLevel = AEL_MINOR
qboolean NPC_CheckForDanger( int alertEvent );
void G_AlertTeam( gentity_t *victim, gentity_t *attacker, float radius, float soundDist );

int NPC_FindSquadPoint( vector3 *position );

void ClearPlayerAlertEvents( void );

qboolean G_BoundsOverlap( const vector3 *mins1, const vector3 *maxs1, const vector3 *mins2, const vector3 *maxs2 );
qboolean NAV_HitNavGoal( vector3 *point, vector3 *mins, vector3 *maxs, vector3 *dest, int radius, qboolean flying );

void NPC_SetMoveGoal( gentity_t *ent, vector3 *point, int radius, qboolean isNavGoal, int combatPoint, gentity_t *targetEnt ); //isNavGoal = qfalse, combatPoint = -1, targetEnt = NULL

qboolean NAV_ClearPathToPoint( gentity_t *self, vector3 *pmins, vector3 *pmaxs, vector3 *point, int clipmask, int okToHitEnt );
void NPC_ApplyWeaponFireDelay( void );

//NPC_FaceXXX suite
qboolean NPC_FacePosition( vector3 *position, qboolean doPitch ); //doPitch = qtrue
qboolean NPC_FaceEntity( gentity_t *ent, qboolean doPitch ); //doPitch = qtrue
qboolean NPC_FaceEnemy( qboolean doPitch ); //doPitch = qtrue

#define	NIF_NONE		(0x0000u)
#define	NIF_FAILED		(0x0001u)	//failed to find a way to the goal
#define	NIF_MACRO_NAV	(0x0002u)	//using macro navigation
#define	NIF_COLLISION	(0x0004u)	//resolving collision with an entity
#define NIF_BLOCKED		(0x0008u)	//blocked from moving

/*
-------------------------
struct navInfo_s
-------------------------
*/

typedef struct navInfo_s {
	gentity_t	*blocker;
	vector3		direction;
	vector3		pathDirection;
	float		distance;
	trace_t		trace;
	uint32_t	flags;
} navInfo_t;

int	NAV_MoveToGoal( gentity_t *self, navInfo_t *info );
void NAV_GetLastMove( navInfo_t *info );
qboolean NAV_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info );


#endif
