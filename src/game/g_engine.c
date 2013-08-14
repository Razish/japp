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

#include "g_local.h"
#include "g_engine.h"

#ifndef OPENJK

// --------------------------------
// Conditional patching

#define HOOK_Q3INFOBOOM			//	Q3infoboom: truncate large arguments
#define HOOK_Q3FILL				//	Q3fill: Drop inactive connections
#define HOOK_RCONExtensions		//	RCON extension: logging commands executed
#define HOOK_HeartbeatSpam		//	Heartbeat spam: Cvar to control it
#define HOOK_SvSayFix			//	SvSay: Add chat separator to svsay command
#define HOOK_DoneDL				//	DoneDL: Return early in donedl to prevent ClientBegin being called before player_die


// --------------------------------
// Engine functions (Will be prefixed with ENG_)

#ifdef _WIN32

	serverStatic_t *svs = (serverStatic_t *)0x606218;
	#define CVAR_FINDVAR	    0x411E70
	#define SYS_LISTFILES		0x44BD30
	#define SYS_FREEFILELIST    0x44BF90
	#define FS_BUILDOSPATH		0x412E60
	EFUNC( const char *,	Cmd_Argv,			(int arg),													0x40F490 );
	EFUNC( void,			Q_stricmp,			(const char *s1, const char *s2),							0x41B8A0 );
	EFUNC( char *,			SV_ExpandNewlines,	(const char *str),											0x4434D0 );
	EFUNC( int,				FS_FOpenFileByMode,	(const char *qpath, fileHandle_t *f, fsMode_t mode),		0x4160C0 );
	EFUNC( void,			FS_FCloseFile,		(fileHandle_t f),											0x4135E0 );
	EFUNC( int,				FS_Write,			(const void *buffer, int len, fileHandle_t f),				0x414350 );
	EFUNC( char *,			FS_BuildOSPath,		(const char *base, const char *game, const char *qpath ),	0x412E60 );
	EFUNC( char **,			Sys_ListFiles,		(const char *directory, const char *extension, char *filter, int *numfiles, int wantsubs ),	0x44BD30 ); 
	EFUNC( void,			Sys_FreeFileList,	(char **list),												0x44BF90 );
	EFUNC( char *,			Sys_Cwd,			(void),														0x44BAB0 );
	EFUNC( cvar_t *,		Cvar_Get,			(const char *var_name, const char *var_value, int flags),	0x412AC0 );

#elif defined(__linux__)

	serverStatic_t *svs = (serverStatic_t *)0x83121E0;
	EFUNC( const char *,	Cmd_Argv,			(int arg),													0x812C264 );
	EFUNC( void,			Q_stricmp,			(const char *s1, const char *s2),							0x807F434 );
	EFUNC( int,				FS_FOpenFileByMode,	(const char *qpath, fileHandle_t *f, fsMode_t mode),		0x8131574 );
	EFUNC( void,			FS_FCloseFile,		(fileHandle_t f),											0x812D1B4 );
	EFUNC( int,				FS_Write,			(const void *buffer, int len, fileHandle_t f),				0x812E074 );
	EFUNC( char *,			FS_BuildOSPath,		(const char *base, const char *game, const char *qpath),	0x812B8F4 );

#endif //_WIN32


// --------------------------------
// Hooks

#ifdef HOOK_Q3INFOBOOM
	//--------------------------------
	//	Name:	Q3infoboom patch
	//	Desc:	Because Luigi Auriemma's 'fix' has side-effects such as userinfo being cut off
	//				from the connect packet, JA++ implements a custom protection against this issue.
	//				Any oversized commands will be truncated, and attacker's information will be logged.
	//				This will also undo the fix from Luigi (revert the command truncation from 0x1FF (511) to 0x3FF (1023)
	//	Hook:	SV_ConnectionlessPacket
	//	Retn:	Q_stricmp
	//--------------------------------

	#ifdef _WIN32
		#define Q3IB_MSGPATCH	0x418B2C	//	.text	81 FE FF 03			cmp		esi, 3FFh
		#define Q3IB_HOOKPOS	0x443F7F	//	.text	E8 1C 79 FD FF		call	Q_stricmp
		#define Q3IB_ORIGBYTES	{ 0xE8, 0x1C, 0x79, 0xFD, 0xFF }
	#elif defined(__linux__)
		#define Q3IB_MSGPATCH	0x807803D	//	.text	81 FB FF 01			cmp		ebx, 1FFh
		#define Q3IB_HOOKPOS	0x8056E23	//	.text	E8 0C 86 02			call	Q_stricmp
		#define Q3IB_ORIGBYTES	{ 0xE8, 0x0C, 0x86, 0x02, 0x00 }
	#endif //_WIN32

	static void USED CheckConnectionlessPacket( const char *cmd, const char *ip )
	{//Truncate any oversized commands
		char *s = (char *)ENG_Cmd_Argv( 1 );

		if ( !Q_stricmp( cmd, "getstatus" ) || !Q_stricmp( cmd, "getinfo" ) )
		{//	We got a risky function here, get arg 1 and truncate if needed
			if ( strlen( s ) > 32 )
			{// 32 chars should be more than enough for the challenge number
				s[32] = '\0';
				G_SecurityLogPrintf( "Attempted q3infoboom from %s with command %s\n", ip, cmd );
			}
		}

		else if ( !Q_stricmp( cmd, "connect" ) || !Q_stricmp( cmd, "rcon" ) )
		{
			if ( strlen( s ) > 980 )
			{
				s[980] = '\0';
				G_SecurityLogPrintf( "Attempted q3infoboom from %s with command %s\n", ip, cmd );
			}
		}
	}

	HOOK( Q3InfoBoom ) {
		StartHook( Q3InfoBoom )
		qasm1( pushad )							// Secure registers
		qasm1( push [esp+48] )					// Push IP
		qasm1( push ebx )						// Push command
		qasm1( call CheckConnectionlessPacket )	// Truncate the argument if needed
		qasm2( add esp, 8 )						// Clean up stack
		qasm1( popad )							// Clean up stack
		qasm1( push ENG_Q_stricmp )				// Push ENG_Q_stricmp
		qasm1( ret )							// Return execution flow into Q_stricmp
		EndHook( Q3InfoBoom )
	}
