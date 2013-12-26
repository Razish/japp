////////////////////////////////
//							  
// Jedi Knight Galaxies crash handler 
//
// In the somewhat unlikely event of a crash
// this code will create a thorough crash log so the cause can be determined
//
////////////////////////////////

#include "q_shared.h"
#include "qcommon/game_version.h"

#if QARCH == 64 || defined(MACOS_X)

// wat do?

void ActivateCrashHandler( void ) {
}

void DeactivateCrashHandler( void ) {
}

#elif QARCH == 32

#ifdef _MSC_VER
	#pragma comment( lib, "DbgHelp" )
	#pragma comment( lib, "Psapi" )
#endif

int	bCrashing = 0;

//#include <disasm/disasm.h>
#include "shared/libudis86/udis86.h"

#include <time.h>
#include "qcommon/disablewarnings.h"
#include "shared/JAPP/jp_crash.h"
#include "g_local.h"

#include <string.h>
#include <stdio.h>

#ifdef _WIN32
	#include <windows.h>
#else
	#define __USE_GNU
	#include <signal.h>
	#include <unistd.h>
	#include <execinfo.h>
	// disasm.h defines REG_xxx constants which will conflict, so undefine them now
	#undef REG_EAX
	#undef REG_EBX
	#undef REG_ECX
	#undef REG_EDX
	#undef REG_ESI
	#undef REG_EDI
	#undef REG_ESP
	#undef REG_EBP
	#include <sys/utsname.h>
	#include <link.h>
	#define __USE_GNU
	#include <sys/ucontext.h>
	#include <features.h>
#endif

static void JKG_FS_WriteString(const char *msg, fileHandle_t f) {
	trap->FS_Write( msg, strlen(msg), f );
}

int StrToDword(const char *str) {
	int len = strlen(str);
	int pos;
	int val;
	char ch;
	int result = 0;
	int hb; // Halfbyte (see below) to start with
	// Layout:
	//      7      6      5      4      3      2      1      0    <-- Halfbytes
	//    <xxxx> <xxxx> <xxxx> <xxxx> <xxxx> <xxxx> <xxxx> <xxxx> <-- Bits (x = bit)
	//
	// Each hex digit represents 4 bits
	// So 8 digits and 32 bits

	if (len < 3 || len > 10) { // Invalid length
		return 0;
	}
	// Determine starting halfbyte:
	hb = (len-3);
	for (pos = 2; pos < len; hb--, pos++) {
		// Alright parse it
		ch = str[pos];
		if (ch >= '0' && ch <= '9')
			val = ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			val = ch - 'A' + 10;
		else if (ch >= 'a' && ch <= 'f')
			val = ch - 'a' + 10;
		else
			return 0; // Bad digit
		// Ok the char is valid, parsy time
		result |= ((val & 15 ) << (hb*4));
	}
	return result;
}

void Cmd_DisAsmDirect_f() {
	char Addrbuf[32];
	ud_t disasm;
	int Addr;
	if (trap->Argc() < 2) {
		trap->Print("Usage: /disasm <address>\n");
		return;
	}
	trap->Argv(1,Addrbuf,32);
	// Look for 0x notation
	if (Addrbuf[0] == '0' && Addrbuf[1] == 'x') {
		Addr = StrToDword(Addrbuf);
	} else {
		Addr = atoi(Addrbuf);
	}
	if (!Addr) {
		trap->Print("Bad pointer provided, aborting\n");
		return;
	}

	ud_init(&disasm);
	ud_set_input_buffer(&disasm, (uint8_t *)Addr, 16);
	ud_set_mode(&disasm, 32);
	ud_set_pc(&disasm, Addr);
	ud_set_syntax(&disasm, UD_SYN_INTEL);

	ud_disassemble(&disasm);

	trap->Print("%08X: %s (%s)\n", Addr, ud_insn_asm(&disasm), ud_insn_hex(&disasm));
}

void JKG_ExtCrashInfo(int fileHandle) {
#ifdef _GAME
	char cs[1024];
	// In case of a crash, the auxiliary library will write a report
	// If jampgame is still loaded when the crash occours (and this is usually the case)
	// this function will be called to provide additional information
	// Such as the map the server was on, the clients on it,etc

	fileHandle_t f = (fileHandle_t)fileHandle;
	JKG_FS_WriteString("----------------------------------------\n"
					   "          Server info / players\n"
					   "----------------------------------------\n", f);
	trap->GetServerinfo( cs, sizeof( cs ) );
	JKG_FS_WriteString(va("Map: %s\n\n", Info_ValueForKey( cs, "mapname" )), f);
	JKG_FS_WriteString(va("Players: %i/%i:\n\n", level.numConnectedClients, level.maxclients), f);
	if (level.numConnectedClients != 0) {
		int i;
		JKG_FS_WriteString("|ID|Name                                |Ping|IP                    |\n",f);
		JKG_FS_WriteString("+--+------------------------------------+----+----------------------+\n",f);
		for (i=0; i < level.maxclients; i++) {
			if ( level.clients[i].pers.connected != CON_DISCONNECTED )
				JKG_FS_WriteString( va( "|%-2i|%-36s|%-4i|%-24s|\n", i, level.clients[i].pers.netname,
					level.clients[i].ps.ping, level.clients[i].sess.IP ), f );
		}
		JKG_FS_WriteString("+--+------------------------------------+----+----------------------+\n",f);
	}
#else
	// TODO: communicate with cgame for server info?
#endif
}

static unsigned long DisasmBacktrace(unsigned char *block, unsigned long base, unsigned long size, unsigned long ip, int n) {
	int i;
	unsigned long abuf[128];
	unsigned long addr;
	unsigned long back;
	unsigned long cmdsize;
	unsigned char *pdata;

	ud_t ud;

	// Check if block is not NULL
	if (block == NULL)	
		return 0;

	// Clamp range to 0-127
	if (n < 0) {		
		n = 0;
	} else if (n > 127) {
		n = 127; 
	}

	// No need to process this one, just return the IP
	if (n == 0)
		return ip;

	// Ensure the IP is within range
	if (ip > base + size)		
		ip = base + size;

	// If the goal instruction is guaranteed going to go under the base address
	// don't bother searching and just return the base address
	if (ip <= base + n)
		return base;
  

	// Calculate how far back we should start scanning
	// assuming an instruction cannot be larger than 16 bytes.
	back = 16 * (n + 3);

	// If this goes too far back, clamp it to block instead
	if (ip < base + back)
		back = ip - base;

	addr = ip - back;
	pdata = block + (addr - base);

	// Prepare udis86
	ud_init(&ud);
	ud_set_mode(&ud, 32);
	ud_set_syntax(&ud, NULL);

	ud_set_input_buffer(&ud, pdata, back + 16);
	ud_set_pc(&ud, addr);

	for (i = 0; addr < ip; i++) {
		abuf[i % 128] = addr;

		cmdsize = ud_disassemble(&ud);

		if (!cmdsize) break;

		addr += cmdsize;
	}

	if (i < n) {
		return abuf[0];
	} else {
		return abuf[(i - n + 128) % 128];
	}
}

