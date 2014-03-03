#include "cg_local.h"
#include "fx_local.h"
#include "cg_media.h"

static const vector3 WHITE = { 1.0f, 1.0f, 1.0f };

void FX_DisruptorMainShot( vector3 *start, vector3 *end ) {
	trap->FX_AddLine( start, end, 0.1f, 6.0f, 0.0f, 1.0f, 0.0f, 0.0f, &WHITE, &WHITE, 0.0f, 150,
		trap->R_RegisterShaderNoMip( "gfx/effects/redLine" ), FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
}

//q3pro/QtZ code
void CG_RailTrail( clientInfo_t *ci, vector3 *start, vector3 *end ) {
	localEntity_t *leCore = CG_AllocLocalEntity();
	refEntity_t   *reCore = &leCore->refEntity;
	localEntity_t *leGlow = CG_AllocLocalEntity();
	refEntity_t   *reGlow = &leGlow->refEntity;
	vector4 color1 = { ci->rgb1.r/255.0f, ci->rgb1.g/255.0f, ci->rgb1.b/255.0f, 1.0f },
			color2 = { ci->rgb2.r/255.0f, ci->rgb2.g/255.0f, ci->rgb2.b/255.0f, 1.0f };

	if ( cgs.gametype >= GT_TEAM ) {
		if ( ci->team == TEAM_RED ) {
			VectorSet4( &color1, 1.0f, 0.0f, 0.0f, 1.0f );
			VectorSet4( &color2, 1.0f, 0.0f, 0.0f, 1.0f );
		}
		else if ( ci->team == TEAM_BLUE ) {
			VectorSet4( &color1, 0.0f, 0.0f, 1.0f, 1.0f );
			VectorSet4( &color2, 0.0f, 0.0f, 1.0f, 1.0f );
		}
		else {
			VectorSet4( &color1, 0.0f, 0.878431f, 1.0f, 1.0f );
			VectorSet4( &color2, 0.0f, 1.0f, 1.0f, 1.0f );
		}
	}

	//Glow
	leGlow->leType = LE_FADE_RGB;
	leGlow->startTime = cg.time;
	leGlow->endTime = cg.time + 1600;
	leGlow->lifeRate = 1.0f / (leGlow->endTime - leGlow->startTime);
	reGlow->shaderTime = cg.time / 1600.0f;
	reGlow->reType = RT_LINE;
	reGlow->radius = 3.0f;
	reGlow->customShader = trap->R_RegisterShaderNoMip( "gfx/misc/whiteline2" );
	VectorCopy(start, &reGlow->origin);
	VectorCopy(end, &reGlow->oldorigin);
	reGlow->shaderRGBA[0] = color1.r * 255;
	reGlow->shaderRGBA[1] = color1.g * 255;
	reGlow->shaderRGBA[2] = color1.b * 255;
	reGlow->shaderRGBA[3] = 255;
	leGlow->color[0] = color1.r * 0.75f;
	leGlow->color[1] = color1.g * 0.75f;
	leGlow->color[2] = color1.b * 0.75f;
	leGlow->color[3] = 1.0f;

	//Core
	leCore->leType = LE_FADE_RGB;
	leCore->startTime = cg.time;
	leCore->endTime = cg.time + 1600;
	leCore->lifeRate = 1.0f / (leCore->endTime - leCore->startTime);
	reCore->shaderTime = cg.time / 1600.0f;
	reCore->reType = RT_LINE;
	reCore->radius = 1.0f;
	reCore->customShader = trap->R_RegisterShaderNoMip( "gfx/misc/whiteline2" );
	VectorCopy(start, &reCore->origin);
	VectorCopy(end, &reCore->oldorigin);
	reCore->shaderRGBA[0] = color1.r * 255;
	reCore->shaderRGBA[1] = color1.g * 255;
	reCore->shaderRGBA[2] = color1.b * 255;
	reCore->shaderRGBA[3] = 255;
	leCore->color[0] = 1.0f;
	leCore->color[1] = 1.0f;
	leCore->color[2] = 1.0f;
	leCore->color[3] = 0.6f;

	AxisClear( reCore->axis );
}

void FX_DisruptorAltShot( vector3 *start, vector3 *end, qboolean fullCharge ) {
	trap->FX_AddLine( start, end, 0.1f, 10.0f, 0.0f, 1.0f, 0.0f, 0.0f, &WHITE, &WHITE, 0.0f, 175,
		trap->R_RegisterShaderNoMip( "gfx/effects/redLine" ), FX_SIZE_LINEAR | FX_ALPHA_LINEAR );

	if ( fullCharge ) {
		const vector3 YELLER = { 0.8f, 0.7f, 0.0f };

		// add some beef
		trap->FX_AddLine( start, end, 0.1f, 7.0f, 0.0f, 1.0f, 0.0f, 0.0f, &YELLER, &YELLER, 0.0f, 150,
			trap->R_RegisterShaderNoMip( "gfx/misc/whiteline2" ), FX_SIZE_LINEAR | FX_ALPHA_LINEAR );
	}
}

#define FX_ALPHA_WAVE		0x00000008

void FX_DisruptorAltMiss( vector3 *origin, vector3 *normal ) {
	vector3 pos, c1, c2;
	addbezierArgStruct_t b;

	VectorMA( origin, 4.0f, normal, &c1 );
	VectorCopy( &c1, &c2 );
	c1.z += 4;
	c2.z += 12;

	VectorAdd( origin, normal, &pos );
	pos.z += 28;

	VectorCopy( origin, &b.start );
	VectorCopy( &pos, &b.end );
	VectorCopy( &c1, &b.control1 );
	VectorCopy( &vec3_origin, &b.control1Vel );
	VectorCopy( &c2, &b.control2 );
	VectorCopy( &vec3_origin, &b.control2Vel );

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;

	VectorCopy(&WHITE, &b.sRGB);
	VectorCopy(&WHITE, &b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 4000;
	b.shader = trap->R_RegisterShaderNoMip( "gfx/effects/smokeTrail" );
	b.flags = FX_ALPHA_WAVE;

	trap->FX_AddBezier( &b );

	trap->FX_PlayEffectID( media.efx.disruptor.altMiss, origin, normal, -1, -1, qfalse );
}

void FX_DisruptorAltHit( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.disruptor.altHit, origin, normal, -1, -1, qfalse );
}

void FX_DisruptorHitWall( vector3 *origin, vector3 *normal ) {
	trap->FX_PlayEffectID( media.efx.disruptor.wallImpact, origin, normal, -1, -1, qfalse );
}

void FX_DisruptorHitPlayer( vector3 *origin, vector3 *normal, qboolean humanoid ) {
	trap->FX_PlayEffectID( media.efx.disruptor.fleshImpact, origin, normal, -1, -1, qfalse );
}
