// this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when following another player or playing back a demo,
//	it will be checked when the snapshot transitions like all the other entities

#include "cg_local.h"
#include "cg_media.h"

// If the ammo has gone low enough to generate the warning, play a sound
void CG_CheckAmmo( void ) {
	int i, total, previous, weapons;

	// see about how many seconds of ammo we have remaining
	weapons = cg.snap->ps.stats[STAT_WEAPONS];
	total = 0;
	for ( i = WP_BRYAR_PISTOL; i < WP_NUM_WEAPONS; i++ ) {
		if ( !(weapons & (1 << i)) )
			continue;
		switch ( i ) {
		case WP_BRYAR_PISTOL:
		case WP_CONCUSSION:
		case WP_BRYAR_OLD:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
		case WP_ROCKET_LAUNCHER:
		case WP_THERMAL:
		case WP_TRIP_MINE:
		case WP_DET_PACK:
		case WP_EMPLACED_GUN:
			total += cg.snap->ps.ammo[weaponData[i].ammoIndex] * 1000;
			break;
		default:
			total += cg.snap->ps.ammo[weaponData[i].ammoIndex] * 200;
			break;
		}
		if ( total >= 5000 ) {
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	previous = cg.lowAmmoWarning;

	if ( total == 0 )
		cg.lowAmmoWarning = 2;
	else
		cg.lowAmmoWarning = 1;

	if ( cg.snap->ps.weapon == WP_SABER )
		cg.lowAmmoWarning = 0;

	// play a sound on transitions
	if ( cg.lowAmmoWarning != previous )
		trap->S_StartLocalSound( media.sounds.weapons.noAmmo, CHAN_LOCAL_SOUND );
}

void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float left, front, up, kick, scale, dist, yaw, pitch;
	int health;
	vector3 dir, angles;
	refdef_t *refdef = CG_GetRefdef();

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 )
		scale = 1;
	else
		scale = 40.0f / health;

	kick = damage * scale;

	if ( kick < 5 )
		kick = 5;
	if ( kick > 10 )
		kick = 10;

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		cg.damageX = 0;
		cg.damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	}
	else {
		// positional
		pitch = pitchByte / 255.0f * 360;
		yaw = yawByte / 255.0f * 360;

		angles.pitch = pitch;
		angles.yaw = yaw;
		angles.roll = 0;

		AngleVectors( &angles, &dir, NULL, NULL );
		VectorSubtract( &vec3_origin, &dir, &dir );

		front = DotProduct( &dir, &refdef->viewaxis[0] );
		left = DotProduct( &dir, &refdef->viewaxis[1] );
		up = DotProduct( &dir, &refdef->viewaxis[2] );

		dir.x = front;
		dir.y = left;
		dir.z = 0;
		dist = VectorLength( &dir );
		if ( dist < 0.1f )
			dist = 0.1f;

		cg.v_dmg_roll = kick * left;
		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1f )
			front = 0.1f;
		cg.damageX = -left / front;
		cg.damageY = up / dist;
	}

	// clamp the position
	if ( cg.damageX > 1.0f )
		cg.damageX = 1.0f;
	if ( cg.damageX < -1.0f )
		cg.damageX = -1.0f;

	if ( cg.damageY > 1.0f )
		cg.damageY = 1.0f;
	if ( cg.damageY < -1.0f )
		cg.damageY = -1.0f;

	// don't let the screen flashes vary as much
	if ( kick > 10 )
		kick = 10;
	cg.damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	cg.damageTime = cg.snap->serverTime;
}

// A respawn happened this snapshot
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// display weapons available
	cg.weaponSelectTime = cg.time;

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
}

extern const char *eventnames[];
void CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops ) {
	int i, event;
	centity_t *cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ps->clientNum];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, &cent->lerpOrigin );
	}

	cent = &cg_entities[ps->clientNum];
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			// or the server told us to play another event instead of a predicted event we already issued
			// or something the server told us changed our prediction causing a different event
			|| (i > ops->eventSequence - MAX_PS_EVENTS && ps->events[i & (MAX_PS_EVENTS - 1)] != ops->events[i & (MAX_PS_EVENTS - 1)]) ) {

			event = ps->events[i & (MAX_PS_EVENTS - 1)];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[i & (MAX_PS_EVENTS - 1)];
			//Raz: Swoop camera fix
			//	cent->playerState = ps;
			CG_EntityEvent( cent, &cent->lerpOrigin );

			cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

			cg.eventSequence++;
		}
	}
}

