// display information while data is being loading

#include "cg_local.h"
#include "ui/ui_shared.h"
#include "cg_media.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );
	trap->UpdateScreen();
}

void CG_LoadingItem( int itemNum ) {
	const gitem_t *item;
	char upperKey[1024];

	item = &bg_itemlist[itemNum];

	if ( !item->classname || !item->classname[0] ) {
		CG_LoadingString( "Unknown item" );
		return;
	}

	strcpy( upperKey, item->classname );
	Q_strupr( upperKey );
	CG_LoadingString( CG_GetStringEdString( "SP_INGAME", upperKey ) );
}

void CG_LoadingClient( int clientNum ) {
	const char *info;
	char personality[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof( personality ) );
	// allow colours and extended ASCII in names on loading screen
//	Q_CleanStr( personality );

	CG_LoadingString( personality );
}

static void CG_LoadBar( void ) {
	const float barWidth = 480.0f, barHeight = 12.0f;
	const float barX = (SCREEN_WIDTH-barWidth)/2.0f, barY = SCREEN_HEIGHT-8.0f-barHeight;
	const float capWidth = 8.0f;

	trap->R_SetColor( &colorWhite );

	// background, left cap, bar, right cap
	CG_DrawPic( barX, barY, barWidth, barHeight, media.gfx.interface.loadBarLEDSurround );
	CG_DrawPic( barX+capWidth, barY, -capWidth, barHeight, media.gfx.interface.loadBarLEDCap );
	CG_DrawPic( barX+capWidth, barY, ((barWidth-(capWidth*2.0f))*cg.loadFrac), barHeight, media.gfx.interface.loadBarLED );
	CG_DrawPic( barX+((barWidth-(capWidth*2.0f))*cg.loadFrac)+capWidth, barY, capWidth, barHeight, media.gfx.interface.loadBarLEDCap );

	if ( cg.loadFrac > 0.0f ) {
		char text[64];
		float textWidth;

		Com_sprintf( text, sizeof( text ), "%3i%%", (int)(floorf(cg.loadFrac * 100.0f)) );
		textWidth = CG_Text_Width( text, 1.0f, FONT_JAPPMONO );
		CG_Text_Paint( (SCREEN_WIDTH/2.0f) - (textWidth/2.0f), SCREEN_HEIGHT-64.0f, 1.0f, &colorWhite, text, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_JAPPMONO );
	}
}

