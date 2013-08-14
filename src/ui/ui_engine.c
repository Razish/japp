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

#include "ui_local.h"
#include "ui_engine.h"

#ifndef OPENJK

// --------------------------------
// Conditional patching

#ifdef _WIN32
	#define HOOK_SERVERSTATUS		// serverstatus: will also handle favourite server list
	#define HOOK_PRIM				// r_primitives: Bounds check for invalid values which result in a black screen
	#define HOOK_CONSCROLL			// ConsoleScroll: Place key catchers for A_MWHEELUP/A_MWHEELDOWN to scroll through the console
//	#define HOOK_CONHISTORY			// ConsoleHistory: Adjust the key catcher for A_CURSOR_DOWN to commit the current text to the console history.
//	#define HOOK_CHATMESSAGE		// ChatMessage: Replace the "Say: " or "Say Team: " with something a little more accurate based on say_team_mod
	#define HOOK_RAGNOS
	#define HOOK_QUERYBOOM			// q3queryboom: Upon recieving an oversized serverinfo response from the master server list, a buffer overflow will occur.
	#define HOOK_CONNJACK			// q3connjack: A malicious server can flood clients with challengeResponse packets to hijack a pending connection
	#define HOOK_MASTER 2			// MasterServer: Replace the master server used to retrieve a server list from (In-case masterjk3.ravensoft.com goes down?)
//	#define HOOK_UPDATE				// UpdateServer: Replace the updater server used to retrieve an motd from (In-case updatejk3.ravensoft.com goes down?)
	#define HOOK_TRANSPTEXT			// TranspText: Allow text to fade even if there is a ^x colour code present
	#define HOOK_CONSOLEEXT			// ConsoleExt: Various console extensions
	#define HOOK_CONSOLETIME		// ConsoleTime: Adds a timestamp to each console print
	#define HOOK_CVARSEC			// CvarSecurity: Add security checks on cvars flagged with CVAR_SYSTEMINFO, so malicious servers can't overwrite unnecessary cvars
	#define HOOK_CVARARGS			// CvarArgs: Cvar_Command should use Cmd_Args when setting a value, so quotes aren't necessary for values like "r g b a"
	#define HOOK_OVERBRIGHT			// Overbright: Add r_mapOverbrightBits support once again. Bad Raven removed it :<
//	#define HOOK_DERP
#elif MAC_PORT
	//Only these hooks right now
	#define HOOK_CONNJACK
	#define HOOK_MASTER
#endif


// --------------------------------
// Engine functions (Will be prefixed with ENG_)

#ifdef _WIN32

	unsigned int *cls_state = (unsigned int *)0x8AF100;
	netadr_t *clc_serverAddress = (netadr_t *)0x9140D4;
	int *cmd_argc = (int *)0xB3CC08;
	char **cmd_argv = (char **)0xB39808;

	#define CVAR_FINDVAR		0x4393B0
	#define CMD_ADDCOMMAND		0x436DA0
	#define CMD_REMOVECOMMAND	0x436E30
	#define FS_FILEFORHANDLE	0x43AB40
	#define FS_BUILDOSPATH		0x43A550
	#define NET_STRINGTOADDR	0x443210
	#define SCR_DRAWSMALLCHAR	0x422850
	#define RE_SETCOLOR			0x491660
	#define	CL_CONSOLEPRINT		0x417630

	EFUNC( void, Com_Printf, (const char *msg, ...), 0x437080 );
	EFUNC( cvar_t *, Cvar_Get, (const char *var_name, const char *var_value, int flags), 0x439470 );
	EFUNC( cvar_t *, Cvar_Set2, (const char *cvarname, const char *newvalue, qboolean force), 0x4396A0 );
	EFUNC( qboolean, NET_CompareAdr, (netadr_t a, netadr_t b), 0x442F30 );
//	EFUNC( void, RE_SetColor, (vector4 *colour), 0x491660 );
//	EFUNC( void, SCR_DrawSmallChar, (int x, int y, int ch), 0x422850 );
	EFUNC( int, FS_Shutdown, (int closemfp), 0x43A730 );
	EFUNC( void, FS_InitFilesystem, ( void ), 0x43A9C0 );
	EFUNC( void, NET_OOBPrint, (netsrc_t source, netadr_t adr, const char *format, ...), 0x443100 );

#elif MAC_PORT //For now this is *only* the disc version of 1.0.1e, I'll do digital download version once this works...

	unsigned int *cls_state = (unsigned int *)0x1138094;
	netadr_t *clc_serverAddress = (netadr_t *)0xA0988C;
	int *cmd_argc = (int *)0x95EEE0;

	#define CVAR_FINDVAR		0x0378e6
	#define CMD_ADDCOMMAND		0x10b140
	#define CMD_REMOVECOMMAND	0x10b1bc
	#define FS_FILEFORHANDLE	0x038d94
	#define FS_BUILDOSPATH		0x038c78
	#define NET_STRINGTOADDR	0x04076e
	#define SCR_DRAWSMALLCHAR	0x018266
//	#define RE_SETCOLOR			0x000000
	#define	CL_CONSOLEPRINT		0x00af50

	EFUNC( cvar_t *, Cvar_Get, (const char *var_name, const char *var_value, int flags), 0x03834e );
	EFUNC( void, Cvar_Set2, (const char *cvarname, const char *newvalue, int force), 0x037db2 );
	EFUNC( qboolean, NET_CompareAdr, (netadr_t a, netadr_t b), 0x040020 );
	EFUNC( int, FS_Shutdown, (int closemfp), 0x038adc );
	EFUNC( void, FS_InitFilesystem, (void), 0x038ba0 );

#endif //_WIN32


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

void ENG_Cmd_AddCommand( const char *cmd, void (*function)() )
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

void ENG_SCR_DrawSmallChar( int x, int y, int chr/*al*/ )
{
#if MAC_PORT
	qasm1( push chr )
	qasm1( push y )
	qasm1( push x )
	qasm2( mov edx, SCR_DRAWSMALLCHAR )
	qasm1( call edx )
	qasm2( add esp, 12 )
#else
	qasm1( push y )
	qasm1( push x )
	qasm2( mov eax, chr )
	qasm2( mov edx, SCR_DRAWSMALLCHAR )
	qasm1( call edx )
	qasm2( add esp, 8 )
#endif
}

