#include "b_local.h"
#include "g_nav.h"
#include "ICARUS/Q3_Interface.h"

extern qboolean NPC_SomeoneLookingAtMe( gentity_t *ent );

/*
void NPC_LostEnemyDecideChase(void)

We lost our enemy and want to drop him but see if we should chase him if we are in the proper bState
*/

void NPC_LostEnemyDecideChase( void ) {
	switch ( NPCInfo->behaviorState ) {
	case BS_HUNT_AND_KILL:
		//We were chasing him and lost him, so try to find him
		if ( NPC->enemy == NPCInfo->goalEntity && NPC->enemy->lastWaypoint != WAYPOINT_NONE ) {//Remember his last valid Wp, then check it out
			//FIXME: Should we only do this if there's no other enemies or we've got LOCKED_ENEMY on?
			NPC_BSSearchStart( NPC->enemy->lastWaypoint, BS_SEARCH );
		}
		//If he's not our goalEntity, we're running somewhere else, so lose him
		break;
	default:
		break;
	}
	G_ClearEnemy( NPC );
}
/*
-------------------------
NPC_StandIdle
-------------------------
*/

void NPC_StandIdle( void ) {
	/*
		//Must be done with any other animations
		if ( NPC->client->ps.legsAnimTimer != 0 )
		return;

		//Not ready to do another one
		if ( TIMER_Done( NPC, "idleAnim" ) == false )
		return;

		int anim = NPC->client->ps.legsAnim;

		if ( anim != BOTH_STAND1 && anim != BOTH_STAND2 )
		return;

		//FIXME: Account for STAND1 or STAND2 here and set the base anim accordingly
		int	baseSeq = ( anim == BOTH_STAND1 ) ? BOTH_STAND1_RANDOM1 : BOTH_STAND2_RANDOM1;

		//Must have at least one random idle animation
		//NOTENOTE: This relies on proper ordering of animations, which SHOULD be okay
		if ( PM_HasAnimation( NPC, baseSeq ) == false )
		return;

		int	newIdle = Q_irand( 0, MAX_IDLE_ANIMS-1 );

		//FIXME: Technically this could never complete.. but that's not really too likely
		while( 1 )
		{
		if ( PM_HasAnimation( NPC, baseSeq + newIdle ) )
		break;

		newIdle = Q_irand( 0, MAX_IDLE_ANIMS );
		}

		//Start that animation going
		NPC_SetAnim( NPC, SETANIM_BOTH, baseSeq + newIdle, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );

		int newTime = PM_AnimLength( NPC->client->clientInfo.animFileIndex, (animNumber_t) (baseSeq + newIdle) );

		//Don't do this again for a random amount of time
		TIMER_Set( NPC, "idleAnim", newTime + Q_irand( 2000, 10000 ) );
		*/
}

qboolean NPC_StandTrackAndShoot( gentity_t *NPC, qboolean canDuck ) {
	qboolean	attack_ok = qfalse;
	qboolean	duck_ok = qfalse;
	qboolean	faced = qfalse;
	float		attack_scale = 1.0f;

	//First see if we're hurt bad- if so, duck
	//FIXME: if even when ducked, we can shoot someone, we should.
	//Maybe is can be shot even when ducked, we should run away to the nearest cover?
	if ( canDuck ) {
		if ( NPC->health < 20 )
			duck_ok = qtrue;
	}

	//NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( !duck_ok ) {//made this whole part a function call
		attack_ok = NPC_CheckCanAttack( attack_scale, qtrue );
		faced = qtrue;
	}

	if ( canDuck && (duck_ok || (!attack_ok && client->ps.weaponTime <= 0)) && ucmd.upmove != -127 ) {//if we didn't attack check to duck if we're not already
		if ( !duck_ok ) {
			if ( NPC->enemy->client ) {
				if ( NPC->enemy->enemy == NPC ) {
					if ( NPC->enemy->client->buttons & BUTTON_ATTACK ) {//FIXME: determine if enemy fire angles would hit me or get close
						if ( NPC_CheckDefend( 1.0f ) )//FIXME: Check self-preservation?  Health?
						{
							duck_ok = qtrue;
						}
					}
				}
			}
		}

		if ( duck_ok ) {//duck and don't shoot
			ucmd.upmove = -127;
			NPCInfo->duckDebounceTime = level.time + 1000;//duck for a full second
		}
	}

	return faced;
}


