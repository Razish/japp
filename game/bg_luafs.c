#if defined(_GAME)
#include "g_local.h"
#elif defined(_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA
static const char FILE_META[] = "File.meta";

jplua_file_t *JPLua_CheckFile(lua_State *L, int idx){
	void *ud = luaL_checkudata( L, idx, FILE_META );
	luaL_argcheck( L, ud != NULL, 1, "'File' expected" );
	return (jplua_file_t *)ud;
}


void JPLua_File_CreateRef( lua_State *L, const char *path, fsMode_t mode ){
	jplua_file_t *file = NULL;
	int len = 0;
	file = (jplua_file_t*)lua_newuserdata( L, sizeof(jplua_file_t) );
	Q_strncpyz(file->name, path, sizeof(file->name));
	file->mode = mode;
	file->lenght = trap->FS_Open( file->name, &file->handle, file->mode );

	if ( !file->handle || (file->lenght <= 0 && file->mode == 0) ) {
		trap->Print( "JPLua_File_Open: Failed to load %s, file doesn't exist\n", file->name );
	}

	luaL_getmetatable( L, FILE_META );
	lua_setmetatable( L, -2 );
}

int JPLua_File_Open( lua_State *L){
	const char *path = luaL_checkstring(L,1);
	int mode = luaL_checkinteger(L,2);
	if (mode < 0 || mode > 3){
		lua_pushnil(L);
		return 1;
	}


	JPLua_File_CreateRef(L, path,mode);
	return 1;
}

static int JPLua_File_Read( lua_State *L){
	jplua_file_t *file = JPLua_CheckFile(L,1);
	int len = luaL_checkinteger(L,2);
	if (len == -1){
		len = file->lenght;
	}
	if (!file->handle){
		lua_pushnil(L);
		return 1;
	}
	char *buffer = (char*)malloc(len);
	memset((void *)buffer, 0, len);

	trap->FS_Read(buffer, len, file->handle);
	lua_pushlstring(L, buffer, len);
	return 1;
}


static int JPLua_File_Write( lua_State *L){
	jplua_file_t *file = JPLua_CheckFile(L,1);
	const char *data = luaL_checkstring(L,2);
	int len = strlen(data);
	if (len <= 0 || !file->handle){
		return 0;
	}
	trap->FS_Write((void *)data, len, file->handle);
	return 0;
}

int JPLua_File_GetFileList( lua_State *L){
	static char list[16384];
	char *folderName = list;
	int num, i, top,folderLen = 0;
	const char *path = luaL_checkstring(L,1);
	const char *ext = luaL_checkstring(L,2);
	size_t skipLenFolder = 0;

    num = trap->FS_GetFileList(path, ext, list, sizeof(list));

	lua_newtable( L );
	top = lua_gettop( L );

	for ( i = 0; i < num; i++ ) {
		qboolean skip = qfalse;
		char *s;
		if ( folderName[0] == '.' ){
			skip = qtrue;
		}
		if ( (s = (char *)Q_strchrs( folderName, "/\\" )) ) {
			if ( !s[1] )
				skip = qtrue;
			*s = '\0';
			skipLenFolder = strlen( ++s ) + 1;
		}
		folderLen = strlen( folderName ) + 1;
		if ( !skip ) {
			lua_pushinteger( L, i ); lua_pushstring( L, folderName ); lua_settable( L, top );
		}
		folderName += folderLen + skipLenFolder;
	}

	return 1;
}

static int JPLua_File_Close(lua_State *L){
	jplua_file_t *file = JPLua_CheckFile(L,1);
	if (!file->handle){
		return 0;
	}
	trap->FS_Close(file->handle);
	return 0;
}

static int JPLua_File_Lenght(lua_State *L){
	jplua_file_t *file = JPLua_CheckFile(L,1);
	if (!file->handle){
		return 0;
	}
	lua_pushinteger(L, file->lenght);
	return 1;
}

static const struct luaL_Reg jplua_file_meta[] = {
	{"Read", JPLua_File_Read},
	{"Write", JPLua_File_Write },
	{"Close", JPLua_File_Close},
	{"Lenght", JPLua_File_Lenght},
	{NULL, NULL},
};


void JPLua_Register_File(lua_State *L){
	const luaL_Reg *r;

	luaL_newmetatable( L, FILE_META ); // Create metatable for File class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2); // Re-push metatable to top of stack
	lua_settable(L, -3); // metatable.__index = metatable

	// fill metatable with fields
	for (r = jplua_file_meta; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	lua_pop(L, -1); // Pop the File class metatable from the stack
}
#endif