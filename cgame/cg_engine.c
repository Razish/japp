//	=================================================
//	Shell System for Hooks (By BobaFett)
//	=================================================
//
//	This system, that I (BobaFett) call The Shell System, allows for hooks to be created
//	on both windows and linux with exactly the same code (OS specific asm changes not included)
//
//	The system works as follows:
//	Since compilers have the tendancy to add prologue and epilogue code to functions, we put our asm inside a safe 'shell'
//	The shell is defined by:
//
//	HOOK( MyHook )
//	{
//		__StartHook( MyHook )	<-- Shell
//		{//Hook code here
//			__asm1__( ret )		<-- Contents of shell
//		}
//		__EndHook( MyHook )		<-- Shell
//	}
//
//	This code should be placed in a function returning a void *, as shown above.
//	When called, it will return the pointer to the shell's contents, which
//	can then be used to place hooks (ie. jumps).
//
//	Note that the shell's contents (the hook in question) are not executed!
//
//
//
//	For the actual asm, 3 defines are available:
//	__asm1__ for zero/single operand opcodes	(push 10	)
//	__asm2__ for dual operand opcodes			(mov eax, 1	)
//	__asmL__ for labels							(mylabel:	)
//
//	To compile this code on linux, you require the following in the gcc command line: -masm=intel
// 
//	NOTE: The hook's execution flow must NEVER get to the shell layer!
//			Always ensure the code is ended with a jump or a return!
//
// ==================================================

#include "cg_local.h"
#include "cg_engine.h"
#include "tr_ext_public.h"

#ifndef OPENJK

// --------------------------------
// Conditional patching

#if !MAC_PORT
	//#define HOOK_LOGGING			//	Logging: Log all Com_Printf calls to the specified file
	#define HOOK_CHATSCROLL			//	ChatScroll: Place key catchers for A_MWHEELUP/A_MWHEELDOWN/A_PAGE_UP/A_PAGE_DOWN to scroll through the chat buffer
	#define HOOK_CHATBACKSPACE		//	ChatBackspace: Place key catchers for Ctrl-Backspace to clear the chat buffer
	//#define HOOK_CONNOTIFY			//	ConsoleNotify: Remove the "Say:" and "Say Team:" for cg_newChatbox 1
	#define HOOK_SENDCHAT
//	#define HOOK_POSTPROCESS		//	PostProcess: Hook after weather
#endif


// --------------------------------
// Engine functions (Will be prefixed with ENG_)

#ifdef _WIN32
	char **cmd_argv = (char **)0xB39808;
	#define CVAR_FINDVAR		0x4393B0
	#define CMD_ADDCOMMAND		0x436DA0
	#define CMD_REMOVECOMMAND	0x436E30
	#define CBUF_ADDTEXT		0x4366A0
	#define SYS_LISTFILES		0x450440
	#define SYS_FREEFILELIST	0x450690
	#define FS_BUILDOSPATH		0x43A550
	#define NET_STRINGTOADDR	0x443210
	EFUNC( void,		Com_Printf,				(const char *msg, ...),													0x437080 );
	EFUNC( cvar_t *,	Cvar_Get,				(const char *var_name, const char *var_value, int flags),				0x439470 );
	EFUNC( void,		Field_CompleteCommand,	(field_t *edit),														0x41B620 );
//	EFUNC( void,		Field_Clear,			(field_t *edit),														0x41B1C0 );
	EFUNC( void,		Cmd_TokenizeString,		(const char *text_in),													0x436C50 );
	EFUNC( char **,		FS_ListFilteredFiles,	(const char *path, const char *extension, char *filter, int *numfiles),	0x43CD70 );