#endif //HOOK_Q3INFOBOOM

#ifdef HOOK_Q3FILL
	//--------------------------------
	//	Name:	Connection Activity Check
	//	Desc:	To verify if a client is valid, the server will check if the connection is active or not.
	//				Q3fill will not send anything after the connect request.
	//				To detect this, we'll place a hook on the function that detects ingame packets
	//				Once a client sends a valid packet, we'll mark their connection as active
	//				If, after a designated time, the client has not sent any valid packets, they will be considered fake and dropped
	//	Hook:	SV_ReadPackets
	//	Retn:	SV_ExecuteClientMessage
	//--------------------------------

	#ifdef _WIN32
		#define CACH_HOOKPOS		0x44420E	// .text	E8 8D 81 FF FF		call	SV_ExecuteClientMessage
		#define CACH_REG			ebx
		#define CACH_SVSCLIENTSPOS	0x606224
		#define CACH_RETFUNC		0x43C3A0	//	.text	SV_ExecuteClientMessage proc near
		#define CACH_ORIGBYTES { 0xE8, 0x8D, 0x81, 0xFF, 0xFF }
	#else
		#define CACH_HOOKPOS		0x80571EF	//	.text	E8 D0 76 FF FF		call	SV_ExecuteClientMessage
		#define CACH_REG			esi
		#define CACH_SVSCLIENTSPOS	0x83121EC
		#define CACH_RETFUNC		0x804E8C4	//	.text	SV_ExecuteClientMessage proc near
		#define CACH_ORIGBYTES { 0xE8, 0xD0, 0x76, 0xFF, 0xFF }
	#endif //_WIN32

	static void USED ConnActivityCheck( int clientNum )
	{
		if ( !level.security.clientConnectionActive[clientNum] )
		{
			trap->Print( "Client %i (%s) has an active connection\n", clientNum, level.clients[clientNum].pers.netname );
			level.clients[clientNum].sess.validated |= (1<<CV_ACTIVECON);
			level.security.clientConnectionActive[clientNum] = qtrue;
		}
	}

	HOOK( CACheck ) {
		StartHook( CACheck )
		qasm1( pushad )								// Secure registers
		qasm2( mov eax, CACH_REG )					// Try to figure out the clientNum
		qasm2( sub eax, DS:[CACH_SVSCLIENTSPOS] )	// Get array offset (in svs.clients)
		qasm2( xor edx, edx )						// Clear upper 32-bits of dividend
		qasm2( mov ecx, 0x51478 )					// Set divisor (sizeof(client_t))
		qasm1( div ecx )							// Divide (edx:eax / ecx -> eax (quotient) + edx (remainder)
		qasm2( cmp eax, MAX_CLIENTS )				// See if its above MAX_CLIENTS
		qasm1( jnb bad )							// If so, bail
		qasm1( push eax )							// Push client num
		qasm1( call ConnActivityCheck )				// Call
		qasm2( add esp, 4 )							// Clean up stack

		qasmL(bad: )
		qasm1( popad )								// Restore registers
		qasm1( push CACH_RETFUNC )					// Push-ret jump to SV_ExecuteClientMessage
		qasm1( ret )
		EndHook( CACheck )
	}
#endif //HOOK_Q3FILL

