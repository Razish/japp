#pragma once

#ifdef JPLUA
typedef struct jplua_file_s {
	char name[MAX_QPATH];
	fileHandle_t handle;
	fsMode_t mode;
	int length;
} jplua_file_t;

void JPLua_File_CreateRef( lua_State *L, const char *path, fsMode_t mode );
int JPLua_File_Open( lua_State *L);
int JPLua_File_GetFileList( lua_State *L);
jplua_file_t *JPLua_CheckFile(lua_State *L, int idx);
void JPLua_Register_File(lua_State *L);

#endif
