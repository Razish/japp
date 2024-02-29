// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"
#include "cg_media.h"
#include "qcommon/q_shared.h"

const stringID_table_t leTypeStrings[NUM_LE_TYPES + 1] = {
    ENUM2STRING(LE_NONE),
    ENUM2STRING(LE_FADE_SCALE_MODEL),
    ENUM2STRING(LE_FRAGMENT),
    ENUM2STRING(LE_PUFF),
    ENUM2STRING(LE_FADE_RGB),
    ENUM2STRING(LE_SCOREPLUM),
    ENUM2STRING(LE_OLINE),
    ENUM2STRING(LE_LINE),
    {NULL, -1},
};

#define MAX_LOCAL_ENTITIES (2048)
localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES] = {};
localEntity_t cg_activeLocalEntities = {};     // double linked list
localEntity_t *cg_freeLocalEntities = nullptr; // single linked list
static uint32_t localEntitySpawnCount = 0u;

// This is called at startup and for tournament restarts
void CG_InitLocalEntities(void) {
    memset(cg_localEntities, 0, sizeof(cg_localEntities));
    cg_activeLocalEntities.next = &cg_activeLocalEntities;
    cg_activeLocalEntities.prev = &cg_activeLocalEntities;
    cg_freeLocalEntities = cg_localEntities;
    for (int i = 0; i < MAX_LOCAL_ENTITIES - 1; i++) {
        cg_localEntities[i].next = &cg_localEntities[i + 1];
    }
}

void CG_FreeLocalEntity(localEntity_t *le) {
    if (!le->prev) {
        trap->Error(ERR_DROP, "CG_FreeLocalEntity: not active");
        return;
    }
    // remove from the doubly linked active list
    le->prev->next = le->next;
    le->next->prev = le->prev;

    // the free list is only singly linked
    le->next = cg_freeLocalEntities;
    cg_freeLocalEntities = le;
}

// will allways succeed, even if it requires freeing an old active entity
localEntity_t *CG_AllocLocalEntity(void) {
    if (!cg_freeLocalEntities) {
        // no free entities, so free the one at the end of the chain
        // remove the oldest active entity
        CG_FreeLocalEntity(cg_activeLocalEntities.prev);
    }

    localEntity_t *le = cg_freeLocalEntities;
    cg_freeLocalEntities = cg_freeLocalEntities->next;
    memset(le, 0, sizeof(*le));

    // link into the active list
    le->next = cg_activeLocalEntities.next;
    le->prev = &cg_activeLocalEntities;
    cg_activeLocalEntities.next->prev = le;
    cg_activeLocalEntities.next = le;

    le->id = localEntitySpawnCount++;

    return le;
}

localEntity_t *CG_FindLocalEntity(uint32_t id) {
    for (localEntity_t *le = cg_activeLocalEntities.prev; le != &cg_activeLocalEntities; le = le->prev) {
        if (le->id == id) {
            return le;
        }
    }

    return nullptr;
}

// A fragment localentity interacts with the environment in some way (hitting walls), or generates more localentities
//	along a trail.

void CG_FragmentBounceSound(localEntity_t *le, trace_t *trace) {
    // half the fragments will make a bounce sounds
    if (rand() & 1) {
        sfxHandle_t s = 0;

        switch (le->leBounceSoundType) {
        case LEBS_ROCK:
            s = media.sounds.environment.rockBounce[Q_irand(0, 1)];
            break;
        case LEBS_METAL:
            s = media.sounds.environment
                    .metalBounce[Q_irand(0, 1)]; // FIXME: make sure that this sound is registered properly...might still be rock bounce sound....
            break;
        default:
            return;
        }

        if (s) {
            trap->S_StartSound(&trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s);
        }

        // bouncers only make the sound once...
        // FIXME: arbitrary...change if it bugs you
        le->leBounceSoundType = LEBS_NONE;
    } else if (rand() & 1) {
        // we may end up bouncing again, but each bounce reduces the chance of playing the sound again or they may make a lot of noise when they settle
        // FIXME: maybe just always do this??
        le->leBounceSoundType = LEBS_NONE;
    }
}