char *ENG_FS_BuildOSPath( const char *base, const char *game/*eax*/, const char *qpath/*edx*/ )
{
#if MAC_PORT
	qasm1( push qpath )
	qasm1( push game )
	qasm1( push base )
	qasm2( mov eax, FS_BUILDOSPATH )
	qasm1( call eax )
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

// --------------------------------
// Hooks

#ifdef HOOK_SERVERSTATUS
	//--------------------------------
	//	Name:	serverstatus patch
	//	Desc:	'serverstatus' has been rerouted to 'query', and will handle querying any favourite server
	//				This hook will catch any unhandled queries and display proper help (including favourite server)
	//	Hook:	CL_ServerStatus_f
	//	Retn:	Com_Printf
	//--------------------------------
	#ifdef _WIN32
		#define SS_HOOKPOS		0x4212DC	//	.text	83 F8 02			cmp eax, 2
											//	.text	74 37				jz short loc_421318
		#define SS_ARGBUF		0xB3980C
		#define SS_CLSSTATE		0x8AF100
		#define SS_CURSERV		0x8AF108
		#define SS_RETSUCCESS	0x42131D	//	Success, go ahead and query
		#define SS_RETFAIL		0x421384	//	Failure, clean up stack

		#define SS_ORIGBYTES { 0x83, 0xF8, 0x02, 0x74, 0x37 }
	#elif defined(MACOS_X)
		#error HOOK_SERVERSTATUS not available on Mac OSX
	#endif

	static const char *SS_msgHelp = "^3Usage: query [server] - 'server' can be [IP], [hostname], favourite server, or '!' for current server\n";
	static const char *SS_msgNoConn = "You're not connected to a server\n";
	static const char *SS_msgCurrServ = "!";

	static void USED SS_DisplayFavourites( void ) {
		unsigned int i;

		//Print favourite servers
		for ( i=0; i<uiLocal.serversCount; i++ )
			Com_Printf( " - %s | %s\n", uiLocal.servers[i].name, uiLocal.servers[i].ip );

		Com_Printf( "Current server: %s", (const char*)0x8AF108 );

		return;
	}

	HOOK( ServerStatus ) {
		StartHook( ServerStatus )
		//We overwrote these opcodes
		qasm2( cmp eax, 2 )
		qasm1( jz gaveArgs )

		//Need more args
		qasm1( push SS_msgHelp )
		qasm1( call Com_Printf )
		qasm2( add esp, 4 )
		qasm1( push SS_RETFAIL )
		qasm1( ret )

		//Gave args, check for ! and check for connectivity if so
		qasmL(gaveArgs: )
		qasm1( push dword ptr [SS_msgCurrServ] )
		qasm1( push dword ptr ds:[SS_ARGBUF] )
		qasm1( call Q_stricmp )
		qasm2( add esp, 8 )
		qasm2( test eax, eax )
		qasm1( je queryCurrent )

		//TODO: Check if arg was a favourite server
	//	qasm1( call SS_Handle )
		qasm2( mov eax, dword ptr ds:[SS_ARGBUF] )
		qasm1( push SS_RETSUCCESS )
		qasm1( ret )

		//Error: Arg was "!", but they're not connected.
		qasmL(isNotConn: )
		qasm1( push SS_msgNoConn )
		qasm1( call Com_Printf )
		qasm2( add esp, 4 )
		qasm1( push SS_RETFAIL )
		qasm1( ret )

		//Arg was "!", query current server
		qasmL(queryCurrent: )
		qasm2( cmp dword ptr ds:[SS_CLSSTATE], 8 )
		qasm1( jne isNotConn )
		qasm2( lea eax, dword ptr ds:[SS_CURSERV] )
		qasm2( mov dword ptr ds:[SS_ARGBUF], eax )
		qasm1( push SS_RETSUCCESS )
		qasm1( ret )
		EndHook( ServerStatus )
	}

#endif //HOOK_SERVERSTATUS

#ifdef HOOK_PRIM
	//--------------------------------
	//	Name:	r_primitives patch
	//	Desc:	Players are often tricked into typing bogus values for r_primitives,
	//				resulting in the renderer not updating the screen
	//	Hook:	?????
	//	Retn:	?????
	//--------------------------------
	#ifdef _WIN32
		#define PRIM_HOOKPOS	0x4AC360	//	.text	8B 0D 24 30 FE		mov ecx, dword_FE3024
		#define PRIM_RETPOS		0x4AC366	//	

		#define PRIM_ORIGBYTES { 0x8B, 0x0D, 0x24, 0x30, 0xFE }
	#elif defined(MACOS_X)
		#error HOOK_PRIM not available on Mac OSX
	#endif

	static char prim_msg[] = "Invalid r_primitives setting detected, reverting to 0\n";
	static char prim_cvar[] = "r_primitives";
	static char prim_value[] = "0";

	HOOK( Primitives ) {
		StartHook( Primitives )
		qasm1( pushad )
		qasm2( mov ecx, ds:[0xFE3024] )
		qasm2( mov ecx, [ecx+0x20] )
		qasm2( cmp ecx, 3 )
		qasm1( jna allok )

		// Bad value, revert to 0
		qasm1( push 1 )					// Force
		qasm1( push offset prim_value )	// Cvar Value
		qasm1( push offset prim_cvar )	// Cvar Name
		qasm1( call ENG_Cvar_Set2 )
		qasm2( add esp, 0x0c )
		qasm1( push offset prim_msg )
		qasm1( call Com_Printf )
		qasm2( add esp, 0x4 )

		qasmL(allok: )
		qasm1( popad )
		qasm2( mov ecx, DS:[0xFE3024] )
		qasm1( push PRIM_RETPOS )
		qasm1( ret )
		EndHook( Primitives )
	}

#endif //HOOK_PRIM

#ifdef HOOK_CONSCROLL
	//--------------------------------
	//	Name:	Console scrolling
	//	Desc:	Add key catchers for A_MWHEELUP and A_MWHEELDOWN to scroll the console
	//	Hook:	Console_Key
	//	Retn:	Console_Key
	//--------------------------------
	#ifdef _WIN32
		#define CS_HOOKPOS		0x41B821	//	.text		81 F9 91			cmp ecx, 145

		#define CS_ORIGBYTES { 0x81, 0xF9, 0x91, 0x00, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_CONSCROLL not available on Mac OSX
	#endif

	HOOK( ConScroll ) {
		StartHook( ConScroll )
		qasm2( cmp ecx, 0x91 ) // A_PAGE_UP
		qasm1( jnz test1 )
		qasm1( push 0x41B829 )
		qasm1( ret )

		qasmL(test1: )
		qasm2( cmp ecx, 0x89 ) // A_MWHEELUP
		qasm1( jnz test2 )
		qasm1( push 0x41B829 )
		qasm1( ret )

		qasmL(test2: )
		qasm2( cmp ecx, 0x8B ) // A_MWHEELDOWN
		qasm1( jnz fail )
		qasm1( push 0x41B83F )
		qasm1( ret )

		qasmL(fail: )
		qasm1( push 0x41B837 )
		qasm1( ret )
		EndHook( ConScroll )
	}

#endif //HOOK_CONSCROLL

#ifdef HOOK_CONHISTORY
	//--------------------------------
	//	Name:	Console history
	//	Desc:	Adjust the key catcher for A_CURSOR_DOWN to commit the current text to the console history.
	//	Hook:	Console_Key
	//	Retn:	Console_Key
	//--------------------------------
	#ifdef _WIN32
		#define CH_HOOKPOS		0x41B89D	//	
	#elif defined(MACOS_X)
		#error HOOK_CONHISTORY not available on Mac OSX
	#endif

	HOOK( ConHistory ) {
		StartHook( ConHistory )
		qasmL(old_test: )
		qasm2( mov esi, 0x8858CC ) // historyLine
		qasm2( cmp esi, 0x8858C8 ) // nextHistoryLine
		qasm1( jz old_fail )
		qasm1( push 0x41B8AB )		// .text	46		inc esi
		qasm1( ret )

		qasmL(old_fail: )
		qasm1( push 0x41B87F ) //clean up stack and retn
		qasm1( ret )
		EndHook( ConHistory )
	}

#endif //HOOK_CONHISTORY

#ifdef HOOK_CHATMESSAGE
	//--------------------------------
	//	Name:	ChatMessage patch
	//	Desc:	Replace the "Say: " or "Say Team: " with something a little more accurate based on say_team_mod
	//	Hook:	Con_DrawNotify
	//	Retn:	Field_BigDraw
	//--------------------------------
	#ifdef _WIN32
		#define CM_HOOKPOS		0x417D33	//	
		#define CM_RETADDY		0x41AF80	//	
	#elif defined(MACOS_X)
		#error HOOK_CHATMESSAGE not available on Mac OSX
	#endif

	HOOK( ChatMessage ) {
		StartHook( ChatMessage )
		qasm2( mov edi, SS_msgNoConn )
		qasm1( push CM_RETADDY )
		qasm1( ret )
		EndHook( ChatMessage )
	}

#endif //HOOK_CHATMESSAGE

#ifdef HOOK_RAGNOS
	//--------------------------------
	//	Name:	Ragnos patch
	//	Desc:	Spawning Ragnos infamously crashes clients, due to invalid Ghoul2 information (bolts?)
	//	Hook:	G2_GetBoltNoRec
	//	Retn:	sub_499850 or G2_GetBoltNoRec
	//--------------------------------
	#ifdef _WIN32
		#define R_HOOKPOS		0x4DFAF9	//	.text		E8 52 9D FB FF	call sub_499850
		#define R_RETSUCCESS	0x499850	//	.text		sub_499850		proc near
		#define R_RETFAIL		0x4999C1/*4DFAFE*/	//	.text		0BC D9 06		fld dword ptr [esi]

		#define R_ORIGBYTES { 0xE8, 0x52, 0x9D, 0xFB, 0xFF }
	#elif defined(MACOS_X)
		#error HOOK_RAGNOS not available on Mac OSX
	#endif

	unsigned int rag_test = 0xffffffff;

	HOOK( Ragnos ) {
		StartHook( Ragnos )
		qasm1( push R_RETSUCCESS )
		qasm1( ret )
		EndHook( Ragnos )
	}

#endif //HOOK_RAGNOS

#ifdef HOOK_QUERYBOOM
	//--------------------------------
	//	Name:	q3queryboom patch
	//	Desc:	Upon recieving an oversized serverinfo response from the master server list, a buffer overflow will occur resulting in a crash
	//	Hook:	CL_ServerInfoPacket
	//	Retn:	CL_ServerInfoPacket
	//--------------------------------
	#ifdef _WIN32
		#define QB_HOOKPOS		0x420083	//	.text		E8 38 49 02 00		call Info_ValueForKey
		#define QB_RETSUCCESS	0x4449C0	//	.text		Info_ValueForKey	proc near
		#define QB_RETFAIL		0x4200AE	//	.text		C3					retn

		#define QB_ORIGBYTES { 0xE8, 0x38, 0x49, 0x02, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_QUERYBOOM not yet available on Mac OSX
	#endif

	static int USED QB_CheckPacket( const char *info ) {
		char *value = NULL;
		int length = 0;
		
		//Overall size > 384
		length = strlen( info );
		if ( length > 384 )
		{
			Com_Printf( "Ignoring server response due to oversized serverinfo packet (size: %i > 384)\n", length );
			return 1;
		}
		
		//hostname > MAX_HOSTNAMELENGTH
		value = Info_ValueForKey( info, "hostname" );
		length = strlen( value );
		if ( length > MAX_HOSTNAMELENGTH )
		{
			Com_Printf( "Ignoring server response due to oversized 'hostname' info key (size: %i > "STRING(MAX_HOSTNAMELENGTH)")\n", length );
			return 1;
		}

		//mapname > MAX_QPATH
		value = Info_ValueForKey( info, "mapname" );
		length = strlen( value );
		if ( length > MAX_QPATH )
		{
			Com_Printf( "Ignoring server response due to oversized 'mapname' info key (size: %i > "STRING(MAX_QPATH)")\n", length );
			return 1;
		}

		//fs_game > MAX_QPATH
		value = Info_ValueForKey( info, "game" );
		length = strlen( value );
		if ( length > MAX_QPATH )
		{
			Com_Printf( "Ignoring server response due to oversized 'game' info key (size: %i > "STRING(MAX_QPATH)")\n", length );
			return 1;
		}

		return 0;
	}

	HOOK( QueryBoom ) {
		StartHook( QueryBoom )
		qasm1( pushad )
		qasm1( push eax )
		qasm1( call QB_CheckPacket )
		qasm2( add esp, 4 )
		qasm2( test eax, eax )
		qasm1( popad )
		qasm1( jnz bad )
		qasm1( push QB_RETSUCCESS )
		qasm1( ret )

		qasmL(bad: )
		qasm1( push QB_RETFAIL )
		qasm1( ret )
		EndHook( QueryBoom )
	}

#endif //HOOK_QUERYBOOM

#ifdef HOOK_CONNJACK
	//--------------------------------
	//	Name:	q3connjack patch
	//	Desc:	A malicious server can flood clients with challengeResponse packets to hijack a pending connection
	//	Hook:	CL_ServerInfoPacket
	//	Retn:	CL_ServerInfoPacket
	//--------------------------------
	#ifdef _WIN32
		#define CJ_HOOKPOS		0x41EAF7	//	.text		83 3D 08 CC B3 00 01	cmp Cmd_Argc, 1
		#define CJ_RETSUCCESS	0x41EAFE	//	.text		B8 3C 5C 54 00			mov eax, offset NullString
		#define CJ_RETFAIL		0x41EAEC	//	.text		5F						pop edi

		#define CJ_ORIGBYTES { 0x83, 0x3D, 0x08, 0xCC, 0xB3 }
	#elif defined(MACOS_X)
		#define CJ_HOOKPOS		0x14E3C
		#define CJ_RETSUCCESS	0x14E43
		#define CJ_RETFAIL		0x14E30

		#define CJ_ORIGBYTES	{ 0xC7, 0x04, 0x24, 0x01, 0x00 }
	#endif

	USED static netadr_t *connJack_from;
	USED static qboolean connJack_result;
	USED static int connJack_argc;

	HOOK( ConnJack ) {
		StartHook( ConnJack )
#if MAC_PORT//upsides: it works
			//downsides: see below
			__asm__ __volatile__ ( "call get_offsetConnJack\n"
								  "get_offsetConnJack:\n"
								  "pop %ebx\n"
								  "leal 0x960(%esp), %eax\n" // Load local variable from into netadr_t *from (0x958 + 0x8)
								  "movl %eax, %edx\n"
								  "leal _connJack_from-get_offsetConnJack(%ebx), %eax\n"
								  "movl %edx, (%eax)\n"

								  "pusha\n"
								  
								  "leal _clc_serverAddress-get_offsetConnJack(%ebx), %edx\n"
								  "movl (%edx), %edx\n"
								  "push 0x10(%edx)\n"
								  "push 0x0c(%edx)\n"
								  "push 0x08(%edx)\n"
								  "push 0x04(%edx)\n"
								  "push (%edx)\n"
								  "leal _connJack_from-get_offsetConnJack(%ebx), %edx\n"
								  "movl (%edx), %edx\n"
								  "push 0x10(%edx)\n"
								  "push 0x0c(%edx)\n"
								  "push 0x08(%edx)\n"
								  "push 0x04(%edx)\n"
								  "push (%edx)\n"
								  "leal _ENG_NET_CompareAdr-get_offsetConnJack(%ebx), %edx\n"
								  "movl (%edx), %edx\n"
								  "call *%edx\n"
								  "movl %eax, %edx\n"
								  "leal _connJack_result-get_offsetConnJack(%ebx), %eax\n"
								  "movl %edx, (%eax)\n"
								  "addl $0x28, %esp\n"
								  
								  "popa\n"
								  
								  "leal _connJack_result-get_offsetConnJack(%ebx), %edx\n"
								  "movl (%edx), %eax\n"
								  "testl %eax, %eax\n"
								  "jnz match\n"
								  
								  "movl $0x14E30, %eax\n"
								  "jmpl *%eax\n"
								  
								  "match:\n"
								  "movl $1, (%esp)\n"
								  "movl $0x14E43, %eax\n"
								  "jmpl *%eax\n");
#else
			qasm2( lea eax, [esp+0x410+0x04] ) // Load local variable from into netadr_t *from
			qasm2( mov connJack_from, eax )
			qasm1( pushad )

			connJack_result = ENG_NET_CompareAdr( *connJack_from, *clc_serverAddress );
			connJack_argc = *cmd_argc;
			
			qasm1( popad )
			qasm2( mov eax, connJack_result )
			qasm2( test eax, eax )
			qasm1( jnz match )

			qasm2( mov eax, CJ_RETFAIL )
			qasm1( jmp eax )

			// Matching:
			qasmL(match: )
			qasm2( cmp connJack_argc, 1 )
			qasm2( mov eax, CJ_RETSUCCESS )
			qasm1( jmp eax )
#endif
		EndHook( ConnJack )
	}

#endif //HOOK_CONNJACK

#if HOOK_MASTER == 1
	//--------------------------------
	//	Name:	Master server patch
	//	Desc:	Use a different master server to retrieve a server list from (In-case masterjk3.ravensoft.com goes down?)
	//	Hook:	NET_StringToAdr in CL_GlobalServers_f
	//	Retn:	CL_GlobalServers_f
	//--------------------------------
	#ifdef _WIN32
		#define MS_HOOKPOS		0x420B87	//	.text		E8 84 26 02			call NET_StringToAdr
		#define MS_RETPOS		0x420B8C	//	.text		A1 08 CC B3			mov eax, Cmd_Argc

		#define MS_ORIGBYTES { 0xE8, 0x84, 0x26, 0x02, 0x00 }
	#elif defined(MACOS_X)
		#define MS_HOOKPOS		0x12b40
		#define	MS_RETPOS		0x12b45

		#define MS_ORIGBYTES { 0xE8, 0x29, 0xDC, 0x02, 0x00 }
	#endif

	USED static char *masterServerString = "masterjk3.ravensoft.com";
#if MAC_PORT
	USED static char *cl_masterString = "cl_master";
	USED static char *cl_oldmasterServerString = "masterjk3.ravensoft.com";
#endif

	HOOK( MasterServer ) {
		StartHook( MasterServer )
#if MAC_PORT
			//Sorry this isn't using standard syntax as below; would be nice to change it really
			__asm__ __volatile__ ( "call get_offsetMasterServer\n"
					"get_offsetMasterServer:\n"
					"pop %ebx\n"
					"push %eax\n"//save original eax
					"push $1\n"
					"leal _cl_oldmasterServerString-get_offsetMasterServer(%ebx), %edx\n"
					"push (%edx)\n"
					"leal _cl_masterString-get_offsetMasterServer(%ebx), %edx\n"
					"push (%edx)\n"
					"leal _ENG_Cvar_Get-get_offsetMasterServer(%ebx), %edx\n"
					"movl (%edx), %edx\n"
					"call *%edx\n"
					"addl $0xC, %esp\n"
					"pop %edx\n"//get original eax
					"push %edx\n"//extra padding for stack alignment
					"push %edx\n"
					"movl 0x4(%eax), %edx\n"
					"push %edx\n"
					"movl $0x4076e, %edx\n"
					"call *%edx\n"
					"addl $0x10, %esp\n"//to keep stack aligned properly
					"push $0x12b45\n"
					"ret\n" );
#else
		//	__asm1__( call M_UpdateString );
			masterServerString = ENG_Cvar_Get( "cl_master", "masterjk3.ravensoft.com", CVAR_ARCHIVE )->string;
			qasm1( push edx )
			qasm1( push masterServerString )
			qasm2( mov edx, NET_STRINGTOADDR )
			qasm1( call edx )
			qasm2( add esp, 8 )
			qasm1( pop edx )
			qasm1( push MS_RETPOS )
			qasm1( ret )
#endif
		EndHook( MasterServer )
	}

#elif HOOK_MASTER == 2
void CL_GlobalServers_f ( void )
{
	char command[1024];
	char *buffptr;
	int count, i;
	netadr_t to;
	int *cls_nummplayerservers = (int *)0x8B4430;
	int *cls_pingUpdateSource = (int *)0x913840;
	int *masterNum = (int*)(0x913844);
	char buf[MAX_TOKEN_CHARS] = {0};

	if ( trap->Cmd_Argc() < 3 ) {
		Com_Printf( "usage: globalservers <master# 0-1> <protocol> [keywords]\n");
		return;
	}

	trap->Cmd_Argv( 1, buf, sizeof( buf ) );
	*masterNum = atoi( buf );

	Com_Printf( "Requesting servers from the master...\n");

	// This function appears totally nerfed from Q3.
	// In Q3, this had a AS_MPLAYERS. This is not
	// used in JA, rather it's forced to AS_GLOBAL (gj raven etc)
	// --eez
	{
		const int bufSize = 64;
		char *cvarBuffer = (char *)malloc(bufSize);
		trap->Cvar_VariableStringBuffer( va( "sv_master%i", (*masterNum > 0) ? *masterNum : 1), cvarBuffer, bufSize );
		qasm1( pushad )
		qasm1( pushfd )
		qasm1( push cvarBuffer )
		qasm2( lea ebx, to )
		qasm2( mov eax, 0x443210 )
		qasm1( call eax )
		qasm2( add esp, 4 )
		qasm1( popfd )
		qasm1( popad )
		*cls_nummplayerservers = -1;
		*cls_pingUpdateSource = 1;
		free( cvarBuffer );
	}

	to.type = NA_IP;
	to.port = BigShort( 29060 );

	// Construct the command
	trap->Cmd_Argv( 2, buf, sizeof( buf ) );
	Com_sprintf( command, sizeof( command ), "getservers %s", buf );

	// tack on keywords
	buffptr = command + strlen( command );
	count = trap->Cmd_Argc();

	//Raz: plz fix
	for (i=3; i<count; i++)
	{
		trap->Cmd_Argv( i, buf, sizeof( buf ) );
		buffptr += sprintf( buffptr, " %s", buf );
	}

	// send it already!
	ENG_NET_OOBPrint(NS_SERVER, to, command);
	*masterNum = 0; // hackery, but necessary hackery
}

#endif //HOOK_MASTER

#ifdef HOOK_UPDATE
	//--------------------------------
	//	Name:	Update server patch
	//	Desc:	Use a different update server to retrieve an motd from (In-case updatejk3.ravensoft.com goes down?)
	//	Hook:	NET_StringToAdr in CL_RequestMotd
	//	Retn:	CL_RequestMotd
	//--------------------------------
	#ifdef _WIN32
		#define US_HOOKPOS		0x41D682	//	.text		E8 89 5B 02 00		call NET_StringToAdr
		#define US_RETPOS		0x41D687	//	.text		A1 08 CC B3			mov eax, Cmd_Argc
		#define US_ORIGBYTES	{ 0xE8, 0x89, 0x5B, 0x02, 0x00 }

	//	#define US_HOOKPOS2		0x41D6BA	//	.text		68 85 71 00 00		push 7185h
	//	#define US_RETPOS2		0x41D6BF	//	.text		51					push ecx
	//	#define US_ORIGBYTES2	{ 0x68, 0x85, 0x71, 0x00, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_UPDATE not available on Mac OSX
	#endif

	USED static char *updateServerString = "updatejk3.ravensoft.com";

	HOOK( UpdateServer ) {
		StartHook( UpdateServer )
		updateServerString = ENG_Cvar_Get( "cl_update", "updatejk3.ravensoft.com", CVAR_ARCHIVE )->string;
		qasm1( push edx )
		qasm1( push updateServerString )
		qasm2( mov edx, NET_STRINGTOADDR )
		qasm1( call edx )
		qasm2( add esp, 8 )
		qasm1( pop edx )
		qasm1( push US_RETPOS )
		qasm1( ret )
		EndHook( UpdateServer )
	}

	/*
	HOOK( UpdateServer2 ) {
		StartHook( UpdateServer2 )
		updateServerPort = ENG_Cvar_Get( "cl_updatePort", "29061", CVAR_ARCHIVE )->integer;
		qasm1( push edx )
		qasm1( push updateServerString )
		qasm2( mov edx, NET_STRINGTOADDR )
		qasm1( call edx )
		qasm2( add esp, 8 )
		qasm1( pop edx )
		qasm1( push US_RETPOS )
		qasm1( ret )
		EndHook( UpdateServer2 )
	}
	*/

#endif //HOOK_UPDATE

#ifdef HOOK_TRANSPTEXT
	//--------------------------------
	//	Name:	Transparent text
	//	Desc:	Keeps alpha color for drawn text with color codes
	//	Hook:	RE_DrawFontText
	//	Retn:	RE_DrawFontText
	//	Note:	Written by Sil =]
	//--------------------------------
	#ifdef _WIN32
		#define TT_HOOKPOS		0x4959C9	//	.text		50					push eax

		#define TT_ORIGBYTES { 0x50, 0xE8, 0x91, 0xBC, 0xFF }
	#elif defined(MACOS_X)
		#error HOOK_TRANSPTEXT not available on Mac OSX
	#endif

	static float tt_rgba[4];

	HOOK( TranspText ) {
		StartHook( TranspText )
		qasm2( mov ecx, [eax] )
		qasm2( mov [tt_rgba    ], ecx ) // R
		qasm2( mov ecx, [eax+0x4] )
		qasm2( mov [tt_rgba+0x4], ecx ) // G
		qasm2( mov ecx, [eax+0x8] )
		qasm2( mov [tt_rgba+0x8], ecx ) // B
		qasm2( mov ecx, [ebp+0x14] )
		qasm2( mov ecx, [ecx+0xC] )
		qasm2( mov [tt_rgba+0xC], ecx ) // A

		qasm2( lea eax, [tt_rgba] )
		qasm1( push eax )
		qasm2( mov ecx, 0x491660 ) //RE_SetColor
		qasm1( call ecx )
		qasm2( add esp, 4 )

		qasm1( push 0x4959D2 )
		qasm1( ret )
		EndHook( TranspText )
	}
#endif //HOOK_TRANSPTEXT

#ifdef HOOK_CONSOLEEXT
	//--------------------------------
	//	Name:	Console extensions
	//	Desc:	Reimplementing various console features from q3
	//	Hook:	Con_DrawConsole
	//	Retn:	Con_DrawConsole
	//--------------------------------
	#ifdef _WIN32
		#define CE_HOOKPOS		0x417E50	//	.text	8B 15 A8 40 91 00		mov edx, cls_glconfig_vidWidth
		#define CE_RETSUCCESS	0x417E73	//	.text	8D 46 F8				lea eax, [esi-8]
		#define CE_RETFAIL		0x418210	//	.text	5F						pop edi

		#define CE_ORIGBYTES { 0x8B, 0x15, 0xA8, 0x40, 0x91 }
	#elif defined(MACOS_X)
		#error HOOK_CONSOLEEXT not available on Mac OSX
	#endif

	// used by other code
	static qboolean		 timestamp24Hour			= qfalse;

	static const char	 ce_japp_outdated[] = "[OUTDATED]";
	static const char	 ce_japp_updated[] = "[Please restart JKA to finish the JA++ update!]";
	static const char	 ce_japp_version[] = JAPP_CLIENT_VERSION_CLEAN;
	static const char	 ce_jamp_version[] = "[00:00:00] JAmp: v1.0.1.0";

	static int			 ce_lenOutdated				= 0;
	static int			 ce_lenUpdated				= 0;
	static int			 ce_lenJapp					= 0;
	static int			 ce_lenJamp					= 0;
	static int			 ce_x						= 0;
	static signed int	 ce_cls_glconfig_vidWidth	= 0;//*(signed int *)0x9140A8;
	static signed int	 ce_cls_glconfig_vidHeight	= 0;//*(signed int *)0x9140AC;
	static float		 ce_frac					= 0;//*(float *)0x883020;
	static int			 ce_lines					= 0;//cls_glconfig_vidHeight * frac;
	static vector4		 ce_colourOutdated			= { 1.0f, 0.0f, 0.0f, 0.8f };
	static vector4		 ce_colourUpdated			= { 0.0f, 1.0f, 0.0f, 0.8f };
	static vector4		 ce_colourJapp				= { 1.0f, 1.0, 0.1, 0.8f };
	static vector4		 ce_colourJamp				= { 0.567f, 0.685f, 1.0f, 0.8f };
	static char			*ce_jampString				= NULL;
	static struct tm	*ce_timeinfo;
	static time_t		 ce_tm;

	HOOK( ConsoleExt ) {
		StartHook( ConsoleExt )

		ce_lenOutdated				= strlen( ce_japp_outdated );
		ce_lenUpdated				= strlen( ce_japp_updated );
		ce_lenJapp					= strlen( ce_japp_version );
		ce_lenJamp					= strlen( ce_jamp_version );
		ce_x						= 0;
		ce_cls_glconfig_vidWidth	= *(signed int *)0x9140A8;
		ce_cls_glconfig_vidHeight	= *(signed int *)0x9140AC;
		ce_frac						= *(float *)0x883020;
		ce_lines					= ce_cls_glconfig_vidHeight * ce_frac;
		ce_jampString				= NULL;


		if ( ce_lines <= 0 ) {
			qasm1( push CE_RETFAIL )
			qasm1( ret )
		}

		if ( ce_lines > ce_cls_glconfig_vidHeight )
			ce_lines = ce_cls_glconfig_vidHeight;

		if ( uiLocal.updateStatus == JAPP_UPDATE_OUTDATED )
		{
			trap->R_SetColor( &ce_colourOutdated );
			for ( ce_x=0; ce_x<ce_lenOutdated; ce_x++ )
				ENG_SCR_DrawSmallChar( ce_cls_glconfig_vidWidth - ( ce_lenOutdated - ce_x ) * SMALLCHAR_WIDTH, (ce_lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2))-(SMALLCHAR_HEIGHT*2), ce_japp_outdated[ce_x] );
		}
		else if ( uiLocal.updateStatus == JAPP_UPDATE_UPDATED )
		{
			trap->R_SetColor( &ce_colourUpdated );
			for ( ce_x=0; ce_x<ce_lenUpdated; ce_x++ )
				ENG_SCR_DrawSmallChar( ce_cls_glconfig_vidWidth - ( ce_lenUpdated - ce_x ) * SMALLCHAR_WIDTH, (ce_lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2))-(SMALLCHAR_HEIGHT*2), ce_japp_updated[ce_x] );
		}

		trap->R_SetColor( &ce_colourJapp );
		for ( ce_x=0; ce_x<ce_lenJapp; ce_x++ )
			ENG_SCR_DrawSmallChar( ce_cls_glconfig_vidWidth - ( ce_lenJapp - ce_x ) * SMALLCHAR_WIDTH, (ce_lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2))-SMALLCHAR_HEIGHT, ce_japp_version[ce_x] );

		trap->R_SetColor( &ce_colourJamp );
		time( &ce_tm );
		ce_timeinfo = localtime ( &ce_tm );
		if ( !timestamp24Hour && ce_timeinfo->tm_hour > 12 )
			ce_timeinfo->tm_hour -= 12;
		ce_jampString = va( "[%02i:%02i:%02i] JAmp: v1.0.1.0", ce_timeinfo->tm_hour, ce_timeinfo->tm_min, ce_timeinfo->tm_sec );
		trap->R_SetColor( &colorTable[CT_WHITE] );
		for ( ce_x=0; ce_x<11; ce_x++ )
			ENG_SCR_DrawSmallChar( ce_cls_glconfig_vidWidth - ( ce_lenJamp - ce_x ) * SMALLCHAR_WIDTH, (ce_lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), ce_jampString[ce_x] );
		trap->R_SetColor( &ce_colourJamp );
		for ( ce_x=11; ce_x<ce_lenJamp; ce_x++ )
			ENG_SCR_DrawSmallChar( ce_cls_glconfig_vidWidth - ( ce_lenJamp - ce_x ) * SMALLCHAR_WIDTH, (ce_lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), ce_jampString[ce_x] );

		qasm1( push CE_RETSUCCESS )
		qasm1( ret )
		EndHook( ConsoleExt )
	}
