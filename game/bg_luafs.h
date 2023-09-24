#pragma once

#ifdef JPLUA

namespace JPLua {

struct file_t {
    char name[MAX_QPATH];
    fileHandle_t handle;
    fsMode_t mode;
    int length;
};

#ifdef JPLUA_INTERNALS
void File_CreateRef(lua_State *L, const char *path, fsMode_t mode);
int File_Open(lua_State *L);
int File_GetFileList(lua_State *L);
file_t *CheckFile(lua_State *L, int idx);
void Register_File(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
