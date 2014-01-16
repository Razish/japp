/************************************************
|*
|* JKG Ban system
|*
|* This module manages IP bans
|*
|* There is support for subnet bans (using * as wildcard) and temporary bans (with reason)
|* The bans are stored in a file specified by g_banfile (bans.dat by default) in json format
|*
|* Structure sample:
|*

{
	"nextid" : 1,
	"bans" : [{
			"id" : 0,
			"mask" : 7,
			"expire" : 1270666947,
			"reason" : "OMGWUT",
			"ip" : [10, 0, 0, 0]
		}]
}
*/

#include "g_local.h"
#include "shared/json/cJSON.h"

typedef struct banentry_s {
	unsigned int	id;
	byteAlias_t		ip;
	unsigned char	mask;
	unsigned int	expireTime;
	char			banreason[64];
	struct banentry_s *next;
} banentry_t;

static banentry_t *banlist = 0;
static unsigned int nextBanId = 0;

void JKG_Bans_Clear()
{
	banentry_t	*entry;
	banentry_t	*next = 0;

	for ( entry = banlist; entry; entry = next )
	{
		next = entry->next;
		free( entry );
	}

	banlist = 0;
}

void JKG_Bans_LoadBans( void )
{
	char			*buffer;
	fileHandle_t	f;
	unsigned int	len;
	int				i;
	int				banCount;
	cJSON			*root;
	cJSON			*bans;
	cJSON			*item;
	cJSON			*ip;
	banentry_t		*entry;

	len = trap->FS_Open( "bans.dat", &f, FS_READ );
	if ( !len || len == -1 )
		return;

	buffer = malloc( len+1 );
	trap->FS_Read( buffer, len, f );
	trap->FS_Close( f );

	buffer[len] = 0;

	root = cJSON_Parse( buffer );
	free( buffer );

	if ( !root )
	{
		trap->Print( "Error: Could not parse banlist\n" );
		return;
	}
	
	JKG_Bans_Clear();

	nextBanId = cJSON_ToInteger( cJSON_GetObjectItem( root, "nextid" ) );
	bans = cJSON_GetObjectItem( root, "bans" );

	banCount = cJSON_GetArraySize( bans );
	for ( i=0; i<banCount; i++ )
	{
		item	= cJSON_GetArrayItem( bans, i );
		ip		= cJSON_GetObjectItem( item, "ip" );

		entry = (banentry_t *)malloc( sizeof( banentry_t ) );
		
		entry->id			= cJSON_ToInteger( cJSON_GetObjectItem( item, "id" ) );
		entry->mask			= cJSON_ToInteger( cJSON_GetObjectItem( item, "mask" ) );
		entry->expireTime	= cJSON_ToInteger( cJSON_GetObjectItem( item, "expire" ) );
		Q_strncpyz( entry->banreason, cJSON_ToString( cJSON_GetObjectItem( item, "reason" ) ), sizeof( entry->banreason ) );
		entry->ip.b[0]		= cJSON_ToInteger( cJSON_GetArrayItem( ip, 0 ) );
		entry->ip.b[1]		= cJSON_ToInteger( cJSON_GetArrayItem( ip, 1 ) );
		entry->ip.b[2]		= cJSON_ToInteger( cJSON_GetArrayItem( ip, 2 ) );
		entry->ip.b[3]		= cJSON_ToInteger( cJSON_GetArrayItem( ip, 3 ) );

		entry->next = banlist;
		banlist = entry;
	}
	cJSON_Delete( root );
}

void JKG_Bans_Init() {
	JKG_Bans_LoadBans();
}

