#pragma once

#if defined(PROJECT_GAME)
	#include "g_local.h"
	typedef gentity_t jpluaEntity_t;
#elif defined(PROJECT_CGAME)
	#include "cg_local.h"
	typedef centity_t jpluaEntity_t;
#endif
#if defined PROJECT_GAME
	int JPLua_Entity_Create(lua_State *L);
	typedef enum jplua_entityFunc_e {
		JPLUA_ENTITY_THINK = 0,

		JPLUA_ENTITY_REACHED,
		JPLUA_ENTITY_BLOCKED,
		JPLUA_ENTITY_TOUCH,
		JPLUA_ENTITY_USE,
		JPLUA_ENTITY_PAIN,
		JPLUA_ENTITY_DIE,
		JPLUA_ENTITY_MAX
	} jplua_entityFunc_t;

	void JPLua_Entity_CallFunction(
		gentity_t *ent,
		jplua_entityFunc_t funcID,
		intptr_t arg1 = 0,
		intptr_t arg2 = 0,
		intptr_t arg3 = 0,
		intptr_t arg4 = 0
		);
#endif

#ifdef JPLUA
typedef struct jplua_entity_s {
	int id;
} jplua_entity_t;

void JPLua_Register_Entity( lua_State *L );
void JPLua_Entity_CreateRef( lua_State *L, jpluaEntity_t *ent );
int JPLua_Entity_Get( lua_State *L );
int JPLua_FindEntityByClassName(lua_State *L);

jpluaEntity_t *JPLua_CheckEntity( lua_State *L, int idx );

#endif // JPLUA
