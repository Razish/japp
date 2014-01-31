// Bryar Pistol Weapon Effects

#include "cg_local.h"
#include "fx_local.h"

/*
-------------------------

	MAIN FIRE

-------------------------
FX_BryarProjectileThink
-------------------------
*/
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.bryarShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarHitWall
-------------------------
*/
void FX_BryarHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarHitPlayer
-------------------------
*/
void FX_BryarHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? cgs.effects.bryarFleshImpactEffect : cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
}


/*
-------------------------

	ALT FIRE

-------------------------
FX_BryarAltProjectileThink
-------------------------
*/
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;
	int t;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	// see if we have some sort of extra charge going on
	for (t = 1; t < cent->currentState.generic1; t++ )
	{
		// just add ourselves over, and over, and over when we are charged
		trap->FX_PlayEffectID( cgs.effects.bryarPowerupShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
	}

	//	for ( int t = 1; t < cent->gent->count; t++ )	// The single player stores the charge in count, which isn't accessible on the client

	trap->FX_PlayEffectID( cgs.effects.bryarShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BryarAltHitWall
-------------------------
*/
void FX_BryarAltHitWall( vector3 *origin, vector3 *normal, int power )
{
	switch( power )
	{
	case 4:
	case 5:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect3, origin, normal, -1, -1, qfalse );
		break;

	case 2:
	case 3:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect2, origin, normal, -1, -1, qfalse );
		break;

	default:
		trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
		break;
	}
}

/*
-------------------------
FX_BryarAltHitPlayer
-------------------------
*/
void FX_BryarAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? cgs.effects.bryarFleshImpactEffect : cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
}


//TURRET
/*
-------------------------
FX_TurretProjectileThink
-------------------------
*/
void FX_TurretProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.turretShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_TurretHitWall
-------------------------
*/
void FX_TurretHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.bryarWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_TurretHitPlayer
-------------------------
*/
void FX_TurretHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? cgs.effects.bryarFleshImpactEffect : cgs.effects.bryarDroidImpactEffect, origin, normal, -1, -1, qfalse );
}



//CONCUSSION (yeah, should probably make a new file for this.. or maybe just move all these stupid semi-redundant fx_ functions into one file)
/*
-------------------------
FX_ConcussionHitWall
-------------------------
*/
void FX_ConcussionHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_ConcussionHitPlayer
-------------------------
*/
void FX_ConcussionHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( cgs.effects.concussionImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_ConcussionProjectileThink
-------------------------
*/
void FX_ConcussionProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.concussionShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
---------------------------
FX_ConcAltShot
---------------------------
*/
static vector3 WHITE	={1.0f,1.0f,1.0f};
static vector3 BRIGHT={0.75f,0.5f,1.0f};

void FX_ConcAltShot( vector3 *start, vector3 *end )
{
	//"concussion/beam"
	trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f,
							1.0f, 0.0f, 0.0f,
							&WHITE, &WHITE, 0.0f,
							175, trap->R_RegisterShader( "gfx/effects/blueLine" ),
							FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	// add some beef
	trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						&BRIGHT, &BRIGHT, 0.0f,
						150, trap->R_RegisterShader( "gfx/misc/whiteline2" ),
						FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}
