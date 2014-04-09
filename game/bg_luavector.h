#pragma once

#ifdef JPLUA

void JPLua_Vector_CreateRef( lua_State *L, float x, float y, float z );
int JPLua_GetVector3( lua_State *L );
vector3 *JPLua_CheckVector( lua_State *L, int idx );
void JPLua_Register_Vector( lua_State *L );

#endif // JPLUA
