//NT - unlagged - new time-shift client functions

#include "g_local.h"
#include "g_unlagged.h"

/*
============
G_ResetTrail

Clear out the given client's origin trails (should be called from ClientBegin and when
the teleport bit is toggled)
============
*/
void G_ResetTrail( gentity_t *ent ) {
	int		i, time;

	// fill up the origin trails with data (assume the current position
	// for the last 1/2 second or so)
	ent->client->trailHead = NUM_CLIENT_TRAILS - 1;
	for ( i = ent->client->trailHead, time = level.time; i >= 0; i--, time -= 50 ) {
		VectorCopy( &ent->r.mins, &ent->client->trail[i].mins );
		VectorCopy( &ent->r.maxs, &ent->client->trail[i].maxs );
		VectorCopy( &ent->r.currentOrigin, &ent->client->trail[i].currentOrigin );
		VectorCopy( &ent->r.currentAngles, &ent->client->trail[i].currentAngles );
		ent->client->trail[i].leveltime = time;
		ent->client->trail[i].time = time;
	}
}


/*
============
G_StoreTrail

Keep track of where the client's been (usually called every ClientThink)
============
*/
void G_StoreTrail( gentity_t *ent ) {
	int		head, newtime;

	head = ent->client->trailHead;

	// if we're on a new frame
	if ( ent->client->trail[head].leveltime < level.time ) {
		// snap the last head up to the end of frame time
		ent->client->trail[head].time = level.previousTime;

		// increment the head
		ent->client->trailHead++;
		if ( ent->client->trailHead >= NUM_CLIENT_TRAILS ) {
			ent->client->trailHead = 0;
		}
		head = ent->client->trailHead;
	}

	if ( ent->r.svFlags & SVF_BOT ) {
		// bots move only once per frame
		newtime = level.time;
	}
	else {
		// calculate the actual server time
		// (we set level.frameStartTime every G_RunFrame)
		newtime = level.previousTime + trap->Milliseconds() - level.frameStartTime;
		if ( newtime > level.time ) {
			newtime = level.time;
		}
		else if ( newtime <= level.previousTime ) {
			newtime = level.previousTime + 1;
		}
	}

	// store all the collision-detection info and the time
	VectorCopy( &ent->r.mins, &ent->client->trail[head].mins );
	VectorCopy( &ent->r.maxs, &ent->client->trail[head].maxs );
	VectorCopy( &ent->r.currentOrigin, &ent->client->trail[head].currentOrigin );
	VectorCopy( &ent->r.currentAngles, &ent->client->trail[head].currentAngles );
	ent->client->trail[head].leveltime = level.time;
	ent->client->trail[head].time = newtime;

	// FOR TESTING ONLY
	//	Com_Printf("level.previousTime: %d, level.time: %d, newtime: %d\n", level.previousTime, level.time, newtime);
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp( float frac, vector3 *start, vector3 *end, vector3 *result ) {
	float	comp = 1.0f - frac;

	result->x = frac * start->x + comp * end->x;
	result->y = frac * start->y + comp * end->y;
	result->z = frac * start->z + comp * end->z;
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified "time"
=================
*/
void G_TimeShiftClient( gentity_t *ent, int time ) {
	int		j, k;

	if ( time > level.time ) {
		time = level.time;
	}

	// find two entries in the origin trail whose times sandwich "time"
	// assumes no two adjacent trail records have the same timestamp
	j = k = ent->client->trailHead;
	do {
		if ( ent->client->trail[j].time <= time )
			break;

		k = j;
		j--;
		if ( j < 0 ) {
			j = NUM_CLIENT_TRAILS - 1;
		}
	} while ( j != ent->client->trailHead );

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if ( j != k ) {
		// make sure it doesn't get re-saved
		if ( ent->client->saved.leveltime != level.time ) {
			// save the current origin and bounding box
			VectorCopy( &ent->r.mins, &ent->client->saved.mins );
			VectorCopy( &ent->r.maxs, &ent->client->saved.maxs );
			VectorCopy( &ent->r.currentOrigin, &ent->client->saved.currentOrigin );
			VectorCopy( &ent->r.currentAngles, &ent->client->saved.currentAngles );
			ent->client->saved.leveltime = level.time;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->trailHead ) {
			float	frac = (float)(ent->client->trail[k].time - time) / (float)(ent->client->trail[k].time - ent->client->trail[j].time);

			// FOR TESTING ONLY
			//			Com_Printf( "level time: %d, fire time: %d, j time: %d, k time: %d\n", level.time, time, ent->client->trail[j].time, ent->client->trail[k].time );

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac, &ent->client->trail[k].currentOrigin, &ent->client->trail[j].currentOrigin, &ent->r.currentOrigin );
			ent->r.currentAngles.yaw = LerpAngle( ent->client->trail[k].currentAngles.yaw, ent->r.currentAngles.yaw, frac );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac, &ent->client->trail[k].mins, &ent->client->trail[j].mins, &ent->r.mins );
			TimeShiftLerp( frac, &ent->client->trail[k].maxs, &ent->client->trail[j].maxs, &ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
		else {
			// we wrapped, so grab the earliest
			VectorCopy( &ent->client->trail[k].currentAngles, &ent->r.currentAngles );
			VectorCopy( &ent->client->trail[k].currentOrigin, &ent->r.currentOrigin );
			VectorCopy( &ent->client->trail[k].mins, &ent->r.mins );
			VectorCopy( &ent->client->trail[k].maxs, &ent->r.maxs );

			// this will recalculate absmin and absmax
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
	}
}

static qboolean shifted = qfalse;
/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, gentity_t *skip ) {
	int i = 0;
	gentity_t *ent = NULL;

	if ( shifted ) {
		trap->Print( "WARNING: Tried to shift all clients when they were already shifted!\n" );
		return;
	}

	if ( time > level.time )
		time = level.time;

	for ( i = 0, ent = g_entities; i < MAX_CLIENTS; i++, ent++ ) {
		if ( ent->client &&
			ent->inuse &&
			ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->tempSpectate < level.time &&
			ent != skip ) {
			G_TimeShiftClient( ent, time );
		}
	}

	shifted = qtrue;
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( gentity_t *ent ) {
	// if it was saved
	if ( ent->client->saved.leveltime == level.time ) {
		// move it back
		VectorCopy( &ent->client->saved.mins, &ent->r.mins );
		VectorCopy( &ent->client->saved.maxs, &ent->r.maxs );
		VectorCopy( &ent->client->saved.currentOrigin, &ent->r.currentOrigin );
		VectorCopy( &ent->client->saved.currentAngles, &ent->r.currentAngles );
		ent->client->saved.leveltime = 0;

		// this will recalculate absmin and absmax
		trap->LinkEntity( (sharedEntity_t *)ent );
	}
}

/*
=======================
G_UnTimeShiftAllClients

Move ALL the clients back to where they were before the time shift,
except for "skip"
=======================
*/
void G_UnTimeShiftAllClients( gentity_t *skip ) {
	int i = 0;
	gentity_t *ent = NULL;

	if ( !shifted ) {
		trap->Print( "WARNING: Tried to unshift all clients when they weren't shifted!\n" );
		return;
	}

	for ( i = 0, ent = g_entities; i < MAX_CLIENTS; i++, ent++ ) {
		if ( ent->client &&
			ent->inuse &&
			ent->client->sess.sessionTeam != TEAM_SPECTATOR &&
			ent->client->tempSpectate < level.time &&
			ent != skip ) {
			G_UnTimeShiftClient( ent );
		}
	}

	shifted = qfalse;
}