#elif defined(MACOS_X)
	#define CVAR_FINDVAR		0x0378e6
	#define CMD_ADDCOMMAND		0x10b140
	#define CMD_REMOVECOMMAND	0x10b1bc
	#define CBUF_ADDTEXT		0x109000
	#define SYS_LISTFILES		0x0b46f2
	#define SYS_FREEFILELIST	0x0b49cc
	#define FS_BUILDOSPATH		0x038c78
	#define NET_STRINGTOADDR	0x04076e
	EFUNC( char **, FS_ListFilteredFiles, (const char *path, const char *extension, char *filter, int *numfiles), 0x03b194 );
#endif //_WIN32

// a bunch of these asm bridges are cdecl on mac, might want to refactor to use EFUNC

cvar_t *ENG_Cvar_FindVar( const char *cvarname )
{
#if MAC_PORT
	qasm2( mov eax, cvarname )
	qasm2( mov edx, CVAR_FINDVAR )
	qasm1( call edx )
#else
	qasm2( mov edi, cvarname )
	qasm2( mov eax, CVAR_FINDVAR )
	qasm1( call eax )
#endif
}

void ENG_Cmd_AddCommand( const char *cmd, void *function )
{
#if MAC_PORT
	qasm1( push function )
	qasm1( push cmd )
	qasm2( mov edx, CMD_ADDCOMMAND )
	qasm1( call edx )
	qasm2( add esp, 8 )
#else
	qasm2( mov ebx, function )
	qasm2( mov eax, cmd )
	qasm2( mov edx, CMD_ADDCOMMAND )
	qasm1( call edx )
#endif
}

void ENG_Cmd_RemoveCommand( const char *cmd )
{
#if MAC_PORT
	qasm1( push cmd )
	qasm2( mov eax, CMD_REMOVECOMMAND )
	qasm1( call eax )
	qasm2( add esp, 4 )
#else
	qasm2( mov edx, cmd )
	qasm2( mov eax, CMD_REMOVECOMMAND )
	qasm1( call eax )
#endif
}

qboolean ENG_NET_StringToAddr( const char *s, netadr_t *a )
{
#if MAC_PORT
	qasm1( push a )
	qasm1( push s )
	qasm2( mov edx, NET_STRINGTOADDR )
	qasm1( call edx )
	qasm2( add esp, 8 )
#else
	qasm2( mov ebx, a )
	qasm1( push s )
	qasm2( mov edx, NET_STRINGTOADDR )
	qasm1( call edx )
	qasm2( add esp, 4 )
#endif
}

void *FindCommandPointer( const char *command )
{
#ifdef _WIN32
	jacmd_t *cmd;

	for ( cmd = *(jacmd_t **)0xB3CC18; cmd; cmd = cmd->next ) {
		if ( !Q_stricmp( command, cmd->name ) )
			return cmd->function;
	}
#endif
	return NULL;
}

void Cmd_CommandCompletion( void(*callback)(const char *s) ) {
	jacmd_t *cmd;

	for ( cmd = *(jacmd_t **)0xB3CC18; cmd; cmd = cmd->next )
		callback( cmd->name );
}

void Cvar_CommandCompletion( void(*callback)(const cvar_t *cvar) ) {
	cvar_t *cv;

	for ( cv = *(cvar_t **)0xB51490; cv; cv = cv->next )
		callback( cv );
}

void Field_Clear( field_t *edit ) {
	memset( edit->buffer, 0, MAX_EDIT_LINE );
	edit->cursor = 0;
	edit->scroll = 0;
}

void ENG_CBuf_AddText( const char *command/*edi*/ )
{
#if MAC_PORT
	qasm1( push command )
	qasm2( mov eax, CBUF_ADDTEXT )
	qasm1( call eax )
	qasm2( add esp, 4 )
#else
	qasm2( mov edi, command )
	qasm2( mov eax, CBUF_ADDTEXT )
	qasm1( call eax )
#endif
}

