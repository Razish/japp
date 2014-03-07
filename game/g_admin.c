// JA++ administration system
//
// This administration system is developed to be flexible, easy to maintain, secure and simple.
//
// There is a list of user/pass combinations to be defined by the server owner, with each user having their own
// permission mask.
//
// All checking of permissions is done before the associated function is called, so you don't have to worry about
// complex control paths
//
// Admin data is saved in JSON format in <fs_homepath>/<fs_game>/admins.json
//

#include "g_local.h"
#include "g_admin.h"
#include "json/cJSON.h"
#include "bg_lua.h"

static adminUser_t *adminUsers = NULL;
static telemark_t *telemarks = NULL;
static qboolean telemarksVisible = qfalse;

// clear all admin accounts and logout all users
static void AM_ClearAccounts( void ) {
	adminUser_t *user = adminUsers;

	while ( user ) {
		adminUser_t *next = user->next;
		gentity_t *ent = NULL;
		int i = 0;

		for ( i=0, ent=g_entities; i<level.maxclients; i++, ent++ ) {
			if ( ent->client->pers.adminUser && ent->client->pers.adminUser == user ) {
				trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"You have been forcefully logged out of admin.\n\"" );
				ent->client->pers.adminUser = NULL;
			}
		}

		free( user );
		user = next;
	}

	adminUsers = NULL;
}

// add or update an existing admin account (user, pass, privileges, login mesage)
void AM_AddAdmin( const char *user, const char *pass, uint32_t privileges, const int rank, const char *loginMsg  ) {
	adminUser_t	*admin = NULL;

	for ( admin=adminUsers; admin; admin=admin->next ) {
		if ( !strcmp( user, admin->user ) ) {
			trap->Print( "Overwriting existing admin: %s/%s:%d (%d) %s\n", admin->user, admin->password, admin->rank, admin->privileges,
				admin->loginMsg );
			break;
		}
	}

	if ( !admin ) {
		// a new admin, insert it to the start of the linked list, user->next will be the old root
		admin = malloc( sizeof( adminUser_t ) );
		memset( admin, 0, sizeof( adminUser_t ) );
		admin->next = adminUsers;
		adminUsers = admin;
	}

	// we're either overwriting an admin, or adding a new one
	Q_strncpyz( admin->user, user, sizeof( admin->user ) );
	Q_strncpyz( admin->password, pass, sizeof( admin->password ) );
	admin->privileges = privileges;
	admin->rank = rank;
	Q_strncpyz( admin->loginMsg, loginMsg, sizeof( admin->loginMsg ) );
}

// delete an admin account and forcefully log out any users
void AM_DeleteAdmin( const char *user ) {
	adminUser_t	*admin = NULL, *prev=NULL, *next=NULL;

	for ( admin=adminUsers; admin; admin=admin->next ) {
		next = admin->next;

		if ( !strcmp( user, admin->user ) ) {
			gentity_t *ent = NULL;
			int i = 0;
			for ( i=0, ent=g_entities; i<level.maxclients; i++, ent++ ) {
				if ( ent->client->pers.adminUser && ent->client->pers.adminUser == admin ) {
					trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_RED"Your admin account has been deleted.\n\"" );
					ent->client->pers.adminUser = NULL;
				}
			}

			trap->Print( "Deleting admin account: %s\n", user );
			free( admin );
			if ( prev )
				prev->next = next;

			// root node
			if ( admin == adminUsers )
				adminUsers = next;

			return;
		}

		prev = admin;
	}

	trap->Print( "No such admin found (%s)\n", user );
}

// list all admin accounts and users logged into them
void AM_ListAdmins( void ) {
	adminUser_t	*admin = NULL;
	int count = 0;

	trap->Print( "Listing admin accounts:\n" );

	for ( admin=adminUsers; admin; admin=admin->next ) {
		gentity_t *ent = NULL;

		trap->Print( " %3d: %s/%s:%d (%d) %s\n", ++count, admin->user, admin->password, admin->rank, admin->privileges, admin->loginMsg );

		for ( ent=g_entities; ent-g_entities<level.maxclients; ent++ ) {
			//TODO: build string like "user1, user2"
			if ( ent->client->pers.adminUser && ent->client->pers.adminUser == admin )
				trap->Print( "      Logged in: %s\n", ent->client->pers.netname );
		}
	}
}

// parse json object for admin accounts
static void AM_ReadAccounts( const char *jsonText ) {
	cJSON *root = NULL, *admins = NULL;
	int adminsCount = 0, i = 0;
	const char *tmp = NULL;
	adminUser_t	*user = NULL;

	root = cJSON_Parse( jsonText );
	if ( !root ) {
		Com_Printf( "ERROR: Could not parse admin info\n" );
		return;
	}

	admins = cJSON_GetObjectItem( root, "admins" );
	adminsCount = cJSON_GetArraySize( admins );

	for ( i=0; i<adminsCount; i++ ) {
		cJSON *item = cJSON_GetArrayItem( admins, i );

		// first, allocate the admin user
		// insert it to the start of the linked list, user->next will be the old root
		user = (adminUser_t *)malloc( sizeof( adminUser_t ) );
		memset( user, 0, sizeof( adminUser_t ) );
		user->next = adminUsers;
		adminUsers = user;

		// user
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "user" ) )) )
			Q_strncpyz( user->user, tmp, sizeof( user->user ) );

		// pass
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "pass" ) )) )
			Q_strncpyz( user->password, tmp, sizeof( user->password ) );

		// privs
		user->privileges = cJSON_ToInteger( cJSON_GetObjectItem( item, "privs" ) );

		// rank
		user->rank = cJSON_ToInteger( cJSON_GetObjectItem( item, "rank" ) );

		// login message
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "message" ) )) )
			Q_strncpyz( user->loginMsg, tmp, sizeof( user->loginMsg ) );
	}
}

// create json object for admin accounts and write to file
static void AM_WriteAccounts( fileHandle_t f ) {
	cJSON *root = NULL, *admins = NULL;
	adminUser_t *admin = NULL;

	root = cJSON_CreateObject();
	admins = cJSON_CreateArray();
	for ( admin=adminUsers; admin; admin=admin->next ) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject( item, "user", admin->user );
		cJSON_AddStringToObject( item, "pass", admin->password );
		cJSON_AddIntegerToObject( item, "privs", admin->privileges );
		cJSON_AddIntegerToObject( item, "rank", admin->rank );
		cJSON_AddStringToObject( item, "message", admin->loginMsg );

		cJSON_AddItemToArray( admins, item );
	}
	cJSON_AddItemToObject( root, "admins", admins );

	Q_WriteJSONToFile( root, f );
}

#define ADMIN_FILE "admins.json"
// load admin accounts from disk after logging everyone out
void AM_LoadAdmins( void ) {
	char *buf = NULL;
	unsigned int len = 0;
	fileHandle_t f = 0;

	AM_ClearAccounts();

	len = trap->FS_Open( ADMIN_FILE, &f, FS_READ );
	Com_Printf( "Loading admin accounts (" ADMIN_FILE ")\n" );

	// no file
	if ( !f )
		return;

	// empty file
	if ( !len || len == -1 ) {
		trap->FS_Close( f );
		return;
	}

	// alloc memory for buffer
	if ( !(buf = (char*)malloc(len+1)) )
		return;

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = 0;

	// pass it off to the json reader
	AM_ReadAccounts( buf );

	free( buf );
}

// save admin accounts to disk
void AM_SaveAdmins( void ) {
	fileHandle_t f;

	trap->FS_Open( ADMIN_FILE, &f, FS_WRITE );
	Com_Printf( "Saving admins (" ADMIN_FILE ")\n" );

	AM_WriteAccounts( f );
}

// returns qtrue if inflicter is higher on the rank hierarchy, and qtrue on equal if japp_passRankConflicts is 1.
//	otherwise, returns qfalse and prints a message to the inflicter to warn them.
qboolean AM_CanInflict( gentity_t *entInflicter, gentity_t *entVictim ) {
	adminUser_t *inflicter = entInflicter->client->pers.adminUser;
	adminUser_t *victim = entVictim->client->pers.adminUser;

	// if either one is not an admin, they lose by default.
	if ( !victim )
		return qtrue; // victim isn't an admin.

	if ( entInflicter == entVictim )
		return qtrue; // you can abuse yourself, of course.

	if( inflicter->rank == victim->rank ) {
		if ( japp_passRankConflicts.integer ) {
			G_LogPrintf( level.log.console, "%s (User: %s) (Rank: %d) inflicting command on lower ranked player %s"
				"(User: %s) (Rank: %d)", entInflicter->client->pers.netname, inflicter->user, inflicter->rank,
				entVictim->client->pers.netname, victim->user, victim->rank );
			return qtrue;
		}
		else {
			trap->SendServerCommand( entInflicter->s.number, "print \""S_COLOR_RED"Can not use admin commands on those of"
				"an equal rank: (japp_passRankConflicts)\n\"");
			return qfalse;
		}
	}

	if ( inflicter->rank > victim->rank ) {
		// inflicter is of a higher rank and so he/she can freely abuse those lesser.
		G_LogPrintf( level.log.console, "%s (User: %s) (Rank: %d) inflicting command on lower ranked player %s (User: %s)"
			"(Rank: %d)", entInflicter->client->pers.netname, inflicter->user, inflicter->rank, entVictim->client->pers.netname,
			victim->user, victim->rank );
		return qtrue;
	}
	else {
		trap->SendServerCommand( entInflicter->s.number, "print \""S_COLOR_RED"Can not use admin commands on those of a"
			"higher rank.\n\"" );
		return qfalse;
	}
}

