#include "cg_local.h"
#include "ui/ui_shared.h"
#include "cg_media.h"
#include "bg_lua.h"
#include "ui/ui_fonts.h"

extern displayContextDef_t cgDC;

int CG_GetSelectedPlayer( void ) {
	if ( cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= numSortedTeamPlayers )
		cg_currentSelectedPlayer.integer = 0;

	return cg_currentSelectedPlayer.integer;
}

qhandle_t CG_StatusHandle( int task ) {
	switch ( task ) {
	case TEAMTASK_OFFENSE:
		return media.gfx.interface.team.assault;

	case TEAMTASK_DEFENSE:
		return media.gfx.interface.team.defend;

	case TEAMTASK_PATROL:
		return media.gfx.interface.team.patrol;

	case TEAMTASK_FOLLOW:
		return media.gfx.interface.team.follow;

	case TEAMTASK_CAMP:
		return media.gfx.interface.team.camp;

	case TEAMTASK_RETRIEVE:
		return media.gfx.interface.team.retrieve;

	case TEAMTASK_ESCORT:
		return media.gfx.interface.team.escort;

	default:
		return media.gfx.interface.team.assault;
	}
}

float CG_GetValue( int ownerDraw ) {
	centity_t *cent = &cg_entities[cg.snap->ps.clientNum];
	clientInfo_t *ci;
	playerState_t *ps = &cg.snap->ps;

	switch ( ownerDraw ) {

	case CG_SELECTEDPLAYER_ARMOR:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return ci->armor;

	case CG_SELECTEDPLAYER_HEALTH:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return ci->health;

	case CG_PLAYER_ARMOR_VALUE:
		return ps->stats[STAT_ARMOR];

	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon )
			return ps->ammo[weaponData[cent->currentState.weapon].ammoIndex];
		break;

	case CG_PLAYER_SCORE:
		return cg.snap->ps.persistant[PERS_SCORE];

	case CG_PLAYER_HEALTH:
		return ps->stats[STAT_HEALTH];

	case CG_RED_SCORE:
		return cgs.scores1;

	case CG_BLUE_SCORE:
		return cgs.scores2;

	case CG_PLAYER_FORCE_VALUE:
		return ps->fd.forcePower;

	default:
		break;
	}

	return -1;
}

// results are valid only for gametypes GT_CTF, GT_CTY and teams TEAM_RED, TEAM_BLUE
// else FLAG_ATBASE is returned
flagStatus_t CG_GetFlagStatus( team_t team ) {
	if ( cgs.gametype >= GT_CTF ) {
		if ( team == TEAM_RED ) {
			return cgs.redflag;
		}
		else if ( team == TEAM_BLUE ) {
			return cgs.blueflag;
		}
	}

	return FLAG_ATBASE;
}

bool CG_OtherTeamHasFlag( void ) {
	if ( cgs.gametype < GT_CTF ) {
		return false;
	}

	const team_t team = (team_t)cg.snap->ps.persistant[PERS_TEAM];
	const flagStatus_t status = CG_GetFlagStatus( team );
	return status == FLAG_TAKEN;
}

bool CG_YourTeamHasFlag( void ) {
	if ( cgs.gametype < GT_CTF ) {
		return false;
	}

	const team_t team = BG_GetOpposingTeam( (team_t)cg.snap->ps.persistant[PERS_TEAM] );
	const flagStatus_t status = CG_GetFlagStatus( team );
	return status == FLAG_TAKEN;
}

bool CG_OtherTeamDroppedFlag( void ) {
	if ( cgs.gametype < GT_CTF ) {
		return false;
	}

	const team_t team = BG_GetOpposingTeam( (team_t)cg.snap->ps.persistant[PERS_TEAM] );
	const flagStatus_t status = CG_GetFlagStatus( team );
	return status == FLAG_DROPPED;
}

bool CG_YourTeamDroppedFlag( void ) {
	if ( cgs.gametype < GT_CTF ) {
		return false;
	}

	const team_t team = BG_GetOpposingTeam( (team_t)cg.snap->ps.persistant[PERS_TEAM] );
	const flagStatus_t status = CG_GetFlagStatus( team );
	return status == FLAG_DROPPED;
}

