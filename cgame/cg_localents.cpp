// Copyright (C) 1999-2000 Id Software, Inc.
//

// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"
#include "cg_media.h"
#include <deque>

#define	MAX_LOCAL_ENTITIES	2048
std::deque<localEntity_t*> cg_localEntities;

// This is called at startup and for tournament restarts
void CG_InitLocalEntities(void){
	cg_localEntities.clear();
}

void CG_FreeLocalEntity(localEntity_t *le){
	localEntity_t *ent = NULL;
	for (auto it = cg_localEntities.begin(); it != cg_localEntities.end(); it++){
		localEntity_t *ent = *it;
		if (ent->id == le->id){
			cg_localEntities.erase(it);
			free(ent);
		}
	}
}

localEntity_t *CG_AllocLocalEntity(void) {
	localEntity_t *le;

	if (cg_localEntities.size() == MAX_LOCAL_ENTITIES){
		CG_FreeLocalEntity(cg_localEntities[0]);
	}

	le = (localEntity_t*)malloc(sizeof(localEntity_t));
	if (!le){
		trap->Error(ERR_DROP, "CG_AllocLocalEntity: failed to allocate");
		return NULL;
	}
	memset(le, 0, sizeof(localEntity_t));
	
	le->id = cg_localEntities.size() + 1;
	cg_localEntities.push_back(le);
	return le;
}

// A fragment localentity interacts with the environment in some way (hitting walls), or generates more localentities
//	along a trail.

// Leave expanding blood puffs behind gibs
void CG_BloodTrail( localEntity_t *le ) {
	int		t;
	int		t2;
	int		step;
	vector3	newOrigin;
	localEntity_t	*blood;

	step = 150;
	t = step * ((cg.time - cg.frametime + step) / step);
	t2 = step * (cg.time / step);

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, &newOrigin );

		blood = CG_SmokePuff( &newOrigin, &vec3_origin,
			20,		// radius
			1, 1, 1, 1,	// color
			2000,		// trailTime
			t,		// startTime
			0,		// fadeInTime
			0,		// flags
			/*media.gfx.world.bloodTrail*/0 );
		// use the optimized version
		blood->leType = LE_FALL_SCALE_FADE;
		// drop a total of 40 units over its lifetime
		blood->pos.trDelta.z = 40;
	}
}

void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
#if 0
	if ( le->leMarkType == LEMT_BLOOD )
		CG_ImpactMark( media.gfx.world.bloodMark, trace->endpos, trace->plane.normal, random()*360, 1,1,1,1, qtrue, radius, qfalse );
	else if ( le->leMarkType == LEMT_BURN )
		CG_ImpactMark( media.gfx.world.burnMark, trace->endpos, trace->plane.normal, random()*360, 1,1,1,1, qtrue, radius, qfalse );
#endif

	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}

void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace ) {
	// half the fragments will make a bounce sounds
	if ( rand() & 1 ) {
		sfxHandle_t	s = 0;

		switch ( le->leBounceSoundType ) {
		case LEBS_ROCK:
			s = media.sounds.environment.rockBounce[Q_irand( 0, 1 )];
			break;
		case LEBS_METAL:
			s = media.sounds.environment.metalBounce[Q_irand( 0, 1 )];// FIXME: make sure that this sound is registered properly...might still be rock bounce sound....
			break;
		default:
			return;
		}

		if ( s ) {
			trap->S_StartSound( &trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}

		// bouncers only make the sound once...
		// FIXME: arbitrary...change if it bugs you
		le->leBounceSoundType = LEBS_NONE;
	}
	else if ( rand() & 1 ) {
		// we may end up bouncing again, but each bounce reduces the chance of playing the sound again or they may make a lot of noise when they settle
		// FIXME: maybe just always do this??
		le->leBounceSoundType = LEBS_NONE;
	}
}

void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vector3	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, &velocity );
	dot = DotProduct( &velocity, &trace->plane.normal );
	VectorMA( &velocity, -2 * dot, &trace->plane.normal, &le->pos.trDelta );

	VectorScale( &le->pos.trDelta, le->bounceFactor, &le->pos.trDelta );

	VectorCopy( &trace->endpos, &le->pos.trBase );
	le->pos.trTime = cg.time;

	// check for stop, making sure that even on low FPS systems it doesn't bobble
	if ( trace->allsolid ||
		(trace->plane.normal.z > 0 &&
		(le->pos.trDelta.z < 40 || le->pos.trDelta.z < -cg.frametime * le->pos.trDelta.z)) ) {
		le->pos.trType = TR_STATIONARY;
	}
	else {

	}
}

