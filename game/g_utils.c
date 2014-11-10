// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_utils.c -- misc utility functions for game module

#include "g_local.h"
#include "bg_saga.h"
#include "qcommon/q_shared.h"

typedef struct shaderRemap_s {
	char oldShader[MAX_QPATH], newShader[MAX_QPATH];
	float timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS (128)

static int remapCount = 0;
static shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

static void AddRemap( const char *oldShader, const char *newShader, float timeOffset ) {
	int i;

	for ( i = 0; i < remapCount; i++ ) {
		if ( !Q_stricmp( oldShader, remappedShaders[i].oldShader ) ) {
			// found it, just update this one
			Q_strncpyz( remappedShaders[i].newShader, newShader, sizeof(remappedShaders[i].newShader) );
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	}
	if ( remapCount < MAX_SHADER_REMAPS ) {
		Q_strncpyz( remappedShaders[remapCount].newShader, newShader, sizeof(remappedShaders[remapCount].newShader) );
		Q_strncpyz( remappedShaders[remapCount].oldShader, oldShader, sizeof(remappedShaders[remapCount].oldShader) );
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
}

static const char *BuildShaderStateConfig( void ) {
	static char buf[MAX_STRING_CHARS * 4];
	char out[(MAX_QPATH * 2) + 5];
	int i;

	memset( buf, 0, MAX_STRING_CHARS );
	for ( i = 0; i < remapCount; i++ ) {
		Com_sprintf( out, sizeof(out), "%s=%s:%5.2f@", remappedShaders[i].oldShader, remappedShaders[i].newShader, remappedShaders[i].timeOffset );
		Q_strcat( buf, sizeof(buf), out );
	}

	return buf;
}

static int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create ) {
	int i;
	char s[MAX_STRING_CHARS];

	if ( !name || !name[0] )
		return 0;

	for ( i = 1; i < max; i++ ) {
		trap->GetConfigstring( start + i, s, sizeof(s) );
		if ( !s[0] )
			break;

		if ( !strcmp( s, name ) )
			return i;
	}

	if ( !create )
		return 0;

	if ( i == max )
		trap->Error( ERR_DROP, "G_FindConfigstringIndex: overflow" );

	trap->SetConfigstring( start + i, name );

	return i;
}

int G_BoneIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_G2BONES, MAX_G2BONES, qtrue );
}

int G_ModelIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_MODELS, MAX_MODELS, qtrue );
}

int G_IconIndex( const char *name ) {
	assert( name && name[0] );
	return G_FindConfigstringIndex( name, CS_ICONS, MAX_ICONS, qtrue );
}

int G_SoundIndex( const char *name ) {
	assert( name && name[0] );
	return G_FindConfigstringIndex( name, CS_SOUNDS, MAX_SOUNDS, qtrue );
}

int G_SoundSetIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_AMBIENT_SET, MAX_AMBIENT_SETS, qtrue );
}

int G_EffectIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_EFFECTS, MAX_FX, qtrue );
}

int G_BSPIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_BSP_MODELS, MAX_SUB_BSP, qtrue );
}

void G_TeamCommand( team_t team, const char *cmd ) {
	int i = 0;
	gclient_t *cl = NULL;

	for ( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ ) {
		if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam == team )
			trap->SendServerCommand( i, cmd );
	}
}

// Searches all active entities for the next one that holds the matching string at fieldofs (use the FOFS() macro) in
//	the structure.
// Searches beginning at the entity after from, or the beginning if NULL
// NULL will be returned if the end of the list is reached.
gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match ) {
	char *s;

	if ( !from )
		from = g_entities;
	else
		from++;

	for ( ; from < &g_entities[level.num_entities]; from++ ) {
		if ( !from->inuse )
			continue;
		s = *(char **)((byte *)from + fieldofs);
		if ( !s )
			continue;
		if ( !Q_stricmp( s, match ) )
			return from;
	}

	return NULL;
}

// given an origin and a radius, return all entities that are in use that are within the list
// returns how many entities were found
int G_RadiusList( vector3 *origin, float radius, gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES] ) {
	gentity_t *ent;
	int entityList[MAX_GENTITIES], numListedEntities;
	vector3 mins, maxs, v;
	int i, e, count = 0;

	if ( radius < 1 )
		radius = 1;

	for ( i = 0; i < 3; i++ ) {
		mins.data[i] = origin->data[i] - radius;
		maxs.data[i] = origin->data[i] + radius;
	}

	numListedEntities = trap->EntitiesInBox( &mins, &maxs, entityList, MAX_GENTITIES );

	for ( e = 0; e < numListedEntities; e++ ) {
		ent = &g_entities[entityList[e]];

		if ( (ent == ignore) || !ent->inuse || ent->takedamage != takeDamage )
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ ) {
			if ( origin->data[i] < ent->r.absmin.data[i] )	v.data[i] = ent->r.absmin.data[i] - origin->data[i];
			else if ( origin->data[i] > ent->r.absmax.data[i] )	v.data[i] = origin->data[i] - ent->r.absmax.data[i];
			else												v.data[i] = 0;
		}

		if ( VectorLength( &v ) >= radius )
			continue;

		// ok, we are within the radius, add us to the incoming list
		ent_list[count++] = ent;
	}

	return count;
}

