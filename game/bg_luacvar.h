#pragma once

#ifdef JPLUA

// Cvar instance userdata type
typedef struct jplua_cvar_s {
	char name[MAX_CVAR_VALUE_STRING];
} jplua_cvar_t;

void JPLua_Cvar_CreateRef( lua_State *L, const char *name );

int JPLua_CreateCvar( lua_State *L );
int JPLua_GetCvar( lua_State *L );

jplua_cvar_t *JPLua_CheckCvar( lua_State *L, int idx );
void JPLua_Register_Cvar( lua_State *L );

#endif // JPLUA
