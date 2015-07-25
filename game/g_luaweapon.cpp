#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_lua.h"

std::unordered_map<weapon_t, lua_weapon> weapon_func_list;
qboolean JPLua_Weapon_CallFunction(gentity_t *ent,weapon_t type, qboolean altFire){
#ifdef JPLUA
	lua_weapon *handle = &weapon_func_list[type];
	if (altFire){
		if (handle->altFire != 0 || handle->altFire != -1) {
			lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, weapon_func_list[type].altFire);
			JPLua_Entity_CreateRef(JPLua.state, ent);
			JPLua_Call(JPLua.state, 1, 0);
			return qtrue;
		}
	}
	if (handle->fire != 0 || handle->fire != -1) {
		lua_rawgeti(JPLua.state, LUA_REGISTRYINDEX, weapon_func_list[type].fire);
		JPLua_Entity_CreateRef(JPLua.state, ent);
		JPLua_Call(JPLua.state, 1, 0);
		return qtrue;
	}
#endif
	return qfalse;
}
#ifdef JPLUA
int JPLua_Weapon_SetFireFunction(lua_State *L){
	weapon_t type = (weapon_t)luaL_checkinteger(L, 1);
	lua_weapon *handle = &weapon_func_list[type];
	if (lua_type(L, 2) != LUA_TFUNCTION){
		return 0;
	}
	if (handle != nullptr) {
		handle->fire = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	else{
		lua_weapon data;
		data.fire = luaL_ref(L, LUA_REGISTRYINDEX);
		weapon_func_list[type] = data;
	}
	return 0;
}

int JPLua_Weapon_SetAltFireFunction(lua_State *L){
	weapon_t type = (weapon_t)luaL_checkinteger(L, 1);
	lua_weapon *handle = &weapon_func_list[type];
	if (lua_type(L, 2) != LUA_TFUNCTION){
		return 0;
	}
	if (handle != nullptr) {
		handle->altFire = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	else{
		lua_weapon data;
		data.altFire = luaL_ref(L, LUA_REGISTRYINDEX);
		weapon_func_list[type] = data;
	}
	return 0;
}

#endif