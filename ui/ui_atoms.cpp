#include "ui_local.h"

#define NUM_UI_ARGSTRS (4)
#define UI_ARGSTR_MASK (NUM_UI_ARGSTRS-1)
static char tempArgStrs[NUM_UI_ARGSTRS][MAX_STRING_CHARS];

static char *UI_Argv( int arg ) {
	static int index = 0;
	char *s = tempArgStrs[index++ & UI_ARGSTR_MASK];
	trap->Cmd_Argv( arg, s, MAX_STRING_CHARS );
	return s;
}

#define NUM_UI_CVARSTRS (4)
#define UI_CVARSTR_MASK (NUM_UI_CVARSTRS-1)
static char tempCvarStrs[NUM_UI_CVARSTRS][MAX_CVAR_VALUE_STRING];

char *UI_Cvar_VariableString( const char *name ) {
	static int index = 0;
	char *s = tempCvarStrs[index++ & UI_ARGSTR_MASK];
	trap->Cvar_VariableStringBuffer( name, s, MAX_CVAR_VALUE_STRING );
	return s;
}

static void	UI_Cache_f( void ) {
	Display_CacheAll();
	if ( trap->Cmd_Argc() == 2 ) {
		int i;
		for ( i = 0; i < uiInfo.q3HeadCount; i++ ) {
			trap->Print( "model %s\n", uiInfo.q3HeadNames[i] );
		}
	}
}

qboolean UI_ConsoleCommand( int realTime ) {
	char *cmd = UI_Argv( 0 );

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	if ( Q_stricmp( cmd, "ui_report" ) == 0 ) {
		UI_Report();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ui_load" ) == 0 ) {
		UI_Load();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ui_opensiegemenu" ) == 0 ) {
		if ( trap->Cvar_VariableValue( "g_gametype" ) == GT_SIEGE ) {
			Menus_CloseAll();
			if ( Menus_ActivateByName( UI_Argv( 1 ) ) ) {
				trap->Key_SetCatcher( KEYCATCH_UI );
			}
		}
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "ui_openmenu" ) ) {
		Menus_CloseAll();
		if ( Menus_ActivateByName( UI_Argv( 1 ) ) ) {
			trap->Key_SetCatcher( KEYCATCH_UI );
		}
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ui_cache" ) == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ui_teamOrders" ) == 0 ) {
		return qtrue;
	}

	if ( Q_stricmp( cmd, "ui_cdkey" ) == 0 ) {
		return qtrue;
	}

	return qfalse;
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0, s1, t0, t1;

	if ( w < 0 ) {	// flip about vertical
		w = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if ( h < 0 ) {	// flip about horizontal
		h = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}

	trap->R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

// Coordinates are 640*480 virtual values
void UI_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_SetColor( NULL );
}

void UI_DrawSides( float x, float y, float w, float h ) {
	trap->R_DrawStretchPic( x, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - 1, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom( float x, float y, float w, float h ) {
	trap->R_DrawStretchPic( x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - 1, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

// Coordinates are 640*480 virtual values
void UI_DrawRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

	UI_DrawTopBottom( x, y, width, height );
	UI_DrawSides( x, y, width, height );

	trap->R_SetColor( NULL );
}
