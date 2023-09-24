#include "b_local.h"
#include "g_nav.h"

void Remote_Strafe(void);

#define VELOCITY_DECAY 0.85f

// Local state enums
enum {
    LSTATE_NONE = 0,
};

void Remote_Idle(void);

void NPC_Remote_Precache(void) {
    G_SoundIndex("sound/chars/remote/misc/fire.wav");
    G_SoundIndex("sound/chars/remote/misc/hiss.wav");
    G_EffectIndex("env/small_explode");
}

void NPC_Remote_Pain(gentity_t *self, gentity_t *attacker, int damage) {
    SaveNPCGlobals();
    SetNPCGlobals(self);
    Remote_Strafe();
    RestoreNPCGlobals();

    NPC_Pain(self, attacker, damage);
}

void Remote_MaintainHeight(void) {
    float dif;

    // Update our angles regardless
    NPC_UpdateAngles(qtrue, qtrue);

    if (NPC->client->ps.velocity.z > 0.0f) {
        NPC->client->ps.velocity.z *= VELOCITY_DECAY;

        if (fabsf(NPC->client->ps.velocity.z) < 2) {
            NPC->client->ps.velocity.z = 0;
        }
    }
    // If we have an enemy, we should try to hover at or a little below enemy eye level
    if (NPC->enemy) {
        if (TIMER_Done(NPC, "heightChange")) {
            TIMER_Set(NPC, "heightChange", Q_irand(1000, 3000));

            // Find the height difference
            dif = (NPC->enemy->r.currentOrigin.z + Q_irand(0, NPC->enemy->r.maxs.z + 8)) - NPC->r.currentOrigin.z;

            // cap to prevent dramatic height shifts
            if (fabsf(dif) > 2) {
                if (fabsf(dif) > 24) {
                    dif = (dif < 0 ? -24 : 24);
                }
                dif *= 10;
                NPC->client->ps.velocity.z = (NPC->client->ps.velocity.z + dif) / 2;
                //	NPC->fx_time = level.time;
                G_Sound(NPC, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));
            }
        }
    } else {
        gentity_t *goal = NULL;

        if (NPCInfo->goalEntity) // Is there a goal?
        {
            goal = NPCInfo->goalEntity;
        } else {
            goal = NPCInfo->lastGoalEntity;
        }
        if (goal) {
            dif = goal->r.currentOrigin.z - NPC->r.currentOrigin.z;

            if (fabsf(dif) > 24) {
                dif = (dif < 0 ? -24 : 24);
                NPC->client->ps.velocity.z = (NPC->client->ps.velocity.z + dif) / 2;
            }
        }
    }

    // Apply friction
    if (NPC->client->ps.velocity.x > 0.0f) {
        NPC->client->ps.velocity.x *= VELOCITY_DECAY;

        if (fabsf(NPC->client->ps.velocity.x) < 1) {
            NPC->client->ps.velocity.x = 0;
        }
    }

    if (NPC->client->ps.velocity.y > 0.0f) {
        NPC->client->ps.velocity.y *= VELOCITY_DECAY;

        if (fabsf(NPC->client->ps.velocity.y) < 1) {
            NPC->client->ps.velocity.y = 0;
        }
    }
}

#define REMOTE_STRAFE_VEL 256
#define REMOTE_STRAFE_DIS 200
#define REMOTE_UPWARD_PUSH 32

void Remote_Strafe(void) {
    int dir;
    vector3 end, right;
    trace_t tr;

    AngleVectors(&NPC->client->renderInfo.eyeAngles, NULL, &right, NULL);

    // Pick a random strafe direction, then check to see if doing a strafe would be
    //	reasonable valid
    dir = (rand() & 1) ? -1 : 1;
    VectorMA(&NPC->r.currentOrigin, REMOTE_STRAFE_DIS * dir, &right, &end);

    trap->Trace(&tr, &NPC->r.currentOrigin, NULL, NULL, &end, NPC->s.number, MASK_SOLID, qfalse, 0, 0);

    // Close enough
    if (tr.fraction > 0.9f) {
        VectorMA(&NPC->client->ps.velocity, REMOTE_STRAFE_VEL * dir, &right, &NPC->client->ps.velocity);

        G_Sound(NPC, CHAN_AUTO, G_SoundIndex("sound/chars/remote/misc/hiss.wav"));

        // Add a slight upward push
        NPC->client->ps.velocity.z += REMOTE_UPWARD_PUSH;

        // Set the strafe start time so we can do a controlled roll
        //	NPC->fx_time = level.time;
        NPCInfo->standTime = level.time + 3000 + random() * 500;
    }
}

#define REMOTE_FORWARD_BASE_SPEED 10
#define REMOTE_FORWARD_MULTIPLIER 5

