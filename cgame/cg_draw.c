// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "bg_saga.h"
#include "ui/ui_shared.h"
#include "ui/ui_public.h"
#include "cg_luaevent.h"
#include "cg_media.h"

extern float CG_RadiusForCent( centity_t *cent );
qboolean CG_CalcMuzzlePoint( int entityNum, vector3 *muzzle );
static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam);
static void CG_DrawSiegeDeathTimer( int timeRemaining );
// nmckenzie: DUEL_HEALTH
void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist );

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

int lastvalidlockdif;

extern float zoomFov; //this has to be global client-side

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

// The time at which you died and the time it will take for you to rejoin game.
int cg_siegeDeathTime = 0;

#define MAX_HUD_TICS 4
const char *armorTicName[MAX_HUD_TICS]		=	{	"armor_tic1",	"armor_tic2",	"armor_tic3",	"armor_tic4",	};
const char *healthTicName[MAX_HUD_TICS]		=	{	"health_tic1",	"health_tic2",	"health_tic3",	"health_tic4",	};
const char *forceTicName[MAX_HUD_TICS]		=	{	"force_tic1",	"force_tic2",	"force_tic3",	"force_tic4",	};
const char *ammoTicName[MAX_HUD_TICS]		=	{	"ammo_tic1",	"ammo_tic2",	"ammo_tic3",	"ammo_tic4",	};

typedef struct saberStyleItem_s {
	const char *name;
	const vector4 *colour;
} saberStyleItem_t;

saberStyleItem_t saberStyleItems[] = {
	{ "None",	&colorTable[CT_WHITE] },
	{ "Fast",	&colorTable[CT_LTBLUE2] },
	{ "Medium",	&colorTable[CT_YELLOW] },
	{ "Strong",	&colorTable[CT_RED] },
	{ "Desann",	&colorTable[CT_HUD_ORANGE] },
	{ "Tavion",	&colorTable[CT_LTPURPLE1] },
	{ "Dual",	&colorTable[CT_YELLOW] },
	{ "Staff",	&colorTable[CT_YELLOW] }
};

const char *showPowersName[] =
{
	"HEAL2",//FP_HEAL
	"JUMP2",//FP_LEVITATION
	"SPEED2",//FP_SPEED
	"PUSH2",//FP_PUSH
	"PULL2",//FP_PULL
	"MINDTRICK2",//FP_TELEPTAHY
	"GRIP2",//FP_GRIP
	"LIGHTNING2",//FP_LIGHTNING
	"DARK_RAGE2",//FP_RAGE
	"PROTECT2",//FP_PROTECT
	"ABSORB2",//FP_ABSORB
	"TEAM_HEAL2",//FP_TEAM_HEAL
	"TEAM_REPLENISH2",//FP_TEAM_FORCE
	"DRAIN2",//FP_DRAIN
	"SEEING2",//FP_SEE
	"SABER_OFFENSE2",//FP_SABER_OFFENSE
	"SABER_DEFENSE2",//FP_SABER_DEFENSE
	"SABER_THROW2",//FP_SABERTHROW
	NULL
};

//Called from UI shared code. For now we'll just redirect to the normal anim load function.


int UI_ParseAnimationFile(const char *filename, animation_t *animset, qboolean isHumanoid)
{
	return BG_ParseAnimationFile(filename, animset, isHumanoid);
}

int MenuFontToHandle( int iMenuFont ) {
	switch ( iMenuFont ) {
		case FONT_SMALL:		return cgDC.Assets.qhSmallFont;
		case FONT_SMALL2:		return cgDC.Assets.qhSmall2Font;
		case FONT_MEDIUM:		return cgDC.Assets.qhMediumFont;
		case FONT_LARGE:		return cgDC.Assets.qhMediumFont;//cgDC.Assets.qhBigFont;
			//fixme? Big fonr isn't registered...?
		//Raz: fonts
		case FONT_JAPPLARGE:	return cgDC.Assets.japp.fontLarge;
		case FONT_JAPPSMALL:	return cgDC.Assets.japp.fontSmall;
		case FONT_JAPPMONO:		return cgDC.Assets.japp.fontMono;
		default:
			break;
	}

	return cgDC.Assets.qhMediumFont;
}

float CG_Text_Width(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_StrLenPixels(text, iFontIndex, scale);
}

float CG_Text_Height(const char *text, float scale, int iMenuFont)
{
	int iFontIndex = MenuFontToHandle(iMenuFont);

	return trap->R_Font_HeightPixels(iFontIndex, scale);
}

#include "qcommon/qfiles.h"	// for STYLE_BLINK etc
void CG_Text_Paint( float x, float y, float scale, const vector4 *color, const char *text, float adjust, int limit, int style, int iMenuFont ) {
	int iStyleOR = 0;
	int iFontIndex = MenuFontToHandle( iMenuFont );

	switch ( style ) {
	case  ITEM_TEXTSTYLE_NORMAL:			iStyleOR = 0;break;					// JK2 normal text
	case  ITEM_TEXTSTYLE_BLINK:				iStyleOR = STYLE_BLINK;break;		// JK2 fast blinking
	case  ITEM_TEXTSTYLE_PULSE:				iStyleOR = STYLE_BLINK;break;		// JK2 slow pulsing
	case  ITEM_TEXTSTYLE_SHADOWED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINED:			iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_OUTLINESHADOWED:	iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	case  ITEM_TEXTSTYLE_SHADOWEDMORE:		iStyleOR = (int)STYLE_DROPSHADOW;break;	// JK2 drop shadow ( need a color for this )
	default:
		break;
	}

	trap->R_Font_DrawString(	x,		// int ox
							y,		// int oy
							text,	// const char *text
							color,	// paletteRGBA_c c
							iStyleOR | iFontIndex,	// const int iFontHandle
							!limit?-1:limit,		// iCharLimit (-1 = none)
							scale	// const float scale = 1.0f
							);
}

/*
================
CG_DrawZoomMask

================
*/
static void CG_DrawZoomMask( void )
{
	vector4		color1;
	float		level;
	static qboolean	flip = qtrue;
	refdef_t *refdef = CG_GetRefdef();

//	int ammo = cg_entities[0].gent->client->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex];
	float cx, cy;
//	int val[5];
	float max, fi;

	// Check for Binocular specific zooming since we'll want to render different bits in each case
	if ( cg.predictedPlayerState.zoomMode == 2 )
	{
		int val, i;
		float off;

		// zoom level
		level = (float)(80.0f - cg.predictedPlayerState.zoomFov) / 80.0f;

		// ...so we'll clamp it
		if ( level < 0.0f )
		{
			level = 0.0f;
		}
		else if ( level > 1.0f )
		{
			level = 1.0f;
		}

		// Using a magic number to convert the zoom level to scale amount
		level *= 162.0f;

		// draw blue tinted distortion mask, trying to make it as small as is necessary to fill in the viewable area
		trap->R_SetColor( &colorTable[CT_WHITE] );
		CG_DrawPic( 34, 48, 570, 362, media.gfx.interface.binoculars.staticMask );

		// Black out the area behind the numbers
		trap->R_SetColor( &colorTable[CT_BLACK]);
		CG_DrawPic( 212, 367, 200, 40, media.gfx.world.whiteShader );

		// Numbers should be kind of greenish
		VectorSet4( &color1, 0.2f, 0.4f, 0.2f, 0.3f );
		trap->R_SetColor( &color1 );

		// Draw scrolling numbers, use intervals 10 units apart--sorry, this section of code is just kind of hacked
		//	up with a bunch of magic numbers.....
		val = ((int)((refdef->viewangles.yaw + 180) / 10)) * 10;
		off = (refdef->viewangles.yaw + 180) - val;

		for ( i = -10; i < 30; i += 10 )
		{
			val -= 10;

			if ( val < 0 )
			{
				val += 360;
			}

			// we only want to draw the very far left one some of the time, if it's too far to the left it will
			//	poke outside the mask.
			if (( off > 3.0f && i == -10 ) || i > -10 )
			{
				// draw the value, but add 200 just to bump the range up...arbitrary, so change it if you like
				CG_DrawNumField( 155 + i * 10 + off * 10, 374, 3, val + 200, 24, 14, NUM_FONT_CHUNKY, qtrue );
				CG_DrawPic( 245 + (i-1) * 10 + off * 10, 376, 6, 6, media.gfx.world.whiteShader );
			}
		}

		CG_DrawPic( 212, 367, 200, 28, media.gfx.interface.binoculars.overlay );

		color1.r = sin( cg.time * 0.01f ) * 0.5f + 0.5f;
		color1.r = color1.g = color1.b = color1.r*color1.r;
		color1.a = 1.0f;

		trap->R_SetColor( &color1 );

		CG_DrawPic( 82, 94, 16, 16, media.gfx.interface.binoculars.circle );

		// Flickery color
		color1.r = 0.7f + crandom() * 0.1f;
		color1.g = 0.8f + crandom() * 0.1f;
		color1.b = 0.7f + crandom() * 0.1f;
		color1.a = 1.0f;
		trap->R_SetColor( &color1 );

		CG_DrawPic( 0, 0, 640, 480, media.gfx.interface.binoculars.mask );

		CG_DrawPic( 4, 282 - level, 16, 16, media.gfx.interface.binoculars.arrow );

		// The top triangle bit randomly flips
		if ( flip )
			CG_DrawPic( 330, 60, -26, -30, media.gfx.interface.binoculars.tri );
		else
			CG_DrawPic( 307, 40,  26,  30, media.gfx.interface.binoculars.tri );

		if ( random() > 0.98f && ( cg.time & 1024 ))
			flip = !flip;
	}
	else if ( cg.predictedPlayerState.zoomMode)
	{
		// disruptor zoom mode
		level = (float)(50.0f - zoomFov) / 50.0f;//(float)(80.0f - zoomFov) / 80.0f;

		// ...so we'll clamp it
		level = Q_clamp( 0.0f, level, 1.0f );

		// Using a magic number to convert the zoom level to a rotation amount that correlates more or less with the zoom artwork.
		level *= 103.0f;

		// Draw target mask
		trap->R_SetColor( &colorTable[CT_WHITE] );
		CG_DrawPic( 0, 0, 640, 480, media.gfx.interface.disruptor.mask );

		// apparently 99.0f is the full zoom level
		if ( level >= 99 )
		{
			// Fully zoomed, so make the rotating insert pulse
			VectorSet4( &color1, 1.0f, 1.0f, 1.0f, 0.7f + sin( cg.time * 0.01f )*0.3f );

			trap->R_SetColor( &color1 );
		}

		// Draw rotating insert
		CG_DrawRotatePic2( 320, 240, 640, 480, -level, media.gfx.interface.disruptor.insert );

		// Increase the light levels under the center of the target
//		CG_DrawPic( 198, 118, 246, 246, media.gfx.interface.disruptor.light );

		// weirdness.....converting ammo to a base five number scale just to be geeky.
/*		val[0] = ammo % 5;
		val[1] = (ammo / 5) % 5;
		val[2] = (ammo / 25) % 5;
		val[3] = (ammo / 125) % 5;
		val[4] = (ammo / 625) % 5;

		color1[0] = 0.2f;
		color1[1] = 0.55f + crandom() * 0.1f;
		color1[2] = 0.5f + crandom() * 0.1f;
		color1[3] = 1.0f;
		trap->R_SetColor( color1 );

		for ( int t = 0; t < 5; t++ )
		{
			cx = 320 + sin( (t*10+45)/57.296f ) * 192;
			cy = 240 + cos( (t*10+45)/57.296f ) * 192;

			CG_DrawRotatePic2( cx, cy, 24, 38, 45 - t * 10, trap->R_RegisterShader( va("gfx/2d/char%d",val[4-t] )));
		}
*/
		//max = ( cg_entities[0].gent->health / 100.0f );


		if ( (cg.snap->ps.eFlags & EF_DOUBLE_AMMO) )
			max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / ((float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max*2.0f);
		else
			max = cg.snap->ps.ammo[weaponData[WP_DISRUPTOR].ammoIndex] / (float)ammoData[weaponData[WP_DISRUPTOR].ammoIndex].max;
		if ( max > 1.0f )
			max = 1.0f;

		VectorSet4( &color1, (1.0f - max) * 2.0f, max * 1.5f, 0.0f, 1.0f );

		// If we are low on health, make us flash
		if ( max < 0.15f && ( cg.time & 512 ))
			VectorClear4( &color1 );

		if ( color1.r > 1.0f )
			color1.r = 1.0f;

		if ( color1.r > 1.0f )
			color1.r = 1.0f;

		trap->R_SetColor( &color1 );

		max *= 58.0f;

		for (fi = 18.5f; fi <= 18.5f + max; fi+= 3 ) // going from 15 to 45 degrees, with 5 degree increments
		{
			cx = 320 + sin( (fi+90.0f)/57.296f ) * 190;
			cy = 240 + cos( (fi+90.0f)/57.296f ) * 190;

			CG_DrawRotatePic2( cx, cy, 12, 24, 90 - fi, media.gfx.interface.disruptor.insertTick );
		}

		if ( cg.predictedPlayerState.weaponstate == WEAPON_CHARGING_ALT )
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );

			// draw the charge level
			max = ( cg.time - cg.predictedPlayerState.weaponChargeTime ) / ( 50.0f * 30.0f ); // bad hardcodedness 50 is disruptor charge unit and 30 is max charge units allowed.

			if ( max > 1.0f )
				max = 1.0f;

			trap->R_DrawStretchPic(257, 435, 134*max, 34, 0, 0, max, 1, media.gfx.interface.disruptor.charge);
		}
//		trap->R_SetColor( colorTable[CT_WHITE] );
//		CG_DrawPic( 0, 0, 640, 480, media.gfx.interface.disruptor.mask );

	}
}


/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, void *ghoul2, int g2radius, qhandle_t skin, vector3 *origin, vector3 *angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3DIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, &ent.origin );
	ent.hModel = model;
	ent.ghoul2 = ghoul2;
	ent.radius = g2radius;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap->R_ClearScene();
	SE_R_AddRefEntityToScene( &ent, MAX_CLIENTS );
	trap->R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vector3 *headAngles )
{
	clientInfo_t	*ci;

	if (clientNum >= MAX_CLIENTS)
	{ //npc?
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	CG_DrawPic( x, y, w, h, ci->modelIcon );

	// if they are deferred, draw a cross out
	if ( ci->deferred )
	{
		CG_DrawPic( x, y, w, h, media.gfx.interface.defer );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vector3			origin, angles;
	vector3			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3DIcons.integer ) {

		x *= cgs.screenXScale;
		y *= cgs.screenYScale;
		w *= cgs.screenXScale;
		h *= cgs.screenYScale;

		VectorClear( &angles );

		cm = media.models.redFlag;

		// offset the origin y and z to center the flag
		trap->R_ModelBounds( cm, &mins, &maxs );

		origin.z = -0.5 * ( mins.z + maxs.z );
		origin.y = 0.5 * ( mins.y + maxs.y );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs.z - mins.z );
		origin.x = len / 0.268;	// len / tan( fov/2 )

		angles.yaw = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = media.models.redFlag;
		} else if( team == TEAM_BLUE ) {
			handle = media.models.blueFlag;
		} else if( team == TEAM_FREE ) {
			handle = 0;//media.models.neutralFlag;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, NULL, 0, 0, &origin, &angles );
	} else if ( cg_drawIcons.integer ) {
		const gitem_t *item = NULL;

			 if ( team == TEAM_RED )	item = BG_FindItemForPowerup( PW_REDFLAG );
		else if ( team == TEAM_BLUE )	item = BG_FindItemForPowerup( PW_BLUEFLAG );
		else if ( team == TEAM_FREE )	item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		else							return;

		if ( item )
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
	}
}

/*
================
CG_DrawHealth
================
*/
void CG_DrawHealth( menuDef_t *menuHUD )
{
	vector4			calcColor;
	playerState_t	*ps;
	int				healthAmt;
	int				i,currValue,inc;
	itemDef_t		*focusItem;
	float percent;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	ps = &cg.snap->ps;

	// What's the health?
	healthAmt = ps->stats[STAT_HEALTH];
	if (healthAmt > ps->stats[STAT_MAX_HEALTH])
	{
		healthAmt = ps->stats[STAT_MAX_HEALTH];
	}


	inc = (float) ps->stats[STAT_MAX_HEALTH] / MAX_HUD_TICS;
	currValue = healthAmt;

	// Print the health tics, fading out the one which is partial health
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, healthTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy( &calcColor, &colorTable[CT_WHITE], sizeof(calcColor) );

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor.a *= percent;		// Fade it out
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		currValue -= inc;
	}

	// Print the mueric amount
	focusItem = Menu_FindItemByName(menuHUD, "healthamount");
	if (focusItem)
	{
		// Print health amount
		trap->R_SetColor( &focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			ps->stats[STAT_HEALTH],
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}

}

/*
================
CG_DrawArmor
================
*/
void CG_DrawArmor( menuDef_t *menuHUD )
{
	vector4			calcColor;
	playerState_t	*ps;
	int				maxArmor;
	itemDef_t		*focusItem;
	float			percent,quarterArmor;
	int				i,currValue,inc;

	//ps = &cg.snap->ps;
	ps = &cg.predictedPlayerState;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	maxArmor = ps->stats[STAT_MAX_HEALTH];

	currValue = ps->stats[STAT_ARMOR];
	inc = (float) maxArmor / MAX_HUD_TICS;

	memcpy( &calcColor, &colorTable[CT_WHITE], sizeof( calcColor ) );
	for (i=(MAX_HUD_TICS-1);i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, armorTicName[i]);

		if (!focusItem)	// This is bad
		{
			continue;
		}

		memcpy( &calcColor, &colorTable[CT_WHITE], sizeof( calcColor ) );

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			percent = (float) currValue / inc;
			calcColor.a *= percent;
		}

		trap->R_SetColor( &calcColor);

		if ((i==(MAX_HUD_TICS-1)) && (currValue < inc))
		{
			if (cg.HUDArmorFlag)
			{
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
			}
		}
		else
		{
				CG_DrawPic(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					focusItem->window.rect.w,
					focusItem->window.rect.h,
					focusItem->window.background
					);
		}

		currValue -= inc;
	}

	focusItem = Menu_FindItemByName(menuHUD, "armoramount");

	if (focusItem)
	{
		// Print armor amount
		trap->R_SetColor( &focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			ps->stats[STAT_ARMOR],
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}

	// If armor is low, flash a graphic to warn the player
	if (ps->stats[STAT_ARMOR])	// Is there armor? Draw the HUD Armor TIC
	{
		quarterArmor = (float) (ps->stats[STAT_MAX_HEALTH] / 4.0f);

		// Make tic flash if armor is at 25% of full armor
		if (ps->stats[STAT_ARMOR] < quarterArmor)		// Do whatever the flash timer says
		{
			if (cg.HUDTickFlashTime < cg.time)			// Flip at the same time
			{
				cg.HUDTickFlashTime = cg.time + 400;
				if (cg.HUDArmorFlag)
				{
					cg.HUDArmorFlag = qfalse;
				}
				else
				{
					cg.HUDArmorFlag = qtrue;
				}
			}
		}
		else
		{
			cg.HUDArmorFlag=qtrue;
		}
	}
	else						// No armor? Don't show it.
	{
		cg.HUDArmorFlag=qfalse;
	}

}

/*
================
CG_DrawSaberStyle

If the weapon is a light saber (which needs no ammo) then draw a graphic showing
the saber style (fast, medium, strong)
================
*/
static void CG_DrawSaberStyle( centity_t *cent, menuDef_t *menuHUD)
{
	itemDef_t		*focusItem;

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	if ( cent->currentState.weapon != WP_SABER )
	{
		return;
	}

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}


	// draw the current saber style in this window
	switch ( cg.predictedPlayerState.fd.saberDrawAnimLevel )
	{
	case 1://FORCE_LEVEL_1:
	case 5://FORCE_LEVEL_5://Tavion

		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_fast");

		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		break;
	case 2://FORCE_LEVEL_2:
	case 6://SS_DUAL
	case 7://SS_STAFF
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_medium");

		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}
		break;
	case 3://FORCE_LEVEL_3:
	case 4://FORCE_LEVEL_4://Desann
		focusItem = Menu_FindItemByName(menuHUD, "saberstyle_strong");

		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );

			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}
		break;
	default:
		break;
	}

}

/*
================
CG_DrawAmmo
================
*/
static void CG_DrawAmmo( centity_t	*cent,menuDef_t *menuHUD)
{
	playerState_t	*ps;
	int				i;
	vector4			calcColor;
	float			value,inc = 0.0f,percent;
	itemDef_t		*focusItem;

	ps = &cg.snap->ps;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	if (!cent->currentState.weapon ) // We don't have a weapon right now
	{
		return;
	}

	value = ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
	if (value < 0)	// No ammo
	{
		return;
	}

	trap->R_SetColor( &colorTable[CT_WHITE] );

	if (weaponData[cent->currentState.weapon].energyPerShot == 0 &&
		weaponData[cent->currentState.weapon].altEnergyPerShot == 0)
	{ //just draw "infinite"
		inc = 8 / MAX_HUD_TICS;
		value = 8;

		focusItem = Menu_FindItemByName(menuHUD, "ammoinfinite");
		trap->R_SetColor( &colorTable[CT_WHITE] );
		if (focusItem)
		{
			UI_DrawProportionalString(focusItem->window.rect.x, focusItem->window.rect.y, "--", NUM_FONT_SMALL, &focusItem->window.foreColor);
		}
	}
	else
	{
		focusItem = Menu_FindItemByName(menuHUD, "ammoamount");
		trap->R_SetColor( &colorTable[CT_WHITE] );
		if (focusItem)
		{

			if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
			{
				inc = (float) (ammoData[weaponData[cent->currentState.weapon].ammoIndex].max*2.0f) / MAX_HUD_TICS;
			}
			else
			{
				inc = (float) ammoData[weaponData[cent->currentState.weapon].ammoIndex].max / MAX_HUD_TICS;
			}
			value =ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];

			CG_DrawNumField (
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				3,
				value,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				NUM_FONT_SMALL,
				qfalse);
		}
	}

	// Draw tics
	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, ammoTicName[i]);

		if (!focusItem)
		{
			continue;
		}

		memcpy( &calcColor, &colorTable[CT_WHITE], sizeof( calcColor ) );

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			percent = value / inc;
			calcColor.a = percent;
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		value -= inc;
	}

}

/*
================
CG_DrawForcePower
================
*/
void CG_DrawForcePower( menuDef_t *menuHUD )
{
	int				i;
	vector4			calcColor;
	float			value,inc,percent;
	itemDef_t		*focusItem;
	const int		maxForcePower = 100;
	qboolean	flash=qfalse;

	// Can we find the menu?
	if (!menuHUD)
	{
		return;
	}

	// Make the hud flash by setting forceHUDTotalFlashTime above cg.time
	if (cg.forceHUDTotalFlashTime > cg.time )
	{
		flash = qtrue;
		if (cg.forceHUDNextFlashTime < cg.time)
		{
			cg.forceHUDNextFlashTime = cg.time + 400;
			trap->S_StartSound (NULL, 0, CHAN_LOCAL, media.sounds.force.noforce );

			if (cg.forceHUDActive)
			{
				cg.forceHUDActive = qfalse;
			}
			else
			{
				cg.forceHUDActive = qtrue;
			}

		}
	}
	else	// turn HUD back on if it had just finished flashing time.
	{
		cg.forceHUDNextFlashTime = 0;
		cg.forceHUDActive = qtrue;
	}

//	if (!cg.forceHUDActive)
//	{
//		return;
//	}

	inc = (float)  maxForcePower / MAX_HUD_TICS;
	value = cg.snap->ps.fd.forcePower;

	for (i=MAX_HUD_TICS-1;i>=0;i--)
	{
		focusItem = Menu_FindItemByName(menuHUD, forceTicName[i]);

		if (!focusItem)
		{
			continue;
		}

//		memcpy( calcColor, &colorTable[CT_WHITE], sizeof( calcColor ) );

		if ( value <= 0 )	// done
		{
			break;
		}
		else if (value < inc)	// partial tic
		{
			if (flash)
				memcpy(&calcColor,  &colorTable[CT_RED], sizeof(vector4));
			else
				memcpy(&calcColor,  &colorTable[CT_WHITE], sizeof(vector4));

			percent = value / inc;
			calcColor.a = percent;
		}
		else
		{
			if (flash)
				memcpy(&calcColor,  &colorTable[CT_RED], sizeof(vector4));
			else
				memcpy(&calcColor,  &colorTable[CT_WHITE], sizeof(vector4));
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			focusItem->window.background
			);

		value -= inc;
	}

	focusItem = Menu_FindItemByName(menuHUD, "forceamount");

	if (focusItem)
	{
		// Print force amount
		trap->R_SetColor( &focusItem->window.foreColor );

		CG_DrawNumField (
			focusItem->window.rect.x,
			focusItem->window.rect.y,
			3,
			cg.snap->ps.fd.forcePower,
			focusItem->window.rect.w,
			focusItem->window.rect.h,
			NUM_FONT_SMALL,
			qfalse);
	}
}