static telemark_t *FindTelemark( const char *name ) {
	telemark_t *tm = NULL;
	for ( tm=telemarks; tm; tm=tm->next ) {
		char cleanedName[MAX_TELEMARK_NAME_LEN];

		Q_strncpyz( cleanedName, tm->name, sizeof( cleanedName ) );
		Q_CleanString( cleanedName, STRIP_COLOUR );

		if ( !Q_stricmp( cleanedName, name ) )
			return tm;
	}

	return NULL;
}

// clear all telemarks
static void AM_ClearTelemarks( void ) {
	telemark_t *tm = telemarks;

	while ( tm ) {
		telemark_t *next = tm->next;
		free( tm );
		tm = next;
	}

	telemarks = NULL;
}


void SP_fx_runner( gentity_t *ent );
static void SpawnTelemark( telemark_t *tm, vector3 *position ) {
	if ( telemarksVisible ) {
		tm->ent = G_Spawn();
		tm->ent->fullName = "env/fire_wall";
		VectorCopy( position, &tm->ent->s.origin );
		SP_fx_runner( tm->ent );
	}
}

// add or update an existing telemark
static telemark_t *AM_AddTelemark( const char *name, vector3 *position ) {
	telemark_t *tm = NULL;

	for ( tm=telemarks; tm; tm=tm->next ) {
		if ( !Q_stricmp( name, tm->name ) ) {
			trap->Print( "Overwriting existing telemark: %s %s\n", tm->name, vtos( position ) );
			break;
		}
	}

	if ( !tm ) {
		// a new telemark, insert it to the start of the linked list, tm->next will be the old root
		tm = malloc( sizeof( telemark_t ) );
		memset( tm, 0, sizeof( telemark_t ) );
		tm->next = telemarks;
		telemarks = tm;
	}

	// we're either overwriting a telemark, or adding a new one
	Q_strncpyz( tm->name, name, sizeof( tm->name ) );
	VectorCopy( position, &tm->position );
	SpawnTelemark( tm, position );

	return tm;
}

// delete a telemark
static void AM_DeleteTelemark( gentity_t *ent, const char *name ) {
	telemark_t *tm = NULL, *prev=NULL, *next=NULL;

	for ( tm=telemarks; tm; tm=tm->next ) {
		next = tm->next;

		if ( !Q_stricmp( name, tm->name ) ) {
			trap->SendServerCommand( ent-g_entities, va( "print \"Deleting telemark '%s'\n\"", name ) );

			if ( telemarksVisible )
				G_FreeEntity( tm->ent );

			free( tm );
			if ( prev )
				prev->next = next;

			// root node
			if ( tm == telemarks )
				telemarks = next;

			return;
		}

		prev = tm;
	}

	trap->SendServerCommand( ent-g_entities, va( "print \"No such telemark found (%s)\n\"", name ) );
}

// parse json object for telemarks
static void AM_ReadTelemarks( const char *jsonText ) {
	cJSON *root = NULL, *tms = NULL;
	int tmCount = 0, i = 0, tmpInt = 0;
	const char *tmp = NULL;
	telemark_t *tm = NULL;

	root = cJSON_Parse( jsonText );
	if ( !root ) {
		Com_Printf( "ERROR: Could not parse telemarks\n" );
		return;
	}

	tms = cJSON_GetObjectItem( root, "telemarks" );
	tmCount = cJSON_GetArraySize( tms );

	for ( i=0; i<tmCount; i++ ) {
		cJSON *item = cJSON_GetArrayItem( tms, i );

		// first, allocate the telemark
		// insert it to the start of the linked list, tm->next will be the old root
		tm = (telemark_t *)malloc( sizeof( telemark_t ) );
		memset( tm, 0, sizeof( telemark_t ) );
		tm->next = telemarks;
		telemarks = tm;

		// name
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "name" ) )) )
			Q_strncpyz( tm->name, tmp, sizeof( tm->name ) );

		// position
		tmpInt = cJSON_ToInteger( cJSON_GetObjectItem( item, "x" ) );
		tm->position.x = tmpInt;
		tmpInt = cJSON_ToInteger( cJSON_GetObjectItem( item, "y" ) );
		tm->position.y = tmpInt;
		tmpInt = cJSON_ToInteger( cJSON_GetObjectItem( item, "z" ) );
		tm->position.z = tmpInt;
	}
}

// create json object for telemarks and write to file
static void AM_WriteTelemarks( fileHandle_t f ) {
	cJSON *root = NULL, *tms = NULL;
	telemark_t *tm = NULL;

	root = cJSON_CreateObject();
	tms = cJSON_CreateArray();
	for ( tm=telemarks; tm; tm=tm->next ) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject( item, "name", tm->name );
		cJSON_AddIntegerToObject( item, "x", tm->position.x );
		cJSON_AddIntegerToObject( item, "y", tm->position.y );
		cJSON_AddIntegerToObject( item, "z", tm->position.z );

		cJSON_AddItemToArray( tms, item );
	}
	cJSON_AddItemToObject( root, "telemarks", tms );

	Q_WriteJSONToFile( root, f );
}

// load telemarks from disk
void AM_LoadTelemarks( void ) {
	char *buf = NULL, loadPath[MAX_QPATH] = {0};
	unsigned int len = 0;
	fileHandle_t f = 0;

	AM_ClearTelemarks();

	Com_sprintf( loadPath, sizeof( loadPath ), "telemarks" PATH_SEP "%s.json", level.rawmapname );
	len = trap->FS_Open( loadPath, &f, FS_READ );
	Com_Printf( "Loading telemarks (%s)\n", loadPath );

	// no file
	if ( !f )
		return;

	// empty file
	if ( !len || len == -1 ) {
		trap->FS_Close( f );
		return;
	}

	// alloc memory for buffer
	if ( !(buf = (char*)malloc(len+1)) )
		return;

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = 0;

	// pass it off to the json reader
	AM_ReadTelemarks( buf );

	free( buf );
}

// save telemarks to disk
void AM_SaveTelemarks( void ) {
	char loadPath[MAX_QPATH] = {0};
	fileHandle_t f;

	Com_sprintf( loadPath, sizeof( loadPath ), "telemarks" PATH_SEP "%s.json", level.rawmapname );
	trap->FS_Open( loadPath, &f, FS_WRITE );
	Com_Printf( "Saving telemarks (%s)\n", loadPath );

	AM_WriteTelemarks( f );
}

// log in using user + pass
static void AM_Login( gentity_t *ent ) {
	char argUser[64] = {0}, argPass[64] = {0};
	adminUser_t *user = NULL, *current = NULL;

	if ( trap->Argc() < 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please enter a name and password to login\n\"" );
		return;
	}

	//Grab user + pass
	trap->Argv( 1, argUser, sizeof( argUser ) );
	trap->Argv( 2, argPass, sizeof( argPass ) );

	// find valid user
	for ( current=adminUsers; current; current=current->next ) {
		if ( !strcmp( current->user, argUser ) ) {
			// case sensitive password
			if ( !strcmp( current->password, argPass ) ) {
				ent->client->pers.adminUser = user = current;
				break;
			}
		}
	}

	if ( user ) {
		char *loginMsg = ent->client->pers.adminUser->loginMsg;
		char *sendMsg = NULL, *tmp = NULL;

		if ( !VALIDSTRING( loginMsg ) ) {
			trap->SendServerCommand( ent-g_entities, "print \"You have logged in\n\"" );
			return;
		}

		sendMsg = Q_strrep( ent->client->pers.adminUser->loginMsg, "$name", ent->client->pers.netname );
		if ( (tmp=Q_strrep( sendMsg, "\\n", "\n" )) ) {
			free( sendMsg );
			sendMsg = tmp;
			tmp = NULL;
		}
		trap->SendServerCommand( -1, va( "print \"%s\n\"", sendMsg ) );
		free( sendMsg );
	}
	else
		trap->SendServerCommand( ent-g_entities, "print \"Invalid login\n\"" );
}

