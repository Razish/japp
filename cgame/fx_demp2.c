#include "cg_local.h"
#include "cg_media.h"

void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.demp2.projectile, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_DEMP2_HitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.demp2.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_DEMP2_HitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.demp2.fleshImpact, origin, normal, -1, -1, qfalse );
}

void FX_DEMP2_AltDetonate( vector3 *org, float size ) {
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_FADE_SCALE_MODEL;
	memset( &ex->refEntity, 0, sizeof( refEntity_t ));

	ex->refEntity.renderfx |= RF_VOLUMETRIC;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 800;//1600;

	ex->radius = size;
	ex->refEntity.customShader = media.gfx.world.demp2Shell;
	ex->refEntity.hModel = media.models.demp2Shell;
	VectorCopy( org, &ex->refEntity.origin );

	ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
}
