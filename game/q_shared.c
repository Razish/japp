// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

/*
-------------------------
GetIDForString 
-------------------------
*/


int GetIDForString ( stringID_table_t *table, const char *string )
{
	int	index = 0;

	while ( ( table[index].name != NULL ) &&
			( table[index].name[0] != 0 ) )
	{
		if ( !Q_stricmp( table[index].name, string ) )
			return table[index].id;

		index++;
	}

	return -1;
}

/*
-------------------------
GetStringForID
-------------------------
*/

const char *GetStringForID( stringID_table_t *table, int id )
{
	int	index = 0;

	while ( ( table[index].name != NULL ) &&
			( table[index].name[0] != 0 ) )
	{
		if ( table[index].id == id )
			return table[index].name;

		index++;
	}

	return NULL;
}

int Com_Clampi( int min, int max, int value ) 
{
	if ( value < min )	return min;
	if ( value > max )	return max;
	return value;
}

float Com_Clamp( float min, float max, float value ) {
	if ( value < min )	return min;
	if ( value > max )	return max;
	return value;
}


/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (char *pathname)
{
	char	*last;
	
	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/

void COM_StripExtension( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '.'), *slash;
	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		Q_strncpyz(out, in, (destsize < dot-in+1 ? destsize : dot-in+1));
	else
		Q_strncpyz(out, in, destsize);
}


/*
==================
COM_DefaultExtension
==================
*/

void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
/*
// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short	(*_BigShort) (short l);
static short	(*_LittleShort) (short l);
static int		(*_BigLong) (int l);
static int		(*_LittleLong) (int l);
static qint64	(*_BigLong64) (qint64 l);
static qint64	(*_LittleLong64) (qint64 l);
static float	(*_BigFloat) (const float *l);
static float	(*_LittleFloat) (const float *l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
qint64 	BigLong64 (qint64 l) {return _BigLong64(l);}
qint64 	LittleLong64 (qint64 l) {return _LittleLong64(l);}
float	BigFloat (const float *l) {return _BigFloat(l);}
float	LittleFloat (const float *l) {return _LittleFloat(l);}
*/

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

qint64 Long64Swap (qint64 ll)
{
	qint64	result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64 Long64NoSwap (qint64 ll)
{
	return ll;
}

typedef union {
    float	f;
    unsigned int i;
} _FloatByteUnion;

float FloatSwap (const float *f) {
	const _FloatByteUnion *in;
	_FloatByteUnion out;

	in = (_FloatByteUnion *)f;
	out.i = LongSwap(in->i);

	return out.f;
}

float FloatNoSwap (const float *f)
{
	return *f;
}

/*
================
Swap_Init
================
*/
/*
void Swap_Init (void)
{
	byte	swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner	
	if ( *(short *)swaptest == 1)
	{
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigLong64 = Long64Swap;
		_LittleLong64 = Long64NoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigLong64 = Long64NoSwap;
		_LittleLong64 = Long64Swap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}
*/

/*
============================================================================

PARSING

============================================================================
*/

static	char	com_token[MAX_TOKEN_CHARS];
static	char	com_parsename[MAX_TOKEN_CHARS];
static	int		com_lines;

void COM_BeginParseSession( const char *name )
{
	com_lines = 0;
	Com_sprintf(com_parsename, sizeof(com_parsename), "%s", name);
}

int COM_GetCurrentParseLine( void )
{
	return com_lines;
}

char *COM_Parse( const char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end (argptr);

	Com_Printf("ERROR: %s, line %d: %s\n", com_parsename, com_lines, string);
}

void COM_ParseWarning( char *format, ... )
{
	va_list argptr;
	static char string[4096];

	va_start (argptr, format);
	Q_vsnprintf( string, sizeof( string ), format, argptr);
	va_end (argptr);

	Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, com_lines, string);
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	int c;

	while( (c = *data) <= ' ') {
		if( !c ) {
			return NULL;
		}
		if( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

int COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;
	
	assert( data_p );

	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
				// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
				// record when we hit a newline
			} else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			} else if ( c == ' ' || c == '\t') {
				whitespace = qtrue;
				in++;
				// an actual token
			} else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace) {
					*out++ = ' ';
					whitespace = qfalse;
				}
				
				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = c;
						in++;
					}
				} else {
					*out = c;
					out++;
					in++;
				}
			}
		}
	}
	*out = 0;
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks )
{
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	const char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			data += 2;
			while (*data && *data != '\n') {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if ( c == '\n' )
			com_lines++;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


#if 0
// no longer used
/*
===============
COM_ParseInfos
===============
*/
int COM_ParseInfos( char *buf, int max, char infos[][MAX_INFO_STRING] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		infos[count][0] = 0;
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( infos[count], key, token );
		}
		count++;
	}

	return count;
}
#endif

