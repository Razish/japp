#include "cg_local.h"
#include "cg_lua.h"
#include "shared/json/cJSON.h"
#include "cg_luaserialiser.h"

#ifdef JPLUA

static const char SERIALISER_META[] = "Serialiser.meta";

//Func: GetSerialiser(fileName)
//Retn: Serialiser object
int JPLua_GetSerialiser( lua_State *L )
{
	const char *path = NULL;
	fsMode_t mode = FS_READ;

	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TSTRING, 1, "'string' expected" );
	path = lua_tostring( L, 1 );

	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TNUMBER, 2, "'integer' expected" );
	mode = (fsMode_t)lua_tointeger( L, 2 );

	JPLua_Serialiser_CreateRef( L, path, mode );
	return 1;
}

//Func: tostring(Serialiser)
//Retn: string representing the Player instance (for debug/error messages)
static int JPLua_Serialiser_ToString( lua_State *L )
{
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );
	lua_pushfstring( L, "Serialiser(%s)", serialiser->fileName );
	return 1;
}

/*
{
	"myShit" : [{
			"key" : "position",
			"type" : 5,
			"value" : [{
					"key" : "x"
					"type" 3
					"value" : 1500
				}, {
					"key" : "y"
					"type" 3
					"value" : 2000
				}, {
					"key" : "z"
					"type" 3
					"value" : 256
				}],
		}, {
			"key" : "name",
			"type" : 4,
			"value" : "Raz0r"
		}, {
			"key" : "model",
			"type" : 4,
			"value" : "imperial/commander"
		}],
	"myShit2" : [{
			"key" : "health"
			"type" : 3
			"value" : 100
		}]
}
*/