static unsigned int GetJumpTarget(ud_t *ud, int operand)
{
	switch (ud->operand[0].size) {
		case 8:
			return ud->pc + ud->operand[operand].lval.sbyte; 
			break;
		case 16:
			return ud->pc + ud->operand[operand].lval.sword; 
			break;
		case 32:
			return ud->pc + ud->operand[operand].lval.sdword; 
			break;
		default:
			return 0;
			break;
	}
}

static const char *JKG_Crash_GetCrashlogName() {
	static char buf[1024] = {0};
	time_t rawtime;

	time( &rawtime );
	strftime( buf, sizeof( buf ), "JAPP-Crashlog_%Y-%m-%d_%H-%M-%S.log", gmtime( &rawtime ) );

	return buf;
}

static void JP_ForceQuit( void ) {
	trap->Error( ERR_DROP, "Server crash\n" );
}

#ifdef _WIN32
#include <dbghelp.h>
#include <tlhelp32.h>
#include <psapi.h>
// Windows version of the crash handler
LPTOP_LEVEL_EXCEPTION_FILTER oldHandler = 0;
// Used in case of a stack overflow
char StackBackup[0x18000];
unsigned int StackBackupStart;


typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

const char *JKG_GetOSDisplayString( )
{
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	PGPI pGPI;
	BOOL bOsVersionInfoEx;
	DWORD dwType;
	char buf[80];
	static char name[1024];

	memset(&name, 0, sizeof(name));
	memset(&si, 0, sizeof(SYSTEM_INFO));
	memset(&osvi, 0, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		return name;

   // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

	pGNSI = (PGNSI) GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetNativeSystemInfo");
	if( pGNSI != NULL ) {
		pGNSI(&si);
	} else {
		GetSystemInfo(&si);
	}

	strcpy(name, "Microsoft ");
	if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion > 4 )
	{
		// Test for the specific product.

		if ( osvi.dwMajorVersion == 6 )
		{
			if( osvi.dwMinorVersion == 0 )	{
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					strcat(name, "Windows Vista ");
				} else  {
					strcat(name, "Windows Server 2008 ");
				}
			}

			if ( osvi.dwMinorVersion == 1 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					strcat(name, "Windows 7 ");
				} else {
					strcat(name, "Windows Server 2008 R2 ");
				}
			}

			pGPI = (PGPI) GetProcAddress( GetModuleHandleA("kernel32.dll"), "GetProductInfo");

			pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

			switch( dwType )
			{
			case PRODUCT_ULTIMATE:
				strcat(name, "Ultimate Edition");
				break;
			/*case PRODUCT_PROFESSIONAL:
				strcat(name, "Professional");
				break;
			*/
			case PRODUCT_HOME_PREMIUM:
				strcat(name, "Home Premium Edition");
				break;
			case PRODUCT_HOME_BASIC:
				strcat(name, "Home Basic Edition");
				break;
			case PRODUCT_ENTERPRISE:
				strcat(name, "Enterprise Edition");
				break;
			case PRODUCT_BUSINESS:
				strcat(name, "Business Edition");
				break;
			case PRODUCT_STARTER:
				strcat(name, "Starter Edition");
				break;
			case PRODUCT_CLUSTER_SERVER:
				strcat(name, "Cluster Server Edition");
				break;
			case PRODUCT_DATACENTER_SERVER:
				strcat(name, "Datacenter Edition");
				break;
			case PRODUCT_DATACENTER_SERVER_CORE:
				strcat(name, "Datacenter Edition (core installation)");
				break;
			case PRODUCT_ENTERPRISE_SERVER:
				strcat(name, "Enterprise Edition");
				break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:
				strcat(name, "Enterprise Edition (core installation)");
				break;
			case PRODUCT_ENTERPRISE_SERVER_IA64:
				strcat(name, "Enterprise Edition for Itanium-based Systems");
				break;
			case PRODUCT_SMALLBUSINESS_SERVER:
				strcat(name, "Small Business Server");
				break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
				strcat(name, "Small Business Server Premium Edition");
				break;
			case PRODUCT_STANDARD_SERVER:
				strcat(name, "Standard Edition");
				break;
			case PRODUCT_STANDARD_SERVER_CORE:
				strcat(name, "Standard Edition (core installation)");
				break;
			case PRODUCT_WEB_SERVER:
				strcat(name, "Web Server Edition");
				break;
			default:
				strcat(name, "Unknown Edition");
				break;
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
		{
			if( GetSystemMetrics(SM_SERVERR2) ) {
				strcat(name, "Windows Server 2003 R2, ");
			} else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER ) {
				strcat(name, "Windows Storage Server 2003");
			/*} else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER ) {
				strcat(name, "Windows Home Server");
			*/
			} else if ( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) {
				strcat(name, "Windows XP Professional x64 Edition");
			} else {
				strcat(name, "Windows Server 2003, ");
			}

			// Test for the server type.
			if ( osvi.wProductType != VER_NT_WORKSTATION ) {
				if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
						strcat(name, "Datacenter Edition for Itanium-based Systems");
					} else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
						strcat(name, "Enterprise Edition for Itanium-based Systems");
					} else {
						strcat(name, "Standard Edition for Itanium-based Systems");
					}
				} else if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) {
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
						strcat(name, "Datacenter x64 Edition");
					} else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
						strcat(name, "Enterprise x64 Edition");
					} else {
						strcat(name, "Standard x64 Edition");
					}
				}
			} else {
				if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER ) {
					strcat(name, "Compute Cluster Edition");
				} else if ( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
					strcat(name, "Datacenter Edition");
				} else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
					strcat(name, "Enterprise Edition");
				} else if ( osvi.wSuiteMask & VER_SUITE_BLADE ) {
					strcat(name, "Web Edition");
				} else {
					strcat(name, "Standard Edition");
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
			strcat(name, "Windows XP ");
			if( osvi.wSuiteMask & VER_SUITE_PERSONAL ) {
				strcat(name, "Home Edition");
			} else {
				strcat(name, "Professional");
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
			strcat(name, "Windows 2000 ");
			if ( osvi.wProductType == VER_NT_WORKSTATION ) {
				strcat(name, "Professional");
			} else {
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER ) {
					strcat(name, "Datacenter Server");
				} else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE ) {
					strcat(name, "Advanced Server");
				} else {
					strcat(name, "Server");
				}
			}
		}

		// Include service pack (if any) and build number.

		if( strlen(osvi.szCSDVersion) > 0 )
		{
			strcat(name, " " );
			strcat(name, osvi.szCSDVersion);
		}

		sprintf( buf, TEXT(" (build %d)"), osvi.dwBuildNumber);
		strcat(name, buf);

		if ( osvi.dwMajorVersion >= 6 ) {
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 ) {
				strcat(name, ", 64-bit");
			} else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL ) {
				strcat(name, ", 32-bit");
			}
		}
	}
	else
	{  
		switch(osvi.dwPlatformId)
		{
		case VER_PLATFORM_WIN32s:
			strcat(name, "Windows 32s");	
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
				strcat(name, "Windows 95");
				if (osvi.szCSDVersion[0] == 'B' || osvi.szCSDVersion[0] == 'C') {
					strcat(name, " OSR2");
				}
			} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
				strcat(name, "Windows 98");
				if (osvi.szCSDVersion[0] == 'A') {
					strcat(name, " SE");
				}
			} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
				strcat(name, "Windows ME");
			} else {
				strcat(name, "Unknown");
			}
			break;
		case VER_PLATFORM_WIN32_NT:
			if (osvi.dwMajorVersion <=4) {
				strcat(name,  "Windows NT");
			}
		}
	}
	return name;
}



