#pragma once
#include "ui/ui_shared.h"

#ifdef JPLUA
typedef struct jplua_interface_menu_s{
	int id;
	char name[64];

}jplua_interface_menu_t;

typedef struct jplua_interface_item_s{
	int id;
	char *name;
	menuDef_t *parent;

}jplua_interface_item_t;

void JPLua_Register_Menu(lua_State *L);
menuDef_t *JPLua_CheckMenu(lua_State *L, int idx);
void JPLua_Menu_CreateRef(lua_State *L, menuDef_t *menu);
//
itemDef_t *JPLua_CheckItem(lua_State *L, int idx);
void JPLua_Item_CreateRef(lua_State *L, itemDef_t *item, menuDef_t *menu);
void JPLua_Register_Item(lua_State *L);
//
void JPLua_Interface_CallMenuFunc(int type, menuDef_t *menu);
void JPLua_Interface_CallItemFunc(int type, itemDef_t *item);
//
itemDef_t *Interface_CreateItem(menuDef_t *menu, const char *name, int type);
menuDef_t *Interface_CreateMenu(float x, float y, float w, float h, const char *name);
//
int JPLua_Interface_CreateMenu(lua_State *L);
int JPLua_Interface_GetMenu(lua_State *L);
#endif
