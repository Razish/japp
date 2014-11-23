#include "g_local.h"
#include "json/cJSON.h"
#include "qcommon/md5.h"

// session data is the only data that stays persistant across level loads and tournament restarts.

// called on game shutdown
void G_WriteClientSessionData( const gclient_t *client ) {
	const clientSession_t *sess = &client->sess;
	fileHandle_t f;
	char fileName[MAX_QPATH] = { '\0' };
	cJSON *root = cJSON_CreateObject();

	cJSON_AddIntegerToObject( root, "sessionTeam", sess->sessionTeam );
	cJSON_AddIntegerToObject( root, "spectatorTime", sess->spectatorTime );
	cJSON_AddIntegerToObject( root, "spectatorState", sess->spectatorState );
	cJSON_AddIntegerToObject( root, "spectatorClient", sess->spectatorClient );
	cJSON_AddIntegerToObject( root, "wins", sess->wins );
	cJSON_AddIntegerToObject( root, "losses", sess->losses );
	cJSON_AddIntegerToObject( root, "setForce", sess->setForce );
	cJSON_AddIntegerToObject( root, "saberLevel", sess->saberLevel );
	cJSON_AddIntegerToObject( root, "selectedFP", sess->selectedFP );
	cJSON_AddIntegerToObject( root, "duelTeam", sess->duelTeam );
	cJSON_AddIntegerToObject( root, "siegeDesiredTeam", sess->siegeDesiredTeam );
	cJSON_AddStringToObject( root, "siegeClass", *sess->siegeClass ? sess->siegeClass : "none" );
	cJSON_AddStringToObject( root, "IP", sess->IP );
	if ( client->pers.adminUser ) {
		char checksum[16];
		char combined[MAX_STRING_CHARS];
		Com_sprintf( combined, sizeof(combined), "%s%s",
			client->pers.adminUser->user, client->pers.adminUser->password );
		Q_ChecksumMD5( combined, strlen( combined ), checksum );
		trap->Print( "\nstoring %s as %s\n", combined, checksum );
		cJSON_AddStringToObject( root, "admin", checksum );
	}
	cJSON_AddBooleanToObject( root, "empowered", !!client->pers.adminData.empowered );
	cJSON_AddBooleanToObject( root, "merc", !!client->pers.adminData.merc );
	cJSON_AddBooleanToObject( root, "silenced", !!client->pers.adminData.silenced );
	cJSON_AddBooleanToObject( root, "slept", !!client->pers.adminData.isSlept );

	Com_sprintf( fileName, sizeof(fileName), "session/client%02i.json", client - level.clients );
	trap->FS_Open( fileName, &f, FS_WRITE );
	//Com_Printf( "Writing session file %s\n", fileName );

	Q_WriteJSONToFile( root, f );
}

// called on a reconnect
void G_ReadClientSessionData( gclient_t *client ) {
	clientSession_t *sess = &client->sess;
	cJSON *root = NULL, *object = NULL;
	char fileName[MAX_QPATH] = { '\0' };
	char *buffer = NULL;
	fileHandle_t f = NULL_FILE;
	unsigned int len = 0;
	const char *tmp = NULL;

	Com_sprintf( fileName, sizeof(fileName), "session/client%02i.json", client - level.clients );
	len = trap->FS_Open( fileName, &f, FS_READ );

	// no file
	if ( !f || !len || len == -1 ) {
		trap->FS_Close( f );
		return;
	}

	buffer = (char *)malloc( len + 1 );
	if ( !buffer ) {
		return;
	}

	trap->FS_Read( buffer, len, f );
	trap->FS_Close( f );
	buffer[len] = '\0';

	// read buffer
	root = cJSON_Parse( buffer );
	free( buffer );

	if ( !root ) {
		Com_Printf( "G_ReadSessionData(%02i): could not parse session data\n", client - level.clients );
		return;
	}

	if ( (object = cJSON_GetObjectItem( root, "sessionTeam" )) ) {
		sess->sessionTeam = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "spectatorTime" )) ) {
		sess->spectatorTime = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "spectatorState" )) ) {
		sess->spectatorState = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "spectatorClient" )) ) {
		sess->spectatorClient = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "wins" )) ) {
		sess->wins = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "losses" )) ) {
		sess->losses = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "setForce" )) ) {
		sess->setForce = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "saberLevel" )) ) {
		sess->saberLevel = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "selectedFP" )) ) {
		sess->selectedFP = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "duelTeam" )) ) {
		sess->duelTeam = cJSON_ToInteger( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "siegeDesiredTeam" )) ) {
		sess->siegeDesiredTeam = cJSON_ToInteger( object );
	}

	if ( (object = cJSON_GetObjectItem( root, "siegeClass" )) ) {
		if ( (tmp = cJSON_ToString( object )) ) {
			Q_strncpyz( sess->siegeClass, tmp, sizeof(sess->siegeClass) );
		}
	}
	if ( (object = cJSON_GetObjectItem( root, "IP" )) ) {
		if ( (tmp = cJSON_ToString( object )) ) {
			Q_strncpyz( sess->IP, tmp, sizeof(sess->IP) );
		}
	}
	if ( (object = cJSON_GetObjectItem( root, "admin" )) ) {
		if ( (tmp = cJSON_ToString( object )) ) {
			client->pers.adminUser = AM_ChecksumLogin( tmp );
		}
	}

	if ( (object = cJSON_GetObjectItem( root, "empowered" )) ) {
		client->pers.adminData.empowered = cJSON_ToBoolean( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "merc" )) ) {
		client->pers.adminData.merc = cJSON_ToBoolean( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "silenced" )) ) {
		client->pers.adminData.silenced = cJSON_ToBoolean( object );
	}
	if ( (object = cJSON_GetObjectItem( root, "slept" )) ) {
		client->pers.adminData.isSlept = cJSON_ToBoolean( object );
	}

	client->ps.fd.saberAnimLevel = sess->saberLevel;
	client->ps.fd.saberDrawAnimLevel = sess->saberLevel;
	client->ps.fd.forcePowerSelected = sess->selectedFP;

	cJSON_Delete( root );
	root = NULL;
}

