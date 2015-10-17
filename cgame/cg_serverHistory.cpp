#include "cg_local.h"

#ifdef SERVER_HISTORY
#include "cJSON/cJSON.h"

typedef struct serverHistoryEntry_s {
	netadr_t					 adr;
	uint32_t					 timestamp;
	struct serverHistoryEntry_s	*next;
} serverHistoryEntry_t;

serverHistoryEntry_t *serverHistory = nullptr;
static const char historyFileName[] = "serverHistory.json";
#endif

void CG_ClearServerHistory( void ) {
#ifdef SERVER_HISTORY
	serverHistoryEntry_t *server = serverHistory;

	while ( server ) {
		serverHistoryEntry_t *next = server->next;
		delete server;
		server = next;
	}

	serverHistory = NULL;
#endif
}

#ifdef SERVER_HISTORY
static void CG_ParseServerHistoryJSON( const char *jsonText ) {
	serverHistoryEntry_t *server = NULL;

	cJSON *root = cJSON_Parse( jsonText );
	if ( !root ) {
		trap->Print( "ERROR: Could not parse server history\n" );
		return;
	}

	cJSON *servers = cJSON_GetObjectItem( root, "servers" );
	int serversCount = cJSON_GetArraySize( servers );

	for ( int i = 0; i < serversCount; i++ ) {
		cJSON *item = cJSON_GetArrayItem( servers, i );

		// first, allocate the server data
		// insert it to the start of the linked list, server->next will be the old root
		server = new serverHistoryEntry_t{};
		server->next = serverHistory;
		serverHistory = server;

		// address
		cJSON *address = cJSON_GetObjectItem( item, "address" );
			server->adr.type = (netadrtype_t)cJSON_ToInteger( cJSON_GetObjectItem( address, "type" ) );
			cJSON *ip = cJSON_GetObjectItem( address, "ip" );
			server->adr.ip[0] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 0 ) );
			server->adr.ip[1] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 1 ) );
			server->adr.ip[2] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 2 ) );
			server->adr.ip[3] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 3 ) );
			server->adr.port = cJSON_ToInteger( cJSON_GetObjectItem( address, "port" ) );

		// timestamp
		server->timestamp = cJSON_ToInteger( cJSON_GetObjectItem( item, "timestamp" ) );
	}

	cJSON_Delete( root );
}
#endif

void CG_WriteServerHistoryJSON( void ) {
#ifdef SERVER_HISTORY
	cJSON *root = cJSON_CreateObject();

	cJSON *servers = cJSON_CreateArray();
	for ( serverHistoryEntry_t *server = serverHistory; server; server = server->next ) {
		cJSON *item = cJSON_CreateObject();

		cJSON *address = cJSON_CreateObject();
			cJSON *ip = cJSON_CreateArray();
			cJSON_AddIntegerToArray( ip, server->adr.ip[0] );
			cJSON_AddIntegerToArray( ip, server->adr.ip[1] );
			cJSON_AddIntegerToArray( ip, server->adr.ip[2] );
			cJSON_AddIntegerToArray( ip, server->adr.ip[3] );
			cJSON_AddItemToObject( item, "ip", ip );
		cJSON_AddItemToObject( item, "address", address );

		cJSON_AddItemToArray( servers, item );
	}
	cJSON_AddItemToObject( root, "servers", servers );

	const char *buffer = cJSON_Serialize( root, 1 );

	fileHandle_t f = NULL_FILE;
	trap->FS_Open( historyFileName, &f, FS_WRITE );
	trap->FS_Write( buffer, strlen( buffer ), f );
	trap->FS_Close( f );

	free( (void *)buffer );

	cJSON_Delete( root );
#endif
}

#ifdef SERVER_HISTORY
static void CG_ReadServerHistory( void ) {
	fileHandle_t f = NULL_FILE;

	CG_ClearServerHistory();

	unsigned int len = trap->FS_Open( historyFileName, &f, FS_READ );

	// no file
	if ( !f ) {
		return;
	}

	// empty file
	if ( !len || len == -1 ) {
		trap->FS_Close( f );
		return;
	}
	char *buf = new char[len+1];
	trap->FS_Read( buf, len, f );
	trap->FS_Close( f );
	buf[len] = '\0';

	// pass it off to the json reader
	CG_ParseServerHistoryJSON( buf );

	delete[] buf;
}

static bool BuildByteFromIP( const char *ip, byteAlias_t *out ) {
	int c;

	int i = 0;
	for ( const char *p = ip; *p && i < 4; p++, i++ ) {
		c = 0;
		while ( *p >= '0' && *p <= '9' ) {
			out->b[i] = out->b[i] * 10 + (*p - '0');
			c++;
			p++;
		}

		// check if we parsed any characters
		if ( !c ) {
			return false;
		}

		// check if we've reached the end of the IP
		if ( !*p || *p == ':' )
			break;

		// the next character must be a period
		if ( *p != '.' ) {
			return false;
		}
	}

	if ( i < 3 ) {
		// parser ended prematurely, so abort
		return false;
	}

	return true;
}
#endif

void CG_UpdateServerHistory( void ) {
#ifdef SERVER_HISTORY
	char address[MAX_STRING_CHARS] = {};
	netadr_t netAddress = {};

	// cl_currentServerIP only exists in OpenJK, use cl_currentServerAddress for jamp
	trap->Cvar_VariableStringBuffer( "cl_currentServerIP", address, sizeof(address) );
	if ( address[0] ) {
		byteAlias_t ba;
		if ( BuildByteFromIP( address, &ba ) ) {
			netAddress.ip[0] = ba.b[0];
			netAddress.ip[1] = ba.b[1];
			netAddress.ip[2] = ba.b[2];
			netAddress.ip[3] = ba.b[3];
			netAddress.type = NA_IP;
		}
		else {
			// should never get here
			trap->Print( "Failed to add current server to history, could not parse cl_currentServerIP\n" );
			return;
		}
	}
	else {
		// cl_currentServerIP only exists in OJK
		trap->Cvar_VariableStringBuffer( "cl_currentServerAddress", address, sizeof(address) );
		//TODO: obtain IP from potential hostname
		//optimisation: scan for IP format address and skip hostname resolution if it matches
		byteAlias_t ba;
		if ( BuildByteFromIP( address, &ba ) ) {
			netAddress.ip[0] = ba.b[0];
			netAddress.ip[1] = ba.b[1];
			netAddress.ip[2] = ba.b[2];
			netAddress.ip[3] = ba.b[3];
			netAddress.type = NA_IP;
		}
		else {
			//TODO: hostname resolution
		}
	}

	if ( !serverHistory ) {
		CG_ReadServerHistory();
	}

	for ( serverHistoryEntry_t *server = serverHistory; server; server = server->next ) {
		if ( !Q_CompareNetAddress( &server->adr, &netAddress ) ) {
			uint32_t now = time( NULL );
		#if defined(_DEBUG)
			trap->Print( "Updating server history timestamp for %s (%i -> %i)\n",
					Q_PrintNetAddress( &server->adr ),
					server->timestamp, now
			);
		#endif
			server->timestamp = now;
			//TODO: sort list?
		}
	}
	//	else, append
#endif
}