// FIXME: should these be exclusive or inclusive..
qboolean CG_OwnerDrawVisible( uint32_t flags ) {
	if ( flags & CG_SHOW_TEAMINFO )
		return (cg_currentSelectedPlayer.integer == numSortedTeamPlayers);

	if ( flags & CG_SHOW_NOTEAMINFO )
		return !(cg_currentSelectedPlayer.integer == numSortedTeamPlayers);

	if ( flags & CG_SHOW_OTHERTEAMHASFLAG )
		return CG_OtherTeamHasFlag();

	if ( flags & CG_SHOW_YOURTEAMHASENEMYFLAG )
		return CG_YourTeamHasFlag();

	if ( flags & (CG_SHOW_BLUE_TEAM_HAS_REDFLAG | CG_SHOW_RED_TEAM_HAS_BLUEFLAG) ) {
		if ( flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG && cgs.redflag == FLAG_TAKEN )
			return qtrue;
		else if ( flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG && cgs.blueflag == FLAG_TAKEN )
			return qtrue;
		return qfalse;
	}

	if ( flags & CG_SHOW_ANYTEAMGAME ) {
		if ( cgs.gametype >= GT_TEAM )
			return qtrue;
	}

	if ( flags & CG_SHOW_ANYNONTEAMGAME ) {
		if ( cgs.gametype < GT_TEAM )
			return qtrue;
	}

	if ( flags & CG_SHOW_CTF ) {
		if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTY )
			return qtrue;
	}

	if ( flags & CG_SHOW_HEALTHCRITICAL ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] < 25 )
			return qtrue;
	}

	if ( flags & CG_SHOW_HEALTHOK ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] >= 25 )
			return qtrue;
	}

	if ( flags & CG_SHOW_SINGLEPLAYER ) {
		if ( cgs.gametype == GT_SINGLE_PLAYER )
			return qtrue;
	}

	if ( flags & CG_SHOW_TOURNAMENT ) {
		if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL )
			return qtrue;
	}

	if ( flags & CG_SHOW_DURINGINCOMINGVOICE ) {
		// ...
	}

	if ( flags & CG_SHOW_IF_PLAYER_HAS_FLAG ) {
		if ( cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG] )
			return qtrue;
	}
	return qfalse;
}

const char *CG_GetKillerText( void ) {
	if ( cg.killerName[0] )
		return va( "%s %s", CG_GetStringEdString( "MP_INGAME", "KILLEDBY" ), cg.killerName );
	return "";
}

const char *CG_GetGameStatusText( void ) {
	if ( cgs.gametype == GT_POWERDUEL )
		return "";

	else if ( cgs.gametype < GT_TEAM ) {
		if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			char sPlaceWith[256];
			trap->SE_GetStringTextString( "MP_INGAME_PLACE_WITH", sPlaceWith, sizeof(sPlaceWith) );

			return va( "%s %s %i", CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ), sPlaceWith, cg.snap->ps.persistant[PERS_SCORE] );
		}
	}

	else {
		if ( cg.teamScores[0] == cg.teamScores[1] )
			return va( "%s %i", CG_GetStringEdString( "MP_INGAME", "TIEDAT" ), cg.teamScores[0] );
		else if ( cg.teamScores[0] >= cg.teamScores[1] )
			return va( "%s, %i / %i", CG_GetStringEdString( "MP_INGAME", "RED_LEADS" ), cg.teamScores[0], cg.teamScores[1] );
		else
			return va( "%s, %i / %i", CG_GetStringEdString( "MP_INGAME", "BLUE_LEADS" ), cg.teamScores[1], cg.teamScores[0] );
	}

	return "";
}

// maxX param is initially an X limit, but is also used as feedback. 0 = text was clipped to fit within, else maxX = next pos
void CG_Text_Paint_Limit( float *maxX, float x, float y, float scale, const vector4 *color, const char *text, float adjust,
	int limit, int iMenuFont, bool customFont )
{
	qboolean bIsTrailingPunctuation;

	//float fMax = *maxX;
	const Font font( iMenuFont, scale, customFont );
	float iPixelLen = font.Width( text );
	if ( x + iPixelLen > *maxX ) {
		// whole text won't fit, so we need to print just the amount that does...
		//  Ok, this is slow and tacky, but only called occasionally, and it works...
		char sTemp[4096] = { 0 };	// lazy assumption
		const char *psText = text;
		char *psOut = &sTemp[0];
		char *psOutLastGood = psOut;
		unsigned int uiLetter;

		while ( *psText && (x + font.Width( sTemp ) <= *maxX)
			&& psOut < &sTemp[sizeof(sTemp)-1] ) {
			int iAdvanceCount;
			psOutLastGood = psOut;

			uiLetter = trap->R_AnyLanguage_ReadCharFromString( psText, &iAdvanceCount, &bIsTrailingPunctuation );
			psText += iAdvanceCount;

			if ( uiLetter > 255 ) {
				*psOut++ = uiLetter >> 8;
				*psOut++ = uiLetter & 0xFF;
			}
			else
				*psOut++ = uiLetter & 0xFF;
		}
		*psOutLastGood = '\0';

		*maxX = 0; // feedback
		font.Paint( x, y, sTemp, color, ITEM_TEXTSTYLE_NORMAL, limit, adjust );
	}
	else {
		// whole text fits fine, so print it all...
		*maxX = x + iPixelLen;	// feedback the next position, as the caller expects
		font.Paint( x, y, text, color, ITEM_TEXTSTYLE_NORMAL, limit, adjust );
	}
}

