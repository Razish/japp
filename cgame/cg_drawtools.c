// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"
#include "qcommon/q_shared.h"
#include "cg_media.h"

/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const vector4 *color ) {
	trap->R_SetColor( color );

	CG_DrawTopBottom( x, y, width, height, size );
	CG_DrawSides( x, y, width, height, size );

	trap->R_SetColor( NULL );
}



/*
=================
CG_GetColorForHealth
=================
*/
void CG_GetColorForHealth( int health, int armor, vector4 *hcolor ) {
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear4( hcolor );	// black
		hcolor->a = 1;
		return;
	}
	count = armor;
	max = health * ARMOR_PROTECTION / (1.0f - ARMOR_PROTECTION);
	if ( max < count ) {
		count = max;
	}
	health += count;

	// set the color based on health
	hcolor->r = 1.0f;
	hcolor->a = 1.0f;
	if ( health >= 100 )	hcolor->b = 1.0f;
	else if ( health < 66 )		hcolor->b = 0;
	else						hcolor->b = (health - 66) / 33.0f;

	if ( health > 60 )		hcolor->g = 1.0f;
	else if ( health < 30 )		hcolor->g = 0;
	else						hcolor->g = (health - 30) / 30.0f;
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides( float x, float y, float w, float h, float size ) {
	size *= cgs.screenXScale;
	trap->R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, media.gfx.world.whiteShader );
	trap->R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, media.gfx.world.whiteShader );
}

void CG_DrawTopBottom( float x, float y, float w, float h, float size ) {
	size *= cgs.screenYScale;
	trap->R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, media.gfx.world.whiteShader );
	trap->R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, media.gfx.world.whiteShader );
}

/*
-------------------------
CGC_FillRect2
real coords
-------------------------
*/
void CG_FillRect2( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, media.gfx.world.whiteShader );
	trap->R_SetColor( NULL );
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, media.gfx.world.whiteShader );

	trap->R_SetColor( NULL );
}


/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	//	x *= (4.0f/3.0f) / ((float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight);
	//	width *= (4.0f/3.0f) / ((float)cgs.glconfig.vidWidth/(float)cgs.glconfig.vidHeight);
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
CG_DrawRotatePic

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
rotates around the upper right corner of the passed in point
=================
*/
void CG_DrawRotatePic( float x, float y, float width, float height, float angle, qhandle_t hShader ) {
	trap->R_DrawRotatePic( x, y, width, height, 0, 0, 1, 1, angle, hShader );
}

/*
================
CG_DrawRotatePic2

Coordinates are 640*480 virtual values
A width of 0 will draw with the original image width
Actually rotates around the center point of the passed in coordinates
=================
*/
void CG_DrawRotatePic2( float x, float y, float width, float height, float angle, qhandle_t hShader ) {
	trap->R_DrawRotatePic2( x, y, width, height, 0, 0, 1, 1, angle, hShader );
}

/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float	ax, ay, aw, ah;
	float size2;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;

	row = ch >> 4;
	col = ch & 15;

	frow = row*0.0625f;
	fcol = col*0.0625f;
	size = 0.03125f;
	size2 = 0.0625f;

	trap->R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + size, frow + size2, media.gfx.interface.charset );

}

