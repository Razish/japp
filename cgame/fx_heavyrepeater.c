#include "cg_local.h"
#include "cg_media.h"

void FX_RepeaterProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	trap->FX_PlayEffectID( media.efx.repeater.projectile, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_RepeaterHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.repeater.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_RepeaterHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.repeater.fleshImpact, origin, normal, -1, -1, qfalse );
}

void FX_Mortar_Missile( centity_t *cent, const struct weaponInfo_s *weapon ) {
	refEntity_t ent;
	vector3 ang;
	float scale = 1.0f;
	refdef_t *refdef = CG_GetRefdef();

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( &cent->lerpOrigin, &ent.origin );

	VectorSubtract( &ent.origin, &refdef->vieworg, &ent.axis[0] );
	if ( VectorNormalize( &ent.axis[0] ) <= 0.1f )
		return;

//	VectorCopy(refdef->viewaxis[2], ent.axis[2]);
//	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);
	vectoangles( &ent.axis[0], &ang );
	ang.roll = cent->trickAlpha;
	cent->trickAlpha += 12; //spin the half-sphere to give a "screwdriver" effect
	AnglesToAxis( &ang, ent.axis );

	//radius must be a power of 2, and is the actual captured texture size
	scale = 0.37f;
	VectorScale( &ent.axis[0],  scale, &ent.axis[0] );
	VectorScale( &ent.axis[1],  scale, &ent.axis[1] );
	VectorScale( &ent.axis[2], -scale, &ent.axis[2] );


	ent.hModel = media.models.halfShield;

	ent.shaderRGBA[0] = 128;	ent.shaderRGBA[1] = 43;		ent.shaderRGBA[2] = 255;	ent.shaderRGBA[3] = 48;
	ent.customShader = trap->R_RegisterShaderNoMip( "gfx/effects/shock_ripple" );
	ent.renderfx = (RF_RGB_TINT|RF_FORCE_ENT_ALPHA);
	SE_R_AddRefEntityToScene( &ent, cent->currentState.number );

	ent.shaderRGBA[0] = 255;	ent.shaderRGBA[1] = 255;	ent.shaderRGBA[2] = 255;	ent.shaderRGBA[3] = 255;
	ent.customShader	= trap->R_RegisterShaderNoMip( "gfx/effects/caustic1" );
	ent.renderfx		= (RF_RGB_TINT|RF_MINLIGHT);
	SE_R_AddRefEntityToScene( &ent, cent->currentState.number );

	scale = (1.15f + Q_fabs( sinf( cg.time / 80.0f ) )*0.65f);
	VectorScale( &ent.axis[0], scale, &ent.axis[0] );
	VectorScale( &ent.axis[1], scale, &ent.axis[1] );
	VectorScale( &ent.axis[2], scale, &ent.axis[2] );
	ent.shaderRGBA[0] = 51;		ent.shaderRGBA[1] = 119;	ent.shaderRGBA[2] = 255;	ent.shaderRGBA[3] = 255;
	ent.customShader = trap->R_RegisterShaderNoMip( "gfx/effects/eplosion_wave" );
	ent.renderfx = (RF_RGB_TINT|RF_ALPHA_DEPTH);
	SE_R_AddRefEntityToScene( &ent, cent->currentState.number );

	trap->R_AddLightToScene( &cent->lerpOrigin, 400, 0.13f, 0.43f, 0.87f );
}

static void CG_DistortionOrb( centity_t *cent ) {
	refEntity_t ent;
	vector3 ang;
	float scale = 0.5f;
	float vLen;
	refdef_t *refdef = CG_GetRefdef();

	if ( !cg_renderToTextureFX.integer )
		return;

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( &cent->lerpOrigin, &ent.origin );

	VectorSubtract( &ent.origin, &refdef->vieworg, &ent.axis[0] );
	vLen = VectorLength( &ent.axis[0] );

	// Entity is right on vieworg.  quit.
	if ( VectorNormalize( &ent.axis[0] ) <= 0.1f )
		return;

//	VectorCopy( refdef->viewaxis[2], ent.axis[2] );
//	CrossProduct( ent.axis[0], ent.axis[2], ent.axis[1] );
	vectoangles( &ent.axis[0], &ang );
	ang.roll = cent->trickAlpha;
	cent->trickAlpha += 16; //spin the half-sphere to give a "screwdriver" effect
	AnglesToAxis( &ang, ent.axis );

	//radius must be a power of 2, and is the actual captured texture size
		 if ( vLen < 128 )	ent.radius = 256;
	else if ( vLen < 256 )	ent.radius = 128;
	else if ( vLen < 512 )	ent.radius = 64;
	else					ent.radius = 32;

	VectorScale( &ent.axis[0],  scale, &ent.axis[0] );
	VectorScale( &ent.axis[1],  scale, &ent.axis[1] );
	VectorScale( &ent.axis[2], -scale, &ent.axis[2] );

	ent.hModel = media.models.halfShield;
	ent.customShader = 0;//media.gfx.world.halfShield;

#if 1
	ent.renderfx = (RF_DISTORTION|RF_RGB_TINT);

	//tint the whole thing a shade of blue
	ent.shaderRGBA[0] = 200.0f;
	ent.shaderRGBA[1] = 200.0f;
	ent.shaderRGBA[2] = 255.0f;
#else //no tint
	ent.renderfx = RF_DISTORTION;
#endif

	SE_R_AddRefEntityToScene( &ent, cent->currentState.number );
}

void FX_RepeaterAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon ) {
	vector3 forward;

	if ( VectorNormalize2( &cent->currentState.pos.trDelta, &forward ) == 0.0f )
		forward.z = 1.0f;

	if ( (cg_newFX.integer & NEWFX_REPEATER_ALT) ) {
		FX_Mortar_Missile( cent, weapon );
		return;
	}

	if ( cg_repeaterOrb.integer )
		CG_DistortionOrb( cent );

	trap->FX_PlayEffectID( media.efx.repeater.altProjectile, &cent->lerpOrigin, &forward, -1, -1, qfalse );
}

void FX_RepeaterAltHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.repeater.altWallImpact, origin, normal, -1, -1, qfalse );
}

void FX_RepeaterAltHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.repeater.altWallImpact, origin, normal, -1, -1, qfalse );
}
