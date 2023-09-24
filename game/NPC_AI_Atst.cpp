#include "b_local.h"

#define MIN_MELEE_RANGE 640
#define MIN_MELEE_RANGE_SQR (MIN_MELEE_RANGE * MIN_MELEE_RANGE)

#define MIN_DISTANCE 128
#define MIN_DISTANCE_SQR (MIN_DISTANCE * MIN_DISTANCE)

#define TURN_OFF 0x00000100 // G2SURFACEFLAG_NODESCENDANTS

#define LEFT_ARM_HEALTH 40
#define RIGHT_ARM_HEALTH 40

void NPC_ATST_Precache(void) {
    G_SoundIndex("sound/chars/atst/atst_damaged1");
    G_SoundIndex("sound/chars/atst/atst_damaged2");

    //	RegisterItem( BG_FindItemForWeapon( WP_ATST_MAIN ));	//precache the weapon
    // rwwFIXMEFIXME: add this weapon
    RegisterItem(BG_FindItemForWeapon(WP_BOWCASTER));       // precache the weapon
    RegisterItem(BG_FindItemForWeapon(WP_ROCKET_LAUNCHER)); // precache the weapon

    G_EffectIndex("env/med_explode2");
    //	G_EffectIndex( "smaller_chunks" );
    G_EffectIndex("blaster/smoke_bolton");
    G_EffectIndex("explosions/droidexplosion1");
}

// Called by NPC's and player in an ATST
void G_ATSTCheckPain(gentity_t *self, gentity_t *other, int damage) {
    // int newBolt;
    // int hitLoc = gPainHitLoc;

    if (rand() & 1) {
        G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged1");
    } else {
        G_SoundOnEnt(self, CHAN_LESS_ATTEN, "sound/chars/atst/atst_damaged2");
    }

    /*
    if ((hitLoc==HL_ARM_LT) && (self->locationDamage[HL_ARM_LT] > LEFT_ARM_HEALTH))
    {
    if (self->locationDamage[hitLoc] >= LEFT_ARM_HEALTH)	// Blow it up?
    {
    newBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*flash3" );
    if ( newBolt != -1 )
    {
    //				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt1, self->s.number);
    ATST_PlayEffect( self, trap->G2API_AddBolt(self->ghoul2, 0, "*flash3"), "env/med_explode2" );
    //G_PlayEffectID( G_EffectIndex("blaster/smoke_bolton"), self->playerModel, newBolt, self->s.number);
    //Maybe bother with this some other time.
    }

    NPC_SetSurfaceOnOff( self, "head_light_blaster_cann", TURN_OFF );
    }
    }
    else if ((hitLoc==HL_ARM_RT) && (self->locationDamage[HL_ARM_RT] > RIGHT_ARM_HEALTH))	// Blow it up?
    {
    if (self->locationDamage[hitLoc] >= RIGHT_ARM_HEALTH)
    {
    newBolt = trap->G2API_AddBolt( self->ghoul2, 0, "*flash4" );
    if ( newBolt != -1 )
    {
    //				G_PlayEffect( "small_chunks", self->playerModel, self->genericBolt2, self->s.number);
    ATST_PlayEffect( self, trap->G2API_AddBolt(self->ghoul2, 0, "*flash4"), "env/med_explode2" );
    //G_PlayEffect( "blaster/smoke_bolton", self->playerModel, newBolt, self->s.number);
    }

    NPC_SetSurfaceOnOff( self, "head_concussion_charger", TURN_OFF );
    }
    }
    */
}

void NPC_ATST_Pain(gentity_t *self, gentity_t *attacker, int damage) {
    G_ATSTCheckPain(self, attacker, damage);
    NPC_Pain(self, attacker, damage);
}

void ATST_Hunt(qboolean visible, qboolean advance) {

    if (NPCInfo->goalEntity == NULL) { // hunt
        NPCInfo->goalEntity = NPC->enemy;
    }

    NPCInfo->combatMove = qtrue;

    NPC_MoveToGoal(qtrue);
}

