//
/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "ui_local.h"
#include "ui_shared.h"

//#define MAX_SABER_DATA_SIZE 0x8000
#define MAX_SABER_DATA_SIZE 0x80000

// On Xbox, static linking lets us steal the buffer from wp_saberLoad
// Just make sure that the saber data size is the same
// Damn. OK. Gotta fix this again. Later.
static char	SaberParms[MAX_SABER_DATA_SIZE];
qboolean	ui_saber_parms_parsed = qfalse;

static qhandle_t redSaberGlowShader;
static qhandle_t redSaberCoreShader;
static qhandle_t orangeSaberGlowShader;
static qhandle_t orangeSaberCoreShader;
static qhandle_t yellowSaberGlowShader;
static qhandle_t yellowSaberCoreShader;
static qhandle_t greenSaberGlowShader;
static qhandle_t greenSaberCoreShader;
static qhandle_t blueSaberGlowShader;
static qhandle_t blueSaberCoreShader;
static qhandle_t purpleSaberGlowShader;
static qhandle_t purpleSaberCoreShader;
	//[RGBSabers]
	static qhandle_t	rgbSaberGlowShader;
	static qhandle_t	rgbSaberCoreShader;

	static qhandle_t	rgbSaberGlow2Shader;
	static qhandle_t	rgbSaberCore2Shader;
	static qhandle_t	rgbSaberTrail2Shader;

	static qhandle_t	rgbSaberGlow3Shader;
	static qhandle_t	rgbSaberCore3Shader;
	static qhandle_t	rgbSaberTrail3Shader;

	static qhandle_t	rgbSaberGlow4Shader;
	static qhandle_t	rgbSaberCore4Shader;
	static qhandle_t	rgbSaberTrail4Shader;

	static qhandle_t	rgbSaberGlow5Shader;
	static qhandle_t	rgbSaberCore5Shader;
	static qhandle_t	rgbSaberTrail5Shader;

	static qhandle_t	blackSaberGlowShader;
	static qhandle_t	blackSaberCoreShader;
	static qhandle_t	blackBlurShader;
	//[/RGBSabers]
	//[Movie Sabers]
#if 0
	//Original Trilogy Sabers
	static qhandle_t otSaberCoreShader;			
	static qhandle_t redOTGlowShader;				
	static qhandle_t orangeOTGlowShader;			
	static qhandle_t yellowOTGlowShader;			
	static qhandle_t greenOTGlowShader;			
	static qhandle_t blueOTGlowShader;			
	static qhandle_t purpleOTGlowShader;			

	//Episode I Sabers
	static qhandle_t ep1SaberCoreShader;
	static qhandle_t redEp1GlowShader;			
	static qhandle_t orangeEp1GlowShader;			
	static qhandle_t yellowEp1GlowShader;			
	static qhandle_t greenEp1GlowShader;			
	static qhandle_t blueEp1GlowShader;			
	static qhandle_t purpleEp1GlowShader;

	//Episode II Sabers
	static qhandle_t ep2SaberCoreShader;
	static qhandle_t whiteIgniteFlare;
	static qhandle_t blackIgniteFlare;
	static qhandle_t redEp2GlowShader;			
	static qhandle_t orangeEp2GlowShader;			
	static qhandle_t yellowEp2GlowShader;			
	static qhandle_t greenEp2GlowShader;			
	static qhandle_t blueEp2GlowShader;			
	static qhandle_t purpleEp2GlowShader;

	//Episode III Sabers
	static qhandle_t ep3SaberCoreShader;
	static qhandle_t whiteIgniteFlare02;
	static qhandle_t blackIgniteFlare02;
	static qhandle_t redIgniteFlare;
	static qhandle_t greenIgniteFlare;
	static qhandle_t purpleIgniteFlare;
	static qhandle_t blueIgniteFlare;
	static qhandle_t orangeIgniteFlare;
	static qhandle_t yellowIgniteFlare;
	static qhandle_t redEp3GlowShader;			
	static qhandle_t orangeEp3GlowShader;			
	static qhandle_t yellowEp3GlowShader;			
	static qhandle_t greenEp3GlowShader;			
	static qhandle_t blueEp3GlowShader;			
	static qhandle_t purpleEp3GlowShader;			
#endif
	//[Movie Sabers]
	//[SFXSabers]
	static qhandle_t sfxSaberTrailShader;
	static qhandle_t sfxSaberBladeShader;
	static qhandle_t sfxSaberBlade2Shader;
	static qhandle_t sfxSaberEndShader;
	static qhandle_t sfxSaberEnd2Shader;
	//[/SFXSabers]

