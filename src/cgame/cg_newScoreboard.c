#include "cg_local.h"
#include "../ui/ui_public.h"
#include "../ui/ui_shared.h"
#include "qcommon/qfiles.h"	// for STYLE_BLINK etc

int JP_GetScoreboardFont( void )
{
	int font = cg_newScoreboardFont.integer;
	BUMP( font, FONT_SMALL );
	CAP( font, FONT_NUM_FONTS );

	return font;
}

void DrawServerInformation( float fade )
{
	int fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	float fontScale = 0.5f, y = 20.0f, lineHeight=14.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };

	trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( cgs.japp.serverName, fontHandle, fontScale )/2.0, y, cgs.japp.serverName, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;

	tmp = va( "%s (%s)", (char *)CG_ConfigString( CS_MESSAGE ), cgs.mapname );
	trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;
	tmp = BG_GetGametypeString( cgs.gametype );
	trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight;

	switch ( cgs.gametype )
	{
	case GT_FFA:
		if ( cgs.timelimit && cgs.fraglimit )	tmp = va( "Until ^3%i ^7frags or ^3%i^7/^3%i minutes", cgs.fraglimit, (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.timelimit )				tmp = va( "Until ^3%i^7/^3%i ^7minutes", (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.fraglimit )				tmp = va( "Until ^3%i ^7frags", cgs.fraglimit );
		else									tmp = "Playing forever!";
		trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
		y += lineHeight*2;
		break;
	case GT_CTF:
	case GT_CTY:
		if ( cgs.timelimit && cgs.capturelimit )	tmp = va( "Until ^3%i^7/^3%i ^7captures or ^3%i^7/^3%i minutes", max(cgs.scores1, cgs.scores2), cgs.capturelimit, (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.timelimit )					tmp = va( "Until ^3%i^7/^3%i ^7minutes", (cg.time-cgs.levelStartTime)/60000, cgs.timelimit );
		else if ( cgs.capturelimit )				tmp = va( "Until ^3%i^7/^3%i ^7captures", max(cgs.scores1, cgs.scores2), cgs.capturelimit );
		else										tmp = "Playing forever!";
		trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
		y += lineHeight*2;
		//FALL THROUGH TO GENERIC TEAM GAME INFO!
	case GT_TEAM:
		if ( cgs.scores1 == cgs.scores2 )
		{
			tmp = "^3Teams are tied";
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( "^1%i ^7/ ^5%i", cgs.scores1, cgs.scores2 );
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
		}
		else if ( cgs.scores1 > cgs.scores2 )
		{
			tmp = "^1Red ^7leads";
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( "^1%i ^7/ ^5%i", cgs.scores1, cgs.scores2 );
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
		}
		else
		{
			tmp = "^5Blue ^7leads";
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
			tmp = va( "^5%i ^7/ ^1%i", cgs.scores2, cgs.scores1 );
			trap_R_Font_DrawString( SCREEN_WIDTH/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			y += lineHeight;
		}
		break;
	default:
		break;
	}
}





void DrawPlayerCount_Free( float fade )
{
	int fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	float fontScale = 0.5f, y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };
	int i = 0, freeCount = 0, specCount = 0, botCount = 0;
	float width = SCREEN_WIDTH/2.0f, lineHeight = 14.0f;

	for ( i=0; i<cg.numScores; i++ )
	{
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == TEAM_FREE )
			freeCount++;
		else if ( ci->team == TEAM_SPECTATOR )
			specCount++;
		if ( ci->botSkill != -1 )
			botCount++;
	}

	if ( botCount )
		tmp = va( "%i players / %i bots", freeCount-botCount, botCount );
	else
		tmp = va( "%i players", freeCount );
	trap_R_Font_DrawString( width/*/2.0*/ - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle, -1, fontScale );
	tmp = va( "%2i spectators", specCount );
	trap_R_Font_DrawString( width/* + width/2.0*/ - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + lineHeight*20, tmp, &colour, fontHandle, -1, fontScale );
}

void DrawPlayerCount_Team( float fade )
{
	int fontHandle = MenuFontToHandle( FONT_JAPPLARGE );
	float fontScale = 0.5f, y = 108.0f;
	const char *tmp = NULL;
	vector4 colour = { 1.0f, 1.0f, 1.0f, fade };
	int i = 0, redCount = 0, blueCount = 0, specCount = 0;
	int pingAccumRed = 0, pingAvgRed = 0, pingAccumBlue=0, pingAvgBlue=0;
	float width = SCREEN_WIDTH/2.0f, lineHeight=14.0f;

#if 0
	if ( cg_debugScoreboard.integer )
		redCount = blueCount = 16;
	else
#endif
	{
		for ( i=0; i<cg.numScores; i++ )
		{
			if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_RED )
			{
				pingAccumRed += cg.scores[i].ping;
				redCount++;
			}
			else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_BLUE )
			{
				blueCount++;
				pingAccumBlue += cg.scores[i].ping;
			}
			else if ( cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR )
				specCount++;
		}
	}

	if ( redCount )
		pingAvgRed = ceilf( pingAccumRed / redCount );
	if ( blueCount )
		pingAvgBlue = ceilf( pingAccumBlue / blueCount );

	if ( cgs.scores1 >= cgs.scores2 )
	{
		tmp = va( "^1%i players ^7(%3i avg ping)", redCount, pingAvgRed );
		trap_R_Font_DrawString( width/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle, -1, fontScale );
		tmp = va( "^5%i players ^7(%3i avg ping)", blueCount, pingAvgBlue );
		trap_R_Font_DrawString( width + width/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle, -1, fontScale );
	}
	else
	{
		tmp = va( "^5%i players ^7(%i avg ping)", blueCount, pingAvgBlue );
		trap_R_Font_DrawString( width/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle, -1, fontScale );
		tmp = va( "^1%i players ^7(%i avg ping)", redCount, pingAvgRed );
		trap_R_Font_DrawString( width + width/2.0 - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y, tmp, &colour, fontHandle, -1, fontScale );
	}

	tmp = va( "%2i spectators", specCount );
	trap_R_Font_DrawString( width/* + width/2.0*/ - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + lineHeight*20, tmp, &colour, fontHandle, -1, fontScale );
}

