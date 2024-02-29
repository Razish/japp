#include "g_local.h"

static float VectorAngle(const vector3 *a, const vector3 *b) {
    const float lA = VectorLength(a);
    const float lB = VectorLength(b);
    const float lAB = lA * lB;

    if (lAB == 0.0f) {
        return 0.0f;
    } else {
        return (float)(acosf(DotProduct(a, b) / lAB) * (180.f / M_PI));
    }
}

static void MakeVector(const vector3 *ain, vector3 *vout) {
    float pitch, yaw, tmp;

    pitch = (float)(ain->pitch * M_PI / 180.0f);
    yaw = (float)(ain->yaw * M_PI / 180.0f);
    tmp = (float)cosf(pitch);

    vout->x = (float)(-tmp * -cosf(yaw));
    vout->y = (float)(sinf(yaw) * tmp);
    vout->z = (float)-sinf(pitch);
}

static qboolean SE_IsPlayerCrouching(const gentity_t *ent) {
    const playerState_t *ps = &ent->client->ps;

    // FIXME: This is no proper way to determine if a client is actually in a crouch position, we want to do this in
    //	order to properly hide a client from the enemy when he is crouching behind an obstacle and could not possibly
    //	be seen.

    if (!ent->inuse || !ps) {
        return qfalse;
    }

    if (ps->forceHandExtend == HANDEXTEND_KNOCKDOWN) {
        return qtrue;
    }

    if (ps->pm_flags & PMF_DUCKED) {
        return qtrue;
    }

    switch (ps->legsAnim) {
    case BOTH_GETUP1:
    case BOTH_GETUP2:
    case BOTH_GETUP3:
    case BOTH_GETUP4:
    case BOTH_GETUP5:
    case BOTH_FORCE_GETUP_F1:
    case BOTH_FORCE_GETUP_F2:
    case BOTH_FORCE_GETUP_B1:
    case BOTH_FORCE_GETUP_B2:
    case BOTH_FORCE_GETUP_B3:
    case BOTH_FORCE_GETUP_B4:
    case BOTH_FORCE_GETUP_B5:
    case BOTH_GETUP_BROLL_B:
    case BOTH_GETUP_BROLL_F:
    case BOTH_GETUP_BROLL_L:
    case BOTH_GETUP_BROLL_R:
    case BOTH_GETUP_FROLL_B:
    case BOTH_GETUP_FROLL_F:
    case BOTH_GETUP_FROLL_L:
    case BOTH_GETUP_FROLL_R:
        return qtrue;
    default:
        break;
    }

    switch (ps->torsoAnim) {
    case BOTH_GETUP1:
    case BOTH_GETUP2:
    case BOTH_GETUP3:
    case BOTH_GETUP4:
    case BOTH_GETUP5:
    case BOTH_FORCE_GETUP_F1:
    case BOTH_FORCE_GETUP_F2:
    case BOTH_FORCE_GETUP_B1:
    case BOTH_FORCE_GETUP_B2:
    case BOTH_FORCE_GETUP_B3:
    case BOTH_FORCE_GETUP_B4:
    case BOTH_FORCE_GETUP_B5:
    case BOTH_GETUP_BROLL_B:
    case BOTH_GETUP_BROLL_F:
    case BOTH_GETUP_BROLL_L:
    case BOTH_GETUP_BROLL_R:
    case BOTH_GETUP_FROLL_B:
    case BOTH_GETUP_FROLL_F:
    case BOTH_GETUP_FROLL_L:
    case BOTH_GETUP_FROLL_R:
        return qtrue;
    default:
        break;
    }

    return qfalse;
}

static qboolean SE_RenderInFOV(const gentity_t *self, const vector3 *testOrigin) {
    const float fov = 110.0f;
    vector3 tmp, aim, view;

    VectorCopy(&self->client->ps.origin, &tmp);
    VectorSubtract(testOrigin, &tmp, &aim);
    MakeVector(&self->client->ps.viewangles, &view);

    // don't network if they're not in our field of view
    // TODO: only skip if they haven't been in our field of view for ~500ms to avoid flickering
    // TODO: also check distance, factoring in delta angle
    if (VectorAngle(&view, &aim) > (fov / 1.2f)) {
#ifdef _DEBUG
        if (self->s.number == 0) {
            trap->Print("WALLHACK[%i]: not in field of view\n", level.time);
        }
#endif // _DEBUG
        return qfalse;
    }

    return qtrue;
}

