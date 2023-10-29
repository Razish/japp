#pragma once

enum weapon_e {
    WP_NONE = 0,
    WP_STUN_BATON,
    WP_MELEE,
    WP_SABER,
    WP_BRYAR_PISTOL,
    WP_BLASTER,
    WP_DISRUPTOR,
    WP_BOWCASTER,
    WP_REPEATER,
    WP_DEMP2,
    WP_FLECHETTE,
    WP_ROCKET_LAUNCHER,
    WP_THERMAL,
    WP_TRIP_MINE,
    WP_DET_PACK,
    WP_CONCUSSION,
    WP_BRYAR_OLD,
    WP_EMPLACED_GUN,
    WP_TURRET,
    WP_NUM_WEAPONS
};
weapon_e BG_FindWeapon(const char *name);

#define FIRST_WEAPON (WP_BRYAR_PISTOL)          // this is the first weapon for next and prev weapon switching
#define LAST_USEABLE_WEAPON (WP_BRYAR_OLD)      // anything > this will be considered not player useable
#define MAX_PLAYER_WEAPONS (WP_NUM_WEAPONS - 1) // this is the max you can switch to and get with the give all.

extern const vector3 WP_MuzzlePoint[WP_NUM_WEAPONS];

enum ammo_e {
    AMMO_NONE = 0,
    AMMO_FORCE,
    AMMO_BLASTER,
    AMMO_POWERCELL,
    AMMO_METAL_BOLTS,
    AMMO_ROCKETS,
    AMMO_EMPLACED,
    AMMO_THERMAL,
    AMMO_TRIPMINE,
    AMMO_DETPACK,
    AMMO_MAX
};

extern const struct weaponData_t {
    const char *longName; // spawning name
    ammo_e ammoIndex;     // index to proper ammo slot
    int ammoLow;          // count when ammo is low
    int shotCost;         // amount of energy used per shot
    int fireTime;         // amount of time between firings
    int charge;
    int chargeMax;
    int chargeTime;
    struct {
        int shotCost;
        int fireTime;
        int charge;
        int chargeMax;
        int chargeTime;
    } alt;
} weaponData[WP_NUM_WEAPONS];

extern const int ammoMax[AMMO_MAX];