// logout
static void AM_Logout( gentity_t *ent ) {
	ent->client->pers.adminUser = NULL;
	trap->SendServerCommand( ent-g_entities, "print \"You have logged out\n\"" );
}

// display list of admins
static void AM_WhoIs( gentity_t *ent ) {
	int i;
	char msg[1024-128] = { 0 };
	gclient_t *cl;

	Q_strcat( msg, sizeof( msg ), S_COLOR_WHITE"Name                                Admin User                      Rank\n" );

	// if the client is an admin, append their name and 'user' to the string
	for ( i=0; i<MAX_CLIENTS; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.adminUser ) {
			char strName[MAX_NETNAME] = {0}, strAdmin[32] = {0}, strRank[12] = {0};

			Q_strncpyz( strName, cl->pers.netname, sizeof( strName ) );
			Q_CleanString( strName, STRIP_COLOUR );
			Q_strncpyz( strAdmin, (cl->pers.adminUser) ? cl->pers.adminUser->user : "", sizeof( strAdmin ) );
			Q_CleanString( strAdmin, STRIP_COLOUR );
			Q_strncpyz( strRank, va( "%d", cl->pers.adminUser->rank ), sizeof( strRank ) );

			Q_strcat( msg, sizeof( msg ), va( "%-36s%-32s%-12s\n", strName, strAdmin, strRank ) );
		}
	}

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}

// display list of players + clientNum + IP + admin
static void AM_Status( gentity_t *ent ) {
	int i;
	char msg[1024-128] = {0};
	gclient_t *cl;

	Q_strcat( msg, sizeof( msg ), S_COLOR_WHITE"clientNum   Name                                IP                      Admin User\n" );

	// build a list of clients
	for ( i=0; i<MAX_CLIENTS; i++ ) {
		const char *tmpMsg = NULL;
		if ( !g_entities[i].inuse )
			continue;

		cl = &level.clients[i];
		if ( cl->pers.netname[0] ) {
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strIP[NET_ADDRSTRMAXLEN] = {0};
			char strAdmin[32] = {0};

			Q_strncpyz( strNum, va( "(%i)", i ), sizeof( strNum ) );
			Q_strncpyz( strName, cl->pers.netname, sizeof( strName ) );
			Q_CleanString( strName, STRIP_COLOUR );
			Q_strncpyz( strIP, cl->sess.IP, sizeof( strIP ) );
			Q_strncpyz( strAdmin, (cl->pers.adminUser) ? cl->pers.adminUser->user : "", sizeof( strAdmin ) );
			Q_CleanString( strAdmin, STRIP_COLOUR );

			tmpMsg = va( "%-12s%-36s%-24s%-32s\n", strNum, strName, strIP, strAdmin );

			if ( strlen( msg ) + strlen( tmpMsg ) >= sizeof( msg ) ) {
				trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
				msg[0] = '\0';
			}
			Q_strcat( msg, sizeof( msg ), tmpMsg );
		}
	}

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}

// announce a message to all clients
static void AM_Announce( gentity_t *ent ) {
	char *msg, arg1[48];
	int targetClient;

	if ( trap->Argc() < 3 ) {
		trap->SendServerCommand( ent-g_entities, va( "print \"Usage: \\ampsay <client> <message>\n\"" ) );
		return;
	}

	msg = ConcatArgs( 2 );
	Q_ConvertLinefeeds( msg );

	//Grab the clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );

	//Check for purposely announcing to all. HACKHACKHACK
	if ( arg1[0] == '-' && arg1[1] == '1' )
		targetClient = -2;

	// Invalid player
	if ( targetClient == -1 )
		return;

	// print to everyone
	else if ( targetClient == -2 )
		trap->SendServerCommand( -1, va( "cp \"%s\"", msg ) );

	// valid client
	else {
		trap->SendServerCommand( targetClient, va( "cp \"%s\"", msg ) );
		trap->SendServerCommand( ent-g_entities, va( "cp \"Relay:\n%s\"", msg ) ); //Helena wanted me to relay it back to the sender
	}

	G_LogPrintf( level.log.console, "AM_Announce: Start\nSender: %s\nMessage: %s\nAM_Announce: End\n", ent->client->pers.netname, msg );

	return;
}

extern void WP_AddToClientBitflags( gentity_t *ent, int entNum );
// ghost specified client (or self)
static void AM_Ghost( gentity_t *ent ) {
	char arg1[64];
	int targetClient;
	gentity_t *targ;

	//Self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT ) : ent-g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	if( !AM_CanInflict( ent, targ ) )
		return;

	if ( targ->client->pers.adminData.isGhost ) {
		targ->client->pers.adminData.isGhost = qfalse;
		targ->r.contents = CONTENTS_SOLID;
		targ->clipmask = CONTENTS_SOLID|CONTENTS_BODY;
		targ->s.trickedentindex = 0; targ->s.trickedentindex2 = 0; //This is entirely for client-side prediction. Please fix me.
		trap->SendServerCommand( targetClient, "cp \""S_COLOR_CYAN"Unghosted\n\"" );
	}
	else {
		targ->client->pers.adminData.isGhost = qtrue;
		targ->r.contents = CONTENTS_BODY;
		targ->clipmask = 267009/*CONTENTS_SOLID*/;

		//This is *entirely* for client-side prediction. Do not rely on it. Please fix me.
		targ->client->ps.fd.forceMindtrickTargetIndex = ~(1<<targetClient);
		targ->client->ps.fd.forceMindtrickTargetIndex2 = ~(1<<targetClient);

		trap->SendServerCommand( targetClient, "cp \"You are now a "S_COLOR_CYAN"ghost\n\"" );
	}
}

// toggle noclip mode
static void AM_Clip( gentity_t *ent ) {
	ent->client->noclip = !ent->client->noclip;
	trap->SendServerCommand( ent-g_entities, va( "print \"Noclip %s\n\"", ent->client->noclip?S_COLOR_GREEN"on":S_COLOR_RED"off" ) );
}