void CG_AddFragment( localEntity_t *le ) {
	vector3	newOrigin;
	trace_t	trace;

	if ( le->forceAlpha ) {
		le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
		le->refEntity.shaderRGBA[3] = le->forceAlpha;
	}

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int		t;
		float	t_e;

		t = le->endTime - cg.time;
		if ( t < (SINK_TIME * 2) ) {
			le->refEntity.renderfx |= RF_FORCE_ENT_ALPHA;
			t_e = (float)((float)(le->endTime - cg.time) / (SINK_TIME * 2));
			t_e = (int)((t_e)* 255);

			if ( t_e > 255 ) {
				t_e = 255;
			}
			if ( t_e < 1 ) {
				t_e = 1;
			}

			if ( le->refEntity.shaderRGBA[3] && t_e > le->refEntity.shaderRGBA[3] ) {
				t_e = le->refEntity.shaderRGBA[3];
			}

			le->refEntity.shaderRGBA[3] = t_e;

			SE_R_AddRefEntityToScene( &le->refEntity, MAX_CLIENTS );
		}
		else {
			SE_R_AddRefEntityToScene( &le->refEntity, MAX_CLIENTS );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, &newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, &le->refEntity.origin, NULL, NULL, &newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0f ) {
		// still in free fall
		VectorCopy( &newOrigin, &le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vector3 angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, &angles );
			AnglesToAxis( &angles, le->refEntity.axis );
			ScaleModelAxis( &le->refEntity );
		}

		SE_R_AddRefEntityToScene( &le->refEntity, MAX_CLIENTS );

		// add a blood trail
		if ( le->leBounceSoundType == LEBS_BLOOD ) {
			CG_BloodTrail( le );
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( trap->CM_PointContents( &trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	if ( !trace.startsolid ) {
		// leave a mark
		CG_FragmentBounceMark( le, &trace );

		// do a bouncy sound
		CG_FragmentBounceSound( le, &trace );

		if ( le->bounceSound ) { //specified bounce sound (debris)
			trap->S_StartSound( &le->pos.trBase, ENTITYNUM_WORLD, CHAN_AUTO, le->bounceSound );
		}

		// reflect the velocity on the trace plane
		CG_ReflectVelocity( le, &trace );

		SE_R_AddRefEntityToScene( &le->refEntity, MAX_CLIENTS );
	}
}

// TRIVIAL LOCAL ENTITIES
// These only do simple scaling or modulation before passing to the renderer

void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = (le->endTime - cg.time) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

static void CG_AddFadeScaleModel( localEntity_t *le ) {
	refEntity_t	*ent = &le->refEntity;

	float frac = (cg.time - le->startTime) / ((float)(le->endTime - le->startTime));

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end

	ent->nonNormalizedAxes = qtrue;

	AxisCopy( axisDefault, ent->axis );

	VectorScale( &ent->axis[0], le->radius*frac, &ent->axis[0] );
	VectorScale( &ent->axis[1], le->radius*frac, &ent->axis[1] );
	VectorScale( &ent->axis[2], le->radius*0.5f*frac, &ent->axis[2] );

	frac = 1.0f - frac;

	ent->shaderRGBA[0] = le->color[0] * frac;
	ent->shaderRGBA[1] = le->color[1] * frac;
	ent->shaderRGBA[2] = le->color[2] * frac;
	ent->shaderRGBA[3] = le->color[3] * frac;

	// add the entity
	SE_R_AddRefEntityToScene( ent, MAX_CLIENTS );
}

static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vector3		delta;
	float		len;
	refdef_t *refdef = CG_GetRefdef();

	re = &le->refEntity;

	if ( le->fadeInTime > le->startTime && cg.time < le->fadeInTime ) {
		// fade / grow time
		c = 1.0f - (float)(le->fadeInTime - cg.time) / (le->fadeInTime - le->startTime);
	}
	else {
		// fade / grow time
		c = (le->endTime - cg.time) * le->lifeRate;
	}

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	if ( !(le->leFlags & LEF_PUFF_DONT_SCALE) ) {
		re->radius = le->radius * (1.0f - c) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, &re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( &re->origin, &refdef->vieworg, &delta );
	len = VectorLength( &delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

static void CG_AddPuff( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vector3		delta;
	float		len;
	refdef_t *refdef = CG_GetRefdef();

	re = &le->refEntity;

	// fade / grow time
	c = (le->endTime - cg.time) / (float)(le->endTime - le->startTime);

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;

	if ( !(le->leFlags & LEF_PUFF_DONT_SCALE) ) {
		re->radius = le->radius * (1.0f - c) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, &re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( &re->origin, &refdef->vieworg, &delta );
	len = VectorLength( &delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

// For rocket smokes that hang in place, fade out, and are removed if the view passes through them.
//	There are often many of these, so it needs to be simple.
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vector3		delta;
	float		len;
	refdef_t *refdef = CG_GetRefdef();

	re = &le->refEntity;

	// fade / grow time
	c = (le->endTime - cg.time) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	re->radius = le->radius * (1.0f - c) + 8;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( &re->origin, &refdef->vieworg, &delta );
	len = VectorLength( &delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}


// This is just an optimized CG_AddMoveScaleFade for blood mists that drift down, fade out, and are emoved if the view
//	passes through them.
// There are often 100+ of these, so it needs to be simple.
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t	*re;
	float		c;
	vector3		delta;
	float		len;
	refdef_t *refdef = CG_GetRefdef();

	re = &le->refEntity;

	// fade time
	c = (le->endTime - cg.time) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin.z = le->pos.trBase.z - (1.0f - c) * le->pos.trDelta.z;

	re->radius = le->radius * (1.0f - c) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( &re->origin, &refdef->vieworg, &delta );
	len = VectorLength( &delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t	*ent;

	ent = &ex->refEntity;

	// add the entity
	SE_R_AddRefEntityToScene( ent, MAX_CLIENTS );

	// add the dlight
	if ( ex->light ) {
		float		light;

		light = (float)(cg.time - ex->startTime) / (ex->endTime - ex->startTime);
		if ( light < 0.5f ) {
			light = 1.0f;
		}
		else {
			light = 1.0f - (light - 0.5f) * 2;
		}
		light = ex->light * light;
		trap->R_AddLightToScene( &ent->origin, light, ex->lightColor.r, ex->lightColor.g, ex->lightColor.b );
	}
}

static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t	re;
	float c;

	re = le->refEntity;

	c = (le->endTime - cg.time) / (float)(le->endTime - le->startTime);
	if ( c > 1 ) {
		c = 1.0f;	// can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33f;

	re.reType = RT_SPRITE;
	re.radius = 42 * (1.0f - c) + 30;

	SE_R_AddRefEntityToScene( &re, MAX_CLIENTS );

	// add the dlight
	if ( le->light ) {
		float		light;

		light = (float)(cg.time - le->startTime) / (le->endTime - le->startTime);
		if ( light < 0.5f ) {
			light = 1.0f;
		}
		else {
			light = 1.0f - (light - 0.5f) * 2;
		}
		light = le->light * light;
		trap->R_AddLightToScene( &re.origin, light, le->lightColor.r, le->lightColor.g, le->lightColor.b );
	}
}

void CG_AddRefEntity( localEntity_t *le ) {
	if ( le->endTime < cg.time ) {
		CG_FreeLocalEntity( le );
		return;
	}
	SE_R_AddRefEntityToScene( &le->refEntity, MAX_CLIENTS );
}

#define NUMBER_SIZE		8

void CG_AddScorePlum( localEntity_t *le ) {
	refEntity_t	*re;
	vector3		origin, delta, dir, vec, up = { 0, 0, 1 };
	float		c, len;
	int			i, score, digits[10], numdigits, negative;
	refdef_t *refdef = CG_GetRefdef();

	re = &le->refEntity;

	c = (le->endTime - cg.time) * le->lifeRate;

	score = le->radius;
	if ( score < 0 ) {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0x11;
		re->shaderRGBA[2] = 0x11;
	}
	else {
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		if ( score >= 50 ) {
			re->shaderRGBA[1] = 0;
		}
		else if ( score >= 20 ) {
			re->shaderRGBA[0] = re->shaderRGBA[1] = 0;
		}
		else if ( score >= 10 ) {
			re->shaderRGBA[2] = 0;
		}
		else if ( score >= 2 ) {
			re->shaderRGBA[0] = re->shaderRGBA[2] = 0;
		}

	}
	if ( c < 0.25f )
		re->shaderRGBA[3] = 0xff * 4 * c;
	else
		re->shaderRGBA[3] = 0xff;

	re->radius = NUMBER_SIZE / 2;

	VectorCopy( &le->pos.trBase, &origin );
	origin.z += 110 - c * 100;

	VectorSubtract( &refdef->vieworg, &origin, &dir );
	CrossProduct( &dir, &up, &vec );
	VectorNormalize( &vec );

	VectorMA( &origin, -10 + 20 * sinf( c * 2 * M_PI ), &vec, &origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( &origin, &refdef->vieworg, &delta );
	len = VectorLength( &delta );
	if ( len < 20 ) {
		CG_FreeLocalEntity( le );
		return;
	}

	negative = qfalse;
	if ( score < 0 ) {
		negative = qtrue;
		score = -score;
	}

	for ( numdigits = 0; !(numdigits && !score); numdigits++ ) {
		digits[numdigits] = score % 10;
		score = score / 10;
	}

	if ( negative ) {
		digits[numdigits] = 10;
		numdigits++;
	}

	for ( i = 0; i < numdigits; i++ ) {
		VectorMA( &origin, (float)(((float)numdigits / 2) - i) * NUMBER_SIZE, &vec, &re->origin );
		re->customShader = media.gfx.interface.numbers[digits[numdigits - 1 - i]];
		SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
	}
}

// For forcefields/other rectangular things
void CG_AddOLine( localEntity_t *le ) {
	refEntity_t	*re;
	float		frac, alpha;

	re = &le->refEntity;

	frac = (cg.time - le->startTime) / (float)(le->endTime - le->startTime);
	if ( frac > 1 )
		frac = 1.0f;	// can happen during connection problems
	else if ( frac < 0 )
		frac = 0.0f;

	// Use the liferate to set the scale over time.
	re->data.line.width = le->data.line.width + (le->data.line.dwidth * frac);
	if ( re->data.line.width <= 0 ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// We will assume here that we want additive transparency effects.
	alpha = le->alpha + (le->dalpha * frac);
	re->shaderRGBA[0] = 0xff * alpha;
	re->shaderRGBA[1] = 0xff * alpha;
	re->shaderRGBA[2] = 0xff * alpha;
	re->shaderRGBA[3] = 0xff * alpha;	// Yes, we could apply c to this too, but fading the color is better for lines.

	re->shaderTexCoord.x = 1;
	re->shaderTexCoord.y = 1;

	re->rotation = 90;

	re->reType = RT_ORIENTEDLINE;

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

// for beams and the like.
void CG_AddLine( localEntity_t *le ) {
	refEntity_t	*re;

	re = &le->refEntity;

	re->reType = RT_LINE;

	SE_R_AddRefEntityToScene( re, MAX_CLIENTS );
}

void CG_AddLocalEntities( void ) {
	localEntity_t	*le;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	for (auto it = cg_localEntities.begin(); it != cg_localEntities.end(); it++){
		le = *it;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FADE_SCALE_MODEL:
			CG_AddFadeScaleModel( le );
			break;

		case LE_FRAGMENT:			// gibs and brass
			CG_AddFragment( le );
			break;

		case LE_PUFF:
			CG_AddPuff( le );
			break;

		case LE_MOVE_SCALE_FADE:		// water bubbles
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:				// teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE: // gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:		// rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_SCOREPLUM:
			CG_AddScorePlum( le );
			break;

		case LE_OLINE:
			CG_AddOLine( le );
			break;

		case LE_SHOWREFENTITY:
			CG_AddRefEntity( le );
			break;

		case LE_LINE:					// oriented lines for FX
			CG_AddLine( le );
			break;
		}
	}
}
