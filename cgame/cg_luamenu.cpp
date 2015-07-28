#include "cg_local.h"
#include "bg_lua.h"
#include "ui/ui_shared.h"
#include "cg_media.h"

#ifdef JPLUA

static const char MENU_META[] = "Menu.meta";
extern menuDef_t Menus[MAX_MENUS];      // defined menus
extern int menuCount;               // how many
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////



menuDef_t *Interface_CreateMenu(float x, float y, float w, float h, const char *name){
	menuDef_t *menu = &Menus[menuCount];

	if (menuCount < MAX_MENUS) {
		Menu_Init(menu);
		menu->window.rect.x = x;
		menu->window.rect.y = y;
		menu->window.rect.w = w;
		menu->window.rect.h = h;
        //Menu_PostParse(menu);
		menu->id = menuCount;
		menu->uselua = qfalse;
		menu->itemCount = 0;
		menu->window.name = BG_StringAlloc(name);
		  menuCount++;
		  return menu;
		}
	return NULL;
	}

static int JPLua_Menu_Finalize(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	if (!menu) return 0;
	Menu_PostParse(menu);
	return 0;
}


void JPLua_Menu_CreateRef(lua_State *L, menuDef_t *menu) {
	jplua_interface_menu_t *data = NULL;

	data = (jplua_interface_menu_t *)lua_newuserdata(L, sizeof(jplua_interface_menu_t));
	data->id = menu->id;
	Q_strncpyz(data->name, menu->window.name, sizeof(data->name));
	luaL_getmetatable(L, MENU_META);
	lua_setmetatable(L, -2);
}

menuDef_t *JPLua_CheckMenu(lua_State *L, int idx) {
	menuDef_t *menu = NULL;
	jplua_interface_menu_t *data;
	void *ud = luaL_checkudata(L, idx, MENU_META);
	luaL_argcheck(L, ud != NULL, 1, "'Menu' expected");
	data = (jplua_interface_menu_t *)ud;
	menu = Menus_FindByName(data->name);
	if (!menu) return NULL;
	return menu;
}

void JPLua_Interface_CallMenuFunc(int type, menuDef_t *menu){
	lua_State *L = JPLua.state;

	switch (type){
		
	case 1:             ///onOpen
		if (menu->lua_onOpen != 0){
			lua_rawgeti(L, LUA_REGISTRYINDEX, menu->lua_onOpen);
			JPLua_Call(L, 0, 0);
			break;
		}

   	case 2:             ///onClose
		if (menu->lua_onClose != 0){
			lua_rawgeti(L, LUA_REGISTRYINDEX, menu->lua_onClose);
			JPLua_Call(L, 0, 0);
			break;
		}

	case 3:             ///onAccept
		if (menu->lua_onAccept != 0){
			lua_rawgeti(L, LUA_REGISTRYINDEX, menu->lua_onAccept);
			JPLua_Call(L, 0, 0);
			break;
		}
		
	case 4:             ///onEsc
		if (menu->lua_onESC != 0){
			lua_rawgeti(L, LUA_REGISTRYINDEX, menu->lua_onESC);
			JPLua_Call(L, 0, 0);
			break;
		}

	default:
		break;


	}

}

static int JPLua_Menu_SetOpenFunc(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);

	if (lua_type(L, 2) == LUA_TFUNCTION){
		menu->lua_onOpen = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!menu->uselua){
			menu->uselua = qtrue;
		}
	}

	return 0;
}

