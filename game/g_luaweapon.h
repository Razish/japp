#pragma once

qboolean JPLua_Weapon_CallFunction(gentity_t *ent, weapon_t type, qboolean altFire);
#ifdef JPLUA
int JPLua_Weapon_SetFireFunction(lua_State *L);
int JPLua_Weapon_SetAltFireFunction(lua_State *L);
#endif