#endif //HOOK_CONSOLEEXT

#ifdef HOOK_CVARSEC
	//--------------------------------
	//	Name:	Cvar security patch
	//	Desc:	Add security checks on cvars flagged with CVAR_SYSTEMINFO, so malicious servers can't overwrite unnecessary cvars
	//	Hook:	CL_SystemInfoChanged
	//	Retn:	Info_ValueForKey
	//--------------------------------
	#ifdef _WIN32
		#define CSEC_HOOKPOS	0x421C5F	//	
		#define CSEC_RETPOS		0x4449C0	//	
		#define CSEC_ORIGBYTES	{ 0xE8, 0x5C, 0x2D, 0x02, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_CVARSEC not available on Mac OSX
	#endif

	static const char *csec_whitelist[] = { // alphabetical because fuck you
		"fs_game",
		"g_synchronousClients",
		"pmove_msec",
		"pmove_fixed"
		"pmove_float",
		"RMG_course",
		"RMG_instances",
		"RMG_map",
		"RMG_mission",
		"RMG_textseed",
		"sv_cheats",
		"sv_pure",
		"sv_referencedPakNames",
		"sv_referencedPaks",
		"sv_serverid",
		"sv_voip",
		"timescale",
		"vm_ui",
		"vm_game",
		"vm_cgame",
	};
	static const int csec_whitelistSize = ARRAY_LEN( csec_whitelist );
	static char *csec_info = NULL;

	static void CSec_CheckWhitelist( void )
	{
		char	key[BIG_INFO_KEY] = {0},
				value[BIG_INFO_VALUE] = {0},
				*s = csec_info;
		int i = 0;

		while ( s )
		{
			Info_NextPair( &s, key, value );

			if ( !key[0] )
				break;

			for ( i=0; i<csec_whitelistSize; i++ )
			{
				if ( !Q_stricmp( csec_whitelist[i], key ) )
					break;
			}

			if ( !Q_stricmp( key, "fs_game" ) )
			{
				if ( strstr( value, "../" ) || strstr( value, "..\\" ) )
				{
				 	Com_Printf( S_COLOR_RED "SECURITY WARNING: server attempted directory traversal on %s=%s\n", key, value );
					Info_RemoveKey_Big( csec_info, key );
					s = csec_info;
				}
			}

			if ( i == csec_whitelistSize )
			{
				Com_Printf( S_COLOR_RED "SECURITY WARNING: server is not allowed to set non-systeminfo cvar %s=%s\n", key, value );
				Info_RemoveKey_Big( csec_info, key );
				s = csec_info;
			}
		}
	}

	HOOK( CvarSecurity ) {
		StartHook( CvarSecurity )
		qasm2( mov csec_info, ecx )
		qasm1( pushad )
		qasm1( call CSec_CheckWhitelist )
		qasm1( popad )

		qasm1( push CSEC_RETPOS )
		qasm1( ret )
		EndHook( CvarSecurity )
	}

