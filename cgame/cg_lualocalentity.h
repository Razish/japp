#pragma once

#if defined(PROJECT_CGAME) || defined(JPLUA)
namespace JPLua{
	typedef struct jplua_localentity_s {
		int id;
	} jplua_localentity_t;
#ifdef JPLUA_INTERNALS
	void Register_LocalEntity(lua_State *L);
	void LocalEntity_CreateRef(lua_State *L, localEntity_t *ent);
	int LocalEntity_Create(lua_State *L);
	localEntity_t *CheckLocalEntity(lua_State *L, int idx);
#endif
}
#endif