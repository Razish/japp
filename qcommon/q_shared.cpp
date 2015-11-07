// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "qcommon/q_shared.h"
#include "cJSON/cJSON.h"

#ifdef PROJECT_GAME
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#elif defined(PROJECT_UI)
#include "ui_local.h"
#endif

#if defined(_WIN32)
#endif

#if defined( PROJECT_GAME ) || defined( PROJECT_CGAME ) || defined( PROJECT_UI )
void( *Com_Error )(int level, const char *error, ...);
void( *Com_Printf )(const char *msg, ...);
#endif

int GetIDForString( const stringID_table_t *table, const char *string ) {
	const stringID_table_t *t;

	for ( t = table; VALIDSTRING( t->name ); t++ ) {
		if ( !Q_stricmp( t->name, string ) )
			return t->id;
	}

	return -1;
}

const char *GetStringForID( const stringID_table_t *table, int id ) {
	const stringID_table_t *t;

	for ( t = table; VALIDSTRING( t->name ); t++ ) {
		if ( t->id == id )
			return t->name;
	}

	return NULL;
}

float Q_clamp( float min, float value, float max ) {
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

int Q_clampi( int min, int value, int max ) {
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

// min
float Q_cap( float value, float max ) {
	return (value > max) ? max : value;
}

int Q_capi( int value, int max ) {
	return (value > max) ? max : value;
}

// max
float Q_bump( float min, float value ) {
	return (value < min) ? min : value;
}

int Q_bumpi( int min, int value ) {
	return (value < min) ? min : value;
}

char *COM_SkipPath( char *pathname ) {
	char *last;

	for ( last = pathname; *pathname; pathname++ ) {
		if ( *pathname == '/' )
			last = pathname + 1;
	}

	return last;
}

void COM_StripExtension( const char *in, char *out, int destsize ) {
	const char *dot = strrchr( in, '.' ), *slash;
	if ( dot && (!(slash = strrchr( in, '/' )) || slash < dot) )
		destsize = (destsize < dot - in + 1 ? destsize : dot - in + 1);

	if ( in == out && destsize > 1 )
		out[destsize - 1] = '\0';
	else
		Q_strncpyz( out, in, destsize );
}

void COM_DefaultExtension( char *path, int maxSize, const char *extension ) {
	const char *dot = strrchr( path, '.' ), *slash;
	if ( dot && (!(slash = strrchr( path, '/' )) || slash < dot) )
		return;
	else
		Q_strcat( path, maxSize, extension );
}

short ShortSwap( short l ) {
	byte b1, b2;

	b1 = l & 255;
	b2 = (l >> 8) & 255;

	return (b1 << 8) + b2;
}

short ShortNoSwap( short l ) {
	return l;
}

int LongSwap( int l ) {
	byte b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l >> 8) & 255;
	b3 = (l >> 16) & 255;
	b4 = (l >> 24) & 255;

	return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
}

int	LongNoSwap( int l ) {
	return l;
}

void Long64Swap( qint64_t *l64 ) {
	qint64_t result;

	result.b0 = l64->b7;
	result.b1 = l64->b6;
	result.b2 = l64->b5;
	result.b3 = l64->b4;
	result.b4 = l64->b3;
	result.b5 = l64->b2;
	result.b6 = l64->b1;
	result.b7 = l64->b0;

	*l64 = result;
}

void Long64NoSwap( qint64_t *ll ) {
}

float FloatSwap( const float *f ) {
	byteAlias_t ba;

	ba.f = *f;
	ba.i = LongSwap( ba.i );

	return ba.f;
}

float FloatNoSwap( const float *f ) {
	return *f;
}

short BigShort( short l ) {
#if defined(MACOS_X)
	return l;
#else
	return ShortSwap( l );
#endif
}

short LittleShort( short l ) {
#if defined(MACOS_X)
	return ShortSwap( l );
#else
	return l;
#endif
}

int BigLong( int l ) {
#if defined(MACOS_X)
	return l;
#else
	return LongSwap( l );
#endif
}

int LittleLong( int l ) {
#if defined(MACOS_X)
	return LongSwap( l );
#else
	return l;
#endif
}

float BigFloat( const float *l ) {
#if defined(MACOS_X)
	return *l;
#else
	return FloatSwap( l );
#endif
}

float LittleFloat( const float l ) {
#if defined(MACOS_X)
	return FloatSwap( &l );
#else
	return l;
#endif
}

static char com_token[MAX_TOKEN_CHARS];
static char com_parsename[MAX_TOKEN_CHARS];
static uint32_t com_lines;

void COM_BeginParseSession( const char *name ) {
	com_lines = 0u;
	Q_strncpyz( com_parsename, name, sizeof(com_parsename) );
}

uint32_t COM_GetCurrentParseLine( void ) {
	return com_lines;
}

char *COM_Parse( const char **data_p ) {
	return COM_ParseExt( data_p, qtrue );
}

void COM_ParseError( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof(string), format, argptr );
	va_end( argptr );

	Com_Printf( "ERROR: %s, line %d: %s\n", com_parsename, com_lines, string );
}

