#pragma once

#include "qcommon/q_engine.h"

// Didz's struct
#define NUM_CON_TIMES 4
#define CON_TEXTSIZE 32768
typedef struct console_s {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int		linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xscale;		// JKA: xscale and yscale replace xadjust from Q3 to
	float	yscale;		// scale console characters down to 'native resolution'
	//	xscale will be 640/vid_width and yscale will be 480/vid_height
	//	For a 1280x1024 resolution, these would be 0.5 and 0.46875 respectively

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
	// for transparent notify lines
	vector4	color;
} console_t;

typedef enum {
	NA_BOT,
	NA_BAD,					// an address lookup failed
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IPX,
	NA_BROADCAST_IPX
} netadrtype_t;

typedef enum netsrc_e {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

#ifndef OPENJK

void (*ENG_Com_Printf)( const char *msg, ... );
qboolean ENG_NET_StringToAddr( const char *s, netadr_t *a );
FILE *FS_FileForHandle( fileHandle_t fh );
char *ENG_FS_BuildOSPath( const char *base, const char *game, const char *qpath );
cvar_t *(*ENG_Cvar_Set2)( const char *cvarname, const char *newvalue, qboolean force );
cvar_t *(*ENG_Cvar_Get)( const char *var_name, const char *var_value, int flags );
cvar_t *ENG_Cvar_FindVar( const char *cvarname );

extern unsigned int *cls_state;

#endif // !OPENJK
