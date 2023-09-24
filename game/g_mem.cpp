#include "g_local.h"

static char memoryPool[256 * 1024];
static int allocPoint;

void *G_Alloc(int size) {
    char *p = NULL;

    if (g_debugAlloc.integer) {
        trap->Print("G_Alloc of %i bytes (%i left)\n", size, (int)(sizeof(memoryPool) - allocPoint - ((size + 31) & ~31)));
    }

    if (allocPoint + size > sizeof(memoryPool)) {
        trap->Error(ERR_DROP, "G_Alloc: failed on allocation of %i bytes\n", size);
        return NULL;
    }

    p = &memoryPool[allocPoint];

    allocPoint += (size + 31) & ~31;

    return p;
}

void G_InitMemory(void) { allocPoint = 0; }

void G_ShowGameMem(void) { trap->Print("Game memory status: %i out of %i bytes allocated\n", allocPoint, (int)sizeof(memoryPool)); }