void G_Throw( gentity_t *targ, vector3 *newDir, float push ) {
	vector3 kvel;
	float mass = (targ->physicsBounce > 0) ? targ->physicsBounce : 200;

	if ( g_gravity.value > 0 ) {
		VectorScale( newDir, g_knockback.value * push / mass * 0.8f, &kvel );
		kvel.z = newDir->z * g_knockback.value * push / mass * 1.5f;
	}
	else
		VectorScale( newDir, g_knockback.value * (float)push / mass, &kvel );

	if ( targ->client )
		VectorAdd( &targ->client->ps.velocity, &kvel, &targ->client->ps.velocity );
	else if ( targ->s.pos.trType != TR_STATIONARY && targ->s.pos.trType != TR_LINEAR_STOP && targ->s.pos.trType != TR_NONLINEAR_STOP ) {
		VectorAdd( &targ->s.pos.trDelta, &kvel, &targ->s.pos.trDelta );
		VectorCopy( &targ->r.currentOrigin, &targ->s.pos.trBase );
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel out the movement immediately
	if ( targ->client && !targ->client->ps.pm_time ) {
		targ->client->ps.pm_time = Q_clampi( 50, push * 2, 200 );
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

// allocate a veh object
#define MAX_VEHICLES_AT_A_TIME (512) // (128)
static Vehicle_t g_vehiclePool[MAX_VEHICLES_AT_A_TIME];
static qboolean g_vehiclePoolOccupied[MAX_VEHICLES_AT_A_TIME];
static qboolean g_vehiclePoolInit = qfalse;
void G_AllocateVehicleObject( Vehicle_t **pVeh ) {
	int i = 0;

	if ( !g_vehiclePoolInit ) {
		g_vehiclePoolInit = qtrue;
		memset( g_vehiclePoolOccupied, 0, sizeof(g_vehiclePoolOccupied) );
	}

	for ( i = 0; i < MAX_VEHICLES_AT_A_TIME; i++ ) {
		// iterate through and try to find a free one
		if ( !g_vehiclePoolOccupied[i] ) {
			g_vehiclePoolOccupied[i] = qtrue;
			memset( &g_vehiclePool[i], 0, sizeof(Vehicle_t) );
			*pVeh = &g_vehiclePool[i];
			return;
		}
	}

	Com_Error( ERR_DROP, "Ran out of vehicle pool slots" );
}

// free the pointer, sort of a lame method
void G_FreeVehicleObject( Vehicle_t *pVeh ) {
	int i = 0;
	for ( i = 0; i < MAX_VEHICLES_AT_A_TIME; i++ ) {
		if ( g_vehiclePoolOccupied[i] && &g_vehiclePool[i] == pVeh ) {
			// guess this is it
			g_vehiclePoolOccupied[i] = qfalse;
			break;
		}
	}
}

static gclient_t *gClPtrs[MAX_GENTITIES];

void G_CreateFakeClient( int entNum, gclient_t **cl ) {
	//	trap->TrueMalloc( (void **)cl, sizeof( gclient_t ) );
	if ( !gClPtrs[entNum] )
		gClPtrs[entNum] = (gclient_t *)BG_Alloc( sizeof(gclient_t) );

	*cl = gClPtrs[entNum];
}

// Finally reworked PM_SetAnim to allow non-pmove calls, so we take our local anim index into account and make the call -rww
void BG_SetAnim( playerState_t *ps, animation_t *animations, int setAnimParts, int anim, uint32_t setAnimFlags, int blendTime );
void G_SetAnim( gentity_t *ent, usercmd_t *ucmd, int setAnimParts, int anim, uint32_t setAnimFlags, int blendTime ) {
	assert( ent->client );
	BG_SetAnim( &ent->client->ps, bgAllAnims[ent->localAnimIndex].anims, setAnimParts, anim, setAnimFlags, blendTime );
}

// Selects a random entity from among the targets
#define MAXCHOICES (32)
gentity_t *G_PickTarget( char *targetname ) {
	gentity_t *ent = NULL, *choice[MAXCHOICES];
	int choices = 0;

	if ( !targetname ) {
		trap->Print( "G_PickTarget called with NULL targetname\n" );
		return NULL;
	}

	while ( 1 ) {
		ent = G_Find( ent, FOFS( targetname ), targetname );
		if ( !ent )
			break;
		choice[choices++] = ent;
		if ( choices == MAXCHOICES )
			break;
	}

	if ( !choices ) {
		trap->Print( "G_PickTarget: target %s not found\n", targetname );
		return NULL;
	}

	return choice[rand() % choices];
}

void GlobalUse( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( !self || (self->flags & FL_INACTIVE) )
		return;

	if ( self->use ) {
		self->use( self, other, activator );
	}
}

void G_UseTargets2( gentity_t *ent, gentity_t *activator, const char *string ) {
	gentity_t *t;

	if ( !ent ) {
		return;
	}

	if ( ent->targetShaderName && ent->targetShaderNewName ) {
		float f = level.time * 0.001f;
		AddRemap( ent->targetShaderName, ent->targetShaderNewName, f );
		trap->SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	if ( !string || !string[0] ) {
		return;
	}

	t = NULL;
	while ( (t = G_Find( t, FOFS( targetname ), string )) != NULL ) {
		if ( t == ent ) {
			trap->Print( "WARNING: Entity used itself.\n" );
		}
		else {
			if ( t->use ) {
				GlobalUse( t, ent, activator );
			}
		}
		if ( !ent->inuse ) {
			trap->Print( "entity was removed while using targets\n" );
			return;
		}
	}
}

// "activator" should be set to the entity that initiated the firing.
// Search for (string)targetname in all entities that match (string)self.target and call their .use function
void G_UseTargets( gentity_t *ent, gentity_t *activator ) {
	if ( !ent )
		return;

	G_UseTargets2( ent, activator, ent->target );
}

// The editor only specifies a single value for angles (yaw), but we have special constants to generate an up or down direction.
//	Angles will be cleared, because it is being used to represent a direction instead of an orientation.
void G_SetMovedir( vector3 *angles, vector3 *movedir ) {
	static vector3 VEC_UP = { 0, -1, 0 };
	static vector3 MOVEDIR_UP = { 0, 0, 1 };
	static vector3 VEC_DOWN = { 0, -2, 0 };
	static vector3 MOVEDIR_DOWN = { 0, 0, -1 };

	if ( VectorCompare( angles, &VEC_UP ) )
		VectorCopy( &MOVEDIR_UP, movedir );
	else if ( VectorCompare( angles, &VEC_DOWN ) )
		VectorCopy( &MOVEDIR_DOWN, movedir );
	else
		AngleVectors( angles, movedir, NULL, NULL );

	VectorClear( angles );
}

void G_InitGentity( gentity_t *e ) {
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->s.modelGhoul2 = 0; //assume not

	trap->ICARUS_FreeEnt( (sharedEntity_t *)e ); // ICARUS information must be added after this point
}

// give us some decent info on all the active ents -rww
static void G_SpewEntList( void ) {
	int i = 0, numNPC = 0, numProjectile = 0, numTempEnt = 0, numTempEntST = 0;
	char className[MAX_STRING_CHARS], buf[256] = { 0 };
	gentity_t *ent;
	fileHandle_t fh;
	time_t rawtime;
	const char *str;

	time( &rawtime );
	strftime( buf, sizeof(buf), "%Y-%m-%d_%H-%M-%S", localtime( &rawtime ) );
	trap->FS_Open( va( "entspew_%s.txt", buf ), &fh, FS_WRITE );
	trap->FS_Write( va( "================================\nEntspew triggered at: %s\n================================\n\n", buf ), sizeof(buf), fh );

	for ( i = 0, ent = g_entities; i < ENTITYNUM_MAX_NORMAL; i++, ent++ ) {
		if ( ent->inuse ) {
			if ( ent->s.eType == ET_NPC )
				numNPC++;
			else if ( ent->s.eType == ET_MISSILE )
				numProjectile++;
			else if ( ent->freeAfterEvent ) {
				numTempEnt++;
				if ( ent->s.eFlags & EF_SOUNDTRACKER )
					numTempEntST++;

				str = va( "TEMPENT %4i: EV %i\n", ent->s.number, ent->s.eType - ET_EVENTS );
				Com_Printf( str );
				if ( fh )
					trap->FS_Write( str, strlen( str ), fh );
			}

			if ( ent->classname && ent->classname[0] )
				Q_strncpyz( className, ent->classname, sizeof(className) );
			else
				Q_strncpyz( className, "Unknown", sizeof(className) );
			str = va( "ENT %4i: Classname %s\n", ent->s.number, className );
			Com_Printf( str );
			if ( fh )
				trap->FS_Write( str, strlen( str ), fh );
		}
	}

	str = va( "TempEnt count: %i\nTempEnt ST: %i\nNPC count: %i\nProjectile count: %i\n", numTempEnt, numTempEntST, numNPC, numProjectile );
	Com_Printf( str );
	if ( fh ) {
		trap->FS_Write( str, strlen( str ), fh );
		trap->FS_Close( fh );
	}
}

// Either finds a free entity, or allocates a new one.
//	The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will never be used by anything else.
// Try to avoid reusing an entity that was recently freed, because it can cause the client to think the entity morphed
//	into something else instead of being removed and recreated, which can cause interpolated angles and bad trails.
gentity_t *G_Spawn( void ) {
	int i, force;
	gentity_t *e = NULL;

	for ( force = 0; force < 2; force++ ) {
		// if we go through all entities and can't find one to free, override the normal minimum times before use
		for ( i = MAX_CLIENTS, e = &g_entities[MAX_CLIENTS]; i<level.num_entities; i++, e++ ) {
			if ( e->inuse )
				continue;

			// the first couple seconds of server time can involve a lot of freeing and allocating, so relax the
			//	replacement policy
			if ( !force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000 )
				continue;

			// reuse this slot
			G_InitGentity( e );
			return e;
		}

		if ( i != MAX_GENTITIES )
			break;
	}
	if ( i == ENTITYNUM_MAX_NORMAL ) {
		G_SpewEntList();
		trap->Error( ERR_DROP, "G_Spawn: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap->LocateGameData( (sharedEntity_t *)level.gentities, level.num_entities, sizeof(gentity_t), &level.clients[0].ps, sizeof(level.clients[0]) );

	G_InitGentity( e );
	return e;
}

qboolean G_EntitiesFree( void ) {
	int i;
	gentity_t *e;

	for ( i = MAX_CLIENTS, e = &g_entities[MAX_CLIENTS]; i < level.num_entities; i++, e++ ) {
		if ( !e->inuse )
			return qtrue;
	}

	return qfalse;
}

#define MAX_G2_KILL_QUEUE (256)
static int gG2KillIndex[MAX_G2_KILL_QUEUE];
static int gG2KillNum = 0;

void G_SendG2KillQueue( void ) {
	char g2KillString[1024];
	int i = 0;

	if ( !gG2KillNum )
		return;

	Com_sprintf( g2KillString, sizeof(g2KillString), "kg2" );

	for ( i = 0; i < gG2KillNum && i < 64; i++ )
		Q_strcat( g2KillString, sizeof(g2KillString), va( " %i", gG2KillIndex[i] ) );

	trap->SendServerCommand( -1, g2KillString );

	// Clear the count because we just sent off the whole queue
	gG2KillNum -= i;
	if ( gG2KillNum < 0 ) {
		// hmm, should be impossible, but I'm paranoid as we're far past beta.
		assert( 0 );
		gG2KillNum = 0;
	}
}

void G_KillG2Queue( int entNum ) {
	if ( gG2KillNum >= MAX_G2_KILL_QUEUE ) {
		// This would be considered a Bad Thing.
#ifdef _DEBUG
		Com_Printf( "WARNING: Exceeded the MAX_G2_KILL_QUEUE count for this frame!\n" );
#endif
		// Since we're out of queue slots, just send it now as a seperate command (eats more bandwidth, but we have no choice)
		trap->SendServerCommand( -1, va( "kg2 %i", entNum ) );
		return;
	}

	gG2KillIndex[gG2KillNum++] = entNum;
}

// Marks the entity as free
void G_FreeEntity( gentity_t *ed ) {
	if ( ed && ed->isSaberEntity ) {
#ifdef _DEBUG
		Com_Printf( "Tried to remove JM saber!\n" );
#endif
		return;
	}

	trap->UnlinkEntity( (sharedEntity_t *)ed ); // unlink from world
	trap->ICARUS_FreeEnt( (sharedEntity_t *)ed ); // ICARUS information must be added after this point

	if ( ed->neverFree )
		return;

	//rww - this may seem a bit hackish, but unfortunately we have no access to anything ghoul2-related on the server and
	//	thus must send a message to let the client know he needs to clean up all the g2 stuff for this now-removed entity
	// force all clients to accept an event to destroy this instance, right now
	if ( ed->s.modelGhoul2 )
		G_KillG2Queue( ed->s.number );

	// and free the server instance too, if there is one.
	if ( ed->ghoul2 )
		trap->G2API_CleanGhoul2Models( &ed->ghoul2 );

	// tell the "vehicle pool" that this one is now free
	if ( ed->s.eType == ET_NPC && ed->m_pVehicle )
		G_FreeVehicleObject( ed->m_pVehicle );

	// this "client" structure is one of our dynamically allocated ones, so free the memory
	if ( ed->s.eType == ET_NPC && ed->client ) {
		int saberEntNum = -1;
		int i = 0;
		if ( ed->client->ps.saberEntityNum )
			saberEntNum = ed->client->ps.saberEntityNum;
		else if ( ed->client->saberStoredIndex )
			saberEntNum = ed->client->saberStoredIndex;

		if ( saberEntNum > 0 && g_entities[saberEntNum].inuse ) {
			g_entities[saberEntNum].neverFree = qfalse;
			G_FreeEntity( &g_entities[saberEntNum] );
		}

		for ( i = 0; i < MAX_SABERS; i++ ) {
			if ( ed->client->weaponGhoul2[i] && trap->G2API_HaveWeGhoul2Models( ed->client->weaponGhoul2[i] ) )
				trap->G2API_CleanGhoul2Models( &ed->client->weaponGhoul2[i] );
		}
	}

	if ( ed->s.eFlags & EF_SOUNDTRACKER ) {
		int i = 0;
		gentity_t *ent = NULL;

		for ( i = 0, ent = g_entities; i < MAX_CLIENTS; i++, ent++ ) {
			if ( ent && ent->inuse && ent->client ) {
				int ch = 0;

				for ( ch = TRACK_CHANNEL_NONE - 50; ch < NUM_TRACK_CHANNELS - 50; ch++ ) {
					if ( ent->client->ps.fd.killSoundEntIndex[ch] == ed->s.number )
						ent->client->ps.fd.killSoundEntIndex[ch] = 0;
				}
			}
		}

		// make sure clientside loop sounds are killed on the tracker and client
		trap->SendServerCommand( -1, va( "kls %i %i", ed->s.trickedentindex, ed->s.number ) );
	}

	memset( ed, 0, sizeof(*ed) );
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
}

// Spawns an event entity that will be auto-removed
//	The origin will be snapped to save net bandwidth, so care must be taken if the origin is right on a surface
//	(snap towards start vector first)
gentity_t *G_TempEntity( vector3 *origin, int event ) {
	gentity_t *e;
	vector3 snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, &snapped );
	VectorSnap( &snapped );		// save network bandwidth
	G_SetOrigin( e, &snapped );
	//	VectorCopy( snapped, e->s.origin );

	// find cluster for PVS
	trap->LinkEntity( (sharedEntity_t *)e );

	return e;
}

// scale health down below 1024 to fit in health bits
void G_ScaleNetHealth( gentity_t *self ) {
	int maxHealth = self->maxHealth;

	if ( maxHealth < 1000 ) { //it's good then
		self->s.maxhealth = maxHealth;
		self->s.health = self->health;

		if ( self->s.health < 0 )
			self->s.health = 0;

		return;
	}

	// otherwise, scale it down
	self->s.maxhealth = (maxHealth / 100);
	self->s.health = (self->health / 100);

	if ( self->s.health < 0 )
		self->s.health = 0;

	// don't let it scale to 0 if the thing is still not "dead"
	if ( self->health > 0 && self->s.health <= 0 )
		self->s.health = 1;
}

// Kills all entities that would touch the proposed new positioning of ent. Ent should be unlinked before calling this!
void G_KillBox( gentity_t *ent ) {
	int i, num, touch[MAX_GENTITIES];
	gentity_t *hit = NULL;
	vector3 mins, maxs;

	VectorAdd( &ent->client->ps.origin, &ent->r.mins, &mins );
	VectorAdd( &ent->client->ps.origin, &ent->r.maxs, &maxs );
	num = trap->EntitiesInBox( &mins, &maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		hit = &g_entities[touch[i]];
		if ( !hit->client )
			continue;

		// don't telefrag yourself!
		if ( hit->s.number == ent->s.number )
			continue;

		// don't telefrag your vehicle!
		if ( ent->r.ownerNum == hit->s.number )
			continue;

		// nail it
		G_Damage( hit, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
	}

}

void G_AvoidBox( gentity_t *ent ) {
	int i, num, touch[MAX_GENTITIES];
	gentity_t *hit = NULL;
	vector3 mins, maxs;

	VectorAdd( &ent->client->ps.origin, &ent->r.mins, &mins );
	VectorAdd( &ent->client->ps.origin, &ent->r.maxs, &maxs );
	num = trap->EntitiesInBox( &mins, &maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ ) {
		vector3 newOrg;

		VectorCopy( &ent->client->ps.origin, &newOrg );
		newOrg.z += 64.0f;

		hit = &g_entities[touch[i]];

		if ( !hit->client || hit->s.number == ent->s.number || hit->s.number == ent->r.ownerNum )
			continue;

		TeleportPlayer( ent, &newOrg, &ent->client->ps.viewangles );
		//	G_Damage( hit, ent, ent, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
	}

}

// Use for non-pmove events that would also be predicted on the client side: jumppads and item pickups
//	Adds an event+parm and twiddles the event counter
void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm ) {
	if ( !ent->client )
		return;
	BG_AddPredictableEventToPlayerstate( event, eventParm, &ent->client->ps );
}

// Adds an event+parm and twiddles the event counter
void G_AddEvent( gentity_t *ent, int event, int eventParm ) {
	int bits;

	if ( !event ) {
		trap->Print( "G_AddEvent: zero event added for entity %i\n", ent->s.number );
		return;
	}

	// clients need to add the event in playerState_t instead of entityState_t
	if ( ent->client ) {
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
	}
	else {
		bits = ent->s.event & EV_EVENT_BITS;
		bits = (bits + EV_EVENT_BIT1) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}
	ent->eventTime = level.time;
}

gentity_t *G_PlayEffect( int fxID, vector3 *org, vector3 *ang ) {
	gentity_t *te = G_TempEntity( org, EV_PLAY_EFFECT );
	VectorCopy( ang, &te->s.angles );
	VectorCopy( org, &te->s.origin );
	te->s.eventParm = fxID;

	return te;
}

// play an effect by the G_EffectIndex'd ID instead of a predefined effect ID
gentity_t *G_PlayEffectID( const int fxID, vector3 *org, vector3 *ang ) {
	gentity_t *te;

	te = G_TempEntity( org, EV_PLAY_EFFECT_ID );
	VectorCopy( ang, &te->s.angles );
	VectorCopy( org, &te->s.origin );
	te->s.eventParm = fxID;

	// play off this dir by default then.
	if ( !te->s.angles.x && !te->s.angles.y && !te->s.angles.z )
		te->s.angles.y = 1;

	return te;
}

gentity_t *G_ScreenShake( vector3 *org, gentity_t *target, float intensity, int duration, qboolean global ) {
	gentity_t *te = G_TempEntity( org, EV_SCREENSHAKE );

	VectorCopy( org, &te->s.origin );
	te->s.angles.x = intensity;
	te->s.time = duration;

	if ( target )
		te->s.modelindex = target->s.number + 1;
	else
		te->s.modelindex = 0;

	if ( global )
		te->r.svFlags |= SVF_BROADCAST;

	return te;
}

void G_MuteSound( int entnum, int channel ) {
	gentity_t *te, *e;

	te = G_TempEntity( &vec3_origin, EV_MUTE_SOUND );
	te->r.svFlags = SVF_BROADCAST;
	te->s.trickedentindex2 = entnum;
	te->s.trickedentindex = channel;

	e = &g_entities[entnum];

	if ( e && (e->s.eFlags & EF_SOUNDTRACKER) ) {
		G_FreeEntity( e );
		e->s.eFlags = 0;
	}
}

void G_Sound( gentity_t *ent, int channel, int soundIndex ) {
	gentity_t *te;

	assert( soundIndex );

	te = G_TempEntity( &ent->r.currentOrigin, EV_GENERAL_SOUND/*, channel*/ );
	te->s.eventParm = soundIndex;
	te->s.saberEntityNum = channel;

	// let the client remember the index of the player entity so he can kill the most recent sound on request
	if ( ent && ent->client && channel > TRACK_CHANNEL_NONE ) {
		if ( g_entities[ent->client->ps.fd.killSoundEntIndex[channel - 50]].inuse &&
			ent->client->ps.fd.killSoundEntIndex[channel - 50] > MAX_CLIENTS ) {
			G_MuteSound( ent->client->ps.fd.killSoundEntIndex[channel - 50], CHAN_VOICE );
			if ( ent->client->ps.fd.killSoundEntIndex[channel - 50] > MAX_CLIENTS && g_entities[ent->client->ps.fd.killSoundEntIndex[channel - 50]].inuse )
				G_FreeEntity( &g_entities[ent->client->ps.fd.killSoundEntIndex[channel - 50]] );
			ent->client->ps.fd.killSoundEntIndex[channel - 50] = 0;
		}

		ent->client->ps.fd.killSoundEntIndex[channel - 50] = te->s.number;
		te->s.trickedentindex = ent->s.number;
		te->s.eFlags = EF_SOUNDTRACKER;
		//Raz: Looping sound fixed so all players get information about it, which can be needed later
		te->r.svFlags |= SVF_BROADCAST;
		//	te->freeAfterEvent = qfalse;
	}
}

void G_SoundAtLoc( vector3 *loc, int channel, int soundIndex ) {
	gentity_t *te = G_TempEntity( loc, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
	te->s.saberEntityNum = channel;
}

void G_EntitySound( gentity_t *ent, int channel, int soundIndex ) {
	gentity_t *te = G_TempEntity( &ent->r.currentOrigin, EV_ENTITY_SOUND );
	te->s.eventParm = soundIndex;
	te->s.clientNum = ent->s.number;
	te->s.trickedentindex = channel;
}

// To make porting from SP easier.
void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath ) {
	gentity_t *te = G_TempEntity( &ent->r.currentOrigin, EV_ENTITY_SOUND );
	te->s.eventParm = G_SoundIndex( soundPath );
	te->s.clientNum = ent->s.number;
	te->s.trickedentindex = channel;
}

// Returns whether or not the targeted entity is useable
qboolean ValidUseTarget( gentity_t *ent ) {
	if ( !ent->use )
		return qfalse;

	// set by target_deactivate
	if ( ent->flags & FL_INACTIVE )
		return qfalse;

	// check for flag that denotes BUTTON_USE useability
	if ( !(ent->r.svFlags & SVF_PLAYER_USABLE) )
		return qfalse;

	return qtrue;
}

// use an ammo/health dispenser on another client
void G_UseDispenserOn( gentity_t *ent, int dispType, gentity_t *target ) {
	if ( dispType == HI_HEALTHDISP ) {
		target->client->ps.stats[STAT_HEALTH] += 4;

		if ( target->client->ps.stats[STAT_HEALTH] > target->client->ps.stats[STAT_MAX_HEALTH] )
			target->client->ps.stats[STAT_HEALTH] = target->client->ps.stats[STAT_MAX_HEALTH];

		target->client->timeMedHealed = level.time + 500;
		target->health = target->client->ps.stats[STAT_HEALTH];
	}
	else if ( dispType == HI_AMMODISP ) {
		if ( ent->client->medSupplyDebounce < level.time ) {
			//do the next increment based on the amount of ammo used per normal shot.
			target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] += weaponData[target->client->ps.weapon].energyPerShot;

			if ( target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] > ammoData[weaponData[target->client->ps.weapon].ammoIndex].max )
				target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] = ammoData[weaponData[target->client->ps.weapon].ammoIndex].max;

			// base the next supply time on how long the weapon takes to fire. Seems fair enough.
			ent->client->medSupplyDebounce = level.time + weaponData[target->client->ps.weapon].fireTime;
		}
		target->client->timeMedSupplied = level.time + 500;
	}
}

// see if this guy needs servicing from a specific type of dispenser
int G_CanUseDispOn( gentity_t *ent, int dispType ) {
	if ( !ent->client || !ent->inuse || ent->health < 1 || ent->client->ps.stats[STAT_HEALTH] < 1 )
		return 0;

	if ( dispType == HI_HEALTHDISP ) {
		if ( ent->client->ps.stats[STAT_HEALTH] < ent->client->ps.stats[STAT_MAX_HEALTH] )
			return 1;

		//otherwise no
		return 0;
	}
	else if ( dispType == HI_AMMODISP ) {
		if ( ent->client->ps.weapon <= WP_NONE || ent->client->ps.weapon > LAST_USEABLE_WEAPON )
			return 0;

		if ( ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex] < ammoData[weaponData[ent->client->ps.weapon].ammoIndex].max )
			return 1;

		// needs none
		return 0;
	}

	return 0;
}

qboolean TryHeal( gentity_t *ent, gentity_t *target ) {
	if ( level.gametype == GT_SIEGE && ent->client->siegeClass != -1 && target && target->inuse && target->maxHealth
		&& target->healingclass && target->healingclass[0] && target->health > 0 && target->health < target->maxHealth ) {
		siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];

		if ( !Q_stricmp( scl->name, target->healingclass ) ) {
			// this thing can be healed by the class this player is using
			if ( target->healingDebounce < level.time ) {
				// do the actual heal
				target->health += 10;
				if ( target->health > target->maxHealth )
					target->health = target->maxHealth;

				target->healingDebounce = level.time + target->healingrate;
				if ( target->healingsound && target->healingsound[0] )
					G_Sound( (target->s.solid == SOLID_BMODEL) ? ent : target, CHAN_AUTO, G_SoundIndex( target->healingsound ) );

				// update net health for bar
				G_ScaleNetHealth( target );
				if ( target->target_ent && target->target_ent->maxHealth ) {
					target->target_ent->health = target->health;
					G_ScaleNetHealth( target->target_ent );
				}
			}

			// keep them in the healing anim even when the healing debounce is not yet expired
			if ( ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD || ent->client->ps.torsoAnim == BOTH_CONSOLE1 )
				ent->client->ps.torsoTimer = 500;
			else
				G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );

			return qtrue;
		}
	}

	return qfalse;
}

