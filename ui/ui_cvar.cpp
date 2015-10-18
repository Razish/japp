#include "ui_local.h"
#include "../game/bg_xcvar.h"

// Cvar callbacks

/*
static void CVU_Derpity( void ) {
// ...
}
*/

void UI_Set2DRatio( const xcvar *cv ) {
	if ( cv->getInt() ) {
		uiInfo.uiDC.widthRatioCoef = (float)(SCREEN_WIDTH * uiInfo.uiDC.glconfig.vidHeight)
								/ (float)(SCREEN_HEIGHT * uiInfo.uiDC.glconfig.vidWidth);
	}
	else {
		uiInfo.uiDC.widthRatioCoef = 1.0f;
	}
}

// Cvar table

#define XCVAR_DECL
#include "ui_xcvar.h"
#undef XCVAR_DECL
