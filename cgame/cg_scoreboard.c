// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"
#include "ui/ui_shared.h"
#include "bg_saga.h"

#ifdef _WIN32
#undef SB_TOP //Raz: FFS windows.h >_>
#endif

#define	SCOREBOARD_X		(0)

#define SB_HEADER			20
#define SB_TOP				(SB_HEADER+72)

// Where the status bar starts, so we don't overwrite it
#define SB_STATUSBAR		420

#define SB_NORMAL_HEIGHT	25
#define SB_INTER_HEIGHT		15 // interleaved height

#define SB_MAXCLIENTS_NORMAL  ((SB_STATUSBAR - SB_TOP) / SB_NORMAL_HEIGHT)
#define SB_MAXCLIENTS_INTER   ((SB_STATUSBAR - SB_TOP) / SB_INTER_HEIGHT - 1)

// Used when interleaved



#define SB_LEFT_BOTICON_X	(SCOREBOARD_X+0)
#define SB_LEFT_HEAD_X		(SCOREBOARD_X+32)
#define SB_RIGHT_BOTICON_X	(SCOREBOARD_X+64)
#define SB_RIGHT_HEAD_X		(SCOREBOARD_X+96)
// Normal
#define SB_BOTICON_X		(SCOREBOARD_X+32)
#define SB_HEAD_X			(SCOREBOARD_X+64)

#define SB_SCORELINE_X		72
#define SB_SCORELINE_WIDTH	(SCREEN_WIDTH - SB_SCORELINE_X * 2)

#define SB_RATING_WIDTH	    0 // (6 * BIGCHAR_WIDTH)
#define SB_NAME_X			(SB_SCORELINE_X)
#define SB_SCORE_X			(SB_SCORELINE_X + .40f * SB_SCORELINE_WIDTH)
#define SB_PING_X			(SB_SCORELINE_X + .60f * SB_SCORELINE_WIDTH)
#define SB_TIME_X			(SB_SCORELINE_X + .85f * SB_SCORELINE_WIDTH)

// The new and improved score board
//
// In cases where the number of clients is high, the score board heads are interleaved
// here's the layout

//
//	0   32   80  112  144   240  320  400   <-- pixel position
//  bot head bot head score ping time name
//
//  wins/losses are drawn on bot icon now

static qboolean localClient; // true if local client has been displayed