static int JPLua_Menu_SetCloseFunc(lua_State *L){
    menuDef_t *menu = JPLua_CheckMenu(L, 1);

	if (lua_type(L, 2) == LUA_TFUNCTION){
		menu->lua_onClose = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!menu->uselua){
			menu->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Menu_SetAcceptFunc(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);

	if (lua_type(L, 2) == LUA_TFUNCTION){
		menu->lua_onAccept = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!menu->uselua){
			menu->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Menu_SetEscFunc(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);

	if (lua_type(L, 2) == LUA_TFUNCTION){
		menu->lua_onESC = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!menu->uselua){
			menu->uselua = qtrue;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

menuDef_t *Menus_GetByID(int id){
	int i;
	for (i = 0; i < menuCount; i++) {
		if (Menus[i].id  == id) {
			return &Menus[i];
		}
	}
	return NULL;
	
}

int JPLua_Interface_CreateMenu(lua_State *L){
	menuDef_t *menu = Interface_CreateMenu(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checkstring(L,5));
	if (!menu){
		lua_pushnil(L);
		return 1;
	}
	JPLua_Menu_CreateRef(L, menu);
	return 1;
}

int JPLua_Interface_GetMenu(lua_State *L){
	menuDef_t *menu = NULL;
	if (lua_type(L, 1) == LUA_TNUMBER){
		int id = luaL_checkinteger(L, 1);
		menu = Menus_GetByID(id);
		if (!menu){
			lua_pushnil(L);
			return 1;
		}
		JPLua_Menu_CreateRef(L, menu);
		return 1;
	}
	else if (lua_type(L, 1) == LUA_TSTRING) {
		const char *name = lua_tostring(L, 1);
		menu = Menus_FindByName(name);
		if (!menu){
			lua_pushnil(L);
			return 1;
		}
		JPLua_Menu_CreateRef(L, menu);
		return 1;
	}
	else{
		lua_pushnil(L);
		return 1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

static int JPLua_Menu_Equals(lua_State *L) {
	menuDef_t *m1 = JPLua_CheckMenu(L, 1), *m2 = JPLua_CheckMenu(L, 2);
	lua_pushboolean(L, (m1->id == m2->id) ? 1 : 0);
	return 1;
}

static int JPLua_Menu_ToString(lua_State *L) {
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	lua_pushfstring(L, "Menu(%d)(%s)", menu->id, menu->window.name);

	return 1;
}

static int JPLua_Menu_AppearanceIncrement(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	float value = luaL_checknumber(L, 2);
	menu->appearanceIncrement = value;
	return 0;
}

static int JPLua_Menu_BackColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->window.backColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_Background(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	const char *value = luaL_checkstring(L, 2);
	menu->window.background = trap->R_RegisterShaderNoMip(value);
	return 0;
}

static int JPLua_Menu_Border(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->window.border = value;
	return 0;
}

static int JPLua_Menu_BorderColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->window.borderColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_BorderSize(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = lua_tointeger(L, 2);
	menu->window.borderSize = value;
	return 0;
}

static int JPLua_Menu_Cinematic(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	menu->window.cinematicName = String_Alloc(luaL_checkstring(L,2));
	return 0;
}

static int JPLua_Menu_DescAlignment(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->descAlignment = value;
	return 0;
}

static int JPLua_Menu_DescColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->descColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_SetDescXY(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	menu->descX = x;
	menu->descY = y;
	return 0;
}

static int JPLua_Menu_SetDescScale(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->descScale = value;
	return 0;
}

static int JPLua_Menu_DisableColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->disableColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_FadeAmount(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->fadeAmount = value;
	return 0;
}

static int JPLua_Menu_FadeClamp(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->fadeClamp = value;
	return 0;
}

static int JPLua_Menu_FadeCycle(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->fadeCycle = value;
	return 0;
}

static int JPLua_Menu_FocusColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->focusColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_SetFont(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	const char *font = luaL_checkstring(L, 2);
	if (trap->R_RegisterFont(font)){
		menu->font = String_Alloc(font);
	}
	return 0;
}

static int JPLua_Menu_Forecolor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->window.foreColor.raw, 4, L, 2);
	qboolean plycolour = lua_toboolean(L, 3);
	if (plycolour){
		menu->window.flags |= WINDOW_PLAYERCOLOR;
	}
	else{
		menu->window.flags |= ~WINDOW_FORECOLORSET;
	}
	return 0;
}

static int JPLua_Menu_FullScreen(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	qboolean value = lua_toboolean(L, 2);
	if (value){
		menu->fullScreen = qtrue;
	}
	else{
		menu->fullScreen = qfalse;
	}
	return 0;
}

static int JPLua_Menu_CreateItem(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	const char *name = luaL_checkstring(L, 2);
	int type = luaL_checkinteger(L, 3);
	itemDef_t *item = Interface_CreateItem(menu, name, type);
	JPLua_Item_CreateRef(L, item, menu);
	return 1;
}

static int JPLua_Menu_SetName(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	menu->window.name = String_Alloc(luaL_checkstring(L, 2));
	return 0;
}

static int JPLua_Menu_OutlineColor(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	JPLua_ReadFloats(menu->window.outlineColor.raw, 4, L, 2);
	return 0;
}

static int JPLua_Menu_OutOfBoundClick(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	qboolean value = lua_toboolean(L, 2);
	
	if (value){
		menu->window.flags |= WINDOW_OOB_CLICK;
	}
	else{
		menu->window.flags &= ~WINDOW_OOB_CLICK;
	}
	return 0;
}

static int JPLua_Menu_OwnerDraw(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int value = luaL_checkinteger(L, 2);
	menu->window.ownerDraw = value;
	return 0;
}

static int JPLua_Menu_OwnerDrawFlag(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
   
	uint32_t bit = luaL_checkinteger(L, 2);
	int i, found = 0;

	for (i = 0; i<32; i++) {
		if (bit & (1 << i))
			found++;
	}

	if (found > 1) {
		lua_pushfstring(L, "too many bits set (%i)\n", found);
		return 1;
	}

	menu->window.ownerDrawFlags |= bit;
	return 0;
}

static int JPLua_Menu_Popup(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	qboolean value = lua_toboolean(L, 2);

	if (value){
		menu->window.flags |= WINDOW_POPUP;
	}
	else{
		menu->window.flags &= ~WINDOW_POPUP;
	}

	return 0;
}

static int JPLua_Menu_Rect(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);
	int w = luaL_checkinteger(L, 4);
	int h = luaL_checkinteger(L, 5);

	menu->window.rect.x = x;
	menu->window.rect.y = y;
	menu->window.rect.w = w;
	menu->window.rect.h = h;
	return 0;
}

static int JPLua_Menu_SoundLoop(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	const char *sound = luaL_checkstring(L, 2);
	menu->soundName = String_Alloc(sound);
	return 0;
}

static int JPLua_Menu_Style(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	int style = luaL_checkinteger(L, 2);
	menu->window.style = style;
	return 0;

}

static int JPLua_Menu_Visible(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	qboolean value = lua_toboolean(L, 2);

	if (value){
		menu->window.flags |= WINDOW_VISIBLE;
	}
	else{
		menu->window.flags &= ~WINDOW_VISIBLE;
	}
	return 0;
}

static int JPLua_Menu_Activate(lua_State *L){
	menuDef_t *menu = JPLua_CheckMenu(L, 1);
	Menus_CloseAll();
	Menus_ActivateByName(menu->window.name);
	trap->Key_SetCatcher(KEYCATCH_CGAME);
	return 0;
}

static const struct luaL_Reg jplua_interface[] = {
	{ "CreateMenu", JPLua_Interface_CreateMenu },
	{ "GetMenu", JPLua_Interface_GetMenu },
	{ NULL, NULL },

};

static const struct luaL_Reg jplua_menu_meta[] = {
	{ "__tostring", JPLua_Menu_ToString },
	{ "__eq", JPLua_Menu_Equals },
	{ "Activate", JPLua_Menu_Activate },
	{ "AppearanceIncrement", JPLua_Menu_AppearanceIncrement },
	{ "Backcolor", JPLua_Menu_BackColor },
	{ "Background", JPLua_Menu_Background },
	{ "Border", JPLua_Menu_Border },
	{ "BorderColor", JPLua_Menu_BorderColor },
	{ "BorderSize", JPLua_Menu_BorderSize },
	{ "Cinematic", JPLua_Menu_Cinematic },
	{ "DescAlignment", JPLua_Menu_DescAlignment },
	{ "DescColor", JPLua_Menu_DescColor },
	{ "SetDescXY", JPLua_Menu_SetDescXY },
	{ "SetDescScale", JPLua_Menu_SetDescScale },
	{ "DisableColor", JPLua_Menu_DisableColor },
	{ "FadeAmount", JPLua_Menu_FadeAmount },
	{ "FadeClamp", JPLua_Menu_FadeClamp },
	{ "FadeCycle", JPLua_Menu_FadeCycle },
	{ "FocusColor", JPLua_Menu_FocusColor },
	{ "Font", JPLua_Menu_SetFont },
	{ "Forecolor", JPLua_Menu_Forecolor },
	{ "Fullscreen", JPLua_Menu_FullScreen },
	{ "CreateItem", JPLua_Menu_CreateItem },
	{ "SetName", JPLua_Menu_SetName },
	{ "OutlineColor", JPLua_Menu_OutlineColor },
	{ "OwnerDraw", JPLua_Menu_OwnerDraw },
	{ "OwnerDrawFlag", JPLua_Menu_OwnerDrawFlag },
	{ "Popup", JPLua_Menu_Popup },
	{ "Rect", JPLua_Menu_Rect },
	{ "SoundLoop", JPLua_Menu_SoundLoop },
	{ "Style", JPLua_Menu_Style },
	{ "Visible", JPLua_Menu_Visible },
	{ "Finalize", JPLua_Menu_Finalize },
	{ NULL, NULL},

};

void JPLua_Register_Menu(lua_State *L) {
	const luaL_Reg *r;

	luaL_newmetatable(L, MENU_META);

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3); 

	for (r = jplua_menu_meta; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	luaL_newlib(L, jplua_interface);
	lua_setglobal(L, "ui");

	lua_pop(L, -1);
}

#endif