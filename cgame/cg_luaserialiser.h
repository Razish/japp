#pragma once

#ifdef JPLUA

// Player instance userdata type
typedef struct jplua_serialiser_s {
	fileHandle_t fileHandle;
	char fileName[MAX_QPATH];
	cJSON *inRoot;
	cJSON *outRoot;
} jplua_serialiser_t;

void JPLua_Serialiser_CreateRef( lua_State *L, const char *path, fsMode_t mode );
int JPLua_GetSerialiser( lua_State *L );
jplua_serialiser_t *JPLua_CheckSerialiser( lua_State *L, int idx );
void JPLua_Register_Serialiser( lua_State *L );

#endif // JPLUA