#define PIC_WIDTH 12

void CG_DrawNewTeamInfo( rectDef_t *rect, float text_x, float text_y, float scale, const vector4 *color, qhandle_t shader ) {
	int xx, i, j, len, count;
	float y, pwidth, lwidth, maxx, leftOver;
	const char *p;
	vector4		hcolor;
	clientInfo_t *ci;
	const gitem_t *item;
	qhandle_t h;
	const Font font( FONT_MEDIUM, scale, false );

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for ( i = 0; i<count; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			len = font.Width( ci->name );
			if ( len > pwidth )
				pwidth = len;
		}
	}

	// max location name width
	lwidth = 0;
	for ( i = 1; i<MAX_LOCATIONS; i++ ) {
		p = CG_GetLocationString( CG_ConfigString( CS_LOCATIONS + i ) );
		if ( p && *p ) {
			len = font.Width( p );
			if ( len > lwidth )
				lwidth = len;
		}
	}

	y = rect->y;

	for ( i = 0; i < count; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			xx = rect->x + 1;
			for ( j = 0; j <= PW_NUM_POWERUPS; j++ ) {
				if ( ci->powerups & (1 << j) ) {
					if ( (item = BG_FindItemForPowerup( (powerup_t)j )) ) {
						CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, trap->R_RegisterShader( item->icon ) );
						xx += PIC_WIDTH;
					}
				}
			}

			// FIXME: max of 3 powerups shown properly
			xx = rect->x + (PIC_WIDTH * 3) + 2;

			CG_GetColorForHealth( ci->health, ci->armor, &hcolor );
			trap->R_SetColor( &hcolor );
			CG_DrawPic( xx, y + 1, PIC_WIDTH - 2, PIC_WIDTH - 2, media.gfx.interface.heart );

			//	Com_sprintf( st, sizeof(st), "%3i %3i", ci->health,	ci->armor );
			//	font.Paint( xx, y + text_y, scale, hcolor, st, 0, 0 );

			// draw weapon icon
			xx += PIC_WIDTH + 1;

			trap->R_SetColor( NULL );
			h = CG_StatusHandle( ci->teamTask );

			if ( h )
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, h );

			xx += PIC_WIDTH + 1;

			leftOver = rect->w - xx;
			maxx = xx + leftOver / 3;

			CG_Text_Paint_Limit( &maxx, xx, y + text_y, scale, color, ci->name, 0, 0, FONT_MEDIUM, false );

			p = CG_GetLocationString( CG_ConfigString( CS_LOCATIONS + ci->location ) );
			if ( !p || !*p )
				p = "unknown";

			xx += leftOver / 3 + 2;
			maxx = rect->w - 4;

			CG_Text_Paint_Limit( &maxx, xx, y + text_y, scale, color, p, 0, 0, FONT_MEDIUM, false );
			y += text_y + 2;
			if ( y + text_y + 2 > rect->y + rect->h )
				break;
		}
	}
}

