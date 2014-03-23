#if defined(_GAME)
#include "g_local.h"
#elif defined(_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"
#include "json/cJSON.h"
#include "bg_luaserialiser.h"

#ifdef JPLUA

static const char SERIALISER_META[] = "Serialiser.meta";

#if defined(_GAME)
static const char *pluginDir = "lua/sv/";
#elif defined(_CGAME)
static const char *pluginDir = "lua/cl/";
#endif

//Func: GetSerialiser( string fileName )
//Retn: Serialiser object
int JPLua_GetSerialiser( lua_State *L ) {
	const char *path = NULL;
	fsMode_t mode = FS_READ;

	//TODO: force reading/writing to plugin directory?
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TSTRING, 1, "'string' expected" );
	path = lua_tostring( L, 1 );

	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TNUMBER, 2, "'integer' expected" );
	mode = (fsMode_t)lua_tointeger( L, 2 );

	JPLua_Serialiser_CreateRef( L, path, mode );
	return 1;
}

//Func: tostring( Serialiser )
//Retn: string representing the Serialiser instance (for debug/error messages)
static int JPLua_Serialiser_ToString( lua_State *L ) {
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );
	lua_pushfstring( L, "Serialiser(%s)", serialiser->fileName );
	return 1;
}

void JPLua_Serialiser_IterateTableWrite( cJSON *parent, const char *name, lua_State *L ) {
	cJSON *table = cJSON_CreateArray();
	int tableIndex = lua_gettop( L );

	lua_pushnil( L );
	// for each element
	while ( lua_next( L, -2 ) != 0 ) {
		cJSON *item = cJSON_CreateObject();
		int keyType = lua_type( L, -2 );
		int valueType = lua_type( L, -1 );

		cJSON_AddIntegerToObject( item, "key_type", keyType );
		if ( keyType == LUA_TSTRING ) {
			cJSON_AddStringToObject( item, "key", lua_tostring( L, -2 ) );
		}
		else if ( keyType == LUA_TNUMBER ) {
			cJSON_AddIntegerToObject( item, "key", lua_tointeger( L, -2 ) );
		}
		else {
			Com_Printf( "Can not serialise key in table %s: invalid type %s\n", name, lua_typename( L, keyType ) );
		}

		cJSON_AddIntegerToObject( item, "value_type", valueType );
		if ( valueType == LUA_TTABLE ) {
			if ( lua_rawequal( L, -1, tableIndex ) ) {
				Com_Printf( "Can not serialise key <FIXME> in table %s: self-references are fatal!\n", name );
				cJSON_Delete( item );
				lua_pop( L, 1 );
				continue;
			}
			lua_pushvalue( L, -1 );
			JPLua_Serialiser_IterateTableWrite( item, "value", L );
		}
		else if ( valueType == LUA_TNUMBER ) {
			cJSON_AddNumberToObject( item, "value", lua_tonumber( L, -1 ) );
		}
		else if ( valueType == LUA_TBOOLEAN ) {
			cJSON_AddIntegerToObject( item, "value", !!lua_toboolean( L, -1 ) );
		}
		else if ( valueType == LUA_TSTRING ) {
			cJSON_AddStringToObject( item, "value", lua_tostring( L, -1 ) );
		}
		else {
			Com_Printf( "Can not serialise value in table %s: invalid type %s\n", name, lua_typename( L, valueType ) );
			cJSON_Delete( item );
			lua_pop( L, 1 );
			continue;
		}

		cJSON_AddItemToArray( table, item );
		lua_pop( L, 1 );
	}
	lua_pop( L, 1 );
	cJSON_AddItemToObject( parent, name, table );
}

void JPLua_Serialiser_IterateTableRead( cJSON *parent, const char *name, lua_State *L ) {
	cJSON *t = NULL;
	int numElements, i, top;

	lua_newtable( L );
	top = lua_gettop( L );

	t = cJSON_GetObjectItem( parent, name );
	numElements = cJSON_GetArraySize( t );

	for ( i = 0; i < numElements; i++ ) {
		cJSON *e, *it;
		int kType, vType;
		const char *tmp;
		char k[256];

		e = cJSON_GetArrayItem( t, i );

		// key
		it = cJSON_GetObjectItem( e, "key" );
		kType = cJSON_ToInteger( cJSON_GetObjectItem( e, "key_type" ) );
		if ( (tmp = cJSON_ToString( it )) )
			Q_strncpyz( k, tmp, sizeof(k) );

		if ( kType == LUA_TSTRING ) {
			lua_pushstring( L, k );
		}
		else if ( kType == LUA_TNUMBER ) {
			lua_pushnumber( L, cJSON_ToNumber( it ) );
		}
		else {
			Com_Printf( "Invalid key type %s when reading table %s\n", lua_typename( L, kType ), name );
		}

		// value must be created based on type.
		it = cJSON_GetObjectItem( e, "value" );
		vType = cJSON_ToInteger( cJSON_GetObjectItem( e, "value_type" ) );
		if ( vType == LUA_TTABLE ) {
			JPLua_Serialiser_IterateTableRead( e, "value", L );
		}
		else if ( vType == LUA_TNUMBER ) {
			lua_pushnumber( L, cJSON_ToNumber( it ) );
		}
		else if ( vType == LUA_TBOOLEAN ) {
			lua_pushboolean( L, cJSON_ToBoolean( it ) );
		}
		else if ( vType == LUA_TSTRING ) {
			char v[1024 * 8]; // should be plenty..
			if ( (tmp = cJSON_ToString( it )) )
				Q_strncpyz( v, tmp, sizeof(v) );
			lua_pushstring( L, v );
		}

		lua_settable( L, top );
	}
}