void JP_DrawMiniScoreboard( void )
{
#ifdef JAPP_MINISCOREBOARD
	int i, rank=0;
	int scoreX1 = cg_hudMiniScoreboardX1.integer, scoreX2 = cg_hudMiniScoreboardX2.integer;
	float scoreY = cg_hudMiniScoreboardY.value, scoreS = cg_hudMiniScoreboardS.value;
	char scoreName[MAX_CLIENTS][64];
	int scoreKills[MAX_CLIENTS], scoreDeaths[MAX_CLIENTS];
	vector4 clrBlack = { 0.0f, 0.0f, 0.0f, cg_hudMiniScoreboardA.value };
	int cap = Server_Supports( SSF_SCOREBOARD_LARGE ) ? MAX_CLIENTS : 20;
	int count = cg_miniScoreboard.integer;

	if ( !cg_miniScoreboard.integer || cgs.gametype != GT_FFA || cg.showScores )
		return;

	if ( count > cap )
		count = cap;

	if ( Server_Supports( SSF_SCOREBOARD_KD ) )
	{
		CG_FillRect( scoreX1-8, scoreY-8, cg_hudMiniScoreboardW.value, scoreY-8 + (cg_hudMiniScoreboardLH.value*((count > cg.numScores ? cg.numScores : count)+1)), clrBlack );

		for ( i=0; i<count; i++ )
		{
			if ( i == cg.numScores)
				break;
			rank++;
			Com_sprintf( scoreName[i], sizeof( scoreName[i] ), "%2i ("S_COLOR_CYAN"%2i"S_COLOR_WHITE") - %s", rank, cg.scores[i].client, cgs.clientinfo[cg.scores[i].client].name );
			scoreKills[i]	= cg.scores[i].score;
			scoreDeaths[i]	= cg.scores[i].deaths;
		}
		CG_Text_Paint( scoreX1, scoreY, scoreS, colorWhite, va( "%-5s%-4s%-36s%s", "Rank", "ID", "Name", "Score" ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_JAPPMONO );
	//	CG_Text_Paint( scoreX2, scoreY, scoreS, colorWhite, "K/D", 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );

		for ( i=0; i<count; i++ )
		{
			if ( i == cg.numScores)
				break;
			scoreY += cg_hudMiniScoreboardLH.value;
			if ( cg.scores[i].client == cg.snap->ps.clientNum )
				CG_FillRect( scoreX1-8, scoreY+2.0f, cg_hudMiniScoreboardW.value, cg_hudMiniScoreboardLH.value, colorBlue );
			CG_Text_Paint( scoreX1, scoreY, scoreS, colorWhite, scoreName[i], 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_JAPPMONO );
		//	CG_Text_Paint( scoreX2, scoreY, scoreS, colorWhite, va( "%i/%i", scoreKills[i], scoreDeaths[i] ), 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		}

	}
	else
	{
		//TODO: Scoreboard for servers that don't support K/D
		CG_Text_Paint( 50, scoreY, scoreS, colorYellow, "TELL RAZ0R TO CODE SCOREBOARDS FOR SERVERS WITHOUT KD SUPPORT FLAGS!", 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
#else
#endif
}

void JP_DrawStats( void )
{
	static int	lastPingGrab		= 0;		// Grab ping every 500ms
	static int	ping				= 999;
	static int	lastLTimeGrab		= 0;		// Grab local time every 250ms
	static char localTimeStr[32]	= { 0 };
	static int	lastMTimeGrab		= 0;		//	Grab map time every 250ms
	static char	mapTimeStr[32]		= { 0 };
	float		speed				= ( sqrtf( cg.predictedPlayerState.velocity.x*cg.predictedPlayerState.velocity.x + cg.predictedPlayerState.velocity.y*cg.predictedPlayerState.velocity.y ) )/* * 0.05f*/;
	char		speedStr[32]		= { 0 };

	if ( !cg_statsHUD.integer )
		return;

	if ( lastPingGrab < cg.time - 250 )
	{//Ping (Update every 250ms)
		ping = cg.snap->ping;
		lastPingGrab = cg.time;
	}

	if ( lastLTimeGrab < cg.time - 250 )
	{//Local time (Grab every 250ms)
		time_t		sysTime		= time( NULL );
		struct tm	*time		= localtime( &sysTime );

		if ( !cg.japp.timestamp24Hour && time->tm_hour > 12 )
			time->tm_hour -= 12;

		Com_sprintf( localTimeStr, sizeof( localTimeStr ), "%02i:%02i:%02i", time->tm_hour, time->tm_min, time->tm_sec );
		lastLTimeGrab = cg.time;
	}

	if ( lastMTimeGrab < cg.time - 250 )
	{//Map time
		static int msec, seconds, mins, tens;

		msec	= cg.time - cgs.levelStartTime;	seconds = msec / 1000;
		mins	= seconds / 60;					seconds	-= mins * 60;
		tens	= seconds / 10;					seconds	-= tens * 10;

		Com_sprintf( mapTimeStr, sizeof( mapTimeStr ), "%i:%i%i", mins, tens, seconds );
		lastMTimeGrab = cg.time;
	}

	//Speedometer
	Com_sprintf( speedStr, sizeof( speedStr ), "%s%04.01f ups", ( speed >= 800.0f ? S_COLOR_RED : ( speed >= 550.0f ? S_COLOR_YELLOW : S_COLOR_WHITE ) ), speed );

	{
		char *statStr = va( "%-12s%i\n%-12s%i\n%-12s%.2f\n\n%-12s%i\n%-12s%i\n\n%-12s%s\n%-12s%s\n\n%-12s%s",
				"Score", cg.snap->ps.persistant[PERS_SCORE],
				"Deaths", cg.snap->ps.persistant[PERS_KILLED],
				"Ratio", cg.snap->ps.persistant[PERS_KILLED] ? (float)((float)cg.snap->ps.persistant[PERS_SCORE]/(float)cg.snap->ps.persistant[PERS_KILLED]) : (float)cg.snap->ps.persistant[PERS_SCORE],
				"Ping", ping,
				"FPS", cg.japp.fps,
				"Local time", localTimeStr,
				"Map Time", mapTimeStr,
				"Speed", speedStr );

		CG_Text_Paint( cg.statsPos.x, cg.statsPos.y, cg_hudStatsScale.value, &colorWhite, statStr, 0.0f, 0, ITEM_ALIGN_RIGHT|ITEM_TEXTSTYLE_OUTLINED, FONT_JAPPMONO );
	}
}

void JP_DrawMovementKeys( void )
{
#if 1
	usercmd_t cmd = { 0 };
	int moveDir = cg.snap->ps.movementDir;
	char str1[32]={0}, str2[32]={0};
	float w1=0.0f, w2=0.0f, height=0.0f;
	int fontIndex = FONT_JAPPMONO;

	if ( !cg_hudMovementKeys.integer || !cg.snap ) //RAZTODO: works with demo playback??
		return;

	if ( cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback )
		trap->GetUserCmd( trap->GetCurrentCmdNumber(), &cmd );
	else
	{
		float xyspeed = sqrtf( cg.snap->ps.velocity.x*cg.snap->ps.velocity.x + cg.snap->ps.velocity.y*cg.snap->ps.velocity.y );

		if ( (cg.snap->ps.pm_flags & PMF_JUMP_HELD) )//zspeed > lastZSpeed || zspeed > 10 )
			cmd.upmove = 1;
		else if ( (cg.snap->ps.pm_flags & PMF_DUCKED) )
			cmd.upmove = -1;

		if ( xyspeed < 10 )
			moveDir = -1;

		switch ( moveDir )
		{
		case 0: // W
			cmd.forwardmove = 1;
			break;
		case 1: // WA
			cmd.forwardmove = 1;
			cmd.rightmove = -1;
			break;
		case 2: // A
			cmd.rightmove = -1;
			break;
		case 3: // AS
			cmd.rightmove = -1;
			cmd.forwardmove = -1;
			break;
		case 4: // S
			cmd.forwardmove = -1;
			break;
		case 5: // SD
			cmd.forwardmove = -1;
			cmd.rightmove = 1;
			break;
		case 6: // D
			cmd.rightmove = 1;
			break;
		case 7: // DW
			cmd.rightmove = 1;
			cmd.forwardmove = 1;
			break;
		default:
			break;
		}
	}

	w1 = CG_Text_Width( "v W ^", cg_hudMovementKeysScale.value, fontIndex );
	w2 = CG_Text_Width( "A S D", cg_hudMovementKeysScale.value, fontIndex );
	height = CG_Text_Height( "A S D v W ^", cg_hudMovementKeysScale.value, fontIndex );

	Com_sprintf( str1, sizeof( str1 ), va( "^%cv ^%cW ^%c^", (cmd.upmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove > 0) ? COLOR_RED : COLOR_WHITE, (cmd.upmove > 0) ? COLOR_RED : COLOR_WHITE ) );
	Com_sprintf( str2, sizeof( str2 ), va( "^%cA ^%cS ^%cD", (cmd.rightmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove < 0) ? COLOR_RED : COLOR_WHITE, (cmd.rightmove > 0) ? COLOR_RED : COLOR_WHITE ) );

	CG_Text_Paint( cg.moveKeysPos.x - max(w1,w2)/2.0, cg.moveKeysPos.y,				cg_hudMovementKeysScale.value, &colorWhite, str1, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, fontIndex );
	CG_Text_Paint( cg.moveKeysPos.x - max(w1,w2)/2.0, cg.moveKeysPos.y + height,	cg_hudMovementKeysScale.value, &colorWhite, str2, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, fontIndex );
#endif
}

#define NUM_ACCEL_SAMPLES (8)
void JP_DrawAccel( void )
{
#if 1 // done in Lua now =]
	if ( !cg_accelerometer.integer )
		return;
	else
	{
		float			accel = 0.0f, maxAccel = 0.0f, speed = 0.0f, maxSpeed = 0.0f, avgAccel = 0.0f;
		static vector3	lastVelocity = { 0.0f, 0.0f, 0.0f };
		static float	lastSpeed = 0.0f, accelSamples[NUM_ACCEL_SAMPLES] = { 0.0f };
		vector3			currentVelocity;
		playerState_t	*ps = NULL;
		int i=0;
		float percent=0.0f;

		if ( (cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback) || !cg.snap )
			ps = &cg.predictedPlayerState;
		else
			ps = &cg.snap->ps;

		VectorCopy( &ps->velocity, &currentVelocity );
		currentVelocity.z = 0.0f;
		speed = VectorLength( &currentVelocity );

		accel = speed-lastSpeed;

		maxSpeed = ((ps->fd.forcePowersActive & (1<<FP_SPEED)) ? 425 : 250);
		maxAccel = (maxSpeed/(float)cg.japp.fps);

		//store for next frame
		VectorCopy( &currentVelocity, &lastVelocity );
		lastSpeed = speed;

		memcpy( &accelSamples[0], &accelSamples[1], sizeof( float ) * (NUM_ACCEL_SAMPLES-1) );
		accelSamples[NUM_ACCEL_SAMPLES-1] = accel;

		for ( i=0; i<NUM_ACCEL_SAMPLES; i++ )
			avgAccel += accelSamples[i];
		avgAccel /= (float)NUM_ACCEL_SAMPLES;

		percent = Q_clamp( -1.0f, avgAccel/maxAccel, 1.0f );
		/*
		UI_DrawProportionalString(	cg.accelerometer.position.x + (cg.accelerometer.size.w/2),
									cg.accelerometer.position.y +  cg.accelerometer.size.h,
									va( "%.3f", avgAccel ),
									UI_SMALLFONT|UI_DROPSHADOW|UI_CENTER,
									&colorTable[CT_RED] );
		*/
		if ( percent && percent < 100 )
			CG_FillRect( cg.accelerometer.position.x,
					 cg.accelerometer.position.y,
					 cg.accelerometer.size.w*percent,
					 cg.accelerometer.size.h,
					 &colorTable[CT_RED] );
		/*
		CG_DrawRect( cg.accelerometer.position.x,
					 cg.accelerometer.position.y,
					 cg.accelerometer.size.w,
					 cg.accelerometer.size.h,
					 0.5f,
					 &colorTable[CT_BLACK] );
		*/
	}
#endif
}


void CG_DrawHUD( centity_t *cent )
{
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*focusItem = NULL;
	const char *scoreStr = NULL;
	int	scoreBias;
	char scoreBiasStr[16];

#ifdef _DEBUG
	if ( cg_debugInfo.integer )
	{
		int x = 0, y = SCREEN_HEIGHT-80;
		char *str = NULL;
		entityState_t *es = &cg_entities[cg.snap->ps.clientNum].currentState;
		playerState_t *ps = &cg.predictedPlayerState;

		if ( cg_smartEntities.integer )
		{//Smart entities
			str = S_COLOR_RED"Smart entities ON";
			x = ( 36 - (trap->R_Font_StrLenPixels( str, 1, 0.8f ) / 2) );
			trap->R_Font_DrawString( x+16, y-180, str, &colorTable[CT_VLTBLUE1], 1, -1, 0.8f );
		}

		if ( cg.snap->ps.eFlags & EF_ALT_DIM )
		{//alt-dim
			str = S_COLOR_RED"Alternate dimension!";
			x = ( 48 - (trap->R_Font_StrLenPixels( str, 1, 0.8f ) / 2) );
			trap->R_Font_DrawString( x+16, y-140, str, &colorTable[CT_VLTBLUE1], 1, -1, 0.8f );
		}

		//(eFlags2 & 512) == grapple is out
		//(eFlags & 65536) == grapple controls movement
		str = va(
S_COLOR_CYAN"legsAnim: %i\n"
S_COLOR_CYAN"torsoAnim: %i\n"
S_COLOR_CYAN"duelIndex: %i\n"
S_COLOR_CYAN"duelInProgress: %i\n"
S_COLOR_CYAN"eFlags: %i\n"
S_COLOR_CYAN"eFlags2: %i\n"
S_COLOR_CYAN"activeForcePass: %i\n"
S_COLOR_CYAN"generic1: %i\n"
S_COLOR_CYAN"genericEnemyIndex: %i\n"
S_COLOR_CYAN"pm_flags: %i\n"
S_COLOR_CYAN"pm_type: %i\n"
S_COLOR_CYAN"ragAttach: %i\n"
S_COLOR_YELLOW"bolt1: %i\n"
S_COLOR_YELLOW"bolt2: %i\n"
S_COLOR_YELLOW"generic1: %i\n"
S_COLOR_YELLOW"genericenemyindex: %i\n"
S_COLOR_CYAN"forcePowerSelected: %i\n"
S_COLOR_CYAN"forcePowersKnown: %i\n"
S_COLOR_GREEN"cg.forceSelect: %i\n"
S_COLOR_GREEN"cg.time: %i\n"
S_COLOR_GREEN"speed: %f\n"
S_COLOR_CYAN"legsTimer: %i\n"
S_COLOR_CYAN"groundEntityNum: %i",
			ps->legsAnim,
			ps->torsoAnim,
			ps->duelIndex,
			ps->duelInProgress,
			ps->eFlags,
			ps->eFlags2,
			ps->activeForcePass,
			ps->generic1,
			ps->genericEnemyIndex,
			ps->pm_flags,
			ps->pm_type,
			ps->ragAttach,
			es->bolt1,
			es->bolt2,
			es->generic1,
			es->genericenemyindex,
			ps->fd.forcePowerSelected,
			ps->fd.forcePowersKnown,
			cg.forceSelect,
			cg.time,
			ps->speed,
			ps->legsTimer,
			ps->groundEntityNum );
		//	x = ( 54 - (trap->R_Font_StrLenPixels( str, 1, 0.8f ) / 2) );
		//	trap->R_Font_DrawString( x+16, y-280, str, colorTable[CT_VLTBLUE1], 1, -1, 0.8f );
		CG_Text_Paint( 16, y-380, 0.5f, &colorTable[CT_WHITE], str, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
#endif

	JP_DrawMiniScoreboard();
	JP_DrawStats();
	JP_DrawMovementKeys();
	JP_DrawAccel();

	if ( JPLua_Event_HUD() )
		return;

	if ( cg_hudFiles.integer == 1 )
	{
		int x = 0;
		int y = SCREEN_HEIGHT-80;
		char ammoString[64];
		int weapX = x;

		UI_DrawProportionalString( /*cg_hudHealthX.value*/x+16,	/*cg_hudHealthY.value*/y+40,		va( "%i", cg.snap->ps.stats[STAT_HEALTH] ),	UI_SMALLFONT|UI_DROPSHADOW, &colorTable[CT_HUD_RED] );
		UI_DrawProportionalString( /*cg_hudArmorX.value*/x+18+14,	/*cg_hudArmorY.value*/y+40+14,	va( "%i", cg.snap->ps.stats[STAT_ARMOR] ),	UI_SMALLFONT|UI_DROPSHADOW, &colorTable[CT_HUD_GREEN] );

		if ( cg.snap->ps.weapon == WP_SABER )
		{
			if ( cg.snap->ps.fd.saberDrawAnimLevel == SS_DUAL )
			{
				Com_sprintf( ammoString, sizeof( ammoString ), "AKIMBO" );
				weapX += 16;
			}
			else if ( cg.snap->ps.fd.saberDrawAnimLevel == SS_STAFF )
			{
				Com_sprintf( ammoString, sizeof( ammoString ), "STAFF" );
				weapX += 16;
			}
			else if ( cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_3 )
			{
				Com_sprintf( ammoString, sizeof( ammoString ), "STRONG" );
				weapX += 16;
			}
			else if ( cg.snap->ps.fd.saberDrawAnimLevel == FORCE_LEVEL_2 )
			{
				Com_sprintf( ammoString, sizeof( ammoString ), "MEDIUM" );
				weapX += 16;
			}
			else
				Com_sprintf( ammoString, sizeof( ammoString ), "FAST" );
		}
		else
			Com_sprintf( ammoString, sizeof( ammoString ), "%i", cg.snap->ps.ammo[weaponData[cent->currentState.weapon].ammoIndex] );

		UI_DrawProportionalString( /*cg_hudAmmoX.value*/SCREEN_WIDTH-(weapX+16+32),	/*cg_hudAmmoY.value*/y+40,		va( "%s", ammoString ),					UI_SMALLFONT|UI_DROPSHADOW, &colorTable[CT_HUD_ORANGE] );
		UI_DrawProportionalString( /*cg_hudForceX.value*/SCREEN_WIDTH-(x+18+14+32),	/*cg_hudForceY.value*/y+40+14,	va( "%i", cg.snap->ps.fd.forcePower ),	UI_SMALLFONT|UI_DROPSHADOW, &colorTable[CT_ICON_BLUE] );
		return;
	}

	// Draw the left HUD
	menuHUD = Menus_FindByName("lefthud");
	Menu_Paint( menuHUD, qtrue );

	if (menuHUD)
	{
	//	itemDef_t *focusItem;

		// Print scanline
		focusItem = Menu_FindItemByName(menuHUD, "scanline");
		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );
			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		// Print frame
		focusItem = Menu_FindItemByName(menuHUD, "frame");
		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );
			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		if (cg.predictedPlayerState.pm_type != PM_SPECTATOR)
		{
			CG_DrawArmor(menuHUD);
			CG_DrawHealth(menuHUD);
		}
	}
	else
	{
		//trap->Error("CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
	}

	//scoreStr = va("Score: %i", cgs.clientinfo[cg.snap->ps.clientNum].score);
	if ( cgs.gametype == GT_DUEL )
	{//A duel that requires more than one kill to knock the current enemy back to the queue
		//show current kills out of how many needed
		scoreStr = va("%s: %i/%i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE], cgs.fraglimit);
	}
	else if (0 && cgs.gametype < GT_TEAM )
	{	// This is a teamless mode, draw the score bias.
		scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores1;
		if (scoreBias == 0)
		{	// We are the leader!
			if (cgs.scores2 <= 0)
			{	// Nobody to be ahead of yet.
				Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), "");
			}
			else
			{
				scoreBias = cg.snap->ps.persistant[PERS_SCORE] - cgs.scores2;
				if (scoreBias == 0)
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (Tie)");
				}
				else
				{
					Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (+%d)", scoreBias);
				}
			}
		}
		else // if (scoreBias < 0)
		{	// We are behind!
			Com_sprintf(scoreBiasStr, sizeof(scoreBiasStr), " (%d)", scoreBias);
		}
		scoreStr = va("%s: %i%s", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE], scoreBiasStr);
	}
	else
	{	// Don't draw a bias.
		scoreStr = va("%s: %i", CG_GetStringEdString("MP_INGAME", "SCORE"), cg.snap->ps.persistant[PERS_SCORE]);
	}

	menuHUD = Menus_FindByName("righthud");
	Menu_Paint( menuHUD, qtrue );

	if (menuHUD)
	{
		if (cgs.gametype != GT_POWERDUEL)
		{
			focusItem = Menu_FindItemByName(menuHUD, "score_line");
			if (focusItem)
			{
				UI_DrawScaledProportionalString(
					focusItem->window.rect.x,
					focusItem->window.rect.y,
					scoreStr,
					UI_RIGHT|UI_DROPSHADOW,
					&focusItem->window.foreColor,
					0.7f);
			}
		}

		// Print scanline
		focusItem = Menu_FindItemByName(menuHUD, "scanline");
		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );
			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		focusItem = Menu_FindItemByName(menuHUD, "frame");
		if (focusItem)
		{
			trap->R_SetColor( &colorTable[CT_WHITE] );
			CG_DrawPic(
				focusItem->window.rect.x,
				focusItem->window.rect.y,
				focusItem->window.rect.w,
				focusItem->window.rect.h,
				focusItem->window.background
				);
		}

		CG_DrawForcePower(menuHUD);

		// Draw ammo tics or saber style
		if ( cent->currentState.weapon == WP_SABER )
		{
			CG_DrawSaberStyle(cent,menuHUD);
		}
		else
		{
			CG_DrawAmmo(cent,menuHUD);
		}
	}
	else
	{
		//trap->Error("CG_ChatBox_ArrayInsert: unable to locate HUD menu file ");
	}
}

#define MAX_SHOWPOWERS NUM_FORCE_POWERS

qboolean ForcePower_Valid(int i)
{
	if (i == FP_LEVITATION ||
		i == FP_SABER_OFFENSE ||
		i == FP_SABER_DEFENSE ||
		i == FP_SABERTHROW)
	{
		return qfalse;
	}

	if (cg.snap->ps.fd.forcePowersKnown & (1 << i))
	{
		return qtrue;
	}

	return qfalse;
}

/*
===================
CG_DrawForceSelect
===================
*/
void CG_DrawForceSelect( void )
{
	int		i;
	int		count;
	int		smallIconSize,bigIconSize;
	int		holdX,x,y,pad;
	int		sideLeftIconCnt,sideRightIconCnt;
	int		sideMax,holdCount,iconCnt;
	int		yOffset = 0;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.forceSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		cg.forceSelect = cg.snap->ps.fd.forcePowerSelected;
		return;
	}

	if (!cg.snap->ps.fd.forcePowersKnown)
	{
		return;
	}

	// count the number of powers owned
	count = 0;

	for (i=0;i < NUM_FORCE_POWERS;++i)
	{
		if (ForcePower_Valid(i))
		{
			count++;
		}
	}

	if (count == 0)	// If no force powers, don't display
	{
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	smallIconSize = 30;
	bigIconSize = 60;
	pad = 12;

	x = 320;
	y = 425;

	// Background
	i = BG_ProperForceIndex(cg.forceSelect) - 1;
	if (i < 0)
	{
		i = MAX_SHOWPOWERS-1;
	}

	trap->R_SetColor(NULL);
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);
	for (iconCnt=1;iconCnt<(sideLeftIconCnt+1);i--)
	{
		if (i < 0)
		{
			i = MAX_SHOWPOWERS-1;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (media.gfx.interface.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, media.gfx.interface.forcePowerIcons[forcePowerSorted[i]] );
			holdX -= (smallIconSize+pad);
		}
	}

	if (ForcePower_Valid(cg.forceSelect))
	{

		// Current Center Icon
		if (media.gfx.interface.forcePowerIcons[cg.forceSelect])
		{
			CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2)) + yOffset, bigIconSize, bigIconSize, media.gfx.interface.forcePowerIcons[cg.forceSelect] ); //only cache the icon for display
		}
	}

	i = BG_ProperForceIndex(cg.forceSelect) + 1;
	if (i>=MAX_SHOWPOWERS)
	{
		i = 0;
	}

	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=1;iconCnt<(sideRightIconCnt+1);i++)
	{
		if (i>=MAX_SHOWPOWERS)
		{
			i = 0;
		}

		if (!ForcePower_Valid(forcePowerSorted[i]))	// Does he have this power?
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (media.gfx.interface.forcePowerIcons[forcePowerSorted[i]])
		{
			CG_DrawPic( holdX, y + yOffset, smallIconSize, smallIconSize, media.gfx.interface.forcePowerIcons[forcePowerSorted[i]] ); //only cache the icon for display
			holdX += (smallIconSize+pad);
		}
	}

	if ( showPowersName[cg.forceSelect] )
	{
		UI_DrawProportionalString(320, y + 30 + yOffset, CG_GetStringEdString("SP_INGAME", showPowersName[cg.forceSelect]), UI_CENTER | UI_SMALLFONT, &colorTable[CT_ICON_BLUE]);
	}
}

/*
===================
CG_DrawInventorySelect
===================
*/
void CG_DrawInvenSelect( void )
{
	int				i;
	int				sideMax,holdCount,iconCnt;
	int				smallIconSize,bigIconSize;
	int				sideLeftIconCnt,sideRightIconCnt;
	int				count;
	int				holdX,x,y,y2,pad;

	// don't display if dead
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return;
	}

	if ((cg.invenSelectTime+WEAPON_SELECT_TIME)<cg.time)	// Time is up for the HUD to display
	{
		return;
	}

	if (!cg.snap->ps.stats[STAT_HOLDABLE_ITEM] || !cg.snap->ps.stats[STAT_HOLDABLE_ITEMS])
	{
		return;
	}

	if (cg.itemSelect == -1)
	{
		cg.itemSelect = bg_itemlist[cg.snap->ps.stats[STAT_HOLDABLE_ITEM]].giTag;
	}

//const int bits = cg.snap->ps.stats[ STAT_ITEMS ];

	// count the number of items owned
	count = 0;
	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ )
	{
		if (/*CG_InventorySelectable(i) && inv_icons[i]*/
			(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) )
		{
			count++;
		}
	}

	if (!count)
	{
		y2 = 0; //err?
		UI_DrawProportionalString(320, y2 + 22, "EMPTY INVENTORY", UI_CENTER | UI_SMALLFONT, &colorTable[CT_ICON_BLUE]);
		return;
	}

	sideMax = 3;	// Max number of icons on the side

	// Calculate how many icons will appear to either side of the center one
	holdCount = count - 1;	// -1 for the center icon
	if (holdCount == 0)			// No icons to either side
	{
		sideLeftIconCnt = 0;
		sideRightIconCnt = 0;
	}
	else if (count > (2*sideMax))	// Go to the max on each side
	{
		sideLeftIconCnt = sideMax;
		sideRightIconCnt = sideMax;
	}
	else							// Less than max, so do the calc
	{
		sideLeftIconCnt = holdCount/2;
		sideRightIconCnt = holdCount - sideLeftIconCnt;
	}

	i = cg.itemSelect - 1;
	if (i<0)
	{
		i = HI_NUM_HOLDABLE-1;
	}

	smallIconSize = 40;
	bigIconSize = 80;
	pad = 16;

	x = 320;
	y = 410;

	// Left side ICONS
	// Work backwards from current icon
	holdX = x - ((bigIconSize/2) + pad + smallIconSize);

	for (iconCnt=0;iconCnt<sideLeftIconCnt;i--)
	{
		if (i<0)
		{
			i = HI_NUM_HOLDABLE-1;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (media.gfx.interface.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, media.gfx.interface.invenIcons[i] );

			trap->R_SetColor(&colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);
				*/

			holdX -= (smallIconSize+pad);
		}
	}

	// Current Center Icon
	if (media.gfx.interface.invenIcons[cg.itemSelect] && BG_IsItemSelectable(&cg.predictedPlayerState, cg.itemSelect))
	{
		int itemNdex;
		trap->R_SetColor(NULL);
		CG_DrawPic( x-(bigIconSize/2), (y-((bigIconSize-smallIconSize)/2))+10, bigIconSize, bigIconSize, media.gfx.interface.invenIcons[cg.itemSelect] );
		trap->R_SetColor(&colorTable[CT_ICON_BLUE]);
		/*CG_DrawNumField ((x-(bigIconSize/2)) + addX, y, 2, cg.snap->ps.inventory[cg.inventorySelect], 6, 12,
			NUM_FONT_SMALL,qfalse);*/

		itemNdex = BG_GetItemIndexByTag(cg.itemSelect, IT_HOLDABLE);
		if (bg_itemlist[itemNdex].classname)
		{
			vector4	textColor = { .312f, .75f, .621f, 1.0f };
			char	text[1024];
			char	upperKey[1024];

			strcpy(upperKey, bg_itemlist[itemNdex].classname);

			if ( trap->SE_GetStringTextString( va("SP_INGAME_%s", upperKey), text, sizeof( text )))
			{
				UI_DrawProportionalString(320, y+45, text, UI_CENTER | UI_SMALLFONT, &textColor);
			}
			else
			{
				UI_DrawProportionalString(320, y+45, bg_itemlist[itemNdex].classname, UI_CENTER | UI_SMALLFONT, &textColor);
			}
		}
	}

	i = cg.itemSelect + 1;
	if (i> HI_NUM_HOLDABLE-1)
	{
		i = 0;
	}

	// Right side ICONS
	// Work forwards from current icon
	holdX = x + (bigIconSize/2) + pad;
	for (iconCnt=0;iconCnt<sideRightIconCnt;i++)
	{
		if (i> HI_NUM_HOLDABLE-1)
		{
			i = 0;
		}

		if ( !(cg.snap->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << i)) || i == cg.itemSelect )
		{
			continue;
		}

		++iconCnt;					// Good icon

		if (!BG_IsItemSelectable(&cg.predictedPlayerState, i))
		{
			continue;
		}

		if (media.gfx.interface.invenIcons[i])
		{
			trap->R_SetColor(NULL);
			CG_DrawPic( holdX, y+10, smallIconSize, smallIconSize, media.gfx.interface.invenIcons[i] );

			trap->R_SetColor(&colorTable[CT_ICON_BLUE]);
			/*CG_DrawNumField (holdX + addX, y + smallIconSize, 2, cg.snap->ps.inventory[i], 6, 12,
				NUM_FONT_SMALL,qfalse);*/

			holdX += (smallIconSize+pad);
		}
	}
}