void Remote_Hunt(qboolean visible, qboolean advance, qboolean retreat) {
    float distance, speed;
    vector3 forward;

    // If we're not supposed to stand still, pursue the player
    if (NPCInfo->standTime < level.time) {
        // Only strafe when we can see the player
        if (visible) {
            Remote_Strafe();
            return;
        }
    }

    // If we don't want to advance, stop here
    if (advance == qfalse && visible == qtrue)
        return;

    // Only try and navigate if the player is visible
    if (visible == qfalse) {
        // Move towards our goal
        NPCInfo->goalEntity = NPC->enemy;
        NPCInfo->goalRadius = 12;

        // Get our direction from the navigator if we can't see our target
        if (NPC_GetMoveDirection(&forward, &distance) == qfalse)
            return;
    } else {
        VectorSubtract(&NPC->enemy->r.currentOrigin, &NPC->r.currentOrigin, &forward);
        VectorNormalize(&forward);
    }

    speed = REMOTE_FORWARD_BASE_SPEED + REMOTE_FORWARD_MULTIPLIER * g_spSkill.integer;
    if (retreat == qtrue) {
        speed *= -1;
    }
    VectorMA(&NPC->client->ps.velocity, speed, &forward, &NPC->client->ps.velocity);
}

void Remote_Fire(void) {
    vector3 delta1, enemy_org1, muzzle1;
    vector3 angleToEnemy1;
    static vector3 forward, vright, up;
    //	static	vector3	muzzle;
    gentity_t *missile;

    CalcEntitySpot(NPC->enemy, SPOT_HEAD, &enemy_org1);
    VectorCopy(&NPC->r.currentOrigin, &muzzle1);

    VectorSubtract(&enemy_org1, &muzzle1, &delta1);

    vectoangles(&delta1, &angleToEnemy1);
    AngleVectors(&angleToEnemy1, &forward, &vright, &up);

    missile = CreateMissile(&NPC->r.currentOrigin, &forward, 1000, 10000, NPC, qfalse);

    G_PlayEffectID(G_EffectIndex("bryar/muzzle_flash"), &NPC->r.currentOrigin, &forward);

    missile->classname = "briar";
    missile->s.weapon = WP_BRYAR_PISTOL;

    missile->damage = 10;
    missile->dflags = DAMAGE_DEATH_KNOCKBACK;
    missile->methodOfDeath = MOD_BRYAR_PISTOL;
    missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}

void Remote_Ranged(qboolean visible, qboolean advance, qboolean retreat) {

    if (TIMER_Done(NPC, "attackDelay")) // Attack?
    {
        TIMER_Set(NPC, "attackDelay", Q_irand(500, 3000));
        Remote_Fire();
    }

    if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) {
        Remote_Hunt(visible, advance, retreat);
    }
}

#define MIN_MELEE_RANGE 320
#define MIN_MELEE_RANGE_SQR (MIN_MELEE_RANGE * MIN_MELEE_RANGE)

#define MIN_DISTANCE 80
#define MIN_DISTANCE_SQR (MIN_DISTANCE * MIN_DISTANCE)

void Remote_Attack(void) {
    float distance;
    qboolean visible;
    float idealDist;
    qboolean advance;
    qboolean retreat;

    if (TIMER_Done(NPC, "spin")) {
        TIMER_Set(NPC, "spin", Q_irand(250, 1500));
        NPCInfo->desiredYaw += Q_irand(-200, 200);
    }
    // Always keep a good height off the ground
    Remote_MaintainHeight();

    // If we don't have an enemy, just idle
    if (NPC_CheckEnemyExt(qfalse) == qfalse) {
        Remote_Idle();
        return;
    }

    // Rate our distance to the target, and our visibilty
    distance = (int)DistanceHorizontalSquared(&NPC->r.currentOrigin, &NPC->enemy->r.currentOrigin);
    visible = NPC_ClearLOS4(NPC->enemy);
    idealDist = MIN_DISTANCE_SQR + (MIN_DISTANCE_SQR * flrand(0, 1));
    advance = (qboolean)(distance > idealDist * 1.25f);
    retreat = (qboolean)(distance < idealDist * 0.75f);

    // If we cannot see our target, move to see it
    if (visible == qfalse) {
        if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) {
            Remote_Hunt(visible, advance, retreat);
            return;
        }
    }

    Remote_Ranged(visible, advance, retreat);
}

void Remote_Idle(void) {
    Remote_MaintainHeight();

    NPC_BSIdle();
}

void Remote_Patrol(void) {
    Remote_MaintainHeight();

    // If we have somewhere to go, then do that
    if (!NPC->enemy) {
        if (UpdateGoal()) {
            // start loop sound once we move
            ucmd.buttons |= BUTTON_WALKING;
            NPC_MoveToGoal(qtrue);
        }
    }

    NPC_UpdateAngles(qtrue, qtrue);
}

void NPC_BSRemote_Default(void) {
    if (NPC->enemy) {
        Remote_Attack();
    } else if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES) {
        Remote_Patrol();
    } else {
        Remote_Idle();
    }
}