// called on a first-time connect
void G_InitClientSessionData( gclient_t *client, char *userinfo, qboolean isBot ) {
	clientSession_t *sess = &client->sess;
	const char *value;

	client->sess.siegeDesiredTeam = TEAM_FREE;

	// initial team determination
	if ( level.gametype >= GT_TEAM ) {
		if ( g_teamAutoJoin.integer && !(g_entities[client - level.clients].r.svFlags & SVF_BOT) ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		}
		else {
			// always spawn as spectator in team games
			if ( !isBot ) {
				sess->sessionTeam = TEAM_SPECTATOR;
			}
			else {
				// bots choose their team on creation
				value = Info_ValueForKey( userinfo, "team" );
				if ( value[0] == 'r' || value[0] == 'R' ) {
					sess->sessionTeam = TEAM_RED;
				}
				else if ( value[0] == 'b' || value[0] == 'B' ) {
					sess->sessionTeam = TEAM_BLUE;
				}
				else {
					sess->sessionTeam = PickTeam( -1 );
				}
				BroadcastTeamChange( client, -1 );
			}
		}
	}
	else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		}
		else {
			switch ( level.gametype ) {
			default:
			case GT_FFA:
			case GT_HOLOCRON:
			case GT_JEDIMASTER:
			case GT_SINGLE_PLAYER:
				if ( g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else if ( g_teamAutoJoin.integer == 2 ) {
					// force joining in all gametypes
					sess->sessionTeam = TEAM_FREE;
				}
				else if ( !isBot ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else {
					// bots automatically join the game
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_DUEL:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_POWERDUEL:
				{
					int loners = 0, doubles = 0;

					G_PowerDuelCount( &loners, &doubles, qtrue );

					if ( !doubles || loners > (doubles / 2) ) {
						sess->duelTeam = DUELTEAM_DOUBLE;
					}
					else {
						sess->duelTeam = DUELTEAM_LONE;
					}
					sess->sessionTeam = TEAM_SPECTATOR;
				}
				break;
			}
		}
	}

	if ( sess->sessionTeam == TEAM_SPECTATOR ) {
		sess->spectatorState = SPECTATOR_FREE;
	}
	else {
		sess->spectatorState = SPECTATOR_NOT;
	}

	sess->spectatorTime = level.time;
	sess->siegeClass[0] = '\0';

	G_WriteClientSessionData( client );
}

static const char *metaFileName = "session/meta.json";

void G_ReadSessionData( void ) {
	char *buffer = NULL;
	fileHandle_t f = NULL_FILE;
	unsigned int len = 0u;
	cJSON *root;

	trap->Print( "G_ReadSessionData: reading %s...", metaFileName );
	len = trap->FS_Open( metaFileName, &f, FS_READ );

	// no file
	if ( !f || !len || len == -1 ) {
		trap->Print( "failed to open file, clearing session data...\n" );
		level.newSession = qtrue;
		return;
	}

	buffer = (char *)malloc( len + 1 );
	if ( !buffer ) {
		trap->Print( "failed to allocate buffer, clearing session data...\n" );
		level.newSession = qtrue;
		return;
	}

	trap->FS_Read( buffer, len, f );
	trap->FS_Close( f );
	buffer[len] = '\0';

	// read buffer
	root = cJSON_Parse( buffer );

	// if the gametype changed since the last session, don't use any client sessions
	if ( level.gametype != cJSON_ToInteger( cJSON_GetObjectItem( root, "gametype" ) ) ) {
		level.newSession = qtrue;
		trap->Print( "gametype changed, clearing session data..." );
	}

	free( buffer );
	cJSON_Delete( root );
	root = NULL;
	trap->Print( "done\n" );
}

void G_WriteSessionData( void ) {
	int i;
	fileHandle_t f;
	const gclient_t *client = NULL;
	cJSON *root = cJSON_CreateObject();

	cJSON_AddIntegerToObject( root, "gametype", level.gametype );

	trap->Print( "G_WriteSessionData: writing %s...", metaFileName );
	trap->FS_Open( metaFileName, &f, FS_WRITE );

	Q_WriteJSONToFile( root, f );

	for ( i = 0, client = level.clients; i < level.maxclients; i++, client++ ) {
		if ( client->pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( client );
		}
	}

	trap->Print( "done\n" );
}