void JKG_Bans_SaveBans()
{
	cJSON			*root;
	cJSON			*bans;
	cJSON			*item;
	cJSON			*ip;
	const char		*buffer;
	fileHandle_t	f;
	banentry_t		*entry;
	unsigned int	curr = time( NULL );
	
	root = cJSON_CreateObject();
	cJSON_AddIntegerToObject( root, "nextid", nextBanId );

	bans = cJSON_CreateArray();
	
	for ( entry = banlist; entry; entry = entry->next )
	{
		// Don't save expired bans
		if ( entry->expireTime && curr >= entry->expireTime )
			continue;

		item	= cJSON_CreateObject();
		ip		= cJSON_CreateArray();

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

	trap->FS_Open( "bans.dat", &f, FS_WRITE );
	trap->FS_Write( buffer, strlen( buffer ), f );
	trap->FS_Close( f );

	free( (void *)buffer );
	cJSON_Delete( root );
}

/* Adds a ban to the banlist 
|* Duration format:
|*
|* <count><specifier> (eg. '12h' for a 12 hour ban)
|*
|* Specifiers:
|*
|* s: seconds
|* m: minutes
|* h: hours
|* d: days
|* n: months (30 days)
|* y: years  (365 days)
|*
|* Specify a NULL duration or a duration of '0' to make the ban permanent
|*
\*/

/* Same as above, but adds bans by string */
/* A '*' can be used as a wildcard to make range bans (ie 150.10.*.*) */

int JKG_Bans_AddBanString( const char *ip, const char *duration, const char *reason )
{
	byteAlias_t		m;
	unsigned char	mask = 15;
	int				i, c;
	const char		*p;
	unsigned int	expire;
	char			type;
	banentry_t		*entry;

	for ( i=0; i<4; i++ )
		m.b[i] = 0;

	i = 0;
	p = ip;
	while ( *p && i < 4 )
	{
		c = 0;
		if ( *p == '*' )
		{
			mask &= ~(1 << i);
			c++;
			p++;
		}
		else
		{
			while ( *p >= '0' && *p <= '9' )
			{
				m.b[i] = m.b[i]*10 + (*p - '0');
				c++;
				p++;
			}
		}
		// Check if we parsed any characters
		if ( !c )
			return -1;

		// Check if we've reached the end of the IP
		if ( !*p || *p == ':' )
			break;

		// The next character MUST be a period
		if ( *p != '.' )
			return -1;

		i++;
		p++;
	}

	if (i < 3)
	{// If i < 3, the parser ended prematurely, so abort
		return -1;
	}
	
	// Parse expire date
	if (!duration || *duration == '0') {
		expire = 0;
	}
	else
	{
		if ( sscanf( duration, "%u%c", &expire, &type ) != 2 )
		{// Could not interpret the data, so we'll put in 12 hours
			expire = 12;
			type = 'h';
		}
		switch ( type )
		{
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
	for ( entry = banlist; entry; entry = entry->next )
	{
		if ( entry->mask == mask && entry->ip.ui == m.ui )
		{
			entry->expireTime = expire;
			if ( reason )
				Q_strncpyz( entry->banreason, reason, sizeof( entry->banreason ) );

			return entry->id;
		}
	}

	//When in rome...
	entry = (banentry_t *)malloc( sizeof( banentry_t ) );
	entry->id = nextBanId++;
	entry->ip.ui = m.ui;
	entry->expireTime = expire;
	entry->mask = mask;
	if ( reason )
		Q_strncpyz( entry->banreason, reason, sizeof( entry->banreason ) );
	else
		entry->banreason[0] = 0;
	//Fix the link
	entry->next = banlist;
	banlist = entry;

	JKG_Bans_SaveBans();

	return entry->id;
}

// Returns the ban message for the IP
// If the IP is not banned, NULL will be returned

static const char *GetRemainingTime( unsigned int expireTime )
{
	unsigned int	curr = time( NULL );
	unsigned int	diff, days, hours, minutes, seconds;

	if ( curr >= expireTime )
		return "Ban expired";
	
	diff = expireTime - curr;

	days	= diff / 86400;	diff -= (days * 86400);
	hours	= diff / 3600;	diff -= (hours * 3600);
	minutes	= diff / 60;	diff -= (minutes * 60);

	seconds = diff;

	return (days) ? va( "%02i:%02i:%02i", hours, minutes, seconds ) : va( "%i day%s - %02i:%02i:%02i", days, days == 1 ? "" : "s", hours, minutes, seconds );
}

const char *JKG_Bans_IsBanned( byte *ip )
{
	banentry_t	*entry;
	banentry_t	*prev = NULL;

	for ( entry=banlist; entry; prev = entry, entry = entry?entry->next:NULL )
	{// Find the ban entry

		if ( entry->mask & 1 && entry->ip.b[0] != ip[0] ) continue;	//	*.0.0.0
		if ( entry->mask & 2 && entry->ip.b[1] != ip[1] ) continue;	//	0.*.0.0
		if ( entry->mask & 4 && entry->ip.b[2] != ip[2] ) continue;	//	0.0.*.0
		if ( entry->mask & 8 && entry->ip.b[3] != ip[3] ) continue;	//	0.0.0.*

		// If we get here, we got a match
		if ( entry->expireTime )
		{//Temporary ban
			if ( time( NULL ) >= entry->expireTime )
			{// The ban expired, unlink this ban and then continue the search

				if (!prev)	banlist = entry->next;		//	root item
				else		prev->next = entry->next;	//	other-wise, fix the link

				free( (void *)entry );
				entry = prev;
				continue;
			}

			// Temporary ban, calculate the remaining time
			return va( "You have been temporarily banned from this server\nTime remaining: %s\nReason: %s\n", GetRemainingTime( entry->expireTime ), entry->banreason[0] ? entry->banreason : "Not specified" );
		}

		else
		{// Permaban
			return va( "You have been permanently banned from this server\nReason: %s\n", entry->banreason[0] ? entry->banreason : "Not specified" );
		}
	}
	return NULL;
}

byteAlias_t *BuildByteFromIP( const char *ip )
{
	static byteAlias_t	m;
	unsigned char		mask = 15;
	int					i;
	int					c;
	const char			*p;

	for ( i=0; i<4; i++ )
		m.b[i] = 0;

	i = 0;
	p = ip;

	while ( *p && i < 4 )
	{
		c = 0;
		if ( *p == '*' )
		{
			mask &= ~(1 << i);
			c++;
			p++;
		}
		else
		{
			while ( *p >= '0' && *p <= '9' )
			{
				m.b[i] = m.b[i]*10 + (*p - '0');
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
	if (i < 3)
		goto Faulty;

	return &m;

Faulty:
	trap->Print( "Faulty IP: %s\n", ip );
	m.ui = 0u;

	return &m;
}

void JKG_Bans_List( void ) {
	banentry_t	*entry;

	for ( entry=banlist; entry; entry = entry->next ) {
		char buf[MAX_STRING_CHARS] = {0};
		char tmp[16] = {0};
		int i=0;

		Q_strcat( buf, sizeof( buf ), va( "  %3d ", entry->id ) );

		// build IP
		if ( entry->mask & 1 )	Q_strcat( tmp, sizeof( tmp ), "*" );
		else					Q_strcat( tmp, sizeof( tmp ), va( "%i", entry->ip.b[0] ) );
		for ( i=1; i<4; i++ ) {
			if ( entry->mask & (1<<i) )		Q_strcat( tmp, sizeof( tmp ), ".*" );
			else							Q_strcat( tmp, sizeof( tmp ), va( ".%i", entry->ip.b[i] ) );
		}
		Q_strcat( buf, sizeof( buf ), va( "%-20s ", tmp ) );
		tmp[0] = '\0';

		// expire time
		Q_strcat( buf, sizeof( buf ), va( "%-20s ", GetRemainingTime( entry->expireTime ) ) );

		// reason
		Q_strcat( buf, sizeof( buf ), entry->banreason );

		trap->Print( "%s\n", buf );
	}
}

qboolean JKG_Bans_Remove( byte *ip )
{
	banentry_t	*entry;
	banentry_t	*prev = NULL;

	for ( entry=banlist; entry; prev = entry, entry = entry?entry->next:NULL )
	{// Find the ban entry
		if ( entry->mask & 1 && entry->ip.b[0] != ip[0] ) continue;	//	*.0.0.0
		if ( entry->mask & 2 && entry->ip.b[1] != ip[1] ) continue;	//	0.*.0.0
		if ( entry->mask & 4 && entry->ip.b[2] != ip[2] ) continue;	//	0.0.*.0
		if ( entry->mask & 8 && entry->ip.b[3] != ip[3] ) continue;	//	0.0.0.*

		// If we get here, we got a match
		if ( !prev )	banlist = entry->next;		//	root item
		else			prev->next = entry->next;	//	other-wise, fix the link

		free( (void *)entry );

		return qtrue;
	}

	return qfalse;
}