/*
===============
COM_ParseString
===============
*/
qboolean COM_ParseString( const char **data, const char **s ) 
{
//	*s = COM_ParseExt( data, qtrue );
	*s = COM_ParseExt( data, qfalse );
	if ( s[0] == 0 ) 
	{
		Com_Printf("unexpected EOF\n");
		return qtrue;
	}
	return qfalse;
}

/*
===============
COM_ParseInt
===============
*/
qboolean COM_ParseInt( const char **data, int *i ) 
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

/*
===============
COM_ParseFloat
===============
*/
qboolean COM_ParseFloat( const char **data, float *f ) 
{
	const char	*token;

	token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*f = atof( token );
	return qfalse;
}

/*
===============
COM_ParseVec4
===============
*/
qboolean COM_ParseVec4( const char **buffer, vector4 *c) 
{
	int i;
	float f;

	for (i = 0; i < 4; i++) 
	{
		if (COM_ParseFloat(buffer, &f)) 
		{
			return qtrue;
		}
		c->data[i] = f;
	}
	return qfalse;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( const char **buf_p, char *match ) {
	char	*token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}


/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
void SkipBracedSection (const char **program) {
	char			*token;
	int				depth;

	depth = 0;
	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine ( const char **data ) {
	const char	*p;
	int		c;

	p = *data;
	while ( (c = *p++) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}


void Parse1DMatrix (const char **buf_p, int x, float *m) {
	char	*token;
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse(buf_p);
		m[i] = atof(token);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix (const char **buf_p, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < y ; i++) {
		Parse1DMatrix (buf_p, x, m + i * x);
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix (const char **buf_p, int z, int y, int x, float *m) {
	int		i;

	COM_MatchToken( buf_p, "(" );

	for (i = 0 ; i < z ; i++) {
		Parse2DMatrix (buf_p, y, x, m + i * x*y);
	}

	COM_MatchToken( buf_p, ")" );
}


/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
		return ( 1 );
	return ( 0 );
}

int Q_islower( int c )
{
	if (c >= 'a' && c <= 'z')
		return ( 1 );
	return ( 0 );
}

int Q_isupper( int c )
{
	if (c >= 'A' && c <= 'Z')
		return ( 1 );
	return ( 0 );
}

int Q_isalpha( int c )
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
		return ( 1 );
	return ( 0 );
}

char* Q_strrchr( const char* string, int c )
{
	char cc = c;
	char *s;
	char *sp=(char *)0;

	s = (char*)string;

	while (*s)
	{
		if (*s == cc)
			sp = s;
		s++;
	}
	if (cc == 0)
		sp = s;

	return sp;
}

/*
=============
Q_strncpyz
 
Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	// bk001129 - also NULL dest
	if ( !dest )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
		return;
	}
	if ( !src )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
		return;
	}

	if ( destsize < 1 )
	{
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
		return;
	}

	strncpy( dest, src, destsize-1 );
	dest[destsize-1] = 0;
}
                 
int Q_stricmpn( const char *s1, const char *s2, int n )
{
	int c1, c2;

	// bk001129 - moved in 1.17 fix not in id codebase
	if ( s1 == NULL ) {
		if ( s2 == NULL )
			return 0;
		else
			return -1;
	}
	else if ( s2==NULL )
		return 1;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0; // strings are equal
}

int Q_strncmp (const char *s1, const char *s2, int n) {
	int		c1, c2;
	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_stricmp (const char *s1, const char *s2) {
	return (s1 && s2) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}


char *Q_strlwr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = tolower(*s);
		s++;
	}
    return s1;
}

char *Q_strupr( char *s1 ) {
    char	*s;

    s = s1;
	while ( *s ) {
		*s = toupper(*s);
		s++;
	}
    return s1;
}


// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}


int Q_PrintStrlen( const char *string ) {
	int			len;
	const char	*p;

	if( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while( *p ) {
		if( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*	d;
	char*	s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		}		
		else if ( c >= 0x20 && c <= 0x7E ) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
Q_strstrip

	Description:	Replace strip[x] in string with repl[x] or remove characters entirely
					Does not strip colours or any characters not specified. See Q_CleanStr
	Mutates:		string
	Return:			--

	Examples:		Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
					Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
*/

void Q_strstrip( char *string, const char *strip, const char *repl )
{
	char		*out=string, *p=string, c;
	const char	*s=strip;
	int			replaceLen = repl?strlen( repl ):0, offset=0;

	while ( (c = *p++) != '\0' )
	{
		for ( s=strip; *s; s++ )
		{
			offset = s-strip;
			if ( c == *s )
			{
				if ( !repl || offset >= replaceLen )
					c = *p++;
				else
					c = repl[offset];
				break;
			}
		}
		*out++ = c;
	}
	*out = '\0';
}

/*
Q_strchrs

	Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
	Mutates:		--
	Return:			first instance of any characters found
					 otherwise NULL
*/

const char *Q_strchrs( const char *string, const char *search )
{
	const char *p = string, *s = search;

	while ( *p != '\0' ) {
		for ( s=search; *s != '\0'; s++ ) {
			if ( *p == *s ) {
				return p;
			}
		}
		p++;
	}

	return NULL;
}

/*
Q_strrep

	Description:	Replace instances of 'old' in 's' with 'new'
	Mutates:		--
	Return:			malloced string containing the replacements
*/

char *Q_strrep( const char *string, const char *substr, const char *replacement ) {
	char *tok = NULL;
	char *newstr = NULL;

	tok = (char *)strstr( string, substr );
	if( tok == NULL ) return strdup( string );
	newstr = (char *)malloc( strlen( string ) - strlen( substr ) + strlen( replacement ) + 1 );
	if( newstr == NULL ) return NULL;
	memcpy( newstr, string, tok - string );
	memcpy( newstr + (tok - string), replacement, strlen( replacement ) );
	memcpy( newstr + (tok - string) + strlen( replacement ), tok + strlen( substr ), strlen( string ) - strlen( substr ) - ( tok - string ) );
	memset( newstr + strlen( string ) - strlen( substr ) + strlen( replacement ), 0, 1 );

	return newstr;
}

char *Q_strrev( char *str ) {
	char *p1, *p2;

	if ( !VALIDSTRING( str ) )
		return str; // could be NULL, or empty string

	for ( p1=str, p2=str+strlen(str)-1; p2 > p1; ++p1, --p2 ) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}

	return str;
}

char *Q_CleanColorStr( char *string ) {
	char *d;
	char *s;
	int c;
	long len = strlen( string );

	s = string;
	d = string;
	while ( len > s-string && (c = *s) != 0 )
	{
		if ( Q_IsColorString( s ) )
			s+=2;
		
		//Oops, went over the end of the string
	//	if ( s-string > len)
	//		break;
		
		*d = *s;

		d++;
		s++;
	}

	*d = '\0';

	return string;
}

/*
============
Q_vsnprintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count 
characters are written and -1 is returned and no trailing '\0' is added.

Q_vsnprintf: always appends a trailing '\0', returns number of characters written (not including terminal \0)
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int Q_vsnprintf( char *str, size_t size, const char *format, va_list args )
{
	int ret;

#ifdef _WIN32
	ret = _vsnprintf( str, size-1, format, args );
#else
	ret = vsnprintf( str, size, format, args );
#endif

	str[size-1] = '\0';

	if ( ret < 0 || ret >= (signed)size )
		return -1;

	return ret;
}

//Raz: Patched version of Com_sprintf
void QDECL Com_sprintf( char *dest, int size, const char *fmt, ...)
{
	int			ret;
	va_list		argptr;

	va_start( argptr, fmt );
	ret = Q_vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );

	if ( ret == -1 )
		Com_Printf( "Com_sprintf: overflow of %i bytes buffer\n", size );
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define	MAX_VA_STRING 32000
#define MAX_VA_BUFFERS 4
#define VA_MASK (MAX_VA_BUFFERS-1)

char *va( const char *format, ... ) {
	va_list		argptr;
	static char	string[MAX_VA_BUFFERS][MAX_VA_STRING];	// in case va is called by nested functions
	static int	index = 0;
	char		*buf;

	va_start( argptr, format );
	buf = (char *)&string[index++ & VA_MASK];
	Q_vsnprintf( buf, MAX_VA_STRING-1, format, argptr );
	va_end( argptr );

	return buf;
}


/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char	pkey[BIG_INFO_KEY];
	static	char value[4][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	int	valueindex = 0;
	char	*o;
	
	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			break;
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char	*o;
	const char	*s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char	*start;
	char	pkey[MAX_INFO_KEY] = {0};
	char	value[MAX_INFO_VALUE] = {0};
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
		return;
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( char *s, const char *key ) {
	char	*start;
	static char pkey[BIG_INFO_KEY] = {0};
	static char value[BIG_INFO_VALUE] = {0};
	char	*o;

	//Raz: moved to heap
	pkey[0] = '\0';
	value[0] = '\0';

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
		return;
	}

	if (strchr (key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}




/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	const char *c = s;

	while ( *c != '\0' )
	{
		if( !Q_isprint( *c ) )
			return qfalse;

		if( *c == '\"' )
			return qfalse;

		if( *c == ';' )
			return qfalse;

		++c;
	}

	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char	newi[MAX_INFO_STRING];

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Printf( "Info_SetValueForKey: Can't use keys or values with a \\\n" );
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Printf( "Info_SetValueForKey: Can't use keys or values with a semicolon\n" );
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Printf( "Info_SetValueForKey: Can't use keys or values with a \"\n" );
		return;
	}

	Info_RemoveKey( s, key );
	if ( !value || !strlen( value ) )
		return;

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) >= MAX_INFO_STRING )
	{
		Com_Printf( "Info_SetValueForKey: Info string length exceeded\n" );
		return;
	}

	strcat( newi, s );
	strcpy( s, newi );
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char	newi[BIG_INFO_STRING];

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	if (strchr (key, '\\') || strchr (value, '\\'))
	{
		Com_Printf ("Can't use keys or values with a \\\n");
		return;
	}

	if (strchr (key, ';') || strchr (value, ';'))
	{
		Com_Printf ("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strchr (key, '\"') || strchr (value, '\"'))
	{
		Com_Printf ("Can't use keys or values with a \"\n");
		return;
	}

	Info_RemoveKey_Big (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) >= BIG_INFO_STRING)
	{
		Com_Printf ("BIG Info string length exceeded\n");
		return;
	}

	strcat (s, newi);
}

//====================================================================


//Raz: Moved this here so all modules can access it
vector4 colorTable[CT_MAX] = 
{
	{ 0.0f,		0.0f,	0.0f,	0.0f },		// CT_NONE
	{ 0.0f,		0.0f,	0.0f,	1.0f },		// CT_BLACK
	{ 1.0f,		0.0f,	0.0f,	1.0f },		// CT_RED
	{ 0.0f,		1.0f,	0.0f,	1.0f },		// CT_GREEN
	{ 0.0f,		0.0f,	1.0f,	1.0f },		// CT_BLUE
	{ 1.0f,		1.0f,	0.0f,	1.0f },		// CT_YELLOW
	{ 1.0f,		0.0f,	1.0f,	1.0f },		// CT_MAGENTA
	{ 0.0f,		1.0f,	1.0f,	1.0f },		// CT_CYAN
	{ 1.0f,		1.0f,	1.0f,	1.0f },		// CT_WHITE
	{ 0.75f,	0.75f,	0.75f,	1.0f },		// CT_LTGREY
	{ 0.50f,	0.50f,	0.50f,	1.0f },		// CT_MDGREY
	{ 0.25f,	0.25f,	0.25f,	1.0f },		// CT_DKGREY
	{ 0.15f,	0.15f,	0.15f,	1.0f },		// CT_DKGREY2
	{ 0.810f,	0.530f,	0.0f,	1.0f },		// CT_VLTORANGE -- needs values
	{ 0.810f,	0.530f,	0.0f,	1.0f },		// CT_LTORANGE
	{ 0.610f,	0.330f,	0.0f,	1.0f },		// CT_DKORANGE
	{ 0.402f,	0.265f,	0.0f,	1.0f },		// CT_VDKORANGE
	{ 0.503f,	0.375f,	0.996f,	1.0f },		// CT_VLTBLUE1
	{ 0.367f,	0.261f,	0.722f,	1.0f },		// CT_LTBLUE1
	{ 0.199f,	0.0f,	0.398f,	1.0f },		// CT_DKBLUE1
	{ 0.160f,	0.117f,	0.324f,	1.0f },		// CT_VDKBLUE1
	{ 0.300f,	0.628f,	0.816f,	1.0f },		// CT_VLTBLUE2 -- needs values
	{ 0.300f,	0.628f,	0.816f,	1.0f },		// CT_LTBLUE2
	{ 0.191f,	0.289f,	0.457f,	1.0f },		// CT_DKBLUE2
	{ 0.125f,	0.250f,	0.324f,	1.0f },		// CT_VDKBLUE2
	{ 0.796f,	0.398f,	0.199f,	1.0f },		// CT_VLTBROWN1 -- needs values
	{ 0.796f,	0.398f,	0.199f,	1.0f },		// CT_LTBROWN1
	{ 0.558f,	0.207f,	0.027f,	1.0f },		// CT_DKBROWN1
	{ 0.328f,	0.125f,	0.035f,	1.0f },		// CT_VDKBROWN1
	{ 0.996f,	0.796f,	0.398f,	1.0f },		// CT_VLTGOLD1 -- needs values
	{ 0.996f,	0.796f,	0.398f,	1.0f },		// CT_LTGOLD1
	{ 0.605f,	0.441f,	0.113f,	1.0f },		// CT_DKGOLD1
	{ 0.386f,	0.308f,	0.148f,	1.0f },		// CT_VDKGOLD1
	{ 0.648f,	0.562f,	0.784f,	1.0f },		// CT_VLTPURPLE1 -- needs values
	{ 0.648f,	0.562f,	0.784f,	1.0f },		// CT_LTPURPLE1
	{ 0.437f,	0.335f,	0.597f,	1.0f },		// CT_DKPURPLE1
	{ 0.308f,	0.269f,	0.375f,	1.0f },		// CT_VDKPURPLE1
	{ 0.816f,	0.531f,	0.710f,	1.0f },		// CT_VLTPURPLE2 -- needs values
	{ 0.816f,	0.531f,	0.710f,	1.0f },		// CT_LTPURPLE2
	{ 0.566f,	0.269f,	0.457f,	1.0f },		// CT_DKPURPLE2
	{ 0.343f,	0.226f,	0.316f,	1.0f },		// CT_VDKPURPLE2
	{ 0.929f,	0.597f,	0.929f,	1.0f },		// CT_VLTPURPLE3
	{ 0.570f,	0.371f,	0.570f,	1.0f },		// CT_LTPURPLE3
	{ 0.355f,	0.199f,	0.355f,	1.0f },		// CT_DKPURPLE3
	{ 0.285f,	0.136f,	0.230f,	1.0f },		// CT_VDKPURPLE3
	{ 0.953f,	0.378f,	0.250f,	1.0f },		// CT_VLTRED1
	{ 0.953f,	0.378f,	0.250f,	1.0f },		// CT_LTRED1
	{ 0.593f,	0.121f,	0.109f,	1.0f },		// CT_DKRED1
	{ 0.429f,	0.171f,	0.113f,	1.0f },		// CT_VDKRED1
	{ 0.25f,	0.0f,	0.0f,	1.0f },		// CT_VDKRED
	{ 0.70f,	0.0f,	0.0f,	1.0f },		// CT_DKRED
	{ 0.717f,	0.902f,	1.0f,	1.0f },		// CT_VLTAQUA
	{ 0.574f,	0.722f,	0.804f,	1.0f },		// CT_LTAQUA
	{ 0.287f,	0.361f,	0.402f,	1.0f },		// CT_DKAQUA
	{ 0.143f,	0.180f,	0.201f,	1.0f },		// CT_VDKAQUA
	{ 0.871f,	0.386f,	0.375f,	1.0f },		// CT_LTPINK
	{ 0.435f,	0.193f,	0.187f,	1.0f },		// CT_DKPINK
	{ 0.0f,		0.5f,	0.5f,	1.0f },		// CT_LTCYAN
	{ 0.0f,		0.25f,	0.25f,	1.0f },		// CT_DKCYAN
	{ 0.179f,	0.51f,	0.92f,	1.0f },		// CT_LTBLUE3
	{ 0.199f,	0.71f,	0.92f,	1.0f },		// CT_LTBLUE3
	{ 0.5f,		0.05f,	0.4f,	1.0f },		// CT_DKBLUE3
	{ 0.0f,		0.613f,	0.097f,	1.0f },		// CT_HUD_GREEN
	{ 0.835f,	0.015f,	0.015f,	1.0f },		// CT_HUD_RED
	{ 0.567f,	0.685f,	1.0f,	0.75f },	// CT_ICON_BLUE
	{ 0.515f,	0.406f,	0.507f,	1.0f },		// CT_NO_AMMO_RED
	{ 1.0f,		0.658f,	0.062f,	1.0f },		// CT_HUD_ORANGE
};


/*
=============
TempVector

This is just a convenience function for making temporary vectors
=============
*/
#define NUM_TEMPVECS 8
static vector3 tempVecs[NUM_TEMPVECS];

vector3 *tv( float x, float y, float z ) {
	static int index;
	vector3 *v = &tempVecs[index++];
	index &= NUM_TEMPVECS-1;

	VectorSet( v, x, y, z );

	return v;
}

/*
=============
VectorToString

This is just a convenience function for printing vectors
=============
*/
static char tempStrs[NUM_TEMPVECS][32];

char *vtos( const vector3 *v ) {
	static int index;
	char *s = tempStrs[index++];
	index &= NUM_TEMPVECS-1;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v->x, (int)v->y, (int)v->z );

	return s;
}