void CG_CheckChangedPredictableEvents( playerState_t *ps ) {
	int i, event;
	centity_t	*cent;

	cent = &cg_entities[ps->clientNum];
	for ( i = ps->eventSequence - MAX_PS_EVENTS; i < ps->eventSequence; i++ ) {
		if ( i >= cg.eventSequence )
			continue;
		// if this event is not further back in than the maximum predictable events we remember
		if ( i > cg.eventSequence - MAX_PREDICTED_EVENTS ) {
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[i & (MAX_PS_EVENTS - 1)] != cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] ) {

				event = ps->events[i & (MAX_PS_EVENTS - 1)];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[i & (MAX_PS_EVENTS - 1)];
				CG_EntityEvent( cent, &cent->lerpOrigin );

				cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

				if ( cg_showMiss.integer )
					trap->Print( "WARNING: changed predicted event\n" );
			}
		}
	}
}

static void pushReward( sfxHandle_t sfx, qhandle_t shader, int rewardCount ) {
	if ( cg.rewardStack < (MAX_REWARDSTACK - 1) ) {
		cg.rewardStack++;
		cg.rewardSound[cg.rewardStack] = sfx;
		cg.rewardShader[cg.rewardStack] = shader;
		cg.rewardCount[cg.rewardStack] = rewardCount;
	}
}

int cgAnnouncerTime = 0; //to prevent announce sounds from playing on top of each other

void CG_CheckLocalSounds( playerState_t *ps, playerState_t *ops ) {
	int highScore;

	// don't play the sounds if the player just changed teams
	if ( ps->persistant[PERS_TEAM] != ops->persistant[PERS_TEAM] )
		return;

	// hit changes
#if 0
	if ( ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS] ) {
		armor  = ps->persistant[PERS_ATTACKEE_ARMOR] & 0xff;
		health = ps->persistant[PERS_ATTACKEE_ARMOR] >> 8;

		if ( armor > health/2 )
			trap->S_StartLocalSound( media.sounds.shieldPierce, CHAN_LOCAL_SOUND );
		else
			trap->S_StartLocalSound( media.sounds.hit, CHAN_LOCAL_SOUND );

		//FIXME: Hit sounds?
		if ( armor > 50 )
			trap->S_StartLocalSound( media.sounds.hitHighArmor, CHAN_LOCAL_SOUND );
		else if ( armor || health > 100 )
			trap->S_StartLocalSound( media.sounds.hitLowArmor, CHAN_LOCAL_SOUND );
		else
			trap->S_StartLocalSound( media.sounds.hit, CHAN_LOCAL_SOUND );
	}
	else if ( ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS] )
		trap->S_StartLocalSound( media.sounds.hitTeam, CHAN_LOCAL_SOUND );