void UI_CacheSaberGlowGraphics( void )
{//FIXME: these get fucked by vid_restarts
	redSaberGlowShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_glow" );
	redSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/red_line" );
	orangeSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_glow" );
	orangeSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/orange_line" );
	yellowSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_glow" );
	yellowSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_line" );
	greenSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_glow" );
	greenSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/green_line" );
	blueSaberGlowShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_glow" );
	blueSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blue_line" );
	purpleSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_glow" );
	purpleSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/purple_line" );
	//[RGBSabers]
	rgbSaberGlowShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow1" );
	rgbSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore1" );

	//Flame 1
	rgbSaberGlow2Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow2" );
	rgbSaberCore2Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore2" );
	rgbSaberTrail2Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail2" );

	//Electric 1
	rgbSaberGlow3Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow3" );
	rgbSaberCore3Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore3" );
	rgbSaberTrail3Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail3" );

	//Flame 2
	rgbSaberGlow4Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow4" );
	rgbSaberCore4Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore4" );
	rgbSaberTrail4Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail4" );

	//Electric 2
	rgbSaberGlow5Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBglow5" );
	rgbSaberCore5Shader			= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBcore5" );
	rgbSaberTrail5Shader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/RGBtrail5" );

	//Black
	blackSaberGlowShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackglow" );
	blackSaberCoreShader		= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blackcore" );
	blackBlurShader				= trap->R_RegisterShaderNoMip( "gfx/effects/sabers/blacktrail" );
	//[/RGBSabers]

	//[SFXSabers]
    sfxSaberTrailShader			= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_trail" );
    sfxSaberBladeShader			= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade" );
    sfxSaberBlade2Shader		= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_blade_rgb" );
    sfxSaberEndShader			= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end" );
    sfxSaberEnd2Shader			= trap->R_RegisterShaderNoMip( "gfx/SFX_Sabers/saber_end_rgb" );
    //[/SFXSabers]

	//[Movie Sabers]
#if 0
	//Original Trilogy Sabers
	otSaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_saberCore" );
	redOTGlowShader				= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_redGlow" );
	orangeOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_orangeGlow" );
	yellowOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_yellowGlow" );
	greenOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_greenGlow" );
	blueOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_blueGlow" );
	purpleOTGlowShader			= trap->R_RegisterShaderNoMip( "gfx/OTsabers/ot_purpleGlow" );

	//Episode I Sabers
	ep1SaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/saber_core" );
	redEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/red_glowa" );
	orangeEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/orange_glowa" );
	yellowEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/yellow_glowa" );
	greenEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/green_glowa" );
	blueEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/blue_glowa" );
	purpleEp1GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep1Sabers/purple_glowa" );

	//Episode II Sabers
	ep2SaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/saber_core" );
	whiteIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/white_ignite_flare" );
	blackIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/black_ignite_flare" );
	redEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/red_glowa" );
	orangeEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/orange_glowa" );
	yellowEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/yellow_glowa" );
	greenEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/green_glowa" );
	blueEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/blue_glowa" );
	purpleEp2GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep2Sabers/purple_glowa" );

	//Episode III Sabers
	ep3SaberCoreShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/saber_core" );
	whiteIgniteFlare02			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/white_ignite_flare" );
	blackIgniteFlare02			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/black_ignite_flare" );
	redIgniteFlare				= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/red_ignite_flare" );
	greenIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/green_ignite_flare" );
	purpleIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/purple_ignite_flare" );
	blueIgniteFlare				= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/blue_ignite_flare" );
	orangeIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/orange_ignite_flare" );
	yellowIgniteFlare			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/yellow_ignite_flare" );
	redEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/red_glowa" );
	orangeEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/orange_glowa" );
	yellowEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/yellow_glowa" );
	greenEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/green_glowa" );
	blueEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/blue_glowa" );
	purpleEp3GlowShader			= trap->R_RegisterShaderNoMip( "gfx/Ep3Sabers/purple_glowa" );
#endif
	//[/Movie Sabers]
}

qboolean UI_ParseLiteral( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		Com_Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

qboolean UI_ParseLiteralSilent( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		return qtrue;
	}

	return qfalse;
}

qboolean UI_SaberParseParm( const char *saberName, const char *parmname, char *saberData ) 
{
	const char	*token;
	const char	*value;
	const char	*p;

	if ( !saberName || !saberName[0] ) 
	{
		return qfalse;
	}

	//try to parse it out
	p = SaberParms;
	// A bogus name is passed in
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, saberName ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{
		return qfalse;
	}

	if ( UI_ParseLiteral( &p, "{" ) ) 
	{
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saberName );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) ) 
		{
			break;
		}

		if ( !Q_stricmp( token, parmname ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			strcpy( saberData, value );
			return qtrue;
		}

		SkipRestOfLine( &p );
		continue;
	}

	return qfalse;
}


qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel )
{
	return UI_SaberParseParm( saberName, "saberModel", saberModel );
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin )
{
	return UI_SaberParseParm( saberName, "customSkin", saberSkin );
}

qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType )
{
	return UI_SaberParseParm( saberName, "saberType", saberType );
}

int UI_SaberNumBladesForSaber( const char *saberName )
{
	int numBlades;
	char	numBladesString[8]={0};
	UI_SaberParseParm( saberName, "numBlades", numBladesString );
	numBlades = atoi( numBladesString );
	if ( numBlades < 1 )
	{
		numBlades = 1;
	}
	else if ( numBlades > 8 )
	{
		numBlades = 8;
	}
	return numBlades;
}

