#pragma once

namespace JPLua {

#ifdef JPLUA

struct luaWeapon_t {
    int fire, altFire; // lua registry handles
};

#ifdef JPLUA_INTERNALS
int Weapon_SetFireFunction(lua_State *L);
int Weapon_SetAltFireFunction(lua_State *L);
#endif // JPLUA_INTERNALS

#endif // JPLUA

qboolean Weapon_CallFunction(gentity_t *ent, weapon_t type, qboolean altFire);

} // namespace JPLua