// teleport (all variations of x to y)
static void AM_Teleport( gentity_t *ent ) {
	//No args means we teleport ourself to our last marked coordinates
	if ( trap->Argc() == 1 && ent->client->pers.adminData.telemark )
		TeleportPlayer( ent, &ent->client->pers.adminData.telemark->position, &ent->client->ps.viewangles );

	// 1 arg means we're teleporting ourself to x (self -> client, self -> namedTelePos)
	else if ( trap->Argc() == 2 ) {
		char arg1[64];
		int targetClient;

		trap->Argv( 1, arg1, sizeof( arg1 ) );
		targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR );

		// no client with this name, check for named teleport
		if ( targetClient == -1 ) {
			char cleanedInput[MAX_TELEMARK_NAME_LEN];
			telemark_t *tm = NULL;

			Q_strncpyz( cleanedInput, arg1, sizeof( cleanedInput ) );
			Q_CleanString( cleanedInput, STRIP_COLOUR );

			tm = FindTelemark( cleanedInput );
			if ( tm ) {
				TeleportPlayer( ent, &tm->position, &ent->client->ps.viewangles );
				return;
			}

			trap->SendServerCommand( ent-g_entities, "print \"AM_Teleport: Assumed telemark but found no matching telemark\n\"" );
		}
		// must be teleporting self -> client
		else if ( g_entities[targetClient].inuse ) {
			vector3 targetPos, telePos, angles;
			trace_t tr;

			VectorCopy( &level.clients[targetClient].ps.origin, &targetPos );
			VectorCopy( &level.clients[targetClient].ps.viewangles, &angles );
			angles.pitch = angles.roll = 0.0f;
			AngleVectors( &angles, &angles, NULL, NULL );

			VectorMA( &targetPos, 64.0f, &angles, &telePos );
			trap->Trace( &tr, &targetPos, NULL, NULL, &telePos, targetClient, CONTENTS_SOLID, qfalse, 0, 0 );
			if ( tr.fraction < 1.0f )
				VectorMA( &tr.endpos, 32.0f, &tr.plane.normal, &telePos );
			else
				VectorCopy( &tr.endpos, &telePos );

			TeleportPlayer( ent, &telePos, &ent->client->ps.viewangles );
		}
	}

	// 2 args mean we're teleporting x to y (client -> client, client -> telemark)
	else if ( trap->Argc() == 3 ) {
		char arg1[64], arg2[64];
		int targetClient1, targetClient2;



		trap->Argv( 1, arg1, sizeof( arg1 ) );	targetClient1 = G_ClientFromString( ent, arg1, FINDCL_SUBSTR );
		trap->Argv( 2, arg2, sizeof( arg2 ) );	targetClient2 = G_ClientFromString( ent, arg2, FINDCL_SUBSTR );

			if( !AM_CanInflict( ent, &g_entities[targetClient1] ) )
		return;

		// first arg is a valid client, attempt to find destination
		if ( targetClient1 != -1 ) {
			// no client with this name - check for telemark
			if ( targetClient2 == -1 ) {
				char cleanedInput[MAX_TELEMARK_NAME_LEN];
				telemark_t *tm = NULL;

				Q_strncpyz( cleanedInput, arg2, sizeof( cleanedInput ) );
				Q_CleanString( cleanedInput, STRIP_COLOUR );

				tm = FindTelemark( cleanedInput );
				if ( tm ) {
					TeleportPlayer( &g_entities[targetClient1], &tm->position, &ent->client->ps.viewangles );
					return;
				}

				trap->SendServerCommand( ent-g_entities, "print \"AM_Teleport: Assumed telemark but found no matching telemark\n\"" );
			}

			// must be teleporting client -> client
			else {
				vector3 targetPos, telePos, angles;
				trace_t tr;

				VectorCopy( &level.clients[targetClient2].ps.origin, &targetPos );
				VectorCopy( &level.clients[targetClient2].ps.viewangles, &angles );
				angles.pitch = angles.roll = 0.0f;
				AngleVectors( &angles, &angles, NULL, NULL );

				VectorMA( &targetPos, 64.0f, &angles, &telePos );
				trap->Trace( &tr, &targetPos, NULL, NULL, &telePos, targetClient2, CONTENTS_SOLID, qfalse, 0, 0 );
				if ( tr.fraction < 1.0f )
					VectorMA( &tr.endpos, 32.0f, &tr.plane.normal, &telePos );
				else
					VectorCopy( &tr.endpos, &telePos );

				TeleportPlayer( &g_entities[targetClient1], &telePos, &g_entities[targetClient1].client->ps.viewangles );
			}
		}
	}

	// amtele x y z - tele ourself to x y z
	else if ( trap->Argc() == 4 ) {
		char argX[8], argY[8], argZ[8];
		vector3 telePos;

		trap->Argv( 1, argX, sizeof( argX ) );	trap->Argv( 2, argY, sizeof( argX ) );	trap->Argv( 3, argZ, sizeof( argX ) );

		VectorSet( &telePos, atoff( argX ), atof( argY ), atof( argZ ) );
		TeleportPlayer( ent, &telePos, &ent->client->ps.viewangles );
	}

	// amtele c x y z - tele c to x y z
	// amtele c x y z r - tele c to x y z with r
	else if ( trap->Argc() > 4 ) {
		char argC[64];
		int targetClient;

		trap->Argv( 1, argC, sizeof( argC ) );
		targetClient = G_ClientFromString( ent, argC, FINDCL_SUBSTR|FINDCL_PRINT );

		if ( targetClient == -1 )
			return;
		else {
			vector3	telePos;
			char argX[16]={0}, argY[16]={0}, argZ[16]={0};

			trap->Argv( 2, argX, sizeof( argX ) );
			trap->Argv( 3, argY, sizeof( argY ) );
			trap->Argv( 4, argZ, sizeof( argZ ) );

			VectorCopy( tv( atoi(argX), atoi(argY), atoi(argZ) ), &telePos );
			// amtele c x y z r
			if ( trap->Argc() == 6 ) {
				vector3 angles = { 0.0f };
				char argR[8]={0};

				trap->Argv( 5, argR, sizeof( argR ) );
				angles.yaw = atoi( argR );
				TeleportPlayer( &g_entities[targetClient], &telePos, &angles );
			}
			// amtele c x y z
			else
				TeleportPlayer( &g_entities[targetClient], &telePos, &g_entities[targetClient].client->ps.viewangles );
		}

	}
}

// teleport self to targeted position
static void AM_GunTeleport( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );
	vector3 telepos;

	VectorMA( &tr->endpos, 48.0f, &tr->plane.normal, &telepos );
	TeleportPlayer( ent, &telepos, &ent->client->ps.viewangles );
}

// teleport targeted client to self
static void AM_GunTeleportRev( gentity_t *ent ) {
	trace_t	*tr = G_RealTrace( ent, 0.0f );
	vector3	angles, telepos;

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS ) {
		AngleVectors( &ent->client->ps.viewangles, &angles, NULL, NULL );
		VectorMA( &ent->client->ps.origin, 48.0f, &angles, &telepos );

		if( !AM_CanInflict( ent, &g_entities[tr->entityNum] ) )
			return;

		TeleportPlayer( &g_entities[tr->entityNum], &telepos, &level.clients[tr->entityNum].ps.viewangles );
	}
}

// mark current location
static void AM_Telemark( gentity_t *ent ) {
	char name[MAX_TELEMARK_NAME_LEN];

	if ( trap->Argc() > 1 )
		Q_strncpyz( name, ConcatArgs( 1 ), sizeof( name ) );
	else
		Com_sprintf( name, sizeof( name ), "default_%s", ent->client->pers.netnameClean );

	ent->client->pers.adminData.telemark = AM_AddTelemark( name, &ent->client->ps.origin );
}

// mark targeted location
static void AM_GunTeleportMark( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );
	vector3 telePos;
	gentity_t *tent;

	VectorMA( &tr->endpos, 48.0f, &tr->plane.normal, &telePos );

	ent->client->pers.adminData.telemark = AM_AddTelemark( va( "default_%s", ent->client->pers.netnameClean ), &telePos );
	tent = G_PlayEffect( EFFECT_EXPLOSION, &tr->endpos, &tr->plane.normal );
	tent->r.svFlags |= SVF_SINGLECLIENT;
	tent->r.singleClient = ent->s.number;
	trap->SendServerCommand( ent-g_entities, va( "print \""S_COLOR_CYAN"Teleport mark created at "S_COLOR_YELLOW"%s\n\"", vtos( &telePos ) ) );
}

// remove a telemark from the list
static void AM_RemoveTelemark( gentity_t *ent ) {
	char arg1[MAX_TELEMARK_NAME_LEN];

	if ( trap->Argc() == 1 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: \\amremovetele <name>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );
	Q_CleanString( arg1, STRIP_COLOUR );

	AM_DeleteTelemark( ent, arg1 );
}

// list all marked positions
static void AM_ListTelemarks( gentity_t *ent ) {
	char msg[3096] = {0};
	telemark_t *tm = NULL;
	int i = 0;

	// append each mark to the end of the string
	Q_strcat( msg, sizeof( msg ), "- Named telemarks\n" );
	Q_strcat( msg, sizeof( msg ), va( "ID %-32s Location\n", "Name" ) );
	for ( tm=telemarks, i=0; tm; tm=tm->next, i++ )
		Q_strcat( msg, sizeof( msg ), va( "%2i %-32s %s\n", i, tm->name, vtos( &tm->position ) ) );

	// send in one big chunk
	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}

// visualise all telemarks
static void AM_SeeTelemarks( gentity_t *ent ) {
	telemark_t *tm = NULL;

	// assume all telemarks have valid ent ptr's
	if ( telemarksVisible ) {
		for ( tm=telemarks; tm; tm=tm->next ) {
			G_FreeEntity( tm->ent );
			tm->ent = NULL;
		}
	}
	else {
		for ( tm=telemarks; tm; tm=tm->next ) {
			tm->ent = G_Spawn();
			tm->ent->fullName = "env/btend";
			VectorCopy( &tm->position, &tm->ent->s.origin );
			SP_fx_runner( tm->ent );
		}
	}
	telemarksVisible = !telemarksVisible;
}

static void AM_SaveTelemarksCmd( gentity_t *ent ) {
	AM_SaveTelemarks();
}

// call an arbitrary vote
static void AM_Poll( gentity_t *ent ) {
	int i=0;
	char arg1[MAX_TOKEN_CHARS] = {0}, arg2[MAX_TOKEN_CHARS] = {0};

	if ( level.voteExecuteTime ) {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Vote already in progress.\n\"" );
		return;
	}

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Please specify a poll.\n\"" );
		return;
	}

	trap->Argv( 0, arg1, sizeof( arg1 ) );
	//Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) );
	Q_strncpyz( arg2, ent->client->pers.netname, sizeof( arg2 ) );
	Q_CleanString( arg2, STRIP_COLOUR );
	Q_strstrip( arg2, "\n\r;\"", NULL );

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap->SendServerCommand( -1, va("print \"%s"S_COLOR_WHITE" %s\n\"", ent->client->pers.netname, G_GetStringEdString(
		"MP_SVGAME", "PLCALLEDVOTE" ) ) );
	level.voteExecuteDelay = japp_voteDelay.integer;
	level.voteTime = level.time;
	level.voteYes = 0;
	level.voteNo = 0;
	level.votePoll = qtrue;

	for ( i=0; i<level.maxclients; i++ )
		level.clients[i].mGameFlags &= ~PSG_VOTED;

	trap->SetConfigstring( CS_VOTE_TIME,		va( "%i", level.voteTime ) );
	trap->SetConfigstring( CS_VOTE_STRING,	level.voteDisplayString );
	trap->SetConfigstring( CS_VOTE_YES,		va( "%i", level.voteYes ) );
	trap->SetConfigstring( CS_VOTE_NO,		va( "%i", level.voteNo ) );
}