int cg_targVeh = ENTITYNUM_NONE;
int cg_targVehLastTime = 0;
qboolean CG_CheckTargetVehicle( centity_t **pTargetVeh, float *alpha )
{
	int targetNum = ENTITYNUM_NONE;
	centity_t	*targetVeh = NULL;

	if ( !pTargetVeh || !alpha )
	{//hey, where are my pointers?
		return qfalse;
	}

	*alpha = 1.0f;

	//FIXME: need to clear all of these when you die?
	if ( cg.predictedPlayerState.rocketLockIndex < ENTITYNUM_WORLD )
	{
		targetNum = cg.predictedPlayerState.rocketLockIndex;
	}
	else if ( cg.crosshairVehNum < ENTITYNUM_WORLD
		&& cg.time - cg.crosshairVehTime < 3000 )
	{//crosshair was on a vehicle in the last 3 seconds
		targetNum = cg.crosshairVehNum;
	}
    else if ( cg.crosshairClientNum < ENTITYNUM_WORLD )
	{
		targetNum = cg.crosshairClientNum;
	}

	if ( targetNum < MAX_CLIENTS )
	{//real client
		if ( cg_entities[targetNum].currentState.m_iVehicleNum >= MAX_CLIENTS )
		{//in a vehicle
			targetNum = cg_entities[targetNum].currentState.m_iVehicleNum;
		}
	}
    if ( targetNum < ENTITYNUM_WORLD
		&& targetNum >= MAX_CLIENTS )
	{
	//	centity_t *targetVeh = &cg_entities[targetNum];
		targetVeh = &cg_entities[targetNum];
		if ( targetVeh->currentState.NPC_class == CLASS_VEHICLE
			&& targetVeh->m_pVehicle
			&& targetVeh->m_pVehicle->m_pVehicleInfo
			&& targetVeh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//it's a vehicle
			cg_targVeh = targetNum;
			cg_targVehLastTime = cg.time;
			*alpha = 1.0f;
		}
		else
		{
			targetVeh = NULL;
		}
	}
	if ( targetVeh )
	{
		*pTargetVeh = targetVeh;
		return qtrue;
	}

	if ( cg_targVehLastTime && cg.time-cg_targVehLastTime < 3000 ) {
		//stay at full alpha for 1 sec after lose them from crosshair
		if ( cg.time-cg_targVehLastTime < 1000 )
			*alpha = 1.0f;

		else //fade out over 2 secs
			*alpha = 1.0f-((cg.time-cg_targVehLastTime-1000)/2000.0f);
	}
	return qfalse;
}

#define MAX_VHUD_SHIELD_TICS 12
#define MAX_VHUD_SPEED_TICS 5
#define MAX_VHUD_ARMOR_TICS 5
#define MAX_VHUD_AMMO_TICS 5

float CG_DrawVehicleShields( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxShields;
	vector4			calcColor;
	itemDef_t		*item;
	float			percShields;

	item = Menu_FindItemByName((menuDef_t	*) menuHUD, "armorbackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxShields = veh->m_pVehicle->m_pVehicleInfo->shields;
	currValue = cg.predictedVehicleState.stats[STAT_ARMOR];
	percShields = (float)currValue/(float)maxShields;
	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxShields / MAX_VHUD_ARMOR_TICS;
	for (i=1;i<=MAX_VHUD_ARMOR_TICS;i++)
	{
		sprintf( itemName, "armor_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *) menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor.a *= percent;		// Fade it out
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}

	return percShields;
}

int cg_vehicleAmmoWarning = 0;
int cg_vehicleAmmoWarningTime = 0;
void CG_DrawVehicleAmmo( const menuDef_t *menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxAmmo;
	vector4	calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "ammobackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = cg.predictedVehicleState.ammo[0];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<=MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammo_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(&calcColor, &g_color_table[ColorIndex(COLOR_RED)], sizeof(vector4));
			calcColor.a = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor.a *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoUpper( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	char		itemName[64];
	float		inc, currValue,maxAmmo;
	vector4		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammoupperbackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[0].ammoMax;
	currValue = cg.predictedVehicleState.ammo[0];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammoupper_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 0 )
		{
			memcpy(&calcColor, &g_color_table[ColorIndex(COLOR_RED)], sizeof(vector4));
			calcColor.a = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor.a *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}


void CG_DrawVehicleAmmoLower( const menuDef_t *menuHUD, const centity_t *veh )
{
	int				i;
	char			itemName[64];
	float			inc, currValue,maxAmmo;
	vector4			calcColor;
	itemDef_t		*item;


	item = Menu_FindItemByName((menuDef_t *)menuHUD, "ammolowerbackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxAmmo = veh->m_pVehicle->m_pVehicleInfo->weapon[1].ammoMax;
	currValue = cg.predictedVehicleState.ammo[1];

	inc = (float) maxAmmo / MAX_VHUD_AMMO_TICS;
	for (i=1;i<MAX_VHUD_AMMO_TICS;i++)
	{
		sprintf( itemName, "ammolower_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg_vehicleAmmoWarningTime > cg.time
			&& cg_vehicleAmmoWarning == 1 )
		{
			memcpy(&calcColor, &g_color_table[ColorIndex(COLOR_RED)], sizeof(vector4));
			calcColor.a = sin(cg.time*0.005)*0.5f+0.5f;
		}
		else
		{
			memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));

			if (currValue <= 0)	// don't show tic
			{
				break;
			}
			else if (currValue < inc)	// partial tic (alpha it out)
			{
				float percent = currValue / inc;
				calcColor.a *= percent;		// Fade it out
			}
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

// The HUD.menu file has the graphic print with a negative height, so it will print from the bottom up.
void CG_DrawVehicleTurboRecharge( const menuDef_t	*menuHUD, const centity_t *veh )
{
	itemDef_t	*item;
	int			height;

	item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "turborecharge");

	//Raz: Crashed here, so place a check and return..
	if ( !veh || !veh->m_pVehicle )
		return;

	if (item)
	{
		float percent=0.0f;
		int diff = ( cg.time - veh->m_pVehicle->m_iTurboTime );

		height = item->window.rect.h;

		if (diff > veh->m_pVehicle->m_pVehicleInfo->turboRecharge)
		{
			percent = 1.0f;
			trap->R_SetColor( &colorTable[CT_GREEN] );
		}
		else
		{
			percent = (float) diff / veh->m_pVehicle->m_pVehicleInfo->turboRecharge;
			if (percent < 0.0f)
			{
				percent = 0.0f;
			}
			trap->R_SetColor( &colorTable[CT_RED] );
		}

		height *= percent;

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			height,
			media.gfx.world.whiteShader);
	}
}

qboolean cg_drawLink = qfalse;
void CG_DrawVehicleWeaponsLinked( const menuDef_t	*menuHUD, const centity_t *veh )
{
	qboolean drawLink = qfalse;
	if ( veh->m_pVehicle
		&& veh->m_pVehicle->m_pVehicleInfo
		&& (veh->m_pVehicle->m_pVehicleInfo->weapon[0].linkable == 2|| veh->m_pVehicle->m_pVehicleInfo->weapon[1].linkable == 2) )
	{//weapon is always linked
		drawLink = qtrue;
	}
	else
	{
//MP way:
		//must get sent over network
		if ( cg.predictedVehicleState.vehWeaponsLinked )
		{
			drawLink = qtrue;
		}
//NOTE: below is SP way
/*

		//just cheat it
		if ( veh->gent->m_pVehicle->weaponStatus[0].linked
			|| veh->gent->m_pVehicle->weaponStatus[1].linked )
		{
			drawLink = qtrue;
		}
*/
	}

	if ( cg_drawLink != drawLink )
	{//state changed, play sound
		cg_drawLink = drawLink;
		trap->S_StartSound (NULL, cg.predictedPlayerState.clientNum, CHAN_LOCAL, trap->S_RegisterSound( "sound/vehicles/common/linkweaps.wav" ) );
	}

	if ( drawLink )
	{
		itemDef_t	*item;

		item = Menu_FindItemByName( (menuDef_t	*) menuHUD, "weaponslinked");

		if (item)
		{
			trap->R_SetColor( &colorTable[CT_CYAN] );

				CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				media.gfx.world.whiteShader);
		}
	}
}

void CG_DrawVehicleSpeed( const menuDef_t	*menuHUD, const centity_t *veh )
{
	int i;
	char itemName[64];
	float inc, currValue,maxSpeed;
	vector4		calcColor;
	itemDef_t	*item;

	item = Menu_FindItemByName((menuDef_t *) menuHUD, "speedbackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	maxSpeed = veh->m_pVehicle->m_pVehicleInfo->speedMax;
	currValue = cg.predictedVehicleState.speed;


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxSpeed / MAX_VHUD_SPEED_TICS;
	for (i=1;i<=MAX_VHUD_SPEED_TICS;i++)
	{
		sprintf( itemName, "speed_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *)menuHUD, itemName);

		if (!item)
		{
			continue;
		}

		if ( cg.time > veh->m_pVehicle->m_iTurboTime )
		{
			memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));
		}
		else	// In turbo mode
		{
			if (cg.VHUDFlashTime < cg.time)
			{
				cg.VHUDFlashTime = cg.time + 200;
				if (cg.VHUDTurboFlag)
				{
					cg.VHUDTurboFlag = qfalse;
				}
				else
				{
					cg.VHUDTurboFlag = qtrue;
				}
			}

			if (cg.VHUDTurboFlag)
				memcpy(&calcColor, &colorTable[CT_LTRED1], sizeof(vector4));
			else
				memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));
		}


		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor.a *= percent;		// Fade it out
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

void CG_DrawVehicleArmor( const menuDef_t *menuHUD, const centity_t *veh )
{
	int			i;
	vector4		calcColor;
	char		itemName[64];
	float		inc, currValue,maxArmor;
	itemDef_t	*item;

	//Raz: Crashed here, place check and return
	if ( !veh || !veh->m_pVehicle )
		return;

	maxArmor = veh->m_pVehicle->m_pVehicleInfo->armor;
	currValue = cg.predictedVehicleState.stats[STAT_HEALTH];

	item = Menu_FindItemByName( (menuDef_t *) menuHUD, "shieldbackground");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}


	// Print all the tics of the shield graphic
	// Look at the amount of health left and show only as much of the graphic as there is health.
	// Use alpha to fade out partial section of health
	inc = (float) maxArmor / MAX_VHUD_SHIELD_TICS;
	for (i=1;i <= MAX_VHUD_SHIELD_TICS;i++)
	{
		sprintf( itemName, "shield_tic%d",	i );

		item = Menu_FindItemByName((menuDef_t *) menuHUD, itemName);

		if (!item)
		{
			continue;
		}


		memcpy(&calcColor, &item->window.foreColor, sizeof(vector4));

		if (currValue <= 0)	// don't show tic
		{
			break;
		}
		else if (currValue < inc)	// partial tic (alpha it out)
		{
			float percent = currValue / inc;
			calcColor.a *= percent;		// Fade it out
		}

		trap->R_SetColor( &calcColor);

		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );

		currValue -= inc;
	}
}

enum
{
	VEH_DAMAGE_FRONT=0,
	VEH_DAMAGE_BACK,
	VEH_DAMAGE_LEFT,
	VEH_DAMAGE_RIGHT,
};

typedef struct veh_damage_s {
	const char *itemName;
	short heavyDamage, lightDamage;
} veh_damage_t;

veh_damage_t vehDamageData[4] =
{
	{ "vehicle_front", SHIPSURF_DAMAGE_FRONT_HEAVY, SHIPSURF_DAMAGE_FRONT_LIGHT },
	{ "vehicle_back", SHIPSURF_DAMAGE_BACK_HEAVY, SHIPSURF_DAMAGE_BACK_LIGHT },
	{ "vehicle_left", SHIPSURF_DAMAGE_LEFT_HEAVY, SHIPSURF_DAMAGE_LEFT_LIGHT },
	{ "vehicle_right", SHIPSURF_DAMAGE_RIGHT_HEAVY, SHIPSURF_DAMAGE_RIGHT_LIGHT },
};

// Draw health graphic for given part of vehicle
void CG_DrawVehicleDamage(const centity_t *veh,int brokenLimbs,const menuDef_t	*menuHUD,float alpha,int index)
{
	itemDef_t		*item;
	int				colorI;
	vector4			color;
	int				graphicHandle=0;

	item = Menu_FindItemByName((menuDef_t *)menuHUD, vehDamageData[index].itemName);
	if (item)
	{
		if (brokenLimbs & (1<<vehDamageData[index].heavyDamage))
		{
			colorI = CT_RED;
			if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
			{
				colorI = CT_DKGREY;
			}
		}
		else if (brokenLimbs & (1<<vehDamageData[index].lightDamage))
		{
			colorI = CT_YELLOW;
		}
		else
		{
			colorI = CT_GREEN;
		}

		VectorCopy4 ( &colorTable[colorI], &color );
		color.a = alpha;
		trap->R_SetColor( &color );

		switch ( index ) {
			case VEH_DAMAGE_FRONT:
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconFrontHandle;
				break;
			case VEH_DAMAGE_BACK:
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconBackHandle;
				break;
			case VEH_DAMAGE_LEFT:
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconLeftHandle;
				break;
			case VEH_DAMAGE_RIGHT:
				graphicHandle = veh->m_pVehicle->m_pVehicleInfo->iconRightHandle;
				break;
			default:
				break;
		}

		if (graphicHandle)
		{
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				graphicHandle );
		}
	}
}


// Used on both damage indicators :  player vehicle and the vehicle the player is locked on
void CG_DrawVehicleDamageHUD(const centity_t *veh,int brokenLimbs,float percShields,char *menuName, float alpha)
{
	menuDef_t		*menuHUD;
	itemDef_t		*item;
	vector4			color;

	menuHUD = Menus_FindByName(menuName);

	if ( !menuHUD )
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "background");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle)
		{
			if ( veh->damageTime > cg.time )
			{//ship shields currently taking damage
				//NOTE: cent->damageAngle can be accessed to get the direction from the ship origin to the impact point (in 3-D space)
				float perc = 1.0f - ((veh->damageTime - cg.time) / 2000.0f/*MIN_SHIELD_TIME*/);
				if ( perc < 0.0f )
				{
					perc = 0.0f;
				}
				else if ( perc > 1.0f )
				{
					perc = 1.0f;
				}
				color.r = item->window.foreColor.r;//flash red
				color.g = item->window.foreColor.g*perc;//fade other colors back in over time
				color.b = item->window.foreColor.b*perc;//fade other colors back in over time
				color.a = item->window.foreColor.a;//always normal alpha
				trap->R_SetColor( &color );
			}
			else
			{
				trap->R_SetColor( &item->window.foreColor );
			}

			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicBackgroundHandle );
		}
	}

	item = Menu_FindItemByName(menuHUD, "outer_frame");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle)
		{
			trap->R_SetColor( &item->window.foreColor );
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicFrameHandle );
		}
	}

	item = Menu_FindItemByName(menuHUD, "shields");
	if (item)
	{
		if (veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle)
		{
			VectorCopy4 ( &colorTable[CT_HUD_GREEN], &color );
			color.a = percShields;
			trap->R_SetColor( &color );
			CG_DrawPic(
				item->window.rect.x,
				item->window.rect.y,
				item->window.rect.w,
				item->window.rect.h,
				veh->m_pVehicle->m_pVehicleInfo->dmgIndicShieldHandle );
		}
	}

	//TODO: if we check nextState.brokenLimbs & prevState.brokenLimbs, we can tell when a damage flag has been added and flash that part of the ship
	//FIXME: when ship explodes, either stop drawing ship or draw all parts black
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_FRONT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_BACK);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_LEFT);
	CG_DrawVehicleDamage(veh,brokenLimbs,menuHUD,alpha,VEH_DAMAGE_RIGHT);
}

qboolean CG_DrawVehicleHud( const centity_t *cent )
{
	itemDef_t		*item;
	menuDef_t		*menuHUD;
	playerState_t	*ps;
	centity_t		*veh;
	float			shieldPerc,alpha;

	menuHUD = Menus_FindByName("swoopvehiclehud");
	if (!menuHUD)
	{
		return qtrue;	// Draw player HUD
	}

	ps = &cg.predictedPlayerState;

	if (!ps || !(ps->m_iVehicleNum))
	{
		return qtrue;	// Draw player HUD
	}
	veh = &cg_entities[ps->m_iVehicleNum];

	if ( !veh || !veh->m_pVehicle )
	{
		return qtrue;	// Draw player HUD
	}

	CG_DrawVehicleTurboRecharge( menuHUD, veh );
	CG_DrawVehicleWeaponsLinked( menuHUD, veh );

	item = Menu_FindItemByName(menuHUD, "leftframe");

	// Draw frame
	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	item = Menu_FindItemByName(menuHUD, "rightframe");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}


	CG_DrawVehicleArmor( menuHUD, veh );

	// Get animal hud for speed
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("tauntaunhud");
//	}


	CG_DrawVehicleSpeed( menuHUD, veh );

	// Revert to swoophud
//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
//		menuHUD = Menus_FindByName("swoopvehiclehud");
//	}

//	if (veh->m_pVehicle->m_pVehicleInfo->type == VH_ANIMAL)
//	{
		shieldPerc = CG_DrawVehicleShields( menuHUD, veh );
//	}

	if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && !veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmo( menuHUD, veh );
	}
	else if (veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID && veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID)
	{
		CG_DrawVehicleAmmoUpper( menuHUD, veh );
		CG_DrawVehicleAmmoLower( menuHUD, veh );
	}

	// If he's hidden, he must be in a vehicle
	if (veh->m_pVehicle->m_pVehicleInfo->hideRider)
	{
		CG_DrawVehicleDamageHUD(veh,cg.predictedVehicleState.brokenLimbs,shieldPerc,"vehicledamagehud",1.0f);

		// Has he targeted an enemy?
		if (CG_CheckTargetVehicle( &veh, &alpha ))
		{
			CG_DrawVehicleDamageHUD(veh,veh->currentState.brokenLimbs,((float)veh->currentState.activeForcePass/10.0f),"enemyvehicledamagehud",alpha);
		}

		return qfalse;	// Don't draw player HUD
	}

	return qtrue;	// Draw player HUD

}

/*
================
CG_DrawStats

================
*/
static void CG_DrawStats( void )
{
	centity_t		*cent;
	playerState_t	*ps;
	qboolean		drawHUD = qtrue;
/*	playerState_t	*ps;
	vector3			angles;
//	vector3		origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}
*/
	cent = &cg_entities[cg.snap->ps.clientNum];
/*	ps = &cg.snap->ps;

	VectorClear( angles );

	// Do start
	if (!cg.interfaceStartupDone)
	{
		CG_InterfaceStartup();
	}

	cgi_UI_MenuPaintAll();*/

	if ( cent )
	{
		ps = &cg.predictedPlayerState;

		if ( (ps->m_iVehicleNum ) )	// In a vehicle???
		{
			drawHUD = CG_DrawVehicleHud( cent );
		}
	}

	if (drawHUD)
	{
		CG_DrawHUD(cent);
	}

	/*CG_DrawArmor(cent);
	CG_DrawHealth(cent);
	CG_DrawAmmo(cent);

	CG_DrawTalk(cent);*/
}

/*
===================
CG_DrawPickupItem
===================
*/
static void CG_DrawPickupItem( void ) {
	int		value;
	vector4 *fadeColor;

	value = cg.itemPickup;
	if ( value && cg_items[ value ].icon != -1 )
	{
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor )
		{
			CG_RegisterItemVisuals( value );
			trap->R_SetColor( fadeColor );
			CG_DrawPic( 573, 320, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			trap->R_SetColor( NULL );
		}
	}
}

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vector4 hcolor;

	hcolor.a = alpha;
	if ( team == TEAM_RED ) {
		hcolor.r = 1;
		hcolor.g = .2f;
		hcolor.b = .2f;
	} else if ( team == TEAM_BLUE ) {
		hcolor.r = .2f;
		hcolor.g = .2f;
		hcolor.b = 1;
	} else {
		return;
	}
//	trap->R_SetColor( hcolor );

	CG_FillRect ( x, y, w, h, &hcolor );
//	CG_DrawPic( x, y, w, h, media.gfx.interface.teamStatusBar );
	trap->R_SetColor( NULL );
}


/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawMiniScoreboard
================
*/
static float CG_DrawMiniScoreboard ( float y )
{
	char temp[MAX_QPATH];
	int xOffset = 0;

	if ( !cg_drawScores.integer )
	{
		return y;
	}

	if (cgs.gametype == GT_SIEGE)
	{ //don't bother with this in siege
		return y;
	}

	if ( cgs.gametype >= GT_TEAM )
	{
		strcpy ( temp, va("%s: ", CG_GetStringEdString("MP_INGAME", "RED")));
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );
		Q_strcat ( temp, MAX_QPATH, va(" %s: ", CG_GetStringEdString("MP_INGAME", "BLUE")) );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( 630 - CG_Text_Width ( temp, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, &colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
	}
	else
	{
		/*
		strcpy ( temp, "1st: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores1==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores1)) );

		Q_strcat ( temp, MAX_QPATH, " 2nd: " );
		Q_strcat ( temp, MAX_QPATH, cgs.scores2==SCORE_NOT_PRESENT?"-":(va("%i",cgs.scores2)) );

		CG_Text_Paint( 630 - CG_Text_Width ( temp, 0.7f, FONT_SMALL ), y, 0.7f, colorWhite, temp, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM );
		y += 15;
		*/
		//rww - no longer doing this. Since the attacker now shows who is first, we print the score there.
	}


	return y;
}

/*
================
CG_DrawEnemyInfo
================
*/
static float CG_DrawEnemyInfo ( float y )
{
	float		size;
	int			clientNum;
	const char	*title;
	clientInfo_t	*ci;
	int xOffset = 0;

	if (!cg.snap)
	{
		return y;
	}

	if ( !cg_drawEnemyInfo.integer )
	{
		return y;
	}

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 )
	{
		return y;
	}

	if (cgs.gametype == GT_POWERDUEL)
	{ //just get out of here then
		return y;
	}

	//Raz: Just use the mini-scoreboard
	#ifdef JAPP_MINISCOREBOARD
		if ( cg_miniScoreboard.integer )
			return y;
	#endif

	if ( cgs.gametype == GT_JEDIMASTER )
	{
		//title = "Jedi Master";
		title = CG_GetStringEdString("MP_INGAME", "MASTERY7");
		clientNum = cgs.jediMaster;

		if ( clientNum < 0 )
		{
			//return y;
//			title = "Get Saber!";
			title = CG_GetStringEdString("MP_INGAME", "GET_SABER");


			size = ICON_SIZE * 1.25;
			y += 5;

			CG_DrawPic( 640 - size - 12 + xOffset, y, size, size, media.gfx.interface.weaponIcons[WP_SABER] );

			y += size;

			/*
			CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ), y, 0.7f, &colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
			y += 15;
			*/

			CG_Text_Paint( 630 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, &colorWhite, title, 0, 0, 0, FONT_MEDIUM );

			return y + BIGCHAR_HEIGHT + 2;
		}
	}
	else if ( cg.snap->ps.duelInProgress )
	{
//		title = "Dueling";
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		clientNum = cg.snap->ps.duelIndex;
	}
	else if ( cgs.gametype == GT_DUEL && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		title = CG_GetStringEdString("MP_INGAME", "DUELING");
		if (cg.snap->ps.clientNum == cgs.duelist1)
		{
			clientNum = cgs.duelist2; //if power duel, should actually draw both duelists 2 and 3 I guess
		}
		else if (cg.snap->ps.clientNum == cgs.duelist2)
		{
			clientNum = cgs.duelist1;
		}
		else if (cg.snap->ps.clientNum == cgs.duelist3)
		{
			clientNum = cgs.duelist1;
		}
		else
		{
			return y;
		}
	}
	else
	{
		/*
		title = "Attacker";
		clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];

		if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum )
		{
			return y;
		}

		if ( cg.time - cg.attackerTime > ATTACKER_HEAD_TIME )
		{
			cg.attackerTime = 0;
			return y;
		}
		*/
		//As of current, we don't want to draw the attacker. Instead, draw whoever is in first place.
		if (cgs.duelWinner < 0 || cgs.duelWinner >= MAX_CLIENTS)
		{
			return y;
		}


		title = va("%s: %i",CG_GetStringEdString("MP_INGAME", "LEADER"), cgs.scores1);

		/*
		if (cgs.scores1 == 1)
		{
			title = va("%i kill", cgs.scores1);
		}
		else
		{
			title = va("%i kills", cgs.scores1);
		}
		*/
		clientNum = cgs.duelWinner;
	}

	if ( clientNum >= MAX_CLIENTS || !(&cgs.clientinfo[ clientNum ]) )
	{
		return y;
	}

	ci = &cgs.clientinfo[ clientNum ];

	size = ICON_SIZE * 1.25;
	y += 5;

	if ( ci->modelIcon )
	{
		CG_DrawPic( 640 - size - 5 + xOffset, y, size, size, ci->modelIcon );
	}

	y += size;

//	CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, &colorWhite, ci->name, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint( 630 - CG_Text_Width ( ci->name, 1.0f, FONT_SMALL2 ) + xOffset, y, 1.0f, &colorWhite, ci->name, 0, 0, 0, FONT_SMALL2 );

	y += 15;
//	CG_Text_Paint( 630 - CG_Text_Width ( title, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, &colorWhite, title, 0, 0, 0, FONT_MEDIUM );
	CG_Text_Paint( 630 - CG_Text_Width ( title, 1.0f, FONT_SMALL2 ) + xOffset, y, 1.0f, &colorWhite, title, 0, 0, 0, FONT_SMALL2 );

	if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{//also print their score
		char text[1024];
		y += 15;
		Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[clientNum].score, cgs.fraglimit );
		CG_Text_Paint( 630 - CG_Text_Width ( text, 0.7f, FONT_MEDIUM ) + xOffset, y, 0.7f, &colorWhite, text, 0, 0, 0, FONT_MEDIUM );
	}

// nmckenzie: DUEL_HEALTH - fixme - need checks and such here.  And this is coded to duelist 1 right now, which is wrongly.
	if ( cgs.showDuelHealths >= 2)
	{
		y += 15;
		if ( cgs.duelist1 == clientNum )
		{
			CG_DrawDuelistHealth ( 640 - size - 5 + xOffset, y, 64, 8, 1 );
		}
		else if ( cgs.duelist2 == clientNum )
		{
			CG_DrawDuelistHealth ( 640 - size - 5 + xOffset, y, 64, 8, 2 );
		}
	}

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;
	int			xOffset = 0;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime,
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define IDEAL_FPS (60.0f)
#define	FPS_FRAMES (16)
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static unsigned short previousTimes[FPS_FRAMES];
	static unsigned short index;
	static int	previous, lastupdate;
	int		t, i, fps, total;
	unsigned short frameTime;
	const int		xOffset = 0;
	int maxFPS = atoi( CG_Cvar_VariableString( "com_maxFPS" ) );


	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap->Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 100)	//don't sample faster than this
	{
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ )
		total += previousTimes[i];
	if ( !total )
		total = 1;

	cg.japp.fps = fps = 1000.0f * (float)( (float)(FPS_FRAMES) / (float)total );

	if ( cg_drawFPS.integer )
	{
		vector4 fpsColour = { 1.0f, 1.0f, 1.0f, 1.0f }, fpsGood = { 0.0f, 1.0f, 0.0f, 1.0f }, fpsBad = { 1.0f, 0.0f, 0.0f, 1.0f };
		CG_LerpColour( &fpsBad, &fpsGood, &fpsColour, min( max(0.0f, fps) / max(IDEAL_FPS, maxFPS), 1.0f ) );

		//	s = va( "%ifps", fps );
		//	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		//	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawStringExt( (SCREEN_WIDTH-5) - w + xOffset, y + 2, s, &fpsColour, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
		y += BIGCHAR_HEIGHT + 4;
		//	w = CG_Text_Width( s, 1.0f, FONT_JAPPMONO );
		//	CG_Text_Paint( (SCREEN_WIDTH-5) - w + xOffset, y + 2, 1.0f, &fpsColour, s, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_JAPPMONO );
	}
	if ( cg_drawFPS.integer == 2 )
	{
	//	int font = FONT_JAPPMONO;
	//	float scale = 0.5f;
		s = va( "%i/%3.2f msec", frameTime, 1000.0f/(float)fps );

		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawStringExt( (SCREEN_WIDTH-5) - w + xOffset, y + 2, s, &colorTable[CT_LTGREY], qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );

	//	w = CG_Text_Width( s, scale, font );
	//	CG_Text_Paint( (SCREEN_WIDTH-5) - w - 5, y - 2, scale, &colorTable[CT_LTGREY], s, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, font );

		y += BIGCHAR_HEIGHT + 4;
	}
	return y;
}

