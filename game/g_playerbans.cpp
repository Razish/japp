// manage IP bans
// there is support for subnet bans (using * as wildcard) and temporary bans (with reason)
// bans are stored in BANFILE as JSON
//
// example:
//	{
//		"nextid" : 1,
//		"bans" : [{
//				"id" : 0,
//				"mask" : 7,
//				"expire" : 1270666947,
//				"reason" : "OMGWUT",
//				"ip" : [10, 0, 0, 0]
//			}]
//	}

#include "g_local.h"
#include "json/cJSON.h"

#define BANFILE "bans.json"

typedef struct banentry_s {
	unsigned int	id;
	byteAlias_t		ip;
	unsigned char	mask;
	unsigned int	expireTime;
	char			banreason[64];
	struct banentry_s *next;
} banentry_t;

static banentry_t *banlist = NULL;
static unsigned int nextBanId = 0;

void JP_Bans_Clear( void ) {
	banentry_t *entry, *next = NULL;

	for ( entry = banlist; entry; entry = next ) {
		next = entry->next;
		free( entry );
	}

	banlist = 0;
}

void JP_Bans_LoadBans( void ) {
	char			*buffer;
	fileHandle_t	f;
	unsigned int	len;
	int				i, banCount;
	cJSON			*root;
	cJSON			*bans;
	cJSON			*item;
	cJSON			*ip;
	banentry_t		*entry;

	len = trap->FS_Open( BANFILE, &f, FS_READ );
	if ( !len || len == -1 )
		return;

	buffer = (char *)malloc( len + 1 );
	trap->FS_Read( buffer, len, f );
	trap->FS_Close( f );

	buffer[len] = 0;

	root = cJSON_Parse( buffer );
	free( buffer );

	if ( !root ) {
		trap->Print( "Error: Could not parse banlist\n" );
		return;
	}

	JP_Bans_Clear();

	nextBanId = cJSON_ToInteger( cJSON_GetObjectItem( root, "nextid" ) );
	bans = cJSON_GetObjectItem( root, "bans" );

	banCount = cJSON_GetArraySize( bans );
	for ( i = 0; i < banCount; i++ ) {
		item = cJSON_GetArrayItem( bans, i );
		ip = cJSON_GetObjectItem( item, "ip" );

		entry = (banentry_t *)malloc( sizeof(banentry_t) );

		entry->id = cJSON_ToInteger( cJSON_GetObjectItem( item, "id" ) );
		entry->mask = cJSON_ToInteger( cJSON_GetObjectItem( item, "mask" ) );
		entry->expireTime = cJSON_ToInteger( cJSON_GetObjectItem( item, "expire" ) );
		Q_strncpyz( entry->banreason, cJSON_ToString( cJSON_GetObjectItem( item, "reason" ) ), sizeof(entry->banreason) );
		entry->ip.b[0] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 0 ) );
		entry->ip.b[1] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 1 ) );
		entry->ip.b[2] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 2 ) );
		entry->ip.b[3] = cJSON_ToInteger( cJSON_GetArrayItem( ip, 3 ) );

		entry->next = banlist;
		banlist = entry;
	}
	cJSON_Delete( root );
}

void JP_Bans_Init( void ) {
	JP_Bans_LoadBans();
}

void JP_Bans_SaveBans( void ) {
	cJSON			*root, *bans, *item, *ip;
	const char		*buffer;
	fileHandle_t	f;
	banentry_t		*entry;
	unsigned int	curr = time( NULL );

	root = cJSON_CreateObject();
	cJSON_AddIntegerToObject( root, "nextid", nextBanId );

	bans = cJSON_CreateArray();

	for ( entry = banlist; entry; entry = entry->next ) {
		// Don't save expired bans
		if ( entry->expireTime && curr >= entry->expireTime )
			continue;

		item = cJSON_CreateObject();
		ip = cJSON_CreateArray();

		cJSON_AddIntegerToObject( item, "id", entry->id );
		cJSON_AddIntegerToObject( item, "mask", entry->mask );
		cJSON_AddIntegerToObject( item, "expire", entry->expireTime );
		cJSON_AddStringToObject( item, "reason", entry->banreason );
		cJSON_AddIntegerToArray( ip, entry->ip.b[0] );
		cJSON_AddIntegerToArray( ip, entry->ip.b[1] );
		cJSON_AddIntegerToArray( ip, entry->ip.b[2] );
		cJSON_AddIntegerToArray( ip, entry->ip.b[3] );
		cJSON_AddItemToObject( item, "ip", ip );

		cJSON_AddItemToArray( bans, item );
	}

	cJSON_AddItemToObject( root, "bans", bans );

	buffer = cJSON_Serialize( root, 1 );

	trap->FS_Open( BANFILE, &f, FS_WRITE );
	trap->FS_Write( buffer, strlen( buffer ), f );
	trap->FS_Close( f );

	free( (void *)buffer );
	cJSON_Delete( root );
}

