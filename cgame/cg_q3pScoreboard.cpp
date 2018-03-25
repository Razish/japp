#include "cg_local.h"
#include "ui/ui_public.h"
#include "ui/ui_shared.h"
#include "qcommon/qfiles.h"	// for STYLE_BLINK etc
#include "cg_media.h"
#include "ui/ui_fonts.h"

static int JP_GetScoreboardFont( void ) {
	return Q_clampi( FONT_SMALL, cg_newScoreboardFont.integer, FONT_NUM_FONTS );
}

static void DrawServerInfo( float fade ) {
	const Font fontLarge( FONT_JAPPLARGE, 0.5f, false );
	const Font fontSmall( FONT_JAPPMONO, 0.5f, false );
	const float lineHeightBig = fontLarge.Height();
	const float lineHeightSmall = fontSmall.Height();
	const char *tmp = NULL;
	float y = SCREEN_HEIGHT - lineHeightSmall - 4.0f;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	colour.a = fade;

	// map name
	tmp = va( "%s (%s)", (char *)CG_ConfigString( CS_MESSAGE ), cgs.mapname );
	fontSmall.Paint( 0.0f, y, tmp, &colour, uiTextStyle_e::Shadowed );
	y -= lineHeightSmall;

	// server name
	fontSmall.Paint( 0.0f, y, cgs.japp.serverName, &colour, uiTextStyle_e::Shadowed );
	y -= lineHeightSmall;

	y = 42.0f;

	// gametype
	tmp = BG_GetGametypeString( cgs.gametype );
	fontLarge.Paint( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::Shadowed );
	y += lineHeightBig;

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

		trap->R_Font_DrawString(
			SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y, tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW,
			-1, fontLarge.scale
		);
		break;

	case GT_CTF:
	case GT_CTY:
		if ( cgs.timelimit && cgs.capturelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "captures or "
				S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i minutes", std::max( cgs.scores1, cgs.scores2 ),
				cgs.capturelimit, (cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.timelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "minutes",
				(cg.time - cgs.levelStartTime) / 60000, cgs.timelimit );
		}
		else if ( cgs.capturelimit ) {
			tmp = va( "Until " S_COLOR_YELLOW "%i" S_COLOR_WHITE "/" S_COLOR_YELLOW "%i " S_COLOR_WHITE "captures",
				std::max( cgs.scores1, cgs.scores2 ), cgs.capturelimit );
		}
		else
			tmp = "Playing forever!";

		fontLarge.Paint( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ), y, tmp, &colour, uiTextStyle_e::Shadowed );
		y += lineHeightBig * 2;
		//FALL THROUGH TO GENERIC TEAM GAME INFO!

	case GT_TEAM:
		if ( cgs.scores1 == cgs.scores2 ) {
			tmp = S_COLOR_YELLOW"Teams are tied";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y,
				tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
			y += lineHeightBig;
			tmp = va( S_COLOR_RED "%i " S_COLOR_WHITE "/ " S_COLOR_CYAN "%i", cgs.scores1, cgs.scores2 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f,
				y, tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
		}
		else if ( cgs.scores1 > cgs.scores2 ) {
			tmp = S_COLOR_RED "Red " S_COLOR_WHITE "leads";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f,
				y, tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
			y += lineHeightBig;
			tmp = va( S_COLOR_RED "%i " S_COLOR_WHITE "/ " S_COLOR_CYAN "%i", cgs.scores1, cgs.scores2 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y,
				tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
		}
		else {
			tmp = S_COLOR_CYAN "Blue " S_COLOR_WHITE "leads";
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y,
				tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
			y += lineHeightBig;
			tmp = va( S_COLOR_CYAN "%i " S_COLOR_WHITE "/ " S_COLOR_RED "%i", cgs.scores2, cgs.scores1 );
			trap->R_Font_DrawString( SCREEN_WIDTH / 2.0f - fontLarge.Width( tmp ) / 2.0f, y,
				tmp, &colour, fontLarge.handle | STYLE_DROPSHADOW, -1, fontLarge.scale );
		}
		//TODO: playing until x/y
		break;

	default:
		break;
	}
}