void CG_ReflectVelocity(localEntity_t *le, trace_t *trace) {
    vector3 velocity;
    float dot;
    int hitTime;

    // reflect the velocity on the trace plane
    hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
    BG_EvaluateTrajectoryDelta(&le->pos, hitTime, &velocity);
    dot = DotProduct(&velocity, &trace->plane.normal);
    VectorMA(&velocity, -2 * dot, &trace->plane.normal, &le->pos.trDelta);

    VectorScale(&le->pos.trDelta, le->bounceFactor, &le->pos.trDelta);

    VectorCopy(&trace->endpos, &le->pos.trBase);
    le->pos.trTime = cg.time;

    // check for stop, making sure that even on low FPS systems it doesn't bobble
    if (trace->allsolid || (trace->plane.normal.z > 0 && (le->pos.trDelta.z < 40 || le->pos.trDelta.z < -cg.frametime * le->pos.trDelta.z))) {
        le->pos.trType = TR_STATIONARY;
    } else {
    }
}

void CG_AddFragment(localEntity_t *le) {
    vector3 newOrigin;
    trace_t trace;

    if (le->forceAlpha) {
        le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
        le->refEntity.shaderRGBA[3] = le->forceAlpha;
    }

    if (le->pos.trType == TR_STATIONARY) {
        // sink into the ground if near the removal time
        int t;
        float t_e;

        t = le->endTime - cg.time;
        if (t < (SINK_TIME * 2)) {
            le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
            t_e = (float)((float)(le->endTime - cg.time) / (SINK_TIME * 2));
            t_e = (int)((t_e)*255);

            if (t_e > 255) {
                t_e = 255;
            }
            if (t_e < 1) {
                t_e = 1;
            }

            if (le->refEntity.shaderRGBA[3] && t_e > le->refEntity.shaderRGBA[3]) {
                t_e = le->refEntity.shaderRGBA[3];
            }

            le->refEntity.shaderRGBA[3] = t_e;

            SE_R_AddRefEntityToScene(&le->refEntity, MAX_CLIENTS);
        } else {
            SE_R_AddRefEntityToScene(&le->refEntity, MAX_CLIENTS);
        }

        return;
    }

    // calculate new position
    BG_EvaluateTrajectory(&le->pos, cg.time, &newOrigin);

    // trace a line from previous position to new position
    CG_Trace(&trace, &le->refEntity.origin, NULL, NULL, &newOrigin, -1, CONTENTS_SOLID);
    if (trace.fraction == 1.0f) {
        // still in free fall
        VectorCopy(&newOrigin, &le->refEntity.origin);

        if (le->leFlags & LEF_TUMBLE) {
            vector3 angles;

            BG_EvaluateTrajectory(&le->angles, cg.time, &angles);
            AnglesToAxis(&angles, le->refEntity.axis);
            ScaleModelAxis(&le->refEntity);
        }

        SE_R_AddRefEntityToScene(&le->refEntity, MAX_CLIENTS);

        return;
    }

    // if it is in a nodrop zone, remove it
    // this keeps gibs from waiting at the bottom of pits of death
    // and floating levels
    if (trap->CM_PointContents(&trace.endpos, 0) & CONTENTS_NODROP) {
        CG_FreeLocalEntity(le);
        return;
    }

    if (!trace.startsolid) {
        // do a bouncy sound
        CG_FragmentBounceSound(le, &trace);

        if (le->bounceSound) { // specified bounce sound (debris)
            trap->S_StartSound(&le->pos.trBase, ENTITYNUM_WORLD, CHAN_AUTO, le->bounceSound);
        }

        // reflect the velocity on the trace plane
        CG_ReflectVelocity(le, &trace);

        SE_R_AddRefEntityToScene(&le->refEntity, MAX_CLIENTS);
    }
}

// TRIVIAL LOCAL ENTITIES
// These only do simple scaling or modulation before passing to the renderer

