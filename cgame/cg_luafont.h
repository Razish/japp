#pragma once

#ifdef JPLUA

namespace JPLua {

// Font instance userdata type
struct luaFont_t {
    int index; // e.g. FONT_SMALL
    int reference;
};

#ifdef JPLUA_INTERNALS
struct fontProperty_t {
    const char *name;
    int (*Get)(lua_State *L, luaFont_t *font);
    void (*Set)(lua_State *L, luaFont_t *font);
};
int fontPropertyCompare(const void *a, const void *b);

// internal: push a Font object on the stack
void Font_CreateRef(lua_State *L, int fontIndex);

// return a Font object
int RegisterFont(lua_State *L);
luaFont_t *CheckFont(lua_State *L, int idx);

// register the Font metatable
void Register_Font(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
