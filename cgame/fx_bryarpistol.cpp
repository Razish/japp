#include "cg_local.h"
#include "fx_local.h"
#include "cg_media.h"

void FX_BryarProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.pistol.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_BryarHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.pistol.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_BryarHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? media.efx.pistol.fleshImpact : media.efx.pistol.droidImpact, origin, normal, -1, -1, qfalse );
}

void FX_BryarAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;
	int t;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	// see if we have some sort of extra charge going on
	for ( t = 1; t < cent->currentState.generic1; t++ ) {
		// just add ourselves over, and over, and over when we are charged
		trap->FX_PlayEffectID( media.efx.pistol.powerupShot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
	}

	trap->FX_PlayEffectID( media.efx.pistol.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_BryarAltHitWall( vector3 *origin, vector3 *normal, int power ) {
	switch ( power ) {
	case 4:
	case 5:
		trap->FX_PlayEffectID( media.efx.pistol.wallImpact3, origin, normal, -1, -1, qfalse );
		break;

	case 2:
	case 3:
		trap->FX_PlayEffectID( media.efx.pistol.wallImpact2, origin, normal, -1, -1, qfalse );
		break;

	default:
		trap->FX_PlayEffectID( media.efx.pistol.wallImpact, origin, normal, -1, -1, qfalse );
		break;
	}
}

void FX_BryarAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? media.efx.pistol.fleshImpact : media.efx.pistol.droidImpact, origin, normal, -1, -1, qfalse );
}

void FX_TurretProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.turretShot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_TurretHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.pistol.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_TurretHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( humanoid ? media.efx.pistol.fleshImpact : media.efx.pistol.droidImpact, origin, normal, -1, -1, qfalse );
}

void FX_ConcussionHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.concussion.impact, origin, normal, -1, -1, qfalse );
}

void FX_ConcussionHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.concussion.impact, origin, normal, -1, -1, qfalse );
}

void FX_ConcussionProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.concussion.shot, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

static const vector3 WHITE = { 1.0f, 1.0f, 1.0f };
static const vector3 BRIGHT = { 0.75f, 0.5f, 1.0f };

void FX_ConcAltShot( vector3 *start, vector3 *end ) {
	trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f, 1.0f, 0.0f, 0.0f, &WHITE, &WHITE, 0.0f, 175,
		trap->R_RegisterShader( "gfx/effects/blueLine" ), FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	// add some beef
	trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f, 1.0f, 0.0f, 0.0f, &BRIGHT, &BRIGHT, 0.0f, 150,
		trap->R_RegisterShader( "gfx/misc/whiteline2" ), FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}
