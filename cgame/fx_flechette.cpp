#include "cg_local.h"
#include "cg_media.h"

void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.flechette.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_FlechetteWeaponHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.flechette.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_FlechetteWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.flechette.fleshImpact, origin, normal, -1, -1, qfalse );
}

void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.flechette.altShot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}
