#include "cg_local.h"
#include "ui/ui_shared.h"
#include "cg_media.h"
#include "bg_luainternal.h"

#ifdef JPLUA

extern menuDef_t Menus[MAX_MENUS];
extern int menuCount;

namespace JPLua {

static const char MENU_META[] = "Menu.meta";

menuDef_t *Interface_CreateMenu(float x, float y, float w, float h, const char *name) {
    menuDef_t *menu = &Menus[menuCount];

    if (menuCount < MAX_MENUS) {
        Menu_Init(menu);
        menu->window.rect.x = x;
        menu->window.rect.y = y;
        menu->window.rect.w = w;
        menu->window.rect.h = h;
        // Menu_PostParse( menu );
        menu->id = menuCount++;
        menu->uselua = qfalse;
        menu->itemCount = 0;
        menu->window.name = BG_StringAlloc(name);
        return menu;
    }
    return NULL;
}

static int Menu_Finalize(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    if (!menu) {
        return 0;
    }

    Menu_PostParse(menu);

    return 0;
}

void Menu_CreateRef(lua_State *L, menuDef_t *menu) {
    interfaceMenu_t *data = (interfaceMenu_t *)lua_newuserdata(L, sizeof(interfaceMenu_t));
    data->id = menu->id;
    Q_strncpyz(data->name, menu->window.name, sizeof(data->name));
    luaL_getmetatable(L, MENU_META);
    lua_setmetatable(L, -2);
}

menuDef_t *CheckMenu(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, MENU_META);
    luaL_argcheck(L, ud != NULL, 1, "'Menu' expected");
    interfaceMenu_t *data = (interfaceMenu_t *)ud;

    return Menus_FindByID(data->id);
}

void Interface_CallMenuFunc(int type, menuDef_t *menu) {
    switch (type) {
    case 1: {
        // onOpen
        if (menu->lua_onOpen != 0) {
            lua_rawgeti(ls.L, LUA_REGISTRYINDEX, menu->lua_onOpen);
            Call(ls.L, 0, 0);
        }
    } break;

    case 2: {
        // onClose
        if (menu->lua_onClose != 0) {
            lua_rawgeti(ls.L, LUA_REGISTRYINDEX, menu->lua_onClose);
            Call(ls.L, 0, 0);
        }
    } break;

    case 3: {
        // onAccept
        if (menu->lua_onAccept != 0) {
            lua_rawgeti(ls.L, LUA_REGISTRYINDEX, menu->lua_onAccept);
            Call(ls.L, 0, 0);
        }
    } break;

    case 4: {
        // onEsc
        if (menu->lua_onESC != 0) {
            lua_rawgeti(ls.L, LUA_REGISTRYINDEX, menu->lua_onESC);
            Call(ls.L, 0, 0);
        }
    } break;

    default: {
    } break;
    }
}

#if 0
	static int Menu_SetOpenFunc( lua_State *L ) {
		menuDef_t *menu = CheckMenu( L, 1 );

		if ( lua_isfunction( L, 2 ) ) {
			menu->lua_onOpen = luaL_ref(L, LUA_REGISTRYINDEX);
			if ( !menu->uselua ) {
				menu->uselua = true;
			}
		}

		return 0;
	}
#endif

#if 0
	static int Menu_SetCloseFunc( lua_State *L ) {
		menuDef_t *menu = CheckMenu( L, 1 );

		if ( lua_isfunction( L, 2 ) ) {
			menu->lua_onClose = luaL_ref( L, LUA_REGISTRYINDEX );
			if ( !menu->uselua ) {
				menu->uselua = true;
			}
		}
		return 0;
	}
#endif

#if 0
	static int Menu_SetAcceptFunc( lua_State *L ) {
		menuDef_t *menu = CheckMenu(L, 1);

		if (lua_type(L, 2) == LUA_TFUNCTION){
			menu->lua_onAccept = luaL_ref(L, LUA_REGISTRYINDEX);
			if (!menu->uselua){
				menu->uselua = true;
			}
		}
		return 0;
	}
#endif

#if 0
	static int Menu_SetEscFunc( lua_State *L ) {
		menuDef_t *menu = CheckMenu( L, 1 );

		if ( lua_isfunction( L, 2 ) ) {
			menu->lua_onESC = luaL_ref( L, LUA_REGISTRYINDEX );
			if ( !menu->uselua ) {
				menu->uselua = true;
			}
		}
		return 0;
	}
