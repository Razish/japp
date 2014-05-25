#pragma once

#ifdef JPLUA

// Logger instance userdata type
typedef struct jplua_logger_s {
	fileHandle_t fileHandle;
	char fileName[MAX_QPATH];
} jplua_logger_t;

void JPLua_Logger_CreateRef( lua_State *L, const char *path );
int JPLua_GetLogger( lua_State *L );
jplua_logger_t *JPLua_CheckLogger( lua_State *L, int idx );
void JPLua_Register_Logger( lua_State *L );

#endif // JPLUA
