#pragma once

#if defined(PROJECT_GAME)
#include "g_local.h"
typedef gentity_t jpluaEntity_t;
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
typedef centity_t jpluaEntity_t;
#endif

#ifdef JPLUA
typedef struct jplua_entity_s {
	int id;
}jplua_entity_t;
void JPLua_Register_Entity(lua_State *L);
void JPLua_Entity_CreateRef(lua_State *L, jpluaEntity_t *ent);
int JPLua_Entity_Get(lua_State *L);

#ifdef PROJECT_CGAME
refEntity_t *JPLua_CheckRefEntity(lua_State *L, int idx);
int JPLua_Entity_CreateRefEntity(lua_State *L);
#elif defined PROJECT_GAME
int JPLua_Entity_Create(lua_State *L);
void JPLua_Entity_CallFunction(gentity_t *ent, int id, void *arg1 = NULL, void *arg2 = NULL, void *arg3 = NULL, void *arg4 = NULL);
#endif
jpluaEntity_t *JPLua_CheckEntity(lua_State *L, int idx);

typedef enum jplua_entity_func {
	JPLUA_ENTITY_THINK = 0,

	JPLUA_ENTITY_REACHED,
	JPLUA_ENTITY_BLOCKED,
	JPLUA_ENTITY_TOUCH,
	JPLUA_ENTITY_USE,
	JPLUA_ENTITY_PAIN,
	JPLUA_ENTITY_DIE,
	JPLUA_ENTITY_MAX
} jplua_entity_func;
#endif