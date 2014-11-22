// this baby parses through a file for specific tokens/keywords and assigns it to whatever you specify based on common
//	types (string/int/float/vec)
// e.g. storing a bunch of weapon information.

#include "qcommon/q_shared.h"

#define MAX_TOKEN_LENGTH 16384

static char tp_token[MAX_TOKEN_LENGTH] = { 0 };
static int tp_lines = 0;
static const char *tp_data = NULL;

void TP_NewParseSession( const char *data ) {
	tp_lines = 1;
	tp_data = data;
	tp_token[0] = 0;
}

static const char *TP_SkipWhitespace( const char *data, qboolean *hasNewLines ) {
	char c;

	while ( (c = *data) <= ' ' ) {
		if ( !c ) {
			return NULL;
		}

		if ( c == '\n' ) {
			tp_lines++;
			*hasNewLines = qtrue;
		}
		data++;
	}

	return data;
}

static const char *TP_ParseExt( qboolean allowLineBreaks ) {
	int			len = 0;
	qboolean	hasNewLines = qfalse;
	const char	*data = tp_data;
	char		quotechar;
	char		c = '\0';

	tp_token[0] = '\0';

	if ( !data ) {
		// make sure incoming data is valid
		tp_data = NULL;
		return tp_token;
	}

	while ( 1 ) {
		//skip whitespace
		data = TP_SkipWhitespace( data, &hasNewLines );

		if ( !data ) {
			tp_data = NULL;
			return tp_token;
		}
		if ( hasNewLines && !allowLineBreaks ) {
			tp_data = data;
			return tp_token;
		}

		c = *data;

		if ( c == '/' && data[1] == '/' ) {
			// skip double slash comments
			data += 2;
			while ( *data && *data != '\n' ) {
				data++;
			}
		}

		else if ( c == '/' && data[1] == '*' ) {
			// skip block comments
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

	if ( c == '\"' || c == '\'' ) {
		// handle quoted strings (both " and ')
		quotechar = c;
		data++;
		while ( 1 ) {
			c = *data++;
			if ( c == quotechar || !c || ((c == '\r' || c == '\n') && !allowLineBreaks) ) {
				tp_token[len] = '\0';
				tp_data = data;
				return tp_token;
			}

			if ( len < MAX_TOKEN_LENGTH ) {
				tp_token[len++] = c;
			}
		}
	}

	// parse a token
	while ( c > 32 ) {
		if ( len < MAX_TOKEN_LENGTH ) {
			tp_token[len++] = c;
		}
		data++;

		c = *data;
		if ( c == '\n' ) {
			tp_lines++;
		}
	};

	tp_token[len] = '\0';

	tp_data = data;
	return tp_token;
}


const char *TP_ParseToken( void ) {
	return TP_ParseExt( qtrue );
}

int TP_CurrentLine( void ) {
	return tp_lines;
}

qboolean TP_ParseString( const char **s ) {
	*s = TP_ParseExt( qfalse );
	return (*s == NULL) ? qtrue : qfalse;
}

qboolean TP_ParseUInt( unsigned int *i ) {
	const char *token = TP_ParseExt( qfalse );

	*i = strtoul( token, NULL, 0 );
	return qfalse;
}

qboolean TP_ParseInt( int *i ) {
	const char *token = TP_ParseExt( qfalse );

	if ( *token == '\0' ) {
		return qtrue;
	}

	*i = atoi( token );
	return qfalse;
}

qboolean TP_ParseShort( short *i ) {
	const char *token = TP_ParseExt( qfalse );

	if ( *token == '\0' ) {
		return qtrue;
	}

	*i = (short)atoi( token );
	return qfalse;
}

qboolean TP_ParseFloat( float *f ) {
	const char *token = TP_ParseExt( qfalse );

	if ( *token == '\0' ) {
		return qtrue;
	}

	*f = atoff( token );
	return qfalse;
}

qboolean TP_ParseVec3( vector3 *vec ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		const char *token = TP_ParseExt( qfalse );

		if ( *token == '\0' ) {
			return qtrue;
		}

		vec->data[i] = atoff( token );
	}

	return qfalse;
}

qboolean TP_ParseVec4( vector4 *vec ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		const char *token = TP_ParseExt( qfalse );
		if ( *token == '\0' ) {
			return qtrue;
		}

		vec->data[i] = atoff( token );
	}

	return qfalse;
}

qboolean TP_ParseByte( byte *i ) {
	const char *token = TP_ParseExt( qfalse );

	if ( *token == '\0' ) {
		return qtrue;
	}

	*i = (byte)atoi( token );
	return qfalse;
}