#endif //HOOK_CVARSEC

#ifdef HOOK_CVARARGS
	//--------------------------------
	//	Name:	CvarArgs patch
	//	Desc:	Cvar_Command should use Cmd_Args when setting a value, so quotes aren't necessary for
	//			values like "r g b a"
	//	Hook:	Cvar_Set2
	//	Retn:	Cvar_Command
	//--------------------------------
	#ifdef _WIN32
		#define CVA_HOOKPOS		0x439B49	//	.text	E8 52 FB FF FF			call Cvar_Set2
		#define CVA_RETPOS		0x439B4E	//	.text	83 C4 0C				add esp, 0Ch
		#define CVA_ORIGBYTES	{ 0xE8, 0x52, 0xFB, 0xFF, 0xFF }
	#elif defined(MACOS_X)
		#error HOOK_CVARARGS not available on Mac OSX
	#endif

	static char cmd_args[MAX_STRING_CHARS];
	static char cmd_arg1[MAX_STRING_CHARS];
	static void Cmd_GetArgs( void ) {
		int i;

		cmd_args[0] = '\0';
		for ( i=1; i<(*cmd_argc); i++ ) {
			Q_strcat( cmd_args, sizeof( cmd_args ), cmd_argv[i] );
			if ( i != (*cmd_argc)-1 )
				Q_strcat( cmd_args, sizeof( cmd_args ), " " );
		}

		Q_strncpyz( cmd_arg1, cmd_argv[0], sizeof( cmd_arg1 ) );
	}

	HOOK( CvarArgs ) {
		StartHook( CvarArgs )
		qasm1( pushad )
		qasm1( call Cmd_GetArgs )
		qasm1( push 0 )
		qasm1( push offset cmd_args )
		qasm1( push offset cmd_arg1 )
		qasm1( call ENG_Cvar_Set2 )
		qasm2( add esp, 0xC )
		qasm1( popad )
		qasm1( push CVA_RETPOS )
		qasm1( ret )
		EndHook( CvarArgs )
	}