void CG_DrawTeamSpectators( rectDef_t *rect, float scale, const vector4 *color, qhandle_t shader ) {
	if ( cg.spectatorLen ) {
		float maxX;

		if ( cg.spectatorWidth == -1 ) {
			cg.spectatorWidth = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if ( cg.spectatorOffset > cg.spectatorLen ) {
			cg.spectatorOffset = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if ( cg.time > cg.spectatorTime ) {
			cg.spectatorTime = cg.time + 10;
			if ( cg.spectatorPaintX <= rect->x + 2 ) {
				if ( cg.spectatorOffset < cg.spectatorLen ) {
					const Font font( FONT_MEDIUM, scale, false );
					cg.spectatorPaintX += font.Width( &cg.spectatorList[cg.spectatorOffset] )
										- 1;
					cg.spectatorOffset++;
				}
				else {
					cg.spectatorOffset = 0;
					if ( cg.spectatorPaintX2 >= 0 )
						cg.spectatorPaintX = cg.spectatorPaintX2;
					else
						cg.spectatorPaintX = rect->x + rect->w - 2;
					cg.spectatorPaintX2 = -1;
				}
			}
			else {
				cg.spectatorPaintX--;
				if ( cg.spectatorPaintX2 >= 0 )
					cg.spectatorPaintX2--;
			}
		}

		maxX = rect->x + rect->w - 2;
		CG_Text_Paint_Limit( &maxX, cg.spectatorPaintX, rect->y + rect->h - 3, scale, color,
			&cg.spectatorList[cg.spectatorOffset], 0, 0, FONT_MEDIUM, false );
		if ( cg.spectatorPaintX2 >= 0 ) {
			float maxX2 = rect->x + rect->w - 2;
			CG_Text_Paint_Limit( &maxX2, cg.spectatorPaintX2, rect->y + rect->h - 3, scale, color, cg.spectatorList, 0,
				cg.spectatorOffset, FONT_MEDIUM, false );
		}
		if ( cg.spectatorOffset && maxX > 0 ) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if ( cg.spectatorPaintX2 == -1 )
				cg.spectatorPaintX2 = rect->x + rect->w - 2;
		}
		else
			cg.spectatorPaintX2 = -1;
	}
}

void CG_DrawMedal( int ownerDraw, rectDef_t *rect, float scale, const vector4 *color, qhandle_t shader ) {
	score_t *score = &cg.scores[cg.selectedScore];
	float value = 0;
	const char *text = NULL;
	vector4 newColour;

	VectorCopy4( color, &newColour );
	newColour.a = 0.25f;

	switch ( ownerDraw ) {
	case CG_ACCURACY:
		value = score->accuracy;
		break;

	case CG_ASSISTS:
		value = score->assistCount;
		break;

	case CG_DEFEND:
		value = score->defendCount;
		break;

	case CG_EXCELLENT:
		value = score->excellentCount;
		break;

	case CG_IMPRESSIVE:
		value = score->impressiveCount;
		break;

	case CG_PERFECT:
		value = score->perfect;
		break;

	case CG_GAUNTLET:
		value = score->gauntletCount;
		break;

	case CG_CAPTURES:
		value = score->captures;
		break;

	default:
		break;

	}

	if ( value > 0 ) {
		if ( ownerDraw != CG_PERFECT ) {
			if ( ownerDraw == CG_ACCURACY ) {
				text = va( "%i%%", (int)value );
				if ( value > 50 )
					newColour.a = 1.0f;
			}
			else {
				text = va( "%i", (int)value );
				newColour.a = 1.0f;
			}
		}
		else {
			if ( value )
				newColour.a = 1.0f;
			text = "Wow";
		}
	}

	trap->R_SetColor( &newColour );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );

	if ( text ) {
		newColour.a = 1.0f;
		const Font font( FONT_MEDIUM, scale, false );
		value = font.Width( text );
		font.Paint( rect->x + (rect->w - value) / 2, rect->y + rect->h + 10, text, &newColour );
	}

	trap->R_SetColor( NULL );
}

