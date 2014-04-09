// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// g_mem.c
//


#include "g_local.h"


#define POOLSIZE	(256 * 1024)

static char		memoryPool[POOLSIZE];
static int		allocPoint;

void *G_Alloc( int size ) {
	char	*p;

	if ( g_debugAlloc.integer ) {
		trap->Print( "G_Alloc of %i bytes (%i left)\n", size, POOLSIZE - allocPoint - ((size + 31) & ~31) );
	}

	if ( allocPoint + size > POOLSIZE ) {
		trap->Error( ERR_DROP, "G_Alloc: failed on allocation of %i bytes\n", size ); // bk010103 - was %u, but is signed
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += (size + 31) & ~31;

	return p;
}

void G_InitMemory( void ) {
	allocPoint = 0;
}

void G_ShowGameMem( void ) {
	trap->Print( "Game memory status: %i out of %i bytes allocated\n", allocPoint, POOLSIZE );
}
