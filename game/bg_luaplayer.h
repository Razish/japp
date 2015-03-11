#pragma once

#ifdef JPLUA

// Player instance userdata type
struct jplua_player_t {
	int clientNum;
};

void JPLua_Player_CreateRef( lua_State *L, int num );
int JPLua_GetPlayer( lua_State *L );
jplua_player_t *JPLua_CheckPlayer( lua_State *L, int idx );
int JPLua_Player_GetMetaTable( lua_State *L );
void JPLua_Register_Player( lua_State *L );

#endif // JPLUA
