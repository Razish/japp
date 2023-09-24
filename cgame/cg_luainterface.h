#pragma once
#include "ui/ui_shared.h"

#ifdef JPLUA

namespace JPLua {

struct interfaceMenu_t {
    int id;
    char name[64];
};

struct interfaceItem_t {
    int id;
    char name[64];
    menuDef_t *parent;
};

void Interface_CallMenuFunc(int type, menuDef_t *menu);
void Interface_CallItemFunc(int type, itemDef_t *item);

itemDef_t *Interface_CreateItem(menuDef_t *menu, const char *name, int type);
menuDef_t *Interface_CreateMenu(float x, float y, float w, float h, const char *name);

#ifdef JPLUA_INTERNALS
// internal functions
void Register_Menu(lua_State *L);
menuDef_t *CheckMenu(lua_State *L, int idx);
void Menu_CreateRef(lua_State *L, menuDef_t *menu);
itemDef_t *CheckItem(lua_State *L, int idx);
void Item_CreateRef(lua_State *L, itemDef_t *item, menuDef_t *menu);
void Register_Item(lua_State *L);
int Interface_CreateMenu(lua_State *L);
int Interface_GetMenu(lua_State *L);
#endif // JPLUA_INTERNALS

} // namespace JPLua

#endif // JPLUA
