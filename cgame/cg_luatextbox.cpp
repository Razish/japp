#include "cg_local.h"
#include "bg_luainternal.h"

#ifdef JPLUA

namespace JPLua {

	static const char TEXTBOX_META[] = "TextBox.meta";

	int TextBoxPropertyCompare( const void *a, const void *b ) {
		return strcmp( (const char *)a, ((textBoxProperty_t *)b)->name );
	}

	// push a TextBox object onto the stack
	void TextBox_CreateRef( lua_State *L ) {
		luaTextBox_t *box = new( lua_newuserdata( L, sizeof(luaTextBox_t) ) ) luaTextBox_t();
		box->font = nullptr;
		box->scale = 1.0f;
		box->text = "";
		luaL_getmetatable( L, TEXTBOX_META );
		lua_setmetatable( L, -2 );
	}

	// ensure the value at the specified index is a valid TextBox object,
	// return the instance if it is, otherwise return NULL.
	luaTextBox_t *CheckTextBox( lua_State *L, int idx ) {
		void *ud = luaL_checkudata( L, idx, TEXTBOX_META );
		luaL_argcheck( L, ud != NULL, 1, "'TextBox' expected" );
		return (luaTextBox_t *)ud;
	}

	int TextBox_GetMetaTable( lua_State *L ) {
		luaL_getmetatable( L, TEXTBOX_META );
		return 1;
	}

	static int TextBox_GetCentered( lua_State *L, luaTextBox_t *box ) {
		lua_pushboolean( L, box->centered );
		return 1;
	}

	static void TextBox_SetCentered( lua_State *L, luaTextBox_t *box ) {
		if ( lua_isboolean( L, 3 ) ) {
			const int centered = lua_toboolean( L, 3 );
			box->centered = centered;
		}
	}

	static int TextBox_GetColour( lua_State *L, luaTextBox_t *box ) {
		lua_newtable( L );
		int top = lua_gettop( L );
		for ( int i = 0; i < 4; i++ ) {
			lua_pushinteger( L, 1 + i );
			lua_pushinteger( L, box->colour.raw[i] );
			lua_settable( L, top );
		}
		return 1;
	}

	static void TextBox_SetColour( lua_State *L, luaTextBox_t *box ) {
		ReadFloats( box->colour.raw, 4, L, 3 );
	}

	static int TextBox_GetFont( lua_State *L, luaTextBox_t *box ) {
		if ( box->font && box->font->reference ) {
			lua_rawgeti( L, LUA_REGISTRYINDEX, box->font->reference );
		}
		else {
			lua_pushnil( L );
		}
		return 1;
	}

	static void TextBox_SetFont( lua_State *L, luaTextBox_t *box ) {
		// if one already exists, remove it from the registry so GC can do its thing
		if ( box->font && box->font->reference ) {
			luaL_unref( L, LUA_REGISTRYINDEX, box->font->reference );
			box->font->reference = 0;
		}
		luaFont_t *font = CheckFont( L, 3 );
		box->font = font;
	}

	static int TextBox_GetScale( lua_State *L, luaTextBox_t *box ) {
		lua_pushnumber( L, box->scale );
		return 1;
	}

	static void TextBox_SetScale( lua_State *L, luaTextBox_t *box ) {
		const float scale = luaL_checknumber( L, 3 );
		box->scale = scale;
	}

	static int TextBox_GetStyle( lua_State *L, luaTextBox_t *box ) {
		lua_pushinteger( L, box->style );
		return 1;
	}

	static void TextBox_SetStyle( lua_State *L, luaTextBox_t *box ) {
		const int style = luaL_checkinteger( L, 3 );
		box->style = style;
	}

	static int TextBox_GetText( lua_State *L, luaTextBox_t *box ) {
		lua_pushstring( L, box->text.c_str() );
		return 1;
	}

	static void TextBox_SetText( lua_State *L, luaTextBox_t *box ) {
		const char *text = luaL_checkstring( L, 3 );
		box->text = text;
	}

	static const textBoxProperty_t textBoxProperties[] = {
		{
			"centered",
			TextBox_GetCentered,
			TextBox_SetCentered
		},
		{
			"colour",
			TextBox_GetColour,
			TextBox_SetColour
		},
		{
			"font",
			TextBox_GetFont,
			TextBox_SetFont
		},
		{
			"scale",
			TextBox_GetScale,
			TextBox_SetScale
		},
		{
			"style",
			TextBox_GetStyle,
			TextBox_SetStyle
		},
		{
			"text",
			TextBox_GetText,
			TextBox_SetText
		}
	};
	static const size_t numTextBoxProperties = ARRAY_LEN( textBoxProperties );

