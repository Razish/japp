//================================
//================================
//	
//	-	jp_admin.c
//		In-game Administration system
//	
//	Author: Raz0r
//	
//	Desc:	This administration system is developed to be flexible,
//			easy to maintain, secure and simple.
//			
//			There is a list of user/pass combinations to be defined by
//			the server owner, with each user having their own permission mask.
//			
//			All checking of permissions is done before the associated function
//			is called, so you don't have to worry about complex control paths
//			
//================================
//================================

#include "g_local.h"
#include "g_admin.h"
#include "shared/JAPP/jp_tokenparser.h"
#include "shared/json/cJSON.h"

static adminUser_t *adminUsers = NULL;

/*
{
	"admins" : [{
			"user" : "Herp",
			"pass" : "Derp",
			"privs" : "-1",
			"message" : "$name ^7has logged in"
		}, {
			"user" : "derpity",
			"pass" : "doo",
			"privs" : "27",
			"message" : "$name ^7is an evil admin"
		}]
}
*/

void AM_AddAdmin( const char *user, const char *pass, uint32_t privileges, const char *loginMsg ) {
	adminUser_t	*admin = NULL;

	for ( admin=adminUsers; admin; admin=admin->next ) {
		if ( !strcmp( user, admin->user ) ) {
			trap->Print( "Overwriting existing admin: %s/%s (%d) %s\n", admin->user, admin->password, admin->privileges,
				admin->loginMsg );
			break;
		}
	}

	if ( !admin )
	{// a new admin, insert it to the start of the linked list, user->next will be the old root
		admin = malloc( sizeof( adminUser_t ) );
		memset( admin, 0, sizeof( adminUser_t ) );
		admin->next = adminUsers;
		adminUsers = admin;
	}

	// we're either overwriting an admin, or adding a new one
	Q_strncpyz( admin->user, user, sizeof( admin->user ) );
	Q_strncpyz( admin->password, pass, sizeof( admin->password ) );
	admin->privileges = privileges;
	Q_strncpyz( admin->loginMsg, loginMsg, sizeof( admin->loginMsg ) );
}

