#pragma once

#ifdef JPLUA

namespace JPLua {

	// Player instance userdata type
	struct luaPlayer_t {
		int clientNum;
	};

#ifdef JPLUA_INTERNALS
	void Player_CreateRef( lua_State *L, int num );
	int GetPlayer( lua_State *L );
	luaPlayer_t *CheckPlayer( lua_State *L, int idx );
	int Player_GetMetaTable( lua_State *L );
	void Register_Player( lua_State *L );
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