void DrawPlayerCount( float fade )
{
	switch ( cgs.gametype )
	{
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
	}
}

int PlayerCount( team_t team )
{
	int i=0, count=0;

	for ( i=0; i<cg.numScores; i++ )
	{
		clientInfo_t *ci = &cgs.clientinfo[cg.scores[i].client];
		if ( ci->team == team )
			count++;
	}

	return count;
}





//Return value: number of players on team 'team'
int ListPlayers_FFA( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, int startIndex, int playerCount )
{
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, fade },
			background	= { 0.75f, 0.75f, 0.75f, 0.6f*fade },
			blue		= { 0.6f, 0.6f, 1.0f, fade };
	int i = 0, count = playerCount, column = 0;
	float endX = SCREEN_WIDTH/2.0, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	if ( !count )
		return 0;

	trap_R_SetColor( &background );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5, cgs.media.japp.scoreboardLine );
	trap_R_SetColor( NULL );

	tmp = "Name";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=startIndex; i<startIndex+playerCount; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE )
		{
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap_R_SetColor( &blue );
			else
				trap_R_SetColor( &background );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.japp.scoreboardLine );
			trap_R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=startIndex; i<startIndex+playerCount; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == TEAM_FREE )
		{
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, fade },
					pingGood	= { 0.0f, 1.0f, 0.0f, fade },
					pingBad		= { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) )
			{
				trap_R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap_R_SetColor( NULL );
			}

			else if ( cg.snap->ps.duelInProgress && (ci-cgs.clientinfo == cg.snap->ps.duelIndex || ci-cgs.clientinfo == cg.snap->ps.clientNum) )
			{
				trap_R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0, lineHeight-2.0f, lineHeight-2.0f, cgs.media.powerDuelAllyShader );
				trap_R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 )
			{
				tmp = "BOT";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) 
			{
				tmp = "READY";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			//Name
			tmp = ci->name;
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Score
			if ( score->ping == -1 )
				tmp = "Connecting";
		//	else if ( team == TEAM_SPECTATOR )
		//		tmp = "Spectating"; //TODO: Name of client they're spectating? possible?
			else
				tmp = va( "%02i/%02i", score->score, score->deaths );

			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 )
			{
				if ( ci->botSkill != -1 )
				{
					tmp = "--";
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}
				else
				{//Ping
					tmp = va( "%i", score->ping );
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &pingColour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}

				//Time
				tmp = va( "%i", score->time );
				trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Return value: number of players on team 'team'
int ListPlayers_TDM( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team )
{
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, fade },
			border		= { 0.6f, 0.6f, 0.6f, fade },
			borderRed	= { 1.0f, 0.8f, 0.8f, fade },
			borderBlue	= { 1.0f, 0.8f, 0.8f, fade },
			blue		= { 0.6f, 0.6f, 1.0f, fade },
			background	= { 0.75f, 0.75f, 0.75f, 0.6f*fade },
			teamRed		= { 0.7f, 0.4f, 0.4f, 0.6f*fade },
			teamBlue	= { 0.4f, 0.4f, 0.7f, 0.6f*fade };
	vector4	*teamBackground	= &background,
			*teamBorder		= &border;
	int i = 0, count = 0, column = 0;
	float endX = SCREEN_WIDTH/2.0, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f, /*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	if ( team == TEAM_RED )
	{
		teamBackground = &teamRed;
		teamBorder = &borderRed;
	}
	else if ( team == TEAM_BLUE )
	{
		teamBackground = &teamBlue;
		teamBorder = &borderBlue;
	}

	for ( i=0; i<cg.numScores; i++ )
	{
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap_R_SetColor( teamBackground );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5, cgs.media.japp.scoreboardLine );
	trap_R_SetColor( NULL );

	tmp = "Name";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Cap";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Def";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Asst";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=0; i<cg.numScores; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team )
		{
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap_R_SetColor( &blue );
			else
				trap_R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.japp.scoreboardLine );
			trap_R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=0; i<cg.numScores; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team )
		{
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, fade },
					pingGood	= { 0.0f, 1.0f, 0.0f, fade },
					pingBad		= { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) )
			{
				trap_R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap_R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 )
			{
				tmp = "BOT";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) 
			{
				tmp = "READY";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			//Name
			tmp = ci->name;
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Score
			tmp = va( "%4i", score->score );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Capture
			tmp = va( "%2i", score->captures );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Defend
			tmp = va( "%2i", score->defendCount );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Assist
			tmp = va( "%2i", score->assistCount );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 )
			{
				if ( ci->botSkill != -1 )
				{
					tmp = "--";
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}
				else
				{//Ping
					tmp = va( "%i", score->ping );
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &pingColour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}

				//Time
				tmp = va( "%i", score->time );
				trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Return value: number of players on team 'team'
int ListPlayers_CTF( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team )
{
	const char *tmp = NULL;
	vector4	white		= { 1.0f, 1.0f, 1.0f, fade },
			border		= { 0.6f, 0.6f, 0.6f, fade },
			borderRed	= { 1.0f, 0.8f, 0.8f, fade },
			borderBlue	= { 1.0f, 0.8f, 0.8f, fade },
			blue		= { 0.6f, 0.6f, 1.0f, fade },
			background	= { 0.75f, 0.75f, 0.75f, 0.6f*fade },
			teamRed		= { 0.7f, 0.4f, 0.4f, 0.6f*fade },
			teamBlue	= { 0.4f, 0.4f, 0.7f, 0.6f*fade };
	vector4	*teamBackground	= &background,
			*teamBorder		= &border;
	int i = 0, count = 0, column = 0;
	float endX = SCREEN_WIDTH/2.0, columnOffset[] = { /*name*/80.0f, /*score*/170.0f, /*capture*/195.0f, /*defend*/220.f, /*assist*/245.f, /*ping*/270.0f, /*time*/295.0f }, savedY=0.0f;

	if ( team == TEAM_RED )
	{
		teamBackground = &teamRed;
		teamBorder = &borderRed;
	}
	else if ( team == TEAM_BLUE )
	{
		teamBackground = &teamBlue;
		teamBorder = &borderBlue;
	}

	for ( i=0; i<cg.numScores; i++ )
	{
		if ( cgs.clientinfo[cg.scores[i].client].team == team )
			count++;
	}

	if ( !count )
		return 0;

	trap_R_SetColor( teamBackground );
		CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight*1.5, cgs.media.japp.scoreboardLine );
	trap_R_SetColor( NULL );

	tmp = "Name";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Score";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Cap";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Def";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Asst";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Ping";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

	tmp = "Time";
	trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
	y += lineHeight*1.5f;

	savedY = y;

	// First pass, background + borders
	for ( i=0; i<cg.numScores; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team )
		{
			column = 0;

			// background
			if ( ci-cgs.clientinfo == cg.snap->ps.clientNum )
				trap_R_SetColor( &blue );
			else
				trap_R_SetColor( teamBackground );

			CG_DrawPic( x + 10.0f - 5.0f, y, endX - 10.0f*2 + 5.0f, lineHeight, cgs.media.japp.scoreboardLine );
			trap_R_SetColor( NULL );

			y += lineHeight;
		}
	}
	y = savedY;

	// Second pass, text + icons
	for ( i=0; i<cg.numScores; i++ )
	{
		score_t *score = &cg.scores[i];
		clientInfo_t *ci = &cgs.clientinfo[score->client];
		if ( ci->team == team )
		{
			vector4	pingColour	= { 1.0f, 1.0f, 1.0f, fade },
					pingGood	= { 0.0f, 1.0f, 0.0f, fade },
					pingBad		= { 1.0f, 0.0f, 0.0f, fade };
			CG_LerpColour( &pingGood, &pingBad, &pingColour, min( score->ping / 300.0f, 1.0f ) );

			column = 0;

			if ( ci->powerups & ((1<<PW_BLUEFLAG)|(1<<PW_REDFLAG)) )
			{
				trap_R_SetColor( &white );
					CG_DrawPic( x+5.0f, y, lineHeight, lineHeight, cgs.media.flagShaders[FLAG_TAKEN] );
				trap_R_SetColor( NULL );
			}

			else if ( score->ping >= 999 || (cg_entities[score->client].currentState.eFlags&EF_CONNECTION) )
			{
				trap_R_SetColor( &white );
					CG_DrawPic( x+5.0f, y+1.0, lineHeight-2.0f, lineHeight-2.0f, cgs.media.connectionShader );
				trap_R_SetColor( NULL );
			}

			else if ( ci->botSkill != -1 )
			{
				tmp = "BOT";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			else if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & (1<<score->client) ) 
			{
				tmp = "READY";
				trap_R_Font_DrawString( x + 8.0f, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			//Name
			tmp = ci->name;
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Score
			tmp = va( "%4i", score->score );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Capture
			tmp = va( "%2i", score->captures );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Defend
			tmp = va( "%2i", score->defendCount );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			//Assist
			tmp = va( "%2i", score->assistCount );
			trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );

			if ( score->ping != -1 )
			{
				if ( ci->botSkill != -1 )
				{
					tmp = "--";
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}
				else
				{//Ping
					tmp = va( "%i", score->ping );
					trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &pingColour, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
				}

				//Time
				tmp = va( "%i", score->time );
				trap_R_Font_DrawString( x + columnOffset[column++] - trap_R_Font_StrLenPixels( tmp, fontHandle, fontScale )/2.0, y + (lineHeight/2.0) - (trap_R_Font_HeightPixels( fontHandle, fontScale )/2.0)-1.0f, tmp, &white, fontHandle|STYLE_DROPSHADOW, -1, fontScale );
			}

			y += lineHeight;
		}
	}

	return count;
}