void NPC_BSIdle( void ) {
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() ) {
		NPC_MoveToGoal( qtrue );
	}

	if ( (ucmd.forwardmove == 0) && (ucmd.rightmove == 0) && (ucmd.upmove == 0) ) {
		//		NPC_StandIdle();
	}

	NPC_UpdateAngles( qtrue, qtrue );
	ucmd.buttons |= BUTTON_WALKING;
}

void NPC_BSRun( void ) {
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() ) {
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

void NPC_BSStandGuard( void ) {
	//FIXME: Use Snapshot info
	if ( NPC->enemy == NULL ) {//Possible to pick one up by being shot
		if ( random() < 0.5f ) {
			if ( NPC->client->enemyTeam ) {
				gentity_t *newenemy = NPC_PickEnemy( NPC, NPC->client->enemyTeam, (NPC->cantHitEnemyCounter < 10), (NPC->client->enemyTeam == NPCTEAM_PLAYER), qtrue );
				//only checks for vis if couldn't hit last enemy
				if ( newenemy ) {
					G_SetEnemy( NPC, newenemy );
				}
			}
		}
	}

	if ( NPC->enemy != NULL ) {
		if ( NPCInfo->tempBehavior == BS_STAND_GUARD ) {
			NPCInfo->tempBehavior = BS_DEFAULT;
		}

		if ( NPCInfo->behaviorState == BS_STAND_GUARD ) {
			NPCInfo->behaviorState = BS_STAND_AND_SHOOT;
		}
	}

	NPC_UpdateAngles( qtrue, qtrue );
}

/*
-------------------------
NPC_BSHuntAndKill
-------------------------
*/

void NPC_BSHuntAndKill( void ) {
	qboolean	turned = qfalse;
	vector3		vec;
	float		enemyDist;
	visibility_t	oEVis;
	int			curAnim;

	NPC_CheckEnemy( NPCInfo->tempBehavior != BS_HUNT_AND_KILL, qfalse, qtrue );//don't find new enemy if this is tempbehav

	if ( NPC->enemy ) {
		oEVis = enemyVisibility = NPC_CheckVisibility( NPC->enemy, CHECK_FOV | CHECK_SHOOT );//CHECK_360|//CHECK_PVS|
		if ( enemyVisibility > VIS_PVS ) {
			if ( !NPC_EnemyTooFar( NPC->enemy, 0, qtrue ) ) {//Enemy is close enough to shoot - FIXME: this next func does this also, but need to know here for info on whether ot not to turn later
				NPC_CheckCanAttack( 1.0f, qfalse );
				turned = qtrue;
			}
		}

		curAnim = NPC->client->ps.legsAnim;
		if ( curAnim != BOTH_ATTACK1 && curAnim != BOTH_ATTACK2 && curAnim != BOTH_ATTACK3 && curAnim != BOTH_MELEE1 && curAnim != BOTH_MELEE2 ) {//Don't move toward enemy if we're in a full-body attack anim
			//FIXME, use IdealDistance to determin if we need to close distance
			VectorSubtract( &NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &vec );
			enemyDist = VectorLength( &vec );
			if ( enemyDist > 48 && ((enemyDist*1.5f)*(enemyDist*1.5f) >= NPC_MaxDistSquaredForWeapon() ||
				oEVis != VIS_SHOOT ||
				//!(ucmd.buttons & BUTTON_ATTACK) ||
				enemyDist > IdealDistance( NPC ) * 3) ) {//We should close in?
				NPCInfo->goalEntity = NPC->enemy;

				NPC_MoveToGoal( qtrue );
			}
			else if ( enemyDist < IdealDistance( NPC ) ) {//We should back off?
				//if(ucmd.buttons & BUTTON_ATTACK)
				{
					NPCInfo->goalEntity = NPC->enemy;
					NPCInfo->goalRadius = 12;
					NPC_MoveToGoal( qtrue );

					ucmd.forwardmove *= -1;
					ucmd.rightmove *= -1;
					VectorScale( &NPC->client->ps.moveDir, -1, &NPC->client->ps.moveDir );

					ucmd.buttons |= BUTTON_WALKING;
				}
			}//otherwise, stay where we are
		}
	}
	else {//ok, stand guard until we find an enemy
		if ( NPCInfo->tempBehavior == BS_HUNT_AND_KILL ) {
			NPCInfo->tempBehavior = BS_DEFAULT;
		}
		else {
			NPCInfo->tempBehavior = BS_STAND_GUARD;
			NPC_BSStandGuard();
		}
		return;
	}

	if ( !turned ) {
		NPC_UpdateAngles( qtrue, qtrue );
	}
}

void NPC_BSStandAndShoot( void ) {
	//FIXME:
	//When our numbers outnumber enemies 3 to 1, or only one of them,
	//go into hunt and kill mode

	//FIXME:
	//When they're all dead, go to some script or wander off to sickbay?

	if ( NPC->client->playerTeam && NPC->client->enemyTeam ) {
		//FIXME: don't realize this right away- or else enemies show up and we're standing around
		/*
		if( teamNumbers[NPC->enemyTeam] == 0 )
		{//ok, stand guard until we find another enemy
		//reset our rush counter
		teamCounter[NPC->playerTeam] = 0;
		NPCInfo->tempBehavior = BS_STAND_GUARD;
		NPC_BSStandGuard();
		return;
		}*/
		/*
		//FIXME: whether to do this or not should be settable
		else if( NPC->playerTeam != TEAM_BORG )//Borg don't rush
		{
		//FIXME: In case reinforcements show up, we should wait a few seconds
		//and keep checking before rushing!
		//Also: what if not everyone on our team is going after playerTeam?
		//Also: our team count includes medics!
		if(NPC->health > 25)
		{//Can we rush the enemy?
		if(teamNumbers[NPC->enemyTeam] == 1 ||
		teamNumbers[NPC->playerTeam] >= teamNumbers[NPC->enemyTeam]*3)
		{//Only one of them or we outnumber 3 to 1
		if(teamStrength[NPC->playerTeam] >= 75 ||
		(teamStrength[NPC->playerTeam] >= 50 && teamStrength[NPC->playerTeam] > teamStrength[NPC->enemyTeam]))
		{//Our team is strong enough to rush
		teamCounter[NPC->playerTeam]++;
		if(teamNumbers[NPC->playerTeam] * 17 <= teamCounter[NPC->playerTeam])
		{//ok, we waited 1.7f think cycles on average and everyone is go, let's do it!
		//FIXME: Should we do this to everyone on our team?
		NPCInfo->behaviorState = BS_HUNT_AND_KILL;
		//FIXME: if the tide changes, we should retreat!
		//FIXME: when do we reset the counter?
		NPC_BSHuntAndKill ();
		return;
		}
		}
		else//Oops!  Something's wrong, reset the counter to rush
		teamCounter[NPC->playerTeam] = 0;
		}
		else//Oops!  Something's wrong, reset the counter to rush
		teamCounter[NPC->playerTeam] = 0;
		}
		}
		*/
	}

	NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( NPCInfo->duckDebounceTime > level.time && NPC->client->ps.weapon != WP_SABER ) {
		ucmd.upmove = -127;
		if ( NPC->enemy ) {
			NPC_CheckCanAttack( 1.0f, qtrue );
		}
		return;
	}

	if ( NPC->enemy ) {
		if ( !NPC_StandTrackAndShoot( NPC, qtrue ) ) {//That func didn't update our angles
			NPCInfo->desiredYaw = NPC->client->ps.viewangles.yaw;
			NPCInfo->desiredPitch = NPC->client->ps.viewangles.pitch;
			NPC_UpdateAngles( qtrue, qtrue );
		}
	}
	else {
		NPCInfo->desiredYaw = NPC->client->ps.viewangles.yaw;
		NPCInfo->desiredPitch = NPC->client->ps.viewangles.pitch;
		NPC_UpdateAngles( qtrue, qtrue );
		//		NPC_BSIdle();//only moves if we have a goal
	}
}

void NPC_BSRunAndShoot( void ) {
	/*if(NPC->playerTeam && NPC->enemyTeam)
	{
	//FIXME: don't realize this right away- or else enemies show up and we're standing around
	if( teamNumbers[NPC->enemyTeam] == 0 )
	{//ok, stand guard until we find another enemy
	//reset our rush counter
	teamCounter[NPC->playerTeam] = 0;
	NPCInfo->tempBehavior = BS_STAND_GUARD;
	NPC_BSStandGuard();
	return;
	}
	}*/

	//NOTE: are we sure we want ALL run and shoot people to move this way?
	//Shouldn't it check to see if we have an enemy and our enemy is our goal?!
	//Moved that check into NPC_MoveToGoal
	//NPCInfo->combatMove = qtrue;

	NPC_CheckEnemy( qtrue, qfalse, qtrue );

	if ( NPCInfo->duckDebounceTime > level.time ) // && NPCInfo->hidingGoal )
	{
		ucmd.upmove = -127;
		if ( NPC->enemy ) {
			NPC_CheckCanAttack( 1.0f, qfalse );
		}
		return;
	}

	if ( NPC->enemy ) {
		int monitor = NPC->cantHitEnemyCounter;
		NPC_StandTrackAndShoot( NPC, qfalse );//(NPCInfo->hidingGoal != NULL) );

		if ( !(ucmd.buttons & BUTTON_ATTACK) && ucmd.upmove >= 0 && NPC->cantHitEnemyCounter > monitor ) {//not crouching and not firing
			vector3	vec;

			VectorSubtract( &NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &vec );
			vec.z = 0;
			if ( VectorLength( &vec ) > 128 || NPC->cantHitEnemyCounter >= 10 ) {//run at enemy if too far away
				//The cantHitEnemyCounter getting high has other repercussions
				//100 (10 seconds) will make you try to pick a new enemy...
				//But we're chasing, so we clamp it at 50 here
				if ( NPC->cantHitEnemyCounter > 60 )
					NPC->cantHitEnemyCounter = 60;

				if ( NPC->cantHitEnemyCounter >= (NPCInfo->stats.aggression + 1) * 10 )
					NPC_LostEnemyDecideChase();

				//chase and face
				ucmd.angles.yaw = 0;
				ucmd.angles.pitch = 0;
				NPCInfo->goalEntity = NPC->enemy;
				NPCInfo->goalRadius = 12;
				//NAV_ClearLastRoute(NPC);
				NPC_MoveToGoal( qtrue );
				NPC_UpdateAngles( qtrue, qtrue );
			}
			else {
				//FIXME: this could happen if they're just on the other side
				//of a thin wall or something else blocking out shot.  That
				//would make us just stand there and not go around it...
				//but maybe it's okay- might look like we're waiting for
				//him to come out...?
				//Current solution: runs around if cantHitEnemyCounter gets
				//to 10 (1 second).
			}
		}
		else {//Clear the can't hit enemy counter here
			NPC->cantHitEnemyCounter = 0;
		}
	}
	else {
		if ( NPCInfo->tempBehavior == BS_HUNT_AND_KILL ) {//lost him, go back to what we were doing before
			NPCInfo->tempBehavior = BS_DEFAULT;
			return;
		}

		//		NPC_BSRun();//only moves if we have a goal
	}
}

//Simply turn until facing desired angles
void NPC_BSFace( void ) {
	//FIXME: once you stop sending turning info, they reset to whatever their delta_angles was last????
	//Once this is over, it snaps back to what it was facing before- WHY???
	if ( NPC_UpdateAngles( qtrue, qtrue ) ) {
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_BSTATE );

		NPCInfo->desiredYaw = client->ps.viewangles.yaw;
		NPCInfo->desiredPitch = client->ps.viewangles.pitch;

		NPCInfo->aimTime = 0;//ok to turn normally now
	}
}