char **ENG_Sys_ListFiles( const char *directory/*edx*/, const char *extension, char *filter/*ecx*/, int *numfiles, int wantsubs )
{
#if MAC_PORT
	qasm1( push wantsubs )
	qasm1( push numfiles )
	qasm1( push filter )
	qasm1( push extension )
	qasm1( push directory )
	qasm2( mov ebx, SYS_LISTFILES )
	qasm1( call ebx )
	qasm2( add esp, 20 )
#else
	qasm1( push wantsubs )
	qasm1( push numfiles )
	qasm2( mov ecx, filter )
	qasm1( push extension )
	qasm2( mov edx, directory )
	qasm2( mov ebx, SYS_LISTFILES )
	qasm1( call ebx )
	qasm2( add esp, 12 )
#endif
}

void ENG_Sys_FreeFileList( char **list )
{
#if MAC_PORT
	qasm1( push list )
	qasm2( mov ecx, SYS_FREEFILELIST )
	qasm1( call ecx )
	qasm2( add esp, 4 )
#else
	qasm2( mov edi, list )
	qasm2( mov ecx, SYS_FREEFILELIST )
	qasm1( call ecx )
#endif
}

char *ENG_FS_BuildOSPath( const char *base, const char *game/*eax*/, const char *qpath/*edx*/ )
{
#if MAC_PORT
	qasm1( push qpath )
	qasm1( push game )
	qasm1( push base )
	qasm2( mov ebx, FS_BUILDOSPATH )
	qasm1( call ebx )
	qasm2( add esp, 12 )
#else
	qasm1( push base )
	qasm2( mov eax, game )
	qasm2( mov edx, qpath )
	qasm2( mov ebx, FS_BUILDOSPATH )
	qasm1( call ebx )
	qasm2( add esp, 4 )
#endif
}

#define BUTTON_ALIAS_LIST( a, b )							{ "+"##a, "-"##a, "+"##b, "-"##b }
#define BUTTON_ALIAS_ADD( newOn, newOff, oldOn, oldOff )	{ ENG_Cmd_RemoveCommand( newOn ); ENG_Cmd_RemoveCommand( newOff ); ENG_Cmd_AddCommand( newOn, FindCommandPointer( oldOn ) ); ENG_Cmd_AddCommand( newOff, FindCommandPointer( oldOff ) ); }
#define BUTTON_ALIAS_REMOVE( a, b )							{ ENG_Cmd_RemoveCommand( a ); ENG_Cmd_RemoveCommand( b ); }

typedef struct buttonTable_s {
	const char *newOn, *newOff, *oldOn, *oldOff;
} buttonTable_t;

static const buttonTable_t buttonTable[] =
{//List of commands to be rerouted
#if !MAC_PORT
	BUTTON_ALIAS_LIST( "grapple", "button12" ),
#endif
};
static const int numButtons = ARRAY_LEN( buttonTable );


// --------------------------------
// Hooks