void COM_ParseWarning( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof(string), format, argptr );
	va_end( argptr );

	Com_Printf( "WARNING: %s, line %d: %s\n", com_parsename, com_lines, string );
}

// parse a token out of a string
//	will never return NULL, just empty strings
// if allowLineBreaks is qtrue then an empty string will be returned if the next token is a newline.
const char *SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	char c;

	while ( (c = *data) <= ' ' ) {
		if ( !c ) {
			return NULL;
		}

		if ( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

// compresses the buffer in-place and adds a null terminator
// returns the strlen of the new buffer
ptrdiff_t COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	assert( data_p );

	in = out = data_p;
	if ( in ) {
		while ( (c = *in) != '\0' ) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while ( *in && *in != '\n' ) {
					in++;
				}
			}

			// skip /* */ comments
			else if ( c == '/' && in[1] == '*' ) {
				while ( *in && (*in != '*' || in[1] != '/') ) {
					in++;
				}
				if ( *in ) {
					in += 2;
				}
			}

			// record when we hit a newline
			else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
			}

			// record when we hit whitespace
			else if ( c == ' ' || c == '\t' ) {
				whitespace = qtrue;
				in++;
				// an actual token
			}
			else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if ( newline ) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				}
				if ( whitespace ) {
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if ( c == '"' ) {
					*out++ = c;
					in++;
					while ( 1 ) {
						c = *in;
						if ( c && c != '"' ) {
							*out++ = c;
							in++;
						}
						else {
							break;
						}
					}
					if ( c == '"' ) {
						*out++ = c;
						in++;
					}
				}
				else {
					*out = c;
					out++;
					in++;
				}
			}
		}
	}
	*out = '\0';
	return out - data_p;
}

char *COM_ParseExt( const char **data_p, qboolean allowLineBreaks ) {
	char c = '\0';
	size_t len = 0u;
	qboolean hasNewLines = qfalse;
	const char *data = *data_p;

	com_token[0] = '\0';

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data ) {
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			data += 2;
			while ( *data && *data != '\n' ) {
				data++;
			}
		}

		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			data += 2;
			while ( *data && (*data != '*' || data[1] != '/') ) {
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		}

		else {
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 ) {
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[len] = '\0';
				*data_p = (char *)data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS ) {
				com_token[len++] = c;
			}
		}
	}

	// parse a regular word
	do {
		if ( len < MAX_TOKEN_CHARS ) {
			com_token[len++] = c;
		}
		data++;
		c = *data;
		if ( c == '\n' ) {
			com_lines++;
		}
	} while ( c > 32 );

	if ( len == MAX_TOKEN_CHARS ) {
		Com_Printf( "Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS );
		len = 0u;
	}
	com_token[len] = '\0';

	*data_p = (char *)data;
	return com_token;
}

