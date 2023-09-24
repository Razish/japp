#include "cg_local.h"
#include "qcommon/q_shared.h"
#include "Ghoul2/G2.h"
#include "cg_media.h"

// rww - The turret is heavily dependant on bone angles. We can't happily set that on the server, so it is done client-only.

void CreepToPosition(vector3 *ideal, vector3 *current) {
    float max_degree_switch = 90;
    int degrees_negative = 0, degrees_positive = 0;
    int angle_ideal, angle_current;
    qboolean doNegative = qfalse;

    angle_ideal = (int)ideal->yaw;
    angle_current = (int)current->yaw;

    if (angle_ideal <= angle_current) {
        degrees_negative = (angle_current - angle_ideal);
        degrees_positive = (360 - angle_current) + angle_ideal;
    } else {
        degrees_negative = angle_current + (360 - angle_ideal);
        degrees_positive = (angle_ideal - angle_current);
    }

    if (degrees_negative < degrees_positive)
        doNegative = qtrue;

    if (doNegative) {
        current->yaw -= max_degree_switch;

        if (current->yaw < ideal->yaw && (current->yaw + (max_degree_switch * 2)) >= ideal->yaw)
            current->yaw = ideal->yaw;
        if (current->yaw < 0)
            current->yaw += 361;
    } else {
        current->yaw += max_degree_switch;

        if (current->yaw > ideal->yaw && (current->yaw - (max_degree_switch * 2)) <= ideal->yaw)
            current->yaw = ideal->yaw;
        if (current->yaw > 360)
            current->yaw -= 361;
    }

    if (ideal->pitch < 0)
        ideal->pitch += 360;

    angle_ideal = (int)ideal->pitch;
    angle_current = (int)current->pitch;

    doNegative = qfalse;

    if (angle_ideal <= angle_current) {
        degrees_negative = (angle_current - angle_ideal);
        degrees_positive = (360 - angle_current) + angle_ideal;
    } else {
        degrees_negative = angle_current + (360 - angle_ideal);
        degrees_positive = (angle_ideal - angle_current);
    }

    if (degrees_negative < degrees_positive)
        doNegative = qtrue;

    if (doNegative) {
        current->pitch -= max_degree_switch;

        if (current->pitch < ideal->pitch && (current->pitch + (max_degree_switch * 2)) >= ideal->pitch)
            current->pitch = ideal->pitch;
        if (current->pitch < 0)
            current->pitch += 361;
    } else {
        current->pitch += max_degree_switch;

        if (current->pitch > ideal->pitch && (current->pitch - (max_degree_switch * 2)) <= ideal->pitch)
            current->pitch = ideal->pitch;
        if (current->pitch > 360)
            current->pitch -= 361;
    }
}

void TurretClientRun(centity_t *ent) {
    if (!ent->ghoul2) {
        weaponInfo_t *weaponInfo;

        trap->G2API_InitGhoul2Model(&ent->ghoul2, CG_ConfigString(CS_MODELS + ent->currentState.modelindex), 0, 0, 0, 0, 0);

        if (!ent->ghoul2)
            return;

        ent->torsoBolt = trap->G2API_AddBolt(ent->ghoul2, 0, "*flash02");

        trap->G2API_SetBoneAngles(ent->ghoul2, 0, "bone_hinge", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time);
        trap->G2API_SetBoneAngles(ent->ghoul2, 0, "bone_gback", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time);
        trap->G2API_SetBoneAngles(ent->ghoul2, 0, "bone_barrel", &vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 100, cg.time);

        trap->G2API_SetBoneAnim(ent->ghoul2, 0, "model_root", 0, 11, BONE_ANIM_OVERRIDE_FREEZE, 0.8f, cg.time, 0, 0);

        ent->turAngles.roll = 0;
        ent->turAngles.pitch = 90;
        ent->turAngles.yaw = 0;

        weaponInfo = &cg_weapons[WP_TURRET];

        if (!weaponInfo->registered)
            CG_RegisterWeapon(WP_TURRET);
    }

    if (ent->currentState.fireflag == 2) {
        // I'm about to blow
        trap->G2API_SetBoneAngles(ent->ghoul2, 0, "bone_hinge", &ent->turAngles, BONE_ANGLES_REPLACE, NEGATIVE_Y, NEGATIVE_Z, NEGATIVE_X, NULL, 100, cg.time);
        return;
    } else if (ent->currentState.fireflag && ent->bolt4 != ent->currentState.fireflag) {
        vector3 muzzleOrg, muzzleDir;
        mdxaBone_t boltMatrix;

        trap->G2API_GetBoltMatrix(ent->ghoul2, 0, ent->torsoBolt, &boltMatrix, &vec3_origin, &ent->lerpOrigin, cg.time, cgs.gameModels, &ent->modelScale);
        BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, &muzzleOrg);
        BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, &muzzleDir);

        trap->FX_PlayEffectID(media.efx.turretMuzzleFlash, &muzzleOrg, &muzzleDir, -1, -1, qfalse);

        ent->bolt4 = ent->currentState.fireflag;
    } else if (!ent->currentState.fireflag)
        ent->bolt4 = 0;

    if (ent->currentState.bolt2 != ENTITYNUM_NONE) {
        // turn toward the enemy
        centity_t *enemy = &cg_entities[ent->currentState.bolt2];

        if (enemy) {
            vector3 enAng, enPos;

            VectorCopy(&enemy->currentState.pos.trBase, &enPos);

            VectorSubtract(&enPos, &ent->lerpOrigin, &enAng);
            VectorNormalize(&enAng);
            vectoangles(&enAng, &enAng);
            enAng.roll = 0;
            enAng.pitch += 90;

            CreepToPosition(&enAng, &ent->turAngles);
        }
    } else {
        vector3 idleAng;
        float turnAmount;

        if (ent->turAngles.yaw > 360)
            ent->turAngles.yaw -= 361;

        if (!ent->dustTrailTime)
            ent->dustTrailTime = cg.time;

        turnAmount = (cg.time - ent->dustTrailTime) * 0.03f;

        if (turnAmount > 360)
            turnAmount = 360;

        idleAng.pitch = 90;
        idleAng.roll = 0;
        idleAng.yaw = ent->turAngles.yaw + turnAmount;
        ent->dustTrailTime = cg.time;

        CreepToPosition(&idleAng, &ent->turAngles);
    }

    if (cg.time < ent->frame_minus1_refreshed) {
        ent->frame_minus1_refreshed = cg.time;
        return;
    }

    ent->frame_minus1_refreshed = cg.time;
    trap->G2API_SetBoneAngles(ent->ghoul2, 0, "bone_hinge", &ent->turAngles, BONE_ANGLES_REPLACE, NEGATIVE_Y, NEGATIVE_Z, NEGATIVE_X, NULL, 100, cg.time);
}