static void JKG_Crash_AddOSData(fileHandle_t f) {
	JKG_FS_WriteString(va("Operating system: %s\n", JKG_GetOSDisplayString()), f);
}

static int GetModuleNamePtr(void* ptr, char *buffFile, char *buffName, void ** ModuleBase, int * ModuleSize) {
	MODULEENTRY32 M = {0};
	HANDLE	hSnapshot;
	
	// BUFFERS MUST BE (AT LEAST) 260 BYTES!!
	if (buffFile) buffFile[0]=0;
	if (buffName) buffName[0]=0;
	if (ModuleBase) *ModuleBase = 0;
	if (ModuleSize) *ModuleSize = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	
	if ((hSnapshot != INVALID_HANDLE_VALUE) && Module32First(hSnapshot, &M)) {
		do {
			if (ptr > (void *)M.modBaseAddr && ptr <= (void *)(M.modBaseAddr+M.modBaseSize)) {
				if (buffFile) strncpy(buffFile, M.szExePath, MAX_PATH);
				if (buffName) strncpy(buffName, M.szModule, 256);
				if (ModuleBase) *ModuleBase = M.modBaseAddr;
				if (ModuleSize) *ModuleSize = M.modBaseSize;
				CloseHandle(hSnapshot);
				return 1;
			}
		} while (Module32Next(hSnapshot, &M));
	}

	CloseHandle(hSnapshot);
	// No matches found
	return 0;
}

static const char *GetExceptionCodeDescription(int ExceptionCode) {
	switch (ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			return " (Access Violation)";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			return " (Array Bounds Exceeded)";
		case EXCEPTION_BREAKPOINT:
			return " (Breakpoint Encountered)";
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return " (Datatype Misallignment)";
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			return " (Denormal Operand (Floating point operation))";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return " (Division By Zero (Floating point operation))";
		case EXCEPTION_FLT_INEXACT_RESULT:
			return " (Inexact Result (Floating point operation))";
		case EXCEPTION_FLT_INVALID_OPERATION:
			return " (Invalid Operation (Floating point operation))";
		case EXCEPTION_FLT_OVERFLOW:
			return " (Overflow (Floating point operation))";
		case EXCEPTION_FLT_STACK_CHECK:
			return " (Stack Overflow/Underflow (Floating point operation))";
		case EXCEPTION_FLT_UNDERFLOW:
			return " (Underflow (Floating point operation))";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return " (Illegal Instruction)";
		case EXCEPTION_IN_PAGE_ERROR:
			return " (In Page Error)";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return " (Division By Zero (Integer operation))";
		case EXCEPTION_INT_OVERFLOW:
			return " (Overflow (Integer operation))";
		case EXCEPTION_INVALID_DISPOSITION:
			return " (Invalid Disposition)";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			return " (Non-Continuable Exception)";
		case EXCEPTION_PRIV_INSTRUCTION:
			return " (Privileged Instruction)";
		case EXCEPTION_SINGLE_STEP:
			return " (Debugger Single Step)";
		case EXCEPTION_STACK_OVERFLOW:
			return " (Stack Overflow)";
		default:
			return "";
	}
}

