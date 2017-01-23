// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "bg_saga.h"
#include "bg_lua.h"
#include "ui/ui_shared.h"

#define __STDC_FORMAT_MACROS // older compilers need this
#include <inttypes.h>

void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == -1 ) {
		return;
	}

	trap->Cmd_Argv( 1, test, 4 );
	trap->SendConsoleCommand( va( "gc %i %i", targetNum, atoi( test ) ) );

}

static void CG_SizeUp_f( void ) {
	trap->Cvar_Set( "cg_viewSize", va( "%i", (int)(cg_viewSize.integer + 10) ) );
}

static void CG_SizeDown_f( void ) {
	trap->Cvar_Set( "cg_viewSize", va( "%i", (int)(cg_viewSize.integer - 10) ) );
}

static void CG_Viewpos_f( void ) {
	refdef_t *refdef = CG_GetRefdef();
	trap->Print( "%s (%i %i %i) : %i\n", cgs.mapname, (int)refdef->vieworg.x, (int)refdef->vieworg.y, (int)refdef->vieworg.z, (int)refdef->viewangles.yaw );
}

void CG_ScoresDown_f( void ) {
	CG_BuildSpectatorString();
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;
		trap->SendClientCommand( "score" );

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if ( !cg.showScores ) {
			cg.showScores = qtrue;
			//	cg.numScores = 0;
		}
	}
	else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

static void CG_ScoresUp_f( void ) {
	if ( cg.showScores ) {
		cg.showScores = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}

static void CG_TellClient( int clientNum ) {
	if ( clientNum == -1 ) {
		return;
	}

	char message[MAX_SAY_TEXT];
	trap->Cmd_Args( message, sizeof(message) );

	char cmd[MAX_STRING_CHARS];
	Com_sprintf( cmd, sizeof(cmd), "tell %i %s\n", clientNum, message );
	trap->SendClientCommand( cmd );
}

static void CG_TellTarget_f( void ) {
	CG_TellClient( CG_CrosshairPlayer() );
}

static void CG_TellAttacker_f( void ) {
	CG_TellClient( CG_LastAttacker() );
}

void CG_SiegeBriefingDisplay( int team, qboolean dontShow );
static void CG_SiegeBriefing_f( void ) {
	int team;

	if ( cgs.gametype != GT_SIEGE )
		return;

	team = cg.predictedPlayerState.persistant[PERS_TEAM];
	if ( team != SIEGETEAM_TEAM1 && team != SIEGETEAM_TEAM2 )
		return;

	CG_SiegeBriefingDisplay( team, qfalse );
}

static void CG_SiegeCvarUpdate_f( void ) {
	int team;

	if ( cgs.gametype != GT_SIEGE )
		return;

	team = cg.predictedPlayerState.persistant[PERS_TEAM];

	if ( team != SIEGETEAM_TEAM1 && team != SIEGETEAM_TEAM2 )
		return;

	CG_SiegeBriefingDisplay( team, qtrue );
}

static void CG_SiegeCompleteCvarUpdate_f( void ) {
	if ( cgs.gametype != GT_SIEGE ) { //Cannot be displayed unless in this gametype
		return;
	}

	// Set up cvars for both teams
	CG_SiegeBriefingDisplay( SIEGETEAM_TEAM1, qtrue );
	CG_SiegeBriefingDisplay( SIEGETEAM_TEAM2, qtrue );
}

static void CG_CopyNames_f( void ) {
#ifdef _WIN32
	char far	*buffer;
	int			bytes = 0;
	HGLOBAL		clipbuffer;
	int			i;
	char		buf[1216] = { 0 };	//(32*36) = 1152 + 64 scrap

	memset( buf, 0, sizeof(buf) );
	buf[0] = '\0';

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		char *toClip = NULL;
		if ( cgs.clientinfo[i].infoValid ) {
			toClip = cgs.clientinfo[i].name;
			Q_strcat( buf, sizeof(buf), va( "%s\n", toClip ) );
		}
	}
	Q_strcat( buf, sizeof(buf), "\r\n" );	//Clipboard requires CRLF ending
	bytes = strlen( buf );

	OpenClipboard( NULL );
	EmptyClipboard();

	clipbuffer = GlobalAlloc( GMEM_DDESHARE, bytes + 1 );
	buffer = (char far *)GlobalLock( clipbuffer ); // 'argument 1' might be '0': this does not adhere to the specification for the function 'GlobalLock'

	if ( !buffer )
		return;// GetLastError() * -1; // Do what you want to signal error

	strcpy( buffer, buf );

	GlobalUnlock( clipbuffer );
	SetClipboardData( CF_TEXT, clipbuffer );
	CloseClipboard();
#else //WIN32
	trap->Print( "Feature not yet implemented for your operating system (copynames)\n" );
#endif //WIN32
}

