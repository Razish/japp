//b_goal.cpp
#include "b_local.h"
#include "ICARUS/Q3_Interface.h"

extern qboolean FlyingCreature( gentity_t *ent );
/*
SetGoal
*/

void SetGoal( gentity_t *goal, float rating )
{
	NPCInfo->goalEntity = goal;
//	NPCInfo->goalEntityNeed = rating;
	NPCInfo->goalTime = level.time;
//	NAV_ClearLastRoute(NPC);
	if ( goal )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: %s @ %s (%f)\n", goal->classname, vtos( goal->currentOrigin), rating );
	}
	else
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: NONE\n" );
	}
}


/*
NPC_SetGoal
*/

void NPC_SetGoal( gentity_t *goal, float rating )
{
	if ( goal == NPCInfo->goalEntity )
	{
		return;
	}

	if ( !goal )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_ERROR, "NPC_SetGoal: NULL goal\n" );
		return;
	}

	if ( goal->client )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_ERROR, "NPC_SetGoal: goal is a client\n" );
		return;
	}

	if ( NPCInfo->goalEntity )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_SetGoal: push %s\n", NPCInfo->goalEntity->classname );
		NPCInfo->lastGoalEntity = NPCInfo->goalEntity;
//		NPCInfo->lastGoalEntityNeed = NPCInfo->goalEntityNeed;
	}

	SetGoal( goal, rating );
}


/*
NPC_ClearGoal
*/

void NPC_ClearGoal( void )
{
	gentity_t	*goal;

	if ( !NPCInfo->lastGoalEntity )
	{
		SetGoal( NULL, 0.0 );
		return;
	}

	goal = NPCInfo->lastGoalEntity;
	NPCInfo->lastGoalEntity = NULL;
//	NAV_ClearLastRoute(NPC);
	if ( goal->inuse && !(goal->s.eFlags & EF_NODRAW) )
	{
//		Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "NPC_ClearGoal: pop %s\n", goal->classname );
		SetGoal( goal, 0 );//, NPCInfo->lastGoalEntityNeed
		return;
	}

	SetGoal( NULL, 0.0 );
}

/*
-------------------------
G_BoundsOverlap
-------------------------
*/

qboolean G_BoundsOverlap(const vector3 *mins1, const vector3 *maxs1, const vector3 *mins2, const vector3 *maxs2)
{//NOTE: flush up against counts as overlapping
	if ( mins1->x > maxs2->x ||
		 mins1->y > maxs2->y ||
		 mins1->z > maxs2->z ||
		 maxs1->x < mins2->x ||
		 maxs1->y < mins2->y ||
		 maxs1->z < mins2->z )
		return qfalse;

	return qtrue;
}

void NPC_ReachedGoal( void )
{
//	Debug_NPCPrintf( NPC, d_npcai, DEBUG_LEVEL_INFO, "UpdateGoal: reached goal entity\n" );
	NPC_ClearGoal();
	NPCInfo->goalTime = level.time;

//MCG - Begin
	NPCInfo->aiFlags &= ~NPCAI_MOVING;
	ucmd.forwardmove = 0;
	//Return that the goal was reached
	trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_MOVE_NAV );
//MCG - End
}
/*
ReachedGoal

id removed checks against waypoints and is now checking surfaces
*/
//qboolean NAV_HitNavGoal( vector3 *point, vector3 *mins, vector3 *maxs, gentity_t *goal, qboolean flying );
qboolean ReachedGoal( gentity_t *goal )
{
	if ( NPCInfo->aiFlags & NPCAI_TOUCHED_GOAL )
	{
		NPCInfo->aiFlags &= ~NPCAI_TOUCHED_GOAL;
		return qtrue;
	}
	return NAV_HitNavGoal( &NPC->r.currentOrigin, &NPC->r.mins, &NPC->r.maxs, &goal->r.currentOrigin, NPCInfo->goalRadius, FlyingCreature( NPC ) );
}

/*
static gentity_t *UpdateGoal( void )

Id removed a lot of shit here... doesn't seem to handle waypoints independantly of goalentity

In fact, doesn't seem to be any waypoint info on entities at all any more?

MCG - Since goal is ALWAYS goalEntity, took out a lot of sending goal entity pointers around for no reason
*/

gentity_t *UpdateGoal( void )
{
	gentity_t	*goal;

	if ( !NPCInfo->goalEntity )
	{
		return NULL;
	}

	if ( !NPCInfo->goalEntity->inuse )
	{//Somehow freed it, but didn't clear it
		NPC_ClearGoal();
		return NULL;
	}

	goal = NPCInfo->goalEntity;

	if ( ReachedGoal( goal ) )
	{
		NPC_ReachedGoal();
		goal = NULL;//so they don't keep trying to move to it
	}//else if fail, need to tell script so?

	return goal;
}
