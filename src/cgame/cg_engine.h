#pragma once

#include "qcommon/q_engine.h"

typedef struct jacmd_s {
	struct jacmd_s *next;
	const char *name;
	void *function;
} jacmd_t;

#define	MAX_EDIT_LINE	256
typedef struct field_s {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

typedef struct vm_s {
    // DO NOT MOVE OR CHANGE THESE WITHOUT CHANGING THE VM_OFFSET_* DEFINES
    // USED BY THE ASM CODE
    int			programStack;		// the vm may be recursively entered
    int			(*systemCall)( int *parms );

	//------------------------------------
   
    char		name[MAX_QPATH];

	// for dynamic linked modules
	void		*dllHandle;
	int			(QDECL *entryPoint)( int callNum, ... );

	// for interpreted modules
	qboolean	currentlyInterpreting;

	qboolean	compiled;
	byte		*codeBase;
	int			codeLength;

	int			*instructionPointers;
	int			instructionPointersLength;

	byte		*dataBase;
	int			dataMask;

	int			stackBottom;		// if programStack < stackBottom, error

	int			numSymbols;
	struct vmSymbol_s	*symbols;

	int			callLevel;			// for debug indenting
	int			breakFunction;		// increment breakCount on function entry to this
	int			breakCount;
} vm_t;

#ifndef OPENJK

extern char **cmd_argv;

void (*ENG_Com_Printf)( const char *msg, ... );
qboolean ENG_NET_StringToAddr( const char *s, netadr_t *a );

void Cmd_CommandCompletion( void(*callback)(const char *s) );
void Cvar_CommandCompletion( void(*callback)(const cvar_t *cvar) );
void Field_Clear( field_t *edit );

cvar_t *ENG_Cvar_FindVar( const char *cvarname );
cvar_t *(*ENG_Cvar_Get)( const char *var_name, const char *var_value, int flags );
void (*ENG_Field_CompleteCommand)( field_t *edit );
void (*ENG_Cmd_TokenizeString)( const char *text_in );
void ENG_CBuf_AddText( const char *command );
void ENG_Cmd_RemoveCommand( const char *cmd );

char *ENG_FS_BuildOSPath( const char *base, const char *game, const char *qpath );
char **ENG_Sys_ListFiles( const char *directory, const char *extension, char *filter, int *numfiles, int wantsubs );
void ENG_Sys_FreeFileList( char **list );
//	extern char **(*ENG_FS_ListFilteredFiles)( const char *path, const char *extension, char *filter, int *numfiles );
//	extern void ENG_FS_FreeFileList( char **list );

#endif // !OPENJK