#if !defined(NO_CRASHHANDLER) && !defined(MACOS_X)
static void CG_Crash_f( void ) {
	qasm1( int 3 );
}
#endif

static void CG_ShowPlayerID_f( void ) {
	for ( int i = 0; i < cgs.maxclients; i++ ) {
		if ( cgs.clientinfo[i].infoValid ) {
			Com_Printf( S_COLOR_WHITE "(" S_COLOR_CYAN "%i" S_COLOR_WHITE ") %s\n", i, cgs.clientinfo[i].name );
		}
	}
}

static const char *pluginDisableStrings[] = {
	"Plugin_NewDrainEFX_needReconnection",
	"Plugin_IgnoreAllPlayers_WhenInDuel",
	"PLugin_EndDuel_Rotation",
	"Plugin_seeBlackSabers_needUserInfoUpdate",
	"Plugin_AutoReplier",
	"Plugin_NewForceEffect",
	"Plugin_No_NewDeathMessage",
	"Plugin_NewForceSight_effect",
	"Plugin_NoAltDim_effect",
	"Plugin_holsteredSaberBolted",
	"Plugin_ledgeGrab",
	"Plugin_NewDFA_PrimAttack",
	"Plugin_NewDFA_AltAttack",
	"Plugin_No_SP_Cartwheel",
	"Plugin_AllowDownloadURL_Redirect",
	"japp_NoKata",
	"japp_NoButterfly",
	"japp_NoStab",
	"japp_NoDFA",
	"japp_OldGrapple",
};
static const size_t numPluginDisableOpts = ARRAY_LEN( pluginDisableStrings );

// 0x30025430 JA+ cgame 1.4 beta3
static void CG_PluginDisable_f( void ) {
	uint32_t i;
	if ( trap->Cmd_Argc() > 1 ) {
		char arg[8] = { 0 }, buf[16] = { 0 };
		int current, index;
		uint32_t toggle;
		trap->Cmd_Argv( 1, arg, sizeof(arg) );
		index = toggle = atoi( arg );
		if ( toggle >= numPluginDisableOpts ) {
			Com_Printf( "Invalid pluginDisable value: %u\n", toggle );
			return;
		}

		trap->Cvar_VariableStringBuffer( "cp_pluginDisable", buf, sizeof(buf) );
		current = atoi( buf );
		toggle = (1 << index);
		trap->Cvar_Set( "cp_pluginDisable", va( "%i", toggle ^ current ) );

		Com_Printf( "%s %s\n", pluginDisableStrings[index], ((current&toggle) ? S_COLOR_GREEN"Allowed" : S_COLOR_RED"Disallowed") );
	}
	else {
		char buf[16] = { 0 };
		trap->Cvar_VariableStringBuffer( "cp_pluginDisable", buf, sizeof(buf) );

		Com_Printf( "Usage: /pluginDisable <ID>\n" );
		for ( i = 0; i < numPluginDisableOpts; i++ ) {
			qboolean allowed = !(atoi( buf ) & (1 << i));
			Com_Printf( S_COLOR_WHITE "(" S_COLOR_CYAN "%i" S_COLOR_WHITE ") ^%c%s\n", i,
				(allowed ? COLOR_GREEN : COLOR_RED), pluginDisableStrings[i]
			);
		}
	}
}

static void CG_ScrollChat_f( void ) {
	char args[8] = { 0 };
	trap->Cmd_Args( args, sizeof(args) );
	CG_ChatboxScroll( atoi( args ) );
}

static void CG_ClearChat_f( void ) {
	CG_ChatboxClear();
}

static void CG_Menu_f( void ) {
	trap->Key_SetCatcher( trap->Key_GetCatcher() ^ KEYCATCH_CGAME );
}

static void Cmd_ChatboxSelectTabNextNoKeys( void ) {
	CG_ChatboxSelectTabNextNoKeys();
}
static void Cmd_ChatboxSelectTabPrevNoKeys( void ) {
	CG_ChatboxSelectTabPrevNoKeys();
}

static void CG_ChatboxFindTab_f( void ) {
	char args[128];
	trap->Cmd_Args( args, sizeof(args) );
	CG_ChatboxSelect( args );
}

#ifdef JPLUA

void CG_LuaDoString_f( void ) {
	char arg[MAX_TOKEN_CHARS];
	char buf[4096] = { 0 };
	int i = 0;
	int argc = trap->Cmd_Argc();

	if ( argc < 2 || !JPLua::IsInitialised() )
		return;

	for ( i = 1; i < argc; i++ ) {
		trap->Cmd_Argv( i, arg, sizeof(arg) );
		Q_strcat( buf, sizeof(buf), va( "%s ", arg ) );
	}

	if ( trap->Key_GetCatcher() & KEYCATCH_CONSOLE ) {
		trap->Print( S_COLOR_CYAN"Executing Lua code...\n" );
	}
	JPLua::DoString( buf );
}