#define USE_DISTANCE (64.0f)
static vector3 playerMins = { -15, -15, DEFAULT_MINS_2 };
static vector3 playerMaxs = { 15, 15, DEFAULT_MAXS_2 };
// Try and use an entity in the world, directly ahead of us
void TryUse( gentity_t *ent ) {
	gentity_t *target;
	trace_t trace;
	vector3 src, dest, vf, viewspot;

	// siege round hasn't begun
	if ( level.gametype == GT_SIEGE && !gSiegeRoundBegun )
		return;

	// invalid ent or dead
	if ( !ent || !ent->client || ent->health < 1 )
		return;

	// busy
	if ( ent->client->ps.weaponTime > 0 && ent->client->ps.torsoAnim != BOTH_BUTTON_HOLD && ent->client->ps.torsoAnim != BOTH_CONSOLE1 )
		return;

	// spectating
	if ( (ent->client->ps.pm_flags & PMF_FOLLOW) || ent->client->sess.sessionTeam == TEAM_SPECTATOR || ent->client->tempSpectate >= level.time )
		return;

	// dragging a corpse
	if ( ent->client->ps.forceHandExtend != HANDEXTEND_NONE && ent->client->ps.forceHandExtend != HANDEXTEND_DRAGGING )
		return;

	// on an emplaced gun or using a vehicle
	if ( ent->client->ps.emplacedIndex )
		return;

	if ( ent->s.number < MAX_CLIENTS && ent->client && ent->client->ps.m_iVehicleNum ) {
		gentity_t *currentVeh = &g_entities[ent->client->ps.m_iVehicleNum];
		if ( currentVeh->inuse && currentVeh->m_pVehicle ) {
			Vehicle_t *pVeh = currentVeh->m_pVehicle;
			if ( !pVeh->m_iBoarding )
				pVeh->m_pVehicleInfo->Eject( pVeh, (bgEntity_t *)ent, qfalse );
			return;
		}
	}

	if ( ent->client->jetPackOn )
		goto tryJetPack;

	if ( ent->client->bodyGrabIndex != ENTITYNUM_NONE ) {
		//then hitting the use key just means let go
		if ( ent->client->bodyGrabTime < level.time ) {
			gentity_t *grabbed = &g_entities[ent->client->bodyGrabIndex];

			if ( grabbed->inuse ) {
				if ( grabbed->client )
					grabbed->client->ps.ragAttach = 0;
				else
					grabbed->s.ragAttach = 0;
			}
			ent->client->bodyGrabIndex = ENTITYNUM_NONE;
			ent->client->bodyGrabTime = level.time + 1000;
		}
		return;
	}

	VectorCopy( &ent->client->ps.origin, &viewspot );
	viewspot.z += ent->client->ps.viewheight;

	VectorCopy( &viewspot, &src );
	AngleVectors( &ent->client->ps.viewangles, &vf, NULL, NULL );

	VectorMA( &src, USE_DISTANCE, &vf, &dest );

	//Trace ahead to find a valid target
	trap->Trace( &trace, &src, &vec3_origin, &vec3_origin, &dest, ent->s.number, MASK_OPAQUE | CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE, qfalse, 0, 0 );

	if ( trace.fraction == 1.0f || trace.entityNum == ENTITYNUM_NONE ) //Raz: slot 0 fix
		goto tryJetPack;

	target = &g_entities[trace.entityNum];

	//Enable for corpse dragging
#if 0
	if ( target->inuse && target->s.eType == ET_BODY && ent->client->bodyGrabTime < level.time ) {
		// then grab the body
		target->s.eFlags |= EF_RAG; //make sure it's in rag state
		// switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
		if ( !ent->s.number )
			target->s.ragAttach = ENTITYNUM_NONE;
		else
			target->s.ragAttach = ent->s.number;
		ent->client->bodyGrabTime = level.time + 1000;
		ent->client->bodyGrabIndex = target->s.number;
		return;
	}
#endif

	//if target is a vehicle then perform appropriate checks
	if ( target && target->m_pVehicle && target->client && target->s.NPC_class == CLASS_VEHICLE && !ent->client->ps.zoomMode ) {
		Vehicle_t *pVeh = target->m_pVehicle;

		if ( pVeh->m_pVehicleInfo ) {
			// user is already on this vehicle so eject him
			if ( ent->r.ownerNum == target->s.number )
				pVeh->m_pVehicleInfo->Eject( pVeh, (bgEntity_t *)ent, qfalse );
			// otherwise board this vehicle.
			else if ( level.gametype < GT_TEAM || !target->alliedTeam || (target->alliedTeam == ent->client->sess.sessionTeam) ) {
				//not belonging to a team, or client is on same team
				pVeh->m_pVehicleInfo->Board( pVeh, (bgEntity_t *)ent );
			}

			//clear the damn button!
			ent->client->pers.cmd.buttons &= ~BUTTON_USE;
			return;
		}
	}

	if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & ((1 << HI_HEALTHDISP) | (1 << HI_AMMODISP))) &&
		target && target->inuse && target->client && target->health > 0 && OnSameTeam( ent, target ) &&
		(G_CanUseDispOn( target, HI_HEALTHDISP ) || G_CanUseDispOn( target, HI_AMMODISP )) ) {
		// a live target that's on my team, we can use him
		if ( G_CanUseDispOn( target, HI_HEALTHDISP ) )
			G_UseDispenserOn( ent, HI_HEALTHDISP, target );
		if ( G_CanUseDispOn( target, HI_AMMODISP ) )
			G_UseDispenserOn( ent, HI_AMMODISP, target );

		// for now, we will use the standard use anim
		if ( ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD )
			ent->client->ps.torsoTimer = 500;
		else
			G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );

		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		return;
	}

	//Check for a use command
	if ( ValidUseTarget( target ) && (level.gametype != GT_SIEGE
		|| !target->alliedTeam || target->alliedTeam != ent->client->sess.sessionTeam || g_ff_objectives.integer) ) {
		if ( ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD || ent->client->ps.torsoAnim == BOTH_CONSOLE1 )
			ent->client->ps.torsoTimer = 500;
		else
			G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD, 0 );
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		if ( target->touch == Touch_Button )
			target->touch( target, ent, NULL );
		else
			GlobalUse( target, ent, ent );
		return;
	}

	if ( TryHeal( ent, target ) )
		return;