#endif //HOOK_CVARARGS

#ifdef HOOK_OVERBRIGHT
	//--------------------------------
	//	Name:	Overbright patch
	//	Desc:	Add r_mapOverbrightBits support once again. Bad Raven removed it :<
	//	Hook:	R_ColorShiftLightingBytes
	//	Retn:	R_LoadLightmaps
	//--------------------------------
	#ifdef _WIN32
		#define OB_HOOKPOS		0x48D034	//	.text	E8 C7 FC FF FF			call R_ColorShiftLightingBytes
		#define OB_RETPOS		0x48D039	//	.text	C6 44 AC 73 FF			mov byte ptr [esp+ebp*4+73h], 0FFh
		#define OB_ORIGBYTES	{ 0xE8, 0xC7, 0xFC, 0xFF, 0xFF }
		#define OB_ADDRESS		0xFE3994
	#elif defined(MACOS_X)
		#error HOOK_OVERBRIGHT not available on Mac OSX
	#endif

	static void R_ColorShiftLightingBytes( byte *in, byte *out ) {
		int shift, r, g, b;
		cvar_t *r_mapOverbrightBits = ENG_Cvar_Get( "r_mapOverbrightBits", "0", CVAR_ARCHIVE );
		unsigned int tr_overbrightBits = *(unsigned int *)OB_ADDRESS;

		// shift the color data based on overbright range
		shift = r_mapOverbrightBits->integer - tr_overbrightBits;

		// shift the data based on overbright range
		r = in[0] << shift;
		g = in[1] << shift;
		b = in[2] << shift;

		// normalize by color instead of saturating to white
		if ( (r|g|b) > 255 ) {
			int		max;

			max = r > g ? r : g;
			max = max > b ? max : b;
			r = r * 255 / max;
			g = g * 255 / max;
			b = b * 255 / max;
		}

		out[0] = r;
		out[1] = g;
		out[2] = b;
		out[3] = in[3];
	}

	HOOK( Overbright ) {
		StartHook( Overbright )
		qasm1( push edi )
		qasm1( push esi )
		qasm1( call R_ColorShiftLightingBytes )
		qasm2( add esp, 8 )
		qasm1( push OB_RETPOS )
		qasm1( ret )
		EndHook( Overbright )
	}

