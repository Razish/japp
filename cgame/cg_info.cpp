// display information while data is being loading

#include "cg_local.h"
#include "ui/ui_shared.h"
#include "cg_media.h"
#include "bg_luaevent.h"

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		26

void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof(cg.infoScreenText) );
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

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	// allow colours and extended ASCII in names on loading screen
	//	Q_CleanStr( personality );

	CG_LoadingString( personality );
}

static void CG_LoadBar( void ) {
	const float barWidth = SCREEN_HEIGHT, barHeight = 12.0f;
	const float barX = (SCREEN_WIDTH - barWidth) / 2.0f, barY = SCREEN_HEIGHT - 8.0f - barHeight;
	const float capWidth = 8.0f;

	trap->R_SetColor( &colorWhite );

	// background, left cap, bar, right cap
	CG_DrawPic( barX, barY, barWidth, barHeight, media.gfx.interface.loadBarLEDSurround );
	CG_DrawPic( barX + capWidth, barY, -capWidth, barHeight, media.gfx.interface.loadBarLEDCap );
	CG_DrawPic( barX + capWidth, barY, ((barWidth - (capWidth*2.0f))*cg.loadFrac), barHeight, media.gfx.interface.loadBarLED );
	CG_DrawPic( barX + ((barWidth - (capWidth*2.0f))*cg.loadFrac) + capWidth, barY, capWidth, barHeight, media.gfx.interface.loadBarLEDCap );

	if ( cg.loadFrac > 0.0f ) {
		char text[64];
		float textWidth;
		const int fontHandle = FONT_JAPPMONO;
		const float fontScale = 1.0f;

		Com_sprintf( text, sizeof(text), "%3i%%", (int)(floorf( cg.loadFrac * 100.0f )) );
		textWidth = Text_Width( text, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (textWidth / 2.0f), SCREEN_HEIGHT - 64.0f, fontScale, &colorWhite, text, 0.0f,
			0, ITEM_TEXTSTYLE_OUTLINED, fontHandle, false
		);
	}
}

#define UI_INFOFONT (UI_BIGFONT)
// Draw all the status / pacifier stuff during level loading
//	overlays UI_DrawConnectScreen
void CG_DrawInformation( void ) {
	const char *s = nullptr;
	int y, value, valueNOFP;
	char buf[MAX_STRING_CHARS];
	int iPropHeight = 18;

	if ( JPLua::Event_ConnectScreen() ) {
		return;
	}

	const char *info = CG_ConfigString( CS_SERVERINFO );
	const char *sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	// draw the levelshot
	s = Info_ValueForKey( info, "mapname" );
	qhandle_t levelshot = trap->R_RegisterShaderNoMip( va( "levelshots/%s", s ) );
	if ( !levelshot ) {
		levelshot = trap->R_RegisterShaderNoMip( "menu/art/unknownmap_mp" );
	}
	trap->R_SetColor( NULL );
	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

	CG_LoadBar();

	// draw the icons of things as they are loaded
	//	CG_DrawLoadingIcons();

	const float fontScale = 1.0f;
	const int fontHandle = FONT_MEDIUM;
	float width = 0.0f;

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		s = va( CG_GetStringEdString( "MENUS", "LOADING_MAPNAME" ), cg.infoScreenText );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), 128 - 32, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
	}
	else {
		s = CG_GetStringEdString( "MENUS", "AWAITING_SNAPSHOT" );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), 128 - 32, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
	}

	// draw info string information
	y = 180 - 32;

	// server hostname
	Q_strncpyz( buf, Info_ValueForKey( info, "sv_hostname" ), sizeof(buf) );
	// allow colours, don't allow extended ASCII
	Q_CleanString( buf, STRIP_EXTASCII );
	width = Text_Width( buf, fontScale, fontHandle, false );
	Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, buf, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
		fontHandle, false
	);
	y += iPropHeight;

	// pure server
	if ( atoi( Info_ValueForKey( sysInfo, "sv_pure" ) ) ) {
		s = CG_GetStringEdString( "MP_INGAME", "PURE_SERVER" );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
	}

	// server-specific message of the day
	s = CG_ConfigString( CS_MOTD );
	if ( s[0] ) {
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
	}

	trap->Cvar_VariableStringBuffer( "cl_motdString", buf, sizeof(buf) );
	if ( buf[0] ) {
		width = Text_Width( buf, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), 420 - iPropHeight, fontScale, &colorWhite, buf, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
	}

	// some extra space after hostname and motd
	y += 10;

#ifdef _DEBUG
	// debug build
	s = "DEBUG BUILD";
	width = Text_Width( s, fontScale, fontHandle, false );
	Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
		fontHandle, false
	);
	y += iPropHeight;