// nmckenzie: DUEL_HEALTH
#define MAX_HEALTH_FOR_IFACE	100
void CG_DrawHealthBarRough (float x, float y, int width, int height, float ratio, const vector4 *color1, const vector4 *color2)
{
	float midpoint, remainder;
	vector4 color3 = {1, 0, 0, .7f};

	midpoint = width * ratio - 1;
	remainder = width - midpoint;
	color3.r = color1->r * 0.5f;

	assert(!(height%4));//this won't line up otherwise.
	CG_DrawRect(x + 1,			y + height/2-1,  midpoint,	1,	   height/4+1,  color1);	// creme-y filling.
	CG_DrawRect(x + midpoint,	y + height/2-1,  remainder,	1,	   height/4+1,  &color3);	// used-up-ness.
	CG_DrawRect(x,				y,				 width,		height, 1,			color2);	// hard crispy shell
}

void CG_DrawDuelistHealth ( float x, float y, float w, float h, int duelist )
{
	vector4 duelHealthColor = { 1.0f, 0.0f, 0.0f, 0.7f };
	float healthSrc = 0.0f;
	float ratio;

	if ( duelist == 1 )
		healthSrc = cgs.duelist1health;
	else if (duelist == 2 )
		healthSrc = cgs.duelist2health;

	ratio = healthSrc / MAX_HEALTH_FOR_IFACE;
	if ( ratio > 1.0f )
		ratio = 1.0f;
	if ( ratio < 0.0f )
		ratio = 0.0f;
	duelHealthColor.r = (ratio * 0.2f) + 0.5f;

	CG_DrawHealthBarRough (x, y, w, h, ratio, &duelHealthColor, &colorTable[CT_WHITE]);	// new art for this?  I'm not crazy about how this looks.
}

/*
=====================
CG_DrawRadar
=====================
*/
float	cg_radarRange = 2500.0f;

#define RADAR_RADIUS			60
#define RADAR_X					(580 - RADAR_RADIUS)
#define RADAR_CHAT_DURATION		6000
static int radarLockSoundDebounceTime = 0;
static int impactSoundDebounceTime = 0;
#define	RADAR_MISSILE_RANGE					3000.0f
#define	RADAR_ASTEROID_RANGE				10000.0f
#define	RADAR_MIN_ASTEROID_SURF_WARN_DIST	1200.0f

float CG_DrawRadar ( float y )
{
	vector4			color;
	vector4			teamColor;
	float			arrow_w;
	float			arrow_h;
	clientInfo_t	*cl;
	clientInfo_t	*local;
	int				i;
	float			arrowBaseScale;
	float			zScale;
	int				xOffset = 0;
	refdef_t *refdef = CG_GetRefdef();

	if (!cg.snap)
	{
		return y;
	}

	// Make sure the radar should be showing
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 )
	{
		return y;
	}

	if ( (cg.predictedPlayerState.pm_flags & PMF_FOLLOW) || cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR )
	{
		return y;
	}

	local = &cgs.clientinfo[ cg.snap->ps.clientNum ];
	if ( !local->infoValid )
	{
		return y;
	}

	// Draw the radar background image
	color.r = color.g = color.b = 1.0f;
	color.a = 0.6f;
	trap->R_SetColor ( &color );
	CG_DrawPic( RADAR_X + xOffset, y, RADAR_RADIUS*2, RADAR_RADIUS*2, media.gfx.interface.radar );

	//Always green for your own team.
	VectorCopy4( &g_color_table[ColorIndex(COLOR_GREEN)], &teamColor );
	teamColor.a = 1.0f;

	// Draw all of the radar entities.  Draw them backwards so players are drawn last
	for ( i = cg.radarEntityCount -1 ; i >= 0 ; i-- )
	{
		vector3		dirLook;
		vector3		dirPlayer;
		float		angleLook;
		float		anglePlayer;
		float		angle;
		float		distance, actualDist;
		centity_t*	cent;

		cent = &cg_entities[cg.radarEntities[i]];

		// Get the distances first
		VectorSubtract ( &cg.predictedPlayerState.origin, &cent->lerpOrigin, &dirPlayer );
		dirPlayer.z = 0;
		actualDist = distance = VectorNormalize ( &dirPlayer );

		if ( distance > cg_radarRange * 0.8f)
		{
			if ( (cent->currentState.eFlags & EF_RADAROBJECT)//still want to draw the direction
				|| ( cent->currentState.eType==ET_NPC//FIXME: draw last, with players...
					&& cent->currentState.NPC_class == CLASS_VEHICLE
					&& cent->currentState.speed > 0 ) )//always draw vehicles
			{
				distance = cg_radarRange*0.8f;
			}
			else
			{
				continue;
			}
		}

		distance  = distance / cg_radarRange;
		distance *= RADAR_RADIUS;

		AngleVectors ( &cg.predictedPlayerState.viewangles, &dirLook, NULL, NULL );

		dirLook.z = 0;
		anglePlayer = atan2( dirPlayer.pitch, dirPlayer.yaw );
		VectorNormalize( &dirLook );
		angleLook = atan2( dirLook.pitch, dirLook.yaw );
		angle = angleLook - anglePlayer;

		switch ( cent->currentState.eType )
		{
			default:
				{
					float  x;
					float  ly;
					qhandle_t shader;
					vector4    color;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 9.0f;
					shader = 0;
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin.z > cg.predictedPlayerState.origin.z)
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin.z - cg.predictedPlayerState.origin.z);

						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale += dif;
					}
					else if (cent->lerpOrigin.z < cg.predictedPlayerState.origin.z)
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin.z - cent->lerpOrigin.z);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
						{
							dif = 0.5f;
						}
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if (cent->currentState.brokenLimbs)
					{ //slightly misleading to use this value, but don't want to add more to entstate.
						//any ent with brokenLimbs non-0 and on radar is an objective ent.
						//brokenLimbs is literal team value.
						char objState[1024];
						int complete;

						//we only want to draw it if the objective for it is not complete.
						//frame represents objective num.
						trap->Cvar_VariableStringBuffer(va("team%i_objective%i", cent->currentState.brokenLimbs, cent->currentState.frame), objState, 1024);

						complete = atoi(objState);

						if (!complete)
						{

							// generic enemy index specifies a shader to use for the radar entity.
							if ( cent->currentState.genericenemyindex )
							{
								color.r = color.g = color.b = color.a = 1.0f;
								shader = cgs.gameIcons[cent->currentState.genericenemyindex];
							}
							else
							{
								if (cg.snap && cent->currentState.brokenLimbs == cg.snap->ps.persistant[PERS_TEAM])
									VectorCopy4( &g_color_table[ColorIndex(COLOR_RED)], &color );
								else
									VectorCopy4( &g_color_table[ColorIndex(COLOR_GREEN)], &color );

								shader = media.gfx.interface.siegeItem;
							}
						}
					}
					else
					{
						color.r = color.g = color.b = color.a = 1.0f;

						// generic enemy index specifies a shader to use for the radar entity.
						if ( cent->currentState.genericenemyindex )
							shader = cgs.gameIcons[cent->currentState.genericenemyindex];
						else
							shader = media.gfx.interface.siegeItem;

					}

					if ( shader )
					{
						// Pulse the alpha if time2 is set.  time2 gets set when the entity takes pain
						if ( (cent->currentState.time2 && cg.time - cent->currentState.time2 < 5000) ||
							(cent->currentState.time2 == 0xFFFFFFFF) )
						{
							if ( (cg.time / 200) & 1 )
								color.a = 0.1f + 0.9f * (float) (cg.time % 200) / 200.0f;
							else
								color.a = 1.0f - 0.9f * (float) (cg.time % 200) / 200.0f;
						}

						trap->R_SetColor ( &color );
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, shader );
					}
				}
				break;

			case ET_NPC://FIXME: draw last, with players...
				if ( cent->currentState.NPC_class == CLASS_VEHICLE
					&& cent->currentState.speed > 0 )
				{
					if ( cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->radarIconHandle )
					{
						float  x;
						float  ly;

						x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

						arrowBaseScale = 9.0f;
						zScale = 1.0f;

						//we want to scale the thing up/down based on the relative Z (up/down) positioning
						if (cent->lerpOrigin.z > cg.predictedPlayerState.origin.z)
						{ //higher, scale up (between 16 and 24)
							float dif = (cent->lerpOrigin.z - cg.predictedPlayerState.origin.z);

							//max out to 1.5x scale at 512 units above local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale += dif;
						}
						else if (cent->lerpOrigin.z < cg.predictedPlayerState.origin.z)
						{ //lower, scale down (between 16 and 8)
							float dif = (cg.predictedPlayerState.origin.z - cent->lerpOrigin.z);

							//half scale at 512 units below local player's height
							dif /= 4096.0f;
							if (dif > 0.5f)
							{
								dif = 0.5f;
							}
							zScale -= dif;
						}

						arrowBaseScale *= zScale;

						if ( cent->currentState.m_iVehicleNum //vehicle has a driver
							&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
						{
							if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
							{
								trap->R_SetColor ( &teamColor );
							}
							else
							{
								trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
							}
						}
						else
						{
							trap->R_SetColor ( NULL );
						}
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, cent->m_pVehicle->m_pVehicleInfo->radarIconHandle );
					}
				}
				break; //maybe do something?

			case ET_MOVER:
				if ( cent->currentState.speed//the mover's size, actually
					&& actualDist < (cent->currentState.speed+RADAR_ASTEROID_RANGE)
					&& cg.predictedPlayerState.m_iVehicleNum )
				{//a mover that's close to me and I'm in a vehicle
					qboolean mayImpact = qfalse;
					float surfaceDist = (actualDist-cent->currentState.speed);
					if ( surfaceDist < 0.0f )
					{
						surfaceDist = 0.0f;
					}
					if ( surfaceDist < RADAR_MIN_ASTEROID_SURF_WARN_DIST )
					{//always warn!
						mayImpact = qtrue;
					}
					else
					{//not close enough to always warn, yet, so check its direction
						vector3	asteroidPos, myPos, moveDir;
						int		predictTime, timeStep = 500;
						float	newDist;
						for ( predictTime = timeStep; predictTime < 5000; predictTime+=timeStep )
						{
							//asteroid dir, speed, size, + my dir & speed...
							BG_EvaluateTrajectory( &cent->currentState.pos, cg.time+predictTime, &asteroidPos );
							//FIXME: I don't think it's calcing "myPos" correctly
							AngleVectors( &cg.predictedVehicleState.viewangles, &moveDir, NULL, NULL );
							VectorMA( &cg.predictedVehicleState.origin, cg.predictedVehicleState.speed*predictTime/1000.0f, &moveDir, &myPos );
							newDist = Distance( &myPos, &asteroidPos );
							if ( (newDist-cent->currentState.speed) <= RADAR_MIN_ASTEROID_SURF_WARN_DIST )//200.0f )
							{//heading for an impact within the next 5 seconds
								mayImpact = qtrue;
								break;
							}
						}
					}
					if ( mayImpact )
					{//possible collision
						vector4	asteroidColor = {0.5f,0.5f,0.5f,1.0f};
						float  x;
						float  ly;
						float asteroidScale = (cent->currentState.speed/2000.0f);//average asteroid radius?
						if ( actualDist > RADAR_ASTEROID_RANGE )
						{
							actualDist = RADAR_ASTEROID_RANGE;
						}
						distance = (actualDist/RADAR_ASTEROID_RANGE)*RADAR_RADIUS;

						x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
						ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

						if ( asteroidScale > 3.0f )
							asteroidScale = 3.0f;
						else if ( asteroidScale < 0.2f )
							asteroidScale = 0.2f;
						arrowBaseScale = (9.0f*asteroidScale);
						if ( impactSoundDebounceTime < cg.time )
						{
							vector3	soundOrg;
							if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
								impactSoundDebounceTime = cg.time + 1000;
							else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
								impactSoundDebounceTime = cg.time + 400;
							else
								impactSoundDebounceTime = cg.time + 100;
							VectorMA( &refdef->vieworg, -500.0f*(surfaceDist/RADAR_ASTEROID_RANGE), &dirPlayer, &soundOrg );
							trap->S_StartSound( &soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/common/impactalarm.wav" ) );
						}
						//brighten it the closer it is
						if ( surfaceDist > RADAR_ASTEROID_RANGE*0.66f )
							asteroidColor.r = asteroidColor.g = asteroidColor.b = 0.7f;
						else if ( surfaceDist > RADAR_ASTEROID_RANGE/3.0f )
							asteroidColor.r = asteroidColor.g = asteroidColor.b = 0.85f;
						else
							asteroidColor.r = asteroidColor.g = asteroidColor.b = 1.0f;

						//alpha out the longer it's been since it was considered dangerous
						if ( (cg.time-impactSoundDebounceTime) > 100 )
							asteroidColor.a = (float)((cg.time-impactSoundDebounceTime)-100)/900.0f;

						trap->R_SetColor ( &asteroidColor );
						CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, trap->R_RegisterShaderNoMip( "gfx/menus/radar/asteroid" ) );
					}
				}
				break;

			case ET_MISSILE:
				if ( //cent->currentState.weapon == WP_ROCKET_LAUNCHER &&//a rocket
					cent->currentState.owner > MAX_CLIENTS //belongs to an NPC
					&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE )
				{//a rocket belonging to an NPC, FIXME: only tracking rockets!
					float  x;
					float  ly;

					x = (float)RADAR_X + (float)RADAR_RADIUS + (float)sin (angle) * distance;
					ly = y + (float)RADAR_RADIUS + (float)cos (angle) * distance;

					arrowBaseScale = 3.0f;
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						//if it's targetting me, then play an alarm sound if I'm in a vehicle
						if ( cent->currentState.otherEntityNum == cg.predictedPlayerState.clientNum || cent->currentState.otherEntityNum == cg.predictedPlayerState.m_iVehicleNum )
						{
							if ( radarLockSoundDebounceTime < cg.time )
							{
								vector3	soundOrg;
								int		alarmSound;
								if ( actualDist > RADAR_MISSILE_RANGE * 0.66f )
								{
									radarLockSoundDebounceTime = cg.time + 1000;
									arrowBaseScale = 3.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm1.wav" );
								}
								else if ( actualDist > RADAR_MISSILE_RANGE/3.0f )
								{
									radarLockSoundDebounceTime = cg.time + 500;
									arrowBaseScale = 6.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm2.wav" );
								}
								else
								{
									radarLockSoundDebounceTime = cg.time + 250;
									arrowBaseScale = 9.0f;
									alarmSound = trap->S_RegisterSound( "sound/vehicles/common/lockalarm3.wav" );
								}
								if ( actualDist > RADAR_MISSILE_RANGE )
								{
									actualDist = RADAR_MISSILE_RANGE;
								}
								VectorMA( &refdef->vieworg, -500.0f*(actualDist/RADAR_MISSILE_RANGE), &dirPlayer, &soundOrg );
								trap->S_StartSound( &soundOrg, ENTITYNUM_WORLD, CHAN_AUTO, alarmSound );
							}
						}
					}
					zScale = 1.0f;

					//we want to scale the thing up/down based on the relative Z (up/down) positioning
					if (cent->lerpOrigin.z > cg.predictedPlayerState.origin.z)
					{ //higher, scale up (between 16 and 24)
						float dif = (cent->lerpOrigin.z - cg.predictedPlayerState.origin.z);

						//max out to 1.5x scale at 512 units above local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
							dif = 0.5f;
						zScale += dif;
					}
					else if (cent->lerpOrigin.z < cg.predictedPlayerState.origin.z)
					{ //lower, scale down (between 16 and 8)
						float dif = (cg.predictedPlayerState.origin.z - cent->lerpOrigin.z);

						//half scale at 512 units below local player's height
						dif /= 1024.0f;
						if (dif > 0.5f)
							dif = 0.5f;
						zScale -= dif;
					}

					arrowBaseScale *= zScale;

					if ( cent->currentState.owner >= MAX_CLIENTS//missile owned by an NPC
						&& cg_entities[cent->currentState.owner].currentState.NPC_class == CLASS_VEHICLE//NPC is a vehicle
						&& cg_entities[cent->currentState.owner].currentState.m_iVehicleNum <= MAX_CLIENTS//Vehicle has a player driver
						&& cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1].infoValid ) //player driver is valid
					{
						cl = &cgs.clientinfo[cg_entities[cent->currentState.owner].currentState.m_iVehicleNum-1];
						if ( cl->team == local->team )
						{
							trap->R_SetColor ( &teamColor );
						}
						else
						{
							trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
						}
					}
					else
					{
						trap->R_SetColor ( NULL );
					}
					CG_DrawPic ( x - 4 + xOffset, ly - 4, arrowBaseScale, arrowBaseScale, media.gfx.interface.automap.rocketIcon );
				}
				break;

			case ET_PLAYER:
			{
				vector4 color;

				cl = &cgs.clientinfo[ cent->currentState.number ];

				// not valid then dont draw it
				if ( !cl->infoValid )
				{
					continue;
				}

				VectorCopy4 ( &teamColor, &color );

				arrowBaseScale = 16.0f;
				zScale = 1.0f;

				// Pulse the radar icon after a voice message
				if ( cent->vChatTime + 2000 > cg.time )
				{
					float f = (cent->vChatTime + 2000 - cg.time) / 3000.0f;
					arrowBaseScale = 16.0f + 4.0f * f;
					color.r = teamColor.r + (1.0f - teamColor.r) * f;
					color.g = teamColor.g + (1.0f - teamColor.g) * f;
					color.b = teamColor.b + (1.0f - teamColor.b) * f;
				}

				trap->R_SetColor ( &color );

				//we want to scale the thing up/down based on the relative Z (up/down) positioning
				if (cent->lerpOrigin.z > cg.predictedPlayerState.origin.z)
				{ //higher, scale up (between 16 and 32)
					float dif = (cent->lerpOrigin.z - cg.predictedPlayerState.origin.z);

					//max out to 2x scale at 1024 units above local player's height
					dif /= 1024.0f;
					if (dif > 1.0f)
					{
						dif = 1.0f;
					}
					zScale += dif;
				}
				else if (cent->lerpOrigin.z < cg.predictedPlayerState.origin.z)
				{ //lower, scale down (between 16 and 8)
					float dif = (cg.predictedPlayerState.origin.z - cent->lerpOrigin.z);

					//half scale at 512 units below local player's height
					dif /= 1024.0f;
					if (dif > 0.5f)
					{
						dif = 0.5f;
					}
					zScale -= dif;
				}

				arrowBaseScale *= zScale;

				arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
				arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

				CG_DrawRotatePic2( RADAR_X + RADAR_RADIUS + sinf( angle ) * distance + xOffset, y + RADAR_RADIUS
					+ cosf( angle ) * distance, arrow_w, arrow_h, (360 - cent->lerpAngles.yaw)
					+ cg.predictedPlayerState.viewangles.yaw, media.gfx.interface.automap.playerIcon );
				break;
			}
		}
	}

	arrowBaseScale = 16.0f;

	arrow_w = arrowBaseScale * RADAR_RADIUS / 128;
	arrow_h = arrowBaseScale * RADAR_RADIUS / 128;

	trap->R_SetColor ( &colorWhite );
	CG_DrawRotatePic2( RADAR_X + RADAR_RADIUS + xOffset, y + RADAR_RADIUS, arrow_w, arrow_h,
					   0, media.gfx.interface.automap.playerIcon );

	return y+(RADAR_RADIUS*2);
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
#if 0
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;
	int			xOffset = 0;

	if ( !cg_drawTimer.integer )
		return y;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
#else
//	vector4 *timeColour = NULL;
	int msec=0, secs=0, mins=0, limitSec=cgs.timelimit*60;
	char *s = NULL;
	float w;

	if ( !cg_drawTimer.integer )
		return y;

	msec = cg.time-cgs.levelStartTime;
	secs = msec/1000;
	mins = secs/60;

/*
	timeColour = &colorTable[CT_WHITE];
	if ( cgs.timelimit && (cg_drawTimer.integer & DRAWTIMER_COLOUR) ) {
		// final minute
		if ( secs >= limitSec-60 )
			timeColour = &colorTable[CT_RED];
		// last quarter
		else if ( secs >= limitSec-(limitSec/4) )
			timeColour = &colorTable[CT_HUD_ORANGE];
		// half way
		else if ( secs >= limitSec/2 )
			timeColour = &colorTable[CT_YELLOW];
	}
*/

	if ( cgs.timelimit && (cg_drawTimer.integer & DRAWTIMER_COUNTDOWN) )
	{// count down
		msec = limitSec*1000 - (msec);
		secs = msec/1000;
		mins = secs/60;
	}

	secs %= 60;
//	msec %= 1000;

	s = va( "%i:%02i", mins, secs );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

#endif
	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/
extern const char *CG_GetLocationString(const char *loc); //cg_main.c
static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vector4		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	const gitem_t *item;
	int ret_y, count;
	int xOffset = 0;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	if ( !Server_Supports( SSF_SPECTINFO ) && (cg.snap->ps.pm_flags & PMF_FOLLOW) )
		return y; //Raz: following in spec, not valid info provided

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+i));
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor.r = 1.0f;
		hcolor.g = 0.0f;
		hcolor.b = 0.0f;
		hcolor.a = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor.r = 0.0f;
		hcolor.g = 0.0f;
		hcolor.b = 1.0f;
		hcolor.a = 0.33f;
	}
	trap->R_SetColor( &hcolor );
	CG_DrawPic( x + xOffset, y, w, h, media.gfx.interface.teamStatusBar );
	trap->R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor.r = hcolor.g = hcolor.b = hcolor.a = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx + xOffset, y,
				ci->name, &hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_GetLocationString(CG_ConfigString(CS_LOCATIONS+ci->location));
				if (!p || !*p)
					p = "unknown";

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth +
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx + xOffset, y,
					p, &hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, &hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 +
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx + xOffset, y,
				st, &hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					media.gfx.interface.defer );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx + xOffset, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
						trap->R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
}


static void CG_DrawPowerupIcons(int y)
{
	int j;
	int ico_size = 64;
	//int y = ico_size/2;
	int xOffset = 0;
	const gitem_t	*item;

	trap->R_SetColor( NULL );

	if (!cg.snap)
	{
		return;
	}

	y += 16;

	//Raz: fixed potential buffer overrun of cg.snap->ps.powerups
	for (j = 0; j < PW_NUM_POWERUPS; j++)
	{
		if (cg.snap->ps.powerups[j] > cg.time)
		{
			int secondsleft = (cg.snap->ps.powerups[j] - cg.time)/1000;

			item = BG_FindItemForPowerup( j );

			if (item)
			{
				int icoShader = 0;
				if (cgs.gametype == GT_CTY && (j == PW_REDFLAG || j == PW_BLUEFLAG))
				{
					if (j == PW_REDFLAG)
					{
						icoShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_rflag_ys" );
					}
					else
					{
						icoShader = trap->R_RegisterShaderNoMip( "gfx/hud/mpi_bflag_ys" );
					}
				}
				else
				{
					icoShader = trap->R_RegisterShader( item->icon );
				}

				CG_DrawPic( (640-(ico_size*1.1)) + xOffset, y, ico_size, ico_size, icoShader );

				y += ico_size;

				if (j != PW_REDFLAG && j != PW_BLUEFLAG && secondsleft < 999)
				{
					UI_DrawProportionalString((640-(ico_size*1.1))+(ico_size/2) + xOffset, y-8, va("%i", secondsleft), UI_CENTER | UI_BIGFONT | UI_DROPSHADOW, &colorTable[CT_WHITE]);
				}

				y += (ico_size/3);
			}
		}
	}
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float y = 0;

	if ( cg.mMapChange )
		return;

	trap->R_SetColor( &colorTable[CT_WHITE] );

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 )
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	if ( cg_drawSnapshot.integer )
		y = CG_DrawSnapshot( y );

	y = CG_DrawFPS( y );
	y = CG_DrawTimer( y );

	if ( ( cgs.gametype >= GT_TEAM || cg.predictedPlayerState.m_iVehicleNum )
		&& cg_drawRadar.integer )
	{//draw Radar in Siege mode or when in a vehicle of any kind
		y = CG_DrawRadar ( y );
	}

	y = CG_DrawEnemyInfo ( y );

	y = CG_DrawMiniScoreboard ( y );

	CG_DrawPowerupIcons(y);
}

/*
===================
CG_DrawReward
===================
*/
#ifdef JK2AWARDS
static void CG_DrawReward( void ) {
	vector4	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap->S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap->R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
		x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf ) ) / 2;
		CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap->R_SetColor( NULL );
}
#endif


qboolean CG_DrawMapChange( void )
{
	if ( cg.mMapChange )
	{
		const char *s = NULL;
		int w;

		trap->R_SetColor( NULL );
		CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, trap->R_RegisterShaderNoMip( "menu/art/unknownmap_mp" ) );

		s = CG_GetStringEdString( "MP_INGAME", "SERVER_CHANGING_MAPS" ); // s = "Server Changing Maps";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

		s = CG_GetStringEdString( "MP_INGAME", "PLEASE_WAIT" ); // s = "Please wait...";
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString( 320 - w/2, 200, s, 1.0F);
		return qtrue;
	}

	return qfalse;
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128

static struct lagometer_s {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char	*s = NULL;
	int			w;  // bk010215 - FIXME char message[1024];

	if ( CG_DrawMapChange() )
		return;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap->GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap->GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_GetStringEdString("MP_INGAME", "CONNECTION_INTERRUPTED"); // s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap->R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = 640 - cg.lagometerPos.x;//48;
	y = 480 - cg.lagometerPos.y;//144;

	trap->R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, media.gfx.interface.lagometer );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, media.gfx.world.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, media.gfx.world.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap->R_SetColor( &g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap->R_SetColor( &g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, media.gfx.world.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap->R_SetColor( &g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap->R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, media.gfx.world.whiteShader );
		}
	}

	trap->R_SetColor( NULL );

	if ( cg_noPredict.integer || g_synchronousClients.integer ) {
		CG_DrawBigString( ax, ay, "snc", 1.0 );
	}
	else if ( cg.snap && cg_lagometer.integer == 2 )
		trap->R_Font_DrawString( ax, ay, va( "%i", cg.snap->ping), &colorTable[CT_WHITE], MenuFontToHandle( FONT_SMALL ), -1, 0.5 );

	CG_DrawDisconnect();
}

void CG_DrawSiegeMessage( const char *str, int objectiveScreen )
{
//	if (!( trap->Key_GetCatcher() & KEYCATCH_UI ))
	{
		trap->OpenUIMenu(UIMENU_CLOSEALL);
		trap->Cvar_Set("cg_siegeMessage", str);
		if (objectiveScreen)
		{
			trap->OpenUIMenu(UIMENU_SIEGEOBJECTIVES);
		}
		else
		{
			trap->OpenUIMenu(UIMENU_SIEGEMESSAGE);
		}
	}
}