static void DrawPlayerCount_Free( float fade ) {
	const Font font( FONT_JAPPLARGE, 0.5f, false );
	const float width = SCREEN_WIDTH / 2.0f, lineHeight = font.Height();
	float y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	int i, freeCount = 0, specCount = 0, botCount = 0;

	colour.a = fade;

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
	if ( botCount ) {
		tmp = va( "%i players / %i bots", freeCount - botCount, botCount );
	}
	else {
		tmp = va( "%i players", freeCount );
	}
	font.Paint( width - font.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::ShadowedMore );

	// spectator count
	tmp = va( "%2i spectators", specCount );
	font.Paint( width - font.Width( tmp ) / 2.0f, y + lineHeight * 20, tmp, &colour, uiTextStyle_e::ShadowedMore );
}

static void DrawPlayerCount_Team( float fade ) {
	const Font font( FONT_JAPPLARGE, 0.5f, false );
	const float width = SCREEN_WIDTH / 2.0f, lineHeight = 14.0f;
	float y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	int i, redCount = 0, blueCount = 0, specCount = 0, pingAccumRed = 0, pingAvgRed = 0, pingAccumBlue = 0, pingAvgBlue = 0;
	colour.a = fade;

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_RED ) {
			pingAccumRed += cg.scores[i].ping;
			redCount++;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_BLUE ) {
			blueCount++;
			pingAccumBlue += cg.scores[i].ping;
		}
		else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR ) {
			specCount++;
		}
	}

	if ( redCount ) {
		pingAvgRed = ceilf( pingAccumRed / redCount );
	}
	if ( blueCount ) {
		pingAvgBlue = ceilf( pingAccumBlue / blueCount );
	}

	if ( cgs.scores1 >= cgs.scores2 ) {
		// red team
		tmp = va( S_COLOR_RED "%i players " S_COLOR_WHITE "(%3i avg ping)", redCount, pingAvgRed );
		font.Paint( width / 2.0f - font.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::ShadowedMore );
		// blue team
		tmp = va( S_COLOR_CYAN "%i players " S_COLOR_WHITE "(%3i avg ping)", blueCount, pingAvgBlue );
		font.Paint( width + width / 2.0f - font.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::ShadowedMore );
	}
	else {
		tmp = va( S_COLOR_CYAN "%i players " S_COLOR_WHITE "(%i avg ping)", blueCount, pingAvgBlue );
		font.Paint( width / 2.0f - font.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::ShadowedMore );
		tmp = va( S_COLOR_RED "%i players " S_COLOR_WHITE "(%i avg ping)", redCount, pingAvgRed );
		font.Paint( width + width / 2.0f - font.Width( tmp ) / 2.0f, y, tmp, &colour, uiTextStyle_e::ShadowedMore );
	}

	tmp = va( "%2i spectators", specCount );
	trap->R_Font_DrawString( width - trap->R_Font_StrLenPixels( tmp, font.handle, font.scale ) / 2.0f, y + lineHeight * 20,
		tmp, &colour, font.handle, -1, font.scale );
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
static int ListPlayers_FFA( float fade, float x, float y, float fontScale, int fontHandle, int startIndex,
	int playerCount ) {
	const char *tmp = NULL;
	vector4 white = { 1.0f, 1.0f, 1.0f, 1.0f },
		background = { 0.75f, 0.75f, 0.75f, 1.0f },
		blue = { 0.6f, 0.6f, 1.0f, 1.0f };
	int i, count = playerCount, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;
	const Font font( fontHandle, fontScale, false );
	const float lineHeight = font.Height();

	white.a = fade;
	background.a = 0.6f * fade;
	blue.a = fade;

	if ( !count )
		return 0;

	trap->R_SetColor( &background );
	CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f * 2 + 5.0f, lineHeight*1.25f, media.gfx.interface.scoreboardLine );
	trap->R_SetColor( NULL );

	tmp = "Name";
	font.Paint(
		x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
		uiTextStyle_e::ShadowedMore
	);

	tmp = "Score";
	font.Paint(
		x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
		uiTextStyle_e::ShadowedMore
	);

	tmp = "Ping";
	font.Paint(
		x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
		uiTextStyle_e::ShadowedMore
	);

	tmp = "Time";
	font.Paint(
		x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
		uiTextStyle_e::ShadowedMore
	);
	y += lineHeight * 1.25f;

	savedY = y;

	// First pass, background
	for ( i = startIndex; i < startIndex + playerCount; i++ ) {
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE ) {
			// background
			if ( ci - cgs.clientinfo == cg.snap->ps.clientNum ) {
				trap->R_SetColor( &blue );
			}
			else {
				trap->R_SetColor( &background );
			}

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
			vector4	pingColour = { 1.0f, 1.0f, 1.0f, 1.0f },
				pingGood = { 0.0f, 1.0f, 0.0f, 1.0f },
				pingBad = { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;

			Q_LerpColour( &pingGood, &pingBad, &pingColour, std::min( score->ping / 300.0f, 1.0f ) );

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
				font.Paint(
					x + 8.0f, y, tmp, &white, uiTextStyle_e::ShadowedMore
				);
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				font.Paint( x + 8.0f, y, tmp, &white, uiTextStyle_e::ShadowedMore );
			}

			//Name
			tmp = ci->name;
			font.Paint(
				x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
				uiTextStyle_e::ShadowedMore
			);

			//Score
			if ( score->ping == -1 ) {
				tmp = "Connecting";
			}
			/*
			else if ( team == TEAM_SPECTATOR ) {
				tmp = "Spectating"; //TODO: Name of client they're spectating? possible?
			}
			*/
			else if ( cg_drawScoresNet.integer ) {
				const int net = score->score - score->deaths;
				tmp = va( "%02i/%02i (%c%i)", score->score, score->deaths, (net >= 0) ? '+' : '-', abs( net ) );
			}
			else {
				tmp = va( "%02i/%02i", score->score, score->deaths );
			}

			font.Paint(
				x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp, &white,
				uiTextStyle_e::ShadowedMore
			);

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					font.Paint(
						x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp,
						&white, uiTextStyle_e::ShadowedMore
					);
				}
				else {//Ping
					tmp = va( "%i", score->ping );
					font.Paint(
						x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp,
						&pingColour, uiTextStyle_e::ShadowedMore
					);
				}

				//Time
				tmp = va( "%i", score->time );
				font.Paint(
					x - (font.Width( tmp ) / 2.0f) + columnOffset[column++], y, tmp,
					&white, uiTextStyle_e::ShadowedMore
				);
			}

			y += lineHeight;
		}
	}

	return count;
}