#define UI_INFOFONT (UI_BIGFONT)
// Draw all the status / pacifier stuff during level loading
//	overlays UI_DrawConnectScreen
void CG_DrawInformation( void ) {
	const char *s, *info, *sysInfo;
	int y, value, valueNOFP;
	qhandle_t levelshot;
	char buf[MAX_STRING_CHARS];
	int iPropHeight = 18;

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	s = Info_ValueForKey( info, "mapname" );
	levelshot = trap->R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
	if ( !levelshot )
		levelshot = trap->R_RegisterShaderNoMip( "menu/art/unknownmap_mp" );
	trap->R_SetColor( NULL );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

	CG_LoadBar();

	// draw the icons of things as they are loaded
//	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		UI_DrawProportionalString( 320, 128-32, va( CG_GetStringEdString( "MENUS", "LOADING_MAPNAME" ), cg.infoScreenText ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	}
	else {
		UI_DrawProportionalString( 320, 128-32, CG_GetStringEdString( "MENUS", "AWAITING_SNAPSHOT" ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	}

	// draw info string information
	y = 180-32;

	// server hostname
	Q_strncpyz( buf, Info_ValueForKey( info, "sv_hostname" ), sizeof( buf ) );
	// allow colours, don't allow extended ASCII
	Q_CleanString( buf, STRIP_EXTASCII );
	UI_DrawProportionalString( 320, y, buf, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	y += iPropHeight;

	// pure server
	if ( atoi( Info_ValueForKey( sysInfo, "sv_pure" ) ) ) {
		UI_DrawProportionalString( 320, y, CG_GetStringEdString( "MP_INGAME", "PURE_SERVER" ), UI_CENTER|UI_INFOFONT|UI_DROPSHADOW,
			&colorWhite );
		y += iPropHeight;
	}

	// server-specific message of the day
	s = CG_ConfigString( CS_MOTD );
	if ( s[0] ) {
		UI_DrawProportionalString( 320, y, s, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
	}

	trap->Cvar_VariableStringBuffer( "cl_motdString", buf, sizeof( buf ) );
	if ( buf[0] )
		UI_DrawProportionalString( 320, 425, buf, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );

	// some extra space after hostname and motd
	y += 10;

#ifdef _DEBUG
	// debug build
	UI_DrawProportionalString( 320, y, "DEBUG BUILD", UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	y += iPropHeight;
#endif

	// JA++ version
	UI_DrawProportionalString( 320, y, JAPP_VERSION_SMALL, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	y += iPropHeight;

	// some extra space after hostname and motd
	y += 10;

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( 320, y, s, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
	}

	// cheats warning
	if ( atoi( Info_ValueForKey( sysInfo, "sv_cheats" ) ) ) {
		UI_DrawProportionalString( 320, y, CG_GetStringEdString( "MP_INGAME", "CHEATSAREENABLED" ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
	}

	// game type
	s = BG_GetGametypeString( cgs.gametype );
	UI_DrawProportionalString( 320, y, s, UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
	y += iPropHeight;

	if ( cgs.gametype != GT_SIEGE ) {
		value = atoi( Info_ValueForKey( info, "timelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "%s %i", CG_GetStringEdString( "MP_INGAME", "TIMELIMIT" ), value ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}

		if ( cgs.gametype < GT_CTF ) {
			value = atoi( Info_ValueForKey( info, "fraglimit" ) );
			if ( value ) {
				UI_DrawProportionalString( 320, y, va( "%s %i", CG_GetStringEdString( "MP_INGAME", "FRAGLIMIT" ), value ),
					UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
				y += iPropHeight;
			}

			if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) {
				value = atoi( Info_ValueForKey( info, "duel_fraglimit" ) );
				if ( value ) {
					UI_DrawProportionalString( 320, y, va( "%s %i", CG_GetStringEdString( "MP_INGAME", "WINLIMIT" ), value ),
						UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
					y += iPropHeight;
				}
			}
		}
	}

	if ( cgs.gametype >= GT_CTF ) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "%s %i", CG_GetStringEdString( "MP_INGAME", "CAPTURELIMIT" ), value ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}
	}

	if ( cgs.gametype >= GT_TEAM ) {
		value = atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, CG_GetStringEdString( "MP_INGAME", "FORCEBASEDTEAMS" ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}
	}

    if ( cgs.gametype != GT_SIEGE ) {
		char fmStr[MAX_STRING_CHARS];
		trap->SE_GetStringTextString( "MP_INGAME_MAXFORCERANK", fmStr, sizeof( fmStr ) );

		valueNOFP = atoi( Info_ValueForKey( info, "g_forcePowerDisable" ) );

		value = atoi( Info_ValueForKey( info, "g_maxForceRank" ) );
		if ( value && !valueNOFP && value < NUM_FORCE_MASTERY_LEVELS ) {
			UI_DrawProportionalString( 320, y, va( "%s %s", fmStr, CG_GetStringEdString( "MP_INGAME",
				forceMasteryLevels[value] ) ), UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}
		else if ( !valueNOFP ) {
			UI_DrawProportionalString( 320, y, va( "%s %s", fmStr, CG_GetStringEdString( "MP_INGAME",
				forceMasteryLevels[NUM_FORCE_MASTERY_LEVELS-1] ) ), UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}

		value = atoi( Info_ValueForKey( info, (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
			? "g_duelWeaponDisable" : "g_weaponDisable" ) );
		//RAZTODO: weapon disable
		if ( cgs.gametype != GT_JEDIMASTER && value ) {
			UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "SABERONLYSET" ) ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}

		if ( valueNOFP ) {
			UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "NOFPSET" ) ),
				UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
			y += iPropHeight;
		}
	}

	// Display the rules based on type
	y += iPropHeight;
	switch ( cgs.gametype ) {
	case GT_FFA:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_FFA_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_HOLOCRON:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_HOLO_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_HOLO_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_JEDIMASTER:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_JEDI_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_JEDI_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_SINGLE_PLAYER:
		break;

	case GT_DUEL:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_DUEL_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_DUEL_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_POWERDUEL:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_POWERDUEL_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_POWERDUEL_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_TEAM:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_TEAM_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_TEAM_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_SIEGE:
		break;

	case GT_CTF:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTF_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTF_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;

	case GT_CTY:
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTY_1" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		y += iPropHeight;
		UI_DrawProportionalString( 320, y, va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTY_2" ) ),
			UI_CENTER|UI_INFOFONT|UI_DROPSHADOW, &colorWhite );
		break;
	default:
		break;
	}
}