#endif

	// JA++ version
	s = JAPP_VERSION_SMALL;
	width = Text_Width( s, fontScale, fontHandle, false );
	Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
		fontHandle, false
	);
	y += iPropHeight;

	// some extra space after hostname and motd
	y += 10;

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
			fontHandle, false
		);
		y += iPropHeight;
	}

	// cheats warning
	if ( atoi( Info_ValueForKey( sysInfo, "sv_cheats" ) ) ) {
		s = CG_GetStringEdString( "MP_INGAME", "CHEATSAREENABLED" );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
	}

	// game type
	s = BG_GetGametypeString( cgs.gametype );
	width = Text_Width( s, fontScale, fontHandle, false );
	Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0, ITEM_TEXTSTYLE_SHADOWED,
		fontHandle, false
	);
	y += iPropHeight;

	if ( cgs.gametype != GT_SIEGE ) {
		value = atoi( Info_ValueForKey( info, "timelimit" ) );
		if ( value ) {
			s = va( "%s %i", CG_GetStringEdString( "MP_INGAME", "TIMELIMIT" ), value );
			//
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}

		if ( cgs.gametype < GT_CTF ) {
			value = atoi( Info_ValueForKey( info, "fraglimit" ) );
			if ( value ) {
				s = va( "%s %i", CG_GetStringEdString( "MP_INGAME", "FRAGLIMIT" ), value );
				width = Text_Width( s, fontScale, fontHandle, false );
				Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
					ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
				);
				y += iPropHeight;
			}

			if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) {
				value = atoi( Info_ValueForKey( info, "duel_fraglimit" ) );
				if ( value ) {
					s = va( "%s %i", CG_GetStringEdString( "MP_INGAME", "WINLIMIT" ), value );
					Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
						ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
					);
					y += iPropHeight;
				}
			}
		}
	}

	if ( cgs.gametype >= GT_CTF ) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			s = va( "%s %i", CG_GetStringEdString( "MP_INGAME", "CAPTURELIMIT" ), value );
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}
	}

	if ( cgs.gametype >= GT_TEAM ) {
		value = atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) );
		if ( value ) {
			s = CG_GetStringEdString( "MP_INGAME", "FORCEBASEDTEAMS" );
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}
	}

	if ( cgs.gametype != GT_SIEGE ) {
		char fmStr[MAX_STRING_CHARS];
		trap->SE_GetStringTextString( "MP_INGAME_MAXFORCERANK", fmStr, sizeof(fmStr) );

		valueNOFP = atoi( Info_ValueForKey( info, "g_forcePowerDisable" ) );

		value = atoi( Info_ValueForKey( info, "g_maxForceRank" ) );
		if ( value && !valueNOFP && value < NUM_FORCE_MASTERY_LEVELS ) {
			s = va( "%s %s", fmStr, CG_GetStringEdString( "MP_INGAME", forceMasteryLevels[value] ) );
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}
		else if ( !valueNOFP ) {
			s = va( "%s %s",
				fmStr, CG_GetStringEdString( "MP_INGAME", forceMasteryLevels[NUM_FORCE_MASTERY_LEVELS - 1] )
			);
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}

		value = atoi( Info_ValueForKey( info, (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL)
			? "g_duelWeaponDisable" : "g_weaponDisable" ) );
		//RAZTODO: weapon disable
		if ( cgs.gametype != GT_JEDIMASTER && value ) {
			s = va( "%s", CG_GetStringEdString( "MP_INGAME", "SABERONLYSET" ) );
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}

		if ( valueNOFP ) {
			s = va( "%s", CG_GetStringEdString( "MP_INGAME", "NOFPSET" ) );
			width = Text_Width( s, fontScale, fontHandle, false );
			Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
				ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
			);
			y += iPropHeight;
		}
	}

	// Display the rules based on type
	y += iPropHeight;
	switch ( cgs.gametype ) {
	case GT_FFA:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_FFA_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_HOLOCRON:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_HOLO_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);

		y += iPropHeight;
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_HOLO_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_JEDIMASTER:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_JEDI_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_JEDI_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_SINGLE_PLAYER:
		//COOPTODO: Rules for cooperative missions - map specific?
		break;

	case GT_DUEL:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_DUEL_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_DUEL_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_POWERDUEL:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_POWERDUEL_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_POWERDUEL_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_TEAM:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_TEAM_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_TEAM_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_SIEGE:
		break;

	case GT_CTF:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTF_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTF_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;

	case GT_CTY:
		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTY_1" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;

		s = va( "%s", CG_GetStringEdString( "MP_INGAME", "RULES_CTY_2" ) );
		width = Text_Width( s, fontScale, fontHandle, false );
		Text_Paint( (SCREEN_WIDTH / 2) - (width / 2.0f), y, fontScale, &colorWhite, s, 0.0f, 0,
			ITEM_TEXTSTYLE_SHADOWED, fontHandle, false
		);
		y += iPropHeight;
		break;
	default:
		break;
	}
}