// kill the current vote
static void AM_KillVote( gentity_t *ent ) {
	//Overkill, but it's a surefire way to kill the vote =]
	level.voteExecuteTime	= 0;
	level.votingGametype	= qfalse;
	level.votingGametypeTo	= level.gametype;
	level.voteTime			= 0;

	level.voteDisplayString[0] = '\0';
	level.voteString[0] = '\0';

	trap->SetConfigstring( CS_VOTE_TIME, "" );
	trap->SetConfigstring( CS_VOTE_STRING, "" );
	trap->SetConfigstring( CS_VOTE_YES, "" );
	trap->SetConfigstring( CS_VOTE_NO, "" );

	trap->SendServerCommand( -1, "print \""S_COLOR_RED"Vote has been killed!\n\"" );
}

// force the specified client to a specific team
static void AM_ForceTeam( gentity_t *ent ) {
	char arg1[64]={0}, arg2[64]={0};
	int targetClient;
	gentity_t *targ;

	if ( trap->Argc() != 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\amforceteam <client> <team>\n\"" );
		return;
	}

	//amforceteam <partial name|clientNum> <team>
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	trap->Argv( 2, arg2, sizeof( arg2 ) );
	targetClient = (trap->Argc()>1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT ) : ent-g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	if( !AM_CanInflict( ent, targ ) )
			return;

	if ( targ->inuse && targ->client && targ->client->pers.connected )
		SetTeam( targ, arg2, qtrue );
}

// force the targeted client to spectator team
static void AM_GunSpectate( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );

	if ( tr->entityNum < MAX_CLIENTS )
	{
		if( !AM_CanInflict( ent, &g_entities[tr->entityNum] ) )
			return;
		SetTeam( &g_entities[tr->entityNum], "s", qtrue );
	}
}

// protect/unprotect the specified client
static void AM_Protect( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int targetClient;
	gentity_t *targ;

	// can protect: self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT ) : ent-g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	if( !AM_CanInflict( ent, targ ) )
			return;

	targ->client->ps.eFlags			^= EF_INVULNERABLE;
	targ->client->invulnerableTimer	= !!( targ->client->ps.eFlags & EF_INVULNERABLE ) ? 0x7FFFFFFF : level.time;

	trap->SendServerCommand( ent-g_entities, va( "print \"%s "S_COLOR_WHITE"has been %s\n\"", targ->client->pers.netname,
		!!(targ->client->ps.eFlags&EF_INVULNERABLE)?S_COLOR_GREEN"protected":S_COLOR_RED"unprotected" ) );
}

// protect/unprotect the targeted client
static void AM_GunProtect( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS ) {
		gclient_t *cl = &level.clients[tr->entityNum];
		if( !AM_CanInflict( ent, &g_entities[tr->entityNum] ) )
			return;


		cl->ps.eFlags			^= EF_INVULNERABLE;
		cl->invulnerableTimer	= !!(cl->ps.eFlags & EF_INVULNERABLE) ? level.time : 0x7FFFFFFF;
	}
}

static void AM_Empower( gentity_t *ent ) {
	char arg1[64] = {0};
	int targetClient;
	gentity_t *targ;

	// can empower: self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT ) : ent-g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	if( !AM_CanInflict( ent, targ ) )
			return;


	targ->client->pers.adminData.empowered = !targ->client->pers.adminData.empowered;
	targ->client->ps.fd.forcePowerSelected = 0; // HACK: What the actual fuck
	if ( targ->client->pers.adminData.empowered ) {
		int i = 0;
		targ->client->ps.eFlags |= EF_BODYPUSH;

		targ->client->pers.adminData.forcePowersKnown = targ->client->ps.fd.forcePowersKnown;

		for ( i=0; i<NUM_FORCE_POWERS; i++ ) {
			targ->client->pers.adminData.forcePowerBaseLevel[i] = targ->client->ps.fd.forcePowerBaseLevel[i];
			targ->client->ps.fd.forcePowerBaseLevel[i] = 3;
			targ->client->pers.adminData.forcePowerLevel[i] = targ->client->ps.fd.forcePowerLevel[i];
			targ->client->ps.fd.forcePowerLevel[i] = 3;
			targ->client->ps.fd.forcePowersKnown |= (1<<i);
		}
	}
	else {
		int i = 0;
		targ->client->ps.eFlags &= ~EF_BODYPUSH;

		targ->client->ps.fd.forcePowersKnown = targ->client->pers.adminData.forcePowersKnown;
		for ( i=0; i<NUM_FORCE_POWERS; i++ ) {
			targ->client->ps.fd.forcePowerBaseLevel[i] = targ->client->pers.adminData.forcePowerBaseLevel[i];
			targ->client->ps.fd.forcePowerLevel[i] = targ->client->pers.adminData.forcePowerLevel[i];
		}
	}
}

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vector3 *pushDir, float strength, qboolean breakSaberLock );
static void Slap( gentity_t *targ ) {
	vector3 newDir = { 0.0f, 0.0f, 0.0f };
	int i=0;

	for ( i=0; i<2; i++ ) {
		newDir.data[i] = crandom();
		if ( newDir.data[i] > 0.0f )	newDir.data[i] = ceilf( newDir.data[i] );
		else							newDir.data[i] = floorf( newDir.data[i] );
	}
	newDir.z = 1.0f;

	G_Knockdown( targ, NULL, &newDir, japp_slapDistance.value, qtrue );
	G_Throw( targ, &newDir, japp_slapDistance.value );
}

// slap the specified client
static void AM_Slap( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int targetClient;

	if ( trap->Argc() != 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"usage: amslap <client>\n\"" );
		return;
	}

	//Can slap: partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );

	if ( targetClient == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

	Slap( &g_entities[targetClient] );
}

// slap the targeted client
static void AM_GunSlap( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );

	if ( tr->entityNum < MAX_CLIENTS )
	{
		if( !AM_CanInflict( ent, &g_entities[tr->entityNum] ) )
			return;
		Slap( &g_entities[tr->entityNum] );
	}
}

static void Freeze( gclient_t *cl ) {
	cl->pers.adminData.isFrozen = qtrue;
	VectorClear( &cl->ps.velocity );
}

static void Unfreeze( gclient_t *cl ) {
	cl->pers.adminData.isFrozen = qfalse;
}

// freeze specified client on the spot
static void AM_Freeze( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int clientNum;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be frozen (Or -1 for all)\n\"" );
		return;
	}

	// grab the clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	clientNum = G_ClientFromString( ent, arg1, FINDCL_SUBSTR );

	// check for purposely freezing all. HACKHACKHACK
	if ( arg1[0] == '-' && arg1[1] == '1' )
		clientNum = -2;

	// freeze everyone
	if ( clientNum == -2 ) {
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
		{
			if( !AM_CanInflict( ent, &g_entities[i] ) )
				return;
			Freeze( &level.clients[i] );
		}
		trap->SendServerCommand( -1, "cp \"You have all been "S_COLOR_CYAN"frozen\n\"" );
	}
	// freeze specified clientNum
	else if ( clientNum != -1 ) {
		gclient_t *cl = &level.clients[clientNum];

		if( !AM_CanInflict( ent, &g_entities[clientNum] ) )
			return;

		if ( cl->pers.adminData.isFrozen ) {
			Unfreeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"unfrozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( clientNum, "cp \"You have been "S_COLOR_CYAN"unfrozen\n\"" );
		}
		else {
			Freeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"frozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( clientNum, "cp \"You have been "S_COLOR_CYAN"frozen\n\"" );
		}
	}
}

// toggle the 'frozen' state of targeted client
static void AM_GunFreeze( gentity_t *ent ) {
	trace_t	*tr = G_RealTrace( ent, 0.0f );

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS ) {
		gclient_t *cl = &level.clients[tr->entityNum];

		if( !AM_CanInflict( ent, &g_entities[tr->entityNum] ) )
			return;

		if ( cl->pers.adminData.isFrozen ) {
			Unfreeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"unfrozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( tr->entityNum, "cp \"You have been "S_COLOR_CYAN"unfrozen\n\"" );
		}
		else {
			Freeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"frozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( tr->entityNum, "cp \"You have been "S_COLOR_CYAN"frozen\n\"" );
		}
	}
#ifdef _DEBUG
	else
		trap->SendServerCommand( ent-g_entities, "print \"Gunfreeze missed!\n\"" );