#ifdef HOOK_CHATSCROLL
	//--------------------------------
	//	Name:	Chat scroll patch
	//	Desc:	Add key catchers for A_MWHEELUP, A_MWHEELDOWN, A_PAGE_UP and A_PAGE_DOWN to scroll the chat buffer
	//	Hook:	Message_Key
	//	Retn:	Message_Key
	//--------------------------------
	#ifdef _WIN32
		#define CS_HOOKPOS		0x41BA79	//	.text	83 F9 0A		cmp ecx, 0Ah
											//	.text	74 16			jz short loc_41BA94

		#define CS_ORIGBYTES { 0x83, 0xF9, 0x0A, 0x74, 0x16 }
	#elif defined(MACOS_X)
		#error HOOK_CHATSCROLL not available on Mac OSX
	#endif

	HOOK( ChatScroll ) {//Chat scrolling
		StartHook( ChatScroll )
		qasm2( cmp ecx, 0x0A ) // A_ENTER - over-wrote these opcodes
		qasm1( jnz test0 )
		qasm1( push 0x41BA94 )
		qasm1( ret )

		qasmL(test0: )
		qasm2( cmp ecx, 0x0D ) // A_ESC??
		qasm1( jnz test1 )
		qasm1( push 0x41BA94 )
		qasm1( ret )

		qasmL(test1: )
		qasm2( cmp ecx, 0x89 ) // A_MWHEELUP
		qasm1( jnz test2 )
		qasm1( jmp scrollUp )

		qasmL(test2: )
		qasm2( cmp ecx, 0x91 ) // A_PAGE_UP
		qasm1( jnz test3 )
		qasm1( jmp scrollUp )

		qasmL(test3: )
		qasm2( cmp ecx, 0x8B ) // A_MWHEELDOWN
		qasm1( jnz test4 )
		qasm1( jmp scrollDown )

		qasmL(test4: )
		qasm2( cmp ecx, 0x9E ) // A_PAGE_DOWN
		qasm1( jnz test5 )
		qasm1( jmp scrollDown )

		qasmL(test5: )
		qasm2( cmp ecx, 0x09 ) // A_TAB
		qasm1( jnz test6 )
		qasm1( jmp scrollTab )

		qasmL(test6: )
		qasm2( cmp ecx, 0xAA ) // A_CURSOR_UP
		qasm1( jnz test7 )
		qasm1( jmp scrollHistUp )

		qasmL(test7: )
		qasm2( cmp ecx, 0xAB ) // A_CURSOR_DOWN
		qasm1( jnz fail )
		qasm1( jmp scrollHistDn )

		qasmL(scrollUp: )
		qasm1( pushad )
		qasm1( push 1 )
		qasm1( call JP_ChatboxScroll )
		qasm2( add esp, 4 )
		qasm1( popad )
		qasm1( push 0x41BA83 )
		qasm1( ret )

		qasmL(scrollDown: )
		qasm1( pushad )
		qasm1( push 0 )
		qasm1( call JP_ChatboxScroll )
		qasm2( add esp, 4 )
		qasm1( popad )
		qasm1( push 0x41BA83 )
		qasm1( ret )

		qasmL(scrollTab: )
		qasm1( pushad )
		qasm1( call JP_ChatboxSelectTabNext )
		qasm1( popad )
		qasm1( push 0x41BA83 )
		qasm1( ret )

		qasmL(scrollHistUp: )
		qasm1( pushad )
		qasm1( call JP_ChatboxHistoryUp )
		qasm1( popad )
		qasm1( push 0x41BA83 )
		qasm1( ret )

		qasmL(scrollHistDn: )
		qasm1( pushad )
		qasm1( call JP_ChatboxHistoryDn )
		qasm1( popad )
		qasm1( push 0x41BA83 )
		qasm1( ret )

		qasmL(fail: )
		qasm1( push 0x41BA83 )
		qasm1( ret )
		EndHook( ChatScroll )
	}

#endif //HOOK_CHATSCROLL

#ifdef HOOK_CHATBACKSPACE
	//--------------------------------
	//	Name:	Chat backspace patch
	//	Desc:	Add key catchers for Ctrl-Backspace to clear the chat buffer
	//	Hook:	Field_CharEvent
	//	Retn:	Field_CharEvent
	//--------------------------------
	#ifdef _WIN32
		#define CBS_HOOKPOS		0x41B2EF	//	.text	83 FB 03		cmp ebx, 3
											//	.text	75 12			jnz short loc_41B306

		#define CBS_ORIGBYTES { 0x83, 0xFB, 0x03, 0x75, 0x12 }
	#elif defined(MACOS_X)
		#error HOOK_CHATBACKSPACE not available on Mac OSX
	#endif

	HOOK( ChatBackspace ) {
		StartHook( ChatBackspace )
		qasm2( cmp ebx, 3 ) // Ctrl-C - overwrote these opcodes
		qasm1( jnz test1 )
		qasm1( push 0x41B2F4 )
		qasm1( ret )

		qasmL(test1: )
		qasm2( cmp ebx, 0x7F ) // Ctrl-Backspace
		qasm1( jnz fail )
		qasm1( push 0x41B2F4 )
		qasm1( ret )

		qasmL(fail: )
		qasm1( push 0x41B306 )
		qasm1( ret )
		EndHook( ChatBackspace )
	}

