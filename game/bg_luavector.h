#pragma once

#ifdef JPLUA

namespace JPLua {

#ifdef JPLUA_INTERNALS
void Vector_CreateRef(lua_State *L, float x, float y, float z);
void Vector_CreateRef(lua_State *L, const vector3 *vec);
int GetVector3(lua_State *L);
vector3 *CheckVector(lua_State *L, int idx);
void Register_Vector(lua_State *L);
#endif

} // namespace JPLua

#endif // JPLUA
