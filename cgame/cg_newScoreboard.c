#include "cg_local.h"
#include "ui/ui_public.h"
#include "ui/ui_shared.h"
#include "qcommon/qfiles.h"	// for STYLE_BLINK etc
#include "cg_media.h"

static int JP_GetScoreboardFont( void ) {
	return Q_clampi( FONT_SMALL, cg_newScoreboardFont.integer, FONT_NUM_FONTS );
}

static void DrawServerInformation( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	const float fontScale = 0.5f, lineHeight = 14.0f;
	const char *tmp = NULL;
	float y = 20.0f;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };

	// server name
	trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( cgs.japp.serverName, fontHandle, fontScale ) / 2.0f,
		y, cgs.japp.serverName, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;

	// map name
	tmp = va( "%s (%s)", (char *)CG_ConfigString( CS_MESSAGE ), cgs.mapname );
	trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
		&colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;

	// gametype
	tmp = BG_GetGametypeString( cgs.gametype );
	trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
		&colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;

	switch ( cgs.gametype ) {
	case GT_FFA:
		if ( cgs.timelimit && cgs.fraglimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i " S_COLOR_WHITE "frags or " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/"
				S_COLOR_YELLOW "%i minutes", cgs.fraglimit, (cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.timelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "minutes",
				(cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.fraglimit )
			tmp = va( "Until " S_COLOR_YELLOW "%i " S_COLOR_WHITE "frags", cgs.fraglimit );
		else
			tmp = "Playing forever!";

		trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
			tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
		break;

	case GT_CTF:
	case GT_CTY:
		if ( cgs.timelimit && cgs.capturelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "captures or "
				S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i minutes", max( cgs.scores1, cgs.scores2 ),
				cgs.capturelimit, (cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.timelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "minutes",
				(cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.capturelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "captures",
				max( cgs.scores1, cgs.scores2 ), cgs.capturelimit );
		}
		else
			tmp = "Playing forever!";

		trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
			tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
		y += lineHeight * 2;
		//FALL THROUGH TO GENERIC TEAM GAME INFO!

	case GT_TEAM:
		if ( cgs.scores1 == cgs.scores2 ) {
			tmp = S_COLOR_YELLOW"Teams are tied";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
				tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( S_COLOR_RED"%i "S_COLOR_WHITE"/ "S_COLOR_CYAN"%i", cgs.scores1, cgs.scores2 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
				y, tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
		}
		else if ( cgs.scores1 > cgs.scores2 ) {
			tmp = S_COLOR_RED"Red "S_COLOR_WHITE"leads";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
				y, tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( S_COLOR_RED"%i "S_COLOR_WHITE"/ "S_COLOR_CYAN"%i", cgs.scores1, cgs.scores2 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
				tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
		}
		else {
			tmp = S_COLOR_CYAN"Blue "S_COLOR_WHITE"leads";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
				tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( S_COLOR_CYAN"%i "S_COLOR_WHITE"/ "S_COLOR_RED"%i", cgs.scores2, cgs.scores1 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y,
				tmp, &colour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
		}
		break;

	default:
		break;

	}
}

static void DrawPlayerCount_Free( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	const float fontScale = 0.5f, width = SCREEN_WIDTH / 2.0f, lineHeight = 14.0f;
	float y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };
	int i, freeCount = 0, specCount = 0, botCount = 0;

	for ( i = 0; i < cg.numScores; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == TEAM_FREE )
			freeCount++;
		else if ( ci->team == TEAM_SPECTATOR )
			specCount++;
		if ( ci->botSkill != -1 )
			botCount++;
	}

	// player count
	if ( botCount )
		tmp = va( "%i players / %i bots", freeCount - botCount, botCount );
	else
		tmp = va( "%i players", freeCount );
	trap->R_Font_DrawString( width - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp, &colour,
		fontHandle, -1, fontScale );

	// spectator count
	tmp = va( "%2i spectators", specCount );
	trap->R_Font_DrawString( width - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y + lineHeight * 20,
		tmp, &colour, fontHandle, -1, fontScale );
}

void DrawPlayerCount_Team( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	const float fontScale = 0.5f, width = SCREEN_WIDTH / 2.0f, lineHeight = 14.0f;
	float y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };
	int i, redCount = 0, blueCount = 0, specCount = 0, pingAccumRed = 0, pingAvgRed = 0, pingAccumBlue = 0, pingAvgBlue = 0;

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_RED ) {
			pingAccumRed += cg.scores[i].ping;
			redCount++;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_BLUE ) {
			blueCount++;
			pingAccumBlue += cg.scores[i].ping;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR )
			specCount++;
	}

	if ( redCount )
		pingAvgRed = ceilf( pingAccumRed / redCount );
	if ( blueCount )
		pingAvgBlue = ceilf( pingAccumBlue / blueCount );

	if ( cgs.scores1 >= cgs.scores2 ) {
		// red team
		tmp = va( S_COLOR_RED "%i players " S_COLOR_WHITE "(%3i avg ping)", redCount, pingAvgRed );
		trap->R_Font_DrawString( width / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
			&colour, fontHandle, -1, fontScale );
		// blue team
		tmp = va( S_COLOR_CYAN "%i players " S_COLOR_WHITE "(%3i avg ping)", blueCount, pingAvgBlue );
		trap->R_Font_DrawString( width + width / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
			&colour, fontHandle, -1, fontScale );
	}
	else {
		tmp = va( S_COLOR_CYAN "%i players " S_COLOR_WHITE "(%i avg ping)", blueCount, pingAvgBlue );
		trap->R_Font_DrawString( width / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
			&colour, fontHandle, -1, fontScale );
		tmp = va( S_COLOR_RED "%i players " S_COLOR_WHITE "(%i avg ping)", redCount, pingAvgRed );
		trap->R_Font_DrawString( width + width / 2.0f - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y, tmp,
			&colour, fontHandle, -1, fontScale );
	}

	tmp = va( "%2i spectators", specCount );
	trap->R_Font_DrawString( width - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f, y + lineHeight * 20,
		tmp, &colour, fontHandle, -1, fontScale );
}