void AM_DeleteAdmin( const char *user ) {
	adminUser_t	*admin = NULL, *prev=NULL, *next=NULL;

	for ( admin=adminUsers; admin; admin=admin->next ) {
		next = admin->next;

		if ( !strcmp( user, admin->user ) )
		{// remove it
			gentity_t *ent = NULL;
			for ( ent=g_entities; ent-g_entities<level.maxclients; ent++ ) {
				if ( ent->client->pers.adminUser && ent->client->pers.adminUser == admin ) {
					trap->SendServerCommand( ent-g_entities, "print \"^1Your admin account has been deleted.\n\"" );
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

void AM_ListAdmins( void ) {
	adminUser_t	*admin = NULL;
	int count = 0;

	trap->Print( "Listing admin accounts:\n" );

	for ( admin=adminUsers; admin; admin=admin->next ) {
		gentity_t *ent = NULL;

		trap->Print( " %3d: %s/%s (%d) %s\n", ++count, admin->user, admin->password, admin->privileges, admin->loginMsg );

		for ( ent=g_entities; ent-g_entities<level.maxclients; ent++ ) {
			if ( ent->client->pers.adminUser && ent->client->pers.adminUser == admin )
				trap->Print( "      Logged in: %s\n", ent->client->pers.netname );
		}
	}
}

static void AM_ProcessUsers( const char *jsonText )
{
	cJSON *root = NULL, *admins = NULL;
	int adminsCount = 0, i = 0;
	const char *tmp = NULL;
	adminUser_t	*user = adminUsers;

	root = cJSON_Parse( jsonText );
	if ( !root )
	{
		Com_Printf( "- ERROR: Could not parse admin info\n" );
		return;
	}

	admins = cJSON_GetObjectItem( root, "admins" );
	adminsCount = cJSON_GetArraySize( admins );

	for ( i=0; i<adminsCount; i++ )
	{
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

		// login message
		if ( (tmp=cJSON_ToString( cJSON_GetObjectItem( item, "message" ) )) )
			Q_strncpyz( user->loginMsg, tmp, sizeof( user->loginMsg ) );
	}
}

void AM_ParseAdmins( void )
{
	char			*buf = NULL, loadPath[MAX_QPATH] = {0};
	unsigned int	len = 0;
	fileHandle_t	f = 0;
	adminUser_t *user = adminUsers;

	// clear the admins table
	while ( user ) {
		adminUser_t *next = user->next;

		gentity_t *ent = NULL;
		for ( ent=g_entities; ent-g_entities<level.maxclients; ent++ ) {
			if ( ent->client->pers.adminUser && ent->client->pers.adminUser == user ) {
				trap->SendServerCommand( ent-g_entities, "print \"^3You have been forcefully logged out of admin.\n\"" );
				ent->client->pers.adminUser = NULL;
			}
		}

		free( user );
		user = next;
	}
	adminUsers = NULL;

	Com_Printf( "Loading Admins\n" );
	Com_sprintf( loadPath, sizeof( loadPath ), "admins.dat" );
	len = trap->FS_Open( loadPath, &f, FS_READ );

	if ( !f )
	{//no file
		Com_Printf( "- ERROR: File not found (%s)\n", loadPath );
		return;
	}

	if ( !len || len == -1 )
	{//empty file
		Com_Printf( "- ERROR: File is empty (%s)\n", loadPath );
		trap->FS_Close( f );
		return;
	}

	if ( !(buf = (char*)malloc(len+1)) )
	{//alloc memory for buffer
		Com_Printf( "- ERROR: Ran out of memory\n", loadPath );
		return;
	}

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = 0;

	// pass it off to the json reader
	AM_ProcessUsers( buf );

	free( buf );

	return;
}

/*
{
	"admins" : [{
			"user" : "Herp",
			"pass" : "Derp",
			"privs" : "-1",
			"message" : "$name ^7has logged in"
		}, {
			"user" : "derpity",
			"pass" : "doo",
			"privs" : "27",
			"message" : "$name ^7is an evil admin"
		}]
}
*/

void AM_SaveAdmins( void ) {
	char			loadPath[MAX_QPATH] = {0};
	const char		*buf = NULL;
	fileHandle_t	f;
	cJSON			*root = NULL, *admins = NULL;
	adminUser_t		*admin = NULL;

	Com_Printf( "Saving admins\n" );
	Com_sprintf( loadPath, sizeof( loadPath ), "admins.dat" );
	trap->FS_Open( loadPath, &f, FS_WRITE );

	root = cJSON_CreateObject();
	admins = cJSON_CreateArray();
	for ( admin=adminUsers; admin; admin=admin->next ) {
		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject( item, "user", admin->user );
		cJSON_AddStringToObject( item, "pass", admin->password );
		cJSON_AddIntegerToObject( item, "privs", admin->privileges );
		cJSON_AddStringToObject( item, "message", admin->loginMsg );

		cJSON_AddItemToArray( admins, item );
	}
	cJSON_AddItemToObject( root, "admins", admins );

	buf = cJSON_Serialize( root, 1 );
	trap->FS_Write( buf, strlen( buf ), f );
	trap->FS_Close( f );

	free( (void *)buf );
	cJSON_Delete( root );
}

static qboolean AM_TM_ProcessTelemark( const char *token )
{//Okay, so we're looping through and setting all the info for each class per team
	const char	*tmp = NULL;
	teleMark_t	*current = &level.adminData.teleMarks[level.adminData.teleMarksIndex];

	//If we're still at the end of the previous telemark, there's no more :o
	if ( !Q_stricmp( token, "}" ) )
		return qfalse;

	//Name
	if ( TP_ParseString( &tmp ) )
		Com_Printf( "Unexpected EOL line %i (Expected 'name')\n", TP_CurrentLine() );
	else
		Q_strncpyz( current->name, tmp, sizeof( current->name ) );

	//Position
	if ( TP_ParseVec3( &current->position ) )
		Com_Printf( "Unexpected EOL line %i (Expected 'position')\n", TP_CurrentLine() );

	//Successful write, fix index
	level.adminData.teleMarksIndex++;

	token = TP_ParseToken();
	return qtrue;
}

void AM_TM_ParseTelemarks( void )
{//Parse telemarks file
	char			*buf = NULL;
	char			loadPath[MAX_QPATH] = { 0 };
	unsigned int	len = 0;
	const char		*token = NULL;
	fileHandle_t	f = 0;

	//Clear the admins table
	memset( &level.adminData.teleMarks, 0, sizeof( teleMark_t ) * 32 );
	level.adminData.teleMarksIndex = 0;

	Com_Printf( "^5JA++: Loading telemark Data...\n" );
	Com_sprintf( loadPath, sizeof(loadPath), "maps\\%s.tele", level.rawmapname );
	len = trap->FS_Open( loadPath, &f, FS_READ );

	if ( !f )
	{//no file
		Com_Printf( "^1Telemark loading failed! (Can't find %s)\n", loadPath );
		return;
	}

	if ( !len || len == -1 )
	{//empty file
		Com_Printf( "^1Telemark loading failed! (%s is empty)\n", loadPath );
		trap->FS_Close( f );
		return;
	}

	if ( !(buf = (char*)malloc( len+1 )) )
	{//alloc memory for buffer
		Com_Printf( "^1Telemark loading failed! (Failed to allocate buffer)\n" );
		return;
	}

	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = 0;
	TP_NewParseSession( buf );

	while ( 1 )
	{//the file is there, and it's not empty
		token = TP_ParseToken();
		if ( !token[0] )
			break;
		
		if ( level.adminData.teleMarksIndex < 32 && !AM_TM_ProcessTelemark( token ) )
			break;
	}

	free( buf );

	return;
}

void AM_TM_SaveTelemarks( void )
{//Save telemarks file
	char			buf[16384] = { 0 };// 16k file size
	char			loadPath[MAX_QPATH];
	fileHandle_t	f;
	teleMark_t		*current = NULL;

	Com_Printf( "^5Saving telemark Data...\n" );
	Com_sprintf( loadPath, sizeof(loadPath), "maps\\%s.tele", level.rawmapname );
	trap->FS_Open( loadPath, &f, FS_WRITE );

	for ( current=level.adminData.teleMarks; current-level.adminData.teleMarks<level.adminData.teleMarksIndex; current++ )
		Q_strcat( buf, sizeof( buf ), va( "{ \"%s\" %i %i %i }\n", current->name, (int)current->position.x, (int)current->position.y, (int)current->position.z ) );

	trap->FS_Write( buf, strlen( buf ), f );
	trap->FS_Close( f );

	return;
}

trace_t *RealTrace( gentity_t *ent, float dist )
{
	static trace_t tr;
	vector3	start, end;

	if ( japp_unlagged.integer )
		G_TimeShiftAllClients( ent->client->pers.cmd.serverTime, ent );

	//Get start
	VectorCopy( &ent->client->ps.origin, &start );
	start.z += ent->client->ps.viewheight; //36.0f;

	//Get end
	AngleVectors( &ent->client->ps.viewangles, &end, NULL, NULL );
	VectorMA( &start, dist ? dist : 16384.0f, &end, &end );

	trap->Trace( &tr, &start, NULL, NULL, &end, ent->s.number, MASK_OPAQUE|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, qfalse, 0, 0 );

	#ifdef _DEBUG
		G_TestLine( &start, &tr.endpos, 0xFF, 7500 );
	#endif

	if ( japp_unlagged.integer )
		G_UnTimeShiftAllClients( ent );

	return &tr;
}

//================================
//================================

//** HANDLED EXPLICITLY
static void AM_Login( gentity_t *ent )
{//Log in using user + pass
	char argUser[64] = {0}, argPass[64] = {0};
	adminUser_t *user = NULL, *current = adminUsers;

	if ( trap->Argc() < 3 ) {
		trap->SendServerCommand( ent-g_entities, "print \"Please enter a name and password to login\n\"" );
		return;
	}

	//Grab user + pass
	trap->Argv( 1, argUser, sizeof( argUser ) );
	trap->Argv( 2, argPass, sizeof( argPass ) );

	for ( current=adminUsers; current; current=current->next )
	{//find valid user
		if ( !strcmp( current->user, argUser ) )
		{
			if ( !strcmp( current->password, argPass ) )
			{//case sensitive password
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

static void AM_Logout( gentity_t *ent )
{//Logout
	ent->client->pers.adminUser = NULL;
	trap->SendServerCommand( ent-g_entities, "print \"You have logged out\n\"" );
}

//#define SENDTOALL
static void AM_WhoIs( gentity_t *ent )
{//Display list of admins
	int			i;
	char		msg[960] = { 0 };
	gclient_t	*cl;

	Q_strcat( msg, sizeof( msg ), "^7Name                                Admin User\n" );

	for ( i=0; i<MAX_CLIENTS; i++ )
	{//If the client is an admin, append their name and 'user' to the string
		cl = &level.clients[i];
		if ( cl->pers.adminUser )
		{
			char strName[MAX_NETNAME] = { 0 };
			char strAdmin[32] = { 0 };

			Q_strncpyz( strName, cl->pers.netname, sizeof( strName ) );
			Q_StripColor( strName );
			Q_strncpyz( strAdmin, (cl->pers.adminUser) ? cl->pers.adminUser->user : "", sizeof( strAdmin ) );
			Q_StripColor( strAdmin );

			Q_strcat( msg, sizeof( msg ), va( "%-36s%-32s\n", strName, strAdmin ) );
		}
	}

#ifdef SENDTOALL
	for ( i=0; i<MAX_CLIENTS; i++ )
	{//Now that we've contructed our list, send it to the admins
		cl = &level.clients[i];
		if ( cl->pers.adminUser )
			trap->SendServerCommand( i, va( "print \"%s\"", msg ) );
	}
#else
	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
#endif
}

static void AM_Status( gentity_t *ent )
{//Display list of players + clientNum + IP + admin
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;

	Q_strcat( msg, sizeof( msg ), "^7clientNum   Name                                IP                      Admin User\n" );

	for ( i=0; i<MAX_CLIENTS; i++ )
	{//Build a list of clients
		char *tmpMsg = NULL;
		if ( !g_entities[i].inuse )
			continue;

		cl = &level.clients[i];
		if ( cl->pers.netname[0] )
		{
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strIP[NET_ADDRSTRMAXLEN] = {0};
			char strAdmin[32] = {0};

			Q_strncpyz( strNum, va( "(%i)", i ), sizeof( strNum ) );
			Q_StripColor( strNum );
			Q_strncpyz( strName, cl->pers.netname, sizeof( strName ) );
			Q_StripColor( strName );
			Q_strncpyz( strIP, cl->sess.IP, sizeof( strIP ) );
			Q_strncpyz( strAdmin, (cl->pers.adminUser) ? cl->pers.adminUser->user : "", sizeof( strAdmin ) );
			Q_StripColor( strAdmin );

			tmpMsg = va( "%-12s%-36s%-24s%-32s\n", strNum, strName, strIP, strAdmin );

			if ( strlen( msg ) + strlen( tmpMsg ) >= sizeof( msg ) )
			{
				trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
				msg[0] = '\0';
			}
			Q_strcat( msg, sizeof( msg ), tmpMsg );
		}
	}

#ifdef SENDTOALL
	for ( i=0; i<MAX_CLIENTS; i++ )
	{//Now that we've contructed our list, send it to the admins
		cl = &level.clients[i];
		if ( cl->pers.adminUser )
			trap->SendServerCommand( i, va( "print \"%s\n\"", msg ) );
	}
#else
	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
#endif
}


//================================
//	Utilities
//================================

static void AM_Announce( gentity_t *ent )
{//Announce a message to all clients
	char	*p;
	char	arg1[48];
	int		targetClient;

	if ( trap->Argc() < 3 )
	{//Not enough args
		trap->SendServerCommand( ent-g_entities, va( "print \"Usage: \\ampsay <client> <message>\n\"" ) );
		return;
	}

	p = ConcatArgs( 2 );
	p = G_NewString( p );

	//Grab the clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = G_ClientNumberFromStrippedName2( arg1 );

	//Check for purposely announcing to all. HACKHACKHACK
	if ( arg1[0] == '-' && arg1[1] == '1' )
		targetClient = -2;

	if ( targetClient == -1 )
	{//Invalid player
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player.\n\"" );
		free( p );
		return;
	}

	//Print to everyone
	else if ( targetClient == -2 )
		trap->SendServerCommand( -1, va( "cp \"%s\"", p ) );

	else
	{//Should be a valid client..
		trap->SendServerCommand( targetClient, va( "cp \"%s\"", p ) );
		trap->SendServerCommand( ent-g_entities, va( "cp \"Relay:\n%s\"", p ) );	//Helena wanted me to relay it back to the sender
	}

	G_LogPrintf( "AM_Announce: Start\nSender: %s\nMessage: %s\nAM_Announce: End\n", ent->client->pers.netname, p );

	free( p );
	return;
}

extern void WP_AddToClientBitflags(gentity_t *ent, int entNum);
static void AM_Ghost( gentity_t *ent )
{//Ghost specified client (or self)
	char		arg1[64];
	int			targetClient;
	gentity_t	*targ;

	//Self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientNumberFromStrippedName2( arg1 ) : ent-g_entities;

	if ( targetClient == -1 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	targ = &g_entities[targetClient];

	if ( targ->client->pers.adminData.isGhost )
	{
		targ->client->pers.adminData.isGhost = qfalse;
		targ->r.contents = CONTENTS_SOLID;
		targ->clipmask = CONTENTS_SOLID|CONTENTS_BODY;
		targ->s.trickedentindex = 0; targ->s.trickedentindex2 = 0; //This is entirely for client-side prediction. Please fix me.
		trap->SendServerCommand( targetClient, "cp \"^5Unghosted\n\"" );
	}
	else
	{
		targ->client->pers.adminData.isGhost = qtrue;
		targ->r.contents = CONTENTS_BODY;
		targ->clipmask = 267009/*CONTENTS_SOLID*/;
		{//This is *entirely* for client-side prediction. Do not rely on it. Please fix me.
			targ->client->ps.fd.forceMindtrickTargetIndex = ~(1<<targetClient);
			targ->client->ps.fd.forceMindtrickTargetIndex2 = ~(1<<targetClient);
		}
		trap->SendServerCommand( targetClient, "cp \"You are now a ^5ghost\n\"" );
	}
}

static void AM_Clip( gentity_t *ent )
{//Toggle noclip mode
	ent->client->noclip = !ent->client->noclip;
	trap->SendServerCommand( ent-g_entities, va( "print \"Noclip %s\n\"", ent->client->noclip?"^2on":"^1off" ) );
}

static void AM_Teleport( gentity_t *ent )
{//Teleport (All variations of x to y)
	//No args means we teleport ourself to our last marked coordinates
	if ( trap->Argc() == 1 && (ent->client->pers.adminData.teleMark.x || ent->client->pers.adminData.teleMark.y || ent->client->pers.adminData.teleMark.z) )
		TeleportPlayer( ent, &ent->client->pers.adminData.teleMark, &ent->client->ps.viewangles );

	else if ( trap->Argc() == 2 )
	{//1 arg means we're teleporting ourself to x (self -> client, self -> namedTelePos)
		char	arg1[64];
		int		targetClient;

		trap->Argv( 1, arg1, sizeof( arg1 ) );
		targetClient = G_ClientNumberFromStrippedName2( arg1 );
		if ( targetClient == -1 )
		{//No client with this name, check for named teleport
			int i;
			char cleanedInput[64];
			char cleanedTelemark[64];

			SanitizeString3( arg1, cleanedInput );

			for ( i=0; i<level.adminData.teleMarksIndex; i++ )
			{//Check all telemarks
				SanitizeString3( level.adminData.teleMarks[i].name, cleanedTelemark );
				if ( !Q_stricmp( cleanedTelemark, cleanedInput ) )
				{//We found one that contains our string
					TeleportPlayer( ent, &level.adminData.teleMarks[i].position, &ent->client->ps.viewangles );
					return;
				}
			}
			trap->SendServerCommand( ent-g_entities, "print \"AM_Teleport: Assumed telemark but found no matching telemark\n\"" );
		}
		else if ( g_entities[targetClient].inuse )
		{//Must be teleporting self -> client
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

	//RAZTODO:
	else if ( trap->Argc() == 3 )
	{//2 args mean we're teleporting x to y (client -> client, client -> telemark)
		char	arg1[64];
		char	arg2[64];
		int		targetClient1;
		int		targetClient2;

		trap->Argv( 1, arg1, sizeof( arg1 ) );	targetClient1 = G_ClientNumberFromStrippedName2( arg1 );
		trap->Argv( 2, arg2, sizeof( arg2 ) );	targetClient2 = G_ClientNumberFromStrippedName2( arg2 );
		
		if ( targetClient1 == -1 )
		{//No client with this name - abort!
			trap->SendServerCommand( ent-g_entities, "print \"Invalid player (First argument)\n\"" );
			return;
		}
		else
		{//First arg is a valid client, attempt to find destination
			if ( targetClient2 == -1 )
			{//No client with this name - check for telemark
				int i;
				char cleanedInput[64];
				char cleanedTelemark[64];

				SanitizeString3( arg2, cleanedInput );

				for ( i=0; i<level.adminData.teleMarksIndex; i++ )
				{//Check all telemarks
					SanitizeString3( level.adminData.teleMarks[i].name, cleanedTelemark );
					if ( !Q_stricmp( cleanedTelemark, cleanedInput ) )
					{//We found one that contains our string
						TeleportPlayer( &g_entities[targetClient1], &level.adminData.teleMarks[i].position, &ent->client->ps.viewangles );
						return;
					}
				}
				trap->SendServerCommand( ent-g_entities, "print \"AM_Teleport: Assumed telemark but found no matching telemark\n\"" );
			}
			else
			{//Must be teleporting client -> client
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

	else if ( trap->Argc() == 4 )
	{//amtele x y z - tele ourself to x y z
		char	argX[8], argY[8], argZ[8];
		vector3	telePos;

		trap->Argv( 1, argX, sizeof( argX ) );	trap->Argv( 2, argY, sizeof( argX ) );	trap->Argv( 3, argZ, sizeof( argX ) );

		VectorSet( &telePos, atoff( argX ), atof( argY ), atof( argZ ) );
		TeleportPlayer( ent, &telePos, &ent->client->ps.viewangles );
	}

	else if ( trap->Argc() > 4 )
	{//amtele c x y z - tele c to x y z
	 //amtele c x y z r - tele c to x y z with r
		char	argC[64];
		int		targetClient;

		trap->Argv( 1, argC, sizeof( argC ) );
		targetClient = G_ClientNumberFromStrippedName2( argC );

		if ( targetClient == -1 )
			trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		else
		{
			vector3	telePos;
			char argX[16]={0}, argY[16]={0}, argZ[16]={0}, argR[8]={0};

			trap->Argv( 2, argX, sizeof( argX ) );
			trap->Argv( 3, argY, sizeof( argY ) );
			trap->Argv( 4, argZ, sizeof( argZ ) );

			VectorCopy( tv( atoi(argX), atoi(argY), atoi(argZ) ), &telePos );
			if ( trap->Argc() == 6 )
			{// amtele c x y z r
				vector3 angles = { 0.0f };
	
				trap->Argv( 5, argR, sizeof( argR ) );
				angles.yaw = atoi( argR );
				TeleportPlayer( &g_entities[targetClient], &telePos, &angles );
			}
			else // amtele c x y z
				TeleportPlayer( &g_entities[targetClient], &telePos, &g_entities[targetClient].client->ps.viewangles );
		}

	}

	//RAZTODO: Coords
}

static void AM_GunTeleport( gentity_t *ent )
{//Teleport self to targeted position
	trace_t	*tr = RealTrace( ent, 0.0f );
	vector3	telepos;

	VectorMA( &tr->endpos, 48.0f, &tr->plane.normal, &telepos );
	TeleportPlayer( ent, &telepos, &ent->client->ps.viewangles );
}

static void AM_GunTeleportRev( gentity_t *ent )
{//Teleport targeted client to self
	trace_t	*tr = RealTrace( ent, 0.0f );
	vector3	angles, telepos;

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS )
	{//Success, teleport them to us
		AngleVectors( &ent->client->ps.viewangles, &angles, NULL, NULL );
		VectorMA( &ent->client->ps.origin, 48.0f, &angles, &telepos );
		TeleportPlayer( &g_entities[tr->entityNum], &telepos, &level.clients[tr->entityNum].ps.viewangles );
	}
}

static teleMark_t *teleMarks = level.adminData.teleMarks;
static teleMark_t *GetTelemark( void )
{
	return &teleMarks[level.adminData.teleMarksIndex];
}
void SP_fx_runner( gentity_t *ent );
static void SpawnTelemark( teleMark_t *tm, vector3 *origin, const char *args )
{
	if ( level.adminData.teleMarksVisual )
	{
		tm->ent = G_Spawn();
		tm->ent->fullName = "env/fire_wall";
		SP_fx_runner( tm->ent );
	}

	Q_strncpyz( tm->name, args, 32 );
	VectorCopy( origin, &tm->position );
}
static void AM_Telemark( gentity_t *ent )
{//Mark current location
	if ( trap->Argc() > 1 )
	{//They provided a name
		char	*args	= ConcatArgs( 1 );
		int		i;

		for ( i=0; i<level.adminData.teleMarksIndex; i++ )
		{//Check for an existing telemark
			if ( !Q_stricmp( args, teleMarks[i].name ) )
			{//Update it
				SpawnTelemark( &teleMarks[i], &ent->client->ps.origin, args );
				VectorCopy( &ent->client->ps.origin, &ent->client->pers.adminData.teleMark );
				return;
			}
		}

		//It doesn't exist yet, check if we have room to add it
		if ( level.adminData.teleMarksIndex < 32 )
		{
			SpawnTelemark( GetTelemark(), &ent->client->ps.origin, args );
			VectorCopy( &ent->client->ps.origin, &ent->client->pers.adminData.teleMark );
			level.adminData.teleMarksIndex++;
		}

		else
			trap->SendServerCommand( ent-g_entities, "print \"Maximum number of telemarks for this map exceeded! (Remove some first)\n\"" );
	}
	else
		trap->SendServerCommand( ent-g_entities, "print \"Please name the telemark\n\"" );
}

static void AM_GunTeleportMark( gentity_t *ent )
{//Mark targeted location
	trace_t		*tr = RealTrace( ent, 0.0f );
	vector3		telePos;
	gentity_t	*tent;

	VectorMA( &tr->endpos, 48.0f, &tr->plane.normal, &telePos );

	VectorCopy( &telePos, &ent->client->pers.adminData.teleMark );
	tent = G_PlayEffect( EFFECT_EXPLOSION, &tr->endpos, &tr->plane.normal );
	tent->r.svFlags |= SVF_SINGLECLIENT;
	tent->r.singleClient = ent->s.number;
	trap->SendServerCommand( ent-g_entities, va( "print \"^5Teleport mark created at ^3%s\n\"", vtos( &telePos ) ) );
}

static void AM_RemoveTelemark( gentity_t *ent )
{//Remove a telemark from the list
	char arg1[8];

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	if ( arg1[0] >= '0' && arg1[0] <= '9' )
	{//If they enter invalid input past this, fuck 'em.
		int index = atoi( arg1 );

		//First, free the ent if needed
		if ( level.adminData.teleMarksVisual )
			G_FreeEntity( teleMarks[atoi( arg1 )].ent );

		//Time to remove it from the list. I hope the ptrs work as expected
		for ( index = atoi( arg1 ); index<level.adminData.teleMarksIndex; index++ )
			memcpy( &teleMarks[index], &teleMarks[index+1], sizeof( teleMark_t ) );
		memset( &level.adminData.teleMarks[index], 0, sizeof( teleMark_t ) );

		//Correct the index
		level.adminData.teleMarksIndex--;
	}

	//RAZTODO: Remove telemarks by name
}

static void AM_ListTelemarks( gentity_t *ent )
{//List all marked positions
	int i;
	char msg[3096] = { 0 };
	
	//Append each mark to the end of the string
	Q_strcat( msg, sizeof( msg ), "- Named telemarks\n" );
	Q_strcat( msg, sizeof( msg ), va( "ID %-32s %s\n", "Name", "Location" ) );
	for ( i=0; i<level.adminData.teleMarksIndex; i++ )
		Q_strcat( msg, sizeof( msg ), va( "%2i %-32s %s\n", i, level.adminData.teleMarks[i].name, vtos( &level.adminData.teleMarks[i].position ) ) );
	
	//Send in one big chunk
	trap->SendServerCommand( ent-g_entities, va( "print \"%s\"", msg ) );
}

static void AM_SeeTelemarks( gentity_t *ent )
{//Visualise all telemarks
	int i;

	if ( level.adminData.teleMarksVisual )
	{//Assume all telemarks have valid ent ptr's
		for ( i=0; i<level.adminData.teleMarksIndex; i++ )
			G_FreeEntity( teleMarks[i].ent );
	}
	else
	{
		for ( i=0; i<level.adminData.teleMarksIndex; i++ )
		{
			teleMarks[i].ent = G_Spawn();
			teleMarks[i].ent->fullName = "env/btend";
			VectorCopy( &teleMarks[i].position, &teleMarks[i].ent->s.origin );
			SP_fx_runner( teleMarks[i].ent );
		}
	}
	level.adminData.teleMarksVisual = !level.adminData.teleMarksVisual;
}

static void AM_SaveTelemarks( gentity_t *ent )
{//Save marked positions RAZFIXME: Temporary?
	AM_TM_SaveTelemarks();
}

static void AM_Poll( gentity_t *ent )
{//Call an arbitrary vote
	int i=0;
	char arg1[MAX_TOKEN_CHARS] = {0}, arg2[MAX_TOKEN_CHARS] = {0};

	if ( level.voteExecuteTime )
	{
		trap->SendServerCommand( ent-g_entities, "print \"^3Vote already in progress.\n\"" );
		return;
	}

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"^3Please specify a poll.\n\"" );
		return;
	}

	trap->Argv( 0, arg1, sizeof( arg1 ) );
	//Q_strncpyz( arg2, ConcatArgs( 2 ), sizeof( arg2 ) );
	Q_strncpyz( arg2, ent->client->pers.netname, sizeof( arg2 ) );
	Q_StripColor( arg2 );
	Q_CleanStr( arg2 );
	Q_strstrip( arg2, "\n\r;\"", NULL );

	Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
	Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	Q_strncpyz( level.voteStringClean, level.voteString, sizeof( level.voteStringClean ) );
	Q_strstrip( level.voteStringClean, "\"\n\r", NULL );

	trap->SendServerCommand( -1, va("print \"%s^7 %s\n\"", ent->client->pers.netname, G_GetStringEdString("MP_SVGAME", "PLCALLEDVOTE") ) );
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

static void AM_KillVote( gentity_t *ent )
{//Kill the current vote
	//Overkill, but it's a surefire way to kill the vote =]
	level.voteExecuteTime	= 0;
	level.votingGametype	= qfalse;
	level.votingGametypeTo	= level.gametype;
	level.voteTime			= 0;

	memset( level.voteDisplayString,	0,	sizeof( level.voteDisplayString ) );
	memset( level.voteString,			0,	sizeof( level.voteString ) );

	trap->SetConfigstring( CS_VOTE_TIME, "" );
	trap->SetConfigstring( CS_VOTE_STRING, "" );
	trap->SetConfigstring( CS_VOTE_YES, "" );
	trap->SetConfigstring( CS_VOTE_NO, "" );

	trap->SendServerCommand( -1, "print \"^1Vote has been killed!\n\"" );
}

static void AM_ForceTeam( gentity_t *ent )
{//Force the specified client to a specific team
	char		arg1[64]={0}, arg2[64]={0};
	int			targetClient;
	gentity_t	*targ;

	//amforceteam <partial name|clientNum> <team>
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	trap->Argv( 2, arg2, sizeof( arg2 ) );
	targetClient = (trap->Argc()>1) ? G_ClientNumberFromStrippedName2( arg1 ) : ent-g_entities;

	if ( targetClient == -1 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	targ = &g_entities[targetClient];

	if ( targ->inuse && targ->client && targ->client->pers.connected )
		SetTeam( targ, arg2 );
}

static void AM_GunSpectate( gentity_t *ent )
{//Force the targeted client to spectator team
	trace_t *tr = RealTrace( ent, 0.0f );

	if ( tr->entityNum < MAX_CLIENTS )
		SetTeam( &g_entities[tr->entityNum], "s" );
}

static void AM_Protect( gentity_t *ent )
{//Protect/unprotect the specified client
	char		arg1[64] = { 0 };
	int			targetClient;
	gentity_t	*targ;

	//Can protect: self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientNumberFromStrippedName2( arg1 ) : ent-g_entities;

	if ( targetClient == -1 || !g_entities[targetClient].inuse || g_entities[targetClient].s.number > level.maxclients )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	targ = &g_entities[targetClient];

	targ->client->ps.eFlags			^= EF_INVULNERABLE;
	targ->client->invulnerableTimer	= !!( targ->client->ps.eFlags & EF_INVULNERABLE ) ? 0x7FFFFFFF : level.time;

	trap->SendServerCommand( ent-g_entities, va( "print \"%s ^7has been %s\n\"", targ->client->pers.netname, !!(targ->client->ps.eFlags&EF_INVULNERABLE)?"^2protected":"^1unprotected" ) );
}

static void AM_GunProtect( gentity_t *ent )
{//Protect/unprotect the targeted client
	trace_t *tr = RealTrace( ent, 0.0f );

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS )
	{
		gclient_t *cl = &level.clients[tr->entityNum];
		cl->ps.eFlags			^= EF_INVULNERABLE;
		cl->invulnerableTimer	= !!(cl->ps.eFlags & EF_INVULNERABLE) ? level.time : 0x7FFFFFFF;
	}
}

static void AM_Empower( gentity_t *ent )
{
	char		arg1[64] = { 0 };
	int			targetClient;
	gentity_t	*targ;

	//Can empower: self, partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = (trap->Argc()>1) ? G_ClientNumberFromStrippedName2( arg1 ) : ent-g_entities;

	if ( targetClient == -1 || !g_entities[targetClient].inuse || g_entities[targetClient].s.number > level.maxclients )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	targ = &g_entities[targetClient];

	targ->client->pers.adminData.empowered = !targ->client->pers.adminData.empowered;
	targ->client->ps.fd.forcePowerSelected = 0; // HACK: What the actual fuck
	if ( targ->client->pers.adminData.empowered )
	{
		int i = 0;
		targ->client->ps.eFlags |= EF_BODYPUSH;

		targ->client->pers.adminData.forcePowersKnown = targ->client->ps.fd.forcePowersKnown;

		for ( i=0; i<NUM_FORCE_POWERS; i++ )
		{
			targ->client->pers.adminData.forcePowerBaseLevel[i] = targ->client->ps.fd.forcePowerBaseLevel[i];		targ->client->ps.fd.forcePowerBaseLevel[i] = 3;
			targ->client->pers.adminData.forcePowerLevel[i] = targ->client->ps.fd.forcePowerLevel[i];				targ->client->ps.fd.forcePowerLevel[i] = 3;
			targ->client->ps.fd.forcePowersKnown |= (1<<i);
		}
	}
	else
	{
		int i = 0;
		targ->client->ps.eFlags &= ~EF_BODYPUSH;

		targ->client->ps.fd.forcePowersKnown = targ->client->pers.adminData.forcePowersKnown;
		for ( i=0; i<NUM_FORCE_POWERS; i++ )
		{
			targ->client->ps.fd.forcePowerBaseLevel[i] = targ->client->pers.adminData.forcePowerBaseLevel[i];
			targ->client->ps.fd.forcePowerLevel[i] = targ->client->pers.adminData.forcePowerLevel[i];
		}
	}
}

float RandFloat( float min, float max );
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

	G_Knockdown( targ, NULL, &newDir, 50.0f, qtrue );
	G_Throw( targ, &newDir, 50.0f );
}

static void AM_Slap( gentity_t *ent )
{//Slap the specified client
	char		arg1[64] = { 0 };
	int			targetClient;

	if ( trap->Argc() != 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"usage: amslap <clientnum or partial name>\n\"" );
		return;
	}

	//Can slap: partial name, clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	targetClient = G_ClientNumberFromStrippedName2( arg1 );

	if ( targetClient == -1 || !g_entities[targetClient].inuse || g_entities[targetClient].s.number > level.maxclients )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	Slap( &g_entities[targetClient] );
}

static void AM_GunSlap( gentity_t *ent )
{//Slap the targeted client
	trace_t *tr = RealTrace( ent, 0.0f );

	if ( tr->entityNum < MAX_CLIENTS )
		Slap( &g_entities[tr->entityNum] );
}

static void Freeze( gclient_t *cl ) {
	cl->pers.adminData.isFrozen = qtrue;
	VectorClear( &cl->ps.velocity );
}
static void Unfreeze( gclient_t *cl ) {
	cl->pers.adminData.isFrozen = qfalse;
}
static void AM_Freeze( gentity_t *ent )
{//Freeze specified client on the spot
	char	arg1[64] = { 0 };
	int		clientNum;

	//RAZFIXME: 'amfreeze den' froze me :<

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be frozen (Or -1 for all)\n\"" );
		return;
	}

	//Grab the clientNum
	trap->Argv( 1, arg1, sizeof( arg1 ) );
	clientNum = G_ClientNumberFromStrippedName2( arg1 );

	//Check for purposely freezing all. HACKHACKHACK
	if ( arg1[0] == '-' && arg1[1] == '1' )
		clientNum = -2;

	if ( clientNum == -2 )
	{//Freeze everyone
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
			Freeze( &level.clients[i] );
		trap->SendServerCommand( -1, "cp \"You have all been ^5frozen\n\"" );
	}
	else if ( clientNum != -1 )
	{//Freeze specified clientNum
		gclient_t *cl = &level.clients[clientNum];
		if ( cl->pers.adminData.isFrozen ) {
			Unfreeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5unfrozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( clientNum, "cp \"You have been ^5unfrozen\n\"" );
		}
		else {
			Freeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5frozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( clientNum, "cp \"You have been ^5frozen\n\"" );
		}
	}
}

static void AM_GunFreeze( gentity_t *ent )
{//Toggle the 'frozen' state of targeted client
	trace_t	*tr = RealTrace( ent, 0.0f );

	if ( tr->entityNum >= 0 && tr->entityNum < MAX_CLIENTS )
	{
		gclient_t *cl = &level.clients[tr->entityNum];
		if ( cl->pers.adminData.isFrozen )
		{
			Unfreeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5unfrozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( tr->entityNum, "cp \"You have been ^5unfrozen\n\"" );
		}
		else
		{
			Freeze( cl );
			trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5frozen\n\"", cl->pers.netname ) );
			trap->SendServerCommand( tr->entityNum, "cp \"You have been ^5frozen\n\"" );
		}
	}
#ifdef _DEBUG
	else
		trap->SendServerCommand( ent-g_entities, "print \"Gunfreeze missed!\n\"" );
#endif
}

static void AM_Silence( gentity_t *ent )
{//Silence specified client
	char	arg1[64] = { 0 };
	int		targetClient;

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be silenced\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	if ( atoi( arg1 ) == -1 )
	{//Silence everyone
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
			level.clients[i].pers.adminData.canTalk = qfalse;
		trap->SendServerCommand( -1, "cp \"You have all been ^5silenced\n\"" );
		return;
	}

	targetClient = G_ClientNumberFromStrippedName2( arg1 );
	if ( targetClient == -1 )
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
	else
	{
		level.clients[targetClient].pers.adminData.canTalk = qfalse;
		trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5silenced\n\"", level.clients[targetClient].pers.netname ) );
		trap->SendServerCommand( targetClient, "cp \"You have been ^5silenced\n\"" );
	}
}

static void AM_Unsilence( gentity_t *ent )
{//Unsilence specified client
	char	arg1[64] = { 0 };
	int		targetClient;

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be un-silenced\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	if ( atoi( arg1 ) == -1 )
	{//Unsilence everyone
		int i;
		for ( i=0; i<MAX_CLIENTS; i++ )
			level.clients[i].pers.adminData.canTalk = qtrue;
		trap->SendServerCommand( -1, "cp \"You have all been ^5unsilenced\n\"" );
		return;
	}

	targetClient = G_ClientNumberFromStrippedName2( arg1 );
	if ( targetClient == -1 )
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
	else
	{
		level.clients[targetClient].pers.adminData.canTalk = qtrue;
		trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^5un-silenced\n\"", level.clients[targetClient].pers.netname ) );
		trap->SendServerCommand( targetClient, "cp \"You have been ^5un-silenced\n\"" );
	}
}

extern void Cmd_Kill_f( gentity_t *ent );
static void AM_Slay( gentity_t *ent )
{//Slay the specified client
	char	arg1[64] = { 0 };
	int		targetClient;

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Please specify a player to be slain\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );

	targetClient = G_ClientNumberFromStrippedName2( arg1 );
	if ( targetClient == -1 || !g_entities[targetClient].inuse )
		trap->SendServerCommand( ent-g_entities, va( "print \"Player is not on the server (%s)\n\"", arg1 ) );
	else
	{
		Cmd_Kill_f( &g_entities[targetClient] );
		trap->SendServerCommand( -1, va( "cp \"%s\n^7has been ^1slain\n\"", level.clients[targetClient].pers.netname ) );
		trap->SendServerCommand( targetClient, "cp \"You have been ^1slain\n\"" );
	}
}

//================================
//	Kick / Ban
//================================

static void AM_Kick( gentity_t *ent )
{//Kick specified client
	char	arg1[64] = { 0 };
	char	*reason = NULL;
	char	string[960] = { 0 };
	int		clientNum;

	if ( trap->Argc() == 1 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\amkick <client> <reason>\n\"" );
		return;
	}

	trap->Argv( 1, arg1, sizeof( arg1 ) );
	if ( trap->Argc() > 2 )
		reason = ConcatArgs( 2 );

	clientNum = G_ClientNumberFromStrippedName2( arg1 );

	if ( clientNum == -1 || !g_entities[clientNum].inuse || g_entities[clientNum].s.number > level.maxclients )
	{//No name match, or not an active client
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
	}
	else
	{
		if ( g_entities[clientNum].inuse )
		{
			strcpy( string, va( "Kicked!\nReason: %s", reason?reason:"Not specified" ) );
			trap->DropClient( clientNum, string );
			//ClientDisconnect( clientNum );
		}
		else
		{
			trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		}
	}
}