#endif //HOOK_OVERBRIGHT

#ifdef HOOK_CONSOLETIME
	//--------------------------------
	//	Name:	Console timestamp patch
	//	Desc:	Adds a timestamp to each console print
	//	Hook:	Com_Printf
	//	Retn:	CL_ConsolePrint
	//--------------------------------
	#ifdef _WIN32
		#define CT_HOOKPOS		0x437165	//	.text	52					push edx ; skipNotify
											//			8D 54 24 10			lea edx, [esp+10h]
		#define CT_RETPOS		0x437173	//	.text	8D 4C 24 0C			lea ecx, [esp+0Ch] ; message
		#define CT_ORIGBYTES	{ 0x52, 0x8D, 0x54, 0x24, 0x10 }

		#define CT_HOOKPOS2		0x4175D8	//	.text	89 0D 08 30 88 00	mov con_x, ecx
		#define CT_RETPOS2		0x4175DE	//	.text	75 06				jnz short loc_4175E6
		#define CT_ORIGBYTES2	{ 0x89, 0x0D, 0x08, 0x30, 0x88 }

		#define CT_HOOKPOS3		0x417705	//	.text	A1 04 30 88 00		mov eax, con_current
		#define CT_RETPOS3		0x41770A	//	.text	99					cdq
		#define CT_ORIGBYTES3	{ 0xA1, 0x04, 0x30, 0x88, 0x00 }
	#elif defined(MACOS_X)
		#error HOOK_CONSOLETIME not available on Mac OSX
	#endif