void CG_LuaList_f( void ) {
	JPLua::ListPlugins();
}

void CG_LuaLoad_f( void ) {
	int argc = trap->Cmd_Argc();
	if ( argc < 2 ) {
		trap->Print( "You must specify a plugin name\n" );
		return;
	}

	for ( int i = 1; i < argc; i++ ) {
		char pluginName[32];
		trap->Cmd_Argv( i, pluginName, sizeof(pluginName) );
		JPLua::plugin_t *plugin = JPLua::FindPlugin( pluginName );
		if ( plugin ) {
			JPLua::EnablePlugin( plugin );
		}
	}
}

void CG_LuaReload_f( void ) {
	if ( trap->Cmd_Argc() == 1 ) {
		// just reload everything
		trap->Print( S_COLOR_CYAN "Reloading JPLua...\n" );
		JPLua::Shutdown( qtrue );
		JPLua::Init();
		return;
	}

	char *args = ConcatArgs( 1 );
	const char *delim = " ";
	//FIXME: p is getting corrupted somehow, and then tries to load a plugin with that corrupted name
	for ( char *p = strtok( args, delim ); p; p = strtok( NULL, delim ) ) {
		JPLua::plugin_t *plugin = JPLua::FindPlugin( p );
		if ( plugin ) {
			JPLua::DisablePlugin( plugin );
			JPLua::EnablePlugin( plugin );
		}
	}
}

void CG_LuaUnload_f( void ) {
	if ( trap->Cmd_Argc() != 2 ) {
		trap->Print( "You must specify the plugin's name\n" );
		return;
	}

	char pluginName[32];
	trap->Cmd_Argv( 1, pluginName, sizeof(pluginName) );
	JPLua::plugin_t *plugin = JPLua::FindPlugin( pluginName );
	if ( plugin ) {
		JPLua::DisablePlugin( plugin );
	}
}

#endif // JPLUA

void CG_FixDirection( void ) {
	if ( cg.japp.isfixedVector ) {
		cg.japp.isfixedVector = qfalse;
		trap->Print( "Direction unset.\n" );
		return;
	}

	if ( trap->Cmd_Argc() == 3 ) {
		cg.japp.fixedVector.x = atof( CG_Argv( 1 ) );
		cg.japp.fixedVector.y = atof( CG_Argv( 2 ) );
	}
	else {
		AngleVectors( &cg.predictedPlayerState.viewangles, &cg.japp.fixedVector, NULL, NULL );
	}

	cg.japp.fixedVector.z = 0;
	cg.japp.isfixedVector = qtrue;
	trap->Print( "Direction set (%.3f,%.3f).\n", cg.japp.fixedVector.x, cg.japp.fixedVector.y );
}

void CG_SayTeam_f( void ) {
	char buf[MAX_TOKEN_CHARS] = { 0 };
	trap->Cmd_Args( buf, sizeof(buf) );
	HandleTeamBinds( buf, sizeof(buf) );
	trap->SendClientCommand( va( "say_team %s", buf ) );
}

static void CG_HudReload_f( void ) {
	String_Init();
	Menu_Reset();

	const char *hudSet = cg_hudFiles.string;
	if ( hudSet[0] == '\0' ) {
		hudSet = "ui/jahud.txt";
	}

	CG_LoadMenus( hudSet );
}

void CG_MessageModeAll_f( void ) {
	if( cg_newChatbox.integer )
		CG_ChatboxOpen( CHAT_ALL );
	else
		trap->SendConsoleCommand( "messagemode" );
}

void CG_MessageModeTeam_f( void ) {
	if( cg_newChatbox.integer )
		CG_ChatboxOpen( CHAT_TEAM );
	else
		trap->SendConsoleCommand( "messagemode2" );
}

void CG_MessageModeTell_f( void ) {
	if( cg_newChatbox.integer )
		CG_ChatboxOpen( CHAT_WHISPER );
	else
		trap->SendConsoleCommand( "messagemode3" );
}

typedef struct command_s {
	const char *name;
	void( *func )(void);
} command_t;