qboolean COM_ParseString( const char **data, const char **s ) {
	*s = COM_ParseExt( data, qfalse );
	if ( s[0] == 0 ) {
		assert( !"unexpected EOF" );
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	return qfalse;
}

qboolean COM_ParseInt( const char **data, int *i ) {
	const char *token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) {
		assert( !"unexpected EOF" );
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

qboolean COM_ParseFloat( const char **data, float *f ) {
	const char *token = COM_ParseExt( data, qfalse );
	if ( token[0] == 0 ) {
		assert( !"unexpected EOF" );
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	*f = atof( token );
	return qfalse;
}

qboolean COM_ParseVector( const char **buffer, vector3 *c ) {
	int i;
	float f;

	for ( i = 0; i < 3; i++ ) {
		if ( COM_ParseFloat( buffer, &f ) ) {
			return qtrue;
		}
		c->raw[i] = f;
	}

	return qfalse;
}

void COM_MatchToken( const char **buf_p, const char *match ) {
	char *token = COM_Parse( buf_p );

	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}

// The next token should be an open brace.
// Skips until a matching close brace is found.
// Internal brace depths are properly skipped.
void SkipBracedSection( const char **program ) {
	char *token;
	int depth = 0;

	do {
		token = COM_ParseExt( program, qtrue );
		if ( token[1] == '\0' ) {
			if ( token[0] == '{' )
				depth++;
			else if ( token[0] == '}' )
				depth--;
		}
	} while ( depth && *program );
}

void SkipRestOfLine( const char **data ) {
	const char *p;
	int c;

	p = *data;
	while ( (c = *p) != '\0' ) {
		p++;
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}

void Parse1DMatrix( const char **buf_p, int x, float *m ) {
	char *token;
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < x; i++ ) {
		token = COM_Parse( buf_p );
		m[i] = atof( token );
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix( const char **buf_p, int y, int x, float *m ) {
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < y; i++ )
		Parse1DMatrix( buf_p, x, m + i*x );

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix( const char **buf_p, int z, int y, int x, float *m ) {
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < z; i++ )
		Parse2DMatrix( buf_p, y, x, m + i*(x*y) );

	COM_MatchToken( buf_p, ")" );
}

qboolean Q_isprint( int c ) {
	return (c >= 0x20 && c <= 0x7E) ? qtrue : qfalse;
}

qboolean Q_islower( int c ) {
	return (c >= 'a' && c <= 'z') ? qtrue : qfalse;
}

qboolean Q_isupper( int c ) {
	return (c >= 'A' && c <= 'Z') ? qtrue : qfalse;
}

qboolean Q_isalpha( int c ) {
	return (Q_islower( c ) || Q_isupper( c )) ? qtrue : qfalse;
}

// true if s is a number (real or integer)
qboolean Q_StringIsNumber( const char *s ) {
	char *p;
	double ret;

	if ( *s == '\0' )
		return qfalse;

	ret = strtod( s, &p );

	if ( ret == HUGE_VAL || errno == ERANGE )
		return qfalse;

	return (*p == '\0') ? qtrue : qfalse;
}

// true if s is an integer
qboolean Q_StringIsInteger( const char *s ) {
	int i, len;
	qboolean foundDigit = qfalse;

	for ( i = 0, len = strlen( s ); i < len; i++ ) {
		if ( i == 0 && s[i] == '-' ) {
			continue;
		}
		if ( !isdigit( s[i] ) ) {
			return qfalse;
		}

		foundDigit = qtrue;
	}

	return foundDigit;
}

// returns true if f is an integer
qboolean Q_isintegral( float f ) {
	return ((int)f == f) ? qtrue : qfalse;
}

// returns last instance of a character in a string
char *Q_strrchr( const char *string, char c ) {
	size_t len = strlen( string );
	char *p = (char *)string + len;
	while ( p >= string ) {
		if ( *p == c )
			return p;
		p--;
	}

	return NULL;
}

// copy string and ensure a trailing null terminator
void Q_strncpyz( char *dest, const char *src, int destsize ) {
	if ( !dest ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
		return;
	}
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: destsize < 1" );
		return;
	}

	strncpy( dest, src, destsize - 1 );
	dest[destsize - 1] = 0;
}

// case insensitive length compare
int Q_stricmpn( const char *s1, const char *s2, int n ) {
	int c1, c2;

	if ( !s1 ) {
		if ( !s2 )
			return 0;
		else
			return -1;
	}
	else if ( !s2 )
		return 1;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
			return 0; // strings are equal until end point

		if ( c1 != c2 ) {
			if ( Q_islower( c1 ) ) c1 -= ('a' - 'A');
			if ( Q_islower( c2 ) ) c2 -= ('a' - 'A');

			if ( c1 != c2 )
				return c1 < c2 ? -1 : 1;
		}
	} while ( c1 );

	return 0; // strings are equal
}

// case sensitive length compare
int Q_strncmp( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
			return 0; // strings are equal until end point

		if ( c1 != c2 )
			return c1 < c2 ? -1 : 1;
	} while ( c1 );

	return 0; // strings are equal
}

// case insensitive compare
int Q_stricmp( const char *s1, const char *s2 ) {
	return (s1 && s2) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}

// convert string to lower-case
void Q_strlwr( char *s1 ) {
	char *s;

	for ( s = s1; *s; s++ )
		*s = tolower( *s );
}

// convert string to upper-case
void Q_strupr( char *s1 ) {
	char *s;

	for ( s = s1; *s; s++ )
		*s = toupper( *s );
}

// concatenate string (size safe, ensures trailing null terminator)
void Q_strcat( char *dest, int size, const char *src ) {
	int len;

	len = strlen( dest );
	if ( len >= size )
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	Q_strncpyz( dest + len, src, size - len );
}

// find the first occurrence of find in s
const char *Q_stristr( const char *s, const char *find ) {
	char c, sc;
	size_t len;

	if ( (c = *find++) != 0 ) {
		if ( c >= 'a' && c <= 'z' )
			c -= ('a' - 'A');
		len = strlen( find );
		do {
			do {
				if ( (sc = *s++) == 0 )
					return NULL;
				if ( sc >= 'a' && sc <= 'z' )
					sc -= ('a' - 'A');
			} while ( sc != c );
		} while ( Q_stricmpn( s, find, len ) );
		s--;
	}

	return s;
}

// string length not counting colour codes
int Q_PrintStrlen( const char *string ) {
	int len;
	const char *p;

	if ( !string )
		return 0;

	for ( p = string, len = 0; *p; /**/ ) {
		if ( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

// replace or strip characters in a string
// replace strip[x] in string with repl[x] or remove characters entirely
// does not strip colours or any characters not specified. See Q_CleanString
// mutates string
// Examples:	Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "123" );	// "Bo1b is h2airy33"
//				Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", "12" );	// "Bo1b is h2airy"
//				Q_strstrip( "Bo\nb is h\rairy!!", "\n\r!", NULL );	// "Bob is hairy"
void Q_strstrip( char *string, const char *strip, const char *repl ) {
	char		*out = string, *p = string, c;
	const char	*s = strip;
	int			replaceLen = repl ? strlen( repl ) : 0, offset = 0;

	while ( (c = *p++) != '\0' ) {
		for ( s = strip; *s; s++ ) {
			offset = s - strip;
			if ( c == *s ) {
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

// find first instance of specified characters, or NULL if none are found
const char *Q_strchrs( const char *string, const char *search ) {
	const char *p = string, *s = search;

	while ( *p != '\0' ) {
		for ( s = search; *s != '\0'; s++ ) {
			if ( *p == *s )
				return p;
		}
		p++;
	}

	return NULL;
}

// replace strings in a string
// caller must free the return value
char *Q_strrep( const char *subject, const char *search, const char *replace ) {
	char *tok = NULL, *newstr = NULL;
	size_t searchLen, replaceLen;

	if ( !(tok = (char *)strstr( subject, search )) )
		return strdup( subject );

	searchLen = strlen( search );
	replaceLen = strlen( replace );

	newstr = (char *)malloc( strlen( subject ) - searchLen + replaceLen + 1 );
	if ( !newstr )
		return NULL;

	memcpy( newstr, subject, tok - subject );
	memcpy( newstr + (tok - subject), replace, replaceLen );
	memcpy( newstr + (tok - subject) + replaceLen, tok + searchLen, strlen( subject ) - searchLen - (tok - subject) );
	memset( newstr + strlen( subject ) - searchLen + replaceLen, 0, 1 );

	return newstr;
}

// reverse a string in place
void Q_strrev( char *str ) {
	char *p1, *p2;

	if ( !VALIDSTRING( str ) )
		return;

	for ( p1 = str, p2 = str + strlen( str ) - 1;
		p2 > p1;
		++p1, --p2 ) {
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
}

// removes extended ASCII and Q3 colour codes
// use STRIP_*** for flags
void Q_CleanString( char *string, uint32_t flags ) {
	qboolean doPass = qtrue;
	char *r, *w; // read, write

	while ( doPass ) {
		doPass = qfalse;
		r = w = string;
		while ( *r ) {
			if ( (flags & STRIP_COLOUR) && Q_IsColorStringExt( r ) ) {
				doPass = qtrue;
				r += 2;
			}
			else if ( (flags & STRIP_EXTASCII) && (*r < 0x20 || *r > 0x7E) )
				r++;
			else {
				// Avoid writing the same data over itself
				if ( w != r )
					*w = *r;
				w++, r++;
			}
		}
		// Add trailing NUL byte if string has shortened
		if ( w < r )
			*w = '\0';
	}
}

void Q_ConvertLinefeeds( char *string ) {
	qboolean doPass = qtrue;
	char *r, *w; // read, write

	while ( doPass ) {
		doPass = qfalse;
		r = w = string;
		while ( *r ) {
			if ( *r == '\\' && *(r + 1) && *(r + 1) == 'n' ) {
				doPass = qtrue;
				*w = '\n';
				r += 2;
				w++;
			}
			else {
				// Avoid writing the same data over itself
				if ( w != r )
					*w = *r;
				w++, r++;
			}
		}
		// Add trailing NUL byte if string has shortened
		if ( w < r )
			*w = '\0';
	}
}

// C99 standard: vsnprintf returns the number of characters (excluding the trailing '\0') which would have been written
//	to the final string if enough space had been available
//	snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')
// win32: _vsnprintf returns the number of characters written, not including the terminating null character, or a negative
//	value if an output error occurs. If the number of characters to write exceeds count, then count characters are written
//	and -1 is returned and no trailing '\0' is added.
// Q_vsnprintf: always appends a trailing '\0', returns number of characters written (not including terminal \0) or returns
//	-1 on failure or if the buffer would be overflowed.
int Q_vsnprintf( char *str, size_t size, const char *format, va_list args ) {
	int ret = vsnprintf( str, size, format, args );

	if ( ret < 0 || ret == (int)size ) {
		str[size - 1] = '\0';
		return (int)size;
	}

	return ret;
}

// format a string into a buffer
void Com_sprintf( char *dest, int size, const char *fmt, ... ) {
	int ret;
	va_list argptr;

	va_start( argptr, fmt );
	ret = Q_vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );

	if ( ret == -1 )
		Com_Printf( "Com_sprintf: overflow of %i bytes buffer\n", size );
}

#define	MAX_VA_STRING (1024*32)
#define MAX_VA_BUFFERS 4
#define VA_MASK (MAX_VA_BUFFERS-1)

// varargs format buffer
// uses a circular buffer, copy after use
const char *va( const char *format, ... ) {
	va_list argptr;
	static char string[MAX_VA_BUFFERS][MAX_VA_STRING]; // in case va is called by nested functions
	static int index = 0;
	char *buf;

	va_start( argptr, format );
	buf = (char *)&string[index++ & VA_MASK];
	Q_vsnprintf( buf, MAX_VA_STRING - 1, format, argptr );
	va_end( argptr );

	return buf;
}

// Searches the string for the given key and returns the associated value, or an empty string.
//	FIXME: overflow check?
const char *Info_ValueForKey( const char *s, const char *key ) {
	char *o, pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE]; // use two buffers so compares work without stomping on each other
	static int vIndex = 0;

	if ( !s || !key )
		return "";

	if ( strlen( s ) >= BIG_INFO_STRING )
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );

	vIndex ^= 1;
	if ( *s == '\\' )
		s++;

	while ( 1 ) {
		o = pkey;
		while ( *s != '\\' ) {
			if ( !*s )
				return "";
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value[vIndex];

		while ( *s != '\\' && *s )
			*o++ = *s++;
		*o = 0;

		if ( !Q_stricmp( key, pkey ) )
			return value[vIndex];

		if ( !*s )
			break;
		s++;
	}

	return "";
}

// Used to iterate through all the key/value pairs in an info string
void Info_NextPair( const char **head, char *key, char *value ) {
	char *o;
	const char *s;

	s = *head;

	if ( *s == '\\' )
		s++;

	key[0] = value[0] = '\0';

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
	while ( *s != '\\' && *s )
		*o++ = *s++;
	*o = 0;

	*head = s;
}

void Info_RemoveKey( char *s, const char *key ) {
	char *start, *o, pkey[MAX_INFO_KEY], value[MAX_INFO_VALUE];

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
		return;
	}

	if ( strchr( key, '\\' ) )
		return;

	while ( 1 ) {
		start = s;
		if ( *s == '\\' )
			s++;
		o = pkey;
		while ( *s != '\\' ) {
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value;
		while ( *s != '\\' && *s ) {
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = '\0';

		if ( !strcmp( key, pkey ) ) {
			memmove( start, s, strlen( s ) + 1 ); // remove this part
			return;
		}

		if ( !*s )
			return;
	}
}

void Info_RemoveKey_Big( char *s, const char *key ) {
	char *start, *o;
	static char pkey[BIG_INFO_KEY], value[BIG_INFO_VALUE];

	pkey[0] = value[0] = '\0';

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
		return;
	}

	if ( strchr( key, '\\' ) )
		return;

	while ( 1 ) {
		start = s;
		if ( *s == '\\' )
			s++;
		o = pkey;
		while ( *s != '\\' ) {
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = '\0';
		s++;

		o = value;
		while ( *s != '\\' && *s ) {
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = '\0';

		if ( !strcmp( key, pkey ) ) {
			memmove( start, s, strlen( s ) + 1 ); // remove this part
			return;
		}

		if ( !*s )
			return;
	}

}

// Some characters are illegal in info strings because they can mess up the server's parsing
qboolean Info_Validate( const char *s ) {
	const char *c;

	for ( c = s; *c != '\0'; c++ ) {
		if ( !Q_isprint( *c ) || *c == '\"' || *c == ';' )
			return qfalse;
	}

	return qtrue;
}

// Changes or adds a key/value pair
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char newi[MAX_INFO_STRING];
	const char *blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING )
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );

	for ( /**/; *blacklist; ++blacklist ) {
		if ( strchr( key, *blacklist ) || strchr( value, *blacklist ) ) {
			Com_Printf( S_COLOR_YELLOW"Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value );
			return;
		}
	}

	Info_RemoveKey( s, key );
	if ( !value || !strlen( value ) )
		return;

	Com_sprintf( newi, sizeof(newi), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) >= MAX_INFO_STRING ) {
		Com_Printf( "Info string length exceeded\n" );
		return;
	}

	strcat( newi, s );
	strcpy( s, newi );
}

// Changes or adds a key/value pair
//	Includes and retains zero-length values
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char newi[BIG_INFO_STRING];
	const char *blacklist = "\\;\"";

	if ( strlen( s ) >= BIG_INFO_STRING )
		Com_Error( ERR_DROP, "Info_SetValueForKey_Big: oversize infostring" );

	for ( /**/; *blacklist; ++blacklist ) {
		if ( strchr( key, *blacklist ) || strchr( value, *blacklist ) ) {
			Com_Printf( S_COLOR_YELLOW"Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value );
			return;
		}
	}

	Info_RemoveKey_Big( s, key );
	if ( !value )
		return;

	Com_sprintf( newi, sizeof(newi), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf( "BIG Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}

const vector4 colorTable[CT_MAX] = {
	{ 0.0f, 0.0f, 0.0f, 0.0f },		// CT_NONE
	{ 0.0f, 0.0f, 0.0f, 1.0f },		// CT_BLACK
	{ 1.0f, 0.0f, 0.0f, 1.0f },		// CT_RED
	{ 0.0f, 1.0f, 0.0f, 1.0f },		// CT_GREEN
	{ 0.0f, 0.0f, 1.0f, 1.0f },		// CT_BLUE
	{ 1.0f, 1.0f, 0.0f, 1.0f },		// CT_YELLOW
	{ 1.0f, 0.0f, 1.0f, 1.0f },		// CT_MAGENTA
	{ 0.0f, 1.0f, 1.0f, 1.0f },		// CT_CYAN
	{ 1.0f, 1.0f, 1.0f, 1.0f },		// CT_WHITE
	{ 0.75f, 0.75f, 0.75f, 1.0f },		// CT_LTGREY
	{ 0.50f, 0.50f, 0.50f, 1.0f },		// CT_MDGREY
	{ 0.25f, 0.25f, 0.25f, 1.0f },		// CT_DKGREY
	{ 0.15f, 0.15f, 0.15f, 1.0f },		// CT_DKGREY2
	{ 0.810f, 0.530f, 0.0f, 1.0f },		// CT_VLTORANGE -- needs values
	{ 0.810f, 0.530f, 0.0f, 1.0f },		// CT_LTORANGE
	{ 0.610f, 0.330f, 0.0f, 1.0f },		// CT_DKORANGE
	{ 0.402f, 0.265f, 0.0f, 1.0f },		// CT_VDKORANGE
	{ 0.503f, 0.375f, 0.996f, 1.0f },		// CT_VLTBLUE1
	{ 0.367f, 0.261f, 0.722f, 1.0f },		// CT_LTBLUE1
	{ 0.199f, 0.0f, 0.398f, 1.0f },		// CT_DKBLUE1
	{ 0.160f, 0.117f, 0.324f, 1.0f },		// CT_VDKBLUE1
	{ 0.300f, 0.628f, 0.816f, 1.0f },		// CT_VLTBLUE2 -- needs values
	{ 0.300f, 0.628f, 0.816f, 1.0f },		// CT_LTBLUE2
	{ 0.191f, 0.289f, 0.457f, 1.0f },		// CT_DKBLUE2
	{ 0.125f, 0.250f, 0.324f, 1.0f },		// CT_VDKBLUE2
	{ 0.796f, 0.398f, 0.199f, 1.0f },		// CT_VLTBROWN1 -- needs values
	{ 0.796f, 0.398f, 0.199f, 1.0f },		// CT_LTBROWN1
	{ 0.558f, 0.207f, 0.027f, 1.0f },		// CT_DKBROWN1
	{ 0.328f, 0.125f, 0.035f, 1.0f },		// CT_VDKBROWN1
	{ 0.996f, 0.796f, 0.398f, 1.0f },		// CT_VLTGOLD1 -- needs values
	{ 0.996f, 0.796f, 0.398f, 1.0f },		// CT_LTGOLD1
	{ 0.605f, 0.441f, 0.113f, 1.0f },		// CT_DKGOLD1
	{ 0.386f, 0.308f, 0.148f, 1.0f },		// CT_VDKGOLD1
	{ 0.648f, 0.562f, 0.784f, 1.0f },		// CT_VLTPURPLE1 -- needs values
	{ 0.648f, 0.562f, 0.784f, 1.0f },		// CT_LTPURPLE1
	{ 0.437f, 0.335f, 0.597f, 1.0f },		// CT_DKPURPLE1
	{ 0.308f, 0.269f, 0.375f, 1.0f },		// CT_VDKPURPLE1
	{ 0.816f, 0.531f, 0.710f, 1.0f },		// CT_VLTPURPLE2 -- needs values
	{ 0.816f, 0.531f, 0.710f, 1.0f },		// CT_LTPURPLE2
	{ 0.566f, 0.269f, 0.457f, 1.0f },		// CT_DKPURPLE2
	{ 0.343f, 0.226f, 0.316f, 1.0f },		// CT_VDKPURPLE2
	{ 0.929f, 0.597f, 0.929f, 1.0f },		// CT_VLTPURPLE3
	{ 0.570f, 0.371f, 0.570f, 1.0f },		// CT_LTPURPLE3
	{ 0.355f, 0.199f, 0.355f, 1.0f },		// CT_DKPURPLE3
	{ 0.285f, 0.136f, 0.230f, 1.0f },		// CT_VDKPURPLE3
	{ 0.953f, 0.378f, 0.250f, 1.0f },		// CT_VLTRED1
	{ 0.953f, 0.378f, 0.250f, 1.0f },		// CT_LTRED1
	{ 0.593f, 0.121f, 0.109f, 1.0f },		// CT_DKRED1
	{ 0.429f, 0.171f, 0.113f, 1.0f },		// CT_VDKRED1
	{ 0.25f, 0.0f, 0.0f, 1.0f },		// CT_VDKRED
	{ 0.70f, 0.0f, 0.0f, 1.0f },		// CT_DKRED
	{ 0.717f, 0.902f, 1.0f, 1.0f },		// CT_VLTAQUA
	{ 0.574f, 0.722f, 0.804f, 1.0f },		// CT_LTAQUA
	{ 0.287f, 0.361f, 0.402f, 1.0f },		// CT_DKAQUA
	{ 0.143f, 0.180f, 0.201f, 1.0f },		// CT_VDKAQUA
	{ 0.871f, 0.386f, 0.375f, 1.0f },		// CT_LTPINK
	{ 0.435f, 0.193f, 0.187f, 1.0f },		// CT_DKPINK
	{ 0.0f, 0.5f, 0.5f, 1.0f },		// CT_LTCYAN
	{ 0.0f, 0.25f, 0.25f, 1.0f },		// CT_DKCYAN
	{ 0.179f, 0.51f, 0.92f, 1.0f },		// CT_LTBLUE3
	{ 0.199f, 0.71f, 0.92f, 1.0f },		// CT_LTBLUE3
	{ 0.5f, 0.05f, 0.4f, 1.0f },		// CT_DKBLUE3
	{ 0.0f, 0.613f, 0.097f, 1.0f },		// CT_HUD_GREEN
	{ 0.835f, 0.015f, 0.015f, 1.0f },		// CT_HUD_RED
	{ 0.567f, 0.685f, 1.0f, 0.75f },	// CT_ICON_BLUE
	{ 0.515f, 0.406f, 0.507f, 1.0f },		// CT_NO_AMMO_RED
	{ 1.0f, 0.658f, 0.062f, 1.0f },		// CT_HUD_ORANGE
};

#define NUM_TEMPVECS 8
static vector3 tempVecs[NUM_TEMPVECS];
vector3 *tv( float x, float y, float z ) {
	static int index = 0;
	vector3 *v = &tempVecs[index++];
	index &= NUM_TEMPVECS - 1;

	VectorSet( v, x, y, z );

	return v;
}

static char tempStrs[NUM_TEMPVECS][32];
char *vtos( const vector3 *v ) {
	static int index = 0;
	char *s = tempStrs[index++];
	index &= NUM_TEMPVECS - 1;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v->x, (int)v->y, (int)v->z );

	return s;
}

void Q_FSBinaryDump( const char *filename, const void *buffer, size_t len ) {
	fileHandle_t f = NULL_FILE;
	trap->FS_Open( filename, &f, FS_WRITE );
	trap->FS_Write( buffer, (int)len, f );
	trap->FS_Close( f );
	f = NULL_FILE;
}

// serialise a JSON object and write it to the specified file
void Q_FSWriteJSON( void *root, fileHandle_t f ) {
	const char *serialised = NULL;

	serialised = cJSON_Serialize( (cJSON *)root, 1 );
	trap->FS_Write( serialised, strlen( serialised ), f );
	trap->FS_Close( f );

	free( (void *)serialised );
	cJSON_Delete( (cJSON *)root );
}

void Q_FSWriteString( fileHandle_t f, const char *msg ) {
	if ( f != NULL_FILE ) {
		trap->FS_Write( msg, strlen( msg ), f );
	}
}

void Q_NewPrintBuffer( printBufferSession_t *session, size_t length,
	void (*callback)( const char *buffer, int clientNum ), int clientNum )
{
	memset( session, 0, sizeof(*session) );
	session->buffer = (char *)malloc( length );
	session->buffer[0] = '\0';
	session->length = 0u;
	session->maxLength = length;
	session->callback = callback;
	session->clientNum = clientNum;
}

void Q_PrintBuffer( printBufferSession_t *session, const char *append ) {
	const size_t appendLen = strlen( append );
	if ( session->length + appendLen >= session->maxLength ) {
		if ( session->callback ) {
			session->callback( session->buffer, session->clientNum );
		}
		session->length = 0u;
		session->buffer[0] = '\0';
	}
	session->length += appendLen;
	Q_strcat( session->buffer, session->maxLength, append );
}

void Q_DeletePrintBuffer( printBufferSession_t *session ) {
	if ( session->callback ) {
		session->callback( session->buffer, session->clientNum );
	}
	if ( session->buffer ) {
		free( session->buffer );
		session->buffer = NULL;
	}
}

// useful if your bit-flags are spread across multiple variables (i.e. mindtrick index)
qboolean Q_InBitflags( const uint32_t *bits, int index, uint32_t bitsPerByte ) {
	return !!( bits[index / bitsPerByte] & (1 << (index % bitsPerByte)) );
}

void Q_AddToBitflags( uint32_t *bits, int index, uint32_t bitsPerByte ) {
	bits[index / bitsPerByte] |= (1 << (index % bitsPerByte));
}

void Q_RemoveFromBitflags( uint32_t *bits, int index, uint32_t bitsPerByte ) {
	bits[index / bitsPerByte] &= ~(1 << (index % bitsPerByte));
}

// can be used for sorting
// returns 0 on match
int Q_CompareNetAddress( const netadr_t *a1, const netadr_t *a2 ) {
	if ( !a1 && a2 ) {
		return 1;
	}
	else if ( a1 && !a2 ) {
		return -1;
	}

	for ( int i = 0; i < 4; i++ ) {
		if ( a1->ip[i] < a2->ip[i] ) {
			return -1;
		}
		else if ( a1->ip[i] > a2->ip[i] ) {
			return 1;
		}
	}

	return 0;
}

const char *Q_PrintNetAddress( const netadr_t *adr ) {
	if ( !adr ) {
		return "";
	}
	return va( "%i.%i.%i.%i", adr->ip[0], adr->ip[1], adr->ip[2], adr->ip[3] );
}

bool Q_PointInBounds( float x, float y, float startX, float startY, float width, float height ) {
	return x > startX
		&& y > startY
		&& x <= (startX + width)
		&& y <= (startY + height);
}

void Q_OpenURL( const char *url ) {
#if defined(_WIN32)
	ShellExecute( NULL, "open", url, NULL, NULL, SW_SHOWNORMAL );
#elif defined(MACOS_X)
	CFURLRef urlRef = CFURLCreateWithBytes( NULL, (UInt8*)url, strlen( url ), kCFStringEncodingASCII, NULL );
		LSOpenCFURLRef( urlRef, 0 );
	CFRelease( urlRef );
#elif defined(__linux__)
	system( va( "xdg-open \"%s\"", url ) );
#endif
}
