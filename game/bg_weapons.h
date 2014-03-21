#pragma once

// Filename:-	bg_weapons.h
//
// This crosses both client and server.  It could all be crammed into bg_public, but isolation of this type of data is best.

typedef enum weapon_e {
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
} weapon_t;

#define FIRST_WEAPON		(WP_BRYAR_PISTOL) // this is the first weapon for next and prev weapon switching
#define LAST_USEABLE_WEAPON (WP_BRYAR_OLD) // anything > this will be considered not player useable
#define MAX_PLAYER_WEAPONS	(WP_NUM_WEAPONS-1) // this is the max you can switch to and get with the give all.

typedef enum ammo_e {
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
} ammo_t;

typedef struct weaponData_s {
	char	longName[32];		// Spawning name

	int		ammoIndex;			// Index to proper ammo slot
	int		ammoLow;			// Count when ammo is low

	int		energyPerShot;		// Amount of energy used per shot
	int		fireTime;			// Amount of time between firings
	int		range;				// Range of weapon

	int		altEnergyPerShot;	// Amount of energy used for alt-fire
	int		altFireTime;		// Amount of time between alt-firings
	int		altRange;			// Range of alt-fire

	int		chargeSubTime;		// ms interval for subtracting ammo during charge
	int		altChargeSubTime;	// above for secondary

	int		chargeSub;			// amount to subtract during charge on each interval
	int		altChargeSub;		// above for secondary

	int		maxCharge;			// stop subtracting once charged for this many ms
	int		altMaxCharge;		// above for secondary
} weaponData_t;
extern weaponData_t weaponData[WP_NUM_WEAPONS];

typedef struct ammoData_s {
	int		max;				// Max amount player can hold of ammo
} ammoData_t;
extern ammoData_t ammoData[AMMO_MAX];