static const command_t commands[] = {
	{ "+scores", CG_ScoresDown_f },
	{ "-scores", CG_ScoresUp_f },
	{ "addbot", NULL },
	{ "bot_order", NULL },
	{ "briefing", CG_SiegeBriefing_f },
	{ "callvote", NULL },
	{ "cgmenu", CG_Menu_f },
	{ "chattabfind", CG_ChatboxFindTab_f },
	{ "chattabnext", Cmd_ChatboxSelectTabNextNoKeys },
	{ "chattabprev", Cmd_ChatboxSelectTabPrevNoKeys },
	{ "clearchat", CG_ClearChat_f },
	{ "copynames", CG_CopyNames_f },
#if !defined(NO_CRASHHANDLER) && !defined(MACOS_X)
	{ "crash", CG_Crash_f },
#endif
	{ "engage_fullforceduel", NULL },
	{ "engage_duel", NULL },
	{ "follow", NULL },
	{ "forcechanged", NULL },
	{ "forcenext", CG_NextForcePower_f },
	{ "forceprev", CG_PrevForcePower_f },
	{ "force_absorb", NULL },
	{ "force_distract", NULL },
	{ "force_forcepowerother", NULL },
	{ "force_heal", NULL },
	{ "force_healother", NULL },
	{ "force_protect", NULL },
	{ "force_pull", NULL },
	{ "force_rage", NULL },
	{ "force_seeing", NULL },
	{ "force_speed", NULL },
	{ "force_throw", NULL },
	{ "give", NULL },
	{ "god", NULL },
	{ "hud_reload", CG_HudReload_f },
	{ "invnext", CG_NextInventory_f },
	{ "invprev", CG_PrevInventory_f },
	{ "kill", NULL },
	{ "levelshot", NULL },
	{ "loaddefered", NULL },
	{ "loaddeferred", CG_LoadDeferredPlayers },
#ifdef JPLUA
	{ "lua", CG_LuaDoString_f },
	{ "lua_list", CG_LuaList_f },
	{ "lua_load", CG_LuaLoad_f },
	{ "lua_reload", CG_LuaReload_f },
	{ "lua_unload", CG_LuaUnload_f },
#endif // JPLUA
	{ "messagemodeAll", CG_MessageModeAll_f },
	{ "messagemodeTeam", CG_MessageModeTeam_f },
	{ "messageModeTell", CG_MessageModeTell_f },
	{ "nextframe", CG_TestModelNextFrame_f },
	{ "nextskin", CG_TestModelNextSkin_f },
	{ "noclip", NULL },
	{ "notarget", NULL },
	{ "npc", NULL },
	{ "pluginDisable", CG_PluginDisable_f },
	{ "prevframe", CG_TestModelPrevFrame_f },
	{ "prevskin", CG_TestModelPrevSkin_f },
	{ "saberAttackCycle", NULL },
	{ "say", NULL },
	{ "say_team", CG_SayTeam_f },
	{ "scrollChat", CG_ScrollChat_f },
	{ "setviewpos", NULL },
	{ "showPlayerID", CG_ShowPlayerID_f },
	{ "siegeCompleteCvarUpdate", CG_SiegeCompleteCvarUpdate_f },
	{ "siegeCvarUpdate", CG_SiegeCvarUpdate_f },
	{ "sizedown", CG_SizeDown_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sm_fix_direction", CG_FixDirection },
	{ "stats", NULL },
	{ "sv_forcenext", NULL },
	{ "sv_forceprev", NULL },
	{ "sv_invnext", NULL },
	{ "sv_invprev", NULL },
	{ "sv_saberswitch", NULL },
	{ "tcmd", CG_TargetCommand_f },
	{ "team", NULL },
	{ "teamtask", NULL },
	{ "tell", NULL },
	{ "tell_attacker", CG_TellAttacker_f },
	{ "tell_target", CG_TellTarget_f },
	{ "testgun", CG_TestGun_f },
	{ "testmodel", CG_TestModel_f },
	{ "use_bacta", NULL },
	{ "use_electrobinoculars", NULL },
	{ "use_field", NULL },
	{ "use_seeker", NULL },
	{ "use_sentry", NULL },
	{ "viewpos", CG_Viewpos_f },
	{ "vote", NULL },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "weaponclean", CG_WeaponClean_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "zoom", NULL },
};
static const size_t numCommands = ARRAY_LEN( commands );

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((command_t*)b)->name );
}

// The string has been tokenized and can be retrieved with Cmd_Argc() / Cmd_Argv()
qboolean CG_ConsoleCommand( void ) {
	const char *cmd = NULL;
	const command_t *command = NULL;

	if ( JPLua::Event_ConsoleCommand() ) {
		return qtrue;
	}

	cmd = CG_Argv( 0 );

	command = (command_t *)bsearch( cmd, commands, numCommands, sizeof(commands[0]), cmdcmp );
	if ( !command )
		return qfalse;

	if ( !command->func )
		return qfalse;

	command->func();
	return qtrue;
}

void CG_InitConsoleCommands( void ) {
	const command_t *cmd = commands;
	size_t i;

	for ( i = 0; i < numCommands; i++, cmd++ )
		trap->AddCommand( cmd->name );
}