#endif //HOOK_CHATBACKSPACE

#ifdef HOOK_CONNOTIFY
	//--------------------------------
	//	Name:	ConsoleNotify patch
	//	Desc:	Remove the "Say:" and "Say Team:" for cg_newChatbox 1
	//	Hook:	MyFunction
	//	Retn:	MyOtherFunction
	//--------------------------------
	#ifdef _WIN32
		#define CN_HOOKPOS		0x417C7C	//	.text	A1 EC 68 88 00		mov eax, chat_team
		#define CN_RETSUCCESS	0x417C81	//	.text	85 C0				test eax, eax
		#define CN_RETFAIL		0x417D3B	//	clean up stack + exit function
	#elif defined(MACOS_X)
		#error HOOK_CONNOTIFY not available on Mac OSX
	#endif

	HOOK( ConsoleNotify ) {
		StartHook( ConsoleNotify )
		qasm2( mov eax, cg_newChatbox.integer );
		qasm2( test eax, eax )
		qasm1( jnz fail )
		qasm1( push CN_RETSUCCESS )
		qasm1( ret )

		qasmL(fail: )
		qasm2( mov eax, 0x8868EC )
		qasm1( push CN_RETFAIL )
		qasm1( ret )
		EndHook( ConsoleNotify )
	}

#endif //HOOK_CONNOTIFY

#ifdef HOOK_SENDCHAT
	//--------------------------------
	//	Name:	SendChat patch
	//	Desc:	Catch and modify outgoing chat messages from the message key-catcher
	//	Hook:	Message_Key
	//	Retn:	Message_Key
	//--------------------------------
	#ifdef _WIN32
		#define SC_HOOKPOS		0x41BAA9	//	.text	8B 0D E8 68 88			mov ecx, chat_playerNum
		#define SC_RETSUCCESS	0x41BAFD	//	.text	83 25 04 F1 80 00 FB	and cls.keyCatchers 0x0FFFFFFFB
		#define SC_RETFAIL		0x41BAAF	//	.text	83 F9 FF				cmp ecx, 0x0FFFFFFFF
		#define chat_playerNum	0x8868E8
		#define SC_ORIGBYTES	{ 0x8B, 0x0D, 0xE8, 0x68, 0x88 }
	#elif defined(MACOS_X)
		#error HOOK_CONNOTIFY not available on Mac OSX
	#endif

	HOOK( SendChat ) {
		StartHook( SendChat )
		qasm1( call JP_ChatboxOutgoing )
		qasm2( test eax, eax )
		qasm1( jz disregard )

		qasm1( push SC_RETSUCCESS )
		qasm1( ret )

		qasmL(disregard: )
		qasm2( mov ecx, ds:[chat_playerNum] )
		qasm1( push SC_RETFAIL )
		qasm1( ret )
		EndHook( SendChat )
	}

#endif //HOOK_SENDCHAT

#ifdef HOOK_POSTPROCESS
	//--------------------------------
	//	Name:	PostProcess patch
	//	Desc:	blah
	//	Hook:	RB_RenderWorldEffects
	//	Retn:	RB_ExecuteRenderCommands
	//--------------------------------
	#ifdef _WIN32
		#define PP_HOOKPOS		0x48C213
		#define PP_RETPOS		0x48C223
		#define PP_ORIGBYTES	{ 0xE8, 0x96, 0xCC, 0xFF, 0xFF }
	#elif defined(MACOS_X)
		#error HOOK_POSTPROCESS not available on Mac OSX
	#endif

	HOOK( PostProcess ) {
		StartHook( PostProcess )
		qasm1( call R_EXT_PostProcess );
		qasm2( add esi, 8 );
		qasm1( push PP_RETPOS );
		qasm1( ret )
		EndHook( PostProcess )
	}