#endif

	// health changes of more than -3 should make pain sounds
	if ( cg_oldPainSounds.integer ) {
		if ( ps->stats[STAT_HEALTH] < (ops->stats[STAT_HEALTH] - 3) ) {
			if ( ps->stats[STAT_HEALTH] > 0 )
				CG_PainEvent( &cg_entities[cg.predictedPlayerState.clientNum], ps->stats[STAT_HEALTH] );
		}
	}

	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted || (cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION) )
		return;

	// reward sounds
	if ( ps->persistant[PERS_CAPTURES] != ops->persistant[PERS_CAPTURES] )
		pushReward( media.sounds.awards.capture, media.gfx.interface.medals.capture, ps->persistant[PERS_CAPTURES] );
	if ( ps->persistant[PERS_IMPRESSIVE_COUNT] != ops->persistant[PERS_IMPRESSIVE_COUNT] )
		pushReward( media.sounds.awards.impressive, media.gfx.interface.medals.impressive, ps->persistant[PERS_IMPRESSIVE_COUNT] );
	if ( ps->persistant[PERS_EXCELLENT_COUNT] != ops->persistant[PERS_EXCELLENT_COUNT] )
		pushReward( media.sounds.awards.excellent, media.gfx.interface.medals.excellent, ps->persistant[PERS_EXCELLENT_COUNT] );
	if ( ps->persistant[PERS_GAUNTLET_FRAG_COUNT] != ops->persistant[PERS_GAUNTLET_FRAG_COUNT] )
		pushReward( media.sounds.awards.humiliation, media.gfx.interface.medals.gauntlet, ps->persistant[PERS_GAUNTLET_FRAG_COUNT] );
	if ( ps->persistant[PERS_DEFEND_COUNT] != ops->persistant[PERS_DEFEND_COUNT] )
		pushReward( media.sounds.awards.defense, media.gfx.interface.medals.defend, ps->persistant[PERS_DEFEND_COUNT] );
	if ( ps->persistant[PERS_ASSIST_COUNT] != ops->persistant[PERS_ASSIST_COUNT] )
		pushReward( media.sounds.awards.assist, media.gfx.interface.medals.assist, ps->persistant[PERS_ASSIST_COUNT] );
	// if any of the player event bits changed
	if ( ps->persistant[PERS_PLAYEREVENTS] != ops->persistant[PERS_PLAYEREVENTS] ) {
		if ( (ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) != (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_DENIEDREWARD) )
			trap->S_StartLocalSound( media.sounds.awards.denied, CHAN_ANNOUNCER );

		else if ( (ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) != (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_GAUNTLETREWARD) )
			trap->S_StartLocalSound( media.sounds.awards.humiliation, CHAN_ANNOUNCER );

		else if ( (ps->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT) != (ops->persistant[PERS_PLAYEREVENTS] & PLAYEREVENT_HOLYSHIT) )
			trap->S_StartLocalSound( media.sounds.awards.holyShit, CHAN_ANNOUNCER );
	}

	// timelimit warnings
	if ( cgs.timelimit > 0 && cgAnnouncerTime < cg.time ) {
		int msec = cg.time - cgs.levelStartTime;
		if ( !(cg.timelimitWarnings & 4) && msec >( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 1 | 2 | 4;
			//RAZFIXME: Add sudden death sound
			//	trap->S_StartLocalSound( media.sounds.suddenDeath, CHAN_ANNOUNCER );
		}
		else if ( !(cg.timelimitWarnings & 2) && msec > (cgs.timelimit - 1) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1 | 2;
			trap->S_StartLocalSound( media.sounds.warning.oneMinute, CHAN_ANNOUNCER );
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.timelimit > 5 && !(cg.timelimitWarnings & 1) && msec > (cgs.timelimit - 5) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			trap->S_StartLocalSound( media.sounds.warning.fiveMinute, CHAN_ANNOUNCER );
			cgAnnouncerTime = cg.time + 3000;
		}
	}

	// fraglimit warnings
	if ( cgs.fraglimit > 0 && cgs.gametype < GT_CTF && cgs.gametype != GT_DUEL && cgs.gametype != GT_POWERDUEL && cgs.gametype != GT_SIEGE && cgAnnouncerTime < cg.time ) {
		highScore = cgs.scores1;
		if ( cgs.gametype == GT_TEAM && cgs.scores2 > highScore )
			highScore = cgs.scores2;

		if ( !(cg.fraglimitWarnings & 4) && highScore == (cgs.fraglimit - 1) ) {
			cg.fraglimitWarnings |= 1 | 2 | 4;
			CG_AddBufferedSound( media.sounds.warning.oneFrag );
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.fraglimit > 2 && !(cg.fraglimitWarnings & 2) && highScore == (cgs.fraglimit - 2) ) {
			cg.fraglimitWarnings |= 1 | 2;
			CG_AddBufferedSound( media.sounds.warning.twoFrag );
			cgAnnouncerTime = cg.time + 3000;
		}
		else if ( cgs.fraglimit > 3 && !(cg.fraglimitWarnings & 1) && highScore == (cgs.fraglimit - 3) ) {
			cg.fraglimitWarnings |= 1;
			CG_AddBufferedSound( media.sounds.warning.threeFrag );
			cgAnnouncerTime = cg.time + 3000;
		}
	}
}

void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops ) {
	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount )
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );

	// respawning
	if ( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT] )
		CG_Respawn();

	if ( cg.mapRestart ) {
		CG_Respawn();
		cg.mapRestart = qfalse;
	}

	if ( cg.snap->ps.pm_type != PM_INTERMISSION && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR )
		CG_CheckLocalSounds( ps, ops );

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents( ps, ops );

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight ) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		if ( cg_instantDuck.integer )
			cg.duckTime = cg.time - DUCK_TIME;
		else
			cg.duckTime = cg.time;
	}
}