static void AM_Ban( gentity_t *ent )
{
	char	arg1[32] = { 0 };
	char	arg2[8] = { 0 };
	char	*arg3 = NULL;
	char	string[960] = { 0 };
	int		targetClient;

	if ( trap->Argc() < 2 )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\amban <client> <duration> <reason>\n\"" );
		return;
	}

	else
	{
		//	clientNum / Partial name
		trap->Argv( 1, arg1, sizeof( arg1 ) );
		targetClient = G_ClientNumberFromStrippedName2( arg1 );
		if ( targetClient == -1 || !g_entities[targetClient].inuse || g_entities[targetClient].s.number > level.maxclients )
		{
			trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
			return;
		}
		Q_strncpyz( arg1, g_entities[targetClient].client->sess.IP, sizeof( arg1 ) );

		trap->Argv( 2, arg2, sizeof( arg2 ) );	//	Duration
		if ( trap->Argc() >= 4 )
			arg3 = ConcatArgs( 3 );				//	Reason

		JKG_Bans_AddBanString( arg1, arg2, arg3 );
		Com_sprintf( string, sizeof( string ), "Banned!\nReason: %s", arg3 ? arg3 : "Not specified" );
		trap->DropClient( targetClient, string );
		//ClientDisconnect( targetClient );
	}

	return;
}