void NPC_BSPointShoot( qboolean shoot ) {//FIXME: doesn't check for clear shot...
	vector3	muzzle, dir, angles, org;

	if ( !NPC->enemy || !NPC->enemy->inuse || (NPC->enemy->NPC && NPC->enemy->health <= 0) ) {//FIXME: should still keep shooting for a second or two after they actually die...
		trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_BSTATE );
		goto finished;
	}

	CalcEntitySpot( NPC, SPOT_WEAPON, &muzzle );
	CalcEntitySpot( NPC->enemy, SPOT_HEAD, &org );//Was spot_org
	//Head is a little high, so let's aim for the chest:
	if ( NPC->enemy->client ) {
		org.z -= 12;//NOTE: is this enough?
	}

	VectorSubtract( &org, &muzzle, &dir );
	vectoangles( &dir, &angles );

	switch ( NPC->client->ps.weapon ) {
	case WP_NONE:
		//	case WP_TRICORDER:
	case WP_STUN_BATON:
	case WP_SABER:
		//don't do any pitch change if not holding a firing weapon
		break;
	default:
		NPCInfo->desiredPitch = NPCInfo->lockedDesiredPitch = AngleNormalize360( angles.pitch );
		break;
	}

	NPCInfo->desiredYaw = NPCInfo->lockedDesiredYaw = AngleNormalize360( angles.yaw );

	if ( NPC_UpdateAngles( qtrue, qtrue ) ) {//FIXME: if angles clamped, this may never work!
		//NPCInfo->shotTime = NPC->attackDebounceTime = 0;

		if ( shoot ) {//FIXME: needs to hold this down if using a weapon that requires it, like phaser...
			ucmd.buttons |= BUTTON_ATTACK;
		}

		//if ( !shoot || !(NPC->svFlags & SVF_LOCKEDENEMY) )
		if ( 1 ) {//If locked_enemy is on, dont complete until it is destroyed...
			trap->ICARUS_TaskIDComplete( (sharedEntity_t *)NPC, TID_BSTATE );
			goto finished;
		}
	}