static void CG_DrawClientScore( int y, score_t *score, const vector4 *color, float fade, qboolean largeFormat ) {
	//vector3	headAngles;
	clientInfo_t	*ci;
	int				iconx = SB_SCORELINE_X - 5;//SB_BOTICON_X + (SB_RATING_WIDTH / 2);
	float			scale = largeFormat ? 1.0f : 0.75f,
		iconSize = largeFormat ? SB_NORMAL_HEIGHT : SB_INTER_HEIGHT;

	iconx -= iconSize;
	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	ci = &cgs.clientinfo[score->client];

	// draw the handicap or bot skill marker (unless player has flag)
	if ( ci->powerups & (1 << PW_NEUTRALFLAG) ) {
		if ( largeFormat )
			CG_DrawFlagModel( iconx, y - (32 - BIGCHAR_HEIGHT) / 2, iconSize, iconSize, TEAM_FREE, qfalse );
		else
			CG_DrawFlagModel( iconx, y, iconSize, iconSize, TEAM_FREE, qfalse );
	}

	else if ( ci->powerups & (1 << PW_REDFLAG) )
		CG_DrawFlagModel( iconx, y, iconSize, iconSize, TEAM_RED, qfalse );

	else if ( ci->powerups & (1 << PW_BLUEFLAG) )
		CG_DrawFlagModel( iconx, y, iconSize, iconSize, TEAM_BLUE, qfalse );

	else if ( cgs.gametype == GT_POWERDUEL && (ci->duelTeam == DUELTEAM_LONE || ci->duelTeam == DUELTEAM_DOUBLE) ) {
		CG_DrawPic( iconx, y, iconSize, iconSize, trap->R_RegisterShaderNoMip(
			(ci->duelTeam == DUELTEAM_LONE) ? "gfx/mp/pduel_icon_lone" : "gfx/mp/pduel_icon_double" ) );
	}

	else if ( cgs.gametype == GT_SIEGE ) {
		// try to draw the shader for this class on the scoreboard
		if ( ci->siegeIndex != -1 ) {
			siegeClass_t *scl = &bgSiegeClasses[ci->siegeIndex];

			if ( scl->classShader )
				CG_DrawPic( iconx, y, largeFormat ? 24 : 12, largeFormat ? 24 : 12, scl->classShader );
		}
	}

	else if ( ci->modelIcon && cg_scoreboardSkinIcons.integer )
		CG_DrawPic( iconx, y, iconSize, iconSize, ci->modelIcon );


	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		vector4 hcolor;
		int		rank;

		localClient = qtrue;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cgs.gametype >= GT_TEAM )
			rank = -1;
		else
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		if ( rank == 0 ) {
			hcolor.r = 0;
			hcolor.g = 0;
			hcolor.b = 0.7f;
		}
		else if ( rank == 1 ) {
			hcolor.r = 0.7f;
			hcolor.g = 0;
			hcolor.b = 0;
		}
		else if ( rank == 2 ) {
			hcolor.r = 0.7f;
			hcolor.g = 0.7f;
			hcolor.b = 0;
		}
		else {
			hcolor.r = 0.7f;
			hcolor.g = 0.7f;
			hcolor.b = 0.7f;
		}

		hcolor.a = fade * 0.7f;
		CG_FillRect( SB_SCORELINE_X - 5, y /*+ 2*/, SB_SCORELINE_WIDTH /*- SB_SCORELINE_X * 2 + 10*/, largeFormat ? SB_NORMAL_HEIGHT : SB_INTER_HEIGHT, &hcolor );
	}

	CG_Text_Paint( SB_NAME_X, y, 0.9f * scale, &colorWhite, ci->name, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );

	if ( score->ping != -1 ) {
		if ( ci->team != TEAM_SPECTATOR || cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) {
			if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL )
				CG_Text_Paint( SB_SCORE_X, y, 1.0f * scale, &colorWhite, va( "%i/%i", ci->wins, ci->losses ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			else {
				if ( Server_Supports( SSF_SCOREBOARD_KD ) )
					CG_Text_Paint( SB_SCORE_X, y, 1.0f * scale, &colorWhite, va( "%i/%i", score->score, score->deaths ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
				else
					CG_Text_Paint( SB_SCORE_X, y, 1.0f * scale, &colorWhite, va( "%i", score->score ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
		}

		CG_Text_Paint( SB_PING_X, y, 1.0f * scale, &colorWhite, va( "%i", score->ping ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint( SB_TIME_X, y, 1.0f * scale, &colorWhite, va( "%i", score->time ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}
	else {
		CG_Text_Paint( SB_SCORE_X, y, 1.0f * scale, &colorWhite, "-", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint( SB_PING_X, y, 1.0f * scale, &colorWhite, "-", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		CG_Text_Paint( SB_TIME_X, y, 1.0f * scale, &colorWhite, "-", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1 << score->client) )
		CG_Text_Paint( cg_scoreboardSkinIcons.integer ? 4 : SB_NAME_X - 48, y + 2, 0.7f * scale, &colorWhite, CG_GetStringEdString( "MP_INGAME", "READY" ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	else if ( cgs.clientinfo[score->client].botSkill != -1 )
		CG_Text_Paint( cg_scoreboardSkinIcons.integer ? 4 : SB_NAME_X - 48, y + 2, 0.7f * scale, &colorWhite, "BOT", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	else if ( score->team == TEAM_SPECTATOR )
		CG_Text_Paint( cg_scoreboardSkinIcons.integer ? 4 : SB_NAME_X - 48, y + 2, 0.7f * scale, &colorWhite, "SPEC", 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
}

static int CG_TeamScoreboard( int y, team_t team, float fade, int maxClients, int lineHeight, qboolean countOnly ) {
	int		i;
	score_t	*score;
	vector4	color;
	int		count;
	clientInfo_t	*ci;

	color.r = color.g = color.b = 1.0f;
	color.a = fade;

	count = 0;
	for ( i = 0; i < cg.numScores && count < maxClients; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];

		if ( team != ci->team )
			continue;

		if ( !countOnly )
			CG_DrawClientScore( y + lineHeight * count, score, &color, fade, lineHeight == SB_NORMAL_HEIGHT );

		count++;
	}

	return count;
}

int	CG_GetClassCount( team_t team, int siegeClass ) {
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	siegeClass_t *scl;

	for ( i = 0; i < cgs.maxclients; i++ ) {
		ci = &cgs.clientinfo[i];

		if ( !ci->infoValid || team != ci->team )
			continue;

		scl = &bgSiegeClasses[ci->siegeIndex];

		// Correct class?
		if ( siegeClass != scl->classShader )
			continue;

		count++;
	}

	return count;
}

int CG_GetTeamNonScoreCount( team_t team ) {
	int i = 0, count = 0;
	clientInfo_t	*ci;

	for ( i = 0; i < cgs.maxclients; i++ ) {
		ci = &cgs.clientinfo[i];

		if ( !ci->infoValid || (team != ci->team && team != ci->siegeDesiredTeam) )
			continue;

		count++;
	}

	return count;
}

int CG_GetTeamCount( team_t team, int maxClients ) {
	int i = 0;
	int count = 0;
	clientInfo_t	*ci;
	score_t	*score;

	for ( i = 0; i < cg.numScores && count < maxClients; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[score->client];

		if ( team != ci->team )
			continue;

		count++;
	}

	return count;

}

int cg_siegeWinTeam = 0;

// Draw the normal in-game scoreboard
qboolean CG_DrawOldScoreboard( void ) {
	int		x, y, i, n1, n2;
	float	fade;
	const vector4 *fadeColor;
	const char *s;
	int maxClients, realMaxClients;
	int lineHeight;
	int topBorderSize, bottomBorderSize;

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores )
		return qfalse;

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0f;
		fadeColor = &colorWhite;
	}
	else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = fadeColor->a;
	}

	// fragged by ... line
	// or if in intermission and duel, prints the winner of the duel round
	if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.duelWinner != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		s = va( "%s"S_COLOR_WHITE" %s", cgs.clientinfo[cgs.duelWinner].name, CG_GetStringEdString( "MP_INGAME", "DUEL_WINS" ) );
		x = (SCREEN_WIDTH) / 2;
		y = 40;
		CG_Text_Paint( x - CG_Text_Width( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL) && cgs.duelist1 != -1 && cgs.duelist2 != -1 &&
		cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		if ( cgs.gametype == GT_POWERDUEL && cgs.duelist3 != -1 ) {
			s = va( "%s"S_COLOR_WHITE" %s %s"S_COLOR_WHITE" %s %s", cgs.clientinfo[cgs.duelist1].name,
				CG_GetStringEdString( "MP_INGAME", "SPECHUD_VERSUS" ), cgs.clientinfo[cgs.duelist2].name,
				CG_GetStringEdString( "MP_INGAME", "AND" ), cgs.clientinfo[cgs.duelist3].name );
		}
		else {
			s = va( "%s"S_COLOR_WHITE" %s %s", cgs.clientinfo[cgs.duelist1].name,
				CG_GetStringEdString( "MP_INGAME", "SPECHUD_VERSUS" ), cgs.clientinfo[cgs.duelist2].name );
		}
		x = (SCREEN_WIDTH) / 2;
		y = 40;
		CG_Text_Paint( x - CG_Text_Width( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( cg.killerName[0] ) {
		s = va( "%s %s", CG_GetStringEdString( "MP_INGAME", "KILLEDBY" ), cg.killerName );
		x = (SCREEN_WIDTH) / 2;
		y = 32;
		CG_Text_Paint( x - CG_Text_Width( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else {
		x = (SCREEN_WIDTH) / 2;
		y = SB_HEADER;
		//	CG_DrawBigString( x, y, s, fade );
		s = cgs.japp.serverName;
		CG_Text_Paint( x - (CG_Text_Width( s, 0.75f, FONT_NONE ) / 2), y, 0.75f, &colorTable[CT_WHITE], s, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_NONE );

		if ( cgs.gametype >= GT_TEAM ) {
			int redCount = 0, blueCount = 0, specCount = 0;
			for ( i = 0; i < cg.numScores; i++ ) {
				if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_RED )
					redCount++;
				else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_BLUE )
					blueCount++;
				else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR )
					specCount++;
			}
			s = va( "Players: "S_COLOR_GREEN"%i"S_COLOR_WHITE"/"S_COLOR_GREEN"%i "S_COLOR_WHITE"("S_COLOR_RED"%i"
				S_COLOR_WHITE"/"S_COLOR_CYAN"%i"S_COLOR_WHITE") - %i spectators", cg.numScores, cgs.maxclients, redCount,
				blueCount, specCount );
		}
		else {
			int specCount = 0;
			for ( i = 0; i < cg.numScores; i++ ) {
				if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR )
					specCount++;
			}
			s = va( "Players: %i/%i - %i spectators", cg.numScores, cgs.maxclients, specCount );
		}
		CG_Text_Paint( x - (CG_Text_Width( s, 0.75f, FONT_NONE ) / 2), y + 15, 0.75f, &colorTable[CT_WHITE], s, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_NONE );

		s = va( "%s (%s)", (char *)CG_ConfigString( CS_MESSAGE ), cgs.mapname );
		CG_Text_Paint( x - (CG_Text_Width( s, 0.75f, FONT_NONE ) / 2), y + 30, 0.75f, &colorTable[CT_WHITE], s, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_NONE );
	}

	if ( cgs.gametype != GT_SIEGE ) {
		if ( cg.teamScores[0] == cg.teamScores[1] )
			s = va( "%s %i", CG_GetStringEdString( "MP_INGAME", "TIEDAT" ), cg.teamScores[0] );
		else if ( cg.teamScores[0] >= cg.teamScores[1] )
			s = va( "%s, %i / %i", CG_GetStringEdString( "MP_INGAME", "RED_LEADS" ), cg.teamScores[0], cg.teamScores[1] );
		else
			s = va( "%s, %i / %i", CG_GetStringEdString( "MP_INGAME", "BLUE_LEADS" ), cg.teamScores[1], cg.teamScores[0] );

		x = (SCREEN_WIDTH) / 2;
		y = SB_HEADER - 24;

		CG_Text_Paint( x - CG_Text_Width( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else if ( cgs.gametype == GT_SIEGE && (cg_siegeWinTeam == 1 || cg_siegeWinTeam == 2) ) {
		if ( cg_siegeWinTeam == 1 )
			s = va( "%s", CG_GetStringEdString( "MP_INGAME", "SIEGETEAM1WIN" ) );
		else
			s = va( "%s", CG_GetStringEdString( "MP_INGAME", "SIEGETEAM2WIN" ) );

		x = (SCREEN_WIDTH) / 2;
		y = 60;

		CG_Text_Paint( x - CG_Text_Width( s, 1.0f, FONT_MEDIUM ) / 2, y, 1.0f, &colorWhite, s, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}

	// scoreboard
	y = SB_TOP - 24; //SB_HEADER

	//	CG_DrawPic( SB_SCORELINE_X - 40, y - 5, SB_SCORELINE_WIDTH + 80, 40, trap->R_RegisterShaderNoMip ( "gfx/menus/menu_buttonback.tga" ) );

	CG_Text_Paint( SB_NAME_X, y, 1.0f, &colorWhite, CG_GetStringEdString( "MP_INGAME", "NAME" ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	if ( cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL ) {
		char sWL[100];
		trap->SE_GetStringTextString( "MP_INGAME_W_L", sWL, sizeof(sWL) );

		CG_Text_Paint( SB_SCORE_X, y, 1.0f, &colorWhite, sWL, 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	}
	else
		CG_Text_Paint( SB_SCORE_X, y, 1.0f, &colorWhite, CG_GetStringEdString( "MP_INGAME", "SCORE" ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	CG_Text_Paint( SB_PING_X, y, 1.0f, &colorWhite, CG_GetStringEdString( "MP_INGAME", "PING" ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );
	CG_Text_Paint( SB_TIME_X, y, 1.0f, &colorWhite, CG_GetStringEdString( "MP_INGAME", "TIME" ), 0, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_MEDIUM );

	y = SB_TOP;

	// If there are more than SB_MAXCLIENTS_NORMAL, use the interleaved scores
	if ( cg.numScores > SB_MAXCLIENTS_NORMAL ) {
		maxClients = SB_MAXCLIENTS_INTER;
		lineHeight = SB_INTER_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 16;
	}
	else {
		maxClients = SB_MAXCLIENTS_NORMAL;
		lineHeight = SB_NORMAL_HEIGHT;
		topBorderSize = 8;
		bottomBorderSize = 8;
	}
	realMaxClients = maxClients;

	localClient = qfalse;


	//I guess this should end up being able to display 19 clients at once.
	//In a team game, if there are 9 or more clients on the team not in the lead, we only want to show 10 of the clients
	//	on the team in the lead, so that we have room to display the clients in the lead on the losing team.

	//I guess this can be accomplished simply by printing the first teams score with a maxClients value passed in
	//	related to how many players are on both teams.
	if ( cgs.gametype >= GT_TEAM ) {
		// teamplay scoreboard
		y += lineHeight / 2;

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			int team1MaxCl = CG_GetTeamCount( TEAM_RED, maxClients );
			int team2MaxCl = CG_GetTeamCount( TEAM_BLUE, maxClients );

			if ( team1MaxCl > 10 && (team1MaxCl + team2MaxCl) > maxClients ) {
				team1MaxCl -= team2MaxCl;
				// subtract as many as you have to down to 10, once we get there we just set it to 10

				if ( team1MaxCl < 10 )
					team1MaxCl = 10;
			}

			team2MaxCl = (maxClients - team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SCREEN_WIDTH - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//	maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SCREEN_WIDTH - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//	maxClients -= n2;

			//	maxClients -= (team1MaxCl+team2MaxCl);
		}
		else {
			int team1MaxCl = CG_GetTeamCount( TEAM_BLUE, maxClients );
			int team2MaxCl = CG_GetTeamCount( TEAM_RED, maxClients );

			if ( team1MaxCl > 10 && (team1MaxCl + team2MaxCl) > maxClients ) {
				team1MaxCl -= team2MaxCl;
				//subtract as many as you have to down to 10, once we get there
				//we just set it to 10

				if ( team1MaxCl < 10 )
					team1MaxCl = 10;
			}

			team2MaxCl = (maxClients - team1MaxCl); //team2 can display however many is left over after team1's display

			n1 = CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SCREEN_WIDTH - SB_SCORELINE_X * 2 + 10, n1 * lineHeight + bottomBorderSize, 0.33f, TEAM_BLUE );
			CG_TeamScoreboard( y, TEAM_BLUE, fade, team1MaxCl, lineHeight, qfalse );
			y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

			//	maxClients -= n1;

			n2 = CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qtrue );
			CG_DrawTeamBackground( SB_SCORELINE_X - 5, y - topBorderSize, SCREEN_WIDTH - SB_SCORELINE_X * 2 + 10, n2 * lineHeight + bottomBorderSize, 0.33f, TEAM_RED );
			CG_TeamScoreboard( y, TEAM_RED, fade, team2MaxCl, lineHeight, qfalse );
			y += (n2 * lineHeight) + BIGCHAR_HEIGHT;

			//	maxClients -= n2;

			//	maxClients -= (team1MaxCl+team2MaxCl);
		}
		maxClients = realMaxClients;
		n1 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;

	}
	else {
		// free for all scoreboard
		n1 = CG_TeamScoreboard( y, TEAM_FREE, fade, maxClients, lineHeight, qfalse );
		y += (n1 * lineHeight) + BIGCHAR_HEIGHT;
		n2 = CG_TeamScoreboard( y, TEAM_SPECTATOR, fade, maxClients - n1, lineHeight, qfalse );
		y += (n2 * lineHeight) + BIGCHAR_HEIGHT;
	}

	if ( !localClient ) {
		// draw local client at the bottom
		for ( i = 0; i<cg.numScores; i++ ) {
			if ( cg.scores[i].client == cg.snap->ps.clientNum ) {
				CG_DrawClientScore( y, &cg.scores[i], fadeColor, fade, lineHeight == SB_NORMAL_HEIGHT );
				break;
			}
		}
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 )
		CG_LoadDeferredPlayers();

	return qtrue;
}