static void AM_BanIP( gentity_t *ent )
{
	char	arg1[32] = { 0 };
	char	arg2[8] = { 0 };
	char	*arg3 = NULL;

	if ( trap->Argc() < 2 )
		trap->SendServerCommand( ent-g_entities, "print \"Syntax: \\ambanip <ip> <duration> <reason>\n\"" );
	else
	{
		trap->Argv( 1, arg1, sizeof( arg1 ) );	//	IP
		trap->Argv( 2, arg2, sizeof( arg2 ) );	//	Duration
		if ( trap->Argc() >= 4 )
			arg3 = ConcatArgs( 3 );				//	Reason

		JKG_Bans_AddBanString( arg1, arg2, arg3 );
	}

	return;
}

#ifdef _DEBUG
static void AM_Test( gentity_t *ent )
{
#if 0
	gentity_t	*ghost = &g_entities[RealTrace( ent, 0.0f )->entityNum];

	if ( ghost->s.number > MAX_CLIENTS )
		return;

	if ( ghost->client->pers.adminData.isGhost2 )
		ghost->r.broadcastClients[0] &= ~(1 << ent->s.number);
	else
		ghost->r.broadcastClients[0] |= (1 << ent->s.number);

//	ghost->r.broadcastClients[0] ^= (1 << ent->s.number);
	ent->r.broadcastClients[0] ^= (1 << ghost->s.number);

	ghost->client->pers.adminData.isGhost2 ^= 1;
#endif
#if 0
	gentity_t       *obj = G_Spawn();
	trace_t         *tr = RealTrace( ent, 0.0f );
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
#endif
#if 0
	int x=0;
	int y=1;
	y /= x;
#endif
}
#endif