tryJetPack:
	//if we got here, we didn't actually use anything else, so try to toggle jetpack if we are in the air, or if it is already on
	if ( ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK) ) {
		if ( ent->client->jetPackOn || ent->client->ps.groundEntityNum == ENTITYNUM_NONE ) {
			ItemUse_Jetpack( ent );
			return;
		}
	}

	// if you used nothing, then try spewing out some ammo
	if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_AMMODISP)) ) {
		trace_t trToss;
		vector3 fAng, fwd;

		VectorSet( &fAng, 0.0f, ent->client->ps.viewangles.yaw, 0.0f );
		AngleVectors( &fAng, &fwd, 0, 0 );

		VectorMA( &ent->client->ps.origin, 64.0f, &fwd, &fwd );
		trap->Trace( &trToss, &ent->client->ps.origin, &playerMins, &playerMaxs, &fwd, ent->s.number, ent->clipmask, qfalse, 0, 0 );
		if ( trToss.fraction == 1.0f && !trToss.allsolid && !trToss.startsolid ) {
			ItemUse_UseDisp( ent, HI_AMMODISP );
			G_AddEvent( ent, EV_USE_ITEM0 + HI_AMMODISP, 0 );
			return;
		}
	}
}

qboolean G_PointInBounds( vector3 *point, vector3 *mins, vector3 *maxs ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( point->data[i] < mins->data[i] )	return qfalse;
		if ( point->data[i] > maxs->data[i] )	return qfalse;
	}

	return qtrue;
}