#endif
}

// silence specified client
static void AM_Silence( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int targetClient;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be silenced\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	// silence everyone
	if ( atoi( arg1 ) == -1 ) {
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
		{
			if( !AM_CanInflict( ent, &g_entities[i] ) )
				return;
			level.clients[i].pers.adminData.canTalk = qfalse;
		}
		trap->SendServerCommand( -1, "cp \"You have all been "S_COLOR_CYAN"silenced\n\"" );
		return;
	}

	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );
	if ( targetClient == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

	level.clients[targetClient].pers.adminData.canTalk = qfalse;
	trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"silenced\n\"", level.clients[targetClient].pers.netname ) );
	trap->SendServerCommand( targetClient, "cp \"You have been "S_COLOR_CYAN"silenced\n\"" );
}

// unsilence specified client
static void AM_Unsilence( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int targetClient;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be un-silenced\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	// unsilence everyone
	if ( atoi( arg1 ) == -1 ) {
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
		{
			if( !AM_CanInflict( ent, &g_entities[i] ) )
				return;
			level.clients[i].pers.adminData.canTalk = qtrue;
		}
		trap->SendServerCommand( -1, "cp \"You have all been "S_COLOR_CYAN"unsilenced\n\"" );
		return;
	}

	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );
	if ( targetClient == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

	level.clients[targetClient].pers.adminData.canTalk = qtrue;
	trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_CYAN"un-silenced\n\"", level.clients[targetClient].pers.netname ) );
	trap->SendServerCommand( targetClient, "cp \"You have been "S_COLOR_CYAN"un-silenced\n\"" );
}

extern void Cmd_Kill_f( gentity_t *ent );
// slay the specified client
static void AM_Slay( gentity_t *ent ) {
	char arg1[64] = { 0 };
	int targetClient;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be slain\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );
	if ( targetClient == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

	Cmd_Kill_f( &g_entities[targetClient] );
	trap->SendServerCommand( -1, va( "cp \"%s\n"S_COLOR_WHITE"has been "S_COLOR_RED"slain\n\"", level.clients[targetClient].pers.netname ) );
	trap->SendServerCommand( targetClient, "cp \"You have been "S_COLOR_RED"slain\n\"" );
}

// kick specified client
static void AM_Kick( gentity_t *ent ) {
	char arg1[64] = {0};
	char *reason = NULL;
	char string[960] = {0};
	int clientNum;

	if ( trap->Argc() == 1 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\amkick <client> <reason>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );
	if ( trap->Argc() > 2 )
		reason = ConcatArgs( 2 );

	clientNum = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );

	if ( clientNum == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[clientNum] ) )
			return;

	Q_strncpyz( string, va( "Kicked!\nReason: %s", reason ? reason : "Not specified" ), sizeof( string ) );
	trap->DropClient( clientNum, string );
//	ClientDisconnect( clientNum );
}

static void AM_Ban( gentity_t *ent ) {
	char arg1[32] = {0}, arg2[8] = {0}, *arg3 = NULL;
	char string[960] = { 0 };
	int targetClient;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\amban <client> <duration> <reason>\n\"" );
		return;
	}

	else {
		//	clientNum / Partial name
		trap->Argv( 1, arg1, sizeof( arg1 ) );
		targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );
		if ( targetClient == -1 )
			return;

		if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

		Q_strncpyz( arg1, g_entities[targetClient].client->sess.IP, sizeof( arg1 ) );

		trap->Argv( 2, arg2, sizeof( arg2 ) );	//	Duration
		if ( trap->Argc() >= 4 )
			arg3 = ConcatArgs( 3 );				//	Reason

		JKG_Bans_AddBanString( arg1, arg2, arg3 );
		Com_sprintf( string, sizeof( string ), "Banned!\nReason: %s", arg3 ? arg3 : "Not specified" );
		trap->DropClient( targetClient, string );
	//	ClientDisconnect( targetClient );
	}

	return;
}

static void AM_BanIP( gentity_t *ent ) {
	char arg1[32] = {0}, arg2[8] = {0}, *arg3 = NULL;

	if ( trap->Argc() < 2 )
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\ambanip <ip> <duration> <reason>\n\"" );
	else {
		trap->Argv( 1, arg1, sizeof( arg1 ) ); // IP
		trap->Argv( 2, arg2, sizeof( arg2 ) ); // Duration
		if ( trap->Argc() >= 4 )
			arg3 = ConcatArgs( 3 ); // Reason

		JKG_Bans_AddBanString( arg1, arg2, arg3 );
	}

	return;
}

#if 0
static void AM_Portal( gentity_t *ent ) {
	gentity_t       *obj = G_Spawn();
	trace_t         *tr = G_RealTrace( ent, 0.0f );
	vector3          forward, up = { 0, 0, 1 };

	obj->classname = "jp_portal";
	VectorCopy( &tr->endpos, &obj->s.origin );
	obj->s.userInt2 = atoi( ConcatArgs( 1 ) );
	VectorCopy( &tr->plane.normal, &obj->s.boneAngles1 );

	AngleVectors( &ent->client->ps.viewangles, &forward, NULL, NULL );
	CrossProduct( &up, &tr->plane.normal, &obj->s.boneAngles2 );
	if ( VectorLength( &obj->s.boneAngles2 ) == 0.0f )
		VectorSet( &obj->s.boneAngles2, 1, 0, 0 );
	VectorNormalize( &obj->s.boneAngles2 );
	CrossProduct( &tr->plane.normal, &obj->s.boneAngles2, &obj->s.boneAngles3 );
	VectorNormalize( &obj->s.boneAngles3 );

	VectorMA( &tr->endpos, 64.0f, &obj->s.boneAngles2, &up );
	#ifdef _DEBUG
		G_TestLine( &tr->endpos, &up, 0x00, 7500 );
	#endif

	VectorMA( &tr->endpos, 64.0f, &obj->s.boneAngles3, &up );
	#ifdef _DEBUG
		G_TestLine( &tr->endpos, &up, 0x77, 7500 );
	#endif

	VectorMA( &tr->endpos, 64.0f, &tr->plane.normal, &up );
	#ifdef _DEBUG
		G_TestLine( &tr->endpos, &up, 2, 7500 );
	#endif

	G_CallSpawn( obj );
	return;
}
#endif

// shader remapping
static void AM_Remap( gentity_t *ent ) {
	//RAZTODO: amremap
	trap->SendServerCommand( ent-g_entities, "print \"AM_Remap: not yet implemented\n\"" );
}

// weather manipulation
static void AM_Weather( gentity_t *ent ) {
	//RAZTODO: amweather
	trap->SendServerCommand( ent-g_entities, "print \"AM_Weather: not yet implemented\n\"" );
}

// spawn an entity
static void AM_EntSpawn( gentity_t *ent ) {
	gentity_t	*obj = G_Spawn();
	char		buf[32] = { 0 };
	trace_t		*tr = G_RealTrace( ent, 0.0f );

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"AM_EntSpawn: syntax is 'amspawn entity' where 'entity' is"
			"misc_model, info_player_start, etc\n\"" );
		return;
	}
	trap->Argv( 1, buf, sizeof( buf ) );

	obj->classname = buf;
	VectorCopy( &tr->endpos, &obj->s.origin );
	G_CallSpawn( obj );
	obj->jpSpawned = qtrue;
	return;
}

// remove an entity
static void AM_EntRemove( gentity_t *ent ) {
	trace_t *tr = G_RealTrace( ent, 0.0f );

	if ( tr->entityNum >= MAX_CLIENTS && tr->entityNum < ENTITYNUM_WORLD ) {
		if ( g_entities[tr->entityNum].jpSpawned )
			G_FreeEntity( g_entities + tr->entityNum );
		else
			trap->SendServerCommand( ent-g_entities, "print \"AM_EntRemove: Tried to remove entity that was not manually spawned\n\"" );
	}
}

void Cmd_NPC_f( gentity_t *ent );
// spawn an NPC
static void AM_NPCSpawn( gentity_t *ent ) {
	Cmd_NPC_f( ent );
}

static void AM_Lua( gentity_t *ent ) {
#ifdef JPLUA
	char *args = NULL;
	int argc = trap->Argc();

	if ( argc < 2 || !JPLua.state )
		return;

	args = ConcatArgs( 1 );

	trap->Print( S_COLOR_CYAN"Executing Lua code: %s\n", args );
	if ( luaL_dostring( JPLua.state, args ) != 0 )
		trap->SendServerCommand( ent-g_entities, va( "print \""S_COLOR_RED"Lua Error: %s\n\"", lua_tostring( JPLua.state, -1 ) ) );
#else
	trap->SendServerCommand( ent-g_entities, "print \"Lua is not supported on this server\n\"" );
#endif
}