static void AM_Remap( gentity_t *ent )
{//Shader remapping
	//RAZTODO: amremap
	trap->SendServerCommand( ent-g_entities, "print \"AM_Remap: not yet implemented\n\"" );
}

static void AM_Weather( gentity_t *ent )
{//Shader remapping
	//RAZTODO: amweather
	trap->SendServerCommand( ent-g_entities, "print \"AM_Weather: not yet implemented\n\"" );
}

static void AM_EntSpawn( gentity_t *ent )
{//Spawn an entity
	gentity_t	*obj = G_Spawn();
	char		buf[32] = { 0 };
	trace_t		*tr = RealTrace( ent, 0.0f );

	if ( trap->Argc() < 2 ) {
		trap->SendServerCommand( ent-g_entities, "print \"AM_EntSpawn: syntax is 'amspawn entity' where 'entity' is misc_model, info_player_start, etc\n\"" );
		return;
	}
	trap->Argv( 1, buf, sizeof( buf ) );

	obj->classname = buf;
	VectorCopy( &tr->endpos, &obj->s.origin );
	G_CallSpawn( obj );
	obj->jpSpawned = qtrue;
	return;
}

#define NUM_REMOVE_ENTS (4)
static void AM_EntRemove( gentity_t *ent )
{//Remove an entity
	trace_t *tr = RealTrace( ent, 0.0f );

	if ( tr->entityNum >= MAX_CLIENTS && tr->entityNum < ENTITYNUM_WORLD ) {
		if ( g_entities[tr->entityNum].jpSpawned )
			G_FreeEntity( g_entities + tr->entityNum );
		else
			trap->SendServerCommand( ent-g_entities, "print \"AM_EntRemove: Tried to remove entity that was not manually spawned\n\"" );
	}
}