static void DrawPlayerCount( float fade ) {
	switch ( cgs.gametype ) {
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_DUEL:
	case GT_POWERDUEL:
	case GT_SINGLE_PLAYER:
		DrawPlayerCount_Free( fade );
		break;

	case GT_TEAM:
	case GT_SIEGE:
	case GT_CTF:
	case GT_CTY:
		DrawPlayerCount_Team( fade );
		break;
	default:
		break;
	}
}

// number of players on team 'team'
static int PlayerCount( team_t team ) {
	int i, count = 0;

	for ( i = 0; i < cg.numScores; i++ ) {
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == team )
			count++;
	}

	return count;
}

// number of players on team TEAM_FREE
static int ListPlayers_FFA( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, int startIndex,
	int playerCount ) {
	const char *tmp = NULL;
	const vector4 white = { 1.0f, 1.0f, 1.0f, fade },
		background = { 0.75f, 0.75f, 0.75f, 0.6f*fade },
		blue = { 0.6f, 0.6f, 1.0f, fade };
	int i, count = playerCount, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;

	if ( !count )
		return 0;

	trap->R_SetColor( &background );
	CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight*1.5f, media.gfx.interface.scoreboardLine );
	trap->R_SetColor( NULL );

	tmp = "Name";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background
	for ( i = startIndex; i < startIndex + playerCount; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE ) {
			// background
			if ( ci - cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( &background );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight, media.gfx.interface.scoreboardLine );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i = startIndex; i < startIndex + playerCount; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE ) {
			vector4	pingColour = { 1.0f, 1.0f, 1.0f, fade };
			const vector4 pingGood = { 0.0f, 1.0f, 0.0f, fade }, pingBad = { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags & EF_CONNECTION) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.connection );
				trap->R_SetColor( NULL );
			}

			else if ( cg.snap->ps.duelInProgress && (ci - cgs.clientinfo == cg.snap->ps.duelIndex
				|| ci - cgs.clientinfo == cg.snap->ps.clientNum) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.powerduelAlly );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			//Name
			tmp = ci->name;
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			//Score
			if ( score->ping == -1 )
				tmp = "Connecting";
			//	else if ( team == TEAM_SPECTATOR )
			//		tmp = "Spectating"; //TODO: Name of client they're spectating? possible?
			else
				tmp = va( "%02i/%02i", score->score, score->deaths );

			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
						fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}
				else {//Ping
					tmp = va( "%i", score->ping );
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
						fontScale ) / 2.0f) - 1.0f, tmp, &pingColour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}

				//Time
				tmp = va( "%i", score->time );
				trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
					fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

