// Bowcaster Weapon

#include "cg_local.h"

/*
---------------------------
FX_BowcasterProjectileThink
---------------------------
*/

void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.bowcasterShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
---------------------------
FX_BowcasterHitWall
---------------------------
*/

void FX_BowcasterHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.bowcasterImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
---------------------------
FX_BowcasterHitPlayer
---------------------------
*/

void FX_BowcasterHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( cgs.effects.bowcasterImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
------------------------------
FX_BowcasterAltProjectileThink
------------------------------
*/

void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.bowcasterShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