void CG_DrawSiegeMessageNonMenu( const char *str )
{
	char	text[1024];
	if (str[0]=='@')
	{
		trap->SE_GetStringTextString(str+1, text, sizeof(text));
		str = text;
	}
	CG_CenterPrint(str, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char *s = NULL;
	int i = 0;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		/*
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
		*/
		i++;
		if ( i >= 50 )
		{//maxed out a line of text, this will make the line spill over onto another line.
			i = 0;
			cg.centerPrintLines++;
		}
		else if ( *s == '\n' )
			cg.centerPrintLines++;
		s++;

	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
	int h;
	vector4 *color;
	const float scale = 1.0; //0.5


	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centerTime.value );
	if ( !color ) {
		return;
	}

	trap->R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width(linebuffer, scale, FONT_MEDIUM);
		h = CG_Text_Height(linebuffer, scale, FONT_MEDIUM);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, scale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
		y += h + 6;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap->R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

#define HEALTH_WIDTH		50.0f
#define HEALTH_HEIGHT		5.0f

//see if we can draw some extra info on this guy based on our class
void CG_DrawSiegeInfo(centity_t *cent, float chX, float chY, float chW, float chH)
{
	siegeExtended_t *se = &cg_siegeExtendedData[cent->currentState.number];
	clientInfo_t *ci;
	const char *configstring, *v;
	siegeClass_t *siegeClass;
	vector4 aColor, cColor;
	float x;
	float y;
	float percent;
	int ammoMax;

	assert(cent->currentState.number < MAX_CLIENTS);

	if (se->lastUpdated > cg.time)
	{ //strange, shouldn't happen
		return;
	}

	if ((cg.time - se->lastUpdated) > 10000)
	{ //if you haven't received a status update on this guy in 10 seconds, forget about it
		return;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{ //he's dead, don't display info on him
		return;
	}

	if (cent->currentState.weapon != se->weapon)
	{ //data is invalidated until it syncs back again
		return;
	}

	ci = &cgs.clientinfo[cent->currentState.number];
	if (ci->team != cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //not on the same team
		return;
	}

	configstring = CG_ConfigString( cg.predictedPlayerState.clientNum + CS_PLAYERS );
	v = Info_ValueForKey( configstring, "siegeclass" );

	if (!v || !v[0])
	{ //don't have siege class in info?
		return;
	}

	siegeClass = BG_SiegeFindClassByName(v);

	if (!siegeClass)
	{ //invalid
		return;
	}

    if (!(siegeClass->classflags & (1<<CFL_STATVIEWER)))
	{ //doesn't really have the ability to see others' stats
		return;
	}

	x = chX+((chW/2)-(HEALTH_WIDTH/2));
	y = (chY+chH) + 8.0f;
	percent = ((float)se->health/(float)se->maxhealth)*HEALTH_WIDTH;

	//color of the bar
	aColor.r = 0.0f;
	aColor.g = 1.0f;
	aColor.b = 0.0f;
	aColor.a = 0.4f;

	//color of greyed out "missing health"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, &cColor);


	//now draw his ammo
	ammoMax = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
	{
		ammoMax *= 2;
	}

	x = chX+((chW/2)-(HEALTH_WIDTH/2));
	y = (chY+chH) + HEALTH_HEIGHT + 10.0f;

	if (!weaponData[cent->currentState.weapon].energyPerShot &&
		!weaponData[cent->currentState.weapon].altEnergyPerShot)
	{ //a weapon that takes no ammo, so show full
		percent = HEALTH_WIDTH;
	}
	else
	{
		percent = ((float)se->ammo/(float)ammoMax)*HEALTH_WIDTH;
	}

	//color of the bar
	aColor.r = 1.0f;
	aColor.g = 1.0f;
	aColor.b = 0.0f;
	aColor.a = 0.4f;

	//color of greyed out "missing health"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, &cColor);
}

//draw the health bar based on current "health" and maxhealth
void CG_DrawHealthBar(centity_t *cent, float chX, float chY, float chW, float chH)
{
	vector4 aColor, cColor;
	float x = chX+((chW/2)-(HEALTH_WIDTH/2));
	float y = (chY+chH) + 8.0f;
	float percent = ((float)cent->currentState.health/(float)cent->currentState.maxhealth)*HEALTH_WIDTH;
	if (percent <= 0)
	{
		return;
	}

	//color of the bar
	if (!cent->currentState.teamowner || cgs.gametype < GT_TEAM)
	{ //not owned by a team or teamplay
		aColor.r = 1.0f;
		aColor.g = 1.0f;
		aColor.b = 0.0f;
		aColor.a = 0.4f;
	}
	else if (cent->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
	{ //owned by my team
		aColor.r = 0.0f;
		aColor.g = 1.0f;
		aColor.b = 0.0f;
		aColor.a = 0.4f;
	}
	else
	{ //hostile
		aColor.r = 1.0f;
		aColor.g = 0.0f;
		aColor.b = 0.0f;
		aColor.a = 0.4f;
	}

	//color of greyed out "missing health"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.4f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, &cColor);
}

//same routine (at least for now), draw progress of a "hack" or whatever
void CG_DrawHaqrBar(float chX, float chY, float chW, float chH)
{
	vector4 aColor, cColor;
	float x = chX+((chW/2)-(HEALTH_WIDTH/2));
	float y = (chY+chH) + 8.0f;
	float percent = (((float)cg.predictedPlayerState.hackingTime-(float)cg.time)/(float)cg.predictedPlayerState.hackingBaseTime)*HEALTH_WIDTH;

	if (percent > HEALTH_WIDTH ||
		percent < 1.0f)
	{
		return;
	}

	//color of the bar
	aColor.r = 1.0f;
	aColor.g = 1.0f;
	aColor.b = 0.0f;
	aColor.a = 0.4f;

	//color of greyed out done area
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, HEALTH_WIDTH, HEALTH_HEIGHT, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f, percent-1.0f, HEALTH_HEIGHT-1.0f, &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+percent, y+1.0f, HEALTH_WIDTH-percent-1.0f, HEALTH_HEIGHT-1.0f, &cColor);

	//draw the hacker icon
	CG_DrawPic(x, y-HEALTH_WIDTH, HEALTH_WIDTH, HEALTH_WIDTH, media.gfx.interface.hackerIcon);
}

//generic timing bar
int cg_genericTimerBar = 0;
int cg_genericTimerDur = 0;
vector4 cg_genericTimerColor;
#define CGTIMERBAR_H			50.0f
#define CGTIMERBAR_W			10.0f
#define CGTIMERBAR_X			(SCREEN_WIDTH-CGTIMERBAR_W-120.0f)
#define CGTIMERBAR_Y			(SCREEN_HEIGHT-CGTIMERBAR_H-20.0f)
void CG_DrawGenericTimerBar(void)
{
	vector4 aColor, cColor;
	float x = CGTIMERBAR_X;
	float y = CGTIMERBAR_Y;
	float percent = ((float)(cg_genericTimerBar-cg.time)/(float)cg_genericTimerDur)*CGTIMERBAR_H;

	if (percent > CGTIMERBAR_H)
		return;

	if (percent < 0.1f)
		percent = 0.1f;

	//color of the bar
	aColor.r = cg_genericTimerColor.r;
	aColor.g = cg_genericTimerColor.g;
	aColor.b = cg_genericTimerColor.b;
	aColor.a = cg_genericTimerColor.a;

	//color of greyed out "missing fuel"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, CGTIMERBAR_W, CGTIMERBAR_H, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(CGTIMERBAR_H-percent), CGTIMERBAR_W-2.0f, CGTIMERBAR_H-1.0f-(CGTIMERBAR_H-percent), &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, CGTIMERBAR_W-2.0f, CGTIMERBAR_H-percent, &cColor);
}

/*
=================
CG_DrawCrosshair
=================
*/

float cg_crosshairPrevPosX = 0;
float cg_crosshairPrevPosY = 0;
#define CRAZY_CROSSHAIR_MAX_ERROR_X	(100.0f*640.0f/480.0f)
#define CRAZY_CROSSHAIR_MAX_ERROR_Y	(100.0f)
void CG_LerpCrosshairPos( float *x, float *y )
{
	if ( cg_crosshairPrevPosX )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f) * 640.0f/480.0f;
		float xDiff = (*x - cg_crosshairPrevPosX);
		if ( fabs(xDiff) > CRAZY_CROSSHAIR_MAX_ERROR_X )
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		if ( xDiff > maxMove )
			*x = cg_crosshairPrevPosX + maxMove;
		else if ( xDiff < -maxMove )
			*x = cg_crosshairPrevPosX - maxMove;
	}
	cg_crosshairPrevPosX = *x;

	if ( cg_crosshairPrevPosY )
	{//blend from old pos
		float maxMove = 30.0f * ((float)cg.frametime/500.0f);
		float yDiff = (*y - cg_crosshairPrevPosY);
		if ( fabs(yDiff) > CRAZY_CROSSHAIR_MAX_ERROR_Y )
			maxMove = CRAZY_CROSSHAIR_MAX_ERROR_X;
		if ( yDiff > maxMove )
			*y = cg_crosshairPrevPosY + maxMove;
		else if ( yDiff < -maxMove )
			*y = cg_crosshairPrevPosY - maxMove;
	}
	cg_crosshairPrevPosY = *y;
}

vector3 cg_crosshairPos={0,0,0};
static void CG_DrawCrosshair( vector3 *worldPoint, int chEntValid ) {
	float		w, h;
	qhandle_t	hShader = 0;
	float		f;
	float		x, y;
	qboolean	corona = qfalse;
	vector4		ecolor = {0,0,0,0};
	centity_t	*crossEnt = NULL;
	float		chX, chY;
	refdef_t *refdef = CG_GetRefdef();

	trap->R_SetColor( NULL );

	if ( worldPoint )
		VectorCopy( worldPoint, &cg_crosshairPos );

	if ( !cg_drawCrosshair.integer )
		return;

	if (cg.snap->ps.fallingToDeath)
		return;

	if ( cg.predictedPlayerState.zoomMode != 0 )
		return;

	if ( cg_crosshairHealth.integer )
	{
		vector4 hcolor;

		CG_ColorForHealth( &hcolor );
		trap->R_SetColor( &hcolor );
	}
	else
	{
		//set color based on what kind of ent is under crosshair
		if ( cg.crosshairClientNum >= ENTITYNUM_WORLD )
		{
			trap->R_SetColor( NULL );
		}
		//rwwFIXMEFIXME: Write this a different way, it's getting a bit too sloppy looking
		else if (chEntValid &&
			(cg_entities[cg.crosshairClientNum].currentState.number < MAX_CLIENTS ||
			cg_entities[cg.crosshairClientNum].currentState.eType == ET_NPC ||
			cg_entities[cg.crosshairClientNum].currentState.shouldtarget ||
			cg_entities[cg.crosshairClientNum].currentState.health || //always show ents with health data under crosshair
			(cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER) ||
			(cg_entities[cg.crosshairClientNum].currentState.eType == ET_MOVER && cg_entities[cg.crosshairClientNum].currentState.teamowner)))
		{
			crossEnt = &cg_entities[cg.crosshairClientNum];

			if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
			{ //don't show up for cloaked guys
				ecolor.r = 1.0;//R
				ecolor.g = 1.0;//G
				ecolor.b = 1.0;//B
			}
			else if ( crossEnt->currentState.number < MAX_CLIENTS )
			{
				if (cgs.gametype >= GT_TEAM &&
					cgs.clientinfo[crossEnt->currentState.number].team == cgs.clientinfo[cg.snap->ps.clientNum].team )
				{
					//Allies are green
					ecolor.r = 0.0;//R
					ecolor.g = 1.0;//G
					ecolor.b = 0.0;//B
				}
				else
				{
					if (cgs.gametype == GT_POWERDUEL &&
						cgs.clientinfo[crossEnt->currentState.number].duelTeam == cgs.clientinfo[cg.snap->ps.clientNum].duelTeam)
					{ //on the same duel team in powerduel, so he's a friend
						ecolor.r = 0.0;//R
						ecolor.g = 1.0;//G
						ecolor.b = 0.0;//B
					}
					else
					{ //Enemies are red
						ecolor.r = 1.0;//R
						ecolor.g = 0.0;//G
						ecolor.b = 0.0;//B
					}
				}

				if (cg.snap->ps.duelInProgress)
				{
					if (crossEnt->currentState.number != cg.snap->ps.duelIndex)
					{ //grey out crosshair for everyone but your foe if you're in a duel
						ecolor.r = 0.4f;
						ecolor.g = 0.4f;
						ecolor.b = 0.4f;
					}
				}
				else if (crossEnt->currentState.bolt1)
				{ //this fellow is in a duel. We just checked if we were in a duel above, so
				  //this means we aren't and he is. Which of course means our crosshair greys out over him.
					ecolor.r = 0.4f;
					ecolor.g = 0.4f;
					ecolor.b = 0.4f;
				}
			}
			else if (crossEnt->currentState.shouldtarget || crossEnt->currentState.eType == ET_NPC)
			{
				//VectorCopy( crossEnt->startRGBA, ecolor );
				if ( !ecolor.r && !ecolor.g && !ecolor.b )
				{
					// We really don't want black, so set it to yellow
					ecolor.r = 1.0f;//R
					ecolor.g = 0.8f;//G
					ecolor.b = 0.3f;//B
				}

				if (crossEnt->currentState.eType == ET_NPC)
				{
					int plTeam;
					if (cgs.gametype == GT_SIEGE)
						plTeam = cg.predictedPlayerState.persistant[PERS_TEAM];
					else
						plTeam = NPCTEAM_PLAYER;

					if ( crossEnt->currentState.powerups & (1 <<PW_CLOAKED) )
					{
						ecolor.r = 1.0f;//R
						ecolor.g = 1.0f;//G
						ecolor.b = 1.0f;//B
					}
					else if ( !crossEnt->currentState.teamowner )
					{ //not on a team
						if (!crossEnt->currentState.teamowner ||
							crossEnt->currentState.NPC_class == CLASS_VEHICLE)
						{ //neutral
							if (crossEnt->currentState.owner < MAX_CLIENTS)
							{ //base color on who is pilotting this thing
								clientInfo_t *ci = &cgs.clientinfo[crossEnt->currentState.owner];

								if (cgs.gametype >= GT_TEAM && ci->team == cg.predictedPlayerState.persistant[PERS_TEAM])
								{ //friendly
									ecolor.r = 0.0f;//R
									ecolor.g = 1.0f;//G
									ecolor.b = 0.0f;//B
								}
								else
								{ //hostile
									ecolor.r = 1.0f;//R
									ecolor.g = 0.0f;//G
									ecolor.b = 0.0f;//B
								}
							}
							else
							{ //unmanned
								ecolor.r = 1.0f;//R
								ecolor.g = 1.0f;//G
								ecolor.b = 0.0f;//B
							}
						}
						else
						{
							ecolor.r = 1.0f;//R
							ecolor.g = 0.0f;//G
							ecolor.b = 0.0f;//B
						}
					}
					else if ( crossEnt->currentState.teamowner != plTeam )
					{// on enemy team
						ecolor.r = 1.0f;//R
						ecolor.g = 0.0f;//G
						ecolor.b = 0.0f;//B
					}
					else
					{ //a friend
						ecolor.r = 0.0f;//R
						ecolor.g = 1.0f;//G
						ecolor.b = 0.0f;//B
					}
				}
				else if ( crossEnt->currentState.teamowner == TEAM_RED
					|| crossEnt->currentState.teamowner == TEAM_BLUE )
				{
					if (cgs.gametype < GT_TEAM)
					{ //not teamplay, just neutral then
						ecolor.r = 1.0f;//R
						ecolor.g = 1.0f;//G
						ecolor.b = 0.0f;//B
					}
					else if ( crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team )
					{ //on the enemy team
						ecolor.r = 1.0f;//R
						ecolor.g = 0.0f;//G
						ecolor.b = 0.0f;//B
					}
					else
					{ //on my team
						ecolor.r = 0.0f;//R
						ecolor.g = 1.0f;//G
						ecolor.b = 0.0f;//B
					}
				}
				else if (crossEnt->currentState.owner == cg.snap->ps.clientNum ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner == cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor.r = 0.0f;//R
					ecolor.g = 1.0f;//G
					ecolor.b = 0.0f;//B
				}
				else if (crossEnt->currentState.teamowner == 16 ||
					(cgs.gametype >= GT_TEAM && crossEnt->currentState.teamowner && crossEnt->currentState.teamowner != cgs.clientinfo[cg.snap->ps.clientNum].team))
				{
					ecolor.r = 1.0f;//R
					ecolor.g = 0.0f;//G
					ecolor.b = 0.0f;//B
				}
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.bolt1 && cg.predictedPlayerState.weapon == WP_SABER)
			{ //can push/pull this mover. Only show it if we're using the saber.
				ecolor.r = 0.2f;
				ecolor.g = 0.5f;
				ecolor.b = 1.0f;

				corona = qtrue;
			}
			else if (crossEnt->currentState.eType == ET_MOVER && crossEnt->currentState.teamowner)
			{ //a team owns this - if it's my team green, if not red, if not teamplay then yellow
				if (cgs.gametype < GT_TEAM)
				{
					ecolor.r = 1.0f;//R
					ecolor.g = 1.0f;//G
					ecolor.b = 0.0f;//B
				}
                else if (cg.predictedPlayerState.persistant[PERS_TEAM] != crossEnt->currentState.teamowner)
				{ //not my team
					ecolor.r = 1.0f;//R
					ecolor.g = 0.0f;//G
					ecolor.b = 0.0f;//B
				}
				else
				{ //my team
					ecolor.r = 0.0f;//R
					ecolor.g = 1.0f;//G
					ecolor.b = 0.0f;//B
				}
			}
			else if (crossEnt->currentState.health)
			{
				if (!crossEnt->currentState.teamowner || cgs.gametype < GT_TEAM)
				{ //not owned by a team or teamplay
					ecolor.r = 1.0f;
					ecolor.g = 1.0f;
					ecolor.b = 0.0f;
				}
				else if (crossEnt->currentState.teamowner == cg.predictedPlayerState.persistant[PERS_TEAM])
				{ //owned by my team
					ecolor.r = 0.0f;
					ecolor.g = 1.0f;
					ecolor.b = 0.0f;
				}
				else
				{ //hostile
					ecolor.r = 1.0f;
					ecolor.g = 0.0f;
					ecolor.b = 0.0f;
				}
			}

			ecolor.a = 1.0f;

			trap->R_SetColor( &ecolor );
		}
	}

	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle )
		{
			hShader = vehCent->m_pVehicle->m_pVehicleInfo->crosshairShaderHandle;
		}
		//bigger by default
		w = cg_crosshairSize.value*2.0f;
		h = w;
	}
	else
	{
		w = h = cg_crosshairSize.value;
	}

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( worldPoint && VectorLength( worldPoint ) )
	{
		if ( !CG_WorldCoordToScreenCoordFloat( worldPoint, &x, &y ) )
		{//off screen, don't draw it
			return;
		}
		//CG_LerpCrosshairPos( &x, &y );
		x -= 320;
		y -= 240;
	}
	else
	{
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
	}

	if ( !hShader )
	{
		hShader = media.gfx.interface.crosshairs[cg_drawCrosshair.integer % NUM_CROSSHAIRS];
	}

	chX = x + refdef->x + 0.5 * (640 - w);
	chY = y + refdef->y + 0.5 * (480 - h);
	trap->R_DrawStretchPic( chX, chY, w, h, 0, 0, 1, 1, hShader );

	//draw a health bar directly under the crosshair if we're looking at something
	//that takes damage
	if (crossEnt &&
		crossEnt->currentState.maxhealth)
	{
		CG_DrawHealthBar(crossEnt, chX, chY, w, h);
		chY += HEALTH_HEIGHT*2;
	}
	else if (crossEnt && crossEnt->currentState.number < MAX_CLIENTS)
	{
		if (cgs.gametype == GT_SIEGE)
		{
			CG_DrawSiegeInfo(crossEnt, chX, chY, w, h);
			chY += HEALTH_HEIGHT*4;
		}
		if (cg.crosshairVehNum && cg.time == cg.crosshairVehTime)
		{ //it was in the crosshair this frame
			centity_t *hisVeh = &cg_entities[cg.crosshairVehNum];

			if (hisVeh->currentState.eType == ET_NPC &&
				hisVeh->currentState.NPC_class == CLASS_VEHICLE &&
				hisVeh->currentState.maxhealth &&
				hisVeh->m_pVehicle)
			{ //draw the health for this vehicle
				CG_DrawHealthBar(hisVeh, chX, chY, w, h);
				chY += HEALTH_HEIGHT*2;
			}
		}
	}

	if (cg.predictedPlayerState.hackingTime)
	{ //hacking something
		CG_DrawHaqrBar(chX, chY, w, h);
	}

	if (cg_genericTimerBar > cg.time)
	{ //draw generic timing bar, can be used for whatever
		CG_DrawGenericTimerBar();
	}

	if ( corona ) // drawing extra bits
	{
		ecolor.a = 0.5f;
		ecolor.r = ecolor.g = ecolor.b = (1 - ecolor.a) * ( sin( cg.time * 0.001f ) * 0.08f + 0.35f ); // don't draw full color
		ecolor.a = 1.0f;

		trap->R_SetColor( &ecolor );

		w *= 2.0f;
		h *= 2.0f;

		trap->R_DrawStretchPic( x + refdef->x + 0.5 * (640 - w),
			y + refdef->y + 0.5 * (480 - h),
			w, h, 0, 0, 1, 1, media.gfx.interface.forceCorona );
	}
	trap->R_SetColor( NULL );
}

/*
================
CG_WorldToScreen
================
*/

qboolean CG_WorldCoordToScreenCoordFloat( const vector3 *point, float *x, float *y ) {
	vector3 trans;
	float xc, yc;
	float px, py;
	float z;
	refdef_t *refdef = CG_GetRefdef();
	const float epsilon = 0.001f;

	px = tanf( refdef->fov_x * M_PI / 360.0f );
	py = tanf( refdef->fov_y * M_PI / 360.0f );

	VectorSubtract( point, &refdef->vieworg, &trans );

	xc = ( SCREEN_WIDTH * cg_viewSize.integer ) / 200.0f;
	yc = ( SCREEN_HEIGHT * cg_viewSize.integer ) / 200.0f;

	z = DotProduct( &trans, &refdef->viewaxis[0] );
	if ( z <= epsilon )
		return qfalse;

	if ( x )
		*x = (SCREEN_WIDTH/2.0f) - DotProduct( &trans, &refdef->viewaxis[1] ) * xc / (z * px);

	if ( y )
		*y = (SCREEN_HEIGHT/2.0f) - DotProduct( &trans, &refdef->viewaxis[2] ) * yc / (z * py);

	return qtrue;
}

qboolean CG_WorldCoordToScreenCoord( vector3 *worldCoord, int *x, int *y ) {
	float xF, yF;
	if ( CG_WorldCoordToScreenCoordFloat( worldCoord, &xF, &yF ) ) {
		*x = (int)xF;
		*y = (int)yF;
		return qtrue;
	}
	return qfalse;
}

/*
====================
CG_SaberClashFlare
====================
*/
int cg_saberFlashTime = 0;
vector3 cg_saberFlashPos = {0, 0, 0};
void CG_SaberClashFlare( void )
{
	int				t, maxTime = 150;
	vector3 dif;
	vector4 color;
	int x,y;
	float v, len;
	trace_t trace;
	refdef_t *refdef = CG_GetRefdef();

	t = cg.time - cg_saberFlashTime;

	if ( t <= 0 || t >= maxTime )
	{
		return;
	}

	// Don't do clashes for things that are behind us
	VectorSubtract( &cg_saberFlashPos, &refdef->vieworg, &dif );

	if ( DotProduct( &dif, &refdef->viewaxis[0] ) < 0.2 )
	{
		return;
	}

	CG_Trace( &trace, &refdef->vieworg, NULL, NULL, &cg_saberFlashPos, -1, CONTENTS_SOLID );

	if ( trace.fraction < 1.0f )
	{
		return;
	}

	len = VectorNormalize( &dif );

	// clamp to a known range
	/*
	if ( len > 800 )
	{
		len = 800;
	}
	*/
	if ( len > 1200 )
	{
		return;
	}

	v = ( 1.0f - ((float)t / maxTime )) * ((1.0f - ( len / 800.0f )) * 2.0f + 0.35f);
	if (v < 0.001f)
	{
		v = 0.001f;
	}

	CG_WorldCoordToScreenCoord( &cg_saberFlashPos, &x, &y );

	color.r = 0.8f;
	color.g = 0.8f;
	color.b = 0.8f;

	trap->R_SetColor( &color );

	CG_DrawPic( x - ( v * 300 ), y - ( v * 300 ),
				v * 600, v * 600,
				trap->R_RegisterShader( "gfx/effects/saberFlare" ));
}

void CG_DottedLine( float x1, float y1, float x2, float y2, float dotSize, int numDots, const vector4 *color, float alpha )
{
	float x, y, xDiff, yDiff, xStep, yStep;
	vector4 colorRGBA;
	int dotNum = 0;

	VectorCopy4( color, &colorRGBA );
	colorRGBA.a = alpha;

	trap->R_SetColor( &colorRGBA );

	xDiff = x2-x1;
	yDiff = y2-y1;
	xStep = xDiff/(float)numDots;
	yStep = yDiff/(float)numDots;

	for ( dotNum = 0; dotNum < numDots; dotNum++ )
	{
		x = x1 + (xStep*dotNum) - (dotSize*0.5f);
		y = y1 + (yStep*dotNum) - (dotSize*0.5f);

		CG_DrawPic( x, y, dotSize, dotSize, media.gfx.world.whiteShader );
	}
}

