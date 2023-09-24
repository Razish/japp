#pragma once

#ifdef JPLUA

namespace JPLua {

// Cvar instance userdata type
struct luaCvar_t {
    char name[MAX_CVAR_VALUE_STRING];
};

// public
void Cvar_Update(const char *name);

#ifdef JPLUA_INTERNALS
void Cvar_CreateRef(lua_State *L, const char *name);
int CreateCvar(lua_State *L);
int GetCvar(lua_State *L);
luaCvar_t *CheckCvar(lua_State *L, int idx);
void Register_Cvar(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