static void AM_ReloadLua( gentity_t *ent ) {
#ifdef JPLUA
	JPLua_Shutdown();
	JPLua_Init();
#else
	trap->SendServerCommand( ent-g_entities, "print \"Lua is not supported on this server\n\"" );
#endif
}

// change map and gamemode
static void AM_Map( gentity_t *ent ) {
	char gametypeStr[32], map[MAX_QPATH];
	char *filter = NULL, *args = ConcatArgs( 1 );
	int gametype = 0, i = 0;

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"AM_Map: syntax is 'ammap gamemode map'\n\"" );
		return;
	}

	trap->Argv( 1, gametypeStr, sizeof( gametypeStr ) );
	gametype = BG_GetGametypeForString( gametypeStr );

	if ( gametype == -1 ) {// So it didn't find a gamemode that matches the arg provided, it could be numeric.
		i = atoi( gametypeStr );
		if ( i >= 0 && i < GT_MAX_GAME_TYPE )
			gametype = i;
		else {
			trap->SendServerCommand( ent-g_entities, va( "print \"AM_Map: argument 1 must be a valid gametype or gametype"
				"number identifier\n\"", map, BG_GetGametypeString( gametype ) ) );
			return;
		}
	}

	trap->Argv( 2, map, sizeof( map ) );

	if ( !japp_allowAnyGametype.integer ) {
		if ( !G_DoesMapSupportGametype( map, gametype ) ) {
			trap->SendServerCommand( ent-g_entities, va( "print \"Map: %s does not support gametype: %s, or the map doesn't"
				"exist.\n\"", map, BG_GetGametypeString( gametype ) ) );
			return;
		}
	}

	if ( (filter = strchr( args, ';' )) != NULL ) {
		*filter = '\0';
		Com_Printf( "AM_Map: %s passed suspicious arguments that contained ; or \\n!\n", ent->client->pers.netname );
	}
	if ( (filter = strchr( args, '\n' )) != NULL ) {
		*filter = '\0';
		Com_Printf( "AM_Map: %s passed suspicious arguments that contained ; or \\n!\n", ent->client->pers.netname );
	}

	trap->SendConsoleCommand( EXEC_APPEND, va( "g_gametype %d\n", gametype ) );
	trap->SendConsoleCommand( EXEC_APPEND, va( "map %s\n", map ) );

	return;
}

static void AM_Vstr( gentity_t *ent ) {
	char *filter = NULL, *args = ConcatArgs( 1 );

	if ( (filter = strchr( args, ';' )) != NULL ) {
		*filter = '\0';
		Com_Printf( "AM_Vstr: %s attempted to issue a non-vstr command\n", ent->client->pers.netname );
	}
	if ( (filter = strchr( args, '\n' )) != NULL ) {
		*filter = '\0';
		Com_Printf( "AM_Vstr: %s attempted to issue a non-vstr command\n", ent->client->pers.netname );
	}

	trap->SendConsoleCommand( EXEC_APPEND, va( "vstr %s\n", args ) );
}

static void AM_Merc( gentity_t *ent ) {
	char		arg1[64] = { 0 };
	int			targetClient;
	gentity_t	*targ;

	//Can merc: self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT ) : ent-g_entities;

	if ( targetClient == -1 )
		return;

	targ = &g_entities[targetClient];

	if( !AM_CanInflict( ent, targ ) )
			return;

	targ->client->pers.adminData.merc = !targ->client->pers.adminData.merc;
	// give everything between WP_NONE and LAST_USEABLE_WEAPON
	if ( targ->client->pers.adminData.merc ) {
		int i=0;
		targ->client->ps.stats[STAT_WEAPONS] = ((1<<LAST_USEABLE_WEAPON)-1) & ~1;
		for ( i=0; i<AMMO_MAX; i++ ) {
			targ->client->ps.ammo[i] = ammoData[i].max;
		}
	}
	// back to spawn weapons, select first usable weapon
	else {
		int i=0, newWeap=-1, wp=targ->client->ps.weapon;

		targ->client->ps.stats[STAT_WEAPONS] = japp_spawnWeaps.integer;

		for ( i=WP_SABER; i<WP_NUM_WEAPONS; i++ ) {
			if ( (targ->client->ps.stats[STAT_WEAPONS] & (1 << i)) ) {
				newWeap = i;
				break;
			}
		}

		if ( newWeap == WP_NUM_WEAPONS ) {
			for ( i=WP_STUN_BATON; i<WP_SABER; i++ ) {
				if ( (targ->client->ps.stats[STAT_WEAPONS] & (1 << i)) ) {
					newWeap = i;
					break;
				}
			}
			if ( newWeap == WP_SABER )
				newWeap = WP_NONE;
		}

		if ( newWeap != -1 )	targ->client->ps.weapon = newWeap;
		else					targ->client->ps.weapon = 0;

		G_AddEvent( ent, EV_NOAMMO, wp );
	}
}

static void AM_Rename( gentity_t *ent ) {
	char arg1[MAX_NETNAME], arg2[MAX_NETNAME], oldName[MAX_NETNAME];
	char info[MAX_INFO_STRING];
	int targetClient;
	gclient_t *cl;

	if ( trap->Argc() != 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Usage: \\amrename <client> <name>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );
	trap->Argv( 2, arg2, sizeof( arg2 ) );

	targetClient = G_ClientFromString( ent, arg1, FINDCL_SUBSTR|FINDCL_PRINT );
	if ( targetClient == -1 )
		return;

	if( !AM_CanInflict( ent, &g_entities[targetClient] ) )
			return;

	cl = &level.clients[targetClient];
	Q_strncpyz( oldName, cl->pers.netname, sizeof( oldName ) );
	ClientCleanName( arg2, cl->pers.netname, sizeof( cl->pers.netname ) );

	if ( !strcmp( oldName, cl->pers.netname ) )
		return;

	Q_strncpyz( cl->pers.netnameClean, cl->pers.netname, sizeof( cl->pers.netnameClean ) );
	Q_CleanString( cl->pers.netnameClean, STRIP_COLOUR );

	// update clientinfo
	trap->GetConfigstring( CS_PLAYERS+targetClient, info, sizeof( info ) );
	Info_SetValueForKey( info, "n", cl->pers.netname );
	trap->SetConfigstring( CS_PLAYERS+targetClient, info );

	// update userinfo (in engine)
	trap->GetUserinfo( targetClient, info, sizeof( info ) );
	Info_SetValueForKey( info, "name", cl->pers.netname );
	trap->SetUserinfo( targetClient, info );
	trap->SendServerCommand( -1, va( "print \"%s"S_COLOR_WHITE" %s %s\n\"", oldName, G_GetStringEdString( "MP_SVGAME",
		"PLRENAME" ), cl->pers.netname ) );
}