#endif

int Interface_CreateMenu(lua_State *L) {
    menuDef_t *menu =
        Interface_CreateMenu(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checkstring(L, 5));
    if (menu) {
        Menu_CreateRef(L, menu);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int Interface_GetMenu(lua_State *L) {
    int arg1Type = lua_type(L, 1);
    if (arg1Type == LUA_TNUMBER) {
        menuDef_t *menu = Menus_FindByID(luaL_checkinteger(L, 1));
        if (menu) {
            Menu_CreateRef(L, menu);
        } else {
            lua_pushnil(L);
        }
    } else if (arg1Type == LUA_TSTRING) {
        menuDef_t *menu = Menus_FindByName(lua_tostring(L, 1));
        if (menu) {
            Menu_CreateRef(L, menu);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int Menu_Equals(lua_State *L) {
    menuDef_t *m1 = CheckMenu(L, 1);
    menuDef_t *m2 = CheckMenu(L, 2);
    lua_pushboolean(L, (m1->id == m2->id) ? 1 : 0);
    return 1;
}

static int Menu_ToString(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    lua_pushfstring(L, "Menu(%d)(%s)", menu->id, menu->window.name);

    return 1;
}

static int Menu_AppearanceIncrement(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    float value = luaL_checknumber(L, 2);
    menu->appearanceIncrement = value;
    return 0;
}

static int Menu_BackColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->window.backColor.raw, 4, L, 2);
    return 0;
}

static int Menu_Background(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    const char *value = luaL_checkstring(L, 2);
    menu->window.background = trap->R_RegisterShaderNoMip(value);
    return 0;
}

static int Menu_Border(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->window.border = value;
    return 0;
}

static int Menu_BorderColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->window.borderColor.raw, 4, L, 2);
    return 0;
}

static int Menu_BorderSize(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = lua_tointeger(L, 2);
    menu->window.borderSize = value;
    return 0;
}

static int Menu_Cinematic(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    menu->window.cinematicName = String_Alloc(luaL_checkstring(L, 2));
    return 0;
}

static int Menu_DescAlignment(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->descAlignment = value;
    return 0;
}

static int Menu_DescColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->descColor.raw, 4, L, 2);
    return 0;
}

static int Menu_SetDescXY(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    menu->descX = x;
    menu->descY = y;
    return 0;
}

static int Menu_SetDescScale(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->descScale = value;
    return 0;
}

static int Menu_DisableColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->disableColor.raw, 4, L, 2);
    return 0;
}

static int Menu_FadeAmount(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->fadeAmount = value;
    return 0;
}

static int Menu_FadeClamp(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->fadeClamp = value;
    return 0;
}

static int Menu_FadeCycle(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->fadeCycle = value;
    return 0;
}

static int Menu_FocusColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->focusColor.raw, 4, L, 2);
    return 0;
}

static int Menu_SetFont(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    const char *font = luaL_checkstring(L, 2);
    if (trap->R_RegisterFont(font)) {
        menu->font = String_Alloc(font);
    }
    return 0;
}

static int Menu_Forecolor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->window.foreColor.raw, 4, L, 2);
    qboolean plycolour = lua_toboolean(L, 3);
    if (plycolour) {
        menu->window.flags |= WINDOW_PLAYERCOLOR;
    } else {
        menu->window.flags |= ~WINDOW_FORECOLORSET;
    }
    return 0;
}

static int Menu_FullScreen(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    qboolean value = lua_toboolean(L, 2);
    menu->fullScreen = value;
    return 0;
}

static int Menu_CreateItem(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    const char *name = luaL_checkstring(L, 2);
    int type = luaL_checkinteger(L, 3);
    itemDef_t *item = Interface_CreateItem(menu, name, type);
    Item_CreateRef(L, item, menu);
    return 1;
}

static int Menu_SetName(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    menu->window.name = String_Alloc(luaL_checkstring(L, 2));
    return 0;
}

static int Menu_OutlineColor(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    ReadFloats(menu->window.outlineColor.raw, 4, L, 2);
    return 0;
}