qboolean G_BoxInBounds( vector3 *point, vector3 *mins, vector3 *maxs, vector3 *boundsMins, vector3 *boundsMaxs ) {
	vector3 boxMins, boxMaxs;

	VectorAdd( point, mins, &boxMins );
	VectorAdd( point, maxs, &boxMaxs );

	if ( boxMaxs.x > boundsMaxs->x ||
		boxMaxs.y > boundsMaxs->y ||
		boxMaxs.z > boundsMaxs->z ||
		boxMins.x < boundsMins->x ||
		boxMins.y < boundsMins->y ||
		boxMins.z < boundsMins->z )
		return qfalse;

	// box is completely contained within bounds
	return qtrue;
}


void G_SetAngles( gentity_t *ent, vector3 *angles ) {
	VectorCopy( angles, &ent->r.currentAngles );
	VectorCopy( angles, &ent->s.angles );
	VectorCopy( angles, &ent->s.apos.trBase );
}

qboolean G_ClearTrace( vector3 *start, vector3 *mins, vector3 *maxs, vector3 *end, int ignore, int clipmask ) {
	static trace_t tr;

	trap->Trace( &tr, start, mins, maxs, end, ignore, clipmask, qfalse, 0, 0 );

	if ( tr.allsolid || tr.startsolid || tr.fraction < 1.0f )
		return qfalse;

	return qtrue;
}

