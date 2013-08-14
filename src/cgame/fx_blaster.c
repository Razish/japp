// Blaster Weapon

#include "cg_local.h"

/*
-------------------------
FX_BlasterProjectileThink
-------------------------
*/

void FX_BlasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.blasterShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BlasterAltFireThink
-------------------------
*/
void FX_BlasterAltFireThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( cgs.effects.blasterShotEffect, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

/*
-------------------------
FX_BlasterWeaponHitWall
-------------------------
*/
void FX_BlasterWeaponHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( cgs.effects.blasterWallImpactEffect, origin, normal, -1, -1, qfalse );
}

/*
-------------------------
FX_BlasterWeaponHitPlayer
-------------------------
*/
void FX_BlasterWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? cgs.effects.blasterFleshImpactEffect : cgs.effects.blasterDroidImpactEffect, origin, normal, -1, -1, qfalse );
}
