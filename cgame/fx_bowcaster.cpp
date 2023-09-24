#include "cg_local.h"
#include "cg_media.h"

void FX_BowcasterProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon) {
    vector3 forward;

    if (VectorNormalize2(&cent->currentState.pos.trDelta, &forward) == 0.0f)
        forward.z = 1.0f;

    trap->FX_PlayEffectID(media.efx.bowcaster.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse);
}

void FX_BowcasterHitWall(vector3 *origin, vector3 *normal) { trap->FX_PlayEffectID(media.efx.bowcaster.impact, origin, normal, -1, -1, qfalse); }

void FX_BowcasterHitPlayer(vector3 *origin, vector3 *normal, qboolean humanoid) {
    trap->FX_PlayEffectID(media.efx.bowcaster.impact, origin, normal, -1, -1, qfalse);
}

void FX_BowcasterAltProjectileThink(centity_t *cent, const struct weaponInfo_s *weapon) {
    vector3 forward;

    if (VectorNormalize2(&cent->currentState.pos.trDelta, &forward) == 0.0f)
        forward.z = 1.0f;

    trap->FX_PlayEffectID(media.efx.bowcaster.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse);
}