void CG_AddFadeRGB(localEntity_t *le) {
    refEntity_t *re;
    float c;

    re = &le->refEntity;

    c = (le->endTime - cg.time) * le->lifeRate;
    c *= 0xff;

    re->shaderRGBA[0] = le->color[0] * c;
    re->shaderRGBA[1] = le->color[1] * c;
    re->shaderRGBA[2] = le->color[2] * c;
    re->shaderRGBA[3] = le->color[3] * c;

    SE_R_AddRefEntityToScene(re, MAX_CLIENTS);
}

static void CG_AddFadeScaleModel(localEntity_t *le) {
    refEntity_t *ent = &le->refEntity;

    float frac = (cg.time - le->startTime) / ((float)(le->endTime - le->startTime));

    frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

    ent->nonNormalizedAxes = qtrue;

    AxisCopy(axisDefault, ent->axis);

    VectorScale(&ent->axis[0], le->radius * frac, &ent->axis[0]);
    VectorScale(&ent->axis[1], le->radius * frac, &ent->axis[1]);
    VectorScale(&ent->axis[2], le->radius * 0.5f * frac, &ent->axis[2]);

    frac = 1.0f - frac;

    ent->shaderRGBA[0] = le->color[0] * frac;
    ent->shaderRGBA[1] = le->color[1] * frac;
    ent->shaderRGBA[2] = le->color[2] * frac;
    ent->shaderRGBA[3] = le->color[3] * frac;

    // add the entity
    SE_R_AddRefEntityToScene(ent, MAX_CLIENTS);
}

static void CG_AddPuff(localEntity_t *le) {
    refEntity_t *re;
    float c;
    vector3 delta;
    float len;
    refdef_t *refdef = CG_GetRefdef();

    re = &le->refEntity;

    // fade / grow time
    c = (le->endTime - cg.time) / (float)(le->endTime - le->startTime);

    re->shaderRGBA[0] = le->color[0] * c;
    re->shaderRGBA[1] = le->color[1] * c;
    re->shaderRGBA[2] = le->color[2] * c;

    re->radius = le->radius * (1.0f - c) + 8;

    BG_EvaluateTrajectory(&le->pos, cg.time, &re->origin);

    // if the view would be "inside" the sprite, kill the sprite
    // so it doesn't add too much overdraw
    VectorSubtract(&re->origin, &refdef->vieworg, &delta);
    len = VectorLength(&delta);
    if (len < le->radius) {
        CG_FreeLocalEntity(le);
        return;
    }

    SE_R_AddRefEntityToScene(re, MAX_CLIENTS);
}

#define NUMBER_SIZE 8

void CG_AddScorePlum(localEntity_t *le) {
    refEntity_t *re;
    vector3 origin, delta, dir, vec, up = {0, 0, 1};
    float c, len;
    int i, score, digits[10], numdigits, negative;
    refdef_t *refdef = CG_GetRefdef();

    re = &le->refEntity;

    c = (le->endTime - cg.time) * le->lifeRate;

    score = le->radius;
    if (score < 0) {
        re->shaderRGBA[0] = 0xff;
        re->shaderRGBA[1] = 0x11;
        re->shaderRGBA[2] = 0x11;
    } else {
        re->shaderRGBA[0] = 0xff;
        re->shaderRGBA[1] = 0xff;
        re->shaderRGBA[2] = 0xff;
        if (score >= 50) {
            re->shaderRGBA[1] = 0;
        } else if (score >= 20) {
            re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
        } else if (score >= 10) {
            re->shaderRGBA[2] = 0;
        } else if (score >= 2) {
            re->shaderRGBA[0] = re->shaderRGBA[2] = 0;
        }
    }
    if (c < 0.25f)
        re->shaderRGBA[3] = 0xff * 4 * c;
    else
        re->shaderRGBA[3] = 0xff;

    re->radius = NUMBER_SIZE / 2;

    VectorCopy(&le->pos.trBase, &origin);
    origin.z += 110 - c * 100;

    VectorSubtract(&refdef->vieworg, &origin, &dir);
    CrossProduct(&dir, &up, &vec);
    VectorNormalize(&vec);

    VectorMA(&origin, -10 + 20 * sinf(c * 2 * M_PI), &vec, &origin);

    // if the view would be "inside" the sprite, kill the sprite
    // so it doesn't add too much overdraw
    VectorSubtract(&origin, &refdef->vieworg, &delta);
    len = VectorLength(&delta);
    if (len < 20) {
        CG_FreeLocalEntity(le);
        return;
    }

    negative = qfalse;
    if (score < 0) {
        negative = qtrue;
        score = -score;
    }

    for (numdigits = 0; !(numdigits && !score); numdigits++) {
        digits[numdigits] = score % 10;
        score = score / 10;
    }

    if (negative) {
        digits[numdigits] = 10;
        numdigits++;
    }

    for (i = 0; i < numdigits; i++) {
        VectorMA(&origin, (float)(((float)numdigits / 2) - i) * NUMBER_SIZE, &vec, &re->origin);
        re->customShader = media.gfx.interface.numbers[digits[numdigits - 1 - i]];
        SE_R_AddRefEntityToScene(re, MAX_CLIENTS);
    }
}