static void JKG_Crash_AddCrashInfo(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	static char buffFile[MAX_PATH] = {0};
	static char buffName[MAX_PATH] = {0};
	unsigned int ModuleBase;
	PEXCEPTION_RECORD ER = EI->ExceptionRecord;
	GetModuleFileNameA(NULL, buffFile, MAX_PATH);
	JKG_FS_WriteString(va("Process: %s\n", buffFile),f);
	
	ModuleBase = SymGetModuleBase(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress);
	if (ModuleBase) {
		GetModuleBaseName(GetCurrentProcess(), (HMODULE)ModuleBase, buffName, 260);
		JKG_FS_WriteString(va("Exception in module: %s\n", buffName), f);
	} else {
		JKG_FS_WriteString("Exception in module: Unknown\n", f);
	}
	
	/*if (GetModuleNamePtr(ER->ExceptionAddress,buffFile,buffName,(void **)&ModuleBase,NULL)) {
		JKG_FS_WriteString(va("Exception in module: %s\n", buffFile), f);
	} else {
		JKG_FS_WriteString("Exception in module: Unknown\n", f);
	}*/
	JKG_FS_WriteString(va("Exception Address: 0x%08X (%s+0x%X)\n", ER->ExceptionAddress, buffName, (unsigned int)ER->ExceptionAddress - ModuleBase), f);
	JKG_FS_WriteString(va("Exception Code: 0x%08X%s\n", ER->ExceptionCode, GetExceptionCodeDescription(ER->ExceptionCode)), f);
	if (ER->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ER->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) { // Access violation, show read/write address
		switch (ER->ExceptionInformation[0]) {
			case 0:
				JKG_FS_WriteString(va("Attempted to read data at: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			case 1:
				JKG_FS_WriteString(va("Attempted to write data to: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			case 2:
				JKG_FS_WriteString(va("DEP exception caused attempting to execute: 0x%08X\n", ER->ExceptionInformation[1]),f);
				break;
			default:
				break;
		}
	}

}

static void JKG_Crash_AddRegisterDump(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	PCONTEXT CR = EI->ContextRecord;
	//PEXCEPTION_RECORD ER = EI->ExceptionRecord;
	JKG_FS_WriteString("General Purpose & Control Registers:\n", f);
	JKG_FS_WriteString(va("EAX: 0x%08X, EBX: 0x%08X, ECX: 0x%08X, EDX: 0x%08X\n", CR->Eax, CR->Ebx, CR->Ecx, CR->Edx),f);
	JKG_FS_WriteString(va("EDI: 0x%08X, ESI: 0x%08X, ESP: 0x%08X, EBP: 0x%08X\n", CR->Edi, CR->Esi, CR->Esp, CR->Ebp),f);
	JKG_FS_WriteString(va("EIP: 0x%08X\n\n", CR->Eip),f);
	JKG_FS_WriteString("Segment Registers:\n", f);
	JKG_FS_WriteString(va("CS: 0x%08X, DS: 0x%08X, ES: 0x%08X\n", CR->SegCs, CR->SegDs, CR->SegEs), f);
	JKG_FS_WriteString(va("FS: 0x%08X, GS: 0x%08X, SS: 0x%08X\n\n", CR->SegFs, CR->SegGs, CR->SegSs), f);
}

static BOOL CALLBACK JKG_Crash_EnumModules( LPSTR ModuleName, DWORD BaseOfDll, PVOID UserContext ) {
	char Path[MAX_PATH] = {0};
	GetModuleFileName((HMODULE)BaseOfDll, Path, MAX_PATH);
	JKG_FS_WriteString(va("0x%08X - %s - %s\n", BaseOfDll, ModuleName, Path), (fileHandle_t)UserContext);
	return TRUE;
}

static void JKG_Crash_ListModules(fileHandle_t f) {
	SymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)JKG_Crash_EnumModules, (PVOID)f );
}

static void JKG_Crash_DisAsm(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	unsigned long addr;
	int sz;
	int disp;
	ud_t da;
	ud_t dasym;
	int dmod;
	int i;
	int showsource = 0;
	unsigned int lastsourceaddr = 0;
	char modname[260];
	PIMAGEHLP_SYMBOL sym = malloc(1024);
	IMAGEHLP_LINE line;
	MEMORY_BASIC_INFORMATION mem = {0};
	memset(sym, 0, 1024);
	memset(&line, 0, sizeof(IMAGEHLP_LINE));
	sym->MaxNameLength = 800;
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

	if (IsBadReadPtr(EI->ExceptionRecord->ExceptionAddress,16)) {
		JKG_FS_WriteString("ERROR: Exception address invalid, cannot create assembly dump\n\n",f);
		return;
	}
	dmod = SymGetModuleBase(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress);
	if (dmod) {
		GetModuleBaseName(GetCurrentProcess(), (HMODULE)dmod, modname, 260);
	} else {
		strcpy(modname,"Unknown");
	}

	if (SymGetSymFromAddr(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress, (PDWORD)&disp, sym)) {
		// We got a symbol, display info
		JKG_FS_WriteString(va("Crash location located at 0x%08X: %s::%s(+0x%X) [Func at 0x%08X]\n",EI->ExceptionRecord->ExceptionAddress, modname,sym->Name, disp, sym->Address), f);
		// Try to find a source file
		if (SymGetLineFromAddr(GetCurrentProcess(), (DWORD)EI->ExceptionRecord->ExceptionAddress, (PDWORD)&disp, &line)) {
			if (disp) {
				JKG_FS_WriteString(va("Source code: %s:%i(+0x%X)\n\n", line.FileName, line.LineNumber, disp), f);
			} else {
				JKG_FS_WriteString(va("Source code: %s:%i\n\n", line.FileName, line.LineNumber), f);
			}
			showsource = 1;
		} else {
			JKG_FS_WriteString("No source code information available\n\n", f);
			showsource = 0;
		}
	} else {
		// We don't have a symbol..
		JKG_FS_WriteString(va("Crash location located at 0x%08X: No symbol information available\n\n", EI->ExceptionRecord->ExceptionAddress), f);
	}
	VirtualQuery(EI->ExceptionRecord->ExceptionAddress, &mem, sizeof(MEMORY_BASIC_INFORMATION));
	// Do a 21 instruction disasm, 10 back and 10 forward

	addr = DisasmBacktrace((unsigned char *)mem.BaseAddress, (unsigned long)mem.BaseAddress, mem.RegionSize, (unsigned long)EI->ExceptionRecord->ExceptionAddress, 10);

	// Initialize udis
	ud_init(&da);
	ud_set_input_buffer(&da, (uint8_t *)addr, 21*16);
	ud_set_mode(&da, 32);
	ud_set_pc(&da, addr);
	ud_set_syntax(&da, UD_SYN_INTEL);

	// Initialize disassembler for symbol resolving
	ud_init(&dasym);
	ud_set_mode(&dasym, 32);
	ud_set_syntax(&dasym, NULL);

	JKG_FS_WriteString("^^^^^^^^^^\n", f);
	for(i=0; i<21; i++) {
		sz = ud_disassemble(&da);
		addr = ud_insn_off(&da);
		if (sz < 1) {
			JKG_FS_WriteString(va("ERROR: Could not disassemble code at 0x%08X, aborting...\n", addr), f);
			return;
		}
		if (addr == (unsigned long)EI->ExceptionRecord->ExceptionAddress) {
			JKG_FS_WriteString("\n=============================================\n", f);
		}
		// Check if this is a new sourcecode line
		if (showsource) {
			if (SymGetLineFromAddr(GetCurrentProcess(), (DWORD)addr, (PDWORD)&disp, &line)) {
				if (line.Address != lastsourceaddr) {
					lastsourceaddr = line.Address;
					if (disp) {
						JKG_FS_WriteString(va("\n--- %s:%i(+0x%X) ---\n\n", line.FileName, line.LineNumber, disp), f);
					} else {
						JKG_FS_WriteString(va("\n--- %s:%i ---\n\n", line.FileName, line.LineNumber), f);
					}
				}
			}
		}
		
		JKG_FS_WriteString(va("0x%08X - %-30s", (unsigned int)ud_insn_off(&da), ud_insn_asm(&da)), f);
		if ( ((da.mnemonic >= UD_Ija && da.mnemonic <= UD_Ijz ) || da.mnemonic == UD_Icall ) && da.operand[0].type == UD_OP_JIMM) {
			// Its a call or jump, see if we got a symbol for it
			// BUT FIRST ;P
			// Since debug compiles employ a call table, we'll disassemble it first
			// if its a jump, we use that address, otherwise, we'll use this one
			unsigned int addr2 = GetJumpTarget(&da, 0);

			if (addr2 != 0) {
				ud_set_input_buffer(&dasym, (uint8_t *)addr2, 21*16);
				ud_set_pc(&dasym, addr2);

				if (ud_disassemble(&dasym)) {
					if (dasym.mnemonic == UD_Ijmp && da.operand[0].type == UD_OP_JIMM) {
						// Its a call table
						if (SymGetSymFromAddr(GetCurrentProcess(), GetJumpTarget(&dasym, 0), (PDWORD)&disp, sym)) {
							// We got a symbol for it!
							if (disp) {
								JKG_FS_WriteString(va(" (%s+0x%X)", sym->Name, disp), f);
							} else {
								JKG_FS_WriteString(va(" (%s)", sym->Name), f);
							}
						}
					} else {
						// Its not a call table
						if (SymGetSymFromAddr(GetCurrentProcess(), addr2, (PDWORD)&disp, sym)) {
							// We got a symbol for it!
							if (disp) {
								JKG_FS_WriteString(va(" (%s+0x%X)", sym->Name, disp), f);
							} else {
								JKG_FS_WriteString(va(" (%s)", sym->Name), f);
							}
						}
					}
				}
			}
		}
		if (addr == (unsigned long)EI->ExceptionRecord->ExceptionAddress) {
			JKG_FS_WriteString(" <-- Exception\n=============================================\n", f);
		}

		JKG_FS_WriteString("\n", f);
	}
	JKG_FS_WriteString("vvvvvvvvvv\n\n", f);
	free(sym);
	
}

void JKG_Crash_HandleStackFrame(STACKFRAME *sf) {
	//char StackBackup[0x18000];
	if (StackBackupStart) {
		if (sf->AddrFrame.Offset >= StackBackupStart && sf->AddrFrame.Offset <= (StackBackupStart+0x18000)) {
			sf->AddrFrame.Offset = (sf->AddrFrame.Offset - StackBackupStart) + (int)&StackBackup[0];
		}
	}
}

static void JKG_Crash_BackTrace(struct _EXCEPTION_POINTERS *EI, fileHandle_t f) {
	HANDLE proc, thread;
	int frameok;
	PIMAGEHLP_SYMBOL sym = malloc(1024);
	IMAGEHLP_LINE line;
	STACKFRAME sf;
	char ModName[260];
	int dmod;
	int disp;
	int gotsource;
	int sourcedisp;
	CONTEXT ctx = *EI->ContextRecord;	// Copy of the context, since it may be changed
	
	memset(&sf, 0, sizeof(STACKFRAME));
	memset(&line, 0, sizeof(IMAGEHLP_LINE));
	line.SizeOfStruct=sizeof(IMAGEHLP_LINE);
	sf.AddrPC.Offset = EI->ContextRecord->Eip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = EI->ContextRecord->Esp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = EI->ContextRecord->Ebp;
	sf.AddrFrame.Mode = AddrModeFlat;
	memset(sym, 0, 1024);
	sym->MaxNameLength = 800;
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	
	proc = GetCurrentProcess();
	thread = GetCurrentThread();
	if (StackBackupStart) {
		JKG_FS_WriteString("WARNING: Program crashed by a stack overflow, the backtrace will be inconsistent\n", f);
	}
	while(1) {
		frameok = StackWalk(IMAGE_FILE_MACHINE_I386, proc, thread,&sf,&ctx,NULL,SymFunctionTableAccess,SymGetModuleBase,NULL);
		if (!frameok || !sf.AddrFrame.Offset) {
			break;
		}
		dmod = SymGetModuleBase(proc,sf.AddrPC.Offset);
		if (!dmod) {
			strcpy(ModName,"Unknown");
		} else {
			GetModuleBaseName(proc,(HMODULE)dmod, ModName, 260);
		}
		
		if (SymGetLineFromAddr(GetCurrentProcess(), sf.AddrPC.Offset, (PDWORD)&sourcedisp, &line)) {
			gotsource = 1;
		} else {
			gotsource = 0;
		}

		if (SymGetSymFromAddr(proc,sf.AddrPC.Offset, (PDWORD)&disp, sym)) {
			if (gotsource) {
				JKG_FS_WriteString(va("%s::%s(+0x%X) [0x%08X] - (%s:%i)\n", ModName, sym->Name, disp, sf.AddrPC.Offset, line.FileName, line.LineNumber), f);
			} else {
				JKG_FS_WriteString(va("%s::%s(+0x%X) [0x%08X]\n", ModName, sym->Name, disp, sf.AddrPC.Offset), f);
			}
		} else {
			if (gotsource) {
				// Not likely...
				JKG_FS_WriteString(va("%s [0x%08X] - (%s:%i)\n", ModName, sf.AddrPC.Offset, line.FileName, line.LineNumber), f);
			} else {
				JKG_FS_WriteString(va("%s [0x%08X]\n", ModName, sf.AddrPC.Offset), f);
			}
		}
	}
	free(sym);
	JKG_FS_WriteString("\n",f);
}

static void InitSymbolPath( char * SymbolPath, const char* ModPath )
{
	static char Path[1024];

	SymbolPath[0] = 0;	// Clear the buffer
	// Creating the default path
	// ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	strcpy( SymbolPath, "." );

	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
	}

	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariableA( "SYSTEMROOT", Path, 1024 ) )
	{
		strcat( SymbolPath, ";" );
		strcat( SymbolPath, Path );
		strcat( SymbolPath, ";" );

		// SYSTEMROOT\System32
		strcat( SymbolPath, Path );
		strcat( SymbolPath, "\\System32" );
	}

   // Add path of gamedata/JKG
	if ( ModPath != NULL )
		if ( ModPath[0] != '\0' )
		{
			strcat( SymbolPath, ";" );
			strcat( SymbolPath, ModPath );
		}
}
void G_ShutdownGame( int restart );