void CG_BracketEntity( centity_t *cent, float radius )
{
	trace_t tr;
	vector3 dif;
	float	len, size, lineLength, lineWidth;
	float	x,	y;
	clientInfo_t *local;
	qboolean isEnemy = qfalse;
	refdef_t *refdef = CG_GetRefdef();

	VectorSubtract( &cent->lerpOrigin, &refdef->vieworg, &dif );
	len = VectorNormalize( &dif );

	if ( cg.crosshairClientNum != cent->currentState.clientNum
		&& (!cg.snap||cg.snap->ps.rocketLockIndex!= cent->currentState.clientNum) )
	{//if they're the entity you're locking onto or under your crosshair, always draw bracket
		//Hmm... for now, if they're closer than 2000, don't bracket?
		if ( len < 2000.0f )
		{
			return;
		}

		CG_Trace( &tr, &refdef->vieworg, NULL, NULL, &cent->lerpOrigin, -1, CONTENTS_OPAQUE );

		//don't bracket if can't see them
		if ( tr.fraction < 1.0f )
		{
			return;
		}
	}

	if ( !CG_WorldCoordToScreenCoordFloat(&cent->lerpOrigin, &x, &y) )
	{//off-screen, don't draw it
		return;
	}

	//just to see if it's centered
	//CG_DrawPic( x-2, y-2, 4, 4, media.gfx.world.whiteShader );

	local = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( cent->currentState.m_iVehicleNum //vehicle has a driver
		&& cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].infoValid )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cgs.clientinfo[ cent->currentState.m_iVehicleNum-1 ].team == local->team )
		{
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_GREEN)] );
		}
		else
		{
			isEnemy = qtrue;
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
		}
	}
	else if ( cent->currentState.teamowner )
	{
		if ( cgs.gametype < GT_TEAM )
		{//ffa?
			isEnemy = qtrue;
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
		}
		else if ( cent->currentState.teamowner != cg.predictedPlayerState.persistant[PERS_TEAM] )
		{// on enemy team
			isEnemy = qtrue;
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
		}
		else
		{ //a friend
			trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_GREEN)] );
		}
	}
	else
	{//FIXME: if we want to ever bracket anything besides vehicles (like siege objectives we want to blow up), we should handle the coloring here
		trap->R_SetColor ( NULL );
	}

	if ( len <= 1.0f )
	{//super-close, max out at 400 times radius (which is HUGE)
		size = radius*400.0f;
	}
	else
	{//scale by dist
		size = radius*(400.0f/len);
	}

	if ( size < 1.0f )
	{
		size = 1.0f;
	}

	//length scales with dist
	lineLength = (size*0.1f);
	if ( lineLength < 0.5f )
	{//always visible
		lineLength = 0.5f;
	}
	//always visible width
	lineWidth = 1.0f;

	x -= (size*0.5f);
	y -= (size*0.5f);

	/*
	if ( x >= 0 && x <= 640
		&& y >= 0 && y <= 480 )
	*/
	{//brackets would be drawn on the screen, so draw them
	//upper left corner
		//horz
        CG_DrawPic( x, y, lineLength, lineWidth, media.gfx.world.whiteShader );
		//vert
        CG_DrawPic( x, y, lineWidth, lineLength, media.gfx.world.whiteShader );
	//upper right corner
		//horz
        CG_DrawPic( x+size-lineLength, y, lineLength, lineWidth, media.gfx.world.whiteShader );
		//vert
        CG_DrawPic( x+size-lineWidth, y, lineWidth, lineLength, media.gfx.world.whiteShader );
	//lower left corner
		//horz
        CG_DrawPic( x, y+size-lineWidth, lineLength, lineWidth, media.gfx.world.whiteShader );
		//vert
        CG_DrawPic( x, y+size-lineLength, lineWidth, lineLength, media.gfx.world.whiteShader );
	//lower right corner
		//horz
        CG_DrawPic( x+size-lineLength, y+size-lineWidth, lineLength, lineWidth, media.gfx.world.whiteShader );
		//vert
        CG_DrawPic( x+size-lineWidth, y+size-lineLength, lineWidth, lineLength, media.gfx.world.whiteShader );
	}
	//Lead Indicator...
	if ( cg_drawVehLeadIndicator.integer )
	{//draw the lead indicator
		if ( isEnemy )
		{//an enemy object
			if ( cent->currentState.NPC_class == CLASS_VEHICLE )
			{//enemy vehicle
				if ( !VectorCompare( &cent->currentState.pos.trDelta, &vec3_origin ) )
				{//enemy vehicle is moving
					if ( cg.predictedPlayerState.m_iVehicleNum )
					{//I'm in a vehicle
						centity_t		*veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
						if ( veh //vehicle cent
							&& veh->m_pVehicle//vehicle
							&& veh->m_pVehicle->m_pVehicleInfo//vehicle stats
							&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE )//valid vehicle weapon
						{
							vehWeaponInfo_t *vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
							if ( vehWeapon
								&& vehWeapon->bIsProjectile//primary weapon's shot is a projectile
								&& !vehWeapon->bHasGravity//primary weapon's shot is not affected by gravity
								&& !vehWeapon->fHoming//primary weapon's shot is not homing
								&& vehWeapon->fSpeed )//primary weapon's shot has speed
							{//our primary weapon's projectile has a speed
								vector3 vehDiff, vehLeadPos;
								float vehDist, eta;
								float leadX, leadY;

								VectorSubtract( &cent->lerpOrigin, &cg.predictedVehicleState.origin, &vehDiff );
								vehDist = VectorNormalize( &vehDiff );
								eta = (vehDist/vehWeapon->fSpeed);//how many seconds it would take for my primary weapon's projectile to get from my ship to theirs
								//now extrapolate their position that number of seconds into the future based on their velocity
								VectorMA( &cent->lerpOrigin, eta, &cent->currentState.pos.trDelta, &vehLeadPos );
								//now we have where we should be aiming at, project that onto the screen at a 2D co-ord
								if ( !CG_WorldCoordToScreenCoordFloat(&cent->lerpOrigin, &x, &y) )
								{//off-screen, don't draw it
									return;
								}
								if ( !CG_WorldCoordToScreenCoordFloat(&vehLeadPos, &leadX, &leadY) )
								{//off-screen, don't draw it
									//just draw the line
									CG_DottedLine( x, y, x, y, 1, 10, &g_color_table[ColorIndex(COLOR_RED)], 0.5f );
									return;
								}
								//draw a line from the ship's cur pos to the lead pos
								CG_DottedLine( x, y, leadX, leadY, 1, 10, &g_color_table[ColorIndex(COLOR_RED)], 0.5f );
								//now draw the lead indicator
								trap->R_SetColor ( &g_color_table[ColorIndex(COLOR_RED)] );
								CG_DrawPic( leadX-8, leadY-8, 16, 16, trap->R_RegisterShader( "gfx/menus/radar/lead" ) );
							}
						}
					}
				}
			}
		}
	}
}

qboolean CG_InFighter( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER )
		{//I'm in a fighter
			return qtrue;
		}
	}
	return qfalse;
}

qboolean CG_InATST( void )
{
	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//I'm in a vehicle
		centity_t *vehCent = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
	    if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->m_pVehicle->m_pVehicleInfo
			&& vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
		{//I'm in an atst
			return qtrue;
		}
	}
	return qfalse;
}

void CG_DrawBracketedEntities( void )
{
	int i;
	for ( i = 0; i < cg.bracketedEntityCount; i++ )
	{
		centity_t *cent = &cg_entities[cg.bracketedEntities[i]];
		CG_BracketEntity( cent, CG_RadiusForCent( cent ) );
	}
}

//--------------------------------------------------------------
static void CG_DrawHolocronIcons(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = 10;
	int starty = 10;//SCREEN_HEIGHT - icon_size*3;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (cg.snap->ps.holocronBits & (1 << forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, media.gfx.interface.forcePowerIcons[forcePowerSorted[i]]);
			starty += (icon_size+2); //+2 for spacing
			if ((starty+icon_size) >= SCREEN_HEIGHT-80)
			{
				starty = 10;//SCREEN_HEIGHT - icon_size*3;
				startx += (icon_size+2);
			}
		}

		i++;
	}
}

static qboolean CG_IsDurationPower(int power)
{
	if (power == FP_HEAL ||
		power == FP_SPEED ||
		power == FP_TELEPATHY ||
		power == FP_RAGE ||
		power == FP_PROTECT ||
		power == FP_ABSORB ||
		power == FP_SEE)
	{
		return qtrue;
	}

	return qfalse;
}

//--------------------------------------------------------------
static void CG_DrawActivePowers(void)
//--------------------------------------------------------------
{
	int icon_size = 40;
	int i = 0;
	int startx = icon_size*2+16;
	int starty = SCREEN_HEIGHT - icon_size*2;

	int endx = icon_size;
	int endy = icon_size;

	if (cg.snap->ps.zoomMode)
	{ //don't display over zoom mask
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	trap->R_SetColor( NULL );

	while (i < NUM_FORCE_POWERS)
	{
		if ((cg.snap->ps.fd.forcePowersActive & (1 << forcePowerSorted[i])) &&
			CG_IsDurationPower(forcePowerSorted[i]))
		{
			CG_DrawPic( startx, starty, endx, endy, media.gfx.interface.forcePowerIcons[forcePowerSorted[i]]);
			startx += (icon_size+2); //+2 for spacing
			if ((startx+icon_size) >= SCREEN_WIDTH-80)
			{
				startx = icon_size*2+16;
				starty += (icon_size+2);
			}
		}

		i++;
	}

	//additionally, draw an icon force force rage recovery
	if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawPic( startx, starty, endx, endy, media.gfx.interface.rageRecovery);
	}
}

//--------------------------------------------------------------
static void CG_DrawRocketLocking( int lockEntNum, int lockTime )
//--------------------------------------------------------------
{
	int		cx, cy;
	vector3	org;
	static	int oldDif = 0;
	centity_t *cent = &cg_entities[lockEntNum];
	vector4 color={0.0f,0.0f,0.0f,0.0f};
	float lockTimeInterval = ((cgs.gametype==GT_SIEGE)?2400.0f:1200.0f)/16.0f;
	//FIXME: if in a vehicle, use the vehicle's lockOnTime...
	int dif = (cg.time - cg.snap->ps.rocketLockTime)/lockTimeInterval;
	int i;
	refdef_t *refdef = CG_GetRefdef();

	if (!cg.snap->ps.rocketLockTime)
	{
		return;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
	{
		return;
	}

	if ( cg.snap->ps.m_iVehicleNum )
	{//driving a vehicle
		centity_t *veh = &cg_entities[cg.snap->ps.m_iVehicleNum];
		if ( veh->m_pVehicle )
		{
			vehWeaponInfo_t *vehWeapon = NULL;
			if ( cg.predictedVehicleState.weaponstate == WEAPON_CHARGING_ALT )
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[1].ID];
				}
			}
			else
			{
				if ( veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID > VEH_WEAPON_BASE
					&& veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID < MAX_VEH_WEAPONS )
				{
					vehWeapon = &g_vehWeaponInfo[veh->m_pVehicle->m_pVehicleInfo->weapon[0].ID];
				}
			}
			if ( vehWeapon != NULL )
			{//we are trying to lock on with a valid vehicle weapon, so use *its* locktime, not the hard-coded one
				if ( !vehWeapon->iLockOnTime )
				{//instant lock-on
					dif = 10.0f;
				}
				else
				{//use the custom vehicle lockOnTime
					lockTimeInterval = (vehWeapon->iLockOnTime/16.0f);
					dif = (cg.time - cg.snap->ps.rocketLockTime)/lockTimeInterval;
				}
			}
		}
	}
	//We can't check to see in pmove if players are on the same team, so we resort
	//to just not drawing the lock if a teammate is the locked on ent
	if (cg.snap->ps.rocketLockIndex >= 0 &&
		cg.snap->ps.rocketLockIndex < ENTITYNUM_NONE)
	{
		clientInfo_t *ci = NULL;

		if (cg.snap->ps.rocketLockIndex < MAX_CLIENTS)
		{
			ci = &cgs.clientinfo[cg.snap->ps.rocketLockIndex];
		}
		else
		{
			ci = cg_entities[cg.snap->ps.rocketLockIndex].npcClient;
		}

		if (ci)
		{
			if (ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
			{
				if (cgs.gametype >= GT_TEAM)
				{
					return;
				}
			}
			else if (cgs.gametype >= GT_TEAM)
			{
				centity_t *hitEnt = &cg_entities[cg.snap->ps.rocketLockIndex];
				if (hitEnt->currentState.eType == ET_NPC &&
					hitEnt->currentState.NPC_class == CLASS_VEHICLE &&
					hitEnt->currentState.owner < ENTITYNUM_WORLD)
				{ //this is a vehicle, if it has a pilot and that pilot is on my team, then...
					if (hitEnt->currentState.owner < MAX_CLIENTS)
					{
						ci = &cgs.clientinfo[hitEnt->currentState.owner];
					}
					else
					{
						ci = cg_entities[hitEnt->currentState.owner].npcClient;
					}
					if (ci && ci->team == cgs.clientinfo[cg.snap->ps.clientNum].team)
					{
						return;
					}
				}
			}
		}
	}

	if (cg.snap->ps.rocketLockTime != -1)
	{
		lastvalidlockdif = dif;
	}
	else
	{
		dif = lastvalidlockdif;
	}

	if ( !cent )
	{
		return;
	}

	VectorCopy( &cent->lerpOrigin, &org );

	if ( CG_WorldCoordToScreenCoord( &org, &cx, &cy ))
	{
		// we care about distance from enemy to eye, so this is good enough
		float sz = Distance( &cent->lerpOrigin, &refdef->vieworg ) / 1024.0f;

		if ( sz > 1.0f )
		{
			sz = 1.0f;
		}
		else if ( sz < 0.0f )
		{
			sz = 0.0f;
		}

		sz = (1.0f - sz) * (1.0f - sz) * 32 + 6;

		cy += sz * 0.5f;

		if ( dif < 0 )
		{
			oldDif = 0;
			return;
		}
		else if ( dif > 8 )
		{
			dif = 8;
		}

		// do sounds
		if ( oldDif != dif )
		{
			if ( dif == 8 ) {
				if ( cg.snap->ps.m_iVehicleNum )	trap->S_StartSound( &org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/weapons/common/lock.wav" ));
				else								trap->S_StartSound( &org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/rocket/lock.wav" ));
			}
			else {
				if ( cg.snap->ps.m_iVehicleNum )	trap->S_StartSound( &org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/vehicles/weapons/common/tick.wav" ));
				else								trap->S_StartSound( &org, 0, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/rocket/tick.wav" ));
			}
		}

		oldDif = dif;

		for ( i = 0; i < dif; i++ )
		{
			color.r = 1.0f;
			color.g = 0.0f;
			color.b = 0.0f;
			color.a = 0.1f * i + 0.2f;

			trap->R_SetColor( &color );

			// our slices are offset by about 45 degrees.
			CG_DrawRotatePic( cx - sz, cy - sz, sz, sz, i * 45.0f, trap->R_RegisterShaderNoMip( "gfx/2d/wedge" ));
		}

		// we are locked and loaded baby
		if ( dif == 8 )
		{
			color.r = color.g = color.b = sin( cg.time * 0.05f ) * 0.5f + 0.5f;
			color.a = 1.0f; // this art is additive, so the alpha value does nothing

			trap->R_SetColor( &color );

			CG_DrawPic( cx - sz, cy - sz * 2, sz * 2, sz * 2, trap->R_RegisterShaderNoMip( "gfx/2d/lock" ));
		}
	}
}

extern void CG_CalcVehMuzzle(Vehicle_t *pVeh, centity_t *ent, int muzzleNum);
qboolean CG_CalcVehicleMuzzlePoint( int entityNum, vector3 *start, vector3 *d_f, vector3 *d_rt, vector3 *d_up)
{
	centity_t *vehCent = &cg_entities[entityNum];
	if ( vehCent->m_pVehicle && vehCent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
	{//draw from barrels
		VectorCopy( &vehCent->lerpOrigin, start );
		start->z += vehCent->m_pVehicle->m_pVehicleInfo->height-DEFAULT_MINS_2-48;
		AngleVectors( &vehCent->lerpAngles, d_f, d_rt, d_up );
		/*
		mdxaBone_t		boltMatrix;
		int				bolt;
		vector3			yawOnlyAngles;

		VectorSet( yawOnlyAngles, 0, vehCent->lerpAngles.yaw, 0 );

		bolt = trap->G2API_AddBolt( vehCent->ghoul2, 0, "*flash1");
		trap->G2API_GetBoltMatrix( vehCent->ghoul2, 0, bolt, &boltMatrix,
									yawOnlyAngles, vehCent->lerpOrigin, cg.time,
									NULL, vehCent->modelScale );

		// work the matrix axis stuff into the original axis and origins used.
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
		BG_GiveMeVectorFromMatrix( &boltMatrix, POSITIVE_X, d_f );
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
		*/
	}
	else
	{
		//check to see if we're a turret gunner on this vehicle
		if ( cg.predictedPlayerState.generic1 )//as a passenger
		{//passenger in a vehicle
			if ( vehCent->m_pVehicle
				&& vehCent->m_pVehicle->m_pVehicleInfo
				&& vehCent->m_pVehicle->m_pVehicleInfo->maxPassengers )
			{//a vehicle capable of carrying passengers
				int turretNum;
				for ( turretNum = 0; turretNum < MAX_VEHICLE_TURRETS; turretNum++ )
				{
					if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iAmmoMax )
					{// valid turret
						if ( vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].passengerNum == cg.predictedPlayerState.generic1 )
						{//I control this turret
							//Go through all muzzles, average their positions and directions and use the result for crosshair trace
							int vehMuzzle, numMuzzles = 0;
							vector3	muzzlesAvgPos={0},muzzlesAvgDir={0};
							int	i;

							for ( i = 0; i < MAX_VEHICLE_TURRET_MUZZLES; i++ )
							{
								vehMuzzle = vehCent->m_pVehicle->m_pVehicleInfo->turret[turretNum].iMuzzle[i];
								if ( vehMuzzle )
								{
									vehMuzzle -= 1;
									CG_CalcVehMuzzle( vehCent->m_pVehicle, vehCent, vehMuzzle );
									VectorAdd( &muzzlesAvgPos, &vehCent->m_pVehicle->m_vMuzzlePos[vehMuzzle], &muzzlesAvgPos );
									VectorAdd( &muzzlesAvgDir, &vehCent->m_pVehicle->m_vMuzzleDir[vehMuzzle], &muzzlesAvgDir );
									numMuzzles++;
								}
								if ( numMuzzles )
								{
									VectorScale( &muzzlesAvgPos, 1.0f/(float)numMuzzles, start );
									VectorScale( &muzzlesAvgDir, 1.0f/(float)numMuzzles, d_f );
									VectorClear( d_rt );
									VectorClear( d_up );
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
		VectorCopy( &vehCent->lerpOrigin, start );
		AngleVectors( &vehCent->lerpAngles, d_f, d_rt, d_up );
	}
	return qfalse;
}

//calc the muzzle point from the e-web itself
void CG_CalcEWebMuzzlePoint(centity_t *cent, vector3 *start, vector3 *d_f, vector3 *d_rt, vector3 *d_up)
{
	int bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*cannonflash");

//	assert(bolt != -1);

	if (bolt != -1)
	{
		mdxaBone_t boltMatrix;

		trap->G2API_GetBoltMatrix_NoRecNoRot(cent->ghoul2, 0, bolt, &boltMatrix, &cent->lerpAngles, &cent->lerpOrigin, cg.time, NULL, &cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, start);
		BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d_f);

		//these things start the shot a little inside the bbox to assure not starting in something solid
		VectorMA(start, -16.0f, d_f, start);

		//I guess
		VectorClear( d_rt );//don't really need this, do we?
		VectorClear( d_up );//don't really need this, do we?
	}
}

/*
=================
CG_`Entity
=================
*/
#define MAX_XHAIR_DIST_ACCURACY	20000.0f
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vector3		start, end;
	int			content;
	int			ignore;
	qboolean	bVehCheckTraceFromCamPos = qfalse;
	refdef_t *refdef = CG_GetRefdef();
	int traces = 0;

	ignore = cg.predictedPlayerState.clientNum;

	if ( cg_dynamicCrosshair.integer )
	{
		vector3 d_f, d_rt, d_up;
		/*
		if ( cg.snap->ps.weapon == WP_NONE ||
			cg.snap->ps.weapon == WP_SABER ||
			cg.snap->ps.weapon == WP_STUN_BATON)
		{
			VectorCopy( refdef->vieworg, start );
			AngleVectors( refdef->viewangles, d_f, d_rt, d_up );
		}
		else
		*/
		//For now we still want to draw the crosshair in relation to the player's world coordinates
		//even if we have a melee weapon/no weapon.
		if ( cg.predictedPlayerState.m_iVehicleNum && (cg.predictedPlayerState.eFlags&EF_NODRAW) )
		{//we're *inside* a vehicle
			//do the vehicle's crosshair instead
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			qboolean gunner = qfalse;

			//if (veh->currentState.owner == cg.predictedPlayerState.clientNum)
			{ //the pilot
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				gunner = CG_CalcVehicleMuzzlePoint(cg.predictedPlayerState.m_iVehicleNum, &start, &d_f, &d_rt, &d_up);
			}
			/*
			else
			{ //a passenger
				ignore = cg.predictedPlayerState.m_iVehicleNum;
				VectorCopy( veh->lerpOrigin, start );
				AngleVectors( veh->lerpAngles, d_f, d_rt, d_up );
				VectorMA(start, 32.0f, d_f, start); //super hack
			}
			*/
			if ( veh->m_pVehicle
				&& veh->m_pVehicle->m_pVehicleInfo
				&& veh->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
				&& cg.distanceCull > MAX_XHAIR_DIST_ACCURACY
				&& !gunner )
			{
				//NOTE: on huge maps, the crosshair gets inaccurate at close range,
				//		so we'll do an extra G2 trace from the refdef->vieworg
				//		to see if we hit anything closer and auto-aim at it if so
				bVehCheckTraceFromCamPos = qtrue;
			}
		}
		else if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex &&
			cg_entities[cg.snap->ps.emplacedIndex].ghoul2 && cg_entities[cg.snap->ps.emplacedIndex].currentState.weapon == WP_NONE)
		{ //locked into our e-web, calc the muzzle from it
			CG_CalcEWebMuzzlePoint(&cg_entities[cg.snap->ps.emplacedIndex], &start, &d_f, &d_rt, &d_up);
		}
		else
		{
			if (cg.snap && cg.snap->ps.weapon == WP_EMPLACED_GUN && cg.snap->ps.emplacedIndex)
			{
				vector3 pitchConstraint;

				ignore = cg.snap->ps.emplacedIndex;

				VectorCopy(&refdef->viewangles, &pitchConstraint);

				if (cg.renderingThirdPerson)
					VectorCopy(&cg.predictedPlayerState.viewangles, &pitchConstraint);
				else
					VectorCopy(&refdef->viewangles, &pitchConstraint);

				if (pitchConstraint.pitch > 40)
					pitchConstraint.pitch = 40;

				AngleVectors( &pitchConstraint, &d_f, &d_rt, &d_up );
			}
			else
			{
				vector3 pitchConstraint;

				if (cg.renderingThirdPerson)
					VectorCopy(&cg.predictedPlayerState.viewangles, &pitchConstraint);
				else
					VectorCopy(&refdef->viewangles, &pitchConstraint);

				AngleVectors( &pitchConstraint, &d_f, &d_rt, &d_up );
			}
			CG_CalcMuzzlePoint(cg.snap->ps.clientNum, &start);
		}

		VectorMA( &start, cg.distanceCull, &d_f, &end );
	}
	else
	{
		VectorCopy( &refdef->vieworg, &start );
		VectorMA( &start, 131072, &refdef->viewaxis[0], &end );
	}

traceAgain:
	if ( cg_dynamicCrosshair.integer && cg_dynamicCrosshairPrecision.integer )
	{ //then do a trace with ghoul2 models in mind
		CG_G2Trace( &trace, &start, &vec3_origin, &vec3_origin, &end,
			ignore, CONTENTS_SOLID|CONTENTS_BODY );
		if ( bVehCheckTraceFromCamPos )
		{
			//NOTE: this MUST stay up to date with the method used in WP_VehCheckTraceFromCamPos
			centity_t *veh = &cg_entities[cg.predictedPlayerState.m_iVehicleNum];
			trace_t	extraTrace;
			vector3	viewDir2End, extraEnd;
			float	minAutoAimDist = Distance( &veh->lerpOrigin, &refdef->vieworg ) + (veh->m_pVehicle->m_pVehicleInfo->length/2.0f) + 200.0f;

			VectorSubtract( &end, &refdef->vieworg, &viewDir2End );
			VectorNormalize( &viewDir2End );
			VectorMA( &refdef->vieworg, MAX_XHAIR_DIST_ACCURACY, &viewDir2End, &extraEnd );
			CG_G2Trace( &extraTrace, &refdef->vieworg, &vec3_origin, &vec3_origin, &extraEnd,
				ignore, CONTENTS_SOLID|CONTENTS_BODY );
			if ( !extraTrace.allsolid
				&& !extraTrace.startsolid )
			{
				if ( extraTrace.fraction < 1.0f )
				{
					if ( (extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY) > minAutoAimDist )
					{
						if ( ((extraTrace.fraction*MAX_XHAIR_DIST_ACCURACY)-Distance( &veh->lerpOrigin, &refdef->vieworg )) < (trace.fraction*cg.distanceCull) )
						{//this trace hit *something* that's closer than the thing the main trace hit, so use this result instead
							memcpy( &trace, &extraTrace, sizeof( trace_t ) );
						}
					}
				}
			}
		}
	}
	else
	{
		CG_Trace( &trace, &start, &vec3_origin, &vec3_origin, &end, ignore, CONTENTS_SOLID|CONTENTS_BODY );
	}

	if (trace.entityNum < MAX_CLIENTS)
	{
		if (CG_IsMindTricked(cg_entities[trace.entityNum].currentState.trickedentindex,
			cg_entities[trace.entityNum].currentState.trickedentindex2,
			cg_entities[trace.entityNum].currentState.trickedentindex3,
			cg_entities[trace.entityNum].currentState.trickedentindex4,
			cg.snap->ps.clientNum))
		{
			if (cg.crosshairClientNum == trace.entityNum)
			{
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}

			CG_DrawCrosshair(&trace.endpos, 0);

			return; //this entity is mind-tricking the current client, so don't render it
		}
		/*
		if ( (cg_entities[trace.entityNum].currentState.bolt1 && !(cg_entities[trace.entityNum].currentState.eFlags & EF_DEAD) && cg_entities[trace.entityNum].currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cg_entities[trace.entityNum].currentState.number)) )
		{
			if ( cg.crosshairClientNum == trace.entityNum )
			{
				cg.crosshairClientNum = ENTITYNUM_NONE;
				cg.crosshairClientTime = 0;
			}
			CG_DrawCrosshair( trace.endpos, 0 );
			return;
		}
		*/

		if ( cg.snap->ps.duelInProgress && cg.snap->ps.duelIndex != trace.entityNum )
		{
			ignore = trace.entityNum;
			VectorCopy( &trace.endpos, &start );
			traces++;
			if ( traces < 10 )
				goto traceAgain;
			else
				return;
		}
	}

	if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{
		if (trace.entityNum < /*MAX_CLIENTS*/ENTITYNUM_WORLD)
		{
			centity_t *veh = &cg_entities[trace.entityNum];
			cg.crosshairClientNum = trace.entityNum;
			cg.crosshairClientTime = cg.time;

			if ( veh->currentState.eType == ET_NPC &&
				veh->currentState.NPC_class == CLASS_VEHICLE &&
				veh->currentState.owner < MAX_CLIENTS )
			{ //draw the name of the pilot then
				cg.crosshairClientNum = veh->currentState.owner;
				cg.crosshairVehNum = veh->currentState.number;
				cg.crosshairVehTime = cg.time;
			}

			CG_DrawCrosshair(&trace.endpos, 1);
		}
		else
		{
			CG_DrawCrosshair(&trace.endpos, 0);
		}
	}

	if ( cg_drawCrosshairNames.integer >= 2 && trace.entityNum >= MAX_CLIENTS )
		return;

	// if the player is in fog, don't show it
	content = trap->CM_PointContents( &trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}

/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	vector4		*color;
	vector4		tcolor;
	char		*name;
	qboolean	isVeh = qfalse;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	//rww - still do the trace, our dynamic crosshair depends on it

	if (cg.crosshairClientNum < ENTITYNUM_WORLD)
	{
		centity_t *veh = &cg_entities[cg.crosshairClientNum];

		if (veh->currentState.eType == ET_NPC &&
			veh->currentState.NPC_class == CLASS_VEHICLE &&
			veh->currentState.owner < MAX_CLIENTS)
		{ //draw the name of the pilot then
			cg.crosshairClientNum = veh->currentState.owner;
			cg.crosshairVehNum = veh->currentState.number;
			cg.crosshairVehTime = cg.time;
			isVeh = qtrue; //so we know we're drawing the pilot's name
		}
	}

	if (cg.crosshairClientNum >= MAX_CLIENTS)
		return;

	if (cg_entities[cg.crosshairClientNum].currentState.powerups & (1 << PW_CLOAKED))
	{
		return;
	}

	if ( cg.snap->ps.duelInProgress )
	{
		centity_t *cent = &cg_entities[cg.crosshairClientNum];
		if (cent->currentState.number != cg.snap->ps.duelIndex &&
			cent->currentState.number != cg.snap->ps.clientNum)
			return;
	}

	if ( cg_drawCrosshairNames.integer == 1 )
	{// draw the name of the player being looked at
		color = CG_FadeColor( cg.crosshairClientTime, 1000 );
		if ( !color ) {
			trap->R_SetColor( NULL );
			return;
		}
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;

	VectorSet4( &tcolor, 1.0f, 1.0f, 1.0f, 1.0f );

	if ( isVeh ) {
		char str[MAX_STRING_CHARS];
		Com_sprintf( str, MAX_STRING_CHARS, "%s (pilot)", name );
		UI_DrawProportionalString( 320, 170, str, UI_CENTER, &tcolor );
	}
	else if ( cg_drawCrosshairNames.integer == 1 )
		UI_DrawProportionalString( 320, 170, name, UI_CENTER, &tcolor );
	else if ( cg_drawCrosshairNames.integer >= 2 && !CG_FadeColor2( &tcolor, cg.crosshairClientTime, 250.0f ) ) {
		int fontHandle = MenuFontToHandle( FONT_JAPPSMALL );
		float fontScale = 0.875f;
		if ( cg_drawCrosshairNames.integer == 3 )
			fontScale *= tcolor.a;
		trap->R_Font_DrawString( SCREEN_WIDTH/2.0 - trap->R_Font_StrLenPixels( name, fontHandle, fontScale )/2.0, SCREEN_HEIGHT/2.0 - 70.0, name, &tcolor, fontHandle, -1, fontScale );
	}

	trap->R_SetColor( NULL );
}

static void CG_DrawClientNames( void ) {
	const char *name = NULL;
	int i;
	const centity_t *cent = NULL;
	const int fontHandle = MenuFontToHandle( FONT_JAPPSMALL );
	const float fontScale = 0.75f, fontHeight = trap->R_Font_HeightPixels( fontHandle, fontScale );
	const vector4 *colour = &g_color_table[ColorIndex(COLOR_WHITE)];
	float x, y;

	if ( !cg_drawSpectatorNames.integer )
		return;

	for ( i=0, cent=cg_entities; i<cgs.maxclients; i++, cent++ ) {
		vector3 point;

		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team == TEAM_SPECTATOR )
			continue;

		name = cgs.clientinfo[i].name;
		VectorCopy( &cent->lerpOrigin, &point );
		point.z += DEFAULT_VIEWHEIGHT; // player height

		if ( CG_WorldCoordToScreenCoordFloat( &point, &x, &y ) ) {
			if ( cgs.gametype >= GT_TEAM )
				colour = &g_color_table[ColorIndex((cgs.clientinfo[i].team == TEAM_RED) ? COLOR_RED : COLOR_CYAN)];
			trap->R_Font_DrawString( x - trap->R_Font_StrLenPixels( name, fontHandle, fontScale )/2.0f, y-(fontHeight/2.0f), name, colour, fontHandle, -1, fontScale );
		}
	}
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void)
{
#if 0
	JP_DrawMiniScoreboard();
	CG_DrawHUD(&cg_entities[cg.predictedPlayerState.clientNum]);
#else
	const char *s=NULL, *s2=NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 0.33f };

	if ( cg.scoreBoardShowing )
		return;

	s = CG_GetStringEdString("MP_INGAME", "SPECTATOR");
	if ((cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) &&
		cgs.duelist1 != -1 &&
		cgs.duelist2 != -1)
	{
		char text[1024];
		int size = 64;

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			Com_sprintf(text, sizeof(text), "%s"S_COLOR_WHITE" %s %s"S_COLOR_WHITE" %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name, CG_GetStringEdString("MP_INGAME", "AND"), cgs.clientinfo[cgs.duelist3].name);
		}
		else
		{
			Com_sprintf(text, sizeof(text), "%s"S_COLOR_WHITE" %s %s", cgs.clientinfo[cgs.duelist1].name, CG_GetStringEdString("MP_INGAME", "SPECHUD_VERSUS"), cgs.clientinfo[cgs.duelist2].name);
		}
		CG_Text_Paint ( 320 - CG_Text_Width ( text, 1.0f, 3 ) / 2, 420, 1.0f, &colorWhite, text, 0, 0, 0, 3 );

		trap->R_SetColor( &colorTable[CT_WHITE] );
		if ( cgs.clientinfo[cgs.duelist1].modelIcon )
		{
			CG_DrawPic( 10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist1].modelIcon );
		}
		if ( cgs.clientinfo[cgs.duelist2].modelIcon )
		{
			CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5), size, size, cgs.clientinfo[cgs.duelist2].modelIcon );
		}

// nmckenzie: DUEL_HEALTH
		if (cgs.gametype == GT_DUEL)
		{
			if ( cgs.showDuelHealths >= 1)
			{	// draw the healths on the two guys - how does this interact with power duel, though?
				CG_DrawDuelistHealth ( 10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 1 );
				CG_DrawDuelistHealth ( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*1.5) - 12, 64, 8, 2 );
			}
		}

		if (cgs.gametype != GT_POWERDUEL)
		{
			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist1].score, cgs.fraglimit );
			CG_Text_Paint( 42 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, &colorWhite, text, 0, 0, 0, 2 );

			Com_sprintf(text, sizeof(text), "%i/%i", cgs.clientinfo[cgs.duelist2].score, cgs.fraglimit );
			CG_Text_Paint( SCREEN_WIDTH-size+22 - CG_Text_Width( text, 1.0f, 2 ) / 2, SCREEN_HEIGHT-(size*1.5) + 64, 1.0f, &colorWhite, text, 0, 0, 0, 2 );
		}

		if (cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1)
		{
			if ( cgs.clientinfo[cgs.duelist3].modelIcon )
			{
				CG_DrawPic( SCREEN_WIDTH-size-10, SCREEN_HEIGHT-(size*2.8), size, size, cgs.clientinfo[cgs.duelist3].modelIcon );
			}
		}
	}

	if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL )
		s2 = CG_GetStringEdString("MP_INGAME", "WAITING_TO_PLAY");	//s = "waiting to play";
	else //if ( cgs.gametype >= GT_TEAM )
		s2 = CG_GetStringEdString("MP_INGAME", "SPEC_CHOOSEJOIN"); //s = "press ESC and use the JOIN menu to play";

	trap->R_SetColor( &colour );
	CG_DrawPic( (SCREEN_WIDTH/3.0) - ((SCREEN_WIDTH/3.0)/2.0), 0, (SCREEN_WIDTH/3.0)*2.0, 60.0f, media.gfx.interface.forceIconBackground );
	trap->R_SetColor( NULL );

	CG_Text_Paint ( (SCREEN_WIDTH/2.0) - (CG_Text_Width( s, 0.5f, FONT_JAPPLARGE )/2.0), 0/*420*/, 0.5f, &colorWhite, s, 0, 0, 0, FONT_JAPPLARGE );
	CG_Text_Paint ( (SCREEN_WIDTH/2.0) - (CG_Text_Width( s2, 0.5f, FONT_JAPPLARGE )/2.0), 16/*440*/, 0.5f, &colorWhite, s2, 0, 0, 0, FONT_JAPPLARGE );

