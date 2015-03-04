#include "cg_local.h"
#include "cg_media.h"

void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.rocket.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_RocketHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.rocket.explosion, origin, normal, -1, -1, qfalse );
}

void FX_RocketHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.rocket.explosion, origin, normal, -1, -1, qfalse );
}

void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.rocket.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}
