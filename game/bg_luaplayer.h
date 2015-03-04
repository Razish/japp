#pragma once

#ifdef JPLUA

// Player instance userdata type 
typedef struct jplua_player_s {
	int clientNum;
} jplua_player_t;

typedef struct jplua_player_func_s {
	const char *name;
	lua_CFunction getfunc;
	lua_CFunction setfunc;
}jplua_player_func_t;

void JPLua_Player_CreateRef( lua_State *L, int num );
int JPLua_GetPlayer( lua_State *L );
jplua_player_t *JPLua_CheckPlayer( lua_State *L, int idx );
int JPLua_Player_GetMetaTable( lua_State *L );
void JPLua_Register_Player( lua_State *L );

#endif // JPLUA