// Specify a NULL duration or a duration of '0' to make the ban permanent
// * can be used as a wildcard to make range bans (ie 150.10.*.*)
int JP_Bans_AddBanString( const char *ip, const char *duration, const char *reason, char *failedMsg, size_t msgLen ) {
	byteAlias_t		m;
	unsigned char	mask = 0u;
	int				i, c;
	const char		*p;
	unsigned int	expire;
	char			type;
	banentry_t		*entry;

	m.ui = 0u;

	i = 0;
	p = ip;
	while ( *p && i < 4 ) {
		c = 0;
		if ( *p == '*' ) {
			mask |= (1 << i);
			c++;
			p++;
		}
		else {
			while ( *p >= '0' && *p <= '9' ) {
				m.b[i] = m.b[i] * 10 + (*p - '0');
				c++;
				p++;
			}
		}
		// Check if we parsed any characters
		if ( !c ) {
			if ( failedMsg ) {
				Q_strncpyz( failedMsg, "Missing IP argument", msgLen );
			}
			return -1;
		}

		// Check if we've reached the end of the IP
		if ( !*p || *p == ':' )
			break;

		// The next character MUST be a period
		if ( *p != '.' ) {
			if ( failedMsg ) {
				Q_strncpyz( failedMsg, "Invalid IP format", msgLen );
			}
			return -1;
		}

		i++;
		p++;
	}

	// If i < 3, the parser ended prematurely, so abort
	if ( i < 3 ) {
		if ( failedMsg ) {
			Com_sprintf( failedMsg, msgLen, "Incomplete IP: %s", ip );
		}
		return -1;
	}

	// Parse expire date
	if ( !duration || *duration == '0' )
		expire = 0;
	else {
		if ( sscanf( duration, "%u%c", &expire, &type ) != 2 ) {// Could not interpret the data, so we'll put in 12 hours
			expire = 12;
			type = 'h';
		}
		switch ( type ) {
		case 'm':
			expire *= 60;
			break;

		// Assume seconds by default
		default:
		case 's':
			break;

		case 'h': // hours
			expire *= 3600;
			break;

		case 'd': // days
			expire *= 86400;
			break;

		case 'M': // months
			expire *= 2592000;
			break;

		case 'y': // years
			expire *= 31536000;
			break;
		}
		expire += time( NULL );
	}

	// Check if this ban already exists
	for ( entry = banlist; entry; entry = entry->next ) {
		if ( entry->mask == mask && entry->ip.ui == m.ui ) {
			entry->expireTime = expire;
			if ( reason )
				Q_strncpyz( entry->banreason, reason, sizeof(entry->banreason) );
			return entry->id;
		}
	}

	entry = (banentry_t *)malloc( sizeof(banentry_t) );
	entry->id = nextBanId++;
	entry->ip.ui = m.ui;
	entry->expireTime = expire;
	entry->mask = mask;
	if ( reason ) {
		Q_strncpyz( entry->banreason, reason, sizeof(entry->banreason) );
	}
	else {
		entry->banreason[0] = '\0';
	}
	//Fix the link
	entry->next = banlist;
	banlist = entry;

	JP_Bans_SaveBans();

	return entry->id;
}

// Returns the ban message for the IP
// If the IP is not banned, NULL will be returned

static const char *GetRemainingTime( unsigned int expireTime ) {
	unsigned int diff, years, months, days, hours, minutes, seconds, curr = time( NULL );

	if ( !expireTime )
		return "Permanent";

	if ( curr >= expireTime )
		return "Ban expired";

	diff = expireTime - curr;

	//TODO: this doesn't consider how many days are in each month, leap years, etc. not really a major problem, but
	//	worth considering in the future
	years = diff / 31536000;	diff -= (years * 31536000);
	months = diff / 2592000;	diff -= (months * 2592000);
	days = diff / 86400;		diff -= (days * 86400);
	hours = diff / 3600;		diff -= (hours * 3600);
	minutes = diff / 60;		diff -= (minutes * 60);
	seconds = diff;

	return va( "%01iy%02im%02id %02i:%02i:%02i", years, months, days, hours, minutes, seconds );
}