static LONG WINAPI UnhandledExceptionHandler (struct _EXCEPTION_POINTERS *EI /*ExceptionInfo*/) {
	// Alright, we got an exception here, create a crash log and let the program grind to a halt :P
	static char SymPath[4096];
	static char basepath[260];
	static char fspath[260];
	const char *filename = JKG_Crash_GetCrashlogName();
	fileHandle_t f;

	SymPath[0] = 0;
	basepath[0] = 0;
	fspath[0] = 0;

	bCrashing = 1;
	InitSymbolPath(SymPath, NULL);
	SymInitialize(GetCurrentProcess(), SymPath, TRUE);
	Com_Printf("------------------------------------------------------------\n");
	Com_Printf("Server crashed. Creating crash log %s...\n", filename);

	trap->FS_Open( filename, &f, FS_WRITE );

	JKG_FS_WriteString("========================================\n"
		               "             JA++ Crash Log\n"
					   "========================================\n", f);
#ifdef _GAME
	JKG_FS_WriteString(va("Version: %s (Windows)\n", JAPP_SERVER_VERSION), f);
	JKG_FS_WriteString(va("Side: Server-side\n"), f);
#else
	JKG_FS_WriteString(va("Version: %s (Windows)\n", JAPP_CLIENT_VERSION_CLEAN), f);
	JKG_FS_WriteString(va("Side: Client-side\n"), f);
#endif
	JKG_FS_WriteString(va("Build Date/Time: %s %s\n", __DATE__, __TIME__), f);
	
	JKG_Crash_AddOSData(f);
	JKG_FS_WriteString("Crash type: Exception\n\n"
					   "----------------------------------------\n"
					   "          Exception Information\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddCrashInfo(EI, f);
	JKG_FS_WriteString("\n"
					   "----------------------------------------\n"
					   "              Register Dump\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddRegisterDump(EI, f);
	JKG_FS_WriteString("----------------------------------------\n"
					   "               Module List\n"
					   "----------------------------------------\n", f);
	JKG_Crash_ListModules(f);
	JKG_FS_WriteString("\n----------------------------------------\n"
					   "          Disassembly/Source code\n"
					   "----------------------------------------\n", f);
	JKG_Crash_DisAsm(EI, f);

	JKG_FS_WriteString("----------------------------------------\n"
					   "                Backtrace\n"
					   "----------------------------------------\n", f);
	JKG_Crash_BackTrace(EI, f);
	JKG_FS_WriteString("----------------------------------------\n"
					   "            Extra Information\n"
					   "----------------------------------------\n", f);
	JKG_ExtCrashInfo( (int)f );
	JKG_FS_WriteString("========================================\n"
					   "             End of crash log\n"
					   "========================================\n", f);
	trap->FS_Close( f );
	SymCleanup( GetCurrentProcess() );
	Com_Printf("Crash report finished, attempting to shut down...\n");
	JP_ForceQuit();	// This will shutdown the engine as well
	// Generally speaking, we'll never get here, as JP_ForceQuit will terminate the process
	return 1;
}

static LONG WINAPI UnhandledExceptionHandler_Failsafe(struct _EXCEPTION_POINTERS *EI) {
	if (EI->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
		// Alright, we got a VERY serious issue here..
		// In this state the exception handler itself will run outta stack too
		// So we'll just use a nice hack here to roll up esp by 16k
		__asm
		{
			mov eax, EI
			mov StackBackupStart, esp
			mov esi, esp
			mov edi, offset StackBackup
			mov ecx, 0x6000
			rep stosd
			add esp, 0x18000
			push eax
			call UnhandledExceptionHandler
			jmp skip
		}
	}
	StackBackupStart=0;
	return UnhandledExceptionHandler(EI);
skip:
	;
}


void ActivateCrashHandler( void ) {
	oldHandler = SetUnhandledExceptionFilter(UnhandledExceptionHandler_Failsafe);
}

void DeactivateCrashHandler( void ) {
	if (!oldHandler) return;
	SetUnhandledExceptionFilter(oldHandler);
}
#else

int oldactsset = 0;
struct sigaction oldact[NSIG];

extern char *strsignal(int __sig) __THROW;

void (*OldHandler)(int signal, siginfo_t *siginfo, ucontext_t *ctx);

static int m_crashloop=0;

// Defined a linked
#define PERM_READ 1
#define PERM_WRITE 2
#define PERM_EXEC 4

typedef struct memblock_s {
	unsigned int start;
	unsigned int end;
	char permissions;
	char name[256];
	struct memblock_s *next;
} memblock_t;

static memblock_t *memblocks = NULL;

static int JKG_SafeMemAddress(unsigned int address) {
	memblock_t *mb;
	if (!memblocks) {
		return 0;
	}
	for (mb = memblocks; mb; mb = mb->next) {
		if (mb->end >= address && mb->start <= address) {
			return mb->permissions;
		}
	}
	return 0;
}

static const char * JKG_GetMemRegion(unsigned int address) {
	memblock_t *mb;
	if (!memblocks) {
		return "Unknown - No memory map info available";
	}
	for (mb = memblocks; mb; mb = mb->next) {
		if (mb->end >= address && mb->start <= address) {
			return mb->name;
		}
	}
	return "Unknown";
}

static void JKG_Free_MemoryMap() {
	memblock_t *block, *next;
	block = memblocks;
	while (block) {
		next = block->next;
		free((void *)block);
		block = next;
	}
	memblocks = NULL;
}

static void JKG_Enum_MemoryMap() {
	char buffer[1024];
	const char *line;

	int skipspaces;

	memblock_t *mb;

	if (memblocks) {
		JKG_Free_MemoryMap();
	}
	FILE *f;
	f = fopen("/proc/self/maps", "r");
	if (!f) {
		return;
	}
	mb = malloc(sizeof(memblock_t));
	while ((line = fgets(buffer, 1024, f)) != 0) {
		if (strlen(line) < 5) {
			// Sanity check, if its less than this, the line probably aint good :P
			continue;
		}
		buffer[strlen(line)-1] = 0; // Remove \n

		if (sscanf(line, "%08X-%08X", &mb->start, &mb->end) != 2) {
			continue; // Couldn't read all values
		}
		line+=18; // Skip over the addresses and get to the permissions
		mb->permissions = 0;
		if (*line == 'r') {
			mb->permissions |= PERM_READ;
		}
		line++;
		if (*line == 'w') {
			mb->permissions |= PERM_WRITE;
		}
		line++;
		if (*line == 'x') {
			mb->permissions |= PERM_EXEC;
		}
		line+=3;	// Skip to the next space

		skipspaces = 3;
		while (*line) {
			if (*line == 32) {
				skipspaces--;
			} else {
				if (skipspaces <= 0) {
					break;
				}
			}
			line++;
		}
		strncpy(mb->name, line, 256);
		// If we get here, the parsing went properly, so lets put it in the list
		mb->next = memblocks;
		memblocks = mb;
		mb = malloc(sizeof(memblock_t));
	}
	free(mb);
	fclose(f);
}

static void JKG_Crash_AddOSData(fileHandle_t f) {
	struct utsname un;
	uname(&un);
	JKG_FS_WriteString(va("Operating system: %s %s %s %s %s %s\n",un.sysname, un.nodename, un.release, un.version, un.machine, un.domainname), f);
}
static void JKG_Crash_AddCrashInfo(int signal, siginfo_t *siginfo, ucontext_t *ctx, fileHandle_t f) {
	JKG_FS_WriteString(va("Exception code: %s (%i)\n", strsignal(signal), signal), f);
	JKG_FS_WriteString(va("Exception address: %p\n", ctx->uc_mcontext.gregs[REG_EIP]), f);
	JKG_FS_WriteString(va("Exception in module: %s\n", JKG_GetMemRegion(ctx->uc_mcontext.gregs[REG_EIP])), f);
	switch (signal) {
		case SIGSEGV:
			switch (siginfo->si_code) {
				case SEGV_MAPERR:
					JKG_FS_WriteString("Exception cause: Address not mapped to object\n", f);
					break;
				case SEGV_ACCERR:
					JKG_FS_WriteString("Exception cause: Invalid permissions for mapped object\n", f);
					break;
				default:
					JKG_FS_WriteString("Exception cause: Unknown\n", f);
					break;
			}
			break;
		case SIGILL:
			switch (siginfo->si_code) {
				case ILL_ILLOPC:
					JKG_FS_WriteString("Exception cause: Illegal opcode\n", f);
					break;
				case ILL_ILLOPN:
					JKG_FS_WriteString("Exception cause: Illegal operand\n", f);
					break;
				case ILL_ILLADR:
					JKG_FS_WriteString("Exception cause: Illegal addressing mode\n", f);
					break;
				case ILL_ILLTRP:
					JKG_FS_WriteString("Exception cause: Illegal trap\n", f);
					break;
				case ILL_PRVOPC:
					JKG_FS_WriteString("Exception cause: Privileged opcode\n", f);
					break;
				case ILL_PRVREG:
					JKG_FS_WriteString("Exception cause: Privileged register\n", f);
					break;
				case ILL_COPROC:
					JKG_FS_WriteString("Exception cause: Coprocessor error\n", f);
					break;
				case ILL_BADSTK:
					JKG_FS_WriteString("Exception cause: Internal stack error\n", f);
					break;
				default:
					JKG_FS_WriteString("Exception cause: Unknown\n", f);
					break;	
			}
			break;

	}
	if (siginfo && signal == SIGSEGV) {
		JKG_FS_WriteString(va("Attempted to reference memory address: %p\n", siginfo->si_addr), f);
	}
}

static void JKG_Crash_AddRegisterDump(ucontext_t *ctx, fileHandle_t f) {
	JKG_FS_WriteString("General Purpose & Control Registers:\n", f);
	JKG_FS_WriteString(va("EAX: 0x%08X, EBX: 0x%08X, ECX: 0x%08X, EDX: 0x%08X\n", ctx->uc_mcontext.gregs[REG_EAX], ctx->uc_mcontext.gregs[REG_EBX], ctx->uc_mcontext.gregs[REG_ECX], ctx->uc_mcontext.gregs[REG_EDX]),f);
	JKG_FS_WriteString(va("EDI: 0x%08X, ESI: 0x%08X, ESP: 0x%08X, EBP: 0x%08X\n", ctx->uc_mcontext.gregs[REG_EDI], ctx->uc_mcontext.gregs[REG_ESI], ctx->uc_mcontext.gregs[REG_ESP], ctx->uc_mcontext.gregs[REG_EBP]),f);
	JKG_FS_WriteString(va("EIP: 0x%08X\n\n",  ctx->uc_mcontext.gregs[REG_EIP]),f);
	JKG_FS_WriteString("Segment Registers:\n", f);
	JKG_FS_WriteString(va("CS: 0x%08X, DS: 0x%08X, ES: 0x%08X\n", ctx->uc_mcontext.gregs[REG_CS], ctx->uc_mcontext.gregs[REG_DS], ctx->uc_mcontext.gregs[REG_ES]), f);
	JKG_FS_WriteString(va("FS: 0x%08X, GS: 0x%08X, SS: 0x%08X\n\n", ctx->uc_mcontext.gregs[REG_FS], ctx->uc_mcontext.gregs[REG_GS], ctx->uc_mcontext.gregs[REG_SS]), f);
}

static void JKG_Crash_ListModules(fileHandle_t f) {
	struct link_map *linkmap = NULL;
	ElfW(Ehdr) * ehdr =(ElfW(Ehdr) *)0x8048000;
	ElfW(Phdr) * phdr;
	ElfW(Dyn) * dyn;
	struct r_debug *rdebug = NULL;
	phdr = (ElfW(Phdr) *)((char *)ehdr + ehdr->e_phoff);

	for ( ; phdr < (ElfW(Phdr) *)((char *)phdr + (ehdr->e_phnum * sizeof(ElfW(Phdr)))); phdr++ ) {
		if (phdr->p_type == PT_DYNAMIC)
			break;
	}

	for (dyn = (ElfW(Dyn) *)phdr->p_vaddr; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag == DT_DEBUG) {
			rdebug = (void *)dyn->d_un.d_ptr;
			break;
		}
	}

	if (!rdebug) {
		JKG_FS_WriteString("Could not locate link map\n",f);
		return;
	}

	linkmap = rdebug->r_map;
	// Rewind to top
	while(linkmap->l_prev)
		linkmap = linkmap->l_prev;

	while (linkmap) {
		if (linkmap->l_addr) {
			if (linkmap->l_name && linkmap->l_name[0]) {
				JKG_FS_WriteString(va("%08X - %s\n", linkmap->l_addr, linkmap->l_name),f);
			} else {
				JKG_FS_WriteString(va("%08X - (Unknown)\n", linkmap->l_addr),f);
			}
		}
		linkmap = linkmap->l_next;
	}
}

static void JKG_Crash_DisAsm(ucontext_t *ctx, fileHandle_t f) {
	unsigned int addr;
	unsigned int eaddr = ctx->uc_mcontext.gregs[REG_EIP];
	int sz;
	int disp;
	ud_t da;
	ud_t dasym;
	memblock_t *mb;
	int i;
	Dl_info info;
	int dladdrok;

	if (!(JKG_SafeMemAddress(eaddr) & PERM_READ)) {
		JKG_FS_WriteString("ERROR: Exception address invalid, cannot create assembly dump\n\n", f);
		return;
	}
	dladdrok = dladdr((void *)eaddr, &info);
	if (dladdrok && info.dli_saddr) {
		disp = eaddr - (unsigned int)info.dli_saddr;
		JKG_FS_WriteString(va("Crash location located at 0x%08X: %s::%s(+0x%X) [Func at 0x%08X]\n", eaddr, info.dli_fname, info.dli_sname, disp, info.dli_saddr), f);
	} else {
		JKG_FS_WriteString(va("Crash location located at 0x%08X: No symbol information available\n\n", eaddr), f);
	}

	// If memblocks is NULL, we wouldn't have gotten here, so its safe to assume it is not
	for (mb = memblocks; mb; mb = mb->next) {
		if (mb->end >= eaddr && mb->start <= eaddr) {
			break; // Got it
		}
	}

	// Do a 21 instruction disasm, 10 back and 10 forward

	addr = DisasmBacktrace((unsigned char *)mb->start, (unsigned long)mb->start, (unsigned long)mb->end - mb->start, (unsigned long)eaddr, 10);

	// Initialize udis
	ud_init(&da);
	ud_set_input_buffer(&da, (uint8_t *)addr, 21*16);
	ud_set_mode(&da, 32);
	ud_set_pc(&da, addr);
	ud_set_syntax(&da, UD_SYN_INTEL);

	// Initialize disassembler for symbol resolving
	ud_init(&dasym);
	ud_set_mode(&dasym, 32);
	ud_set_syntax(&dasym, NULL);

	JKG_FS_WriteString("^^^^^^^^^^\n", f);
	for(i=0; i<21; i++) {
		sz = ud_disassemble(&da);
		addr = ud_insn_off(&da);

		if (sz < 1) {
			JKG_FS_WriteString(va("ERROR: Could not disassemble code at 0x%08X, aborting...\n", addr), f);
			return;
		}
		if (addr == eaddr) {
			JKG_FS_WriteString("\n=============================================\n", f);
		}

		JKG_FS_WriteString(va("0x%08X - %-30s", (unsigned int)ud_insn_off(&da), ud_insn_asm(&da)), f);
		if ( ((da.mnemonic >= UD_Ija && da.mnemonic <= UD_Ijz ) || da.mnemonic == UD_Icall ) && da.operand[0].type == UD_OP_JIMM) {
			// Its a call or jump, see if we got a symbol for it
			// BUT FIRST ;P
			// Since debug compiles employ a call table, we'll disassemble it first
			// if its a jump, we use that address, otherwise, we'll use this one


			unsigned int addr2 = GetJumpTarget(&da, 0);

			if (addr2 != 0) {
				ud_set_input_buffer(&dasym, (uint8_t *)addr2, 21*16);
				ud_set_pc(&dasym, addr2);

				if (ud_disassemble(&dasym)) {
					if (dasym.mnemonic == UD_Ijmp && ( da.operand[0].type == UD_OP_JIMM || da.operand[0].type == UD_OP_IMM )) {
						unsigned int addy;

						if (!(addy = GetJumpTarget(&dasym, 0))) {
							// PLT redirect
							addy = dasym.operand[0].lval.udword + ctx->uc_mcontext.gregs[REG_EBX];
							if ((JKG_SafeMemAddress(addy) & PERM_READ)) {
								addy = *(unsigned int*)addy;
							}
						}
						// Its a call table
						dladdrok = dladdr((void *)addy, &info);
						if (dladdrok && info.dli_saddr) {
							// We got a symbol for it!
							disp = addy - (unsigned int)info.dli_saddr;
							if (disp) {
								JKG_FS_WriteString(va(" (%s+0x%X)", info.dli_sname, disp), f);
							} else {
								JKG_FS_WriteString(va(" (%s)", info.dli_sname), f);
							}
						} else {
							// Its a call table still, do display the real address anyway
							JKG_FS_WriteString(va(" (%08X)", addy), f);
						}
					} else {
						// Its not a call table
						dladdrok = dladdr((void*)addr2, &info);
						if (dladdrok && info.dli_saddr) {
							// We got a symbol for it!
							disp = GetJumpTarget(&dasym, 0) - (unsigned int)info.dli_saddr;
							if (disp) {
								JKG_FS_WriteString(va(" (%s+0x%X)", info.dli_sname, disp), f);
							} else {
								JKG_FS_WriteString(va(" (%s)", info.dli_sname), f);
							}
						}
					}

				}
			}
		}
		if (addr == (int)eaddr) {
			JKG_FS_WriteString(" <-- Exception\n=============================================\n", f);
		}

		JKG_FS_WriteString("\n", f);
	}
	JKG_FS_WriteString("vvvvvvvvvv\n\n", f);
}

static void JKG_Crash_BackTrace(ucontext_t *ctx, fileHandle_t f) {
	char **strings;
	void *array[1024];
	size_t size;
	int i;

	size = backtrace(array,1024);
	JKG_FS_WriteString(va("Stack frames found: %i\n", size), f);
	array[1] = (void*)ctx->uc_mcontext.gregs[REG_EIP];
	strings = (char **)backtrace_symbols(array, size);
	for (i=1; i<size; i++) {
		JKG_FS_WriteString(va("%i) %s\n", i, strings[i]), f);
	}
	free(strings);
	JKG_FS_WriteString("\n",f);
}

static void CrashHandler(int signal, siginfo_t *siginfo, ucontext_t *ctx) {
	// Very basic atm, will be expanded soon
	const char *filename = JKG_Crash_GetCrashlogName();
	fileHandle_t f;

	m_crashloop++;
	if (m_crashloop>2) {
		Com_Printf("Critical error: Recursive crashing detected, terminating...\n");
		exit(1);
	}

	bCrashing = 1;
	Com_Printf("------------------------------------------------------------\n");
#ifdef _GAME
	Com_Printf("Server crashed. Creating crash log %s...\n", filename);
#else
	Com_Printf("Client crashed. Creating crash log %s...\n", filename);
#endif
	trap->FS_Open( filename, &f, FS_WRITE );
	JKG_FS_WriteString("========================================\n"
		               "             JA++ Crash Log\n"
					   "========================================\n", f);
#ifdef _GAME
	JKG_FS_WriteString(va("Version: %s (Linux)\n", JAPP_SERVER_VERSION), f);
	JKG_FS_WriteString(va("Side: Server-side\n"), f);
#else
	JKG_FS_WriteString(va("Version: %s (Linux)\n", JAPP_CLIENT_VERSION_CLEAN), f);
	JKG_FS_WriteString(va("Side: Client-side\n"), f);
#endif
	JKG_FS_WriteString(va("Build Date/Time: %s %s\n", __DATE__, __TIME__), f);
	JKG_Crash_AddOSData(f);
	JKG_Enum_MemoryMap();
	JKG_FS_WriteString("Crash type: Exception\n\n"
					   "----------------------------------------\n"
					   "          Exception Information\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddCrashInfo(signal, siginfo, ctx, f);
	JKG_FS_WriteString("\n"
					   "----------------------------------------\n"
					   "              Register Dump\n"
					   "----------------------------------------\n", f);
	JKG_Crash_AddRegisterDump(ctx, f);
	JKG_FS_WriteString("----------------------------------------\n"
					   "               Module List\n"
					   "----------------------------------------\n", f);
	JKG_Crash_ListModules(f);
	JKG_FS_WriteString("\n----------------------------------------\n"
					   "          Disassembly/Source code\n"
					   "----------------------------------------\n", f);
	JKG_Crash_DisAsm(ctx, f);

	JKG_FS_WriteString("----------------------------------------\n"
					   "                Backtrace\n"
					   "----------------------------------------\n", f);
	JKG_Crash_BackTrace(ctx, f);
	JKG_FS_WriteString("----------------------------------------\n"
					   "            Extra Information\n"
					   "----------------------------------------\n", f);
	JKG_ExtCrashInfo( (int)f );
	JKG_FS_WriteString("========================================\n"
					   "             End of crash log\n"
					   "========================================\n", f);

	trap->FS_Close( f );
	JKG_Free_MemoryMap();
	Com_Printf("Crash report finished, attempting to shut down...\n");
	if (m_crashloop < 2) {	// If we crashed here before, skip the quit call
		JP_ForceQuit();	// This will shutdown the engine as well
	}
	// We'll never get here, but just in case, forward the call to the old crash handler if we DO get here
	OldHandler = (void *)oldact[signal].sa_sigaction;
	(*OldHandler)(signal, siginfo, ctx);
}

static __sighandler_t CTRLCHandler(int signal, struct sigcontext ctx) {
	// Display a message saying not to use Ctrl-C
	// Begin the message with \r to overwrite the ^C that might show up
	Com_Printf("\rPlease don't close the server using Ctrl-C, use quit instead\n");
	return 0;
}

void ActivateCrashHandler( void ) {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	memset(&oldact, 0, sizeof(oldact));
	act.sa_sigaction = (void *)CrashHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &act, &oldact[SIGSEGV]);
	sigaction(SIGILL, &act, &oldact[SIGILL]);
	sigaction(SIGFPE, &act, &oldact[SIGFPE]);
	sigaction(SIGBUS, &act, &oldact[SIGBUS]);

	oldactsset = 1;
	signal(SIGINT, (void *)CTRLCHandler);
}

void DeactivateCrashHandler( void ) {
	if (!oldactsset) {
		return;
	}
	sigaction(SIGSEGV, &oldact[SIGSEGV], NULL);
	sigaction(SIGILL, &oldact[SIGILL], NULL);
	sigaction(SIGFPE, &oldact[SIGFPE], NULL);
	sigaction(SIGBUS, &oldact[SIGBUS], NULL);
}

#endif

#endif //QARCH == 32
