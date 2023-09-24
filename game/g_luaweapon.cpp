#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#endif

#include "bg_luainternal.h"

#ifdef JPLUA
#include <unordered_map>
#endif

namespace JPLua {

#ifdef JPLUA
std::unordered_map<int, luaWeapon_t> weaponCallbacks;
#endif // JPLUA

qboolean Weapon_CallFunction(gentity_t *ent, weapon_t type, qboolean altFire) {
#ifdef JPLUA
    luaWeapon_t *handle = &weaponCallbacks[type];
    if (altFire) {
        if (handle->altFire && handle->altFire != -1) {
            lua_rawgeti(ls.L, LUA_REGISTRYINDEX, handle->altFire);
            Entity_CreateRef(ls.L, ent);
            lua_pushinteger(ls.L, ent->client->ps.weaponChargeTime);
            Call(ls.L, 2, 0);
            return qtrue;
        }
    }
    if (handle->fire && handle->fire != -1) {
        lua_rawgeti(ls.L, LUA_REGISTRYINDEX, handle->fire);
        Entity_CreateRef(ls.L, ent);
        lua_pushinteger(ls.L, ent->client->ps.weaponChargeTime);
        Call(ls.L, 2, 0);
        return qtrue;
    }
#endif
    return qfalse;
}

#ifdef JPLUA
int Weapon_SetFireFunction(lua_State *L) {
    weapon_t type = (weapon_t)luaL_checkinteger(L, 1);
    luaWeapon_t *handle = &weaponCallbacks[type];
    if (lua_type(L, 2) != LUA_TFUNCTION) {
        return 0;
    }

    if (handle) {
        handle->fire = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        luaWeapon_t data;
        data.fire = luaL_ref(L, LUA_REGISTRYINDEX);
        weaponCallbacks[type] = data;
    }
    return 0;
}

int Weapon_SetAltFireFunction(lua_State *L) {
    weapon_t type = (weapon_t)luaL_checkinteger(L, 1);
    luaWeapon_t *handle = &weaponCallbacks[type];
    if (lua_type(L, 2) != LUA_TFUNCTION) {
        return 0;
    }
    if (handle) {
        handle->altFire = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        luaWeapon_t data;
        data.altFire = luaL_ref(L, LUA_REGISTRYINDEX);
        weaponCallbacks[type] = data;
    }
    return 0;
}
#endif

} // namespace JPLua