#if 0
	else if ( shoot && (NPC->svFlags & SVF_LOCKEDENEMY) )
		float	dist = VectorLength( &dir );
		float	yawMiss, yawMissAllow = NPC->enemy->r.maxs.x;
		float	pitchMiss, pitchMissAllow = (NPC->enemy->r.maxs.z - NPC->enemy->r.mins.z) / 2;

		if ( yawMissAllow < 8.0f )
			yawMissAllow = 8.0f;

		if ( pitchMissAllow < 8.0f )
			pitchMissAllow = 8.0f;

		yawMiss = tanf( DEG2RAD( AngleDelta( NPC->client->ps.viewangles.yaw, NPCInfo->desiredYaw ) ) ) * dist;
		pitchMiss = tanf( DEG2RAD( AngleDelta( NPC->client->ps.viewangles.pitch, NPCInfo->desiredPitch ) ) ) * dist;

		if ( yawMissAllow >= yawMiss && pitchMissAllow > pitchMiss )
			ucmd.buttons |= BUTTON_ATTACK;
	}
#endif

	return;

finished:
	NPCInfo->desiredYaw = client->ps.viewangles.yaw;
	NPCInfo->desiredPitch = client->ps.viewangles.pitch;

	NPCInfo->aimTime = 0;//ok to turn normally now
}