static void AM_LockTeam( gentity_t *ent ) {
	char arg1[16];
	team_t team;

	if ( trap->Argc() != 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Usage: \\amlockteam <team, -1>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	if ( atoi( arg1 ) == -1 ) {
		int i;
		qboolean lockedAny = qfalse;
		for ( i=0; i<TEAM_NUM_TEAMS; i++ ) {
			if ( !level.lockedTeams[i] )
				lockedAny = qtrue;
			level.lockedTeams[i] = qtrue;
		}
		// force all to spectator? unlock spectator?
		if ( lockedAny )
			trap->SendServerCommand( -1, "print \""S_COLOR_YELLOW"All teams are locked\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "red" ) || !Q_stricmp( arg1, "r" ) )
		team = TEAM_RED;
	else if ( !Q_stricmp( arg1, "blue" ) || !Q_stricmp( arg1, "b" ) )
		team = TEAM_BLUE;
	else if ( !Q_stricmp( arg1, "spectator" ) || !Q_stricmp( arg1, "s" ) )
		team = TEAM_SPECTATOR;
	else {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Invalid team\n\"" );
		return;
	}

	// check if it's already locked
	if ( level.lockedTeams[team] )
		return;

	level.lockedTeams[team] = qtrue;
	trap->SendServerCommand( -1, va( "print \"%s "S_COLOR_WHITE"team has been locked\n\"", TeamName( team ) ) );
}

static void AM_UnlockTeam( gentity_t *ent ) {
	char arg1[16];
	team_t team;

	if ( trap->Argc() != 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Usage: \\amunlockteam <team, -1>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	if ( atoi( arg1 ) == -1 ) {
		int i;
		qboolean unlockedAny = qfalse;
		for ( i=0; i<TEAM_NUM_TEAMS; i++ ) {
			if ( level.lockedTeams[i] )
				unlockedAny = qtrue;
			level.lockedTeams[i] = qfalse;
		}
		if ( unlockedAny )
			trap->SendServerCommand( -1, "print \""S_COLOR_CYAN"All teams are unlocked\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "red" ) || !Q_stricmp( arg1, "r" ) )
		team = TEAM_RED;
	else if ( !Q_stricmp( arg1, "blue" ) || !Q_stricmp( arg1, "b" ) )
		team = TEAM_BLUE;
	else if ( !Q_stricmp( arg1, "spectator" ) || !Q_stricmp( arg1, "s" ) )
		team = TEAM_SPECTATOR;
	else {
		trap->SendServerCommand( ent-g_entities, "print \""S_COLOR_YELLOW"Invalid team\n\"" );
		return;
	}

	// check if it's already unlocked
	if ( !level.lockedTeams[team] )
		return;

	level.lockedTeams[team] = qfalse;
	trap->SendServerCommand( -1, va( "print \"%s "S_COLOR_WHITE"team has been unlocked\n\"", TeamName( team ) ) );
}

//Walk around no weapon no melee
//admin chat logs

typedef struct adminCommand_s {
	const char	*cmd;
	uint32_t	privilege;
	void		(*func)(gentity_t *ent);
} adminCommand_t;

static const adminCommand_t adminCommands[] = {
	// these must be in alphabetical order for the binary search to work
//	{ "amlogin",		-1,				AM_Login			}, // log in using user + pass (Handled explicitly!!!)
	{ "amban",			PRIV_BAN,		AM_Ban				}, // ban specified client (Client + duration + reason)
	{ "ambanip",		PRIV_BAN,		AM_BanIP			}, // ban specified IP (IP/range-ban + duration + reason)
	{ "amclip",			PRIV_CLIP,		AM_Clip				}, // toggle noclip mode
	{ "amempower",		PRIV_EMPOWER,	AM_Empower			}, // empower the specified client
	{ "amforceteam",	PRIV_FORCETEAM,	AM_ForceTeam		}, // force the specified client to a specific team
	{ "amfreeze",		PRIV_FREEZE,	AM_Freeze			}, // freeze specified client on the spot
	{ "amghost",		PRIV_GHOST,		AM_Ghost			}, // ghost specified client (or self)
	{ "amkick",			PRIV_KICK,		AM_Kick				}, // kick specified client
	{ "amkillvote",		PRIV_KILLVOTE,	AM_KillVote			}, // kill the current vote
	{ "amlisttele",		PRIV_TELEPORT,	AM_ListTelemarks	}, // list all marked positions
	{ "amlockteam",		PRIV_LOCKTEAM,	AM_LockTeam			}, // prevent clients from joining a team
	{ "amlogout",		-1,				AM_Logout			}, // logout
	{ "amluaexec",		PRIV_LUA,		AM_Lua				}, // execute Lua code
	{ "amluareload",	PRIV_LUA,		AM_ReloadLua		}, // reload JPLua system
	{ "ammap",			PRIV_MAP,		AM_Map				}, // change map and gamemode
	{ "ammerc",			PRIV_MERC,		AM_Merc				}, // give all weapons
	{ "amnpc",			PRIV_NPCSPAWN,	AM_NPCSpawn			}, // spawn an NPC (Including vehicles)
	{ "ampoll",			PRIV_POLL,		AM_Poll				}, // call an arbitrary vote
	{ "amprotect",		PRIV_PROTECT,	AM_Protect			}, // protect the specified client
	{ "ampsay",			PRIV_ANNOUNCE,	AM_Announce			}, // announce a message to the specified client (Or all)
	{ "amremap",		PRIV_REMAP,		AM_Remap			}, // shader remapping
	{ "amremovetele",	PRIV_TELEPORT,	AM_RemoveTelemark	}, // remove a telemark from the list
	{ "amrename",		PRIV_RENAME,	AM_Rename			}, // rename a c;ient
	{ "amsavetele",		PRIV_TELEPORT,	AM_SaveTelemarksCmd	}, // save marked positions RAZFIXME: Temporary?
	{ "amseetele",		PRIV_TELEPORT,	AM_SeeTelemarks		}, // visualise all telemarks
	{ "amsilence",		PRIV_SILENCE,	AM_Silence			}, // silence specified client
	{ "amslap",			PRIV_SLAP,		AM_Slap				}, // slap the specified client
	{ "amslay",			PRIV_SLAY,		AM_Slay				}, // slay the specified client
	{ "amspawn",		PRIV_ENTSPAWN,	AM_EntSpawn			}, // spawn an entity
	{ "amstatus",		PRIV_STATUS,	AM_Status			}, // display list of players + clientNum + IP + admin
	{ "amtele",			PRIV_TELEPORT,	AM_Teleport			}, // teleport (All variations of x to y)
	{ "amtelemark",		PRIV_TELEPORT,	AM_Telemark			}, // mark current location
	{ "amunlockteam",	PRIV_LOCKTEAM,	AM_UnlockTeam		}, // allow clients to join a team
	{ "amunsilence",	PRIV_SILENCE,	AM_Unsilence		}, // unsilence specified client
	{ "amunspawn",		PRIV_ENTSPAWN,	AM_EntRemove		}, // remove an entity
	{ "amvstr",			PRIV_VSTR,		AM_Vstr				}, // execute a variable string
	{ "amweather",		PRIV_WEATHER,	AM_Weather			}, // weather effects
	{ "amwhois",		PRIV_WHOIS,		AM_WhoIs			}, // display list of admins
	{ "gunfreeze",		PRIV_FREEZE,	AM_GunFreeze		}, // toggle the 'frozen' state of targeted client
	{ "gunprotect",		PRIV_PROTECT,	AM_GunProtect		}, // protect the targeted client
	{ "gunslap",		PRIV_SLAP,		AM_GunSlap			}, // slap the targeted client
	{ "gunspectate",	PRIV_FORCETEAM,	AM_GunSpectate		}, // force the targeted client to spectator team
	{ "guntele",		PRIV_TELEPORT,	AM_GunTeleport		}, // teleport self to targeted position
	{ "guntelemark",	PRIV_TELEPORT,	AM_GunTeleportMark	}, // mark targeted location
	{ "guntelerev",		PRIV_TELEPORT,	AM_GunTeleportRev	}, // teleport targeted client to self
};
static const size_t numAdminCommands = ARRAY_LEN( adminCommands );

static int cmdcmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((adminCommand_t*)b)->cmd );
}

qboolean AM_HasPrivilege( const gentity_t *ent, uint32_t privilege ) {
	adminUser_t *user = ent->client->pers.adminUser;

	if ( user && (user->privileges & privilege) )
		return qtrue;

	return qfalse;
}

//	Handle admin related commands.
//	Return true if the command exists and/or everything was handled fine.
//	Return false if command didn't exist, so we can tell them.
qboolean AM_HandleCommands( gentity_t *ent, const char *cmd ) {
	adminCommand_t *command = NULL;

	if ( !Q_stricmp( cmd, "amlogin" ) ) {
		AM_Login( ent );
		return qtrue;
	}

	command = (adminCommand_t *)bsearch( cmd, adminCommands, numAdminCommands, sizeof( adminCommands[0] ), cmdcmp );
	if ( !command )
		return qfalse;

	else if ( !AM_HasPrivilege( ent, command->privilege ) ) {
		trap->SendServerCommand( ent-g_entities, "print \"Insufficient privileges\n\"" );
		return qtrue;
	}

	G_LogPrintf( level.log.console, "* Admin command (%s) executed by (%s)\n", cmd, ent->client->pers.netname );
	command->func( ent );

	return qtrue;
}

void AM_PrintCommands( gentity_t *ent ) {
	const adminCommand_t *command = NULL;
	adminUser_t *user = ent->client->pers.adminUser;
	char buf[256] = {0};
	int toggle = 0;
	unsigned int count = 0;
	const unsigned int limit = 72;
	size_t i;

	Q_strcat( buf, sizeof( buf ), "Admin commands:\n   " );

	if ( !user ) {
		Q_strcat( buf, sizeof( buf ), " "S_COLOR_RED"Unavailable" );
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

	for ( i=0, command=adminCommands; i<numAdminCommands; i++, command++ ) {
		if ( AM_HasPrivilege( ent, command->privilege ) ) {
			const char *tmpMsg = va( " ^%c%s", (++toggle&1?COLOR_GREEN:COLOR_YELLOW), command->cmd );

			//newline if we reach limit
			if ( count >= limit ) {
				tmpMsg = va( "\n   %s", tmpMsg );
				count = 0;
			}

			if ( strlen( buf ) + strlen( tmpMsg ) >= sizeof( buf ) ) {
				trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", buf ) );
				buf[0] = '\0';
			}
			count += strlen( tmpMsg );
			Q_strcat( buf, sizeof( buf ), tmpMsg );
		}
	}

	trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\n\"", buf ) );
}