#ifdef HOOK_RCONExtensions
	//--------------------------------
	//	Name:	RCON example hook
	//	Desc:	Just a simple hook in SVC_RemoteCommand.
	//	Hook:	SVC_RemoteCommand
	//	Retn:	SV_ExecuteClientMessage
	//--------------------------------

	#ifdef _WIN32
		#define RCON_HOOKPOS	0x443CED
		#define RCON_RETFUNC	0x410EE0
		#define RCON_ORIGBYTES { 0xE8, 0xEE, 0xD1, 0xFC, 0xFF }
	#else
		#define RCON_HOOKPOS	0x8056B26
		#define RCON_RETFUNC	0x80744E4
		#define RCON_ORIGBYTES { 0xE8, 0xB9, 0xD9, 0x01, 0x00 }
	#endif //_WIN32

	static void USED RCONExtensions( const char *command ) {
		G_LogPrintf( "RCON executed: %s", command );
	}

	HOOK( RCONExtensions ) {
		StartHook( RCONExtensions )
		qasm1( pushad )					// Secure registers
		qasm1( push [esi] )				// Pass command to RconFix
		qasm1( call RCONExtensions )	// Call
		qasm2( add esp, 4 )				// Clean up stack
		qasm1( popad )					// Restore registers
		qasm1( push RCON_RETFUNC )		// Push Com_Milliseconds (In the engine)
		qasm1( ret )					// Return execution flow into Com_Milliseconds as if nothing happened
		EndHook( RCONExtensions )
	}
#endif //HOOK_RCONExtensions

#ifdef HOOK_HeartbeatSpam
	//--------------------------------
	//	Name:	Heartbeat Spam
	//	Desc:	Added a cvar check to enable/disable printing of "Sending heartbeat to %s"
	//	Hook:	UNNAMED (TODO: Check q3 source)
	//	Retn:	Com_Printf or Com_DPrintf
	//--------------------------------

#ifdef _WIN32
	#define HB_HOOKPOS			0x443804
	#define HB_RETFAIL			0x40FDB0
	#define HB_RETSUCCESS		0x40FBE0
	#define HB_ORIGBYTES { 0xE8, 0xD7, 0xC3, 0xFC, 0xFF }
#else
	#define HB_HOOKPOS			0x80564E7
	#define HB_RETFAIL			0x8072ED4
	#define HB_RETSUCCESS		0x8072CA4
	#define HB_ORIGBYTES { 0xE8, 0xB8, 0xC7, 0x01, 0x00 }
#endif //_WIN32

	static int USED HB_Check( void ) {
		return japp_showHeartbeats.integer;
	}

	HOOK( HeartbeatSpam ) {
		StartHook( HeartbeatSpam )
		qasm1( pushad )
		qasm1( call HB_Check ) // call because had issues with gcc's inline assembler -_-...
		qasm2( cmp eax, 0 )
		qasm1( jnz allowSpam )
		qasm1( popad )
		qasm1( push HB_RETFAIL )
		qasm1( ret )

		qasmL(allowSpam: )
		qasm1( popad )
		qasm1( push HB_RETSUCCESS )		// Push Com_Printf (In the engine)
		qasm1( ret )					// Return execution flow into Com_Printf
		EndHook( HeartbeatSpam )
	}
#endif //HOOK_HeartbeatSpam

#ifdef HOOK_SvSayFix
	//--------------------------------
	//	Name:	SvSay Fix
	//	Desc:	Add chat separator to svsay command
	//	Hook:	SV_SvSay_f
	//	Retn:	SV_SendServerCommand
	//--------------------------------

#ifdef _WIN32
	#define SVS_HOOKPOS			0x43A744
	#define SVS_RETPOS			0x43A749
	#define SVS_ORIGBYTES { 0xE8, 0x87, 0x8E, 0x00, 0x00 }
	#define SVS_STACKOFFSET 20
#else
	#define SVS_HOOKPOS			0x804FB2D
	#define SVS_RETPOS			0x804FB32
	#define SVS_ORIGBYTES { 0xE8, 0xE2, 0x66, 0x00, 0x00 }
	#define SVS_STACKOFFSET 28
#endif //_WIN32

	static void USED SvSay_Handler( char *msg ) {
		trap->SendServerCommand( -1, va( "chat \"Server^7\x19: ^7%s\"", msg+8 ) );
	}

	HOOK( SvSay ) {
		StartHook( SvSay )
		qasm1( pushad )
		qasm1( push [esp+SVS_STACKOFFSET] )
		qasm1( call SvSay_Handler )
		qasm2( add esp, 4 )
		qasm1( popad )
		qasm1( push SVS_RETPOS )
		qasm1( ret )
		EndHook( SvSay )
	}
#endif //HOOK_SvSayFix

