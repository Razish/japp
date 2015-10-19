#include "b_local.h"
#include "g_nav.h"

void Interrogator_Idle( void );
void DeathFX( gentity_t *ent );

enum {
	LSTATE_BLADESTOP = 0,
	LSTATE_BLADEUP,
	LSTATE_BLADEDOWN,
};

void NPC_Interrogator_Precache( void ) {
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	G_SoundIndex( "sound/chars/mark1/misc/anger.wav" );
	G_SoundIndex( "sound/chars/probe/misc/talk" );
	G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject" );
	G_SoundIndex( "sound/chars/interrogator/misc/int_droid_explo" );
	G_EffectIndex( "explosions/droidexplosion1" );
}

void Interrogator_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc ) {
	self->client->ps.velocity.z = -100;
	/*
	self->locationDamage[HL_NONE] += damage;
	if (self->locationDamage[HL_NONE] > 40)
	{
	DeathFX(self);
	self->client->ps.eFlags |= EF_NODRAW;
	self->contents = CONTENTS_CORPSE;
	}
	else
	*/
	{
		self->client->ps.eFlags2 &= ~EF2_FLYING;//moveType = MT_WALK;
		self->client->ps.velocity.x = Q_irand( -10, -20 );
		self->client->ps.velocity.y = Q_irand( -10, -20 );
		self->client->ps.velocity.z = -100;
	}
	//self->takedamage = qfalse;
	//self->client->ps.eFlags |= EF_NODRAW;
	//self->contents = 0;
	return;
}


void Interrogator_PartsMove( void ) {
	// Syringe
	if ( TIMER_Done( NPC, "syringeDelay" ) ) {
		NPC->pos1.y = AngleNormalize360( NPC->pos1.y );

		if ( (NPC->pos1.y < 60) || (NPC->pos1.y > 300) ) {
			NPC->pos1.y += Q_irand( -20, 20 );	// Pitch
		}
		else if ( NPC->pos1.y > 180 ) {
			NPC->pos1.y = Q_irand( 300, 360 );	// Pitch
		}
		else {
			NPC->pos1.y = Q_irand( 0, 60 );	// Pitch
		}

		//	gi.G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone1, NPC->pos1, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );
		NPC_SetBoneAngles( NPC, "left_arm", &NPC->pos1 );

		TIMER_Set( NPC, "syringeDelay", Q_irand( 100, 1000 ) );
	}

	// Scalpel
	if ( TIMER_Done( NPC, "scalpelDelay" ) ) {
		// Change pitch
		if ( NPCInfo->localState == LSTATE_BLADEDOWN )	// Blade is moving down
		{
			NPC->pos2.x -= 30;
			if ( NPC->pos2.x < 180 ) {
				NPC->pos2.x = 180;
				NPCInfo->localState = LSTATE_BLADEUP;	// Make it move up
			}
		}
		else											// Blade is coming back up
		{
			NPC->pos2.x += 30;
			if ( NPC->pos2.x >= 360 ) {
				NPC->pos2.x = 360;
				NPCInfo->localState = LSTATE_BLADEDOWN;	// Make it move down
				TIMER_Set( NPC, "scalpelDelay", Q_irand( 100, 1000 ) );
			}
		}

		NPC->pos2.x = AngleNormalize360( NPC->pos2.x );
		//	gi.G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone2, NPC->pos2, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

		NPC_SetBoneAngles( NPC, "right_arm", &NPC->pos2 );
	}

	// Claw
	NPC->pos3.y += Q_irand( 10, 30 );
	NPC->pos3.y = AngleNormalize360( NPC->pos3.y );
	//gi.G2API_SetBoneAnglesIndex( &NPC->ghoul2[NPC->playerModel], NPC->genericBone3, NPC->pos3, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL );

	NPC_SetBoneAngles( NPC, "claw", &NPC->pos3 );

}

#define VELOCITY_DECAY	0.85f
#define HUNTER_UPWARD_PUSH	2