void ATST_Ranged(qboolean visible, qboolean advance, qboolean altAttack) {

    if (TIMER_Done(NPC, "atkDelay") && visible) // Attack?
    {
        TIMER_Set(NPC, "atkDelay", Q_irand(500, 3000));

        if (altAttack) {
            ucmd.buttons |= BUTTON_ATTACK | BUTTON_ALT_ATTACK;
        } else {
            ucmd.buttons |= BUTTON_ATTACK;
        }
    }

    if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) {
        ATST_Hunt(visible, advance);
    }
}

void ATST_Attack(void) {
    qboolean altAttack = qfalse, visible = qfalse, advance = qfalse;
    int blasterTest, chargerTest;
    float distance;

    if (!NPC_CheckEnemyExt(qfalse)) {
        NPC->enemy = NULL;
        return;
    }

    NPC_FaceEnemy(qtrue);

    // Rate our distance to the target, and our visibilty
    distance = (int)DistanceHorizontalSquared(&NPC->r.currentOrigin, &NPC->enemy->r.currentOrigin);
    visible = NPC_ClearLOS4(NPC->enemy) ? qtrue : qfalse;
    advance = (distance > MIN_DISTANCE_SQR) ? qtrue : qfalse;

    // If we cannot see our target, move to see it
    if (!visible) {
        if (NPCInfo->scriptFlags & SCF_CHASE_ENEMIES) {
            ATST_Hunt(visible, advance);
            return;
        }
    }

    // Decide what type of attack to do
    if (distance > MIN_MELEE_RANGE_SQR) {
        // DIST_LONG
        //		NPC_ChangeWeapon( WP_ATST_SIDE );
        // rwwFIXMEFIXME: make atst weaps work.

        // See if the side weapons are there
        blasterTest = trap->G2API_GetSurfaceRenderStatus(NPC->ghoul2, 0, "head_light_blaster_cann");
        chargerTest = trap->G2API_GetSurfaceRenderStatus(NPC->ghoul2, 0, "head_concussion_charger");

        // It has both side weapons
        if (blasterTest != -1 && !(blasterTest & TURN_OFF) && chargerTest != -1 && !(chargerTest & TURN_OFF))
            altAttack = Q_irand(0, 1) ? qtrue : qfalse;
        else if (blasterTest != -1 && !(blasterTest & TURN_OFF))
            altAttack = qfalse;
        else if (chargerTest != -1 && !(chargerTest & TURN_OFF))
            altAttack = qtrue;
        else
            NPC_ChangeWeapon(WP_NONE);
    } else {
        // DIST_MELEE
        //	NPC_ChangeWeapon( WP_ATST_MAIN );
    }

    NPC_FaceEnemy(qtrue);
    ATST_Ranged(visible, advance, altAttack);
}

void ATST_Patrol(void) {
    if (NPC_CheckPlayerTeamStealth()) {
        NPC_UpdateAngles(qtrue, qtrue);
        return;
    }

    // If we have somewhere to go, then do that
    if (!NPC->enemy) {
        if (UpdateGoal()) {
            ucmd.buttons |= BUTTON_WALKING;
            NPC_MoveToGoal(qtrue);
            NPC_UpdateAngles(qtrue, qtrue);
        }
    }
}

void ATST_Idle(void) {

    NPC_BSIdle();

    NPC_SetAnim(NPC, SETANIM_BOTH, BOTH_STAND1, SETANIM_FLAG_NORMAL);
}

void NPC_BSATST_Default(void) {
    if (NPC->enemy) {
        if ((NPCInfo->scriptFlags & SCF_CHASE_ENEMIES)) {
            NPCInfo->goalEntity = NPC->enemy;
        }
        ATST_Attack();
    } else if (NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES) {
        ATST_Patrol();
    } else {
        ATST_Idle();
    }
}