//Will render a list of players on team 'team' at 'x','y'
//	will use relevant information based on gametype
int ListPlayers_Free( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, int startIndex, int playerCount )
{
	switch ( cgs.gametype )
	{
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
		CG_Error( "Tried to use non-team scoreboard on team gametype" );
		break;
	}

	return -1;
}

//Will render a list of players on team 'team' at 'x','y'
//	will use relevant information based on gametype
int ListPlayers_Team( float fade, float x, float y, float fontScale, int fontHandle, float lineHeight, team_t team )
{
	switch ( cgs.gametype )
	{
	case GT_FFA:
	case GT_HOLOCRON:
	case GT_JEDIMASTER:
	case GT_DUEL:
	case GT_POWERDUEL:
	case GT_SINGLE_PLAYER:
		CG_Error( "Tried to use team scoreboard on non-team gametype" );
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
	}

	return -1;
}

void DrawSpectators( float fade )
{
	int fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	float fontScale = 0.5f, y = 128.0f, lineHeight=14.0f;
	vector4	white = { 1.0f, 1.0f, 1.0f, fade };

	CG_BuildSpectatorString();
	cg.scoreboard.spectatorWidth = CG_Text_Width( cg.scoreboard.spectatorList, fontScale, fontHandle );

	if ( cg.scoreboard.spectatorLen )
	{
		float dt = (cg.time - cg.scoreboard.spectatorResetTime)*0.0625f;
		cg.scoreboard.spectatorX = SCREEN_WIDTH - (1.0f)*dt;

		if ( cg.scoreboard.spectatorX < 0 - cg.scoreboard.spectatorWidth )
		{
			cg.scoreboard.spectatorX = SCREEN_WIDTH;
			cg.scoreboard.spectatorResetTime = cg.time;
		}
		CG_Text_Paint( cg.scoreboard.spectatorX, (y + lineHeight*20) - 3, fontScale, &white, cg.scoreboard.spectatorList, 0, 0, ITEM_TEXTSTYLE_SHADOWED, fontHandle );
	}
}

