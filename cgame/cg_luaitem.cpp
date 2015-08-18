#include "cg_local.h"
#include "bg_lua.h"
#include "ui/ui_shared.h"
#ifdef JPLUA

static const char ITEM_META[] = "Item.meta";
extern void Item_InitControls( itemDef_t *item );
extern void Item_ValidateTypeData( itemDef_t *item );

itemDef_t *Interface_CreateItem( menuDef_t *menu, const char *name, int type ) {
	if ( menu->itemCount < MAX_MENUITEMS ) {
		menu->items[menu->itemCount] = (itemDef_t *)BG_Alloc( sizeof(itemDef_t) );
		Item_Init( menu->items[menu->itemCount] );
		menu->items[menu->itemCount]->id = menu->itemCount;
		menu->items[menu->itemCount]->window.name = BG_StringAlloc( name );
		menu->items[menu->itemCount]->parent = menu;
		menu->itemCount++;
		return menu->items[menu->itemCount-1];
	}

	return NULL;
}

itemDef_t *Interface_FindItemByID( menuDef_t *menu, int id ) {
	for ( int i = 0; i < menu->itemCount; i++ ) {
		if ( menu->items[i]->id == id ) {
			return menu->items[i];
		}
	}

	return NULL;
}

itemDef_t *Interface_FindItemByName( menuDef_t *menu, char *name ) {
	for ( int i = 0; i < menu->itemCount; i++ ) {
		if ( !Q_stricmp( menu->items[i]->window.name, name ) ) {
			return menu->items[i];
		}
	}

	return NULL;
}

void JPLua_Item_CreateRef( lua_State *L, itemDef_t *item, menuDef_t *menu ) {
	jplua_interface_item_t *data = (jplua_interface_item_t *)lua_newuserdata( L, sizeof(jplua_interface_item_t) );
	data->id = item->id;
	Q_strncpyz( data->name, item->window.name, sizeof(data->name) );
	data->parent = menu;
	luaL_getmetatable( L, ITEM_META );
	lua_setmetatable( L, -2 );
}

itemDef_t *JPLua_CheckItem( lua_State *L, int idx ) {
	jplua_interface_item_t *data;
	void *ud = luaL_checkudata( L, idx, ITEM_META );

	luaL_argcheck( L, ud != NULL, 1, "'Item' expected" );
	data = (jplua_interface_item_t *)ud;
	return Interface_FindItemByName( data->parent,data->name );
}

