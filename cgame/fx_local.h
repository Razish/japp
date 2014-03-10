#pragma once

// NOTENOTE This is not the best, DO NOT CHANGE THESE!
#define	FX_ALPHA_LINEAR		(0x00000001u)
#define	FX_SIZE_LINEAR		(0x00000100u)
#define FX_TAPER			(0x01000000u)
#define FX_BRANCH			(0x02000000u)
#define FX_GROW				(0x04000000u)


// Bryar
void FX_BryarProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BryarAltProjectileThink(  centity_t *cent, const struct weaponInfo_s *weapon );

// Disruptor
void FX_DisruptorMainShot( vector3 *start, vector3 *end );
void FX_DisruptorAltShot( vector3 *start, vector3 *end, qboolean fullCharge );
void FX_DisruptorAltMiss( vector3 *origin, vector3 *normal );
void FX_DisruptorAltHit( vector3 *origin, vector3 *normal );
void FX_DisruptorHitWall( vector3 *origin, vector3 *normal );
void FX_DisruptorHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

// Bowcaster
void FX_BowcasterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_BowcasterHitWall( vector3 *origin, vector3 *normal );
void FX_BowcasterHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

// Heavy Repeater
void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RepeaterHitWall( vector3 *origin, vector3 *normal );
void FX_RepeaterAltHitWall( vector3 *origin, vector3 *normal );
void FX_RepeaterHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_RepeaterAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );

// DEMP2
void FX_DEMP2_ProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_DEMP2_HitWall( vector3 *origin, vector3 *normal );
void FX_DEMP2_HitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_DEMP2_AltDetonate( vector3 *org, float size );

// Golan Arms Flechette
void FX_FlechetteProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_FlechetteWeaponHitWall( vector3 *origin, vector3 *normal );
void FX_FlechetteWeaponHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
void FX_FlechetteAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );

// Personal Rocket Launcher
void FX_RocketProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon );
void FX_RocketHitWall( vector3 *origin, vector3 *normal );
void FX_RocketHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid );
