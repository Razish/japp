//
/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

#include "ui_local.h"
#include "ui_shared.h"

void WP_SaberLoadParms( void );
qboolean WP_SaberParseParm( const char *saberName, const char *parmname, char *saberData );
saber_colors_t TranslateSaberColor( const char *name );
const char *SaberColorToString( saber_colors_t color );
saber_styles_t TranslateSaberStyle( const char *name );
saberType_t TranslateSaberType( const char *name );

qboolean ui_saber_parms_parsed = qfalse;

static qhandle_t
redSaberGlowShader, redSaberCoreShader,
orangeSaberGlowShader, orangeSaberCoreShader,
yellowSaberGlowShader, yellowSaberCoreShader,
greenSaberGlowShader, greenSaberCoreShader,
blueSaberGlowShader, blueSaberCoreShader,
purpleSaberGlowShader, purpleSaberCoreShader,
rgbSaberGlowShader, rgbSaberCoreShader,
rgbSaberGlow2Shader, rgbSaberCore2Shader, rgbSaberTrail2Shader,
rgbSaberGlow3Shader, rgbSaberCore3Shader, rgbSaberTrail3Shader,
rgbSaberGlow4Shader, rgbSaberCore4Shader, rgbSaberTrail4Shader,
rgbSaberGlow5Shader, rgbSaberCore5Shader, rgbSaberTrail5Shader,
blackSaberGlowShader, blackSaberCoreShader, blackBlurShader,
sfxSaberTrailShader, sfxSaberBladeShader, sfxSaberBlade2Shader, sfxSaberEndShader, sfxSaberEnd2Shader;

void UI_CacheSaberGlowGraphics( void ) {//FIXME: these get fucked by vid_restarts
	redSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_glow" );
	redSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_line" );
	orangeSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_glow" );
	orangeSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_line" );
	yellowSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_glow" );
	yellowSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_line" );
	greenSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_glow" );
	greenSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_line" );
	blueSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_glow" );
	blueSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_line" );
	purpleSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_glow" );
	purpleSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_line" );
	rgbSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow1" );
	rgbSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore1" );

	//Flame 1
	rgbSaberGlow2Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow2" );
	rgbSaberCore2Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore2" );
	rgbSaberTrail2Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail2" );

	//Electric 1
	rgbSaberGlow3Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow3" );
	rgbSaberCore3Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore3" );
	rgbSaberTrail3Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail3" );

	//Flame 2
	rgbSaberGlow4Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow4" );
	rgbSaberCore4Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore4" );
	rgbSaberTrail4Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail4" );

	//Electric 2
	rgbSaberGlow5Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow5" );
	rgbSaberCore5Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore5" );
	rgbSaberTrail5Shader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail5" );

	//Black
	blackSaberGlowShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackglow" );
	blackSaberCoreShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackcore" );
	blackBlurShader = trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blacktrail" );

	sfxSaberTrailShader = trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_trail" );
	sfxSaberBladeShader = trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade" );
	sfxSaberBlade2Shader = trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade_rgb" );
	sfxSaberEndShader = trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end" );
	sfxSaberEnd2Shader = trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end_rgb" );
}

qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel ) {
	return WP_SaberParseParm( saberName, "saberModel", saberModel );
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin ) {
	return WP_SaberParseParm( saberName, "customSkin", saberSkin );
}

qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType ) {
	return WP_SaberParseParm( saberName, "saberType", saberType );
}

int UI_SaberNumBladesForSaber( const char *saberName ) {
	int numBlades;
	char	numBladesString[8] = { 0 };
	WP_SaberParseParm( saberName, "numBlades", numBladesString );
	numBlades = atoi( numBladesString );
	if ( numBlades < 1 ) {
		numBlades = 1;
	}
	else if ( numBlades > 8 ) {
		numBlades = 8;
	}
	return numBlades;
}

qboolean UI_SaberShouldDrawBlade( const char *saberName, int bladeNum ) {
	int bladeStyle2Start = 0, noBlade = 0;
	char	bladeStyle2StartString[8] = { 0 };
	char	noBladeString[8] = { 0 };
	WP_SaberParseParm( saberName, "bladeStyle2Start", bladeStyle2StartString );
	if ( bladeStyle2StartString[0] ) {
		bladeStyle2Start = atoi( bladeStyle2StartString );
	}
	if ( bladeStyle2Start
		&& bladeNum >= bladeStyle2Start ) {//use second blade style
		WP_SaberParseParm( saberName, "noBlade2", noBladeString );
		if ( noBladeString[0] ) {
			noBlade = atoi( noBladeString );
		}
	}
	else {//use first blade style
		WP_SaberParseParm( saberName, "noBlade", noBladeString );
		if ( noBladeString[0] ) {
			noBlade = atoi( noBladeString );
		}
	}
	return ((qboolean)(noBlade == 0));
}


qboolean UI_IsSaberTwoHanded( const char *saberName ) {
	int twoHanded;
	char	twoHandedString[8] = { 0 };
	WP_SaberParseParm( saberName, "twoHanded", twoHandedString );
	if ( !twoHandedString[0] ) {//not defined defaults to "no"
		return qfalse;
	}
	twoHanded = atoi( twoHandedString );
	return ((qboolean)(twoHanded != 0));
}

float UI_SaberBladeLengthForSaber( const char *saberName, int bladeNum ) {
	char	lengthString[8] = { 0 };
	float	length = 40.0f;
	WP_SaberParseParm( saberName, "saberLength", lengthString );
	if ( lengthString[0] ) {
		length = atof( lengthString );
		if ( length < 0.0f ) {
			length = 0.0f;
		}
	}

	WP_SaberParseParm( saberName, va( "saberLength%d", bladeNum + 1 ), lengthString );
	if ( lengthString[0] ) {
		length = atof( lengthString );
		if ( length < 0.0f ) {
			length = 0.0f;
		}
	}

	return length;
}

float UI_SaberBladeRadiusForSaber( const char *saberName, int bladeNum ) {
	char	radiusString[8] = { 0 };
	float	radius = 3.0f;
	WP_SaberParseParm( saberName, "saberRadius", radiusString );
	if ( radiusString[0] ) {
		radius = atof( radiusString );
		if ( radius < 0.0f ) {
			radius = 0.0f;
		}
	}

	WP_SaberParseParm( saberName, va( "saberRadius%d", bladeNum + 1 ), radiusString );
	if ( radiusString[0] ) {
		radius = atof( radiusString );
		if ( radius < 0.0f ) {
			radius = 0.0f;
		}
	}

	return radius;
}

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName, int destsize ) {
	char stringedSaberName[1024] = { 0 };
	qboolean ret = WP_SaberParseParm( saberName, "name", stringedSaberName );

	// if it's a stringed reference translate it
	if ( ret && stringedSaberName[0] == '@' )
		trap->SE_GetStringTextString( &stringedSaberName[1], saberProperName, destsize );
	else if ( !stringedSaberName[0] )
		Q_strncpyz( saberProperName, va( S_COLOR_RED"<%s"S_COLOR_RED">", saberName ), destsize );
	else // no stringed so just use it as it
		Q_strncpyz( saberProperName, stringedSaberName, destsize );

	return ret;
}

qboolean UI_SaberValidForPlayerInMP( const char *saberName ) {
	char allowed[8] = { 0 };
	if ( !WP_SaberParseParm( saberName, "notInMP", allowed ) ) {//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] ) {//not defined, default is yes
		return qtrue;
	}
	else {//return value
#ifndef _GAME
		return qtrue;
#else
		return ((qboolean)(atoi(allowed)==0));
#endif
	}
}

void UI_SaberLoadParms( void ) {
	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

	WP_SaberLoadParms();
}

void RGB_LerpColor( vector3 *from, vector3 *to, float frac, vector3 *out ) {
	vector3 diff;
	int i;

	VectorSubtract( to, from, &diff );

	VectorCopy( from, out );

	for ( i = 0; i < 3; i++ )
		out->data[i] += diff.data[i] * frac;

}

int getint( char **buf ) {
	double temp;
	temp = strtod( *buf, buf );
	return (int)temp;
}

void ParseRGBSaber( char *str, vector3 *c ) {
	char *p = str;
	int i;

	for ( i = 0; i < 3; i++ ) {
		c->data[i] = (float)getint( &p );
		p++;
	}
}

static void UI_RGBForSaberColor( saber_colors_t color, vector3 *rgb, int bnum ) {
	int i;

	switch ( color ) {
	case SABER_RED:
		VectorSet( rgb, 1.0f, 0.2f, 0.2f );
		break;
	case SABER_ORANGE:
		VectorSet( rgb, 1.0f, 0.5f, 0.1f );
		break;
	case SABER_YELLOW:
		VectorSet( rgb, 1.0f, 1.0f, 0.2f );
		break;
	case SABER_GREEN:
		VectorSet( rgb, 0.2f, 1.0f, 0.2f );
		break;
	case SABER_BLUE:
		VectorSet( rgb, 0.2f, 0.4f, 1.0f );
		break;
	case SABER_PURPLE:
		VectorSet( rgb, 0.9f, 0.2f, 1.0f );
		break;
	case SABER_BLACK:
		VectorSet( rgb, 1.0f, 1.0f, 1.0f );
	default:
	case SABER_RGB:
		if ( bnum == 0 ) {
			rgb->r = atoi( UI_Cvar_VariableString( "ui_sab1_r" ) );
			rgb->g = atoi( UI_Cvar_VariableString( "ui_sab1_g" ) );
			rgb->b = atoi( UI_Cvar_VariableString( "ui_sab1_b" ) );
		}
		else {
			rgb->r = atoi( UI_Cvar_VariableString( "ui_sab2_r" ) );
			rgb->g = atoi( UI_Cvar_VariableString( "ui_sab2_g" ) );
			rgb->b = atoi( UI_Cvar_VariableString( "ui_sab2_b" ) );
		}
		for ( i = 0; i < 3; i++ )
			rgb->data[i] /= 255;
		break;
	}
}
void UI_DoSaber( vector3 *origin, vector3 *dir, float length, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight, int cnum, int bnum )
//void CG_DoSaber( vector3 *origin, vector3 *dir, float length, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight )
{
	vector3		mid;
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;
	refEntity_t sbak;
	vector3 rgb = { 1, 1, 1 };
	int i;
	float lol;

	// if the thing is so short, just forget even adding me.
	if ( length < 0.5f )
		return;

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, &mid );

	switch ( color ) {
	case SABER_RED:
		glow = redSaberGlowShader;
		blade = redSaberCoreShader;
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		blade = orangeSaberCoreShader;
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		blade = yellowSaberCoreShader;
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		blade = greenSaberCoreShader;
		break;
	case SABER_BLUE:
		glow = blueSaberGlowShader;
		blade = blueSaberCoreShader;
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		blade = purpleSaberCoreShader;
		break;
	default:
	case SABER_RGB:
		glow = rgbSaberGlowShader;
		blade = rgbSaberCoreShader;
		break;
	case SABER_FLAME1:
		glow = rgbSaberGlow2Shader;
		blade = rgbSaberCore2Shader;
		break;
	case SABER_ELEC1:
		glow = rgbSaberGlow3Shader;
		blade = rgbSaberCore3Shader;
		break;
	case SABER_FLAME2:
		glow = rgbSaberGlow4Shader;
		blade = rgbSaberCore4Shader;
		break;
	case SABER_ELEC2:
		glow = rgbSaberGlow5Shader;
		blade = rgbSaberCore5Shader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		blade = blackSaberCoreShader;
		break;
	}

	UI_RGBForSaberColor( color, &rgb, bnum );

	memset( &saber, 0, sizeof(refEntity_t) );

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if ( length < lengthMax )
		radiusmult = 1.0f + (2.0f / length);		// Note this creates a curve, and length cannot be < 0.5.
	else
		radiusmult = 1.0f;

	for ( i = 0; i < 3; i++ )
		rgb.data[i] *= 255;
	radiusRange = radius * 0.075f;
	radiusStart = radius - radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;

	VectorCopy( origin, &saber.origin );
	VectorCopy( dir, &saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;

	if ( color < SABER_RGB )
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else {
		for ( i = 0; i < 3; i++ )
			saber.shaderRGBA[i] = rgb.data[i];
		saber.shaderRGBA[3] = 0xff;
	}
	//	saber.renderfx = rfx;

	//RAZTODO: Pass in cent info so we can cull!!!
	//Raz: Glow
	//	saber.renderfx = RF_DEPTHHACK;
	SE_R_AddRefEntityToScene( &saber, MAX_CLIENTS );
	saber.renderfx = 0;

	// Do the hot core
	VectorMA( origin, length, dir, &saber.origin );
	VectorMA( origin, -1, dir, &saber.oldorigin );


	//	CG_TestLine(saber.origin, saber.oldorigin, 50, 0x000000ff, 3);
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius / 3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//	saber.radius = (1.0f + crandom() * 0.2f)*radiusmult;

	saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;

	memcpy( &sbak, &saber, sizeof(sbak) );

	if ( color >= SABER_RGB ) {
		switch ( color ) {
		default:
		case SABER_RGB:
			sbak.customShader = rgbSaberCoreShader;
			break;
		case SABER_FLAME1:
			sbak.customShader = rgbSaberCore2Shader;
			break;
		case SABER_ELEC1:
			sbak.customShader = rgbSaberCore3Shader;
			break;
		case SABER_FLAME2:
			sbak.customShader = rgbSaberCore4Shader;
			break;
		case SABER_ELEC2:
			sbak.customShader = rgbSaberCore5Shader;
			break;
		case SABER_BLACK:
			sbak.customShader = blackSaberCoreShader;
			break;
		}
	}

	sbak.shaderRGBA[0] = sbak.shaderRGBA[1] = sbak.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;

	lol = Q_fabs( (sinf( (float)trap->Milliseconds() / 400.0f )) );
	lol = (lol * 0.1f) + 1.0f;
	sbak.radius = lol;

	SE_R_AddRefEntityToScene( &sbak, MAX_CLIENTS );
}

void UI_DoSFXSaber( vector3 *blade_muz, vector3 *blade_tip, vector3 *trail_tip, vector3 *trail_muz, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight, qboolean doTrail, int cnum, int bnum ) {
	vector3	mid, blade_dir, end_dir, trail_dir, base_dir;
	float	radiusmult, effectradius, coreradius, effectalpha = 1.0f, AngleScale = 1.0f;
	float	blade_len, trail_len, base_len;
	vector3 rgb = { 1, 1, 1 };
	int i;

	qhandle_t	glow = 0;
	refEntity_t saber, sbak;

	VectorSubtract( blade_tip, blade_muz, &blade_dir );
	VectorSubtract( trail_tip, trail_muz, &trail_dir );
	blade_len = lengthMax;//VectorLength(blade_dir);
	trail_len = VectorLength( &trail_dir );
	VectorNormalize( &blade_dir );
	VectorNormalize( &trail_dir );

	if ( lengthMax < 1.0f ) {
		return;
	}

	VectorSubtract( trail_tip, blade_tip, &end_dir );
	VectorSubtract( trail_muz, blade_muz, &base_dir );
	base_len = VectorLength( &base_dir );
	VectorNormalize( &end_dir );
	VectorNormalize( &base_dir );

	switch ( color ) {
	case SABER_RED:
		glow = redSaberGlowShader;
		break;
	case SABER_ORANGE:
		glow = orangeSaberGlowShader;
		break;
	case SABER_YELLOW:
		glow = yellowSaberGlowShader;
		break;
	case SABER_GREEN:
		glow = greenSaberGlowShader;
		break;
	case SABER_PURPLE:
		glow = purpleSaberGlowShader;
		break;
		//	case SABER_WHITE:
	case SABER_RGB:
	case SABER_FLAME1:
	case SABER_ELEC1:
	case SABER_FLAME2:
	case SABER_ELEC2:
		glow = rgbSaberGlowShader;
		break;
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		break;
	default:
		glow = blueSaberGlowShader;
		break;
	}

	VectorMA( blade_muz, blade_len * 0.5f, &blade_dir, &mid );

	memset( &saber, 0, sizeof(refEntity_t) );

	if ( blade_len < lengthMax ) {
		radiusmult = 0.5f + ((blade_len / lengthMax) / 2);
	}
	else {
		radiusmult = 1.0f;
	}

	effectradius = ((radius * 1.6f * 1.0f) + crandom() * 0.1f)*radiusmult;
	coreradius = ((radius * 0.4f * 1.0f) + crandom() * 0.1f)*radiusmult;

	UI_RGBForSaberColor( color, &rgb, bnum );
	for ( i = 0; i<3; i++ )
		rgb.data[i] *= 255;
	{
		saber.renderfx = rfx;
		if ( blade_len - ((effectradius*1.0f) / 2) > 0 ) {
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (blade_len - (saber.radius / 2));
			VectorCopy( blade_muz, &saber.origin );
			VectorCopy( &blade_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * 1.0f;
			else {
				for ( i = 0; i < 3; i++ )
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( blade_muz, blade_len, &blade_dir, &saber.origin );
		VectorMA( blade_muz, -1, &blade_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else {
			for ( i = 0; i < 3; i++ )
				saber.shaderRGBA[i] = rgb.data[i];
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if ( color >= SABER_RGB /*|| color == SABER_WHITE*/ ) {// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			sbak.reType = RT_LINE;
			sbak.shaderTexCoord.x = sbak.shaderTexCoord.y = 1.0f;
			sbak.shaderRGBA[0] = sbak.shaderRGBA[1] = sbak.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
	}

	{
		saber.renderfx = rfx;
		if ( trail_len - ((effectradius*AngleScale) / 2) > 0 ) {
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (trail_len - (saber.radius / 2));
			VectorCopy( trail_muz, &saber.origin );
			VectorCopy( &trail_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * effectalpha;
			else {
				for ( i = 0; i < 3; i++ )
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}

			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( trail_muz, trail_len, &trail_dir, &saber.origin );
		VectorMA( trail_muz, -1, &trail_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else {
			for ( i = 0; i < 3; i++ )
				saber.shaderRGBA[i] = rgb.data[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if ( color >= SABER_RGB /*|| color == SABER_WHITE*/ ) {// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			sbak.reType = RT_LINE;
			sbak.shaderTexCoord.x = sbak.shaderTexCoord.y = 1.0f;
			sbak.shaderRGBA[0] = sbak.shaderRGBA[1] = sbak.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
	}

	VectorMA( blade_muz, blade_len - 0.5f, &blade_dir, blade_tip );
	VectorMA( trail_muz, trail_len - 0.5f, &trail_dir, trail_tip );

	if ( base_len > 2 ) {
		saber.renderfx = rfx;
		if ( base_len - (effectradius*AngleScale) > 0 ) {
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (base_len - (effectradius*AngleScale));
			VectorMA( blade_muz, ((effectradius*AngleScale) / 2), &base_dir, &saber.origin );
			VectorCopy( &base_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * effectalpha;
			else {
				for ( i = 0; i < 3; i++ )
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( blade_muz, base_len, &base_dir, &saber.origin );
		VectorMA( blade_muz, -0.1f, &base_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		if ( color < SABER_RGB /*&& color != SABER_WHITE*/ )
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else {
			for ( i = 0; i < 3; i++ )
				saber.shaderRGBA[i] = rgb.data[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if ( color >= SABER_RGB /*|| color == SABER_WHITE*/ ) {// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
	}
}

void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vector3 *origin, vector3 *angles, int bladeNum ) {
	vector3	org_, end,
		axis_[3] = { { 0.0f } };	// shut the compiler up
	mdxaBone_t	boltMatrix;
	effectTrailArgStruct_t fx;
	saber_colors_t bladeColor;
	float bladeLength, bladeRadius;
	char bladeColorString[MAX_QPATH];
	int snum;
	const char *tagName;
	int bolt;
	qboolean tagHack = qfalse;
	int styleToUse = atoi( UI_Cvar_VariableString( "cg_saberBladeStyle" ) );


	if ( (item->flags&ITF_ISSABER) && saberModel < 2 ) {
		snum = 0;
		trap->Cvar_VariableStringBuffer( "ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else//if ( item->flags&ITF_ISSABER2 ) - presumed
	{
		snum = 1;
		trap->Cvar_VariableStringBuffer( "ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}

	if ( !trap->G2API_HasGhoul2ModelOnIndex( &(item->ghoul2), saberModel ) ) {//invalid index!
		return;
	}

	bladeColor = TranslateSaberColor( bladeColorString );

	bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );

	tagName = va( "*blade%d", bladeNum + 1 );
	bolt = trap->G2API_AddBolt( item->ghoul2, saberModel, tagName );

	if ( bolt == -1 ) {
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = trap->G2API_AddBolt( item->ghoul2, saberModel, "*flash" );
		if ( bolt == -1 ) {//no tag_flash either?!!
			bolt = 0;
		}
	}

	//	angles.pitch = curYaw;
	//	angles.roll = 0;

	trap->G2API_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, &vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, &org_ );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, &axis_[0] );//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
	//		...changed this back to NEGATIVE_Y
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, &axis_[1] );//right ... and changed this to NEGATIVE_X
	BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_Z, &axis_[2] );//up

	VectorMA( &org_, bladeLength, &axis_[0], &end );

	VectorAdd( &end, &axis_[0], &end );

	if ( tagHack ) {
		switch ( saberType ) {
		default:
		case SABER_SINGLE:
			VectorMA( &org_, 1.0f, &axis_[0], &org_ );
			break;
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 0 )
				VectorMA( &org_, 12 * 1.0f, &axis_[0], &org_ );

			if ( bladeNum == 1 ) {
				VectorScale( &axis_[0], -1, &axis_[0] );
				VectorMA( &org_, 12 * 1.0f, &axis_[0], &org_ );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
				VectorMA( &org_, -1 * 1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 1 )
				VectorMA( &org_, 1 * 1.0f, &axis_[1], &org_ );
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
				VectorMA( &org_, -3 * 1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 1 )
				VectorMA( &org_, 3 * 1.0f, &axis_[1], &org_ );
			break;
		case SABER_ARC:
			VectorSubtract( &axis_[1], &axis_[2], &axis_[1] );
			VectorNormalize( &axis_[1] );
			switch ( bladeNum ) {
			case 0:
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], 0.75f, &axis_[0] );
				VectorScale( &axis_[1], 0.25f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			case 1:
				VectorScale( &axis_[0], 0.25f, &axis_[0] );
				VectorScale( &axis_[1], 0.75f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			case 2:
				VectorMA( &org_, -8 * 1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -0.25f, &axis_[0] );
				VectorScale( &axis_[1], 0.75f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			case 3:
				VectorMA( &org_, -16 * 1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -0.75f, &axis_[0] );
				VectorScale( &axis_[1], 0.25f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			default:
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
				VectorMA( &org_, -3 * 1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 2 )
				VectorMA( &org_, 3 * 1.0f, &axis_[1], &org_ );
			break;
		case SABER_CLAW:
			switch ( bladeNum ) {
			case 0:
				VectorMA( &org_, 2 * 1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2 * 1.0f, &axis_[2], &org_ );
				break;
			case 1:
				VectorMA( &org_, 2 * 1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2 * 1.0f, &axis_[2], &org_ );
				VectorMA( &org_, 2 * 1.0f, &axis_[1], &org_ );
				break;
			case 2:
				VectorMA( &org_, 2 * 1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2 * 1.0f, &axis_[2], &org_ );
				VectorMA( &org_, -2 * 1.0f, &axis_[1], &org_ );
				break;
			default:
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum ) {
			case 0:
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			case 1:
				VectorScale( &axis_[0], 0.33f, &axis_[0] );
				VectorScale( &axis_[2], 0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			case 2:
				VectorScale( &axis_[0], -0.33f, &axis_[0] );
				VectorScale( &axis_[2], 0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			case 3:
				VectorScale( &axis_[0], -1, &axis_[0] );
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			case 4:
				VectorScale( &axis_[0], -0.33f, &axis_[0] );
				VectorScale( &axis_[2], -0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			case 5:
				VectorScale( &axis_[0], 0.33f, &axis_[0] );
				VectorScale( &axis_[2], -0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8 * 1.0f, &axis_[0], &org_ );
				break;
			default:
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum ) {
			case 0:
				VectorMA( &org_, 24 * 1.0f, &axis_[0], &org_ );
				break;
			case 1:
				VectorMA( &org_, -6 * 1.0f, &axis_[1], &org_ );
				VectorMA( &org_, 24 * 1.0f, &axis_[0], &org_ );
				break;
			case 2:
				VectorMA( &org_, 6 * 1.0f, &axis_[1], &org_ );
				VectorMA( &org_, 24 * 1.0f, &axis_[0], &org_ );
				break;
			case 3:
				VectorMA( &org_, -32 * 1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -1, &axis_[0] );
				break;
			default:
				break;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		}
	}

	VectorCopy( &org_, &fx.mVerts[0].origin );
	VectorMA( &end, 3.0f, &axis_[0], &fx.mVerts[1].origin );
	VectorCopy( &end, &fx.mVerts[2].origin );
	VectorMA( &org_, 3.0f, &axis_[0], &fx.mVerts[3].origin );


	//Raz: Temporarily switch to basejka sabers for flame and electric users
	if ( bladeColor == SABER_FLAME1 || bladeColor == SABER_ELEC1 ||
		bladeColor == SABER_FLAME2 || bladeColor == SABER_ELEC2 ||
		bladeColor == SABER_BLACK )
		styleToUse = 0;

	// Pass in the renderfx flags attached to the saber weapon model...this is done so that saber glows
	//	will get rendered properly in a mirror...not sure if this is necessary??
	//CG_DoSaber( org_, axis_[0], saberLen, client->saber[saberNum].blade[bladeNum].lengthMax, client->saber[saberNum].blade[bladeNum].radius,
	//	scolor, renderfx, (qboolean)(saberNum==0&&bladeNum==0) );
	switch ( styleToUse ) {
	case 0:
		UI_DoSaber( &org_, &axis_[0], bladeLength, bladeLength, bladeRadius, bladeColor, 0, qfalse, 0, snum );
		break;
		/*
			case 1:
			UI_DoEp1Saber( fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin, bladeLength, bladeRadius, bladeColor, 0, false, false, 0, snum );
			break;
			case 2:
			UI_DoEp2Saber( fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin, bladeLength, bladeRadius, bladeColor, 0, false, false, 0, snum );
			break;
			case 3:
			UI_DoEp3Saber( fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin, bladeLength, bladeRadius, bladeColor, 0, false, false, 0, snum );
			break;
			*/
	default:
	case 1:
		UI_DoSFXSaber( &fx.mVerts[0].origin, &fx.mVerts[1].origin, &fx.mVerts[2].origin, &fx.mVerts[3].origin, bladeLength, bladeRadius, bladeColor, 0, qfalse, qfalse, 0, snum );
		break;
		/*
			case 5:
			UI_DoOTSaber( fx.mVerts[0].origin, fx.mVerts[1].origin, fx.mVerts[2].origin, fx.mVerts[3].origin, bladeLength, bladeRadius, bladeColor, 0, false, false, 0, snum );
			break;
			*/
	}
}

/*
void UI_SaberDrawBlades( itemDef_t *item, vector3 *origin, vector3 *angles )
{
//NOTE: only allows one saber type in view at a time
char saber[MAX_QPATH];
if ( item->flags&ITF_ISSABER )
{
trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
if ( !UI_SaberValidForPlayerInMP( saber ) )
{
trap->Cvar_Set( "ui_saber", "kyle" );
trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
}
}
else if ( item->flags&ITF_ISSABER2 )
{
trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
if ( !UI_SaberValidForPlayerInMP( saber ) )
{
trap->Cvar_Set( "ui_saber2", "kyle" );
trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
}
}
else
{
return;
}
if ( saber[0] )
{
saberType_t saberType;
int curBlade;
int numBlades = UI_SaberNumBladesForSaber( saber );
if ( numBlades )
{//okay, here we go, time to draw each blade...
char	saberTypeString[MAX_QPATH]={0};
UI_SaberTypeForSaber( saber, saberTypeString );
saberType = TranslateSaberType( saberTypeString );
for ( curBlade = 0; curBlade < numBlades; curBlade++ )
{
UI_SaberDrawBlade( item, saber, saberType, origin, angles, curBlade );
}
}
}
}
*/

void UI_GetSaberForMenu( char *saber, int saberNum ) {
	char saberTypeString[MAX_QPATH] = { 0 };
	saberType_t saberType = SABER_NONE;

	if ( saberNum == 0 ) {
		trap->Cvar_VariableStringBuffer( "ui_saber", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) ) {
			trap->Cvar_Set( "ui_saber", "kyle" );
			trap->Cvar_VariableStringBuffer( "ui_saber", saber, MAX_QPATH );
		}
	}
	else {
		trap->Cvar_VariableStringBuffer( "ui_saber2", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) ) {
			trap->Cvar_Set( "ui_saber2", "kyle" );
			trap->Cvar_VariableStringBuffer( "ui_saber2", saber, MAX_QPATH );
		}
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber( saber, saberTypeString );
	if ( saberTypeString[0] ) {
		saberType = TranslateSaberType( saberTypeString );
	}

	switch ( uiInfo.movesTitleIndex ) {
	case MD_ACROBATICS:
		break;
	case MD_SINGLE_FAST:
	case MD_SINGLE_MEDIUM:
	case MD_SINGLE_STRONG:
		if ( saberType != SABER_SINGLE )
			Q_strncpyz( saber, "single_1", MAX_QPATH );
		break;
	case MD_DUAL_SABERS:
		if ( saberType != SABER_SINGLE )
			Q_strncpyz( saber, "single_1", MAX_QPATH );
		break;
	case MD_SABER_STAFF:
		if ( saberType == SABER_SINGLE || saberType == SABER_NONE )
			Q_strncpyz( saber, "dual_1", MAX_QPATH );
		break;
	default:
		break;
	}
}

void UI_SaberDrawBlades( itemDef_t *item, vector3 *origin, vector3 *angles ) {
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	int saberNum = 0;
	int saberModel = 0;
	int	numSabers = 1;

	if ( (item->flags&ITF_ISCHARACTER)//hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ ) {
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ ) {
		if ( (item->flags&ITF_ISCHARACTER) )//hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu( saber, saberNum );
			saberModel = saberNum + 1;
		}
		else if ( (item->flags&ITF_ISSABER) ) {
			trap->Cvar_VariableStringBuffer( "ui_saber", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) ) {
				trap->Cvar_Set( "ui_saber", "kyle" );
				trap->Cvar_VariableStringBuffer( "ui_saber", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else if ( (item->flags&ITF_ISSABER2) ) {
			trap->Cvar_VariableStringBuffer( "ui_saber2", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) ) {
				trap->Cvar_Set( "ui_saber2", "kyle" );
				trap->Cvar_VariableStringBuffer( "ui_saber2", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else {
			return;
		}
		if ( saber[0] ) {
			saberType_t saberType;
			int curBlade = 0;
			int numBlades = UI_SaberNumBladesForSaber( saber );
			if ( numBlades ) {//okay, here we go, time to draw each blade...
				char	saberTypeString[MAX_QPATH] = { 0 };
				UI_SaberTypeForSaber( saber, saberTypeString );
				saberType = TranslateSaberType( saberTypeString );
				for ( curBlade = 0; curBlade < numBlades; curBlade++ ) {
					if ( UI_SaberShouldDrawBlade( saber, curBlade ) ) {
						UI_SaberDrawBlade( item, saber, saberModel, saberType, origin, angles, curBlade );
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar( itemDef_t *item ) {
	int	numSabers = 1;
	int	saberNum = 0;

	if ( trap->G2API_HasGhoul2ModelOnIndex( &(item->ghoul2), 2 ) ) {//remove any extra models
		trap->G2API_RemoveGhoul2Model( &(item->ghoul2), 2 );
	}
	if ( trap->G2API_HasGhoul2ModelOnIndex( &(item->ghoul2), 1 ) ) {//remove any extra models
		trap->G2API_RemoveGhoul2Model( &(item->ghoul2), 1 );
	}

	if ( uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ ) {
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ ) {
		//bolt sabers
		char modelPath[MAX_QPATH];
		char skinPath[MAX_QPATH];
		char saber[MAX_QPATH];

		UI_GetSaberForMenu( saber, saberNum );

		//RAZTODO: UI_SaberAttachToChar is different in JA+
		if ( UI_SaberModelForSaber( saber, modelPath ) ) {//successfully found a model
			int g2Saber = trap->G2API_InitGhoul2Model( &(item->ghoul2), modelPath, 0, 0, 0, 0, 0 ); //add the model
			if ( g2Saber ) {
				int boltNum;
				//get the customSkin, if any
				if ( UI_SaberSkinForSaber( saber, skinPath ) ) {
					int g2skin = trap->R_RegisterSkin( skinPath );
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, g2skin );//this is going to set the surfs on/off matching the skin file
				}
				else {
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, 0 );//turn off custom skin
				}
				if ( saberNum == 0 ) {
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*r_hand" );
				}
				else {
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*l_hand" );
				}
				trap->G2API_AttachG2Model( item->ghoul2, g2Saber, item->ghoul2, boltNum, 0 );
			}
		}
	}
}