static qboolean SE_RenderIsVisible(const gentity_t *self, const vector3 *startPos, const vector3 *testOrigin, qboolean reversedCheck) {
    trace_t results;

    trap->Trace(&results, startPos, NULL, NULL, testOrigin, self - g_entities, MASK_SOLID, qfalse, 0, 0);

    if (results.fraction < 1.0f) {
        if ((results.surfaceFlags & SURF_FORCEFIELD) || (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_GLASS ||
            (results.surfaceFlags & MATERIAL_MASK) == MATERIAL_SHATTERGLASS) {
            // FIXME: This is a quick hack to render people and things through glass and force fields, but will also take
            //	effect even if there is another wall between them (and double glass) - which is bad, of course, but
            //	nothing i can prevent right now.
            if (reversedCheck || SE_RenderIsVisible(self, testOrigin, startPos, qtrue)) {
                return qtrue;
            }
        }

        return qfalse;
    }

    return qtrue;
}

// [0]	= playerOrigin
// [1]	= playerOrigin							+ height
// [2]	= playerOrigin	+ forward
// [3]	= playerOrigin	+ forward				+ height
// [4]	= playerOrigin	+ right		- forward
// [5]	= playerOrigin	+ right		- forward	+ height
// [6]	= playerOrigin	- right		- forward
// [7]	= playerOrigin	- right		- forward	+ height
static qboolean SE_RenderPlayerPoints(qboolean isCrouching, const vector3 *playerAngles, const vector3 *playerOrigin, vector3 playerPoints[9]) {
    int isHeight = isCrouching ? 32 : 56;
    vector3 forward, right, up;

    AngleVectors(playerAngles, &forward, &right, &up);

    VectorMA(playerOrigin, 32.0f, &up, &playerPoints[0]);
    VectorMA(playerOrigin, 64.0f, &forward, &playerPoints[1]);
    VectorMA(&playerPoints[1], 64.0f, &right, &playerPoints[1]);
    VectorMA(playerOrigin, 64.0f, &forward, &playerPoints[2]);
    VectorMA(&playerPoints[2], -64.0f, &right, &playerPoints[2]);
    VectorMA(playerOrigin, -64.0f, &forward, &playerPoints[3]);
    VectorMA(&playerPoints[3], 64.0f, &right, &playerPoints[3]);
    VectorMA(playerOrigin, -64.0f, &forward, &playerPoints[4]);
    VectorMA(&playerPoints[4], -64.0f, &right, &playerPoints[4]);

    VectorMA(&playerPoints[1], isHeight, &up, &playerPoints[5]);
    VectorMA(&playerPoints[2], isHeight, &up, &playerPoints[6]);
    VectorMA(&playerPoints[3], isHeight, &up, &playerPoints[7]);
    VectorMA(&playerPoints[4], isHeight, &up, &playerPoints[8]);

    return qtrue;
}

static qboolean SE_RenderPlayerChecks(const gentity_t *self, const vector3 *playerOrigin, vector3 playerPoints[9]) {
    trace_t results;
    int i;

    for (i = 0; i < 9; i++) {
        if (trap->PointContents(&playerPoints[i], self - g_entities) == CONTENTS_SOLID) {
            trap->Trace(&results, playerOrigin, NULL, NULL, &playerPoints[i], self - g_entities, MASK_SOLID, qfalse, 0, 0);
            VectorCopy(&results.endpos, &playerPoints[i]);
        }
    }

    return qtrue;
}

static qboolean SE_NetworkPlayer(const gentity_t *self, const gentity_t *other) {
    int i;
    vector3 startPos, targPos[9];
    uint32_t contents;

    VectorCopy(&self->client->ps.origin, &startPos);

    contents = trap->PointContents(&startPos, self - g_entities);

    // translucent, we should probably just network them anyways
    if (contents & (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME)) {
        return qtrue;
    }

    // entirely in an opaque surface, no point networking them.
    if (contents & (CONTENTS_SOLID | CONTENTS_TERRAIN | CONTENTS_OPAQUE)) {
#ifdef _DEBUG
        if (self->s.number == 0) {
            trap->Print("WALLHACK[%i]: inside opaque surface\n", level.time);
        }
#endif // _DEBUG
        return qfalse;
    }

    // plot their bbox pointer into targPos[]
    SE_RenderPlayerPoints(SE_IsPlayerCrouching(other), &other->client->ps.viewangles, &other->client->ps.origin, targPos);
    SE_RenderPlayerChecks(self, &other->client->ps.origin, targPos);

    for (i = 0; i < 9; i++) {
        if (SE_RenderIsVisible(self, &startPos, &targPos[i], qfalse)) {
            return qtrue;
        }
    }

#ifdef _DEBUG
    if (self->s.number == 0) {
        trap->Print("WALLHACK[%i]: not visible\n", level.time);
    }
#endif // _DEBUG

    return qfalse;
}

// Tracing non-players seems to have a bad effect, we know players are limited to 32 per frame, however other gentities
//	that are being added are not! It's stupid to actually add traces for it, even with a limited form i used before of 2
//	traces per object. There are too many to track and simply networking them takes less FPS either way
qboolean G_EntityOccluded(const gentity_t *self, const gentity_t *other) {
    // This is a non-player object, just send it (see above).
    if (!other->inuse || other->s.number >= level.maxclients) {
        return qtrue;
    }

    // If this player is me, or my spectee, we will always draw and don't trace.
    if (self == other) {
        return qtrue;
    }

    if (self->client->ps.zoomMode) {
        return qtrue;
    }

    // Not rendering; this player is not in our FOV.
    if (!SE_RenderInFOV(self, &other->client->ps.origin)) {
        return qtrue;
    }

    // Not rendering; this player's traces did not appear in my screen.
    if (!SE_NetworkPlayer(self, other)) {
        return qtrue;
    }

    return qfalse;
}