/*
void NPC_BSMove(void)
Move in a direction, face another
*/
void NPC_BSMove( void ) {
	gentity_t	*goal = NULL;

	NPC_CheckEnemy( qtrue, qfalse, qtrue );
	if ( NPC->enemy ) {
		NPC_CheckCanAttack( 1.0f, qfalse );
	}
	else {
		NPC_UpdateAngles( qtrue, qtrue );
	}

	goal = UpdateGoal();
	if ( goal ) {
		//		NPCInfo->moveToGoalMod = 1.0f;

		NPC_SlideMoveToGoal();
	}
}

/*
void NPC_BSShoot(void)
Move in a direction, face another
*/

void NPC_BSShoot( void ) {
	//	NPC_BSMove();

	enemyVisibility = VIS_SHOOT;

	if ( client->ps.weaponstate != WEAPON_READY && client->ps.weaponstate != WEAPON_FIRING ) {
		client->ps.weaponstate = WEAPON_READY;
	}

	WeaponThink( qtrue );
}

/*
void NPC_BSPatrol( void )

Same as idle, but you look for enemies every "vigilance"
using your angles, HFOV, VFOV and visrange, and listen for sounds within earshot...
*/
void NPC_BSPatrol( void ) {
	//int	alertEventNum;

	if ( level.time > NPCInfo->enemyCheckDebounceTime ) {
		NPCInfo->enemyCheckDebounceTime = level.time + (NPCInfo->stats.vigilance * 1000);
		NPC_CheckEnemy( qtrue, qfalse, qtrue );
		if ( NPC->enemy ) {//FIXME: do anger script
			NPCInfo->behaviorState = BS_HUNT_AND_KILL;
			//NPC_AngerSound();
			return;
		}
	}

	//FIXME: Implement generic sound alerts
	/*
	alertEventNum = NPC_CheckAlertEvents( qtrue, qtrue );
	if( alertEventNum != -1 )
	{//If we heard something, see if we should check it out
	if ( NPC_CheckInvestigate( alertEventNum ) )
	{
	return;
	}
	}
	*/

	NPCInfo->investigateSoundDebounceTime = 0;
	//FIXME if there is no nav data, we need to do something else
	// if we're stuck, try to move around it
	if ( UpdateGoal() ) {
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );

	ucmd.buttons |= BUTTON_WALKING;
}

