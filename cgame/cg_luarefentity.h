#pragma once
#if defined(PROJECT_CGAME) || defined(JPLUA)
namespace JPLua {
typedef struct jplua_refentity_s {
    refEntity_t *p;
} jplua_refentity_t;

#ifdef JPLUA_INTERNALS
void Register_RefEntity(lua_State *L);
void RefEntity_CreateRef(lua_State *L, refEntity_t *ent);
int RefEntity_Create(lua_State *L);
refEntity_t *CheckRefEntity(lua_State *L, int idx);
#endif
} // namespace JPLua
#endif