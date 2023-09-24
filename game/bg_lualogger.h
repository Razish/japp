#pragma once

#ifdef JPLUA

namespace JPLua {

// Logger instance userdata type
struct logger_t {
    fileHandle_t fileHandle;
    char fileName[MAX_QPATH];
};

#ifdef JPLUA_INTERNALS
void Logger_CreateRef(lua_State *L, const char *path);
int GetLogger(lua_State *L);
logger_t *CheckLogger(lua_State *L, int idx);
void Register_Logger(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