void Interrogator_MaintainHeight( void ) {
	float	dif;
	//	vector3	endPos;
	//	trace_t	trace;

	NPC->s.loopSound = G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_lp" );
	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at about enemy eye level
	if ( NPC->enemy ) {
		// Find the height difference
		dif = (NPC->enemy->r.currentOrigin.z + NPC->enemy->r.maxs.z) - NPC->r.currentOrigin.z;

		// cap to prevent dramatic height shifts
		if ( fabsf( dif ) > 2 ) {
			if ( fabsf( dif ) > 16 ) {
				dif = (dif < 0 ? -16 : 16);
			}

			NPC->client->ps.velocity.z = (NPC->client->ps.velocity.z + dif) / 2;
		}
	}
	else {
		gentity_t *goal = NULL;

		if ( NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCInfo->goalEntity;
		}
		else {
			goal = NPCInfo->lastGoalEntity;
		}
		if ( goal ) {
			dif = goal->r.currentOrigin.z - NPC->r.currentOrigin.z;

			if ( fabsf( dif ) > 24 ) {
				ucmd.upmove = (ucmd.upmove < 0 ? -4 : 4);
			}
			else {
				if ( (int)NPC->client->ps.velocity.z ) {
					NPC->client->ps.velocity.z *= VELOCITY_DECAY;

					if ( fabsf( NPC->client->ps.velocity.z ) < 2 ) {
						NPC->client->ps.velocity.z = 0;
					}
				}
			}
		}
		// Apply friction
		else if ( (int)NPC->client->ps.velocity.z ) {
			NPC->client->ps.velocity.z *= VELOCITY_DECAY;

			if ( fabsf( NPC->client->ps.velocity.z ) < 1 ) {
				NPC->client->ps.velocity.z = 0;
			}
		}
	}

	// Apply friction
	if ( (int)NPC->client->ps.velocity.x ) {
		NPC->client->ps.velocity.x *= VELOCITY_DECAY;

		if ( fabsf( NPC->client->ps.velocity.x ) < 1 ) {
			NPC->client->ps.velocity.x = 0;
		}
	}

	if ( (int)NPC->client->ps.velocity.y ) {
		NPC->client->ps.velocity.y *= VELOCITY_DECAY;

		if ( fabsf( NPC->client->ps.velocity.y ) < 1 ) {
			NPC->client->ps.velocity.y = 0;
		}
	}
}

#define HUNTER_STRAFE_VEL	32
#define HUNTER_STRAFE_DIS	200