#endif
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	const char *s = NULL, *sParm = NULL;
	int sec;
	char sYes[20]={0}, sNo[20]={0}, sVote[20]={0}, sCmd[100]={0};

	if ( !cgs.voteTime )
		return;

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap->S_StartLocalSound( media.sounds.interface.talk, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 )
		sec = 0;

		 if ( !strncmp( cgs.voteString, "map_restart", 11 ) )		trap->SE_GetStringTextString( "MENUS_RESTART_MAP",	sCmd, sizeof( sCmd ) );
	else if ( !strncmp( cgs.voteString, "vstr nextmap", 12 ) )		trap->SE_GetStringTextString( "MENUS_NEXT_MAP",		sCmd, sizeof( sCmd ) );
	else if ( !strncmp( cgs.voteString, "g_doWarmup", 10 ) )		trap->SE_GetStringTextString( "MENUS_WARMUP",		sCmd, sizeof( sCmd ) );
	else if ( !strncmp( cgs.voteString, "g_gametype", 10 ) ) {
		trap->SE_GetStringTextString( "MENUS_GAME_TYPE", sCmd, sizeof( sCmd ) );

			 if ( !stricmp( cgs.voteString+11, "Free For All" ) )			sParm = CG_GetStringEdString( "MENUS", "FREE_FOR_ALL" );
		else if ( !stricmp( cgs.voteString+11, "Duel" ) )					sParm = CG_GetStringEdString( "MENUS", "DUEL" );
		else if ( !stricmp( cgs.voteString+11, "Holocron FFA" ) )			sParm = CG_GetStringEdString( "MENUS", "HOLOCRON_FFA" );
		else if ( !stricmp( cgs.voteString+11, "Power Duel" ) )				sParm = CG_GetStringEdString( "MENUS", "POWERDUEL" );
		else if ( !stricmp( cgs.voteString+11, "Team FFA" ) )				sParm = CG_GetStringEdString( "MENUS", "TEAM_FFA" );
		else if ( !stricmp( cgs.voteString+11, "Siege" ) )					sParm = CG_GetStringEdString( "MENUS", "SIEGE" );
		else if ( !stricmp( cgs.voteString+11, "Capture the Flag" ) )		sParm = CG_GetStringEdString( "MENUS", "CAPTURE_THE_FLAG" );
		else if ( !stricmp( cgs.voteString+11, "Capture the Ysalamiri" ) )	sParm = CG_GetStringEdString( "MENUS", "CAPTURE_THE_YSALIMARI" );
	}
	else if ( !strncmp( cgs.voteString, "map", 3 ) ) {
		trap->SE_GetStringTextString( "MENUS_NEW_MAP", sCmd, sizeof( sCmd ) );
		sParm = cgs.voteString+4;
	}

	else if ( !strncmp( cgs.voteString, "kick", 4 ) ) {
		trap->SE_GetStringTextString( "MENUS_KICK_PLAYER", sCmd, sizeof( sCmd ) );
		sParm = cgs.voteString+5;
	}

	//Raz: added
	else {
		sParm = cgs.voteString;
	}

	trap->SE_GetStringTextString( "MENUS_VOTE",	sVote, sizeof( sVote ) );
	trap->SE_GetStringTextString( "MENUS_YES",	sYes, sizeof( sYes ) );
	trap->SE_GetStringTextString( "MENUS_NO",	sNo, sizeof( sNo ) );

	if ( VALIDSTRING( sParm ) )		s = va( "%s(%i):<%s %s> %s:%i %s:%i", sVote, sec, sCmd, sParm, sYes, cgs.voteYes, sNo, cgs.voteNo );
	else							s = va( "%s(%i):<%s> %s:%i %s:%i",    sVote, sec, sCmd,        sYes, cgs.voteYes, sNo, cgs.voteNo );

	CG_DrawSmallString( 4, 58, s, 1.0F );

	if ( cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR ) {
		s = CG_GetStringEdString( "MP_INGAME", "OR_PRESS_ESC_THEN_CLICK_VOTE" ); // s = "or press ESC then click Vote";
		CG_DrawSmallString( 4, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0f );
	}
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
//		trap->S_StartLocalSound( media.sounds.interface.talk, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	if (strstr(cgs.teamVoteString[cs_offset], "leader"))
	{
		int i = 0;

		while (cgs.teamVoteString[cs_offset][i] && cgs.teamVoteString[cs_offset][i] != ' ')
		{
			i++;
		}

		if (cgs.teamVoteString[cs_offset][i] == ' ')
		{
			int voteIndex = 0;
			char voteIndexStr[256];

			i++;

			while (cgs.teamVoteString[cs_offset][i])
			{
				voteIndexStr[voteIndex] = cgs.teamVoteString[cs_offset][i];
				voteIndex++;
				i++;
			}
			voteIndexStr[voteIndex] = 0;

			voteIndex = atoi(voteIndexStr);

			s = va("TEAMVOTE(%i):(Make %s the new team leader) yes:%i no:%i", sec, cgs.clientinfo[voteIndex].name,
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
		else
		{
			s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
									cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
		}
	}
	else
	{
		s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
								cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	}
	CG_DrawSmallString( 4, 90, s, 1.0F );
}

qboolean CG_DrawQ3PScoreboard( void );
static qboolean CG_DrawScoreboard( void ) {
	if ( cg_newScoreboard.integer == 1 )
		return CG_DrawQ3PScoreboard();
	else
		return CG_DrawOldScoreboard();
#if 0
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );
		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
		fade = *fadeColor;
	}


	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#endif
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void )
{
	const char	*s;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.scoreBoardShowing )
		return qfalse;

//	s = "following";
	if (cgs.gametype == GT_POWERDUEL)
	{
		clientInfo_t *ci = &cgs.clientinfo[ cg.snap->ps.clientNum ];

			 if ( ci->duelTeam == DUELTEAM_LONE )	s = CG_GetStringEdString( "MP_INGAME", "FOLLOWINGLONE" );
		else if ( ci->duelTeam == DUELTEAM_DOUBLE )	s = CG_GetStringEdString( "MP_INGAME", "FOLLOWINGDOUBLE" );
		else										s = CG_GetStringEdString( "MP_INGAME", "FOLLOWING" );
	}
	else {
		s = CG_GetStringEdString( "MP_INGAME", "FOLLOWING" );
	}

	CG_Text_Paint ( 320 - CG_Text_Width ( s, 0.875f, FONT_JAPPLARGE ) / 2, 72, 0.875f, &colorWhite, s, 0, 0, 0, FONT_JAPPLARGE );

	s = cgs.clientinfo[ cg.snap->ps.clientNum ].name;
	CG_Text_Paint ( 320 - CG_Text_Width ( s, 0.875f, FONT_JAPPLARGE ) / 2, 96, 0.875f, &colorWhite, s, 0, 0, 0, FONT_JAPPLARGE );

	return qtrue;
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
#if 0
	const char	*s;
	int			w;

	if ( !cg_drawStatus.integer || !cg_drawAmmoWarning.integer || !cg.lowAmmoWarning )
		return;

	if ( cg.lowAmmoWarning == 2 )	s = "OUT OF AMMO";
	else							s = "LOW AMMO WARNING";

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
#endif
}



/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
//		s = "Waiting for players";
		s = CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS");
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
	{
		// find the two active players
		clientInfo_t	*ci1, *ci2, *ci3;

		ci1 = NULL;
		ci2 = NULL;
		ci3 = NULL;

		if (cgs.gametype == GT_POWERDUEL)
		{
			if (cgs.duelist1 != -1)
			{
				ci1 = &cgs.clientinfo[cgs.duelist1];
			}
			if (cgs.duelist2 != -1)
			{
				ci2 = &cgs.clientinfo[cgs.duelist2];
			}
			if (cgs.duelist3 != -1)
			{
				ci3 = &cgs.clientinfo[cgs.duelist3];
			}
		}
		else
		{
			for ( i = 0 ; i < cgs.maxclients ; i++ ) {
				if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
					if ( !ci1 ) {
						ci1 = &cgs.clientinfo[i];
					} else {
						ci2 = &cgs.clientinfo[i];
					}
				}
			}
		}
		if ( ci1 && ci2 )
		{
			if (ci3)
			{
				s = va( "%s vs %s and %s", ci1->name, ci2->name, ci3->name );
			}
			else
			{
				s = va( "%s vs %s", ci1->name, ci2->name );
			}
			w = CG_Text_Width(s, 0.6f, FONT_MEDIUM);
			CG_Text_Paint(320 - w / 2, 60, 0.6f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
		}
	} else {
		//RAZFIXME: global gametype names
		if ( cgs.gametype == GT_FFA ) {
			s = CG_GetStringEdString("MENUS", "FREE_FOR_ALL");//"Free For All";
		} else if ( cgs.gametype == GT_HOLOCRON ) {
			s = CG_GetStringEdString("MENUS", "HOLOCRON_FFA");//"Holocron FFA";
		} else if ( cgs.gametype == GT_JEDIMASTER ) {
			//Raz: JK2 gametypes
			//s = CG_GetStringEdString("MENUS", "POWERDUEL");//"Jedi Master";??
			s = "Jedi Master";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = CG_GetStringEdString("MENUS", "TEAM_FFA");//"Team FFA";
		} else if ( cgs.gametype == GT_SIEGE ) {
			s = CG_GetStringEdString("MENUS", "SIEGE");//"Siege";
		} else if ( cgs.gametype == GT_CTF ) {
			s = CG_GetStringEdString("MENUS", "CAPTURE_THE_FLAG");//"Capture the Flag";
		} else if ( cgs.gametype == GT_CTY ) {
			s = CG_GetStringEdString("MENUS", "CAPTURE_THE_YSALIMARI");//"Capture the Ysalamiri";
		} else if ( cgs.gametype == GT_SINGLE_PLAYER ) {
			s = "Cooperative";
		} else {
			s = "Unknown";
		}
		w = CG_Text_Width(s, 1.5f, FONT_MEDIUM);
		CG_Text_Paint(320 - w / 2, 90, 1.5f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE,FONT_MEDIUM);
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
//	s = va( "Starts in: %i", sec + 1 );
	s = va( "%s: %i",CG_GetStringEdString("MP_INGAME", "STARTS_IN"), sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;

		if (cgs.gametype != GT_SIEGE)
		{
			switch ( sec ) {
			case 0:
				trap->S_StartLocalSound( media.sounds.warning.count1, CHAN_ANNOUNCER );
				break;
			case 1:
				trap->S_StartLocalSound( media.sounds.warning.count2, CHAN_ANNOUNCER );
				break;
			case 2:
				trap->S_StartLocalSound( media.sounds.warning.count3, CHAN_ANNOUNCER );
				break;
			default:
				break;
			}
		}
	}
	switch ( cg.warmupCount ) {
	case 0:
		scale = 1.25f;
		break;
	case 1:
		scale = 1.15f;
		break;
	case 2:
		scale = 1.05f;
		break;
	default:
		scale = 0.9f;
		break;
	}

	w = CG_Text_Width(s, scale, FONT_MEDIUM);
	CG_Text_Paint(320 - w / 2, 125, scale, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE, FONT_MEDIUM);
}

//==================================================================================
/*
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap->Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

void CG_DrawFlagStatusQ3P( void )
{
	int		myFlag		= 0,
			theirFlag	= 0,
			team		= 0,
			fontHandle	= MenuFontToHandle( FONT_JAPPLARGE );
	float	iconSize	= ICON_SIZE,
			textOffset	= 20.0f,
			x			= SCREEN_WIDTH/2.0f,
			y			= SCREEN_HEIGHT-iconSize,
			fontScale	= 0.75f;
	vector4	faded		= { 0.25f, 0.25f, 0.25f, 0.75f },
			defer		= { 1.0f, 1.0f, 1.0f, 0.9f };
	char	*tmp		= NULL;

	if ( !cg.snap )
		return;

	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_CTY )
		return;

	team = cg.snap->ps.persistant[PERS_TEAM];

	if ( team == TEAM_RED )
	{
		myFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_rflag_x" : "gfx/hud/mpi_rflag" );
		theirFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_bflag_ys" : "gfx/hud/mpi_bflag" );
	}
	else
	{
		myFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_bflag_x" : "gfx/hud/mpi_bflag" );
		theirFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_rflag_ys" : "gfx/hud/mpi_rflag" );
	}

	if ( CG_OtherTeamHasFlag() )
	{
		trap->R_SetColor( &faded );
		CG_DrawPic( x - (iconSize*1.5f), y, iconSize, iconSize, myFlag );

		trap->R_SetColor( &defer );
		CG_DrawPic( x - (iconSize*1.5f), y, iconSize, iconSize, media.gfx.interface.defer );

	//	trap->R_Font_DrawString( ICON_SIZE, 96 + (ICON_SIZE/2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale )), "Flag stolen!", colorYellow, fontHandle, -1, fontScale );
		trap->R_SetColor( NULL );
	}
	else if ( CG_OtherTeamDroppedFlag() )
	{
		trap->R_SetColor( &faded );
		CG_DrawPic( x - (iconSize*1.5f), y, iconSize, iconSize, myFlag );
		trap->R_SetColor( NULL );
	}
	else
		CG_DrawPic( x - (iconSize*1.5), y, iconSize, iconSize, myFlag );

	if ( team == TEAM_RED )
	{
		tmp = va( "%i", cgs.scores1 );
		trap->R_Font_DrawString( x - (iconSize*1.5f) + (iconSize/2.0f) - (trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0f), y - textOffset, tmp, &colorTable[CT_HUD_RED], fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	}
	else
	{
		tmp = va( "%i", cgs.scores2 );
		trap->R_Font_DrawString( x - (iconSize*1.5f) + (iconSize/2.0f) - (trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0f), y - textOffset, tmp, &colorTable[CT_CYAN], fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	}
	trap->R_SetColor( NULL );

	if ( CG_YourTeamHasFlag() )
	{
		trap->R_SetColor( &faded );
		CG_DrawPic( x + (iconSize*0.5f), y, iconSize, iconSize, theirFlag );

		trap->R_SetColor( &defer );
		CG_DrawPic( x + (iconSize*0.5f), y, iconSize, iconSize, media.gfx.interface.team.assault );

		trap->R_SetColor( NULL );
	}
	else if ( CG_YourTeamDroppedFlag() )
	{
		trap->R_SetColor( &faded );
		CG_DrawPic( x + (iconSize*0.5f), y, iconSize, iconSize, theirFlag );

		trap->R_SetColor( NULL );
	}
	else
		CG_DrawPic( x + (iconSize*0.5f), y, iconSize, iconSize, theirFlag );
	if ( team == TEAM_RED )
	{
		tmp = va( "%i", cgs.scores2 );
		trap->R_Font_DrawString( x + (iconSize*0.5f) + (iconSize/2.0f) - (trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0f), y - textOffset, tmp, &colorTable[CT_CYAN], fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	}
	else
	{
		tmp = va( "%i", cgs.scores1 );
		trap->R_Font_DrawString( x + (iconSize*0.5f) + (iconSize/2.0f) - (trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0f), y - textOffset, tmp, &colorTable[CT_HUD_RED], fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	}
	trap->R_SetColor( NULL );
}

void CG_DrawFlagStatus( void )
{
	int myFlag = 0;
	int theirFlag = 0;
	int startDrawPos = 2;
	int ico_size = 32;

	trap->R_SetColor( NULL );

	if ( cg_drawStatus.integer == 2 )
	{
		CG_DrawFlagStatusQ3P();
		return;
	}

	if ( !cg.snap )
		return;

	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_CTY )
		return;

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED )
	{
		myFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_rflag_x" : "gfx/hud/mpi_rflag" );
		theirFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_bflag_ys" : "gfx/hud/mpi_bflag" );
	}
	else
	{
		myFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_bflag_x" : "gfx/hud/mpi_bflag" );
		theirFlag = trap->R_RegisterShaderNoMip( (cgs.gametype == GT_CTY) ? "gfx/hud/mpi_rflag_ys" : "gfx/hud/mpi_rflag" );
	}

	if ( CG_YourTeamHasFlag() )
	{
		//CG_DrawPic( startDrawPos, 330, ico_size, ico_size, theirFlag );
		CG_DrawPic( 2, 330-startDrawPos, ico_size, ico_size, theirFlag );
		startDrawPos += ico_size+2;
	}

	if ( CG_OtherTeamHasFlag() )
	{
		//CG_DrawPic( startDrawPos, 330, ico_size, ico_size, myFlag );
		CG_DrawPic( 2, 330-startDrawPos, ico_size, ico_size, myFlag );
	}
}

//draw meter showing jetpack fuel when it's not full
#define JPFUELBAR_H			100.0f
#define JPFUELBAR_W			20.0f
#define JPFUELBAR_X			(SCREEN_WIDTH-JPFUELBAR_W-8.0f)
#define JPFUELBAR_Y			260.0f
void CG_DrawJetpackFuel(void)
{
	vector4 aColor, cColor;
	float x = JPFUELBAR_X;
	float y = JPFUELBAR_Y;
	float percent = ((float)cg.snap->ps.jetpackFuel/100.0f)*JPFUELBAR_H;

	if (percent > JPFUELBAR_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//color of the bar
	aColor.r = 0.5f;
	aColor.g = 0.0f;
	aColor.b = 0.0f;
	aColor.a = 0.8f;

	//color of greyed out "missing fuel"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, JPFUELBAR_W, JPFUELBAR_H, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(JPFUELBAR_H-percent), JPFUELBAR_W-1.0f, JPFUELBAR_H-1.0f-(JPFUELBAR_H-percent), &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, JPFUELBAR_W-1.0f, JPFUELBAR_H-percent, &cColor);
}

//draw meter showing e-web health when it is in use
#define EWEBHEALTH_H			100.0f
#define EWEBHEALTH_W			20.0f
#define EWEBHEALTH_X			(SCREEN_WIDTH-EWEBHEALTH_W-8.0f)
#define EWEBHEALTH_Y			290.0f
void CG_DrawEWebHealth(void)
{
	vector4 aColor, cColor;
	float x = EWEBHEALTH_X;
	float y = EWEBHEALTH_Y;
	centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];
	float percent = ((float)eweb->currentState.health/eweb->currentState.maxhealth)*EWEBHEALTH_H;

	if (percent > EWEBHEALTH_H)
	{
		return;
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	//kind of hacky, need to pass a coordinate in here
	if (cg.snap->ps.jetpackFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f);
	}
	if (cg.snap->ps.cloakFuel < 100)
	{
		x -= (JPFUELBAR_W+8.0f);
	}

	//color of the bar
	aColor.r = 0.5f;
	aColor.g = 0.0f;
	aColor.b = 0.0f;
	aColor.a = 0.8f;

	//color of greyed out "missing fuel"
	cColor.r = 0.5f;
	cColor.g = 0.5f;
	cColor.b = 0.5f;
	cColor.a = 0.1f;

	//draw the background (black)
	CG_DrawRect(x, y, EWEBHEALTH_W, EWEBHEALTH_H, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much health there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(EWEBHEALTH_H-percent), EWEBHEALTH_W-1.0f, EWEBHEALTH_H-1.0f-(EWEBHEALTH_H-percent), &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, EWEBHEALTH_W-1.0f, EWEBHEALTH_H-percent, &cColor);
}

//draw meter showing cloak fuel when it's not full
#define CLFUELBAR_H			100.0f
#define CLFUELBAR_W			20.0f
#define CLFUELBAR_X			(SCREEN_WIDTH-CLFUELBAR_W-8.0f)
#define CLFUELBAR_Y			260.0f
void CG_DrawCloakFuel(void)
{
	vector4 aColor, cColor;
	float x = CLFUELBAR_X;
	float y = CLFUELBAR_Y;
	float percent = ((float)cg.snap->ps.cloakFuel/100.0f)*CLFUELBAR_H;

	if (percent > CLFUELBAR_H)
	{
		return;
	}

	if ( cg.snap->ps.jetpackFuel < 100 )
	{//if drawing jetpack fuel bar too, then move this over...?
		x -= (JPFUELBAR_W+8.0f);
	}

	if (percent < 0.1f)
	{
		percent = 0.1f;
	}

	VectorSet4( &aColor, 0.0f, 0.0f, 0.6f, 0.8f ); // color of the bar
	VectorSet4( &cColor, 0.1f, 0.1f, 0.3f, 0.1f ); // color of greyed out "missing fuel"

	//draw the background (black)
	CG_DrawRect(x, y, CLFUELBAR_W, CLFUELBAR_H, 1.0f, &colorTable[CT_BLACK]);

	//now draw the part to show how much fuel there is in the color specified
	CG_FillRect(x+1.0f, y+1.0f+(CLFUELBAR_H-percent), CLFUELBAR_W-1.0f, CLFUELBAR_H-1.0f-(CLFUELBAR_H-percent), &aColor);

	//then draw the other part greyed out
	CG_FillRect(x+1.0f, y+1.0f, CLFUELBAR_W-1.0f, CLFUELBAR_H-percent, &cColor);
}

int cgRageTime = 0;
int cgRageFadeTime = 0;
float cgRageFadeVal = 0;

int cgRageRecTime = 0;
int cgRageRecFadeTime = 0;
float cgRageRecFadeVal = 0;

int cgAbsorbTime = 0;
int cgAbsorbFadeTime = 0;
float cgAbsorbFadeVal = 0;

int cgProtectTime = 0;
int cgProtectFadeTime = 0;
float cgProtectFadeVal = 0;

int cgYsalTime = 0;
int cgYsalFadeTime = 0;
float cgYsalFadeVal = 0;

qboolean gCGHasFallVector = qfalse;
vector3 gCGFallVector;

/*
=================
CG_Draw2D
=================
*/
extern int cgSiegeRoundState;
extern int cgSiegeRoundTime;