// For forcefields/other rectangular things
void CG_AddOLine(localEntity_t *le) {
    refEntity_t *re;
    float frac, alpha;

    re = &le->refEntity;

    frac = (cg.time - le->startTime) / (float)(le->endTime - le->startTime);
    if (frac > 1)
        frac = 1.0f; // can happen during connection problems
    else if (frac < 0)
        frac = 0.0f;

    // Use the liferate to set the scale over time.
    re->data.line.width = le->data.line.width + (le->data.line.dwidth * frac);
    if (re->data.line.width <= 0) {
        CG_FreeLocalEntity(le);
        return;
    }

    // We will assume here that we want additive transparency effects.
    alpha = le->alpha + (le->dalpha * frac);
    re->shaderRGBA[0] = 0xff * alpha;
    re->shaderRGBA[1] = 0xff * alpha;
    re->shaderRGBA[2] = 0xff * alpha;
    re->shaderRGBA[3] = 0xff * alpha; // Yes, we could apply c to this too, but fading the color is better for lines.

    re->shaderTexCoord.x = 1;
    re->shaderTexCoord.y = 1;

    re->rotation = 90;

    re->reType = RT_ORIENTEDLINE;

    SE_R_AddRefEntityToScene(re, MAX_CLIENTS);
}

// for beams and the like.
void CG_AddLine(localEntity_t *le) {
    refEntity_t *re;

    re = &le->refEntity;

    re->reType = RT_LINE;

    SE_R_AddRefEntityToScene(re, MAX_CLIENTS);
}

void CG_AddLocalEntities(void) {
    // int numAdded[NUM_LE_TYPES] = {};

    localEntity_t *next;
    // walk the list backwards, so any new local entities generated (trails, marks, etc) will be present this frame
    for (localEntity_t *le = cg_activeLocalEntities.prev; le != &cg_activeLocalEntities; le = next) {
        // grab next now, so if the local entity is freed we still have it
        next = le->prev;

        if (cg.time >= le->endTime) {
            CG_FreeLocalEntity(le);
            continue;
        }

        // numAdded[le->leType]++;

        switch (le->leType) {
        default:
            break;

        case LE_FADE_SCALE_MODEL:
            CG_AddFadeScaleModel(le);
            break;

        case LE_FRAGMENT:
            CG_AddFragment(le);
            break;

        case LE_PUFF:
            CG_AddPuff(le);
            break;

        case LE_FADE_RGB:
            CG_AddFadeRGB(le);
            break;

        case LE_SCOREPLUM:
            CG_AddScorePlum(le);
            break;

        case LE_OLINE:
            CG_AddOLine(le);
            break;

        case LE_LINE:
            CG_AddLine(le);
            break;
        }
    }

    // for (leType_e leType = LE_NONE; leType < NUM_LE_TYPES; leType = (leType_e)(leType + 1)) {
    //     const char *leTypeStr = GetStringForID(leTypeStrings, leType);
    //     Com_Printf("CG_AddLocalEntities: %s: %i\n", leTypeStr, numAdded[leType]);
    // }
}
