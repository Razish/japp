#include "q_shared.h"
#include "q_engine.h"

// ==================================================
// UnlockMemory (WIN32 & Linux compatible)
// --------------------------------------------------
// Makes the memory at address writable for at least size bytes.
// Returns 1 if successful, returns 0 on failure.
// ==================================================
qboolean UnlockMemory( int address, int size )
{
	#ifdef _WIN32

		DWORD dummy;
		return ( VirtualProtect( (LPVOID)address, size, PAGE_EXECUTE_READWRITE, &dummy ) != 0 );

	#else // Linux is a bit more tricky

		int ret, page1, page2;
		page1 = address & ~( getpagesize() - 1);
		page2 = (address+size) & ~( getpagesize() - 1);

		if ( page1 == page2 )
			return (mprotect( (char *)page1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC ) == 0);
		else
		{
			ret = mprotect( (char *)page1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC );
			if ( ret ) return 0;
			return (mprotect( (char *)page2, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC ) == 0);
		}

	#endif
}

// ==================================================
// LockMemory (WIN32 & Linux compatible)
// --------------------------------------------------
// Makes the memory at address read-only for at least size bytes.
// Returns 1 if successful, returns 0 on failure.
// ==================================================
qboolean LockMemory( int address, int size )
{
	#ifdef _WIN32

		DWORD dummy;
		return ( VirtualProtect( (LPVOID)address, size, PAGE_EXECUTE_READ, &dummy ) != 0 );

	#else // Linux is a bit more tricky

		int ret, page1, page2;
		page1 = address & ~( getpagesize() - 1);
		page2 = (address+size) & ~( getpagesize() - 1);

		if( page1 == page2 )
			return (mprotect( (char *)page1, getpagesize(), PROT_READ | PROT_EXEC ) == 0);
		else
		{
			ret = mprotect( (char *)page1, getpagesize(), PROT_READ | PROT_EXEC );
			if ( ret ) return 0;
			return (mprotect( (char *)page2, getpagesize(), PROT_READ | PROT_EXEC ) == 0);
		}

	#endif
}

#ifndef OPENJK
void PlaceHook( hookEntry_t *hook )
{
	qboolean success = qfalse;

	if ( hook && (success = UnlockMemory( hook->hookPosition, 5 )) )
	{
		unsigned int forward = (unsigned int)((void*(*)())hook->hookForward)(); //i never want to see this line again
		if ( !memcmp( (const void *)&hook->origBytes[0], (const void *)hook->hookPosition, sizeof( hook->origBytes ) ) )
		{
			//	memcpy( &hook->origBytes[0], (void *)hook->hookPosition, 5 );
			*(unsigned char *)(hook->hookPosition) = hook->hookOpcode;
			*(unsigned int *)(hook->hookPosition+1) = (forward) - ((unsigned int)(hook->hookPosition)+5);
			success = LockMemory( hook->hookPosition, 5 );
		}
		else
			success = qfalse;
	}
	#ifdef _DEBUG
		if ( hook )
			Com_Printf( success ? va( "  %s\n", hook->name )
								: va( "^1Warning: Failed to place hook: %s\n", hook->name ) );
	#endif
}

void RemoveHook( const hookEntry_t *hook )
{
	qboolean success = qfalse;

	if ( hook && (success = UnlockMemory( hook->hookPosition, 5 )) )
	{
		memcpy( (void *)hook->hookPosition, hook->origBytes, 5 );
		success = LockMemory( hook->hookPosition, 5 );
	}
	#ifdef _DEBUG
		if ( hook )
			Com_Printf( success ? va( "  %s\n", hook->name )
								: va( "^1Warning: Failed to remove hook: %s\n", hook->name ) );
	#endif
}
#endif // !OPENJK