	static int TextBox_Index( lua_State *L ) {
		luaTextBox_t *box = CheckTextBox( L, 1 );
		const char *key = lua_tostring( L, 2 );
		int returnValues = 0;

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );
		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		// assume it's a field
		const textBoxProperty_t *property = (textBoxProperty_t *)bsearch( key, textBoxProperties, numTextBoxProperties,
			sizeof(textBoxProperty_t), TextBoxPropertyCompare
		);
		if ( property ) {
			if ( property->Get ) {
				returnValues += property->Get( L, box );
			}
		}
		else {
			lua_pushnil( L );
			returnValues++;
		}

		return returnValues;
	}

	static int TextBox_NewIndex( lua_State *L ) {
		luaTextBox_t *box = CheckTextBox( L, 1 );
		const char *key = lua_tostring( L, 2 );

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );

		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		// assume it's a field
		const textBoxProperty_t *property = (textBoxProperty_t *)bsearch(key, textBoxProperties, numTextBoxProperties,
			sizeof(textBoxProperty_t), TextBoxPropertyCompare
		);
		if ( property ) {
			if ( property->Set ) {
				property->Set( L, box );
			}
		}
		else {
			// ...
		}

		return 0;
	}

	//Func: CreateTextBox( fontIndex )
	//Retn: TextBox object
	int CreateTextBox( lua_State *L ) {
		TextBox_CreateRef( L );
		return 1;
	}

	//Func: TextBox1 == TextBox2
	//Retn: boolean value of whether TextBox1 is the same TextBox as TextBox2
	static int TextBox_Equals( lua_State *L ) {
		luaTextBox_t *box1 = CheckTextBox( L, 1 );
		luaTextBox_t *box2 = CheckTextBox( L, 2 );
		bool eq = (box1->text.compare( box2->text ) == 0)
			&& (box1->font == box2->font)
			&& flcmp( box1->scale, box2->scale );
		lua_pushboolean( L, eq ? 1 : 0 );
		return 1;
	}

	//Func: tostring( TextBox )
	//Retn: string representing the TextBox instance (for debug/error messages)
	static int TextBox_ToString( lua_State *L ) {
		luaTextBox_t *box = CheckTextBox( L, 1 );
		lua_pushfstring( L, "TextBox(%s)", box->text.c_str() );
		return 1;
	}

	static int TextBox_GarbageCollect( lua_State *L ) {
		luaTextBox_t *box = static_cast<luaTextBox_t *>( CheckTextBox( L, 1 ) );
		box->~luaTextBox_t();

		if ( box->font && box->font->reference ) {
			luaL_unref( L, LUA_REGISTRYINDEX, box->font->reference );
			box->font->reference = 0;
		}

		box->font = nullptr;

		return 1;
	}

	//Func: TextBox:Draw( number x, number y )
	//Retn: --
	static int TextBox_Draw( lua_State *L ) {
		luaTextBox_t *box = CheckTextBox( L, 1 );
		float x = luaL_checknumber( L, 2 );
		float y = luaL_checknumber( L, 3 );
		const char *text = box->text.c_str();
		float scale = box->scale;
		int style = box->style;
		int customFont = 0; // ???
		int iMenuFont = box->font ? box->font->index : (customFont ? 0 : FONT_MEDIUM);

		if ( box->centered ) {
			float width = 0.0f;
			if ( customFont ) {
				width = trap->R_Font_StrLenPixels( text, box->font->handle, scale );
			}
			else {
				width = CG_Text_Width( text, scale, box->font->index );
			}
			x -= width / 2.0f;
		}

		CG_Text_Paint( x, y, scale, &box->colour, text, 0.0f, 0, style, iMenuFont,
			customFont
		);
		return 0;
	}

	static const struct luaL_Reg textBoxMeta[] = {
		{ "__index", TextBox_Index },
		{ "__newindex", TextBox_NewIndex },
		{ "__eq", TextBox_Equals },
		{ "__tostring", TextBox_ToString },
		{ "__gc", TextBox_GarbageCollect },
		{ "Draw", TextBox_Draw },
		{ NULL, NULL }
	};

	// register the TextBox class for Lua
	void Register_TextBox( lua_State *L ) {
		luaL_newmetatable( L, TEXTBOX_META ); // create metatable for TextBox class, push on stack

		// lua won't attempt to directly index userdata, only via metatables
		// make this metatable's __index loopback to itself so can index the object directly
		lua_pushstring( L, "__index" );
		lua_pushvalue( L, -2 ); // re-push metatable to top of stack
		lua_settable( L, -3 ); // metatable.__index = metatable

		// fill metatable with fields
		for ( const luaL_Reg *r = textBoxMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}

		lua_pop( L, -1 ); // pop the TextBox class metatable from the stack
	}

} // namespace JPLua

#endif // JPLUA