#ifdef HOOK_DoneDL
	//--------------------------------
	//      Name:   DoneDL Fix
	//      Desc:   /donedl is often abused to make flags disappear and other game-related data
	//                      We prevent active clients from using this.
	//      Hook:   SV_SendClientGameState
	//      Retn:   SV_SendClientGameState
	//--------------------------------

	#ifdef _WIN32
		#define DDL_HOOKPOS			0x43AEF4	//	mov dword ptr [esi], CS_PRIMED
		#define DDL_RETSUCCESS		0x43AEFA	//	mov dword ptr [esi+39430h], 0
		#define DDL_ORIGBYTES		{ 0xC7, 0x06, 0x03, 0x00, 0x00 }
	#else
		#define DDL_HOOKPOS			0x804CF54	// mov dword ptr [esi], CS_PRIMED
		#define DDL_RETSUCCESS		0x804CF5A	// mov eax, [esi+39458h]
		#define DDL_ORIGBYTES		{ 0xC7, 0x06, 0x03, 0x00, 0x00 }
	#endif //_WIN32

	static void USED DoneDL_Handler( client_t *client )
	{
		// fix: set CS_PRIMED only when CS_CONNECTED is current state
		if ( client->state == CS_CONNECTED )
			client->state = CS_PRIMED;
		else
		{
			char tmpIP[NET_ADDRSTRMAXLEN] = {0};
			NET_AddrToString( tmpIP, sizeof( tmpIP ), &client->netchan.remoteAddress );
			G_SecurityLogPrintf( "Client %d (%s) probably tried \"donedl\" exploit when client->state(%d)!=CS_CONNECTED(%d) [IP: %s]\n", client->gentity->s.number, client->name, client->state, CS_CONNECTED, tmpIP );
		}
	}

	HOOK( DoneDL ) {
		StartHook( DoneDL )
		qasm1( pushad )
		qasm1( push esi )
		qasm1( call DoneDL_Handler )
		qasm2( add esp, 4 )
		qasm1( popad )
		qasm1( push DDL_RETSUCCESS )
		qasm1( ret )
		EndHook( DoneDL )
	}
#endif //HOOK_DoneDL


// --------------------------------
// Hook table

static hookEntry_t hooks[] =
{
	#ifdef HOOK_Q3INFOBOOM
		HOOKDEF( Q3IB_HOOKPOS,	Q3IB_ORIGBYTES,	0xE8,	Hook_Q3InfoBoom,		"Q3InfoBoom patch"							),
	#endif
	#ifdef HOOK_Q3FILL
		HOOKDEF( CACH_HOOKPOS,	CACH_ORIGBYTES,	0xE8,	Hook_CACheck,			"Connection activity check (Anti-Q3Fill)"	),
	#endif
	#ifdef HOOK_RCONExtensions
		HOOKDEF( RCON_HOOKPOS,	RCON_ORIGBYTES,	0xE8,	Hook_RCONExtensions,	"RCON extensions"							),
	#endif
	#ifdef HOOK_HeartbeatSpam
		HOOKDEF( HB_HOOKPOS,	HB_ORIGBYTES,	0xE8,	Hook_HeartbeatSpam,		"'Sending heartbeat' spam"					),
	#endif
	#ifdef HOOK_SvSayFix
		HOOKDEF( SVS_HOOKPOS,	SVS_ORIGBYTES,	0xE9,	Hook_SvSay,				"SvSay fix"									),
	#endif
	#ifdef HOOK_DoneDL
		HOOKDEF( DDL_HOOKPOS,	DDL_ORIGBYTES,	0xE9,	Hook_DoneDL,			"DoneDL fix"									),
	#endif
};
static const int numHooks = ARRAY_LEN( hooks );

#endif // !OPENJK

void ActivateCrashHandler( void );
void DeactivateCrashHandler( void );
void PatchEngine( void )
{
	#ifndef OPENJK
		int i;

		Com_Printf( "JA++: Installing engine patches (GAME)\n" );

		#ifdef HOOK_Q3INFOBOOM
			PATCH( Q3IB_MSGPATCH, unsigned int, 0x3FF );
		#endif

		for ( i=0; i<numHooks; i++ )
			PlaceHook( &hooks[i] );
	
		ActivateCrashHandler();
		level.security.isPatched = qtrue;
	#else
		Com_Printf( "Engine modifications are disabled\n" );
	#endif // !OPENJK
}

void UnpatchEngine( void )
{
	#ifndef OPENJK
	if ( level.security.isPatched )
	{
		int i;

		Com_Printf( "JA++: Removing engine patches (GAME)\n" );

		level.security.isPatched = qfalse;
		DeactivateCrashHandler();

		for ( i=0; i<numHooks; i++ )
			RemoveHook( &hooks[i] );

		#ifdef HOOK_Q3INFOBOOM
			PATCH( Q3IB_MSGPATCH, unsigned int, 0x1FF );
		#endif
	}
	#endif //!OPENJK
}