// returns number of players on team 'team'
static int ListPlayers_TDM( float fade, float x, float y, float fontScale, int fontHandle, team_t team ) {
	const char *tmp = NULL;
	vector4	white = { 1.0f, 1.0f, 1.0f, 1.0f },
		blue = { 0.6f, 0.6f, 1.0f, 1.0f },
		background = { 0.75f, 0.75f, 0.75f, 1.0f },
		teamRed = { 0.7f, 0.4f, 0.4f, 1.0f },
		teamBlue = { 0.4f, 0.4f, 0.7f, 1.0f };
	const vector4 *teamBackground = &background;
	int i, count = 0, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*net*/220.0f,
		/*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;
	white.a = blue.a = fade;
	background.a = teamRed.a = teamBlue.a = 0.6f * fade;
	const Font font( fontHandle, fontScale, false );
	const float lineHeight = font.Height();

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
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Score";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Net";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Ping";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Time";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
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
			int tmpI;
			vector4	pingColour = { 1.0f, 1.0f, 1.0f, 1.0f }, pingGood = { 0.0f, 1.0f, 0.0f, 1.0f },
				pingBad = { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;
			Q_LerpColour( &pingGood, &pingBad, &pingColour, std::min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags & EF_CONNECTION) ) {
				trap->R_SetColor( &white );
				CG_DrawPic( x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.connection );
				trap->R_SetColor( NULL );
			}

			else if ( cg.snap->ps.duelInProgress && (ci - cgs.clientinfo == cg.snap->ps.duelIndex
				|| ci - cgs.clientinfo == cg.snap->ps.clientNum) ) {
				trap->R_SetColor( &white );
				CG_DrawPic(
					x + 5.0f, y + 1.0f, lineHeight - 2.0f, lineHeight - 2.0f, media.gfx.interface.powerduelAlly
				);
				trap->R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 ) {
				tmp = "BOT";
				font.Paint( x + 8.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				font.Paint( x + 8.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
			}

			// Name
			tmp = ci->name;
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Score
			tmp = va( "%02i/%02i", score->score, score->deaths );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Net
			tmpI = score->score - score->deaths;
			tmp = va( "%c%i", (tmpI >= 0) ? '+' : '-', abs( tmpI ) );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					font.Paint(
						x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
					);
				}
				else {
					// Ping
					tmp = va( "%i", score->ping );
					font.Paint(
						x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &pingColour,
						uiTextStyle_e::Shadowed
					);
				}

				// Time
				tmp = va( "%i", score->time );
				font.Paint(
					x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
				);
			}

			y += lineHeight;
		}
	}

	return count;
}

// returns number of players on team 'team'
static int ListPlayers_CTF( float fade, float x, float y, float fontScale, int fontHandle, team_t team ) {
	const char *tmp = NULL;
	vector4 white = { 1.0f, 1.0f, 1.0f, 1.0f },
		blue = { 0.6f, 0.6f, 1.0f, 1.0f },
		background = { 0.75f, 0.75f, 0.75f, 1.0f },
		teamRed = { 0.7f, 0.4f, 0.4f, 1.0f },
		teamBlue = { 0.4f, 0.4f, 0.7f, 1.0f };
	const vector4	*teamBackground = &background;
	int i, count = 0, column = 0;
	const float endX = SCREEN_WIDTH / 2.0f, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f,
		/*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f };
	float savedY = 0.0f;
	white.a = blue.a = fade;
	background.a = teamRed.a = teamBlue.a = 0.6f * fade;
	const Font font( fontHandle, fontScale, false );
	const float lineHeight = font.Height();

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
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Score";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Cap";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Def";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Asst";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Ping";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );

	tmp = "Time";
	font.Paint( x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
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
			vector4 pingColour = { 1.0f, 1.0f, 1.0f, 1.0f }, pingGood = { 0.0f, 1.0f, 0.0f, 1.0f },
				pingBad = { 1.0f, 0.0f, 0.0f, 1.0f };
			pingColour.a = pingGood.a = pingBad.a = fade;
			Q_LerpColour( &pingGood, &pingBad, &pingColour, std::min( score->ping / 300.0f, 1.0f ) );

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
				font.Paint( x + 8.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) ) {
				tmp = "READY";
				font.Paint( x + 8.0f, y, tmp, &white, uiTextStyle_e::Shadowed );
			}

			// Name
			tmp = ci->name;
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Score
			tmp = va( "%4i", score->score );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Capture
			tmp = va( "%2i", score->captures );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Defend
			tmp = va( "%2i", score->defendCount );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			// Assist
			tmp = va( "%2i", score->assistCount );
			font.Paint(
				x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
			);

			if ( score->ping != -1 ) {
				if ( ci->botSkill != -1 ) {
					tmp = "--";
					font.Paint(
						x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
					);
				}
				else {
					// Ping
					tmp = va( "%i", score->ping );
					font.Paint(
						x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &pingColour,
						uiTextStyle_e::Shadowed
					);
				}

				// Time
				tmp = va( "%i", score->time );
				font.Paint(
					x + columnOffset[column++] - font.Width( tmp ) / 2.0f, y, tmp, &white, uiTextStyle_e::Shadowed
				);
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
		return ListPlayers_FFA( fade, x, y, fontScale, fontHandle, startIndex, playerCount );

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
		return ListPlayers_TDM( fade, x, y, fontScale, fontHandle, team );

	case GT_SIEGE:
		break;

	case GT_CTF:
	case GT_CTY:
		return ListPlayers_CTF( fade, x, y, fontScale, fontHandle, team );

	default:
		break;
	}

	return -1;
}

static void DrawSpectators( float fade ) {
	const Font font( JP_GetScoreboardFont(), 0.5f, false );
	const float lineHeight = font.Height();;
	float y = 128.0f;
	vector4 white = { 1.0f, 1.0f, 1.0f, 1.0f };
	white.a = fade;

	CG_BuildSpectatorString();
	cg.scoreboard.spectatorWidth = font.Width( cg.scoreboard.spectatorList );

	if ( cg.scoreboard.spectatorLen ) {
		const float dt = (cg.time - cg.scoreboard.spectatorResetTime)*0.0625f;
		cg.scoreboard.spectatorX = SCREEN_WIDTH - (1.0f)*dt;

		if ( cg.scoreboard.spectatorX < 0 - cg.scoreboard.spectatorWidth ) {
			cg.scoreboard.spectatorX = SCREEN_WIDTH;
			cg.scoreboard.spectatorResetTime = cg.time;
		}
		font.Paint(
			cg.scoreboard.spectatorX, (y + lineHeight * 20) - 3, cg.scoreboard.spectatorList, &white,
			uiTextStyle_e::Shadowed
		);
	}
}

static void DrawPlayers_Free( float fade ) {
	const int fontHandle = JP_GetScoreboardFont();
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
	const Font font( JP_GetScoreboardFont(), 0.5f );
	const float lineHeight = font.Height();
	float x = 0, y = 128.0f;

	if ( cgs.scores1 >= cgs.scores2 ) {
		ListPlayers_Team( fade, x, y, font.scale, font.handle, lineHeight, TEAM_RED );
		ListPlayers_Team( fade, x + SCREEN_WIDTH / 2.0f, y, font.scale, font.handle, lineHeight, TEAM_BLUE );
	}
	else {
		ListPlayers_Team( fade, x, y, font.scale, font.handle, lineHeight, TEAM_BLUE );
		ListPlayers_Team( fade, x + SCREEN_WIDTH / 2.0f, y, font.scale, font.handle, lineHeight, TEAM_RED );
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

static const char *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec" };

// e.g. #st, #nd, #rd, #th
static const char *GetDateSuffix( int date ) {
	switch ( date % 10 ) {
	case 1:
		return "st";
	case 2:
		return "nd";
	case 3:
		return "rd";
	default:
		return "th";
	}
}

// shows current date and JA++ version
static void DrawClientInfo( float fade ) {
	struct tm *timeinfo;
	time_t tm;
	char buf[256];
	const Font font( FONT_JAPPMONO, 0.5f, false );
	vector4 colour;
	float y = SCREEN_HEIGHT - 4.0f;

	VectorCopy4( &g_color_table[ColorIndex( COLOR_ORANGE )], &colour );
	colour.a = fade;

#ifdef REVISION
	y -= font.Height( REVISION );
	// JA++ version
	font.Paint( SCREEN_WIDTH - font.Width( REVISION ), y, REVISION, &colour, uiTextStyle_e::ShadowedMore );
#endif

	// date
	time( &tm );
	timeinfo = localtime( &tm );

	Com_sprintf( buf, sizeof(buf), "%s %i%s %04i, %02i:%02i:%02i",
		months[timeinfo->tm_mon], timeinfo->tm_mday, GetDateSuffix( timeinfo->tm_mday ),
		1900 + timeinfo->tm_year, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec
	);

	y -= font.Height( buf );
	font.Paint( SCREEN_WIDTH - font.Width( buf ), y, buf, &colour, uiTextStyle_e::ShadowedMore );
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

	DrawServerInfo( fade );

	DrawPlayerCount( fade );
	DrawPlayers( fade );

	DrawClientInfo( fade );

	CG_LoadDeferredPlayers();
	return qtrue;
}