/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
#include "ui/menudef.h"	// for "ITEM_TEXTSTYLE_SHADOWED"
void CG_DrawStringExt( int x, int y, const char *string, const vector4 *setColor,
	qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars ) {
	if ( trap->R_Language_IsAsian() ) {
		// hack-a-doodle-do (post-release quick fix code)...
		//
		vector4 color;
		memcpy( &color, setColor, sizeof(color) );	// de-const it
		CG_Text_Paint( x, y, 1.0f,	// float scale,
			&color,		// vector4 color,
			string,		// const char *text,
			0.0f,		// float adjust,
			0,			// int limit,
			shadow ? ITEM_TEXTSTYLE_SHADOWED : 0,	// int style,
			FONT_MEDIUM		// iMenuFont
			);
	}
	else {
		vector4		color;
		const char	*s;
		int			xx;

		// draw the drop shadow
		if ( shadow ) {
			color.r = color.g = color.b = 0;
			color.a = setColor->a;
			trap->R_SetColor( &color );
			s = string;
			xx = x;
			while ( *s ) {
				if ( Q_IsColorString( s ) ) {
					s += 2;
					continue;
				}
				CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
				xx += charWidth;
				s++;
			}
		}

		// draw the colored text
		s = string;
		xx = x;
		trap->R_SetColor( setColor );
		while ( *s ) {
			if ( Q_IsColorString( s ) ) {
				if ( !forceColor ) {
					memcpy( &color, &g_color_table[ColorIndex( *(s + 1) )], sizeof(color) );
					color.a = setColor->a;
					trap->R_SetColor( &color );
				}
				s += 2;
				continue;
			}
			CG_DrawChar( xx, y, charWidth, charHeight, *s );
			xx += charWidth;
			s++;
		}
		trap->R_SetColor( NULL );
	}
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	vector4 color;

	VectorSet4( &color, 1.0f, 1.0f, 1.0f, alpha );
	CG_DrawStringExt( x, y, s, &color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigStringColor( int x, int y, const char *s, const vector4 *color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	vector4 color;

	VectorSet4( &color, 1.0f, 1.0f, 1.0f, alpha );
	CG_DrawStringExt( x, y, s, &color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, const vector4 *color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		}
		else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x / 64.0f;
	t1 = y / 64.0f;
	s2 = (x + w) / 64.0f;
	t2 = (y + h) / 64.0f;
	trap->R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	int		w, h;
	refdef_t *refdef = CG_GetRefdef();

	w = cgs.glconfig.vidWidth;
	h = cgs.glconfig.vidHeight;

	if ( refdef->x == 0 && refdef->y == 0 &&
		refdef->width == w && refdef->height == h ) {
		return;		// full screen rendering
	}

	top = refdef->y;
	bottom = top + refdef->height - 1;
	left = refdef->x;
	right = left + refdef->width - 1;

	// clear above view screen
	CG_TileClearBox( 0, 0, w, top, media.gfx.interface.backTile );

	// clear below view screen
	CG_TileClearBox( 0, bottom, w, h - bottom, media.gfx.interface.backTile );

	// clear left of view screen
	CG_TileClearBox( 0, top, left, bottom - top + 1, media.gfx.interface.backTile );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, media.gfx.interface.backTile );
}



/*
================
CG_FadeColor
================
*/
vector4 *CG_FadeColor( int startMsec, int totalMsec ) {
	static vector4 color;
	int			t;
	int fadeTime = 500;//FADE_TIME;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	//Raz: This colour shouldn't be visible yet
	if ( t < 0 )
		return NULL;

	// fade out
	VectorSet4( &color, 1.0f, 1.0f, 1.0f, 1.0f );
	if ( totalMsec - t < fadeTime )
		color.a = (totalMsec - t) * 1.0f / fadeTime;

	return &color;
}

//Returns qtrue if the fade is complete, don't draw shadows etc
//			qfalse if it has either partially faded, or not at all.
qboolean CG_FadeColor2( vector4 *color, int startMsec, int totalMsec ) {
	float dt = cg.time - startMsec;

	if ( dt > totalMsec ) {
		color->a = 0.0f;
		return qtrue;
	}
	//RAZTODO: else if ( dt < 0 )
	else {
		dt = Q_fabs( dt );
		color->a = 1.0f - dt / totalMsec;
		return qfalse;
	}
}

void CG_LerpColour( const vector4 *start, const vector4 *end, vector4 *out, float point ) {
	out->r = start->r + point*(end->r - start->r);
	out->g = start->g + point*(end->g - start->g);
	out->b = start->b + point*(end->b - start->b);
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForGivenHealth( vector4 *hcolor, int health ) {
	// set the color based on health
	hcolor->r = 1.0f;
	if ( health >= 100 )	hcolor->b = 1.0f;
	else if ( health < 66 )		hcolor->b = 0;
	else						hcolor->b = (health - 66) / 33.0f;

	if ( health > 60 )		hcolor->g = 1.0f;
	else if ( health < 30 ) 	hcolor->g = 0;
	else						hcolor->g = (health - 30) / 30.0f;
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForHealth( vector4 *hcolor ) {
	int		health;
	int		count;
	int		max;

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	health = cg.snap->ps.stats[STAT_HEALTH];

	if ( health <= 0 ) {
		VectorClear4( hcolor );	// black
		hcolor->a = 1;
		return;
	}

	count = cg.snap->ps.stats[STAT_ARMOR];
	max = health * ARMOR_PROTECTION / (1.0f - ARMOR_PROTECTION);
	if ( max < count )
		count = max;
	health += count;

	hcolor->a = 1.0f;
	CG_ColorForGivenHealth( hcolor, health );
}

/*
==============
CG_DrawNumField

Take x,y positions as if 640 x 480 and scales them to the proper resolution

==============
*/
void CG_DrawNumField( int x, int y, int width, int value, int charWidth, int charHeight, int style, qboolean zeroFill ) {
	char	num[16], *ptr;
	int		l;
	int		frame;
	int		xWidth;
	int		i = 0;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	default:
		break;
	}

	Com_sprintf( num, sizeof(num), "%i", value );
	l = strlen( num );
	if ( l > width )
		l = width;

	// FIXME: Might need to do something different for the chunky font??
	switch ( style ) {
	case NUM_FONT_SMALL:
		xWidth = charWidth;
		break;
	case NUM_FONT_CHUNKY:
		xWidth = (charWidth / 1.2f) + 2;
		break;
	default:
	case NUM_FONT_BIG:
		xWidth = (charWidth / 2) + 7;//(charWidth/6);
		break;
	}

	if ( zeroFill ) {
		for ( i = 0; i < (width - l); i++ ) {
			switch ( style ) {
			case NUM_FONT_SMALL:
				CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.smallNumbers[0] );
				break;
			case NUM_FONT_CHUNKY:
				CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.chunkyNumbers[0] );
				break;
			default:
			case NUM_FONT_BIG:
				CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.numbers[0] );
				break;
			}
			x += 2 + (xWidth);
		}
	}
	else {
		x += 2 + (xWidth)*(width - l);
	}

	ptr = num;
	while ( *ptr && l ) {
		if ( *ptr == '-' )
			frame = STAT_MINUS;
		else
			frame = *ptr - '0';

		switch ( style ) {
		case NUM_FONT_SMALL:
			CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.smallNumbers[frame] );
			x++;	// For a one line gap
			break;
		case NUM_FONT_CHUNKY:
			CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.chunkyNumbers[frame] );
			break;
		default:
		case NUM_FONT_BIG:
			CG_DrawPic( x, y, charWidth, charHeight, media.gfx.interface.numbers[frame] );
			break;
		}

		x += (xWidth);
		ptr++;
		l--;
	}

}