extern int team1Timed;
extern int team2Timed;

int cg_beatingSiegeTime = 0;

int cgSiegeRoundBeganTime = 0;
int cgSiegeRoundCountTime = 0;

static void CG_DrawSiegeTimer(int timeRemaining, qboolean isMyTeam)
{ //rwwFIXMEFIXME: Make someone make assets and use them.
  //this function is pretty much totally placeholder.
//	int x = 0;
//	int y = SCREEN_HEIGHT-160;
	int fColor = 0;
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	strcpy(timeStr, va( "%i:%02i", minutes, seconds ));

	if (isMyTeam)
	{
		fColor = CT_HUD_RED;
	}
	else
	{
		fColor = CT_HUD_GREEN;
	}

//	trap->Cvar_Set("ui_siegeTimer", timeStr);

//	UI_DrawProportionalString( x+16, y+40, timeStr,
//		UI_SMALLFONT|UI_DROPSHADOW, colorTable[fColor] );

	item = Menu_FindItemByName(menuHUD, "timer");
	if (item)
	{
		UI_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW,
			&colorTable[fColor] );
	}

}

static void CG_DrawSiegeDeathTimer( int timeRemaining )
{
	int minutes = 0;
	int seconds = 0;
	char timeStr[1024];
	menuDef_t	*menuHUD = NULL;
	itemDef_t	*item = NULL;

	menuHUD = Menus_FindByName("mp_timer");
	if (!menuHUD)
	{
		return;
	}

	item = Menu_FindItemByName(menuHUD, "frame");

	if (item)
	{
		trap->R_SetColor( &item->window.foreColor );
		CG_DrawPic(
			item->window.rect.x,
			item->window.rect.y,
			item->window.rect.w,
			item->window.rect.h,
			item->window.background );
	}

	seconds = timeRemaining;

	while (seconds >= 60)
	{
		minutes++;
		seconds -= 60;
	}

	if (seconds < 10)
	{
		strcpy(timeStr, va( "%i:0%i", minutes, seconds ));
	}
	else
	{
		strcpy(timeStr, va( "%i:%i", minutes, seconds ));
	}

	item = Menu_FindItemByName(menuHUD, "deathtimer");
	if (item)
	{
		UI_DrawProportionalString(
			item->window.rect.x,
			item->window.rect.y,
			timeStr,
			UI_SMALLFONT|UI_DROPSHADOW,
			&item->window.foreColor );
	}

}

int cgSiegeEntityRender = 0;

static void CG_DrawSiegeHUDItem(void)
{
	void *g2;
	qhandle_t handle;
	vector3 origin, angles;
	vector3 mins, maxs;
	float len;
	centity_t *cent = &cg_entities[cgSiegeEntityRender];

	if (cent->ghoul2)
	{
		g2 = cent->ghoul2;
		handle = 0;
	}
	else
	{
		handle = cgs.gameModels[cent->currentState.modelindex];
		g2 = NULL;
	}

	if (handle)
	{
		trap->R_ModelBounds( handle, &mins, &maxs );
	}
	else
	{
		VectorSet(&mins, -16, -16, -20);
		VectorSet(&maxs, 16, 16, 32);
	}

	origin.z = -0.5 * ( mins.z + maxs.z );
	origin.y = 0.5 * ( mins.y + maxs.y );
	len = 0.5 * ( maxs.z - mins.z );
	origin.x = len / 0.268;

	VectorClear(&angles);
	angles.yaw = cg.autoAngles.yaw;

	CG_Draw3DModel( 8, 8, 64, 64, handle, g2, cent->currentState.g2radius, 0, &origin, &angles );

	cgSiegeEntityRender = 0; //reset for next frame
}

/*====================================
chatbox functionality -rww
====================================*/
#define	CHATBOX_CUTOFF_LEN	550
#define	CHATBOX_FONT_HEIGHT	20

//utility func, insert a string into a string at the specified
//place (assuming this will not overflow the buffer)
void CG_ChatBox_StrInsert(char *buffer, int place, char *str)
{
	int insLen = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i+insLen+1] = 0; //terminate the string at its new length
	while (i >= place)
	{
		buffer[i+insLen] = buffer[i];
		i--;
	}

	i++;
	while (k < insLen)
	{
		buffer[i] = str[k];
		i++;
		k++;
	}
}

//add chatbox string
void CG_ChatBox_AddString(char *chatStr)
{
	chatBoxItem_t *chat = &cg.chatItems[cg.chatItemActive];
	float chatLen;

	if (cg_chatbox.integer<=0)
	{ //don't bother then.
		return;
	}

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chatStr) > sizeof(chat->string))
	{ //too long, terminate at proper len.
		chatStr[sizeof(chat->string)-1] = 0;
	}

	strcpy(chat->string, chatStr);
	chat->time = cg.time + cg_chatbox.integer;

	chat->lines = 1;

	chatLen = CG_Text_Width(chat->string, 1.0f, FONT_SMALL);
	if (chatLen > CHATBOX_CUTOFF_LEN)
	{ //we have to break it into segments...
        int i = 0;
		int lastLinePt = 0;
		char s[2];

		chatLen = 0;
		while (chat->string[i])
		{
			s[0] = chat->string[i];
			s[1] = 0;
			chatLen += CG_Text_Width(s, 0.65f, FONT_SMALL);

			if (chatLen >= CHATBOX_CUTOFF_LEN)
			{
				int j = i;
				while (j > 0 && j > lastLinePt)
				{
					if (chat->string[j] == ' ')
					{
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ')
				{
					i = j;
				}

                chat->lines++;
				CG_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chatLen = 0;
				lastLinePt = i+1;
			}
			i++;
		}
	}

	cg.chatItemActive++;
	if (cg.chatItemActive >= MAX_CHATBOX_ITEMS)
	{
		cg.chatItemActive = 0;
	}
}

//insert item into array (rearranging the array if necessary)
void CG_ChatBox_ArrayInsert(chatBoxItem_t **array, int insPoint, int maxNum, chatBoxItem_t *item)
{
    if (array[insPoint])
	{ //recursively call, to move everything up to the top
		if (insPoint+1 >= maxNum)
		{
			trap->Error( ERR_DROP, "CG_ChatBox_ArrayInsert: Exceeded array size");
		}
		CG_ChatBox_ArrayInsert(array, insPoint+1, maxNum, array[insPoint]);
	}

	//now that we have moved anything that would be in this slot up, insert what we want into the slot
	array[insPoint] = item;
}

//go through all the chat strings and draw them if they are not yet expired
static QINLINE void CG_ChatBox_DrawStrings(void)
{
	chatBoxItem_t *drawThese[MAX_CHATBOX_ITEMS];
	int numToDraw = 0;
	int linesToDraw = 0;
	int i = 0;
	int x = 30;
	float y = cg.scoreBoardShowing ? 475 : cg_chatboxHeight.integer;
	float fontScale = 0.65f;

	if (!cg_chatbox.integer)
	{
		return;
	}

	memset(drawThese, 0, sizeof(drawThese));

	while (i < MAX_CHATBOX_ITEMS)
	{
		if (cg.chatItems[i].time >= cg.time)
		{
			int check = numToDraw;
			int insertionPoint = numToDraw;

			while (check >= 0)
			{
				if (drawThese[check] &&
					cg.chatItems[i].time < drawThese[check]->time)
				{ //insert here
					insertionPoint = check;
				}
				check--;
			}
			CG_ChatBox_ArrayInsert(drawThese, insertionPoint, MAX_CHATBOX_ITEMS, &cg.chatItems[i]);
			numToDraw++;
			linesToDraw += cg.chatItems[i].lines;
		}
		i++;
	}

	if (!numToDraw)
	{ //nothing, then, just get out of here now.
		return;
	}

	//move initial point up so we draw bottom-up (visually)
	y -= (CHATBOX_FONT_HEIGHT*fontScale)*linesToDraw;

	//we have the items we want to draw, just quickly loop through them now
	i = 0;
	while (i < numToDraw)
	{
		CG_Text_Paint(x, y, fontScale, &colorWhite, drawThese[i]->string, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		y += ((CHATBOX_FONT_HEIGHT*fontScale)*drawThese[i]->lines);
		i++;
	}
}

static void CG_Draw2DScreenTints( void )
{
	float			rageTime, rageRecTime, absorbTime, protectTime, ysalTime;
	vector4			hcolor;
	refdef_t *refdef = CG_GetRefdef();

	if ( !cg_drawScreenTints.integer )
		return;

	if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR)
	{
		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_RAGE))
		{
			if (!cgRageTime)
			{
				cgRageTime = cg.time;
			}

			rageTime = (float)(cg.time - cgRageTime);

			rageTime /= 9000;

			if (rageTime < 0)
				rageTime = 0;
			if (rageTime > 0.15f)
				rageTime = 0.15f;

			VectorSet4( &hcolor, 0.7f, 0.0f, 0.0f, rageTime );

			if (!cg.renderingThirdPerson)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}

			cgRageFadeTime = 0;
			cgRageFadeVal = 0;
		}
		else if (cgRageTime)
		{
			if (!cgRageFadeTime)
			{
				cgRageFadeTime = cg.time;
				cgRageFadeVal = 0.15f;
			}

			rageTime = cgRageFadeVal;

			cgRageFadeVal -= (cg.time - cgRageFadeTime)*0.000005;

			if (rageTime < 0)
				rageTime = 0;
			if (rageTime > 0.15f)
				rageTime = 0.15f;

			if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
			{
				float checkRageRecTime = rageTime;

				if (checkRageRecTime < 0.15f)
					checkRageRecTime = 0.15f;

				hcolor.a = checkRageRecTime;
				hcolor.r = rageTime*4;
				if (hcolor.r < 0.2f)
				{
					hcolor.r = 0.2f;
				}
				hcolor.g = 0.2f;
				hcolor.b = 0.2f;
			}
			else
			{
				hcolor.a = rageTime;
				hcolor.r = 0.7f;
				hcolor.g = 0;
				hcolor.b = 0;
			}

			if (!cg.renderingThirdPerson && rageTime)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}
			else
			{
				if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
				{
					hcolor.a = 0.15f;
					hcolor.r = 0.2f;
					hcolor.g = 0.2f;
					hcolor.b = 0.2f;
					CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
				}
				cgRageTime = 0;
			}
		}
		else if (cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
		{
			if (!cgRageRecTime)
			{
				cgRageRecTime = cg.time;
			}

			rageRecTime = (float)(cg.time - cgRageRecTime);

			rageRecTime /= 9000;

			if (rageRecTime < 0.15f)//0)
				rageRecTime = 0.15f;//0;
			if (rageRecTime > 0.15f)
				rageRecTime = 0.15f;

			hcolor.a = rageRecTime;
			hcolor.x = 0.2f;
			hcolor.y = 0.2f;
			hcolor.z = 0.2f;

			if (!cg.renderingThirdPerson)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}

			cgRageRecFadeTime = 0;
			cgRageRecFadeVal = 0;
		}
		else if (cgRageRecTime)
		{
			if (!cgRageRecFadeTime)
			{
				cgRageRecFadeTime = cg.time;
				cgRageRecFadeVal = 0.15f;
			}

			rageRecTime = cgRageRecFadeVal;

			cgRageRecFadeVal -= (cg.time - cgRageRecFadeTime)*0.000005;

			if (rageRecTime < 0)
				rageRecTime = 0;
			if (rageRecTime > 0.15f)
				rageRecTime = 0.15f;

			hcolor.a = rageRecTime;
			hcolor.r = 0.2f;
			hcolor.g = 0.2f;
			hcolor.b = 0.2f;

			if (!cg.renderingThirdPerson && rageRecTime)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}
			else
			{
				cgRageRecTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_ABSORB))
		{
			if (!cgAbsorbTime)
			{
				cgAbsorbTime = cg.time;
			}

			absorbTime = (float)(cg.time - cgAbsorbTime);

			absorbTime /= 9000;

			if (absorbTime < 0)
				absorbTime = 0;
			if (absorbTime > 0.15f)
				absorbTime = 0.15f;

			hcolor.a = absorbTime/2;
			hcolor.r = 0;
			hcolor.g = 0;
			hcolor.b = 0.7f;

			if (!cg.renderingThirdPerson)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}

			cgAbsorbFadeTime = 0;
			cgAbsorbFadeVal = 0;
		}
		else if (cgAbsorbTime)
		{
			if (!cgAbsorbFadeTime)
			{
				cgAbsorbFadeTime = cg.time;
				cgAbsorbFadeVal = 0.15f;
			}

			absorbTime = cgAbsorbFadeVal;

			cgAbsorbFadeVal -= (cg.time - cgAbsorbFadeTime)*0.000005f;

			if (absorbTime < 0)
				absorbTime = 0;
			if (absorbTime > 0.15f)
				absorbTime = 0.15f;

			hcolor.a = absorbTime/2;
			hcolor.r = 0;
			hcolor.g = 0;
			hcolor.b = 0.7f;

			if (!cg.renderingThirdPerson && absorbTime)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}
			else
			{
				cgAbsorbTime = 0;
			}
		}

		if (cg.snap->ps.fd.forcePowersActive & (1 << FP_PROTECT))
		{
			if (!cgProtectTime)
			{
				cgProtectTime = cg.time;
			}

			protectTime = (float)(cg.time - cgProtectTime);

			protectTime /= 9000;

			if (protectTime < 0)
				protectTime = 0;
			if (protectTime > 0.15f)
				protectTime = 0.15f;

			hcolor.a = protectTime/2;
			hcolor.r = 0;
			hcolor.g = 0.7f;
			hcolor.b = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}

			cgProtectFadeTime = 0;
			cgProtectFadeVal = 0;
		}
		else if (cgProtectTime)
		{
			if (!cgProtectFadeTime)
			{
				cgProtectFadeTime = cg.time;
				cgProtectFadeVal = 0.15f;
			}

			protectTime = cgProtectFadeVal;

			cgProtectFadeVal -= (cg.time - cgProtectFadeTime)*0.000005f;

			if (protectTime < 0)
				protectTime = 0;
			if (protectTime > 0.15f)
				protectTime = 0.15f;

			hcolor.a = protectTime/2;
			hcolor.r = 0;
			hcolor.g = 0.7f;
			hcolor.b = 0;

			if (!cg.renderingThirdPerson && protectTime)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}
			else
			{
				cgProtectTime = 0;
			}
		}

		if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && (cg.time - cg.snap->ps.rocketLockTime) > 0)
		{
			CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
		}

		if (BG_HasYsalamiri(cgs.gametype, &cg.snap->ps))
		{
			if (!cgYsalTime)
			{
				cgYsalTime = cg.time;
			}

			ysalTime = (float)(cg.time - cgYsalTime);

			ysalTime /= 9000;

			if (ysalTime < 0)
				ysalTime = 0;
			if (ysalTime > 0.15f)
				ysalTime = 0.15f;

			hcolor.a = ysalTime/2;
			hcolor.r = 0.7f;
			hcolor.g = 0.7f;
			hcolor.b = 0;

			if (!cg.renderingThirdPerson)
			{
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			}

			cgYsalFadeTime = 0;
			cgYsalFadeVal = 0;
		}
		else if (cgYsalTime)
		{
			if (!cgYsalFadeTime)
			{
				cgYsalFadeTime = cg.time;
				cgYsalFadeVal = 0.15f;
			}

			ysalTime = cgYsalFadeVal;

			cgYsalFadeVal -= (cg.time - cgYsalFadeTime)*0.000005f;

			if (ysalTime < 0)
				ysalTime = 0;
			if (ysalTime > 0.15f)
				ysalTime = 0.15f;

			hcolor.a = ysalTime/2;
			hcolor.r = 0.7f;
			hcolor.g = 0.7f;
			hcolor.b = 0;

			if (!cg.renderingThirdPerson && ysalTime)
				CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);
			else
				cgYsalTime = 0;
		}
	}

	if ( (refdef->viewContents&CONTENTS_LAVA) )
	{//tint screen red
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor.a = 0.5 + (0.15f*sin( phase ));
		hcolor.r = 0.7f;
		hcolor.g = 0;
		hcolor.b = 0;

		CG_DrawRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor  );
	}
	else if ( (refdef->viewContents&CONTENTS_SLIME) )
	{//tint screen green
		float phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		hcolor.a = 0.4 + (0.1f*sin( phase ));
		hcolor.r = 0;
		hcolor.g = 0.7f;
		hcolor.b = 0;

		CG_DrawRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor  );
	}
	else if ( (refdef->viewContents&CONTENTS_WATER) )
	{//tint screen light blue -- FIXME: don't do this if CONTENTS_FOG? (in case someone *does* make a water shader with fog in it?)
		float phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2;
		hcolor.a = 0.3f + (0.05f*sinf( phase ));
		hcolor.r = 0;
		hcolor.g = 0.2f;
		hcolor.b = 0.8f;

		CG_DrawRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor  );
	}
}

void CG_Draw2D( void ) {
	float			inTime = cg.invenSelectTime+WEAPON_SELECT_TIME;
	float			wpTime = cg.weaponSelectTime+WEAPON_SELECT_TIME;
	float			fallTime;
	float			bestTime;
	int				drawSelect = 0;

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot )
		return;

	if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR ) {
		cgRageTime = 0;
		cgRageFadeTime = 0;
		cgRageFadeVal = 0;

		cgRageRecTime = 0;
		cgRageRecFadeTime = 0;
		cgRageRecFadeVal = 0;

		cgAbsorbTime = 0;
		cgAbsorbFadeTime = 0;
		cgAbsorbFadeVal = 0;

		cgProtectTime = 0;
		cgProtectFadeTime = 0;
		cgProtectFadeVal = 0;

		cgYsalTime = 0;
		cgYsalFadeTime = 0;
		cgYsalFadeVal = 0;
	}

	if ( cg_draw2D.integer == 0 ) {
		//Raz: If you fall to your death, then turn cg_draw2D off, your camera will not update.
		//	Clear the fall vector to avoid that.
		gCGHasFallVector = qfalse;
		VectorClear( &gCGFallVector );
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		if ( !cg_newChatbox.integer )
			CG_ChatBox_DrawStrings();
		else
			CG_ChatboxDraw();

		CG_DrawMapChange();
		return;
	}

	CG_Draw2DScreenTints();

	if (cg.snap->ps.rocketLockIndex != ENTITYNUM_NONE && (cg.time - cg.snap->ps.rocketLockTime) > 0)
	{
		CG_DrawRocketLocking( cg.snap->ps.rocketLockIndex, cg.snap->ps.rocketLockTime );
	}

	if (cg.snap->ps.holocronBits)
	{
		CG_DrawHolocronIcons();
	}
	if (cg.snap->ps.fd.forcePowersActive || cg.snap->ps.fd.forceRageRecoveryTime > cg.time)
	{
		CG_DrawActivePowers();
	}

	if (cg.snap->ps.jetpackFuel < 100)
	{ //draw it as long as it isn't full
        CG_DrawJetpackFuel();
	}
	if (cg.snap->ps.cloakFuel < 100)
	{ //draw it as long as it isn't full
		CG_DrawCloakFuel();
	}
	if (cg.predictedPlayerState.emplacedIndex > 0)
	{
		centity_t *eweb = &cg_entities[cg.predictedPlayerState.emplacedIndex];

		if (eweb->currentState.weapon == WP_NONE)
		{ //using an e-web, draw its health
			CG_DrawEWebHealth();
		}
	}

	// Draw this before the text so that any text won't get clipped off
	CG_DrawZoomMask();

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair( NULL, 0 );
		CG_DrawClientNames();
		CG_SaberClashFlare();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

			if ( /*cg_drawStatus.integer*/0 ) {
				//Reenable if stats are drawn with menu system again
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}

			//CG_DrawTemporaryStats();

			CG_DrawAmmoWarning();

			CG_DrawCrosshairNames();

			if (cg_drawStatus.integer)
			{
				CG_DrawIconBackground();
			}

			if (inTime > wpTime)
			{
				drawSelect = 1;
				bestTime = cg.invenSelectTime;
			}
			else //only draw the most recent since they're drawn in the same place
			{
				drawSelect = 2;
				bestTime = cg.weaponSelectTime;
			}

			if (cg.forceSelectTime > bestTime)
			{
				drawSelect = 3;
			}

			switch(drawSelect)
			{
			case 1:
				CG_DrawInvenSelect();
				break;
			case 2:
				CG_DrawWeaponSelect();
				break;
			case 3:
				CG_DrawForceSelect();
				break;
			default:
				break;
			}

			if (cg_drawStatus.integer)
			{
				//Powerups now done with upperright stuff
				//CG_DrawPowerupIcons();

				CG_DrawFlagStatus();
			}

			CG_SaberClashFlare();

			if (cg_drawStatus.integer)
			{
				CG_DrawStats();
			}

			CG_DrawPickupItem();
#ifdef JK2AWARDS
			//Do we want to use this system again at some point?
			CG_DrawReward();
#endif
		}

	}

	if (cg.snap->ps.fallingToDeath)
	{
		vector4	hcolor;

		fallTime = (float)(cg.time - cg.snap->ps.fallingToDeath);

		fallTime /= (FALL_FADE_TIME/2);

		if (fallTime < 0)
		{
			fallTime = 0;
		}
		if (fallTime > 1)
		{
			fallTime = 1;
		}

		hcolor.a = fallTime;
		hcolor.r = 0;
		hcolor.g = 0;
		hcolor.b = 0;

		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, &hcolor);

		if (!gCGHasFallVector)
		{
			VectorCopy(&cg.snap->ps.origin, &gCGFallVector);
			gCGHasFallVector = qtrue;
		}
	}
	else
	{
		if (gCGHasFallVector)
		{
			gCGHasFallVector = qfalse;
			VectorClear(&gCGFallVector);
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

#if 0
	if (!cg_paused.integer) {
		CG_DrawBracketedEntities();
		CG_DrawUpperRight();
	}
#else
	CG_DrawBracketedEntities();
	CG_DrawUpperRight();
#endif

	if ( !CG_DrawFollow() ) {
		CG_DrawWarmup();
	}

	if (cgSiegeRoundState)
	{
		char pStr[1024];
		int rTime = 0;

		//cgSiegeRoundBeganTime = 0;

		switch (cgSiegeRoundState)
		{
		case 1:
			CG_CenterPrint(CG_GetStringEdString("MP_INGAME", "WAITING_FOR_PLAYERS"), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			break;
		case 2:
			rTime = (SIEGE_ROUND_BEGIN_TIME - (cg.time - cgSiegeRoundTime));

			if (rTime < 0)
			{
				rTime = 0;
			}
			if (rTime > SIEGE_ROUND_BEGIN_TIME)
			{
				rTime = SIEGE_ROUND_BEGIN_TIME;
			}

			rTime /= 1000;

			rTime += 1;

			if (rTime < 1)
			{
				rTime = 1;
			}

			if (rTime <= 3 && rTime != cgSiegeRoundCountTime)
			{
				cgSiegeRoundCountTime = rTime;

				switch (rTime)
				{
				case 1:
					trap->S_StartLocalSound( media.sounds.warning.count1, CHAN_ANNOUNCER );
					break;
				case 2:
					trap->S_StartLocalSound( media.sounds.warning.count2, CHAN_ANNOUNCER );
					break;
				case 3:
					trap->S_StartLocalSound( media.sounds.warning.count3, CHAN_ANNOUNCER );
					break;
				default:
					break;
				}
			}

			strcpy(pStr, va("%s %i...", CG_GetStringEdString("MP_INGAME", "ROUNDBEGINSIN"), rTime));
			CG_CenterPrint(pStr, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
			//same
			break;
		default:
			break;
		}

		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundTime)
	{
		CG_CenterPrint("", SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH);
		cgSiegeRoundTime = 0;

		//cgSiegeRoundBeganTime = cg.time;
		cgSiegeEntityRender = 0;
	}
	else if (cgSiegeRoundBeganTime)
	{ //Draw how much time is left in the round based on local info.
		int timedTeam = TEAM_FREE;
		int timedValue = 0;

		if (cgSiegeEntityRender)
		{ //render the objective item model since this client has it
			CG_DrawSiegeHUDItem();
		}

		if (team1Timed)
		{
			timedTeam = TEAM_RED; //team 1
			if (cg_beatingSiegeTime)
			{
				timedValue = cg_beatingSiegeTime;
			}
			else
			{
				timedValue = team1Timed;
			}
		}
		else if (team2Timed)
		{
			timedTeam = TEAM_BLUE; //team 2
			if (cg_beatingSiegeTime)
			{
				timedValue = cg_beatingSiegeTime;
			}
			else
			{
				timedValue = team2Timed;
			}
		}

		if (timedTeam != TEAM_FREE)
		{ //one of the teams has a timer
			int timeRemaining;
			qboolean isMyTeam = qfalse;

			if (cgs.siegeTeamSwitch && !cg_beatingSiegeTime)
			{ //in switchy mode but not beating a time, so count up.
				timeRemaining = (cg.time-cgSiegeRoundBeganTime);
				if (timeRemaining < 0)
				{
					timeRemaining = 0;
				}
			}
			else
			{
				timeRemaining = (((cgSiegeRoundBeganTime)+timedValue) - cg.time);
			}

			if (timeRemaining > timedValue)
			{
				timeRemaining = timedValue;
			}
			else if (timeRemaining < 0)
			{
				timeRemaining = 0;
			}

			if (timeRemaining)
			{
				timeRemaining /= 1000;
			}

			if (cg.predictedPlayerState.persistant[PERS_TEAM] == timedTeam)
			{ //the team that's timed is the one this client is on
				isMyTeam = qtrue;
			}

			CG_DrawSiegeTimer(timeRemaining, isMyTeam);
		}
	}
	else
	{
		cgSiegeEntityRender = 0;
	}

	if ( cg_siegeDeathTime )
	{
		int timeRemaining = ( cg_siegeDeathTime - cg.time );

		if ( timeRemaining < 0 )
		{
			timeRemaining = 0;
			cg_siegeDeathTime = 0;
		}

		if ( timeRemaining )
		{
			timeRemaining /= 1000;
		}

		CG_DrawSiegeDeathTimer( timeRemaining );
	}

	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}

	// always draw chat
	if ( !cg_newChatbox.integer )
		CG_ChatBox_DrawStrings();
	else
		CG_ChatboxDraw();
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vector3		baseOrg;
	refdef_t *refdef = CG_GetRefdef();

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && ( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		//RAZTODO: tourney scoreboard?
		cg.currentRefdef = REFDEF_SCOREBOARD;
		refdef = CG_GetRefdef();

		refdef->rdflags |= RDF_NOWORLDMODEL;

		CG_TileClear();
		if ( !cg_newChatbox.integer )
			CG_ChatBox_DrawStrings();
		else
			CG_ChatboxDraw();

		CG_ScoresDown_f();
		CG_DrawScoreboard();

		cg.currentRefdef = REFDEF_DEFAULT;
		refdef = CG_GetRefdef();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		trap->Error( ERR_DROP, "CG_DrawActive: Undefined stereoView" );
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( &refdef->vieworg, &baseOrg );
	if ( separation != 0 ) {
		VectorMA( &refdef->vieworg, -separation, &refdef->viewaxis[1], &refdef->vieworg );
	}

	refdef->rdflags |= RDF_DRAWSKYBOX;

	// draw 3D view
	trap->R_RenderScene( refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( &baseOrg, &refdef->vieworg );
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