int JPLua_Serialiser_Close( lua_State *L );

//Func: AddTable( string name, table )
//Retn: --
int JPLua_Serialiser_AddTable( lua_State *L ) {
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );

	if ( lua_type( L, 2 ) != LUA_TSTRING ) {
		JPLua_Serialiser_Close( L );
		luaL_argcheck( L, 1, 2, "'string' expected" );
	}
	if ( lua_type( L, 3 ) != LUA_TTABLE ) {
		JPLua_Serialiser_Close( L );
		luaL_argcheck( L, 1, 3, "'table' expected" );
	}

	if ( !serialiser->write ) {
		Com_Printf( "Serialiser is not available to write table\n" );
		return 0;
	}

	JPLua_Serialiser_IterateTableWrite( serialiser->outRoot, lua_tostring( L, 2 ), L );

	return 0;
}

//Func: GetTable( string name )
//Retn: table
int JPLua_Serialiser_GetTable( lua_State *L ) {
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );

	if ( lua_type( L, 2 ) != LUA_TSTRING ) {
		JPLua_Serialiser_Close( L );
		luaL_argcheck( L, 1, 2, "'string' expected" );
	}

	if ( !serialiser->read ) {
		Com_Printf( "Serialiser is not available to read table\n" );
		return 0;
	}

	JPLua_Serialiser_IterateTableRead( serialiser->inRoot, lua_tostring( L, 2 ), L );

	return 1;
}

//Func: Serialiser:Close()
//Retn: --
int JPLua_Serialiser_Close( lua_State *L ) {
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );

	if ( serialiser->write ) {
		const char *buffer = cJSON_Serialize( serialiser->outRoot, 1 );

		trap->FS_Write( buffer, strlen( buffer ), serialiser->fileHandle );
		free( (void *)buffer );

		cJSON_Delete( serialiser->outRoot );
		serialiser->outRoot = NULL;
		serialiser->write = qfalse;
	}

	if ( serialiser->read ) {
		serialiser->inRoot = NULL;
		serialiser->read = qfalse;
	}

	trap->FS_Close( serialiser->fileHandle );
	serialiser->fileHandle = 0;
	serialiser->fileName[0] = '\0';

	return 0;
}

// Push a Serialiser instance for a client number onto the stack
void JPLua_Serialiser_CreateRef( lua_State *L, const char *path, fsMode_t mode ) {
	jplua_serialiser_t *serialiser = NULL;
	int len = 0;

	serialiser = (jplua_serialiser_t *)lua_newuserdata( L, sizeof(jplua_serialiser_t) );
	Com_sprintf( serialiser->fileName, sizeof(serialiser->fileName), "%s%s/%s", pluginDir, JPLua.currentPlugin->name, path );
	len = trap->FS_Open( serialiser->fileName, &serialiser->fileHandle, mode );

	if ( mode == FS_WRITE ) {
		serialiser->write = qtrue;
		serialiser->read = qfalse;
		serialiser->outRoot = cJSON_CreateObject();
	}
	else if ( mode == FS_READ ) {
		serialiser->read = qtrue;
		serialiser->write = qfalse;
		if ( len > 0 ) {
			char *contents = (char *)malloc( len );

			trap->FS_Read( contents, len, serialiser->fileHandle );
			serialiser->inRoot = cJSON_Parse( contents );
			if ( !serialiser->inRoot )
				Com_Printf( "Couldn't parse serialised JSON data %s\n", path );

			free( contents );
			contents = NULL;
		}
		else
			serialiser->inRoot = NULL;
	}

	luaL_getmetatable( L, SERIALISER_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Serialiser instance,
// Return the instance if it is, otherwise return NULL.
jplua_serialiser_t *JPLua_CheckSerialiser( lua_State *L, int idx ) {
	void *ud = luaL_checkudata( L, idx, SERIALISER_META );
	luaL_argcheck( L, ud != NULL, 1, "'Serialiser' expected" );
	return (jplua_serialiser_t *)ud;
}

static const struct luaL_Reg jplua_serialiser_meta[] = {
	{ "__tostring", JPLua_Serialiser_ToString },
	{ "AddTable", JPLua_Serialiser_AddTable },
	{ "ReadTable", JPLua_Serialiser_GetTable },
	{ "Close", JPLua_Serialiser_Close },
	{ NULL, NULL }
};

// Register the Serialiser class for Lua
void JPLua_Register_Serialiser( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, SERIALISER_META ); // Create metatable for Serialiser class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	// fill metatable with fields
	for ( r = jplua_serialiser_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Serialiser class metatable from the stack
}

#endif // JPLUA