#endif //HOOK_POSTPROCESS

#ifdef HOOK_DERP
	//--------------------------------
	//	Name:	xxxxx patch
	//	Desc:	xxxxxxxxxxxxxxxxx
	//	Hook:	MyFunction
	//	Retn:	MyOtherFunction
	//--------------------------------
	#ifdef _WIN32
		#define D_HOOKPOS		0xFFFFFF	//	
		#define D_RETSUCCESS	0xFFFFFF	//	
		#define D_RETFAIL		0xFFFFFF	//	
		#define D_ORIGBYTES		{ 0x00, 0x00, 0x00, 0x00, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_DERP not available on Mac OSX
	#endif

	HOOK( Derp ) {
		StartHook( Derp )
		qasm1( ret )
		EndHook( Derp )
	}

#endif //HOOK_DERP

static hookEntry_t hooks[] =
{
	#ifdef HOOK_CHATSCROLL
		HOOKDEF( CS_HOOKPOS,	CS_ORIGBYTES,	0xE9,	Hook_ChatScroll,	"Chat scrolling" ),
	#endif
	#ifdef HOOK_CHATBACKSPACE
		HOOKDEF( CBS_HOOKPOS,	CBS_ORIGBYTES,	0xE9,	Hook_ChatBackspace,	"Chat backspace" ),
	#endif
	#ifdef HOOK_CONNOTIFY
		HOOKDEF( CN_HOOKPOS,	CN_ORIGBYTES,	0xE9,	Hook_ConsoleNotify,	"Console notify" ),
	#endif
	#ifdef HOOK_SENDCHAT
		HOOKDEF( SC_HOOKPOS,	SC_ORIGBYTES,	0xE9,	Hook_SendChat,		"Send Chat" ),
	#endif
	#ifdef HOOK_POSTPROCESS
		HOOKDEF( PP_HOOKPOS,	PP_ORIGBYTES,	0xE8,	Hook_PostProcess,	"Post Processing" ),
	#endif
	#ifdef HOOK_LOGGING
		HOOKDEF( LOG_HOOKPOS,	LOG_HOOKPOS,	0xE8,	Hook_Logging,		"Logging helper" ),
	#endif
};
static const int numHooks = ARRAY_LEN( hooks );

#endif // !OPENJK

void PatchEngine( void )
{//Place all patches
	#ifndef OPENJK
		int i;

		Com_Printf( "Installing engine patches (CGAME)\n" );

		#ifndef MAC_PORT
			//this is added again
			ENG_Cmd_RemoveCommand( "clear" );
		#endif

		// place hooks
		for ( i=0; i<numHooks; i++ )
			PlaceHook( &hooks[i] );

		// alias buttons
		for ( i=0; i<numButtons; i++ )
			BUTTON_ALIAS_ADD( buttonTable[i].newOn, buttonTable[i].newOff, buttonTable[i].oldOn, buttonTable[i].oldOff )

		//Byte patches
	//	PATCH( 0x00000000, byte, 0x00 );

		Com_Printf( "\n" );
	#else
		Com_Printf( "Engine modifications are disabled\n" );
	#endif // !OPENJK
}

void UnpatchEngine( void )
{//Remove all patches
	#ifndef OPENJK
		int i;

		Com_Printf( "^5JA++ Removing engine patches (CGAME)\n" );

		// remove hooks
		for ( i=0; i<numHooks; i++ )
			RemoveHook( &hooks[i] );

		// alias buttons
		for ( i=0; i<numButtons; i++ )
			BUTTON_ALIAS_REMOVE( buttonTable[i].newOn, buttonTable[i].newOff );

		Com_Printf( "\n" );
	#endif // !OPENJK
}