#include "ui/ui_shared.h"	// for some text style junk
void UI_DrawProportionalString( int x, int y, const char* str, int style, const vector4 *color ) {
	// having all these different style defines (1 for UI, one for CG, and now one for the re->font stuff)
	//	is dumb, but for now...
	//
	int iStyle = 0;
	int iMenuFont = (style & UI_SMALLFONT) ? FONT_SMALL : FONT_MEDIUM;

	switch ( style & (UI_LEFT | UI_CENTER | UI_RIGHT) ) {
	default:
	case UI_LEFT:
		// nada...
		break;

	case UI_CENTER:
		x -= CG_Text_Width( str, 1.0f, iMenuFont ) / 2;
		break;

	case UI_RIGHT:
		x -= CG_Text_Width( str, 1.0f, iMenuFont ) / 2;
		break;
	}

	if ( style & UI_DROPSHADOW )
		iStyle = ITEM_TEXTSTYLE_SHADOWED;
	else if ( style & (UI_BLINK | UI_PULSE) )
		iStyle = ITEM_TEXTSTYLE_BLINK;

	CG_Text_Paint( x, y, 1.0f, color, str, 0, 0, iStyle, iMenuFont );
}

void UI_DrawScaledProportionalString( int x, int y, const char* str, int style, const vector4 *color, float scale ) {
	// having all these different style defines (1 for UI, one for CG, and now one for the re->font stuff)
	//	is dumb, but for now...
	//
	int iStyle = 0;

	switch ( style & (UI_LEFT | UI_CENTER | UI_RIGHT) ) {
	default:
	case UI_LEFT:
		// nada...
		break;

	case UI_CENTER:
		x -= CG_Text_Width( str, scale, FONT_MEDIUM ) / 2;
		break;

	case UI_RIGHT:
		x -= CG_Text_Width( str, scale, FONT_MEDIUM ) / 2;
		break;
	}

	if ( style & UI_DROPSHADOW )
		iStyle = ITEM_TEXTSTYLE_SHADOWED;
	else if ( style & (UI_BLINK | UI_PULSE) )
		iStyle = ITEM_TEXTSTYLE_BLINK;

	CG_Text_Paint( x, y, scale, color, str, 0, 0, iStyle, FONT_MEDIUM );
}




