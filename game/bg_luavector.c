#if defined(_GAME)
	#include "g_local.h"
#elif defined(_CGAME)
	#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

static const char VECTOR_META[] = "Vector.meta";

//Func: Vector3( [float x, float y, float z] )
//Retn: Vector object
int JPLua_GetVector3( lua_State *L ) {
	float x = 0.0f, y = 0.0f, z = 0.0f;

	if ( lua_type( L, 1 ) == LUA_TNUMBER && lua_type( L, 2 ) == LUA_TNUMBER && lua_type( L, 3 ) == LUA_TNUMBER ) {
		x = lua_tonumber( L, 1 );
		y = lua_tonumber( L, 2 );
		z = lua_tonumber( L, 3 );
	}

	JPLua_Vector_CreateRef( L, x, y, z );
	return 1;
}

//Func: Vector.__index
//Retn: requested component
static int JPLua_Vector_Index( lua_State *L ) {
	vector3 *v = JPLua_CheckVector( L, 1 );
	const char *key = lua_tostring( L, 2 );

	// see if this key is a function/constant in the metatable
	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );
	if ( !lua_isnil( L, -1 ) )
		return 1;

	// assume it's a component
		 if ( !strcmp( key, "x" ) || !strcmp( key, "r" ) || !strcmp( key, "pitch" ) )
		lua_pushnumber( L, v->x );
	else if ( !strcmp( key, "y" ) || !strcmp( key, "g" ) || !strcmp( key, "yaw" ) )
		lua_pushnumber( L, v->y );
	else if ( !strcmp( key, "z" ) || !strcmp( key, "b" ) || !strcmp( key, "roll" ) )
		lua_pushnumber( L, v->z );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Vector.__newindex = f
//Retn: N/A
static int JPLua_Vector_NewIndex( lua_State *L ) {
	vector3 *v = JPLua_CheckVector( L, 1 );
	const char *key = lua_tostring( L, 2 );

		 if ( !strcmp( key, "x" ) || !strcmp( key, "r" ) || !strcmp( key, "pitch" ) )
		v->x = luaL_checknumber( L, 3 );
	else if ( !strcmp( key, "y" ) || !strcmp( key, "g" ) || !strcmp( key, "yaw" ) )
		v->y = luaL_checknumber( L, 3 );
	else if ( !strcmp( key, "z" ) || !strcmp( key, "b" ) || !strcmp( key, "roll" ) )
		v->z = luaL_checknumber( L, 3 );

	return 0;
}

//Func: v1 + v2
//Retn: Vector v3
static int JPLua_Vector_Add( lua_State *L ) {
	vector3 *v1 = JPLua_CheckVector( L, 1 ),
			*v2 = JPLua_CheckVector( L, 2 ),
			 v3;

	VectorAdd( v1, v2, &v3 );

	JPLua_Vector_CreateRef( L, v3.x, v3.y, v3.z );

	return 1;
}

//Func: Vector1 == Vector2
//Retn: boolean value of whether Vector1 is the same as Vector2
static int JPLua_Vector_Equals( lua_State *L ) {
	vector3 *v1 = JPLua_CheckVector( L, 1 ), *v2 = JPLua_CheckVector( L, 2 );

	lua_pushboolean( L, VectorCompare( v1, v2 ) ? 1 : 0 );

	return 1;
}

//Func: v1 / f
//Retn: Vector v2
static int JPLua_Vector_Divide( lua_State *L ) {
	vector3 *v1 = JPLua_CheckVector( L, 1 ), v2;
	float f = luaL_checknumber( L, 2 );

	VectorScale( v1, 1.0f/f, &v2 );

	JPLua_Vector_CreateRef( L, v2.x, v2.y, v2.z );

	return 1;
}

//Func: v1 * f
//Retn: Vector v2
static int JPLua_Vector_Multiply( lua_State *L ) {
	vector3 *v1 = JPLua_CheckVector( L, 1 ), v2;
	float f = luaL_checknumber( L, 2 );

	VectorScale( v1, f, &v2 );

	JPLua_Vector_CreateRef( L, v2.x, v2.y, v2.z );

	return 1;
}

//Func: v1 - v2
//Retn: Vector v3
static int JPLua_Vector_Subtract( lua_State *L ) {
	vector3 *v1 = JPLua_CheckVector( L, 1 ),
			*v2 = JPLua_CheckVector( L, 2 ),
			 v3;

	VectorSubtract( v1, v2, &v3 );

	JPLua_Vector_CreateRef( L, v3.x, v3.y, v3.z );

	return 1;
}

//Func: tostring( Vector )
//Retn: string representing the Vector instance (for debug/error messages)
static int JPLua_Vector_ToString( lua_State *L ) {
	vector3 *v = JPLua_CheckVector( L, 1 );
	char str[64];

	Com_sprintf( str, sizeof( str ), "%.3f %.3f %.3f", v->x, v->y, v->z );
	lua_pushfstring( L, "Vector3( %s )", str );

	return 1;
}

// Push a Vector instance onto the stack
void JPLua_Vector_CreateRef( lua_State *L, float x, float y, float z ) {
	vector3 *v = NULL;

	v = (vector3 *)lua_newuserdata( L, sizeof( *v ) );
	VectorSet( v, x, y, z );

	luaL_getmetatable( L, VECTOR_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Vector instance,
// Return the instance if it is, otherwise return NULL.
vector3 *JPLua_CheckVector( lua_State *L, int idx ) {
	void *ud = luaL_checkudata( L, idx, VECTOR_META );

	luaL_argcheck( L, ud != NULL, 1, "'Vector3' expected" );

	return (vector3 *)ud;
}

static const struct luaL_Reg jplua_vector_meta[] = {
	{ "__index",	JPLua_Vector_Index },
	{ "__newindex",	JPLua_Vector_NewIndex },
	{ "__add",		JPLua_Vector_Add },
	{ "__eq",		JPLua_Vector_Equals },
	{ "__div",		JPLua_Vector_Divide },
	{ "__mul",		JPLua_Vector_Multiply },
	{ "__sub",		JPLua_Vector_Subtract },
	{ "__tostring",	JPLua_Vector_ToString },
	{ NULL,			NULL }
};

// Register the Vector class for Lua
void JPLua_Register_Vector( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, VECTOR_META ); // Create metatable for Vector class, push on stack

	for ( r=jplua_vector_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Vector class metatable from the stack
}

#endif // JPLUA