/*
void NPC_BSDefault(void)
uses various scriptflags to determine how an npc should behave
*/
extern void NPC_CheckGetNewWeapon( void );
extern void NPC_BSST_Attack( void );

void NPC_BSDefault( void ) {
	//	vector3		enemyDir;
	//	float		enemyDist;
	//	float		shootDist;
	//	qboolean	enemyFOV = qfalse;
	//	qboolean	enemyShotFOV = qfalse;
	//	qboolean	enemyPVS = qfalse;
	//	vector3		enemyHead;
	//	vector3		muzzle;
	//	qboolean	enemyLOS = qfalse;
	//	qboolean	enemyCS = qfalse;
	qboolean	move = qtrue;
	//	qboolean	shoot = qfalse;


	if ( NPCInfo->scriptFlags & SCF_FIRE_WEAPON ) {
		WeaponThink( qtrue );
	}

	if ( NPCInfo->scriptFlags & SCF_FORCED_MARCH ) {//being forced to walk
		if ( NPC->client->ps.torsoAnim != TORSO_SURRENDER_START ) {
			NPC_SetAnim( NPC, SETANIM_TORSO, TORSO_SURRENDER_START, SETANIM_FLAG_HOLD );
		}
	}
	//look for a new enemy if don't have one and are allowed to look, validate current enemy if have one
	NPC_CheckEnemy( (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES), qfalse, qtrue );
	if ( !NPC->enemy ) {//still don't have an enemy
		if ( !(NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) ) {//check for alert events
			//FIXME: Check Alert events, see if we should investigate or just look at it
			int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qtrue, AEL_DISCOVERED );

			//There is an event to look at
			if ( alertEvent >= 0 && level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID ) {//heard/saw something
				if ( level.alertEvents[alertEvent].level >= AEL_DISCOVERED && (NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES) ) {//was a big event
					if ( level.alertEvents[alertEvent].owner && level.alertEvents[alertEvent].owner->client && level.alertEvents[alertEvent].owner->health >= 0 && level.alertEvents[alertEvent].owner->client->playerTeam == NPC->client->enemyTeam ) {//an enemy
						G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
					}
				}
				else {//FIXME: investigate lesser events
				}
			}
			//FIXME: also check our allies' condition?
		}
	}

	if ( NPC->enemy && !(NPCInfo->scriptFlags&SCF_FORCED_MARCH) ) {
		// just use the stormtrooper attack AI...
		NPC_CheckGetNewWeapon();
		if ( NPC->client->leader
			&& NPCInfo->goalEntity == NPC->client->leader
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPC, TID_MOVE_NAV ) ) {
			NPC_ClearGoal();
		}
		NPC_BSST_Attack();
		return;
	}

	if ( UpdateGoal() ) {//have a goal
		if ( !NPC->enemy
			&& NPC->client->leader
			&& NPCInfo->goalEntity == NPC->client->leader
			&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPC, TID_MOVE_NAV ) ) {
			NPC_BSFollowLeader();
		}
		else {
			//set angles
			if ( (NPCInfo->scriptFlags & SCF_FACE_MOVE_DIR) || NPCInfo->goalEntity != NPC->enemy ) {//face direction of movement, NOTE: default behavior when not chasing enemy
				NPCInfo->combatMove = qfalse;
			}
			else {//face goal.. FIXME: what if have a navgoal but want to face enemy while moving?  Will this do that?
				vector3	dir, angles;

				NPCInfo->combatMove = qfalse;

				VectorSubtract( &NPCInfo->goalEntity->r.currentOrigin, &NPC->r.currentOrigin, &dir );
				vectoangles( &dir, &angles );
				NPCInfo->desiredYaw = angles.yaw;
				if ( NPCInfo->goalEntity == NPC->enemy ) {
					NPCInfo->desiredPitch = angles.pitch;
				}
			}

			//set movement
			//override default walk/run behavior
			//NOTE: redundant, done in NPC_ApplyScriptFlags
			if ( NPCInfo->scriptFlags & SCF_RUNNING ) {
				ucmd.buttons &= ~BUTTON_WALKING;
			}
			else if ( NPCInfo->scriptFlags & SCF_WALKING ) {
				ucmd.buttons |= BUTTON_WALKING;
			}
			else if ( NPCInfo->goalEntity == NPC->enemy ) {
				ucmd.buttons &= ~BUTTON_WALKING;
			}
			else {
				ucmd.buttons |= BUTTON_WALKING;
			}

			if ( NPCInfo->scriptFlags & SCF_FORCED_MARCH ) {//being forced to walk
				//if ( g_crosshairEntNum != NPC->s.number )
				if ( !NPC_SomeoneLookingAtMe( NPC ) ) {//don't walk if player isn't aiming at me
					move = qfalse;
				}
			}

			if ( move ) {
				//move toward goal
				NPC_MoveToGoal( qtrue );
			}
		}
	}
	else if ( !NPC->enemy && NPC->client->leader ) {
		NPC_BSFollowLeader();
	}

	//update angles
	NPC_UpdateAngles( qtrue, qtrue );
}