void JPLua_Interface_CallItemFunc( int type, itemDef_t *item ) {
	lua_State *L = JPLua.state;

	switch ( type ) {
	case 1: {
		// onFocus
		if ( item->lua_onFocus ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_onFocus );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 2: {
		// onLeaveFocus
		if ( item->lua_leaveFocus ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_leaveFocus );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 3: {
		// onAction
		if ( item->lua_action ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_action );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 4: {
		// onAccept
		if ( item->lua_accept ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_accept );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 5: {
		// onMouseEnter
		if ( item->lua_mouseEnter ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_mouseEnter );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 6: {
		// onMouseEnterText
		if ( item->lua_mouseEnterText ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_mouseEnterText );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 7: {
		// onMouseLeave
		if ( item->lua_mouseLeave ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_mouseLeave );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	case 8: {
		// onMouseLeaveText
		if ( item->lua_mouseLeaveText ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, item->lua_mouseLeaveText );
			JPLua_Call( L, 0, 0 );
			break;
		}
	} break;
	default: {
		break;
	} break;
	}

}

#if 0
static int JPLua_Item_SetFocusFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_onFocus = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetLeaveFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_leaveFocus = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetActionFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_action = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetAcceptFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_accept = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetMouseEnterFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_mouseEnter = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetMouseEnterTextFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_mouseEnterText = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetMouseLeaveFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_mouseLeave = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_SetMouseLeaveTextFunc( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		item->lua_mouseLeaveText = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !item->uselua ) {
			item->uselua = qtrue;
		}
	}

	return 0;
}
#endif

static int JPLua_Item_Equals( lua_State *L ) {
	itemDef_t *i1 = JPLua_CheckItem( L, 1 );
	itemDef_t *i2 = JPLua_CheckItem( L, 2 );

	lua_pushboolean( L, (i1->id == i2->id) ? 1 : 0 );

	return 1;
}

static int JPLua_Item_ToString( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	lua_pushfstring( L, "Item(%d)(%s)", item->id, item->window.name );

	return 1;
}

static int JPLua_Item_Finalize( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	Item_InitControls( item );

	return 0;
}

static int JPLua_Item_AddColorRange( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	float low = luaL_checknumber( L, 2 );
	float high = luaL_checknumber( L, 3 );
	colorRangeDef_t color;
	color.low = low;
	color.high = high;
	JPLua_ReadFloats( color.color.raw, 4, L, 4 );
	if ( item->numColors < MAX_COLOR_RANGES ) {
		memcpy( &item->colorRanges[item->numColors], &color, sizeof(color) );
		item->numColors++;
	}

	return 0;
}

static int JPLua_Item_Align( lua_State *L ){
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->alignment = luaL_checkinteger( L, 2 );

	return 0;
}

//TOGO: GetAutowrapped
static int JPLua_Item_Autowrapped( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->window.flags |= WINDOW_AUTOWRAPPED;
	}
	else {
		item->window.flags &= ~WINDOW_AUTOWRAPPED;
	}

	return 0;
}

static int JPLua_Item_AppearanceSlot( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->appearanceSlot = luaL_checkinteger( L, 2 );

	return 0;
}

//TODO: implement SetAssetModel
static int JPLua_Item_AssetModel( lua_State *L ) {
	//itemDef_t *item = JPLua_CheckItem( L, 1 );
	//const char *model = luaL_checkstring( L, 2 );
	//FIXME: need to String_Alloc space for string, copy and assign to item?

	return 0;
}

static int JPLua_Item_AssetShader( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->asset = trap->R_RegisterShaderNoMip( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_BackColor( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	JPLua_ReadFloats( item->window.backColor.raw, 4, L, 2 );

	return 0;
}

static int JPLua_Item_Background( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.background = trap->R_RegisterShaderNoMip( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_Border( lua_State *L ){
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.border = luaL_checkinteger( L, 2 );

	return 0;
}

static int JPLua_Item_BorderColor( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	JPLua_ReadFloats( item->window.borderColor.raw, 4, L, 2 );

	return 0;
}

static int JPLua_Item_BorderSize( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.borderSize = luaL_checknumber( L, 2 );

	return 0;
}

static int JPLua_Item_Cinematic( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.cinematicName = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_Columns( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int num = luaL_checkinteger( L, 2 );
	if ( lua_type( L, 3 ) != LUA_TTABLE ) {
		trap->Print( "JPLua_Item_Columns failed, Argument 3 not a table\n" );
		JPLua_StackDump( L );
		return 0;
	}


	listBoxDef_t *listPtr;

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	listPtr = (listBoxDef_t *)item->typeData;
	if ( num > MAX_LB_COLUMNS ) {
		num = MAX_LB_COLUMNS;
	}
	listPtr->numColumns = num;

	for ( int i = 0; i < num && lua_next( L, 3 );  i++ ) {
		if ( lua_type( L, -1 ) != LUA_TTABLE ) {
			trap->Print( "JPLua_Item_Colums failed, Arg 3 contains not a table\n" );
			JPLua_StackDump( L );
			return 0;
		}
		for ( int j = 0; i < 3 && lua_next( L, -1 ); i++ ) {
			switch ( j ) {
			case 0: {
				listPtr->columnInfo[i].pos = luaL_checknumber( L, -2 );
				lua_pop( L, 1 );
				break;
			} break;
			case 1: {
				listPtr->columnInfo[i].width = luaL_checknumber( L, -2 );
				lua_pop( L, 1 );
				break;
			} break;
			case 2: {
				listPtr->columnInfo[i].maxChars = luaL_checknumber( L, -2 );
				lua_pop( L, 1 );
				break;
			} break;
			default: {
				break;
			} break;
			}
		}
	}

	return 0;
}

static int JPLua_Item_DescText( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->descText = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

//TODO: GetDecoration
static int JPLua_Item_Decoration( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->window.flags |= WINDOW_DECORATION;
	}
	else {
		item->window.flags &= ~WINDOW_DECORATION;
	}

	return 0;
}

static int JPLua_Item_DoubleClick( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		listPtr->lua_DoubleClick = luaL_ref( L, LUA_REGISTRYINDEX );
		if ( !listPtr->uselua ) {
			listPtr->uselua = qtrue;
		}
	}

	return 0;
}

static int JPLua_Item_ElementHeight( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	float value = luaL_checknumber( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	listPtr->elementHeight = value;

	return 0;
}

static int JPLua_Item_ElementType( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int value = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	listPtr->elementStyle = value;

	return 0;
}

static int JPLua_Item_ElementWidth( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	float value = luaL_checknumber( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	listPtr->elementWidth = value;

	return 0;
}

static int JPLua_Item_Feeder( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->special.i = luaL_checkinteger( L, 2 );

	return 0;
}

//FIXME: rename, move to header
extern const char *styles[];

typedef struct itemFlagsDef_s {
	const char *string;
	int value;
} itemFlagsDef_t;

static const itemFlagsDef_t itemFlags[] = {
	{ "WINDOW_INACTIVE", WINDOW_INACTIVE },
	{ NULL, 0 }
};

static int JPLua_Item_Flag( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	const char *flag = luaL_checkstring( L, 2 );
	int i;
	for ( i = 0; styles[i]; i++ ) {
		if ( !Q_stricmp( flag, itemFlags[i].string ) ) {
			item->window.flags |= itemFlags[i].value;
			break;
		}
	}

	if ( itemFlags[i].string == NULL ) {
		trap->Print( S_COLOR_YELLOW "Unknown item style value: '%s'\n", flag );
	}

	return 0;
}

static int JPLua_Item_FocusSound( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->focusSound = trap->S_RegisterSound( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_Font( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	Item_ValidateTypeData( item );
	item->iMenuFont = luaL_checkinteger( L, 2 );

	return 0;
}

static int JPLua_Item_Forecolor( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	JPLua_ReadFloats( item->window.foreColor.raw, 4, L, 2 );

	//TODO: better bit-flag checking, i.e. number of bits set
	int flag = luaL_checkinteger( L, 3 );

	switch ( flag ) {
	case 1: {
		// playerColor
		item->window.flags |= WINDOW_PLAYERCOLOR;
		break;
	} break;

	case 2: {
		// forecolorSet
		item->window.flags |= WINDOW_FORECOLORSET;
		break;
	} break;

	default: {
		break;
	} break;
	}

	return 0;
}

static int JPLua_Item_Group( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	//TODO: validate item as per above? for next few functions too

	item->window.group = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_HorizontalScroll( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->window.flags |= WINDOW_HORIZONTAL;
	}
	else {
		item->window.flags &= ~WINDOW_HORIZONTAL;
	}

	return 0;
}

static int JPLua_Item_isCharacter( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->flags |= ITF_ISCHARACTER;
	}
	else {
		item->flags &= ~ITF_ISCHARACTER;
	}

	return 0;
}

static int JPLua_Item_isSaber( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->flags |= ITF_ISSABER;
	}
	else {
		item->flags &= ~ITF_ISSABER;
	}

	return 0;

}

static int JPLua_Item_isSaber2( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->flags |= ITF_ISSABER2;
	}
	else {
		item->flags &= ~ITF_ISSABER2;
	}

	return 0;
}

static int JPLua_Item_maxChars( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int value = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;
	editPtr->maxChars = value;

	return 0;
}

static int JPLua_Item_maxPaintChars( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int value = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	editFieldDef_t *editPtr = (editFieldDef_t *)item->typeData;
	editPtr->maxPaintChars = value;

	return 0;
}

static int JPLua_Item_ModelAngles( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int x = luaL_checkinteger( L, 2 );
	int y = luaL_checkinteger( L, 3 );
	int z = luaL_checkinteger( L, 4 );

	Item_ValidateTypeData( item );
	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	modelPtr->angle = x;
	modelPtr->angle2 = y;
	modelPtr->angle3 = z;

	return 0;
}

static int JPLua_Item_ModelFovXY( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	float x = luaL_checknumber( L, 2 );
	float y = luaL_checknumber( L, 3 );

	Item_ValidateTypeData( item );
	//FIXME: early return if not valid?

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	modelPtr->fov_x = x;
	modelPtr->fov_y = y;

	return 0;
}

static int JPLua_Item_Origin( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	vector3 *vec = JPLua_CheckVector( L, 2 );

	Item_ValidateTypeData( item );

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	VectorCopy( vec, &modelPtr->origin );

	return 0;
}

static int JPLua_Item_Rotation( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int x = luaL_checkinteger( L, 2 );
	int y = luaL_checkinteger( L, 3 );
	int z = luaL_checkinteger( L, 4 );

	Item_ValidateTypeData( item );

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	modelPtr->rotationSpeed = x;
	modelPtr->rotationSpeed2 = y;
	modelPtr->rotationSpeed3 = z;

	return 0;
}

static int JPLua_Item_G2Bounds( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	vector3 *mins = JPLua_CheckVector( L, 2 );
	vector3 *maxs = JPLua_CheckVector( L, 3 );

	Item_ValidateTypeData( item );

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	VectorCopy( mins, &modelPtr->g2mins );
	VectorCopy( maxs, &modelPtr->g2maxs );

	return 0;
}

static int JPLua_Item_G2Scale( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	vector3 *scale = JPLua_CheckVector( L, 2 );

	Item_ValidateTypeData( item );

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	VectorCopy( scale, &modelPtr->g2scale );

	return 0;
}

static int JPLua_Item_G2Skin( lua_State *L ){
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	const char *skin = luaL_checkstring( L, 3 );

	Item_ValidateTypeData( item );

	char modelPath[MAX_QPATH] = {};
	char skinpath[MAX_QPATH] = {};
	char *finalSkin = NULL;
	if ( !Q_stricmp( skin, "model" ) ) {
		trap->Cvar_VariableStringBuffer( skin, skinpath, sizeof( skinpath ) );

		char *skinPtr = strstr( skinpath, "/" );

		if ( skinPtr && strchr( skinPtr, '|' ) && skinPtr[0] == '/' ) {
			// got a custom skin (3 part)
			skinPtr[0] = '\0';
			Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/|%s", skinpath, skinPtr + 1 );
		}
		else if ( skinPtr && skinPtr[0] == '/' ) {
			// got a team/variant skin
			skinPtr[0] = '\0';
			Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model_%s.skin", skinpath, skinPtr + 1 );
		}
		else {
			// regular/default model
			Com_sprintf( modelPath, sizeof(modelPath), "models/players/%s/model_default.skin", skinpath );
		}
		finalSkin = &modelPath[0];
	}

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	modelPtr->g2skin = trap->R_RegisterSkin( finalSkin );

	return 0;
}

//FIXNE: move decl to header
extern stringID_table_t animTable[MAX_ANIMATIONS + 1];

static int JPLua_Item_G2Anim( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	const char *anim = luaL_checkstring( L, 2 );

	Item_ValidateTypeData( item );

	modelDef_t *modelPtr = (modelDef_t *)item->typeData;
	for ( int i = 0; i < MAX_ANIMATIONS; i++ ) {
		if ( !Q_stricmp( anim, animTable[i].name ) ) {
			// found it
			modelPtr->g2anim = i;
			return 1;
		}
	}

	return 0;
}

static int JPLua_Item_Name( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.name = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_NotSelectable( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	Item_ValidateTypeData( item );

	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;
	if ( item->type == ITEM_TYPE_LISTBOX && listPtr ) {
		listPtr->notselectable = value;
	}

	return 0;
}

static int JPLua_Item_OutlineColor( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	JPLua_ReadFloats( item->window.outlineColor.raw, 4, L, 2 );

	return 0;
}

static int JPLua_Item_OwnerDraw( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int value = luaL_checkinteger( L, 2 );

	item->window.ownerDraw = value;
	item->type = ITEM_TYPE_OWNERDRAW;

	return 0;
}

static int JPLua_Item_OwnerDrawFlag( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	uint32_t bit = luaL_checkinteger( L, 2 );
	int found = 0;

	for ( uint32_t i = 0; i < 32u; i++ ) {
		if ( (bit & (1 << i)) ) {
			found++;
		}
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits set (%i)\n", found );
		return 1;
	}

     item->window.ownerDrawFlags |= bit;
	 return 0;
}

static int JPLua_Item_Rect( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int x = luaL_checkinteger( L, 2 );
	int y = luaL_checkinteger( L, 3 );
	int w = luaL_checkinteger( L, 4 );
	int h = luaL_checkinteger( L, 5 );

	item->window.rect.x = x;
	item->window.rect.y = y;
	item->window.rect.w = w;
	item->window.rect.h = h;

	return 0;
}

#if 0
static int JPLua_Item_Special( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->special.f = luaL_checknumber( L, 2 );

	return 0;
}
#endif

static int JPLua_Item_Style( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->window.style = luaL_checkinteger( L, 2 );

	return 0;
}

static int JPLua_Item_Text( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->text = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_TextAlign( lua_State *L ){
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->textalignment = luaL_checkinteger( L, 2 );

	return 0;
}

static int JPLua_Item_TextAlignXY( lua_State *L ){
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->textalignx = luaL_checknumber( L, 2 );
	item->textaligny = luaL_checknumber( L, 3 );

	return 0;
}

static int JPLua_Item_TextScale( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->textscale = luaL_checknumber( L, 2 );

	return 0;

}

static int JPLua_Item_TextStyle( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->textStyle = luaL_checkinteger( L, 2 );

	return 0;
}

static int JPLua_Item_Text2( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->text2 = String_Alloc( luaL_checkstring( L, 2 ) );

	return 0;
}

static int JPLua_Item_Text2AlignXY( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	float x = luaL_checknumber( L, 2 );
	float y = luaL_checknumber( L, 3 );
	item->text2alignx = x;
	item->text2aligny = y;

	return 0;
}

#if 0
static int JPLua_Item_Type( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	item->type = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );

	return 0;
}
#endif

static int JPLua_Item_Visible( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->window.flags |= WINDOW_VISIBLE;
	}
	else {
		item->window.flags &= ~WINDOW_VISIBLE;
	}

	return 0;
}

#if 0
static int JPLua_Item_Wrapped( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	if ( value ) {
		item->window.flags |= WINDOW_WRAPPED;
	}
	else {
		item->window.flags &= WINDOW_WRAPPED;
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_MaxLineChars( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int maxChars = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	textScrollDef_t *scrollPtr = (textScrollDef_t *)item->typeData;
	scrollPtr->maxLineChars = maxChars;

	return 0;
}
#endif

#if 0
static int JPLua_Item_LineHeight( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	int	height = luaL_checkinteger( L, 2 );

	Item_ValidateTypeData( item );
	if ( !item->typeData ) {
		return 0;
	}

	textScrollDef_t *scrollPtr = (textScrollDef_t *)item->typeData;
	scrollPtr->lineHeight = height;

	return 0;
}
#endif

#if 0
static int JPLua_Item_Invertyesno( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->invertYesNo = luaL_checkinteger( L, 2 );

	return 0;
}
#endif

#if 0
static int JPLua_Item_ScrollHidden( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );
	qboolean value = lua_toboolean( L, 2 );

	Item_ValidateTypeData( item );
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;

	if ( item->type == ITEM_TYPE_LISTBOX && listPtr ) {
		listPtr->scrollhidden = value;
	}

	return 0;
}
#endif

#if 0
static int JPLua_Item_XOffset( lua_State *L ) {
	itemDef_t *item = JPLua_CheckItem( L, 1 );

	item->xoffset = luaL_checkinteger( L, 2 );

	return 0;
}
#endif

#if 0
static int JPLua_Item_Slider( lua_State *L ) {
    itemDef_t *item = JPLua_CheckItem( L, 1 );

	int x = luaL_checkinteger( L, 2 );
	int y = luaL_checkinteger( L, 3 );
	int w = luaL_checkinteger( L, 4 );
	int h = luaL_checkinteger( L, 5 );

	item->slider.x = x;
	item->slider.y = y;
	item->slider.w = w;
	item->slider.h = h;

	return 0;
}
#endif

static const struct luaL_Reg jplua_item_meta[] = {
	{ "__tostring", JPLua_Item_ToString },
	{ "__eq", JPLua_Item_Equals },
	{ "AddColorRange", JPLua_Item_AddColorRange },
	{ "Align", JPLua_Item_Align },
	{ "AutoWrapped", JPLua_Item_Autowrapped },
	{ "AppearanceSlot", JPLua_Item_AppearanceSlot },
	{ "AssetModel", JPLua_Item_AssetModel },
	{ "AssetShader", JPLua_Item_AssetShader },
	{ "Backcolor", JPLua_Item_BackColor },
	{ "Background", JPLua_Item_Background },
	{ "Border", JPLua_Item_Border },
	{ "BorderColor", JPLua_Item_BorderColor },
	{ "BorderSize", JPLua_Item_BorderSize },
	{ "Cinematic", JPLua_Item_Cinematic },
	{ "Columns", JPLua_Item_Columns },
	{ "DescText", JPLua_Item_DescText },
	{ "Decoration", JPLua_Item_Decoration },
	{ "DoubleClick", JPLua_Item_DoubleClick },
	{ "ElementHeight", JPLua_Item_ElementHeight },
	{ "ElementType", JPLua_Item_ElementType },
	{ "ElementWidth", JPLua_Item_ElementWidth },
	{ "Feeder", JPLua_Item_Feeder },
	{ "Flag", JPLua_Item_Flag },
	{ "FocusSound", JPLua_Item_FocusSound },
	{ "Font", JPLua_Item_Font },
	{ "Forecolor", JPLua_Item_Forecolor },
	{ "Group", JPLua_Item_Group },
	{ "HorizontalScroll", JPLua_Item_HorizontalScroll },
	{ "isCharacter", JPLua_Item_isCharacter },
	{ "isSaber", JPLua_Item_isSaber },
	{ "isSaber2", JPLua_Item_isSaber2 },
	{ "maxChars", JPLua_Item_maxChars },
	{ "maxPaintChars", JPLua_Item_maxPaintChars },
	{ "ModelAngles", JPLua_Item_ModelAngles },
	{ "ModelFOV", JPLua_Item_ModelFovXY },
	{ "ModelOrigin", JPLua_Item_Origin },
	{ "ModelRotation", JPLua_Item_Rotation },
	{ "G2Bounds", JPLua_Item_G2Bounds },
	{ "G2Scale", JPLua_Item_G2Scale },
	{ "G2Skin", JPLua_Item_G2Skin },
	{ "G2Anim", JPLua_Item_G2Anim },
	{ "Name", JPLua_Item_Name },
	{ "NotSelectable", JPLua_Item_NotSelectable },
	{ "OutlineColor", JPLua_Item_OutlineColor },
	{ "OwnerDraw", JPLua_Item_OwnerDraw },
	{ "OwnerDrawFlag", JPLua_Item_OwnerDrawFlag },
	{ "Rect", JPLua_Item_Rect },
	{ "Special", JPLua_Item_Style },
	{ "Text", JPLua_Item_Text },
	{ "Text2", JPLua_Item_Text2 },
	{ "TextAlign", JPLua_Item_TextAlign },
	{ "TextAlignXY", JPLua_Item_TextAlignXY },
	{ "TextScale", JPLua_Item_TextScale },
	{ "TextStyle", JPLua_Item_TextStyle },
	{ "Text2AlignXY", JPLua_Item_Text2AlignXY },
	{ "Style", JPLua_Item_Style },
	{ "Visible", JPLua_Item_Visible },
	{ "Finalize", JPLua_Item_Finalize },
	{ NULL, NULL },

};

void JPLua_Register_Item( lua_State *L ) {
	luaL_newmetatable( L, ITEM_META );

	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 );
	lua_settable( L, -3 );

	for ( const luaL_Reg *r = jplua_item_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 );
}

#endif