#if 0
	static int Menu_OutOfBoundClick( lua_State *L ) {
		menuDef_t *menu = CheckMenu( L, 1 );
		qboolean value = lua_toboolean( L, 2 );

		if ( value ) {
			menu->window.flags |= WINDOW_OOB_CLICK;
		}
		else {
			menu->window.flags &= ~WINDOW_OOB_CLICK;
		}
		return 0;
	}
#endif

static int Menu_OwnerDraw(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int value = luaL_checkinteger(L, 2);
    menu->window.ownerDraw = value;
    return 0;
}

static int Menu_OwnerDrawFlag(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);

    // FIXME: is there a smarter way to do this?
    uint32_t bit = luaL_checkinteger(L, 2);
    int found = 0;
    for (int i = 0; i < 32; i++) {
        if (bit & (1 << i)) {
            found++;
        }
    }

    if (found > 1) {
        lua_pushfstring(L, "too many bits set (%i)\n", found);
        return 1;
    }

    menu->window.ownerDrawFlags |= bit;
    return 0;
}

static int Menu_Popup(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    qboolean value = lua_toboolean(L, 2);

    if (value) {
        menu->window.flags |= WINDOW_POPUP;
    } else {
        menu->window.flags &= ~WINDOW_POPUP;
    }

    return 0;
}

static int Menu_Rect(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
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

static int Menu_SoundLoop(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    const char *sound = luaL_checkstring(L, 2);
    menu->soundName = String_Alloc(sound);
    return 0;
}

static int Menu_Style(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    int style = luaL_checkinteger(L, 2);
    menu->window.style = style;
    return 0;
}

static int Menu_Visible(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    qboolean value = lua_toboolean(L, 2);

    if (value) {
        menu->window.flags |= WINDOW_VISIBLE;
    } else {
        menu->window.flags &= ~WINDOW_VISIBLE;
    }
    return 0;
}

static int Menu_Activate(lua_State *L) {
    menuDef_t *menu = CheckMenu(L, 1);
    Menus_CloseAll();
    Menus_ActivateByName(menu->window.name);
    trap->Key_SetCatcher(KEYCATCH_CGAME);
    return 0;
}

static const struct luaL_Reg interfaceMeta[] = {
    {"CreateMenu", Interface_CreateMenu},
    {"GetMenu", Interface_GetMenu},
    {NULL, NULL},
};

static const struct luaL_Reg menuMeta[] = {
    {"__tostring", Menu_ToString},
    {"__eq", Menu_Equals},
    {"Activate", Menu_Activate},
    {"AppearanceIncrement", Menu_AppearanceIncrement},
    {"Backcolor", Menu_BackColor},
    {"Background", Menu_Background},
    {"Border", Menu_Border},
    {"BorderColor", Menu_BorderColor},
    {"BorderSize", Menu_BorderSize},
    {"Cinematic", Menu_Cinematic},
    {"DescAlignment", Menu_DescAlignment},
    {"DescColor", Menu_DescColor},
    {"SetDescXY", Menu_SetDescXY},
    {"SetDescScale", Menu_SetDescScale},
    {"DisableColor", Menu_DisableColor},
    {"FadeAmount", Menu_FadeAmount},
    {"FadeClamp", Menu_FadeClamp},
    {"FadeCycle", Menu_FadeCycle},
    {"FocusColor", Menu_FocusColor},
    {"Font", Menu_SetFont},
    {"Forecolor", Menu_Forecolor},
    {"Fullscreen", Menu_FullScreen},
    {"CreateItem", Menu_CreateItem},
    {"SetName", Menu_SetName},
    {"OutlineColor", Menu_OutlineColor},
    {"OwnerDraw", Menu_OwnerDraw},
    {"OwnerDrawFlag", Menu_OwnerDrawFlag},
    {"Popup", Menu_Popup},
    {"Rect", Menu_Rect},
    {"SoundLoop", Menu_SoundLoop},
    {"Style", Menu_Style},
    {"Visible", Menu_Visible},
    {"Finalize", Menu_Finalize},
    {NULL, NULL},
};

void Register_Menu(lua_State *L) {
    luaL_newmetatable(L, MENU_META);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (const luaL_Reg *r = menuMeta; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    luaL_newlib(L, interfaceMeta);
    lua_setglobal(L, "ui");

    lua_pop(L, -1);
}

} // namespace JPLua

#endif // JPLUA
