#pragma once

#ifdef JPLUA

#if defined(PROJECT_GAME)
#include "game/g_local.h"
#elif defined(PROJECT_CGAME)
#include "cgame/cg_local.h"
#endif

namespace JPLua {

#if defined(PROJECT_GAME)
typedef gentity_t jpluaEntity_t;
#elif defined(PROJECT_CGAME)
typedef centity_t jpluaEntity_t;
#endif

#if defined(PROJECT_GAME)
enum entityFunc_e {
    JPLUA_ENTITY_THINK = 0,

    JPLUA_ENTITY_REACHED,
    JPLUA_ENTITY_BLOCKED,
    JPLUA_ENTITY_TOUCH,
    JPLUA_ENTITY_USE,
    JPLUA_ENTITY_PAIN,
    JPLUA_ENTITY_DIE,
    JPLUA_ENTITY_MAX
};

void Entity_CallFunction(gentity_t *ent, entityFunc_e funcID, intptr_t arg1 = 0, intptr_t arg2 = 0, intptr_t arg3 = 0, intptr_t arg4 = 0);

#ifdef JPLUA_INTERNALS
int Entity_Create(lua_State *L);
#endif // JPLUA_INTERNALS

#endif // PROJECT_GAME

struct luaEntity_t {
    int id;
};

#ifdef JPLUA_INTERNALS
struct entityProperty_t {
    const char *name;
    int (*Get)(lua_State *L, jpluaEntity_t *ent);
    void (*Set)(lua_State *L, jpluaEntity_t *ent);
};
int EntityPropertyCompare(const void *a, const void *b);

int Entity_GetMetaTable(lua_State *L);
void Register_Entity(lua_State *L);
void Entity_CreateRef(lua_State *L, jpluaEntity_t *ent);
int Entity_Get(lua_State *L);
#if defined(PROJECT_GAME)
int FindEntityByClassName(lua_State *L);
#endif
jpluaEntity_t *CheckEntity(lua_State *L, int idx);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
