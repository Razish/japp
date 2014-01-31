// Copyright (C) 1999-2000 Id Software, Inc.
//
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

#define NUM_UI_ARGSTRS (4)
#define UI_ARGSTR_MASK (NUM_UI_ARGSTRS-1)
static char tempArgStrs[NUM_UI_ARGSTRS][MAX_STRING_CHARS];

static char *UI_Argv( int arg ) {
	static int index=0;
	char *s = tempArgStrs[index++ & UI_ARGSTR_MASK];
	trap->Cmd_Argv( arg, s, MAX_STRING_CHARS );
	return s;
}

#define NUM_UI_CVARSTRS (4)
#define UI_CVARSTR_MASK (NUM_UI_CVARSTRS-1)
static char tempCvarStrs[NUM_UI_CVARSTRS][MAX_CVAR_VALUE_STRING];

char *UI_Cvar_VariableString( const char *name ) {
	static int index=0;
	char *s = tempCvarStrs[index++ & UI_ARGSTR_MASK];
	trap->Cvar_VariableStringBuffer( name, s, MAX_CVAR_VALUE_STRING );
	return s;
}

static void	UI_Cache_f( void ) {
	Display_CacheAll();
	if (trap->Cmd_Argc() == 2) {
		int i;
		for ( i=0; i<uiInfo.q3HeadCount; i++ ) {
			trap->Print( "model %s\n", uiInfo.q3HeadNames[i] );
		}
	}
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand( int realTime ) {
	char	*cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realTime;

	cmd = UI_Argv( 0 );

	// ensure minimum menu data is available
	//Menu_Cache();

	if ( Q_stricmp (cmd, "ui_report") == 0 ) {
		UI_Report();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_load") == 0 ) {
		UI_Load();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_opensiegemenu" ) == 0 )
	{
		if ( trap->Cvar_VariableValue ( "g_gametype" ) == GT_SIEGE )
		{
			Menus_CloseAll();
			if (Menus_ActivateByName(UI_Argv(1)))
			{
				trap->Key_SetCatcher( KEYCATCH_UI );
			}
		}
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "ui_openmenu" ) )
	{
		//if ( trap->Cvar_VariableValue ( "developer" ) )
		{
			Menus_CloseAll();
			if ( Menus_ActivateByName( UI_Argv( 1 ) ) )
				trap->Key_SetCatcher( KEYCATCH_UI );
			return qtrue;
		}
	}

	/*
	if ( Q_stricmp (cmd, "remapShader") == 0 ) {
		if (trap->Cmd_Argc() == 4) {
			char shader1[MAX_QPATH];
			char shader2[MAX_QPATH];
			Q_strncpyz(shader1, UI_Argv(1), sizeof(shader1));
			Q_strncpyz(shader2, UI_Argv(2), sizeof(shader2));
			trap->R_RemapShader(shader1, shader2, UI_Argv(3));
			return qtrue;
		}
	}
	*/

	if ( Q_stricmp (cmd, "ui_cache") == 0 ) {
		UI_Cache_f();
		return qtrue;
	}

	if ( Q_stricmp (cmd, "ui_teamOrders") == 0 ) {
	//	UI_TeamOrdersMenu_f();
		return qtrue;
	}


	if ( Q_stricmp (cmd, "ui_cdkey") == 0 ) {
	//	UI_CDKeyMenu_f();
		return qtrue;
	}

#ifdef FAV_SERVERS
	if ( !Q_stricmp( cmd, "server" ) )
	{
		unsigned int i=0;
		char *args = UI_Argv( 1 );
		for ( ; i<uiLocal.serversCount; i++ )
		{
			if ( !Q_stricmp( uiLocal.servers[i].name, args ) )
			{//Found it
				//Auto-login
				trap->Cvar_Set( "cp_login", uiLocal.servers[i].adminPassword );

				//Connect
				trap->Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiLocal.servers[i].ip ) );
				return qtrue;
			}
		}
		Com_Printf( "Favourite server not found!\n" );
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "japp_favsrv_list" ) )
	{
		unsigned int i = 0;
		netadr_t netAddr = { 0 };

		Com_Printf( S_COLOR_YELLOW"%-32s%-32sIP\n", "Name", "Address" );
		for ( i=0; i<uiLocal.serversCount; i++ )
		{
			if ( ENG_NET_StringToAddr( uiLocal.servers[i].ip, &netAddr ) )
				Com_Printf( S_COLOR_WHITE"%-32s%-32s%i.%i.%i.%i:%i\n", uiLocal.servers[i].name, uiLocal.servers[i].ip, netAddr.ip[0], netAddr.ip[1], netAddr.ip[2], netAddr.ip[3], BigShort( netAddr.port ) );
			else
				Com_Printf( S_COLOR_WHITE"%-32s%-32s"S_COLOR_RED"Could not resolve address\n", uiLocal.servers[i].name, uiLocal.servers[i].ip );
		}
		Com_Printf( "\n" );
		return qtrue;
	}

	if ( !Q_stricmp( cmd, "japp_favsrv_add" ) )
	{
		netadr_t netAddr = { 0 };
		char serverName[64] = { 0 };
		char serverAddrBuf[64] = { 0 };
		char *serverAddress = &serverAddrBuf[0];

		Q_strncpyz( serverName, UI_Argv( 1 ), sizeof( serverName ) );
		Q_strncpyz( serverAddrBuf, UI_Argv( 2 ), sizeof( serverAddrBuf ) );

		if ( trap->Cmd_Argc() < 3 ) {
			Com_Printf( S_COLOR_YELLOW"Usage: japp_favsrv_add [name] [ip or hostname or ! for current server] [optional admin password]\n" );
		}
		else
		{
			if ( !Q_stricmp( serverAddress, "!" ) )
			{
				uiClientState_t clientState;
				trap->GetClientState( &clientState );
				if ( clientState.connState == CA_ACTIVE )
					serverAddress = (char*)0x8AF108;
				else
				{
					Com_Printf( "You must be connected to a server to add the current server to your favourites\n" );
					return qtrue;
				}
			}
			if ( !ENG_NET_StringToAddr( serverAddress, &netAddr ) )
			{//If it's not a valid address, don't bother adding it!
				Com_Printf( "Bad server address: %s\n", serverAddress );
			}
			else
			{
				unsigned int i = 0;
				unsigned int targetIP = *(unsigned int *)&netAddr.ip[0];
				unsigned short targetPort = BigShort( netAddr.port );

				for ( i=0; i<uiLocal.serversCount; i++ )
				{
					if ( ENG_NET_StringToAddr( uiLocal.servers[i].ip, &netAddr ) && targetIP == *(unsigned int*)&netAddr.ip[0] && targetPort == BigShort( netAddr.port ) )
					{//Same IP and port
						Com_Printf( "Favourite server IP already found (name: "S_COLOR_CYAN"%s"S_COLOR_WHITE", addr: "S_COLOR_CYAN"%s"S_COLOR_WHITE", ip: "S_COLOR_CYAN"%i.%i.%i.%i:%i"S_COLOR_WHITE")\n", uiLocal.servers[i].name, uiLocal.servers[i].ip, netAddr.ip[0], netAddr.ip[1], netAddr.ip[2], netAddr.ip[3], BigShort( netAddr.port ) );
						return qtrue;
					}
					if ( !Q_stricmp( serverName, uiLocal.servers[i].name ) || !Q_stricmp( serverAddress, uiLocal.servers[i].ip ) )
					{
						Com_Printf( "Favourite server '%s' already found (addr: %s)\n", uiLocal.servers[i].name, uiLocal.servers[i].ip );
						return qtrue;
					}
				}

				//If we got here, there were no matches - append the favourite server.
				Com_Printf( "Adding favourite server %s (addr: %s)\n", serverName, serverAddress );
				Q_strncpyz( uiLocal.servers[uiLocal.serversCount].ip, serverAddress, sizeof( uiLocal.servers[0].ip ) );
				Q_strncpyz( uiLocal.servers[uiLocal.serversCount].name, serverName, sizeof( uiLocal.servers[0].name ) );
				uiLocal.serversCount++;
			}
		}
		return qtrue;
	}
#endif //FAV_SERVERS

	return qfalse;
}

void UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader ) {
	float	s0;
	float	s1;
	float	t0;
	float	t1;

	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1;
	}

	trap->R_DrawStretchPic( x, y, w, h, s0, t0, s1, t1, hShader );
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );
	trap->R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_SetColor( NULL );
}

void UI_DrawSides(float x, float y, float w, float h) {
	trap->R_DrawStretchPic( x, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x + w - 1, y, 1, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom(float x, float y, float w, float h) {
	trap->R_DrawStretchPic( x, y, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap->R_DrawStretchPic( x, y + h - 1, w, 1, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, const vector4 *color ) {
	trap->R_SetColor( color );

	UI_DrawTopBottom(x, y, width, height);
	UI_DrawSides(x, y, width, height);

	trap->R_SetColor( NULL );
}