void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, uint32_t ownerDrawFlags,
	int align, float special, float scale, const vector4 *color, qhandle_t shader, int textStyle, int iMenuFont,
	bool customFont )
{
	//Ignore all this, at least for now. May put some stat stuff back in menu files later.
#if 0
	rectDef_t rect;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	/*
	if ( ownerDrawFlags != 0 && !CG_OwnerDrawVisible( ownerDrawFlags ) ) {
		return;
	}
	*/

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case CG_PLAYER_ARMOR_ICON:
		CG_DrawPlayerArmorIcon( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;

	case CG_PLAYER_ARMOR_ICON2D:
		CG_DrawPlayerArmorIcon( &rect, qtrue );
		break;

	case CG_PLAYER_ARMOR_VALUE:
		CG_DrawPlayerArmorValue( &rect, scale, color, shader, textStyle );
		break;

	case CG_PLAYER_FORCE_VALUE:
		CG_DrawPlayerForceValue( &rect, scale, color, shader, textStyle );
		return;

	case CG_PLAYER_AMMO_ICON:
		CG_DrawPlayerAmmoIcon( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;

	case CG_PLAYER_AMMO_ICON2D:
		CG_DrawPlayerAmmoIcon( &rect, qtrue );
		break;

	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue( &rect, scale, color, shader, textStyle );
		break;

	case CG_SELECTEDPLAYER_HEAD:
		CG_DrawSelectedPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse );
		break;

	case CG_VOICE_HEAD:
		CG_DrawSelectedPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue );
		break;

	case CG_VOICE_NAME:
		CG_DrawSelectedPlayerName( &rect, scale, color, qtrue, textStyle );
		break;

	case CG_SELECTEDPLAYER_STATUS:
		CG_DrawSelectedPlayerStatus( &rect );
		break;

	case CG_SELECTEDPLAYER_ARMOR:
		CG_DrawSelectedPlayerArmor( &rect, scale, color, shader, textStyle );
		break;

	case CG_SELECTEDPLAYER_HEALTH:
		CG_DrawSelectedPlayerHealth( &rect, scale, color, shader, textStyle );
		break;

	case CG_SELECTEDPLAYER_NAME:
		CG_DrawSelectedPlayerName( &rect, scale, color, qfalse, textStyle );
		break;

	case CG_SELECTEDPLAYER_LOCATION:
		CG_DrawSelectedPlayerLocation( &rect, scale, color, textStyle );
		break;

	case CG_SELECTEDPLAYER_WEAPON:
		CG_DrawSelectedPlayerWeapon( &rect );
		break;

	case CG_SELECTEDPLAYER_POWERUP:
		CG_DrawSelectedPlayerPowerup( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;

	case CG_PLAYER_HEAD:
		CG_DrawPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;

	case CG_PLAYER_ITEM:
		CG_DrawPlayerItem( &rect, scale, ownerDrawFlags & CG_SHOW_2DONLY );
		break;

	case CG_PLAYER_SCORE:
		CG_DrawPlayerScore( &rect, scale, color, shader, textStyle );
		break;

	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth( &rect, scale, color, shader, textStyle );
		break;

	case CG_RED_SCORE:
		CG_DrawRedScore( &rect, scale, color, shader, textStyle );
		break;

	case CG_BLUE_SCORE:
		CG_DrawBlueScore( &rect, scale, color, shader, textStyle );
		break;

	case CG_RED_NAME:
		CG_DrawRedName( &rect, scale, color, textStyle );
		break;

	case CG_BLUE_NAME:
		CG_DrawBlueName( &rect, scale, color, textStyle );
		break;

	case CG_BLUE_FLAGHEAD:
		CG_DrawBlueFlagHead( &rect );
		break;

	case CG_BLUE_FLAGSTATUS:
		CG_DrawBlueFlagStatus( &rect, shader );
		break;

	case CG_BLUE_FLAGNAME:
		CG_DrawBlueFlagName( &rect, scale, color, textStyle );
		break;

	case CG_RED_FLAGHEAD:
		CG_DrawRedFlagHead( &rect );
		break;

	case CG_RED_FLAGSTATUS:
		CG_DrawRedFlagStatus( &rect, shader );
		break;

	case CG_RED_FLAGNAME:
		CG_DrawRedFlagName( &rect, scale, color, textStyle );
		break;

	case CG_PLAYER_LOCATION:
		CG_DrawPlayerLocation( &rect, scale, color, textStyle );
		break;

	case CG_TEAM_COLOR:
		CG_DrawTeamColor( &rect, color );
		break;

	case CG_CTF_POWERUP:
		CG_DrawCTFPowerUp( &rect );
		break;

	case CG_AREA_POWERUP:
		CG_DrawAreaPowerUp( &rect, align, special, scale, color );
		break;

	case CG_PLAYER_STATUS:
		CG_DrawPlayerStatus( &rect );
		break;

	case CG_PLAYER_HASFLAG:
		CG_DrawPlayerHasFlag( &rect, qfalse );
		break;

	case CG_PLAYER_HASFLAG2D:
		CG_DrawPlayerHasFlag( &rect, qtrue );
		break;

	case CG_AREA_SYSTEMCHAT:
		CG_DrawAreaSystemChat( &rect, scale, color, shader );
		break;

	case CG_AREA_TEAMCHAT:
		CG_DrawAreaTeamChat( &rect, scale, color, shader );
		break;

	case CG_AREA_CHAT:
		CG_DrawAreaChat( &rect, scale, color, shader );
		break;

	case CG_GAME_TYPE:
		CG_DrawGameType( &rect, scale, color, shader, textStyle );
		break;

	case CG_GAME_STATUS:
		CG_DrawGameStatus( &rect, scale, color, shader, textStyle );
		break;

	case CG_KILLER:
		CG_DrawKiller( &rect, scale, color, shader, textStyle );
		break;

	case CG_ACCURACY:
	case CG_ASSISTS:
	case CG_DEFEND:
	case CG_EXCELLENT:
	case CG_IMPRESSIVE:
	case CG_PERFECT:
	case CG_GAUNTLET:
	case CG_CAPTURES:
		CG_DrawMedal( ownerDraw, &rect, scale, color, shader );
		break;

	case CG_SPECTATORS:
		CG_DrawTeamSpectators( &rect, scale, color, shader );
		break;

	case CG_TEAMINFO:
		if ( cg_currentSelectedPlayer.integer == numSortedTeamPlayers )
			CG_DrawNewTeamInfo( &rect, text_x, text_y, scale, color, shader );
		break;

	case CG_CAPFRAGLIMIT:
		CG_DrawCapFragLimit( &rect, scale, color, shader, textStyle );
		break;

	case CG_1STPLACE:
		CG_Draw1stPlace( &rect, scale, color, shader, textStyle );
		break;

	case CG_2NDPLACE:
		CG_Draw2ndPlace( &rect, scale, color, shader, textStyle );
		break;

	default:
		break;
	}
#endif
}

