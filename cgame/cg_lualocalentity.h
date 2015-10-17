#pragma once

#ifdef JPLUA

namespace JPLua {

	struct luaLocalEntity_t {
		int id;
	};

#ifdef JPLUA_INTERNALS
	void Register_LocalEntity( lua_State *L );
	void LocalEntity_CreateRef( lua_State *L, localEntity_t *ent );
	int LocalEntity_Create( lua_State *L );
	localEntity_t *CheckLocalEntity( lua_State *L, int idx );
#endif // JPLUA_INTERNALS

}

#endif // JPLUA
