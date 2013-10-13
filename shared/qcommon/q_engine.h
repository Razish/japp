#pragma once

typedef struct netadr_s {
	int				type;
	byte			ip[4];
	byte			ipx[10];
	unsigned short	port;
} netadr_t;

// Common engine modification data and functions

typedef struct hookEntry_s {
	const unsigned int	hookPosition;	//	The code we're patching
	unsigned char		origBytes[5];	//	What it originally was
	unsigned char		hookOpcode;		//	CALL or JMP
	const unsigned int	hookForward;	//	Function to direct the control flow into
	const char			*name;			//	Long name of the hook
} hookEntry_t;

typedef struct cvarEntry_s {
	const char		*cvarname;
	unsigned int	flags;
} cvarEntry_t;

typedef enum engineDisable_e {
	ENGINE_DISABLE_NONE=0,
	ENGINE_DISABLE_UI_CVAR,
	ENGINE_DISABLE_UI_HOOKS,
	ENGINE_DISABLE_UI_COMMANDS,
	ENGINE_DISABLE_UI_PATCHES,
	ENGINE_DISABLE_CGAME_HOOKS,
	ENGINE_DISABLE_CGAME_BUTTONS,
	ENGINE_DISABLE_CGAME_PATCHES,
} engineDisable_t;

#if defined(__GCC__) || defined(MINGW32) || defined(MAC_PORT)
	#define USED __attribute__((used))
	#define NORET __attribute__((noreturn))
#else //defined(__GCC__) || defined(MINGW32)
	#define USED
	#define NORET
#endif //defined(__GCC__) || defined(MINGW32)

#define HOOKDEF( pos, origBytes, opcode, fwd, name ) { \
	pos, origBytes, opcode, (unsigned int)fwd, name \
}

#define HOOK( name ) void NORET *Hook_##name( void )

#define EFUNC( type, name, args, address ) type USED (*ENG_##name) args = (type (*)args)address

#define CVAR_UNLOCK( cvarname, cvarflags ) { \
	ENG_Cvar_FindVar( cvarname )->flags = cvarflags; \
}

#define CMD_REROUTE( old, new, from, to ) { \
	ENG_Cmd_RemoveCommand( old ); \
	ENG_Cmd_AddCommand( new, (void (*)())to ); \
}

#define PATCH( address, type, to ) { \
	UnlockMemory( address, 1 ); \
	*(type *)address = (type)to; \
	LockMemory( address, 1 ); \
}

// Helper functions
qboolean UnlockMemory( int address, int size );
qboolean LockMemory( int address, int size );
#ifndef OPENJK
	void PlaceHook( hookEntry_t *hook );
	void RemoveHook( const hookEntry_t *hook );
#endif

//API
void PatchEngine( void );
void UnpatchEngine( void );
