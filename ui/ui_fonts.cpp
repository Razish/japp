#if defined( PROJECT_UI )
	#include "ui_local.h"
#elif defined( PROJECT_CGAME )
	#include "cgame/cg_local.h"
#endif
#include "ui/ui_shared.h"
#include "qcommon/qfiles.h"	// for STYLE_BLINK etc
#include "ui/ui_fonts.h"

#if defined(PROJECT_CGAME)
	extern displayContextDef_t cgDC;
#endif

static qhandle_t GetRealHandle( int iMenuFont ) {
#if defined(PROJECT_UI)
	const cachedAssets_t &assets = uiInfo.uiDC.Assets;
#elif defined(PROJECT_CGAME)
	const cachedAssets_t &assets = cgDC.Assets;
#endif

	switch ( iMenuFont ) {

	case FONT_SMALL: {
		return assets.qhSmallFont;
	} break;

	default:
	case FONT_MEDIUM: {
		return assets.qhMediumFont;
	} break;

	case FONT_LARGE: {
		return assets.qhBigFont;
	} break;

	case FONT_SMALL2: {
		return assets.qhSmall2Font;
	} break;

	case FONT_JAPPLARGE: {
		return assets.japp.fontLarge;
	} break;

	case FONT_JAPPSMALL: {
		return assets.japp.fontSmall;
	} break;

	case FONT_JAPPMONO: {
		return assets.japp.fontMono;
	} break;

	}
}

Font::Font( qhandle_t fontHandle, float fontScale, bool isCustomFont )
	: handle( fontHandle ), scale( fontScale ), customFont( isCustomFont )
{
}

float Font::Width( const char *text ) const {
	qhandle_t realHandle = customFont ? handle : GetRealHandle( handle );

#if 0
	float f1 = trap->ext.R_Font_StrLenPixels( text, iFontIndex, scale );
	float f2 = trap->R_Font_StrLenPixels( text, realHandle, scale );
	if ( flcmp( f1, f2 ) ) {
		// ...
	}
	return japp_preciseFonts.integer ? f1 : f2;
#else
	return japp_preciseFonts.integer
		? trap->ext.R_Font_StrLenPixels( text, realHandle, scale )
		: trap->R_Font_StrLenPixels( text, realHandle, scale );
#endif
}

float Font::Height( const char *text ) const {
	qhandle_t realHandle = customFont ? handle : GetRealHandle( handle );

	//TODO: trap->ext.***
	return trap->R_Font_HeightPixels( realHandle, scale );
}

void Font::Paint( float x, float y, const char *text, const vector4 *colour, int style, int limit, float adjust ) const {
	uint32_t iStyleOR = 0u;
	switch ( style ) {

	default:
	case ITEM_TEXTSTYLE_NORMAL: {
		// normal text
		iStyleOR = 0;
	} break;

	case ITEM_TEXTSTYLE_BLINK: // fast blinking
	case ITEM_TEXTSTYLE_PULSE: { // slow pulsing
		iStyleOR = STYLE_BLINK;
	} break;

	case ITEM_TEXTSTYLE_SHADOWED: // drop shadow
	case ITEM_TEXTSTYLE_OUTLINED: // drop shadow
	case ITEM_TEXTSTYLE_OUTLINESHADOWED: // drop shadow
	case ITEM_TEXTSTYLE_SHADOWEDMORE: { // drop shadow
		iStyleOR = STYLE_DROPSHADOW;
	} break;

	}

	//TODO: left, center, right alignment
	//TODO: styles (blink, pulse)

	trap->R_Font_DrawString(
		x, y, text, colour,
		iStyleOR | (customFont ? handle : GetRealHandle( handle )),
		limit ? limit : -1, // -1 = no limit
		scale
	);

	trap->R_SetColor( nullptr );
}
