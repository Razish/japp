#pragma once

void		TP_NewParseSession( const char *data );
const char	*TP_ParseToken();
int			TP_CurrentLine();
qboolean		TP_ParseString( const char **s );
qboolean		TP_ParseUInt( unsigned int *i ) ;
qboolean		TP_ParseInt( int *i ) ;
qboolean		TP_ParseShort( short *i );
qboolean		TP_ParseFloat( float *f );
qboolean		TP_ParseVec3( vector3 *vec );
qboolean		TP_ParseVec4( vector4 *vec );
qboolean		TP_ParseByte( byte *i );