#define EXAMPLE_TIMESTAMP "[^800:00:00^8] "
#define TIMESTAMP_LENGTH sizeof( EXAMPLE_TIMESTAMP )

	static const char *ct_printMsg = NULL;
	static const char *ct_newMsg = NULL;
	static unsigned int ct_msgFlag = 0;

	static void CT_FormatMessage( void ) {
		cvar_t *cg_consoleTimeStamp = ENG_Cvar_Get( "cg_consoleTimeStamp", "0", CVAR_ARCHIVE );
		char *str = (char *)ct_printMsg;
		char timestamp[TIMESTAMP_LENGTH] = { 0 };
		unsigned int *con_x = (unsigned int *)0x883008;

		while ( str = strchr( str, '^' ) )
		{//Fix ^8 and ^9 to show ^0 and ^1
			if ( str[1] == '8' || str[1] == '9' )
				str[1] -= 8;
			str++;
		}
		if ( cg_consoleTimeStamp->integer && !*con_x )
		{//only apply a timestamp if we're at the start of the line
			struct tm *timeinfo;
			time_t tm;
			cvar_t *cg_chatTimeColour = ENG_Cvar_Get( "cg_consoleTimeColour", "5", CVAR_ARCHIVE );

			time( &tm );
			timeinfo = localtime ( &tm );
			if ( !timestamp24Hour && timeinfo->tm_hour > 12 )
				timeinfo->tm_hour -= 12;
			Com_sprintf( timestamp, sizeof( timestamp ), "[^%c%02i:%02i:%02i^7] ", *cg_chatTimeColour->string, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
		}
		ct_newMsg = va( "%s%s", timestamp, ct_printMsg );
	}

	HOOK( ConsoleTime ) {
		StartHook( ConsoleTime )
		qasm1( push edx ) //edx = skipNotify
		qasm2( lea edx, [esp+16] )
		qasm2( mov ct_printMsg, edx ) //edx = msg
		qasm1( call CT_FormatMessage ) //apply timestamp if necessary
		qasm1( push ct_newMsg )
		qasm2( mov edx, CL_CONSOLEPRINT )
		qasm1( call edx )
		qasm2( add esp, 8 )
		qasm2( mov ct_msgFlag, 0 ) //msgFlag = 0
		qasm1( push CT_RETPOS )
		qasm1( ret )
		EndHook( ConsoleTime )
	}

	HOOK( ConsoleTime2 ) {
		StartHook( ConsoleTime2 )
		qasm2( mov ds:[0x883008], 0 ) //con.x = 0
		qasm2( mov ct_msgFlag, 1 ) // msgFlag = 1
		qasm1( push CT_RETPOS2 )
		qasm1( ret )
		EndHook( ConsoleTime2 )
	}

	static void FixConX( void ) {//set con.x to the width of a timestamp
		unsigned int *con_x = (unsigned int *)0x883008;
		if ( ct_msgFlag && !*con_x && ENG_Cvar_Get( "cg_consoleTimeStamp", "0", CVAR_ARCHIVE )->integer )
			*con_x = 11;
	}

	HOOK( ConsoleTime3 ) {
		StartHook( ConsoleTime3 )
		qasm1( call FixConX )
		qasm2( mov eax, ds:[0x883004] ) // we trampled over these opcodes: mov eax, con.current
		qasm1( push CT_RETPOS3 )
		qasm1( ret )
		EndHook( ConsoleTime3 )
	}

#if 0
	//--------------------------------
	//	Name:	Viewlog timestamp patch
	//	Desc:	Adds a timestamp to each viewlog print
	//	Hook:	Com_Printf
	//	Retn:	Sys_Print
	//--------------------------------
	#ifdef _WIN32
		#define VT_HOOKPOS		0x437177	//	
		#define VT_RETPOS		0x4544C0	//	.text:004544C0				Sys_Print proc near
	#elif defined(MACOS_X)
		#error HOOK_CONSOLETIME not available on Mac OSX
	#endif

	HOOK( ViewlogTime ) {
		StartHook( ViewlogTime )
		qasm2( mov ct_printMsg, ecx )
		qasm1( call CT_FormatMessage )
		qasm2( mov ecx, ct_newMsg )
		qasm1( push VT_RETPOS )
		qasm1( ret )
		EndHook( ConsoleTime )
	}
#endif
#endif //HOOK_CONSOLETIME

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
{//List of hooks to be placed
	#ifdef HOOK_SERVERSTATUS
		HOOKDEF( SS_HOOKPOS, SS_ORIGBYTES, 0xE9, Hook_ServerStatus, "Server query extensions" ),
	#endif
	#ifdef HOOK_PRIM
		HOOKDEF( PRIM_HOOKPOS, PRIM_ORIGBYTES, 0xE9, Hook_Primitives, "r_primitives bounds check" ),
	#endif
	#ifdef HOOK_CONSCROLL
		HOOKDEF( CS_HOOKPOS, CS_ORIGBYTES, 0xE9, Hook_ConScroll, "Console scrolling" ),
	#endif
	#ifdef HOOK_CONHISTORY
		HOOKDEF( CH_HOOKPOS, CH_ORIGBYTES, 0xE9, Hook_ConHistory, "Console history improvements" ),
	#endif
	#ifdef HOOK_CHATMESSAGE
		HOOKDEF( CM_HOOKPOS, CM_ORIGBYTES, 0xE8, Hook_ChatMessage, "Chat message replacement" ),
	#endif
	#ifdef HOOK_RAGNOS
		HOOKDEF( R_HOOKPOS, R_ORIGBYTES, 0xE8, Hook_Ragnos, "G2 invalid bolt patch" ),
	#endif
	#ifdef HOOK_QUERYBOOM
		HOOKDEF( QB_HOOKPOS, QB_ORIGBYTES, 0xE8, Hook_QueryBoom, "q3queryboom protection" ),
	#endif
	#ifdef HOOK_CONNJACK
		HOOKDEF( CJ_HOOKPOS, CJ_ORIGBYTES, 0xE9, Hook_ConnJack, "q3connjack protection" ),
	#endif
	#if HOOK_MASTER == 1
		HOOKDEF( MS_HOOKPOS, MS_ORIGBYTES, 0xE8, Hook_MasterServer, "Master server replacement" ),
	#endif
	#ifdef HOOK_UPDATE
		HOOKDEF( US_HOOKPOS, US_ORIGBYTES, 0xE8, Hook_UpdateServer, "Update server replacement" ),
	#endif
	#ifdef HOOK_TRANSPTEXT
		HOOKDEF( TT_HOOKPOS, TT_ORIGBYTES, 0xE9, Hook_TranspText, "Text transparency fix" ),
	#endif
	#ifdef HOOK_CONSOLEEXT
		HOOKDEF( CE_HOOKPOS, CE_ORIGBYTES, 0xE9, Hook_ConsoleExt, "Console extensions" ),
	#endif
	#ifdef HOOK_CONSOLETIME
		HOOKDEF( CT_HOOKPOS, CT_ORIGBYTES, 0xE9, Hook_ConsoleTime, "Console timestamps" ),
		HOOKDEF( CT_HOOKPOS2, CT_ORIGBYTES2, 0xE9, Hook_ConsoleTime2, "Console timestamps Pt II" ),
		HOOKDEF( CT_HOOKPOS3, CT_ORIGBYTES3, 0xE9, Hook_ConsoleTime3, "Console timestamps Pt III" ),
	//	HOOKDEF( VT_HOOKPOS, VT_ORIGBYTES, 0xE8, Hook_ViewlogTime, "Viewlog timestamps" ),
	#endif
	#ifdef HOOK_CVARSEC
		HOOKDEF( CSEC_HOOKPOS, CSEC_ORIGBYTES, 0xE8, Hook_CvarSecurity, "Cvar Security" ),
	#endif
	#ifdef HOOK_CVARARGS
		HOOKDEF( CVA_HOOKPOS, CVA_ORIGBYTES, 0xE9, Hook_CvarArgs, "Cvar Arguments" ),
	#endif
	#ifdef HOOK_OVERBRIGHT
		HOOKDEF( OB_HOOKPOS, OB_ORIGBYTES, 0xE9, Hook_Overbright, "Map Overbright Bits" ),
	#endif
	#ifdef HOOK_DERP
		HOOKDEF( W_HOOKPOS, W_ORIGBYTES, 0xE8, Hook_Derp, "Derpity Doo!" ),
	#endif
};
static const int numHooks = ARRAY_LEN( hooks );

static cvarEntry_t cvars[] =
{//List of cvars to be unlocked
	{ "r_DynamicGlowDelta", CVAR_ARCHIVE },
	{ "r_DynamicGlowIntensity", CVAR_ARCHIVE },
	{ "r_DynamicGlowPasses", CVAR_ARCHIVE },
	{ "r_DynamicGlowSoft", CVAR_ARCHIVE },
	{ "r_DynamicGlowWidth", CVAR_ARCHIVE|CVAR_LATCH },
	{ "r_DynamicGlowHeight", CVAR_ARCHIVE|CVAR_LATCH },
	{ "r_ambientScale", CVAR_ARCHIVE },
	{ "r_directedScale", CVAR_ARCHIVE },
	{ "r_fullbright", CVAR_ARCHIVE },
	{ "viewlog", CVAR_ARCHIVE },
	{ "r_autoMapDisable", CVAR_ARCHIVE },
	{ "r_clear", CVAR_ARCHIVE },
	{ "r_drawfog", CVAR_ARCHIVE },
};
static const int numCvars = ARRAY_LEN( cvars );

typedef struct cmdTable_s {
	const char		*strFrom,	*strTo;
	unsigned int	funcFrom,	funcTo;
} cmdTable_t;

static const cmdTable_t cmdTable[] =
{//List of commands to be rerouted
#if !MAC_PORT
	{ "serverstatus", "serverstatus", 0x4212C0, (unsigned int)JP_QueryServer },
	{ "globalservers", "globalservers", 0x420B30, (unsigned int)CL_GlobalServers_f },
#endif
};
static const int numCmds = ARRAY_LEN( cmdTable );

#endif // !OPENJK

#ifdef HOOK_CONSOLETIME
	static void Get24HourTime( void )
	{
		#ifdef _WIN32
			//Detect the timestamp format via the registry
			#define JP_TIMESTAMP_REGISTRY_KEY "Control Panel\\International"
			#define JP_TIMESTAMP_REGISTRY_NAME "sTimeFormat"
			char registryValue[256] = { 0 };
			HKEY hkey;
			unsigned long datalen = sizeof( registryValue );  // data field length(in), data returned length(out) 
			unsigned long datatype; // #defined in winnt.h (predefined types 0-11)
			LSTATUS error;
			if ( (error = RegOpenKeyExA( (HKEY)HKEY_CURRENT_USER, (LPCSTR)JP_TIMESTAMP_REGISTRY_KEY, (DWORD)0, (REGSAM)KEY_QUERY_VALUE, &hkey )) == ERROR_SUCCESS )
			{
				if ( (error = RegQueryValueExA( (HKEY)hkey, (LPCSTR)JP_TIMESTAMP_REGISTRY_NAME, (LPDWORD)NULL, (LPDWORD)&datatype, (LPBYTE)registryValue, (LPDWORD)&datalen)) == ERROR_SUCCESS )
				{
					if ( registryValue[0] == 'H' )
						timestamp24Hour = qtrue;
					RegCloseKey( hkey );
				}
				else
					Com_Printf( "^1Error, couldn't query registry string %s, error code %i\n", "sTimeFormat", error );
			}
			else
				Com_Printf( "^1Error, couldn't open registry key %s, error code %i\n", JP_TIMESTAMP_REGISTRY_KEY, error );
		#endif
	}
#endif

#if !MAC_PORT
static void Minimize( void ) {
	ShowWindow( FindWindowA( "Jedi Knight®: Jedi Academy (MP)", NULL ), SW_MINIMIZE );
}
#endif

void PatchEngine( void )
{//Place all patches
	#ifndef OPENJK
		int i;
	
		Com_Printf( "Installing engine patches (UI)\n" );
	
		#ifdef HOOK_CONSOLETIME
			Get24HourTime();
		#endif
	
		#ifdef _WIN32
			_setmaxstdio( 2048 );
			ENG_FS_InitFilesystem();
		#endif
	
		// unlock cvars
		for ( i=0; i<numCvars; i++ )
			CVAR_UNLOCK( cvars[i].cvarname, cvars[i].flags )
	
		// place hooks
		for ( i=0; i<numHooks; i++ )
			PlaceHook( &hooks[i] );
	
		// re-route commands
		for ( i=0; i<numCmds; i++ )
			CMD_REROUTE( cmdTable[i].strFrom, cmdTable[i].strTo, cmdTable[i].funcFrom, cmdTable[i].funcTo )
		ENG_Cmd_AddCommand( "server", NULL ); //Add to the auto-complete list, but don't attach a function - it's explicitly handled in UI code -_-..
		ENG_Cmd_AddCommand( "japp_favsrv_list", NULL );
		ENG_Cmd_AddCommand( "japp_favsrv_add", NULL );
		ENG_Cmd_AddCommand( "jappvideomode", NULL );
		#if !MAC_PORT
			ENG_Cmd_AddCommand( "mini", Minimize );
			ENG_Cmd_AddCommand( "ragequit", (void (*)())0x4374F0 );
			ENG_Cmd_AddCommand( "rq", (void (*)())0x4374F0 );
		#endif
		ENG_Cmd_AddCommand( "update", UpdateJAPP );
	
		#if !MAC_PORT
			//Byte patches
			PATCH( 0x41D9B8, byte, 0xB8 );				//	NO-CD
			PATCH( 0x45DE61, byte, 0xB8 );				//	NO-CD
			PATCH( 0x52C329, byte, 0xB8 );				//	NO-CD
			PATCH( 0x454B5A, unsigned short, 0x9090 );	//	Alt-enter
			PATCH( 0x414B78, byte, 0xEB );				//	Uncap cl_timeNudge
			PATCH( 0x414B84, byte, 0xEB );				//	Uncap cl_timeNudge
			PATCH( 0x41A404, byte, 0xEB );				//	Uncap cl_maxPackets
			PATCH( 0x41A412, byte, 0xEB );				//	Uncap cl_maxPackets
			PATCH( 0x41A55B, byte, 0xEB );				//	Uncap cl_packetdup
			PATCH( 0x41A569, byte, 0xEB );				//	Uncap cl_packetdup
			PATCH( 0x48904b, unsigned short, 0x9090 );	//	Bypass sv_cheats for 'r_we'
			PATCH( 0x41CA05, unsigned short, 0x9090 );	//	Bypass g_synchronousClients for 'record' (demo recording)
			PATCH( 0x41CA1D, byte, 0xEB );				//	Bypass g_synchronousClients for 'record' (demo recording)
			PATCH( 0x436C83, byte, 0x77 );				//	Fix Cmd_TokenizeString's ANSI support (replaces JG (0x7F) by JA (0x77))
			PATCH( 0x41B988, unsigned int, 0x90909090 );//	Don't send chat command if there was no '/' or '\\' in the console buffer
			PATCH( 0x41B98c, byte, 0x90 );				//	Don't send chat command if there was no '/' or '\\' in the console buffer
			PATCH( 0x41B955, unsigned short, 0x9090 );	//	Autocomplete console command even if it didn't start with '/' or '\\'
			PATCH( 0x4385C9, unsigned short, 0x0001 );	//	set 'viewlog' cvar flags to CVAR_ARCHIVE instead of CVAR_CHEAT
		#endif	
		//RAZTODO: Hook instead for cvar
	//	UnlockMemory( 0x416FFD, 17 );
	//	memset( (void *)0x416FFD, 0x90, 17 );		//	Don't erase console field upon toggling
	//	LockMemory( 0x416FFD, 17 );
	
		Com_Printf( "\n" );
	#else
		Com_Printf( "Engine modifications are disabled\n" );
	#endif
}

void UnpatchEngine( void )
{//Remove all patches
	#ifndef OPENJK
		int i;
	
		Com_Printf( "Removing engine patches (UI)\n" );
	
		// re-route commands
		ENG_Cmd_RemoveCommand( "jappvideomode" );
		#if !MAC_PORT
			ENG_Cmd_RemoveCommand( "mini" );
			ENG_Cmd_RemoveCommand( "ragequit" );
			ENG_Cmd_RemoveCommand( "rq" );
		#endif
		ENG_Cmd_RemoveCommand( "update" );
	
		for ( i=0; i<numCmds; i++ )
			CMD_REROUTE( cmdTable[i].strTo, cmdTable[i].strFrom, cmdTable[i].funcTo, cmdTable[i].funcFrom );
	
		// remove hooks
		for ( i=0; i<numHooks; i++ )
			RemoveHook( &hooks[i] );
	
		#if !MAC_PORT
			// byte patches
			PATCH( 0x41D9B8, byte, 0xE8 );				//	NO-CD
			PATCH( 0x45DE61, byte, 0xE8 );				//	NO-CD
			PATCH( 0x52C329, byte, 0xE8 );				//	NO-CD
			PATCH( 0x454B5A, unsigned short, 0x7539 );	//	Alt-enter
			PATCH( 0x414B78, byte, 0x7D );				//	Uncap cl_timeNudge
			PATCH( 0x414B84, byte, 0x7E );				//	Uncap cl_timeNudge
			PATCH( 0x41A404, byte, 0x7D );				//	Uncap cl_maxPackets
			PATCH( 0x41A412, byte, 0x7E );				//	Uncap cl_maxPackets
			PATCH( 0x41A55B, byte, 0x7D );				//	Uncap cl_packetdup
			PATCH( 0x41A569, byte, 0x7E );				//	Uncap cl_packetdup
			PATCH( 0x48904b, unsigned short, 0x7440 );	//	Bypass sv_cheats for 'r_we'
			PATCH( 0x41CA05, unsigned short, 0x7418 );	//	Bypass g_synchronousClients for 'record' (demo recording)
			PATCH( 0x41CA1D, byte, 0x7A );				//	Bypass g_synchronousClients for 'record' (demo recording)
			PATCH( 0x436C83, byte, 0x7F );				//	Fix Cmd_TokenizeString's ANSI support (replaces JG (0x7F) by JA (0x77))
			PATCH( 0x41B988, unsigned int, 0xE813AD01 );//	Don't send chat command if there was no '/' or '\\' in the console buffer
			PATCH( 0x41B98c, byte, 0x00 );				//	Don't send chat command if there was no '/' or '\\' in the console buffer
			PATCH( 0x41B955, unsigned short, 0xEB05 );	//	Autocomplete console command even if it didn't start with '/' or '\\'
			PATCH( 0x4385C9, unsigned short, 0x0002 );	//	set 'viewlog' cvar flags to CVAR_ARCHIVE instead of CVAR_CHEAT
		#endif	
	
		Com_Printf( "\n" );
	#endif
}