// Sets the pos trajectory for a fixed position
void G_SetOrigin( gentity_t *ent, vector3 *origin ) {
	VectorCopy( origin, &ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( &ent->s.pos.trDelta );

	VectorCopy( origin, &ent->r.currentOrigin );
}

qboolean G_CheckInSolid( gentity_t *self, qboolean fix ) {
	trace_t trace;
	vector3 end, mins;

	VectorCopy( &self->r.currentOrigin, &end );
	end.z += self->r.mins.z;
	VectorCopy( &self->r.mins, &mins );
	mins.z = 0;

	trap->Trace( &trace, &self->r.currentOrigin, &mins, &self->r.maxs, &end, self->s.number, self->clipmask, qfalse, 0, 0 );
	if ( trace.allsolid || trace.startsolid )
		return qtrue;

	if ( trace.fraction < 1.0f ) {
		// Put them at end of trace and check again
		if ( fix ) {
			vector3 neworg;

			VectorCopy( &trace.endpos, &neworg );
			neworg.z -= self->r.mins.z;
			G_SetOrigin( self, &neworg );
			trap->LinkEntity( (sharedEntity_t *)self );

			return G_CheckInSolid( self, qfalse );
		}
		else
			return qtrue;
	}

	return qfalse;
}

// debug polygons only work when running a local game with r_debugSurface set to 2
int DebugLine( vector3 *start, vector3 *end, int color ) {
	vector3 points[4], dir, cross, up = { 0, 0, 1 };
	float dot;

	VectorCopy( start, &points[0] );
	VectorCopy( start, &points[1] );
	//	points[1][2] -= 2;
	VectorCopy( end, &points[2] );
	//	points[2][2] -= 2;
	VectorCopy( end, &points[3] );


	VectorSubtract( end, start, &dir );
	VectorNormalize( &dir );
	dot = DotProduct( &dir, &up );
	if ( dot > 0.99f || dot < -0.99f )
		VectorSet( &cross, 1, 0, 0 );
	else
		CrossProduct( &dir, &up, &cross );

	VectorNormalize( &cross );

	VectorMA( &points[0], 2, &cross, &points[0] );
	VectorMA( &points[1], -2, &cross, &points[1] );
	VectorMA( &points[2], -2, &cross, &points[2] );
	VectorMA( &points[3], 2, &cross, &points[3] );

	return trap->DebugPolygonCreate( color, 4, points );
}

void G_ROFF_NotetrackCallback( gentity_t *cent, const char *notetrack ) {
	char type[256];
	int i = 0, addlArg = 0;

	if ( !cent || !notetrack )
		return;

	while ( notetrack[i] && notetrack[i] != ' ' ) {
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if ( !i || !type[0] )
		return;

	if ( notetrack[i] == ' ' )
		addlArg = 1;

	if ( !strcmp( type, "loop" ) ) {
		if ( addlArg ) {
			// including an additional argument means reset to original position before loop
			VectorCopy( &cent->s.origin2, &cent->s.pos.trBase );
			VectorCopy( &cent->s.origin2, &cent->r.currentOrigin );
			VectorCopy( &cent->s.angles2, &cent->s.apos.trBase );
			VectorCopy( &cent->s.angles2, &cent->r.currentAngles );
		}

		trap->ROFF_Play( cent->s.number, cent->roffid, qfalse );
	}
}

void G_SpeechEvent( gentity_t *self, int event ) {
	G_AddEvent( self, event, 0 );
}

qboolean G_ExpandPointToBBox( vector3 *point, const vector3 *mins, const vector3 *maxs, int ignore, int clipmask ) {
	trace_t tr;
	vector3 start, end;
	int i;

	VectorCopy( point, &start );

	for ( i = 0; i < 3; i++ ) {
		VectorCopy( &start, &end );
		end.data[i] += mins->data[i];
		trap->Trace( &tr, &start, &vec3_origin, &vec3_origin, &end, ignore, clipmask, qfalse, 0, 0 );
		if ( tr.allsolid || tr.startsolid )
			return qfalse;
		if ( tr.fraction < 1.0f ) {
			VectorCopy( &start, &end );
			end.data[i] += maxs->data[i] - (mins->data[i] * tr.fraction);

			trap->Trace( &tr, &start, &vec3_origin, &vec3_origin, &end, ignore, clipmask, qfalse, 0, 0 );

			if ( tr.allsolid || tr.startsolid )
				return qfalse;

			if ( tr.fraction < 1.0f )
				return qfalse;

			VectorCopy( &end, &start );
		}
	}

	//expanded it, now see if it's all clear
	trap->Trace( &tr, &start, mins, maxs, &start, ignore, clipmask, qfalse, 0, 0 );
	if ( tr.allsolid || tr.startsolid )
		return qfalse;
	VectorCopy( &start, point );
	return qtrue;
}

extern qboolean G_FindClosestPointOnLineSegment( const vector3 *start, const vector3 *end, const vector3 *from, vector3 *result );
float ShortestLineSegBewteen2LineSegs( vector3 *start1, vector3 *end1, vector3 *start2, vector3 *end2, vector3 *close_pnt1, vector3 *close_pnt2 ) {
	float current_dist, new_dist;
	vector3 new_pnt;
	//start1, end1 : the first segment
	//start2, end2 : the second segment

	//output, one point on each segment, the closest two points on the segments.

	//compute some temporaries:
	//vec start_dif = start2 - start1
	vector3 start_dif;
	vector3 v1;
	vector3 v2;
	float v1v1, v2v2, v1v2;
	float denom;

	VectorSubtract( start2, start1, &start_dif );
	//vec v1 = end1 - start1
	VectorSubtract( end1, start1, &v1 );
	//vec v2 = end2 - start2
	VectorSubtract( end2, start2, &v2 );
	//
	v1v1 = DotProduct( &v1, &v1 );
	v2v2 = DotProduct( &v2, &v2 );
	v1v2 = DotProduct( &v1, &v2 );

	//the main computation

	denom = (v1v2 * v1v2) - (v1v1 * v2v2);

	//if denom is small, then skip all this and jump to the section marked below
	if ( fabsf( denom ) > 0.001f ) {
		float s = -((v2v2*DotProduct( &v1, &start_dif )) - (v1v2*DotProduct( &v2, &start_dif ))) / denom;
		float t = ((v1v1*DotProduct( &v2, &start_dif )) - (v1v2*DotProduct( &v1, &start_dif ))) / denom;
		qboolean done = qtrue;

		if ( s < 0 ) {
			done = qfalse;
			s = 0;// and see note below
		}

		if ( s > 1 ) {
			done = qfalse;
			s = 1;// and see note below
		}

		if ( t < 0 ) {
			done = qfalse;
			t = 0;// and see note below
		}

		if ( t > 1 ) {
			done = qfalse;
			t = 1;// and see note below
		}

		//vec close_pnt1 = start1 + s * v1
		VectorMA( start1, s, &v1, close_pnt1 );
		//vec close_pnt2 = start2 + t * v2
		VectorMA( start2, t, &v2, close_pnt2 );

		current_dist = Distance( close_pnt1, close_pnt2 );
		//now, if none of those if's fired, you are done.
		if ( done )
			return current_dist;
		//If they did fire, then we need to do some additional tests.

		//What we are gonna do is see if we can find a shorter distance than the above
		//involving the endpoints.
	}
	//******start here for paralell lines with current_dist = infinity****
	else
		current_dist = Q3_INFINITE;

	//test all the endpoints
	new_dist = Distance( start1, start2 );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( start1, end2 );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( end1, start2 );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( end1, end2 );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	// then we have 4 more point / segment tests

	G_FindClosestPointOnLineSegment( start2, end2, start1, &new_pnt );
	new_dist = Distance( start1, &new_pnt );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( &new_pnt, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start2, end2, end1, &new_pnt );
	new_dist = Distance( end1, &new_pnt );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( &new_pnt, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start1, end1, start2, &new_pnt );
	new_dist = Distance( start2, &new_pnt );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( &new_pnt, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start1, end1, end2, &new_pnt );
	new_dist = Distance( end2, &new_pnt );
	if ( new_dist < current_dist ) {
		// then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( &new_pnt, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	return current_dist;
}

void GetAnglesForDirection( const vector3 *p1, const vector3 *p2, vector3 *out ) {
	vector3 v;

	VectorSubtract( p2, p1, &v );
	vectoangles( &v, out );
}

static qboolean cmpSubCase( const char *s1, const char *s2 ) {
	return (strstr( s1, s2 ) != NULL) ? qtrue : qfalse;
}
static qboolean cmpSub( const char *s1, const char *s2 ) {
	return (Q_stristr( s1, s2 ) != NULL) ? qtrue : qfalse;
}
static qboolean cmpWholeCase( const char *s1, const char *s2 ) {
	return (!strcmp( s1, s2 )) ? qtrue : qfalse;
}
static qboolean cmpWhole( const char *s1, const char *s2 ) {
	return (!Q_stricmp( s1, s2 )) ? qtrue : qfalse;
}

int G_ClientFromString( const gentity_t *ent, const char *match, uint32_t flags ) {
	char cleanedMatch[MAX_NETNAME];
	int i;
	gentity_t *e;
	qboolean substr = !!(flags & FINDCL_SUBSTR);
	qboolean firstMatch = !!(flags & FINDCL_FIRSTMATCH);
	qboolean print = !!(flags & FINDCL_PRINT);
	qboolean caseSensitive = !!(flags & FINDCL_CASE);
	qboolean( *compareFunc )(const char *s1, const char *s2);
	if ( caseSensitive )
		compareFunc = substr ? cmpSubCase : cmpWholeCase;
	else
		compareFunc = substr ? cmpSub : cmpWhole;

	// First check for clientNum match
	if ( Q_StringIsInteger( match ) ) {
		i = atoi( match );
		if ( i >= 0 && i < level.maxclients ) {
			e = g_entities + i;
			if ( e->inuse && e->client->pers.connected != CON_DISCONNECTED )
				return i;
			if ( print )
				trap->SendServerCommand( ent - g_entities, va( "print \"Client %d is not on the server\n\"", i ) );
			return -1;
		}
		else {
			if ( print )
				trap->SendServerCommand( ent - g_entities, va( "print \"Client %d is out of range [0, %d]\n\"", i, level.maxclients - 1 ) );
			return -1;
		}
	}

	// Failed, check for a name match
	Q_strncpyz( cleanedMatch, match, sizeof(cleanedMatch) );
	Q_CleanString( cleanedMatch, STRIP_COLOUR );

	if ( firstMatch ) {
		for ( i = 0, e = g_entities; i < level.maxclients; i++, e++ ) {
			if ( compareFunc( e->client->pers.netnameClean, cleanedMatch ) && e->inuse
				&& e->client->pers.connected != CON_DISCONNECTED ) {
				return i;
			}
		}
	}
	else {
		int numMatches, matches[MAX_CLIENTS];

		// find all matching names
		for ( i = 0, numMatches = 0, e = g_entities; i < level.maxclients; i++, e++ ) {
			if ( !e->inuse || e->client->pers.connected == CON_DISCONNECTED )
				continue;
			if ( compareFunc( e->client->pers.netnameClean, cleanedMatch ) )
				matches[numMatches++] = i;
		}

		// success
		if ( numMatches == 1 )
			return matches[0];

		// multiple matches, can occur on substrings and if duplicate names are allowed
		else if ( numMatches ) {
			char msg[MAX_TOKEN_CHARS];
			Com_sprintf( msg, sizeof(msg), "Found %d matches:\n", numMatches );
			for ( i = 0; i < numMatches; i++ ) {
				Q_strcat( msg, sizeof(msg), va( "  "S_COLOR_WHITE"("S_COLOR_CYAN"%02i"S_COLOR_WHITE") %s\n", matches[i],
					g_entities[matches[i]].client->pers.netname ) );
			}
			trap->SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
			return -1;
		}
	}

	//Failed, target client does not exist
	if ( print )
		trap->SendServerCommand( ent - g_entities, va( "print \"Client %s does not exist\n\"", cleanedMatch ) );
	return -1;
}

// trace from eyes using unlagged
trace_t *G_RealTrace( gentity_t *ent, float dist ) {
	static trace_t tr;
	vector3	start, end;

	if ( japp_unlagged.integer )
		G_TimeShiftAllClients( ent->client->pers.cmd.serverTime, ent );

	//Get start
	VectorCopy( &ent->client->ps.origin, &start );
	start.z += ent->client->ps.viewheight; //36.0f;

	//Get end
	AngleVectors( &ent->client->ps.viewangles, &end, NULL, NULL );
	VectorMA( &start, (dist > 0.0f) ? dist : 16384.0f, &end, &end );

	trap->Trace( &tr, &start, NULL, NULL, &end, ent->s.number, MASK_OPAQUE | CONTENTS_BODY | CONTENTS_ITEM | CONTENTS_CORPSE, qfalse, 0, 0 );

	if ( g_debugTrace.integer ) {
		G_TestLine( &start, &tr.endpos, COLOR_MAGENTA, 2500 );
	}

	if ( japp_unlagged.integer ) {
		G_UnTimeShiftAllClients( ent );
	}

	return &tr;
}

#define NUM_CLIENTINFOBUFFERS (4)
const char *G_PrintClient( int clientNum ) {
	static char buf[NUM_CLIENTINFOBUFFERS][MAX_STRING_CHARS];
	static int index = 0;
	char *out = buf[(index++)&(NUM_CLIENTINFOBUFFERS-1)];
	gentity_t *ent = g_entities + clientNum;

	Com_sprintf( out, MAX_STRING_CHARS, "\"%02i\" \"%s\" \"%s\"", clientNum, ent->client->pers.netname,
		ent->client->sess.IP );

	return out;
}
