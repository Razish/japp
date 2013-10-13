// Golan Arms Flechette Weapon

#include "cg_local.h"

/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.flechetteShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_FlechetteWeaponHitWall
-------------------------
*/
void FX_FlechetteWeaponHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.flechetteWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_FlechetteWeaponHitPlayer
-------------------------
*/
void FX_FlechetteWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( cgs.effects.flechetteFleshImpactEffect, origin, normal, -1, -1, qfalse );
}


/*
-------------------------
FX_FlechetteProjectileThink
-------------------------
*/

void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.flechetteAltShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}