void Interrogator_Strafe( void ) {
	int		dir;
	vector3	end, right;
	trace_t	tr;
	float	dif;

	AngleVectors( &NPC->client->renderInfo.eyeAngles, NULL, &right, NULL );

	// Pick a random strafe direction, then check to see if doing a strafe would be
	//	reasonable valid
	dir = (rand() & 1) ? -1 : 1;
	VectorMA( &NPC->r.currentOrigin, HUNTER_STRAFE_DIS * dir, &right, &end );

	trap->Trace( &tr, &NPC->r.currentOrigin, NULL, NULL, &end, NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

	// Close enough
	if ( tr.fraction > 0.9f ) {
		VectorMA( &NPC->client->ps.velocity, HUNTER_STRAFE_VEL * dir, &right, &NPC->client->ps.velocity );

		// Add a slight upward push
		if ( NPC->enemy ) {
			// Find the height difference
			dif = (NPC->enemy->r.currentOrigin.z + 32) - NPC->r.currentOrigin.z;

			// cap to prevent dramatic height shifts
			if ( fabsf( dif ) > 8 ) {
				dif = (dif < 0 ? -HUNTER_UPWARD_PUSH : HUNTER_UPWARD_PUSH);
			}

			NPC->client->ps.velocity.z += dif;

		}

		// Set the strafe start time
		//NPC->fx_time = level.time;
		NPCInfo->standTime = level.time + 3000 + random() * 500;
	}
}

#define HUNTER_FORWARD_BASE_SPEED	10
#define HUNTER_FORWARD_MULTIPLIER	2

void Interrogator_Hunt( qboolean visible, qboolean advance ) {
	float	distance, speed;
	vector3	forward;

	Interrogator_PartsMove();

	NPC_FaceEnemy( qfalse );

	//If we're not supposed to stand still, pursue the player
	if ( NPCInfo->standTime < level.time ) {
		// Only strafe when we can see the player
		if ( visible ) {
			Interrogator_Strafe();
			if ( NPCInfo->standTime > level.time ) {//successfully strafed
				return;
			}
		}
	}

	//If we don't want to advance, stop here
	if ( advance == qfalse )
		return;

	//Only try and navigate if the player is visible
	if ( visible == qfalse ) {
		// Move towards our goal
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = 12;

		//Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection( &forward, &distance ) == qfalse )
			return;
	}
	else {
		VectorSubtract( &NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &forward );
		VectorNormalize( &forward );
	}

	speed = HUNTER_FORWARD_BASE_SPEED + HUNTER_FORWARD_MULTIPLIER * g_spSkill.getInt();
	VectorMA( &NPC->client->ps.velocity, speed, &forward, &NPC->client->ps.velocity );
}

#define MIN_DISTANCE		64

void Interrogator_Melee( qboolean visible, qboolean advance ) {
	if ( TIMER_Done( NPC, "attackDelay" ) )	// Attack?
	{
		// Make sure that we are within the height range before we allow any damage to happen
		if ( NPC->r.currentOrigin.z >= NPC->enemy->r.currentOrigin.z + NPC->enemy->r.mins.z && NPC->r.currentOrigin.z + NPC->r.mins.z + 8 < NPC->enemy->r.currentOrigin.z + NPC->enemy->r.maxs.z ) {
			//gentity_t *tent;

			TIMER_Set( NPC, "attackDelay", Q_irand( 500, 3000 ) );
			G_Damage( NPC->enemy, NPC, NPC, 0, 0, 2, DAMAGE_NO_KNOCKBACK, MOD_MELEE );

			//	NPC->enemy->client->poisonDamage = 18;
			//	NPC->enemy->client->poisonTime = level.time + 1000;

			// Drug our enemy up and do the wonky vision thing
			//			tent = G_TempEntity( NPC->enemy->r.currentOrigin, EV_DRUGGED );
			//			tent->owner = NPC->enemy;

			//rwwFIXMEFIXME: poison damage

			G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/interrogator/misc/torture_droid_inject.mp3" ) );
		}
	}

	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES ) {
		Interrogator_Hunt( visible, advance );
	}
}

void Interrogator_Attack( void ) {
	float		distance;
	qboolean	visible;
	qboolean	advance;

	// Always keep a good height off the ground
	Interrogator_MaintainHeight();

	//randomly talk
	if ( TIMER_Done( NPC, "patrolNoise" ) ) {
		if ( TIMER_Done( NPC, "angerNoise" ) ) {
			G_SoundOnEnt( NPC, CHAN_AUTO, va( "sound/chars/probe/misc/talk.wav", Q_irand( 1, 3 ) ) );

			TIMER_Set( NPC, "patrolNoise", Q_irand( 4000, 10000 ) );
		}
	}

	// If we don't have an enemy, just idle
	if ( NPC_CheckEnemyExt( qfalse ) == qfalse ) {
		Interrogator_Idle();
		return;
	}

	// Rate our distance to the target, and our visibilty
	distance = (int)DistanceHorizontalSquared( &NPC->r.currentOrigin, &NPC->enemy->r.currentOrigin );
	visible = NPC_ClearLOS4( NPC->enemy );
	advance = (qboolean)(distance > MIN_DISTANCE*MIN_DISTANCE);

	if ( !visible ) {
		advance = qtrue;
	}
	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES ) {
		Interrogator_Hunt( visible, advance );
	}

	NPC_FaceEnemy( qtrue );

	if ( !advance ) {
		Interrogator_Melee( visible, advance );
	}
}

void Interrogator_Idle( void ) {
	if ( NPC_CheckPlayerTeamStealth() ) {
		G_SoundOnEnt( NPC, CHAN_AUTO, "sound/chars/mark1/misc/anger.wav" );
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}

	Interrogator_MaintainHeight();

	NPC_BSIdle();
}

void NPC_BSInterrogator_Default( void ) {
	//NPC->e_DieFunc = dieF_Interrogator_die;

	if ( NPC->enemy ) {
		Interrogator_Attack();
	}
	else {
		Interrogator_Idle();
	}

}