void CG_MouseEvent( int x, int y ) {
	cgDC.cursorx = cgs.cursorX = Q_clamp( 0.0f, cgs.cursorX + x, SCREEN_WIDTH );
	cgDC.cursory = cgs.cursorY = Q_clamp( 0.0f, cgs.cursorY + y, SCREEN_HEIGHT );

	cursorType_e cursorType = Display_CursorType( cgs.cursorX, cgs.cursorY );
	if ( cursorType == CURSOR_NONE
		|| cursorType == CURSOR_ARROW )
	{
		cgs.activeCursor = media.gfx.interface.cursor;
	}
	else if ( cursorType == CURSOR_SIZER ) {
		cgs.activeCursor = media.gfx.interface.cursorSize;
	}

	if ( cgs.capturedItem ) {
		Display_MouseMove( cgs.capturedItem, x, y );
	}
	else {
		Display_MouseMove( NULL, cgs.cursorX, cgs.cursorY );
	}
}

void CG_EventHandling( int type ) {
	switch ( type ) {
	case CGAME_EVENT_NONE:
		CG_ChatboxClose();
		break;

	default:
		break;
	}
}

void CG_KeyEvent( int key, qboolean down ) {
	if ( !down ) {
		return;
	}

	if (JPLua::Event_KeyDown(key)){
		return;
	}

	if ( CG_ChatboxActive() ) {
		CG_ChatboxChar( key );
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL || cg.predictedPlayerState.pm_type == PM_JETPACK
		|| cg.predictedPlayerState.pm_type == PM_NORMAL || (cg.predictedPlayerState.pm_type == PM_SPECTATOR
		&& cg.showScores == qfalse) ) {
		CG_EventHandling( CGAME_EVENT_NONE );
		trap->Key_SetCatcher( 0 );
		return;
	}

	Display_HandleKey( key, down, cgs.cursorX, cgs.cursorY );

	if ( cgs.capturedItem ) {
		cgs.capturedItem = NULL;
	}
	else if ( key == A_MOUSE2 && down ) {
		cgs.capturedItem = Display_CaptureItem( cgs.cursorX, cgs.cursorY );
	}
}

int CG_ClientNumFromName( const char *p ) {
	int i;
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( cgs.clientinfo[i].infoValid && !Q_stricmp( cgs.clientinfo[i].name, p ) )
			return i;
	}

	return -1;
}

void CG_ShowResponseHead( void ) {
	Menus_OpenByName( "voiceMenu" );
	trap->Cvar_Set( "cl_conXOffset", "72" );
	cg.voiceTime = cg.time;
}

void CG_RunMenuScript( char **args ) {
}

qboolean CG_DeferMenuScript( char **args ) {
	return qfalse;
}

void CG_GetTeamColor( vector4 *color ) {
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		color->r = 1.0f;
		color->g = 0.0f;
		color->b = 0.0f;
		color->a = 0.25f;
	}
	else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		color->r = 0.0f;
		color->g = 0.0f;
		color->b = 1.0f;
		color->a = 0.25f;
	}
	else {
		color->r = 0.0f;
		color->g = 0.17f;
		color->b = 0.0f;
		color->a = 0.25f;
	}
}