qboolean UI_SaberShouldDrawBlade( const char *saberName, int bladeNum )
{
	int bladeStyle2Start = 0, noBlade = 0;
	char	bladeStyle2StartString[8]={0};
	char	noBladeString[8]={0};
	UI_SaberParseParm( saberName, "bladeStyle2Start", bladeStyle2StartString );
	if ( bladeStyle2StartString[0] )
	{
		bladeStyle2Start = atoi( bladeStyle2StartString );
	}
	if ( bladeStyle2Start
		&& bladeNum >= bladeStyle2Start )
	{//use second blade style
		UI_SaberParseParm( saberName, "noBlade2", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	else
	{//use first blade style
		UI_SaberParseParm( saberName, "noBlade", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	return ((qboolean)(noBlade==0));
}


qboolean UI_IsSaberTwoHanded( const char *saberName )
{
	int twoHanded;
	char	twoHandedString[8]={0};
	UI_SaberParseParm( saberName, "twoHanded", twoHandedString );
	if ( !twoHandedString[0] )
	{//not defined defaults to "no"
		return qfalse;
	}
	twoHanded = atoi( twoHandedString );
	return ((qboolean)(twoHanded!=0));
}

float UI_SaberBladeLengthForSaber( const char *saberName, int bladeNum )
{
	char	lengthString[8]={0};
	float	length = 40.0f;
	UI_SaberParseParm( saberName, "saberLength", lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberLength%d", bladeNum+1), lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	return length;
}

float UI_SaberBladeRadiusForSaber( const char *saberName, int bladeNum )
{
	char	radiusString[8]={0};
	float	radius = 3.0f;
	UI_SaberParseParm( saberName, "saberRadius", radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberRadius%d", bladeNum+1), radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	return radius;
}

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName, int destsize )
{
	char stringedSaberName[1024] = {0};
	qboolean ret = UI_SaberParseParm( saberName, "name", stringedSaberName );

	// if it's a stringed reference translate it
	if ( ret && stringedSaberName[0] == '@')
		trap->SE_GetStringTextString( &stringedSaberName[1], saberProperName, destsize );
	else if ( !stringedSaberName[0] )
		Q_strncpyz( saberProperName, va( "^1<%s^1>", saberName ), destsize );
	else // no stringed so just use it as it
		Q_strncpyz( saberProperName, stringedSaberName, destsize );

	return ret;
}

qboolean UI_SaberValidForPlayerInMP( const char *saberName )
{
	char allowed [8]={0};
	if ( !UI_SaberParseParm( saberName, "notInMP", allowed ) )
	{//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] )
	{//not defined, default is yes
		return qtrue;
	}
	else
	{//return value
		#ifndef _GAME
			return qtrue;
		#else
			return ((qboolean)(atoi(allowed)==0));
		#endif
	}
}

void UI_SaberLoadParms( void ) 
{
	int			len, totallen, saberExtFNLen, fileCnt, i;
	char		*holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t f;
	char buffer[MAX_MENUFILE];

	//ui.Printf( "UI Parsing *.sab saber definitions\n" );
	
	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';

	//now load in the extra .npc extensions
	fileCnt = trap->FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = trap->FS_Open( va( "ext_data/sabers/%s", holdChar), &f, FS_READ );

		if (!f)
		{
			continue;
		}

		if ( len == -1 ) 
		{
			Com_Printf( "UI_SaberLoadParms: error reading %s\n", holdChar );
		}
		else
		{
			if (len > sizeof(buffer) )
			{
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: file %s too large to read (max=%d)", holdChar, sizeof(buffer) );
			}
			trap->FS_Read( buffer, len, f );
			trap->FS_Close( f );
			buffer[len] = 0;

			if ( totallen && *(marker-1) == '}' )
			{//don't let it end on a } because that should be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++; 
			}
			len = COM_Compress( buffer );

			if ( totallen + len >= MAX_SABER_DATA_SIZE ) {
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: ran out of space before reading %s\n(you must make the .sab files smaller)", holdChar );
			}
			strcat( marker, buffer );

			totallen += len;
			marker += len;
		}
	}
}

//[RGBSabers]
void RGB_LerpColor(vector3 *from, vector3 *to, float frac, vector3 *out)
{
	vector3 diff;
	int i;

	VectorSubtract(to,from,&diff);

	VectorCopy(from,out);

	for(i=0;i<3;i++)
		out->data[i] += diff.data[i] * frac;

}

int getint(char **buf)
{
	double temp;
	temp = strtod(*buf,buf);
	return (int)temp;
}

void ParseRGBSaber(char *str, vector3 *c)
{
	char *p = str;
	int i;

	for(i=0;i<3;i++)
	{
		c->data[i] = (float)getint(&p);
		p++;
	}
}

#if 0
void UI_DoSaber( vector3 *origin, vector3 *dir, float length, float lengthMax, float radius, saber_colors_t color, int snum )
//[/RGBSabers]
{
	vector3		mid, rgb={1,1,1};
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;
	//[RGBSabers]
	int i;
	//[/RGBSabers]

	if ( length < 0.5f )
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, mid );

	switch( color )
	{
		case SABER_RED:
			glow = redSaberGlowShader;
			blade = redSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			glow = orangeSaberGlowShader;
			blade = orangeSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			glow = yellowSaberGlowShader;
			blade = yellowSaberCoreShader;
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			glow = greenSaberGlowShader;
			blade = greenSaberCoreShader;
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			glow = blueSaberGlowShader;
			blade = blueSaberCoreShader;
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			glow = purpleSaberGlowShader;
			blade = purpleSaberCoreShader;
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
		//[RGBSabers]
		default:
			//Implies other 'colours' in default:
		case SABER_RGB:
			{
				if(snum == 0)
					VectorSet( rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value );
				else
					VectorSet( rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value );

				for(i=0;i<3;i++)
					rgb[i]/=255;

				glow = rgbSaberGlowShader;
				blade = rgbSaberCoreShader;
			}
			break;
		//[/RGBSabers]
	}

	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
	/*
	if ( doLight )
	{//FIXME: RGB combine all the colors of the sabers you're using into one averaged color!
		cgi_R_AddLightToScene( mid, (length*2.0f) + (random()*8.0f), rgb[0], rgb[1], rgb[2] );
	}
	*/

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.  
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax )
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	radiusRange = radius * 0.075f;
	radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;


	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	//[RGBSabers]
	if(color < SABER_RGB)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		int i;
		for(i=0;i<3;i++)
			saber.shaderRGBA[i] = ClampChar((int)rgb[i]*255);
		saber.shaderRGBA[3] = 255;
	}
	//[/RGBSabers]
	//saber.renderfx = rfx;

	SE_R_AddRefEntityToScene( &saber, MAX_CLIENTS );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
//	saber.radius = (1.0 + crandom() * 0.2f)*radiusmult;

	SE_R_AddRefEntityToScene( &saber, MAX_CLIENTS );

	//[RGBSabers]
	if(color < SABER_RGB)
		return;

	saber.customShader = rgbSaberCore2Shader;
	saber.reType = RT_LINE;
	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	SE_R_AddRefEntityToScene( &saber, MAX_CLIENTS );
	//[/RGBSabers]
}
#endif

//[RGBSabers]
static void UI_RGBForSaberColor( saber_colors_t color, vector3 *rgb, int bnum )
{
	int i;

	switch( color )
	{
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
		if(bnum == 0)
		{
			rgb->r = atoi( UI_Cvar_VariableString( "ui_sab1_r" ) );
			rgb->g = atoi( UI_Cvar_VariableString( "ui_sab1_g" ) );
			rgb->b = atoi( UI_Cvar_VariableString( "ui_sab1_b" ) );
		}
		else
		{
			rgb->r = atoi( UI_Cvar_VariableString( "ui_sab2_r" ) );
			rgb->g = atoi( UI_Cvar_VariableString( "ui_sab2_g" ) );
			rgb->b = atoi( UI_Cvar_VariableString( "ui_sab2_b" ) );
		}
		for(i=0;i<3;i++)
			rgb->data[i]/=255;
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
	//[RGBSabers]
	refEntity_t sbak;
	vector3 rgb={1,1,1};
	int i;
	float lol;
	//[/RGBSabers]

	// if the thing is so short, just forget even adding me.
	if ( length < 0.5f )
		return;

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, &mid );

	switch( color )
	{
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
		//[RGBSabers]
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
		//[/RGBSabers]
	}

	UI_RGBForSaberColor( color, &rgb , bnum);

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.  
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax)
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	else
		radiusmult = 1.0;

	//[RGBSabers]
	for ( i=0; i<3; i++ )
		rgb.data[i] *= 255;
	//[/RGBSabers]
	radiusRange = radius * 0.075f;
	radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;

	VectorCopy( origin, &saber.origin );
	VectorCopy( dir, &saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;

	//[RGBSabers]
	if ( color < SABER_RGB )
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		for ( i=0; i<3; i++ )
			saber.shaderRGBA[i] = rgb.data[i];
		saber.shaderRGBA[3] = 0xff;
	}
	//[/RGBSabers]
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
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//	saber.radius = (1.0 + crandom() * 0.2f)*radiusmult;

	saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;

	//[RGBSabers]
#if 0
	if(color < SABER_RGB)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		for(i=0;i<3;i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}


	//	SE_R_AddRefEntityToScene( &saber, MAX_CLIENTS );

	if(color < SABER_RGB)
		return;
#endif

	memcpy( &sbak, &saber, sizeof( sbak ) );

	if ( color >= SABER_RGB )
	{
		switch ( color )
		{
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

	lol = Q_fabs((sinf((float)trap->Milliseconds() / 400.0f)));
	lol = (lol * 0.1f) + 1.0;
	sbak.radius = lol;

	SE_R_AddRefEntityToScene( &sbak, MAX_CLIENTS );
	//[/RGBSabers]
}

//[SFXSabers]
void UI_DoSFXSaber( vector3 *blade_muz, vector3 *blade_tip, vector3 *trail_tip, vector3 *trail_muz, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight, qboolean doTrail, int cnum, int bnum )
{
	vector3	mid, blade_dir, end_dir, trail_dir, base_dir;
	float	radiusmult, effectradius, coreradius, effectalpha=1.0f, AngleScale=1.0f;
	float	blade_len, end_len, trail_len, base_len;
	//[RGBSabers]
	vector3 rgb={1,1,1};
	int i;
	//[/RGBSabers]

	qhandle_t	glow = 0;
	refEntity_t saber,sbak;

	VectorSubtract( blade_tip, blade_muz, &blade_dir );
	VectorSubtract( trail_tip, trail_muz, &trail_dir );
	blade_len = lengthMax;//VectorLength(blade_dir);
	trail_len = VectorLength(&trail_dir);
	VectorNormalize(&blade_dir);
	VectorNormalize(&trail_dir);

	if ( lengthMax < 1.0f )
	{
		return;
	}

	VectorSubtract( trail_tip, blade_tip, &end_dir );
	VectorSubtract( trail_muz, blade_muz, &base_dir );
	end_len = VectorLength(&end_dir);
	base_len = VectorLength(&base_dir);
	VectorNormalize(&end_dir);
	VectorNormalize(&base_dir);

	switch( color )
	{
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
		//[RGBSabers]
		//	case SABER_WHITE:
	case SABER_RGB:
	case SABER_FLAME1:
	case SABER_ELEC1:
	case SABER_FLAME2:
	case SABER_ELEC2:
		glow = rgbSaberGlowShader;
		break;
#if _DISABLED
	case SABER_BLACK:
		glow = blackSaberGlowShader;
		break;
#endif
		//[/RGBSabers]
	default:
		glow = blueSaberGlowShader;
		break;
	}

	VectorMA( blade_muz, blade_len * 0.5f, &blade_dir, &mid );

	memset( &saber, 0, sizeof( refEntity_t ));

	if (blade_len < lengthMax)
	{
		radiusmult = 0.5 + ((blade_len / lengthMax)/2);
	}
	else
	{
		radiusmult = 1.0;
	}

	effectradius	= ((radius * 1.6 * 1.0f) + crandom() * 0.1f)*radiusmult;
	coreradius		= ((radius * 0.4 * 1.0f) + crandom() * 0.1f)*radiusmult;

	UI_RGBForSaberColor( color, &rgb, bnum );
	//[RGBSabers]
	for(i=0;i<3;i++)
		rgb.data[i] *= 255;
	//[/RGBSabers]

	{
		saber.renderfx = rfx;
		if(blade_len-((effectradius*1.0f)/2) > 0)
		{
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (blade_len - (saber.radius/2));
			VectorCopy( blade_muz, &saber.origin );
			VectorCopy( &blade_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			//[RGBSabers]
			if(color < SABER_RGB /*&& color != SABER_WHITE*/)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * 1.0f;
			else
			{
				for(i=0;i<3;i++)
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			//[/RGBSabers]

			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( blade_muz, blade_len, &blade_dir, &saber.origin );
		VectorMA( blade_muz, -1, &blade_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		//[RGBSabers]
		if(color < SABER_RGB /*&& color != SABER_WHITE*/)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for(i=0;i<3;i++)
				saber.shaderRGBA[i] = rgb.data[i];
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if(color >= SABER_RGB /*|| color == SABER_WHITE*/)
		{// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			sbak.reType = RT_LINE;
			sbak.shaderTexCoord.x = sbak.shaderTexCoord.y = 1.0f;
			sbak.shaderRGBA[0] = sbak.shaderRGBA[1] = sbak.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
		//[/RGBSabers]
	}

	{
		saber.renderfx = rfx;
		if(trail_len-((effectradius*AngleScale)/2) > 0)
		{
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (trail_len - (saber.radius/2));
			VectorCopy( trail_muz, &saber.origin );
			VectorCopy( &trail_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			//[RGBSabers]
			if(color < SABER_RGB /*&& color != SABER_WHITE*/)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * effectalpha;
			else
			{
				for(i=0;i<3;i++)
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			//[/RGBSabers]

			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( trail_muz, trail_len, &trail_dir, &saber.origin );
		VectorMA( trail_muz, -1, &trail_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		//[RGBSabers]
		if(color < SABER_RGB /*&& color != SABER_WHITE*/)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for(i=0;i<3;i++)
				saber.shaderRGBA[i] = rgb.data[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if(color >= SABER_RGB /*|| color == SABER_WHITE*/)
		{// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			sbak.reType = RT_LINE;
			sbak.shaderTexCoord.x = sbak.shaderTexCoord.y = 1.0f;
			sbak.shaderRGBA[0] = sbak.shaderRGBA[1] = sbak.shaderRGBA[2] = sbak.shaderRGBA[3] = 0xff;
			sbak.radius = coreradius;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
		//[/RGBSabers]

	}

	VectorMA( blade_muz, blade_len - 0.5, &blade_dir, blade_tip );
	VectorMA( trail_muz, trail_len - 0.5, &trail_dir, trail_tip );

	if(base_len > 2)
	{
		saber.renderfx = rfx;
		if(base_len-(effectradius*AngleScale) > 0)
		{
			saber.radius = effectradius*AngleScale;
			saber.saberLength = (base_len - (effectradius*AngleScale));
			VectorMA( blade_muz, ((effectradius*AngleScale)/2), &base_dir, &saber.origin );
			VectorCopy( &base_dir, &saber.axis[0] );
			saber.reType = RT_SABER_GLOW;
			saber.customShader = glow;
			//[RGBSabers]
			if(color < SABER_RGB /*&& color != SABER_WHITE*/)
				saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff * effectalpha;
			else
			{
				for(i=0;i<3;i++)
					saber.shaderRGBA[i] = rgb.data[i] * effectalpha;
				saber.shaderRGBA[3] = 255 * effectalpha;
			}
			//[/RGBSabers]
			SE_R_AddRefEntityToScene( &saber, cnum );
		}

		// Do the hot core
		VectorMA( blade_muz, base_len, &base_dir, &saber.origin );
		VectorMA( blade_muz, -0.1, &base_dir, &saber.oldorigin );

		saber.customShader = sfxSaberBladeShader;
		saber.reType = RT_LINE;

		saber.radius = coreradius;
		saber.saberLength = base_len;

		saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
		//[RGBSabers]
		if(color < SABER_RGB /*&& color != SABER_WHITE*/)
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
		else
		{
			for(i=0;i<3;i++)
				saber.shaderRGBA[i] = rgb.data[i];
			saber.shaderRGBA[3] = 255;
		}
		sbak = saber;
		SE_R_AddRefEntityToScene( &saber, cnum );

		if(color >= SABER_RGB /*|| color == SABER_WHITE*/)
		{// Add the saber surface that provides color.

			sbak.customShader = sfxSaberBlade2Shader;
			saber.reType = RT_LINE;
			saber.shaderTexCoord.x = saber.shaderTexCoord.y = 1.0f;
			saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
			saber.radius = coreradius;
			saber.saberLength = base_len;
			SE_R_AddRefEntityToScene( &sbak, cnum );
		}
		//[/RGBSabers]
	}
}
//[/SFXSabers]

const char *SaberColorToString( saber_colors_t color )
{
	if ( color == SABER_RED )		return "red";
	if ( color == SABER_ORANGE )	return "orange";
	if ( color == SABER_YELLOW )	return "yellow";
	if ( color == SABER_GREEN )		return "green";
	if ( color == SABER_BLUE )		return "blue";
	if ( color == SABER_PURPLE )	return "purple";
	//[RGBSabers]
	if ( color == SABER_RGB )		return "rgb";
	if ( color == SABER_FLAME1 )	return "rgb2";
	if ( color == SABER_ELEC1 )		return "rgb3";
	if ( color == SABER_FLAME2 )	return "rgb4";
	if ( color == SABER_ELEC2 )		return "rgb5";
	if ( color == SABER_BLACK )		return "black";
	//[/RGBSabers]

	return NULL;
}
saber_colors_t TranslateSaberColor( const char *name ) 
{
	if ( !Q_stricmp( name, "red" ) )		return SABER_RED;
	if ( !Q_stricmp( name, "orange" ) )		return SABER_ORANGE;
	if ( !Q_stricmp( name, "yellow" ) )		return SABER_YELLOW;
	if ( !Q_stricmp( name, "green" ) )		return SABER_GREEN;
	if ( !Q_stricmp( name, "blue" ) )		return SABER_BLUE;
	if ( !Q_stricmp( name, "purple" ) )		return SABER_PURPLE;
	//[RGBSabers]
	if ( !Q_stricmp( name, "rgb" ) )		return SABER_RGB;
	if ( !Q_stricmp( name, "rgb2" ) )		return SABER_FLAME1;
	if ( !Q_stricmp( name, "rgb3" ) )		return SABER_ELEC1;
	if ( !Q_stricmp( name, "rgb4" ) )		return SABER_FLAME2;
	if ( !Q_stricmp( name, "rgb5" ) )		return SABER_ELEC2;
	if ( !Q_stricmp( name, "black" ) )		return SABER_BLACK;
	//[/RGBSabers]
	if ( !Q_stricmp( name, "random" ) )		return ((saber_colors_t)(Q_irand( SABER_ORANGE, SABER_PURPLE )));
	return SABER_BLUE;
}

saberType_t TranslateSaberType( const char *name ) 
{
	if ( !Q_stricmp( name, "SABER_SINGLE" ) )		return SABER_SINGLE;
	if ( !Q_stricmp( name, "SABER_STAFF" ) )		return SABER_STAFF;
	if ( !Q_stricmp( name, "SABER_BROAD" ) )		return SABER_BROAD;
	if ( !Q_stricmp( name, "SABER_PRONG" ) )		return SABER_PRONG;
	if ( !Q_stricmp( name, "SABER_DAGGER" ) )		return SABER_DAGGER;
	if ( !Q_stricmp( name, "SABER_ARC" ) )			return SABER_ARC;
	if ( !Q_stricmp( name, "SABER_SAI" ) )			return SABER_SAI;
	if ( !Q_stricmp( name, "SABER_CLAW" ) )			return SABER_CLAW;
	if ( !Q_stricmp( name, "SABER_LANCE" ) )		return SABER_LANCE;
	if ( !Q_stricmp( name, "SABER_STAR" ) )			return SABER_STAR;
	if ( !Q_stricmp( name, "SABER_TRIDENT" ) )		return SABER_TRIDENT;
	if ( !Q_stricmp( name, "SABER_SITH_SWORD" ) )	return SABER_SITH_SWORD;
	return SABER_SINGLE;
}

#if 0
void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vector3 *origin, vector3 *angles, int bladeNum )
{

	char bladeColorString[MAX_QPATH];
	saber_colors_t bladeColor;
	float bladeLength,bladeRadius;
	vector3	bladeOrigin={0};
	vector3	axis[3]={{0}};
//	vector3	angles={0};
	mdxaBone_t	boltMatrix;
	qboolean tagHack = qfalse;
	char *tagName;
	int bolt;
	float scale;
	//[RGBSabers]
	int snum;
	//[/RGBSabers]

	if ( (item->flags&ITF_ISSABER) && saberModel < 2 )
	{
		//[RGBSabers]
		snum = 0;
		trap->Cvar_VariableStringBuffer("ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else//if ( item->flags&ITF_ISSABER2 ) - presumed
	{
		snum = 1;
		//[/RGBSabers]
		trap->Cvar_VariableStringBuffer("ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}

	if ( !trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),saberModel) )
	{//invalid index!
		return;
	}

	bladeColor = TranslateSaberColor( bladeColorString );

	bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );

	tagName = va( "*blade%d", bladeNum+1 );
	bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, tagName );
	
	if ( bolt == -1 )
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, "*flash" );
		if ( bolt == -1 )
		{//no tag_flash either?!!
			bolt = 0;
		}
	}
	
//	angles.pitch = curYaw;
//	angles.roll = 0;

	trap->G2API_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bladeOrigin);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
																//		...changed this back to NEGATIVE_Y		
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, axis[1]);//right ... and changed this to NEGATIVE_X
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, axis[2]);//up

	// Where do I get scale from?
//	scale = DC->xscale;
	scale = 1.0f;

	if ( tagHack )
	{
		switch ( saberType )
		{
		default:
		case SABER_SINGLE:
				VectorMA( bladeOrigin, scale, axis[0], bladeOrigin );
			break;
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			if ( bladeNum == 1 )
			{
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -1*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 1*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_ARC:
			VectorSubtract( axis[1], axis[2], axis[1] );
			VectorNormalize( axis[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], 0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 1:
				VectorScale( axis[0], 0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 2:
				VectorMA( bladeOrigin, -8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 3:
				VectorMA( bladeOrigin, -16*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[1], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, -2*scale, axis[1], bladeOrigin );
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 4:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 5:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, -6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorMA( bladeOrigin, -32*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -1, axis[0] );
				break;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		}
	}
	if ( saberType == SABER_SITH_SWORD )
	{//draw no blade
		return;
	}

	//[RGBSabers]
	UI_DoSaber( bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum );
	//[/RGBSabers]

}
#endif
void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vector3 *origin, vector3 *angles, int bladeNum )
{
	vector3	org_, end,
	axis_[3] = {{0.0f}};	// shut the compiler up
	mdxaBone_t	boltMatrix;
	vector3 futureAngles;
	effectTrailArgStruct_t fx;
	int	useModelIndex = 0;
	saber_colors_t bladeColor;
	float bladeLength,bladeRadius;
	char bladeColorString[MAX_QPATH];
	//[RGBSabers]
	int snum;
	//[/RGBSabers]
	char *tagName;
	int bolt;
	qboolean tagHack = qfalse;
	int styleToUse = atoi(UI_Cvar_VariableString( "cg_saberBladeStyle" ));


	if ( (item->flags&ITF_ISSABER) && saberModel < 2 )
	{
		//[RGBSabers]
		snum = 0;
		trap->Cvar_VariableStringBuffer("ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else//if ( item->flags&ITF_ISSABER2 ) - presumed
	{
		snum = 1;
		//[/RGBSabers]
		trap->Cvar_VariableStringBuffer("ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}

	if ( !trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),saberModel) )
	{//invalid index!
		return;
	}

	bladeColor = TranslateSaberColor( bladeColorString );

	bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );

	futureAngles.pitch	= angles->pitch;
	futureAngles.yaw	= angles->yaw;
	futureAngles.roll	= angles->roll;

	useModelIndex = 0;

	tagName = va( "*blade%d", bladeNum+1 );
	bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, tagName );
	
	if ( bolt == -1 )
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = trap->G2API_AddBolt( item->ghoul2,saberModel, "*flash" );
		if ( bolt == -1 )
		{//no tag_flash either?!!
			bolt = 0;
		}
	}
	
//	angles.pitch = curYaw;
//	angles.roll = 0;

	trap->G2API_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, &vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, &org_);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, &axis_[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
																//		...changed this back to NEGATIVE_Y		
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, &axis_[1]);//right ... and changed this to NEGATIVE_X
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, &axis_[2]);//up

	VectorMA( &org_, bladeLength, &axis_[0], &end );

	VectorAdd( &end, &axis_[0], &end );

	if ( tagHack )
	{
		switch ( saberType )
		{
		default:
		case SABER_SINGLE:
				VectorMA( &org_, 1.0f, &axis_[0], &org_ );
			break;
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 0 )
				VectorMA( &org_, 12*1.0f, &axis_[0], &org_ );

			if ( bladeNum == 1 ) {
				VectorScale( &axis_[0], -1, &axis_[0] );
				VectorMA( &org_, 12*1.0f, &axis_[0], &org_ );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
				VectorMA( &org_, -1*1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 1 )
				VectorMA( &org_, 1*1.0f, &axis_[1], &org_ );
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
				VectorMA( &org_, -3*1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 1 )
				VectorMA( &org_, 3*1.0f, &axis_[1], &org_ );
			break;
		case SABER_ARC:
			VectorSubtract( &axis_[1], &axis_[2], &axis_[1] );
			VectorNormalize( &axis_[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
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
				VectorMA( &org_, -8*1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -0.25f, &axis_[0] );
				VectorScale( &axis_[1], 0.75f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			case 3:
				VectorMA( &org_, -16*1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -0.75f, &axis_[0] );
				VectorScale( &axis_[1], 0.25f, &axis_[1] );
				VectorAdd( &axis_[0], &axis_[1], &axis_[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
				VectorMA( &org_, -3*1.0f, &axis_[1], &org_ );
			else if ( bladeNum == 2 )
				VectorMA( &org_, 3*1.0f, &axis_[1], &org_ );
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( &org_, 2*1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2*1.0f, &axis_[2], &org_ );
				break;
			case 1:
				VectorMA( &org_, 2*1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2*1.0f, &axis_[2], &org_ );
				VectorMA( &org_, 2*1.0f, &axis_[1], &org_ );
				break;
			case 2:
				VectorMA( &org_, 2*1.0f, &axis_[0], &org_ );
				VectorMA( &org_, 2*1.0f, &axis_[2], &org_ );
				VectorMA( &org_, -2*1.0f, &axis_[1], &org_ );
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			case 1:
				VectorScale( &axis_[0], 0.33f, &axis_[0] );
				VectorScale( &axis_[2], 0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			case 2:
				VectorScale( &axis_[0], -0.33f, &axis_[0] );
				VectorScale( &axis_[2], 0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			case 3:
				VectorScale( &axis_[0], -1, &axis_[0] );
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			case 4:
				VectorScale( &axis_[0], -0.33f, &axis_[0] );
				VectorScale( &axis_[2], -0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			case 5:
				VectorScale( &axis_[0], 0.33f, &axis_[0] );
				VectorScale( &axis_[2], -0.67f, &axis_[2] );
				VectorAdd( &axis_[0], &axis_[2], &axis_[0] );
				VectorMA( &org_, 8*1.0f, &axis_[0], &org_ );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( &org_, 24*1.0f, &axis_[0], &org_ );
				break;
			case 1:
				VectorMA( &org_, -6*1.0f, &axis_[1], &org_ );
				VectorMA( &org_, 24*1.0f, &axis_[0], &org_ );
				break;
			case 2:
				VectorMA( &org_, 6*1.0f, &axis_[1], &org_ );
				VectorMA( &org_, 24*1.0f, &axis_[0], &org_ );
				break;
			case 3:
				VectorMA( &org_, -32*1.0f, &axis_[0], &org_ );
				VectorScale( &axis_[0], -1, &axis_[0] );
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
	switch( styleToUse )
	{
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

void UI_GetSaberForMenu( char *saber, int saberNum )
{
	char saberTypeString[MAX_QPATH]={0};
	saberType_t saberType = SABER_NONE;

	if ( saberNum == 0 )
	{
		trap->Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap->Cvar_Set( "ui_saber", "kyle" );
			trap->Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		}
	}
	else
	{
		trap->Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap->Cvar_Set( "ui_saber2", "kyle" );
			trap->Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		}
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber( saber, saberTypeString );
	if ( saberTypeString[0] )
	{
		saberType = TranslateSaberType( saberTypeString );
	}

	switch ( uiInfo.movesTitleIndex )
	{
	case 0://MD_ACROBATICS:
		break;
	case 1://MD_SINGLE_FAST:
	case 2://MD_SINGLE_MEDIUM:
	case 3://MD_SINGLE_STRONG:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 4://MD_DUAL_SABERS:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 5://MD_SABER_STAFF:
		if ( saberType == SABER_SINGLE || saberType == SABER_NONE )
		{
			Q_strncpyz(saber,"dual_1",MAX_QPATH);
		}
		break;
	}

}

void UI_SaberDrawBlades( itemDef_t *item, vector3 *origin, vector3 *angles )
{
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	int saberNum = 0;
	int saberModel = 0;
	int	numSabers = 1;

	if ( (item->flags&ITF_ISCHARACTER)//hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		if ( (item->flags&ITF_ISCHARACTER) )//hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu( saber, saberNum );
			saberModel = saberNum + 1;
		}
		else if ( (item->flags&ITF_ISSABER) )
		{
			trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap->Cvar_Set( "ui_saber", "kyle" );
				trap->Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else if ( (item->flags&ITF_ISSABER2) )
		{
			trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap->Cvar_Set( "ui_saber2", "kyle" );
				trap->Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else
		{
			return;
		}
		if ( saber[0] )
		{
			saberType_t saberType;
			int curBlade = 0;
			int numBlades = UI_SaberNumBladesForSaber( saber );
			if ( numBlades )
			{//okay, here we go, time to draw each blade...
				char	saberTypeString[MAX_QPATH]={0};
				UI_SaberTypeForSaber( saber, saberTypeString );
				saberType = TranslateSaberType( saberTypeString );
				for ( curBlade = 0; curBlade < numBlades; curBlade++ )
				{
					if ( UI_SaberShouldDrawBlade( saber, curBlade ) )
					{
						UI_SaberDrawBlade( item, saber, saberModel, saberType, origin, angles, curBlade );
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar( itemDef_t *item )
{
	int	numSabers = 1;
 	int	saberNum = 0;

	if ( trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),2) )
	{//remove any extra models
		trap->G2API_RemoveGhoul2Model(&(item->ghoul2), 2);
	}
	if ( trap->G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),1) )
	{//remove any extra models
		trap->G2API_RemoveGhoul2Model(&(item->ghoul2), 1);
	}

	if ( uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		//bolt sabers
		char modelPath[MAX_QPATH];
		char skinPath[MAX_QPATH];
		char saber[MAX_QPATH]; 

		UI_GetSaberForMenu( saber, saberNum );

		//RAZTODO: UI_SaberAttachToChar is different in JA+
		if ( UI_SaberModelForSaber( saber, modelPath ) )
		{//successfully found a model
			int g2Saber = trap->G2API_InitGhoul2Model( &(item->ghoul2), modelPath, 0, 0, 0, 0, 0 ); //add the model
			if ( g2Saber )
			{
				int boltNum;
				//get the customSkin, if any
				if ( UI_SaberSkinForSaber( saber, skinPath ) )
				{
					int g2skin = trap->R_RegisterSkin(skinPath);
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, g2skin );//this is going to set the surfs on/off matching the skin file
				}
				else
				{
					trap->G2API_SetSkin( item->ghoul2, g2Saber, 0, 0 );//turn off custom skin
				}
				if ( saberNum == 0 )
				{
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*r_hand");
				}
				else
				{
					boltNum = trap->G2API_AddBolt( item->ghoul2, 0, "*l_hand");
				}
				trap->G2API_AttachG2Model( item->ghoul2, g2Saber, item->ghoul2, boltNum, 0);
			}
		}
	}
}

// Fill in with saber hilts
void UI_SaberGetHiltInfo( const char *singleHilts[MAX_SABER_HILTS], const char *staffHilts[MAX_SABER_HILTS] )
{
	int	numSingleHilts = 0, numStaffHilts = 0;
	const char	*saberName;
	const char	*token;
	const char	*p;

	//go through all the loaded sabers and put the valid ones in the proper list
	p = SaberParms;
	COM_BeginParseSession("saberlist");

	// look for a saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{//invalid name
			continue;
		}
		saberName = String_Alloc( token );
		//see if there's a "{" on the next line
		SkipRestOfLine( &p );

		if ( UI_ParseLiteralSilent( &p, "{" ) ) 
		{//nope, not a name, keep looking
			continue;
		}

		//this is a saber name
		if ( !UI_SaberValidForPlayerInMP( saberName ) )
		{
			SkipBracedSection( &p );
			continue;
		}

		if ( UI_IsSaberTwoHanded( saberName ) )
		{
			if ( numStaffHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				staffHilts[numStaffHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many two-handed sabers, ignoring saber '%s'\n", saberName );
			}
		}
		else
		{
			if ( numSingleHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				singleHilts[numSingleHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many one-handed sabers, ignoring saber '%s'\n", saberName );
			}
		}
		//skip the whole braced section and move on to the next entry
		SkipBracedSection( &p );
	}
	//null terminate the list so the UI code knows where to stop listing them
	singleHilts[numSingleHilts] = NULL;
	staffHilts[numStaffHilts] = NULL;
}