const char *JP_Bans_IsBanned( byte *ip ) {
	banentry_t *entry, *prev = NULL;

	for ( entry = banlist; entry; prev = entry, entry = entry ? entry->next : NULL ) {// Find the ban entry

		if ( !(entry->mask & 1) && entry->ip.b[0] != ip[0] ) continue;	//	*.0.0.0
		if ( !(entry->mask & 2) && entry->ip.b[1] != ip[1] ) continue;	//	0.*.0.0
		if ( !(entry->mask & 4) && entry->ip.b[2] != ip[2] ) continue;	//	0.0.*.0
		if ( !(entry->mask & 8) && entry->ip.b[3] != ip[3] ) continue;	//	0.0.0.*

		// If we get here, we got a match
		if ( entry->expireTime ) {//Temporary ban
			if ( time( NULL ) >= entry->expireTime ) {// The ban expired, unlink this ban and then continue the search

				if ( !prev )	banlist = entry->next;		//	root item
				else		prev->next = entry->next;	//	other-wise, fix the link

				free( (void *)entry );
				entry = prev;
				continue;
			}

			// Temporary ban, calculate the remaining time
			return va( "You have been temporarily banned from this server\nTime remaining: %s\nReason: %s\n", GetRemainingTime( entry->expireTime ), entry->banreason[0] ? entry->banreason : "Not specified" );
		}

		else {// Permaban
			return va( "You have been permanently banned from this server\nReason: %s\n", entry->banreason[0] ? entry->banreason : "Not specified" );
		}
	}
	return NULL;
}

byteAlias_t *BuildByteFromIP( const char *ip ) {
	static byteAlias_t	m;
	unsigned char		mask = 0u;
	int					i, c;
	const char			*p;

	for ( i = 0; i < 4; i++ )
		m.b[i] = 0;

	i = 0;
	p = ip;

	while ( *p && i < 4 ) {
		c = 0;
		if ( *p == '*' ) {
			mask |= (1 << i);
			c++;
			p++;
		}
		else {
			while ( *p >= '0' && *p <= '9' ) {
				m.b[i] = m.b[i] * 10 + (*p - '0');
				c++;
				p++;
			}
		}

		// Check if we parsed any characters
		if ( !c )
			goto Faulty;

		// Check if we've reached the end of the IP
		if ( !*p || *p == ':' )
			break;

		// The next character MUST be a period
		if ( *p != '.' )
			goto Faulty;

		i++;
		p++;
	}

	// If i < 3, the parser ended prematurely, so abort
	if ( i < 3 )
		goto Faulty;

	return &m;

Faulty:
	trap->Print( "Faulty IP: %s\n", ip );
	m.ui = 0u;

	return &m;
}

void JP_Bans_List( void ) {
	banentry_t *entry;
	int numBans = 0;

	//TODO: table header

	for ( entry = banlist; entry; entry = entry->next ) {
		char buf[MAX_STRING_CHARS] = { 0 };
		char tmp[16] = { 0 };
		int i = 0;

		Q_strcat( buf, sizeof(buf), va( " #%03d ", ++numBans ) );

		// build IP
		if ( entry->mask & 1 )	Q_strcat( tmp, sizeof(tmp), "*" );
		else					Q_strcat( tmp, sizeof(tmp), va( "%i", entry->ip.b[0] ) );
		for ( i = 1; i < 4; i++ ) {
			if ( entry->mask & (1 << i) )	Q_strcat( tmp, sizeof(tmp), ".*" );
			else							Q_strcat( tmp, sizeof(tmp), va( ".%i", entry->ip.b[i] ) );
		}
		Q_strcat( buf, sizeof(buf), va( "%-20s ", tmp ) );
		tmp[0] = '\0';

		// expire time
		Q_strcat( buf, sizeof(buf), va( "%-20s ", GetRemainingTime( entry->expireTime ) ) );

		// reason
		Q_strcat( buf, sizeof(buf), entry->banreason );

		trap->Print( "%s\n", buf );
	}
}

qboolean JP_Bans_Remove( byte *ip ) {
	banentry_t	*entry;
	banentry_t	*prev = NULL;

	for ( entry = banlist; entry; prev = entry, entry = entry ? entry->next : NULL ) {// Find the ban entry
		if ( !(entry->mask & 1) && entry->ip.b[0] != ip[0] ) continue;	//	*.0.0.0
		if ( !(entry->mask & 2) && entry->ip.b[1] != ip[1] ) continue;	//	0.*.0.0
		if ( !(entry->mask & 4) && entry->ip.b[2] != ip[2] ) continue;	//	0.0.*.0
		if ( !(entry->mask & 8) && entry->ip.b[3] != ip[3] ) continue;	//	0.0.0.*

		// If we get here, we got a match
		if ( !prev )	banlist = entry->next;		//	root item
		else			prev->next = entry->next;	//	other-wise, fix the link

		free( (void *)entry );

		return qtrue;
	}

	return qfalse;
}