void JPLua_Serialiser_IterateTableWrite( cJSON *parent, const char *name, lua_State *L )
{
	cJSON *table = cJSON_CreateArray();
	int tableIndex = lua_gettop( L );

	lua_pushnil( L );
	while ( lua_next( L, -2 ) != 0 )
	{//for each element
		cJSON *item = cJSON_CreateObject();
		int keyType = lua_type(L, -2);
		int valueType = lua_type(L, -1);
		
		cJSON_AddIntegerToObject( item, "key_type", keyType );
		if ( keyType == LUA_TSTRING )
			cJSON_AddStringToObject( item, "key", lua_tostring( L, -2 ) );
		else if ( keyType == LUA_TNUMBER )
			cJSON_AddIntegerToObject( item, "key", lua_tointeger( L, -2 ) );

		cJSON_AddIntegerToObject( item, "value_type", valueType );
		if ( valueType == LUA_TTABLE )
		{
			if ( lua_rawequal( L, -1, tableIndex ) )
			{
				Com_Printf( "Can not serialise key <FIXME> in table %s: self-references are fatal!\n", name );
				cJSON_Delete( item );
				lua_pop( L, 1 );
				continue;
			}
			lua_pushvalue( L, -1 );
			JPLua_Serialiser_IterateTableWrite( item, "value", /*lua_tostring( L, -2 ),*/ L );
		}
		else if ( valueType == LUA_TNUMBER )
			cJSON_AddNumberToObject( item, "value", lua_tonumber( L, -1 ) );
		else if ( valueType == LUA_TSTRING )
			cJSON_AddStringToObject( item, "value", lua_tostring( L, -1 ) );
		else
		{
			Com_Printf( "Can not serialise key <FIXME> in table %s: invalid value type\n", name );
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

void JPLua_Serialiser_IterateTableRead( cJSON *parent, const char *name, lua_State *L )
{
	cJSON *table = cJSON_CreateArray();
	int tableIndex = lua_gettop( L );

	lua_pushnil( L );
	while ( lua_next( L, -2 ) != 0 )
	{//for each element
		cJSON *item = cJSON_CreateObject();
		int keyType = lua_type(L, -2);
		int valueType = lua_type(L, -1);
		
		cJSON_AddIntegerToObject( item, "key_type", keyType );
		if ( keyType == LUA_TSTRING )
			cJSON_AddStringToObject( item, "key", lua_tostring( L, -2 ) );
		else if ( keyType == LUA_TNUMBER )
			cJSON_AddIntegerToObject( item, "key", lua_tointeger( L, -2 ) );

		cJSON_AddIntegerToObject( item, "value_type", valueType );
		if ( valueType == LUA_TTABLE )
		{
			if ( lua_rawequal( L, -1, tableIndex ) )
			{
				Com_Printf( "Can not serialise key <FIXME> in table %s: self-references are fatal!\n", name );
				cJSON_Delete( item );
				lua_pop( L, 1 );
				continue;
			}
			lua_pushvalue( L, -1 );
			JPLua_Serialiser_IterateTableWrite( item, "value", /*lua_tostring( L, -2 ),*/ L );
		}
		else if ( valueType == LUA_TNUMBER )
			cJSON_AddNumberToObject( item, "value", lua_tonumber( L, -1 ) );
		else if ( valueType == LUA_TSTRING )
			cJSON_AddStringToObject( item, "value", lua_tostring( L, -1 ) );
		else
		{
			Com_Printf( "Can not serialise key <FIXME> in table %s: invalid value type\n", name );
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

//Func: AddTable( string, table )
//Retn: --
int JPLua_Serialiser_AddTable( lua_State *L )
{
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );
	const char *tableName = NULL;
	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TSTRING, 2, "'string' expected" );
	luaL_argcheck( L, lua_type( L, 3 ) == LUA_TTABLE, 3, "'table' expected" );
	
	tableName = lua_tostring( L, 2 );
	JPLua_Serialiser_IterateTableWrite( serialiser->outRoot, tableName, L );

	return 0;
}

//Func: GetTable('string')
//Retn: table
int JPLua_Serialiser_GetTable( lua_State *L )
{
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );
	const char *tableName = NULL;
	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TSTRING, 2, "'string' expected" );
	
	tableName = lua_tostring( L, 2 );
	JPLua_Serialiser_IterateTableRead( serialiser->inRoot, tableName, L );

	return 0;
}

//Func: Serialiser:Close()
//Retn: --
int JPLua_Serialiser_Close( lua_State *L )
{
	jplua_serialiser_t *serialiser = JPLua_CheckSerialiser( L, 1 );
	const char *buffer = cJSON_Serialize( serialiser->outRoot, 1 );

	trap->FS_Write( buffer, strlen( buffer ), serialiser->fileHandle );
	trap->FS_Close( serialiser->fileHandle );
	serialiser->fileHandle = 0;

	free( (void *)buffer );
	cJSON_Delete( serialiser->outRoot );
	serialiser->fileName[0] = '\0';

	return 0;
}

// Push a Serialiser instance for a client number onto the stack
void JPLua_Serialiser_CreateRef( lua_State *L, const char *path, fsMode_t mode )
{
	jplua_serialiser_t *serialiser = NULL;
	int len = 0;

	serialiser = (jplua_serialiser_t *)lua_newuserdata( L, sizeof( jplua_serialiser_t ) );
	Q_strncpyz( serialiser->fileName, path, sizeof( serialiser->fileName ) );
	len = trap->FS_Open( path, &serialiser->fileHandle, mode );
	if ( len > 0 )
	{
		char *contents = (char*)malloc( len );

		trap->FS_Read( contents, len, serialiser->fileHandle );
		serialiser->outRoot = cJSON_CreateObject();
		serialiser->inRoot = cJSON_Parse( contents );
		if ( !serialiser->inRoot )
		{
			Com_Printf( "Couldn't parse serialised JSON data %s\n", path );
		}

		free( contents );
		contents = NULL;
	}

	luaL_getmetatable( L, SERIALISER_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Serialiser instance,
// Return the instance if it is, otherwise return NULL.
jplua_serialiser_t *JPLua_CheckSerialiser( lua_State *L, int idx )
{
	void *ud = luaL_checkudata( L, idx, SERIALISER_META );
	luaL_argcheck( L, ud != NULL, 1, "'Serialiser' expected" );
	return (jplua_serialiser_t *)ud;
}

static const struct luaL_Reg jplua_serialiser_meta[] = {
	{ "__tostring",			JPLua_Serialiser_ToString },
	{ "AddTable",			JPLua_Serialiser_AddTable },
	{ "ReadTable",			JPLua_Serialiser_GetTable },
	{ "Close",				JPLua_Serialiser_Close },
	{ NULL, NULL }
};

// Register the Serialiser class for Lua
void JPLua_Register_Serialiser( lua_State *L )
{
	luaL_newmetatable( L, SERIALISER_META ); // Create metatable for Serialiser class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	luaL_register( L, NULL, jplua_serialiser_meta ); // Fill metatable with fields
	lua_pop( L, -1 ); // Pop the Serialiser class metatable from the stack
}

#endif // JPLUA