void DrawPlayers_Free( float fade )
{
	int fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	float fontScale = 0.45f, x = 0, y = 128.0f, lineHeight=14.0f;
	int playerCount = PlayerCount( TEAM_FREE );

	ListPlayers_Free( fade, x,						y, fontScale, fontHandle, lineHeight, 0, (playerCount+1)/2 );	// dirty hack, 7 players means 4 on left, so +1 then /2 (8/2==4) will give a good number (4 left, 3 right)
																													//	similarly, 8 players means 4 on left, so +1 then /2 (9/2==4) will give a good number (4 left, 4 right)
	ListPlayers_Free( fade, x + SCREEN_WIDTH/2.0,	y, fontScale, fontHandle, lineHeight, (playerCount+1)/2, playerCount/2 );

	DrawSpectators( fade );
}

void DrawPlayers_Team( float fade )
{
	int fontHandle = MenuFontToHandle( JP_GetScoreboardFont() );
	float fontScale = 0.5f/*0.17f*/, x = 0, y = 128.0f, lineHeight=14.0f;

	if ( cgs.scores1 >= cgs.scores2 )
	{
		ListPlayers_Team( fade, x,						y, fontScale, fontHandle, lineHeight, TEAM_RED );
		ListPlayers_Team( fade, x + SCREEN_WIDTH/2.0,	y, fontScale, fontHandle, lineHeight, TEAM_BLUE );
	}
	else
	{
		ListPlayers_Team( fade, x,						y, fontScale, fontHandle, lineHeight, TEAM_BLUE );
		ListPlayers_Team( fade, x + SCREEN_WIDTH/2.0,	y, fontScale, fontHandle, lineHeight, TEAM_RED );
	}

	DrawSpectators( fade );
}

//controller for drawing the 'players' section
//	will call the relevant functions based on gametype
void DrawPlayers( float fade )
{
	switch ( cgs.gametype )
	{
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
	}
}

//Scoreboard entry point
//	This will be called even if the scoreboard is not showing - and will return false if it has faded aka 'not showing'
//	It will return true if the scoreboard is showing
qboolean CG_DrawQ3PScoreboard( void )
{
	vector4 fadeWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
	float fade = 1.0f;

	if ( cg.warmup && !cg.showScores )
		return qfalse;

	if ( !cg.showScores && cg.snap->ps.pm_type != PM_DEAD && cg.snap->ps.pm_type != PM_INTERMISSION )
	{
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