// returns number of players on team 'team'
static int ListPlayers_TDM( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team ) {
	const char *tmp = NULL;
	const vector4	white = { 1.0f, 1.0f, 1.0f, fade },
		blue = { 0.6f, 0.6f, 1.0f, fade },
		background = { 0.75f, 0.75f, 0.75f, 0.6f*fade },
		teamRed = { 0.7f, 0.4f, 0.4f, 0.6f*fade },
		teamBlue = { 0.4f, 0.4f, 0.7f, 0.6f*fade };
	const vector4 *teamBackground = &background;
	int i, count = 0, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f,
		/*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;

	if ( team == TEAM_RED )
		teamBackground = &teamRed;
	else if ( team == TEAM_BLUE )
		teamBackground = &teamBlue;

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap->R_SetColor( teamBackground );
	CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight*1.5f, media.gfx.interface.scoreboardLine );
	trap->R_SetColor( NULL );

	tmp = "Name";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Cap";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Def";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Asst";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background
	for ( i = 0; i < cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			// background
			if ( ci - cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight, media.gfx.interface.scoreboardLine );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i = 0; i < cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			vector4	pingColour = { 1.0f, 1.0f, 1.0f, fade };
			const vector4 pingGood = { 0.0f, 1.0f, 0.0f, fade }, pingBad = { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags & EF_CONNECTION) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.connection );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			// Name
			tmp = ci->name;
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Score
			tmp = va( "%4i", score->score );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Capture
			tmp = va( "%2i", score->captures );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Defend
			tmp = va( "%2i", score->defendCount );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Assist
			tmp = va( "%2i", score->assistCount );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
						tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}
				else {
					// Ping
					tmp = va( "%i", score->ping );
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
						tmp, &pingColour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}

				// Time
				tmp = va( "%i", score->time );
				trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
					fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
					tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

// returns number of players on team 'team'
static int ListPlayers_CTF( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team ) {
	const char *tmp = NULL;
	const vector4	white = { 1.0f, 1.0f, 1.0f, fade },
		blue = { 0.6f, 0.6f, 1.0f, fade },
		background = { 0.75f, 0.75f, 0.75f, 0.6f*fade },
		teamRed = { 0.7f, 0.4f, 0.4f, 0.6f*fade },
		teamBlue = { 0.4f, 0.4f, 0.7f, 0.6f*fade };
	const vector4	*teamBackground = &background;
	int i, count = 0, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f,
		/*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;

	if ( team == TEAM_RED )
		teamBackground = &teamRed;
	else if ( team == TEAM_BLUE )
		teamBackground = &teamBlue;

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap->R_SetColor( teamBackground );
	CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight*1.5f, media.gfx.interface.scoreboardLine );
	trap->R_SetColor( NULL );

	tmp = "Name";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Cap";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Def";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Asst";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle, fontScale ) / 2.0f,
		y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f, tmp, &white,
		fontHandle | STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background
	for ( i = 0; i < cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			// background
			if ( ci - cgs.clientinfo == cg.snap->ps.clientNum )
				trap->R_SetColor( &blue );
			else
				trap->R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight, media.gfx.interface.scoreboardLine );
			trap->R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i = 0; i < cg.numScores; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team ) {
			vector4 pingColour = { 1.0f, 1.0f, 1.0f, fade };
			const vector4  pingGood = { 0.0f, 1.0f, 0.0f, fade }, pingBad = { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( ci->powerups & ((1 << PW_BLUEFLAG) | (1 << PW_REDFLAG)) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y, lineHeight, lineHeight, media.gfx.interface.team.flags[FLAG_TAKEN] );
				trap->R_SetColor( NULL );
			}

			else if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags & EF_CONNECTION) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.connection );
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				trap->R_Font_DrawString( x + 8.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			// Name
			tmp = ci->name;
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Score
			tmp = va( "%4i", score->score );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Capture
			tmp = va( "%2i", score->captures );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Defend
			tmp = va( "%2i", score->defendCount );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			// Assist
			tmp = va( "%2i", score->assistCount );
			trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
				fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle, fontScale ) / 2.0f) - 1.0f,
				tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
						fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}
				else {
					// Ping
					tmp = va( "%i", score->ping );
					trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
						fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
						fontScale ) / 2.0f) - 1.0f, tmp, &pingColour, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
				}

				// Time
				tmp = va( "%i", score->time );
				trap->R_Font_DrawString( x + columnOffset[column++] - trap->R_Font_StrLenPixels( tmp, fontHandle,
					fontScale ) / 2.0f, y + (lineHeight / 2.0f) - (trap->R_Font_HeightPixels( fontHandle,
					fontScale ) / 2.0f) - 1.0f, tmp, &white, fontHandle | STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

// render a list of players on team 'team' at 'x', 'y' using relevant information based on gametype
static int ListPlayers_Free( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight,
	int startIndex, int playerCount ) {
	switch ( cgs.gametype ) {
	case GT_FFA:
		return ListPlayers_FFA( fade, x, y, fontScale, fontHandle, lineHeight, startIndex, playerCount );

	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_DUEL:
	case GT_POWERDUEL:
	case GT_SINGLE_PLAYER:
		break;

	case GT_TEAM:
	case GT_SIEGE:
		break;

	case GT_CTF:
	case GT_CTY:
		trap->Error( ERR_DROP, "Tried to use non-team scoreboard on team gametype" );
		break;

	default:
		break;
	}

	return -1;
}

// render a list of players on team 'team' at 'x', 'y' using relevant information based on gametype
static int ListPlayers_Team( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team ) {
	switch ( cgs.gametype ) {
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_DUEL:
	case GT_POWERDUEL:
	case GT_SINGLE_PLAYER:
		trap->Error( ERR_DROP, "Tried to use team scoreboard on non-team gametype" );
		break;

	case GT_TEAM:
		return ListPlayers_TDM( fade, x, y, fontScale, fontHandle, lineHeight, team );
		break;

	case GT_SIEGE:
		break;

	case GT_CTF:
	case GT_CTY:
		return ListPlayers_CTF( fade, x, y, fontScale, fontHandle, lineHeight, team );
		break;

	default:
		break;
	}

	return -1;
}

static void DrawSpectators( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	const float fontScale = 0.5f, lineHeight = 14.0f;
	float y = 128.0f;
	const vector4 white = { 1.0f, 1.0f, 1.0f, fade };

	CG_BuildSpectatorString();
	cg.scoreboard.spectatorWidth = CG_Text_Width( cg.scoreboard.spectatorList, fontScale, fontHandle );

	if ( cg.scoreboard.spectatorLen ) {
		const float dt = (cg.time - cg.scoreboard.spectatorResetTime)*0.0625f;
		cg.scoreboard.spectatorX = SCREEN_WIDTH - (1.0f)*dt;

		if ( cg.scoreboard.spectatorX < 0 - cg.scoreboard.spectatorWidth ) {
			cg.scoreboard.spectatorX = SCREEN_WIDTH;
			cg.scoreboard.spectatorResetTime = cg.time;
		}
		CG_Text_Paint( cg.scoreboard.spectatorX, (y + lineHeight * 20) - 3, fontScale, &white, cg.scoreboard.spectatorList,
			0, 0, ITEM_TEXTSTYLE_SHADOWED, fontHandle );
	}
}

static void DrawPlayers_Free( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	const float fontScale = 0.45f, lineHeight = 14.0f;
	float x = 0, y = 128.0f;
	const int playerCount = PlayerCount( TEAM_FREE );

	// dirty hack, 7 players means 4 on left, so +1 then /2 (8/2==4) will give a good number (4 left, 3 right)
	//	similarly, 8 players means 4 on left, so +1 then /2 (9/2==4) will give a good number (4 left, 4 right)
	ListPlayers_Free( fade, x, y, fontScale, fontHandle, lineHeight, 0, (playerCount + 1) / 2 );
	ListPlayers_Free( fade, x + SCREEN_WIDTH / 2.0f, y, fontScale, fontHandle, lineHeight, (playerCount + 1) / 2, playerCount / 2 );

	DrawSpectators( fade );
}

static void DrawPlayers_Team( float fade ) {
	const qhandle_t fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	const float fontScale = 0.5f, lineHeight = 14.0f;
	float x = 0, y = 128.0f;

	if ( cgs.scores1 >= cgs.scores2 ) {
		ListPlayers_Team( fade, x, y, fontScale, fontHandle, lineHeight, TEAM_RED );
		ListPlayers_Team( fade, x + SCREEN_WIDTH / 2.0f, y, fontScale, fontHandle, lineHeight, TEAM_BLUE );
	}
	else {
		ListPlayers_Team( fade, x, y, fontScale, fontHandle, lineHeight, TEAM_BLUE );
		ListPlayers_Team( fade, x + SCREEN_WIDTH / 2.0f, y, fontScale, fontHandle, lineHeight, TEAM_RED );
	}

	DrawSpectators( fade );
}

// controller for drawing the 'players' section. will call the relevant functions based on gametype
static void DrawPlayers( float fade ) {
	switch ( cgs.gametype ) {
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_DUEL:
	case GT_POWERDUEL:
	case GT_SINGLE_PLAYER:
		DrawPlayers_Free( fade );
		break;

	case GT_TEAM:
	case GT_SIEGE:
	case GT_CTF:
	case GT_CTY:
		DrawPlayers_Team( fade );
		break;

	default:
		break;
	}
}

//Scoreboard entry point
//	This will be called even if the scoreboard is not showing - and will return false if it has faded aka 'not showing'
//	It will return true if the scoreboard is showing
qboolean CG_DrawQ3PScoreboard( void ) {
	vector4 fadeWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fade = 1.0f;

	if ( cg.warmup && !cg.showScores )
		return qfalse;

	if ( !cg.showScores && cg.snap->ps.pm_type != PM_DEAD && cg.snap->ps.pm_type != PM_INTERMISSION ) {
		if ( CG_FadeColor2( &fadeWhite, cg.scoreFadeTime, 300.0f ) )
			return qfalse;

		fade = fadeWhite.a;
	}

	DrawServerInformation( fade );

	DrawPlayerCount( fade );
	DrawPlayers( fade );

	CG_LoadDeferredPlayers();
	return qtrue;
}