void Cmd_NPC_f( gentity_t *ent );
static void AM_NPCSpawn( gentity_t *ent )
{//Spawn an NPC
	Cmd_NPC_f( ent );
}

static void AM_Lua( gentity_t *ent ) {
#ifdef JPLUA
	char *args = NULL;
	int argc = trap->Argc();

	if ( argc < 2 || !JPLua.state )
		return;

	args = ConcatArgs( 1 );

	trap->Print( "^5Executing Lua code: %s\n", args );
	if ( luaL_dostring( JPLua.state, args ) != 0 )
		trap->SendServerCommand( ent-g_entities, va( "print \"^1Lua Error: %s\n\"", lua_tostring( JPLua.state, -1 ) ) );
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


static void AM_Map( gentity_t *ent )
{//Change map and gamemode
	char gametypeStr[12];
	char map[MAX_QPATH];
	char *filter = NULL, *args = ConcatArgs( 1 );
	int gametype = 0;
	int i = 0;
	
	

	if(trap->Argc() < 2)
	{
		trap->SendServerCommand( ent-g_entities, "print \"AM_Map: syntax is 'ammap gamemode map'\n\"" );
		return;
	}

	trap->Argv(1, gametypeStr, 12); //Random size for now
	gametype = BG_GetGametypeForString( gametypeStr );
	
	if(gametype == -1) //So it didn't find a gamemode that matches the arg provided, it could be numeric.
	{
		i = atoi(gametypeStr);
		if(i >= 0 && i < GT_MAX_GAME_TYPE)
			gametype = i;
		else
		{
			trap->SendServerCommand( ent-g_entities, va( "print \"AM_Map: argument 1 must be a valid gametype or gametype number identifier\n\"", map, BG_GetGametypeString(gametype)) );
			return;
		}
	}

	trap->Argv(2, map, MAX_QPATH);

	if( !!japp_allowAnyGametype.integer == 0 )
	{
		if( G_DoesMapSupportGametype( map, gametype ) == qfalse )
		{
			trap->SendServerCommand( ent-g_entities, va( "print \"Map: %s does not support gametype: %s, or the map doesn't exist.\n\"", map, BG_GetGametypeString(gametype)) );
			return;
		}
	}
	//Such copypaste
	if ( (filter = strchr( args, ';' )) != NULL ) {
		*filter = '\0';
		Com_Printf( "AM_Map: %s passed suspicious arguments that contained ; or \\n!\n", ent->client->pers.netname );
	}
	if ( (filter = strchr( args, '\n' )) != NULL ) { //so multi-use
		*filter = '\0';
		Com_Printf( "AM_Map: %s passed suspicious arguments that contained ; or \\n!\n", ent->client->pers.netname );
	} //wow
	
	trap->SendConsoleCommand(EXEC_APPEND, va("g_gametype %d\n", gametype) );
	trap->SendConsoleCommand(EXEC_APPEND, va("map %s\n", map) );


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
	targetClient = (trap->Argc()>1) ? G_ClientNumberFromStrippedName2( arg1 ) : ent-g_entities;

	if ( targetClient == -1 || !g_entities[targetClient].inuse || g_entities[targetClient].s.number > level.maxclients )
	{
		trap->SendServerCommand( ent-g_entities, "print \"Invalid player\n\"" );
		return;
	}

	targ = &g_entities[targetClient];

	targ->client->pers.adminData.merc = !targ->client->pers.adminData.merc;
	if ( targ->client->pers.adminData.merc )
	{// give everything between WP_NONE and LAST_USEABLE_WEAPON
		int i=0;
		targ->client->ps.stats[STAT_WEAPONS] = ((1<<LAST_USEABLE_WEAPON)-1) & ~1;
		for ( i=0; i<AMMO_MAX; i++ ) {
			targ->client->ps.ammo[i] = ammoData[i].max;
		}
	}
	else
	{// back to spawn weapons, select first usable weapon
		int i=0, newWeap=-1, wp=targ->client->ps.weapon;

		targ->client->ps.stats[STAT_WEAPONS] = japp_spawnWeaps.integer;

		for ( i=WP_SABER; i<WP_NUM_WEAPONS; i++ )
		{
			if ( (targ->client->ps.stats[STAT_WEAPONS] & (1 << i)) )
			{
				newWeap = i;
				break;
			}
		}

		if ( newWeap == WP_NUM_WEAPONS )
		{
			for ( i=WP_STUN_BATON; i<WP_SABER; i++ )
			{
				if ( (targ->client->ps.stats[STAT_WEAPONS] & (1 << i)) )
				{
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

//spawn weapons/holocrons
//Walk around no weapon no melee
//admin chat logs

typedef struct adminCommand_s {
	const char	*cmd;
	uint32_t	privilege;
	void		(*func)(gentity_t *ent);
} adminCommand_t;

static const adminCommand_t adminCommands[] = {
	// these must be in alphabetical order for the binary search to work
//	{	"amlogin",		-1,				AM_Login			},	//	Log in using user + pass (Handled explicitly!!!)
/**/{	"amban",		PRIV_BAN,		AM_Ban				},	//	Ban specified client (Client + duration + reason)
/**/{	"ambanip",		PRIV_BAN,		AM_BanIP			},	//	Ban specified IP (IP/range-ban + duration + reason)
/**/{	"amclip",		PRIV_CLIP,		AM_Clip				},	//	Toggle noclip mode
/**/{	"amempower",	PRIV_EMPOWER,	AM_Empower			},	//	Empower the specified client
/**/{	"amforceteam",	PRIV_FORCETEAM,	AM_ForceTeam		},	//	Force the specified client to a specific team
/**/{	"amfreeze",		PRIV_FREEZE,	AM_Freeze			},	//	Freeze specified client on the spot
/**/{	"amghost",		PRIV_GHOST,		AM_Ghost			},	//	Ghost specified client (or self)
/**/{	"amkick",		PRIV_KICK,		AM_Kick				},	//	Kick specified client
/**/{	"amkillvote",	PRIV_KILLVOTE,	AM_KillVote			},	//	Kill the current vote
/**/{	"amlisttele",	PRIV_TELEPORT,	AM_ListTelemarks	},	//	List all marked positions
/**/{	"amlogout",		-1,				AM_Logout			},	//	Logout
/**/{	"amluaexec",	PRIV_LUA,		AM_Lua				},	//	Execute Lua code
/**/{	"amluareload",	PRIV_LUA,		AM_ReloadLua		},	//	Reload JPLua system
/**/{	"ammap",		PRIV_MAP,		AM_Map				},	//	Change map and gamemode
/**/{	"ammerc",		PRIV_MERC,		AM_Merc				},	//	Give all weapons
/**/{	"amnpc",		PRIV_NPCSPAWN,	AM_NPCSpawn			},	//	Spawn an NPC (Including vehicles)
/**/{	"ampoll",		PRIV_POLL,		AM_Poll				},	//	Call an arbitrary vote
/**/{	"amprotect",	PRIV_PROTECT,	AM_Protect			},	//	Protect the specified client
/**/{	"ampsay",		PRIV_ANNOUNCE,	AM_Announce			},	//	Announce a message to the specified client (Or all)
	{	"amremap",		PRIV_REMAP,		AM_Remap			},	//	Shader remapping
/**/{	"amremovetele",	PRIV_TELEPORT,	AM_RemoveTelemark	},	//	Remove a telemark from the list
/**/{	"amsavetele",	PRIV_TELEPORT,	AM_SaveTelemarks	},	//	Save marked positions RAZFIXME: Temporary?
/**/{	"amseetele",	PRIV_TELEPORT,	AM_SeeTelemarks		},	//	Visualise all telemarks
/**/{	"amsilence",	PRIV_SILENCE,	AM_Silence			},	//	Silence specified client
/**/{	"amslap",		PRIV_SLAP,		AM_Slap				},	//	Slap the specified client
/**/{	"amslay",		PRIV_SLAY,		AM_Slay				},	//	Slay the specified client
	{	"amspawn",		PRIV_ENTSPAWN,	AM_EntSpawn			},	//	Spawn an entity
/**/{	"amstatus",		PRIV_STATUS,	AM_Status			},	//	Display list of players + clientNum + IP + admin
/**/{	"amtele",		PRIV_TELEPORT,	AM_Teleport			},	//	Teleport (All variations of x to y)
/**/{	"amtelemark",	PRIV_TELEPORT,	AM_Telemark			},	//	Mark current location
#ifdef _DEBUG
	{	"amtest",		-1,				AM_Test				},	//	Test :D
#endif
/**/{	"amunsilence",	PRIV_SILENCE,	AM_Unsilence		},	//	Unsilence specified client
/**/{	"amunspawn",	PRIV_ENTSPAWN,	AM_EntRemove		},	//	Remove an entity
/**/{	"amvstr",		PRIV_VSTR,		AM_Vstr				},	//	Execute a variable string
	{	"amweather",	PRIV_WEATHER,	AM_Weather			},	//	Weather effects
/**/{	"amwhois",		PRIV_WHOIS,		AM_WhoIs			},	//	Display list of admins
/**/{	"gunfreeze",	PRIV_FREEZE,	AM_GunFreeze		},	//	Toggle the 'frozen' state of targeted client
/**/{	"gunprotect",	PRIV_PROTECT,	AM_GunProtect		},	//	Protect the targeted client
/**/{	"gunslap",		PRIV_SLAP,		AM_GunSlap			},	//	Slap the targeted client
/**/{	"gunspectate",	PRIV_FORCETEAM,	AM_GunSpectate		},	//	Force the targeted client to spectator team
/**/{	"guntele",		PRIV_TELEPORT,	AM_GunTeleport		},	//	Teleport self to targeted position
/**/{	"guntelemark",	PRIV_TELEPORT,	AM_GunTeleportMark	},	//	Mark targeted location
/**/{	"guntelerev",	PRIV_TELEPORT,	AM_GunTeleportRev	},	//	Teleport targeted client to self
	{	NULL,			0,				NULL				},
};
static const int numAdminCommands = ARRAY_LEN( adminCommands );

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

	G_LogPrintf( "* Admin command (%s) executed by (%s)\n", cmd, ent->client->pers.netname );
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

	Q_strcat( buf, sizeof( buf ), "Admin commands:\n   " );

	if ( !user ) {
		Q_strcat( buf, sizeof( buf ), " ^1Unavailable" );
		trap->SendServerCommand( ent-g_entities, va( "print \"%s\n\"", buf ) );
		return;
	}

	for ( command=adminCommands; command && command->cmd; command++ ) {
		if ( AM_HasPrivilege( ent, command->privilege ) ) {
			char *tmpMsg = va( " ^%c%s", (++toggle&1?'2':'3'), command->cmd );

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
