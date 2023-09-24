// Copyright (C) 2001-2002 Raven Software
//
// bg_weapons.c -- part of bg_pmove functionality

#include "qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

// Muzzle point table...
const vector3 WP_MuzzlePoint[WP_NUM_WEAPONS] = {
    // forward, right, up
    {0.0f, 0.0f, 0.0f},    // WP_NONE,
    {0.0f, 8.0f, 0.0f},    // WP_STUN_BATON,
    {0.0f, 8.0f, 0.0f},    // WP_MELEE,
    {8.0f, 16.0f, 0.0f},   // WP_SABER,
    {12.0f, 6.0f, -6.0f},  // WP_BRYAR_PISTOL,
    {12.0f, 6.0f, -6.0f},  // WP_BLASTER,
    {12.0f, 6.0f, -6.0f},  // WP_DISRUPTOR,
    {12.0f, 2.0f, -6.0f},  // WP_BOWCASTER,
    {12.0f, 4.5f, -6.0f},  // WP_REPEATER,
    {12.0f, 6.0f, -6.0f},  // WP_DEMP2,
    {12.0f, 6.0f, -6.0f},  // WP_FLECHETTE,
    {12.0f, 8.0f, -4.0f},  // WP_ROCKET_LAUNCHER,
    {12.0f, 0.0f, -4.0f},  // WP_THERMAL,
    {12.0f, 0.0f, -10.0f}, // WP_TRIP_MINE,
    {12.0f, 0.0f, -4.0f},  // WP_DET_PACK,
    {12.0f, 6.0f, -6.0f},  // WP_CONCUSSION
    {12.0f, 6.0f, -6.0f},  // WP_BRYAR_OLD,
};

const weaponData_t weaponData[WP_NUM_WEAPONS] = {
    // longname						ammoIndex			ammoLow	shotCost	fireTime	charge	chargeMax	chargeTime	alt.ShotCost	alt.fireTime	alt.charge	alt.chargeMax
    // alt.chargeTime
    {"No Weapon", AMMO_NONE, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0}},
    {"Stun Baton", AMMO_NONE, 5, 0, 400, 0, 0, 0, {0, 400, 0, 0, 0}},
    {"Melee", AMMO_NONE, 5, 0, 400, 0, 0, 0, {0, 400, 0, 0, 0}},
    {"Lightsaber", AMMO_NONE, 5, 0, 100, 0, 0, 0, {0, 100, 0, 0, 0}},
    {"Bryar Pistol", AMMO_BLASTER, 0, 0, 800, 0, 0, 0, {0, 800, 0, 0, 0}},
    {"E11 Blaster Rifle", AMMO_BLASTER, 5, 2, 350, 0, 0, 0, {3, 150, 0, 0, 0}},
    {"Tenloss Disruptor Rifle", AMMO_POWERCELL, 5, 5, 600, 0, 0, 0, {6, 1300, 3, 1700, 200}},
    {"Wookiee Bowcaster", AMMO_POWERCELL, 5, 5, 1000, 5, 1700, 400, {5, 750, 0, 0, 0}},
    {"Imperial Heavy Repeater", AMMO_METAL_BOLTS, 5, 1, 100, 0, 0, 0, {15, 800, 0, 0, 0}},
    {"DEMP2", AMMO_POWERCELL, 5, 8, 500, 0, 0, 0, {6, 900, 3, 2100, 250}},
    {"Golan Arms Flechette", AMMO_METAL_BOLTS, 5, 10, 700, 0, 0, 0, {15, 800, 0, 0, 0}},
    {"Merr-Sonn Missile System", AMMO_ROCKETS, 5, 1, 900, 0, 0, 0, {2, 1200, 0, 0, 0}},
    {"Thermal Detonator", AMMO_THERMAL, 0, 1, 800, 0, 0, 0, {1, 400, 0, 0, 0}},
    {"Trip Mine", AMMO_TRIPMINE, 0, 1, 800, 0, 0, 0, {1, 400, 0, 0, 0}},
    {"Det Pack", AMMO_DETPACK, 0, 1, 800, 0, 0, 0, {0, 400, 0, 0, 0}},
    {"Concussion Rifle", AMMO_METAL_BOLTS, 40, 40, 800, 0, 0, 0, {50, 1200, 0, 0, 0}},
    {"Bryar Pistol", AMMO_BLASTER, 15, 2, 400, 0, 0, 0, {2, 400, 1, 1500, 200}},
    {"Emplaced Gun", AMMO_NONE, 0, 0, 100, 0, 0, 0, {0, 100, 0, 0, 0}},
    {"Turret", AMMO_NONE, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0}},
};

const int ammoMax[AMMO_MAX] = {
    0,   // AMMO_NONE
    100, // AMMO_FORCE
    300, // AMMO_BLASTER
    300, // AMMO_POWERCELL
    300, // AMMO_METAL_BOLTS
    25,  // AMMO_ROCKETS
    800, // AMMO_EMPLACED
    10,  // AMMO_THERMAL
    10,  // AMMO_TRIPMINE
    10   // AMMO_DETPACK
};

weapon_t BG_FindWeapon(const char *name) {
    qboolean numeric = qtrue;
    const char *p = NULL;
    int i;
    const weaponData_t *wd = NULL;

    for (p = name; *p; p++) {
        if (Q_isalpha(*p)) {
            numeric = qfalse;
            break;
        }
    }

    Com_Printf("BG_FindWeapon( \"%s\" ), numeric = %s\n", name, numeric ? "true" : "false");

    if (numeric) {
        weapon_t wp = (weapon_t)atoi(name);
        if (wp <= WP_NONE || wp > LAST_USEABLE_WEAPON) {
            return WP_NONE;
        }
        return wp;
    }

    for (i = 0, wd = weaponData; i < WP_NUM_WEAPONS; i++, wd++) {
        if (!Q_stricmp(name, wd->longName)) {
            return (weapon_t)i;
        }
    }

    return WP_NONE;
}
