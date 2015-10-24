#include "cg_local.h"
#include "bg_luainternal.h"

#ifdef JPLUA

namespace JPLua {

	static const char FONT_META[] = "Font.meta";

	int FontPropertyCompare( const void *a, const void *b ) {
		return strcmp( (const char *)a, ((fontProperty_t *)b)->name );
	}

	// push a Font object for a fontIndex onto the stack
	void Font_CreateRef( lua_State *L, int fontIndex ) {
		if ( fontIndex < 0 || fontIndex >= FONT_NUM_FONTS ) {
			lua_pushnil( L );
			return;
		}

		luaFont_t *font = (luaFont_t *)lua_newuserdata( L, sizeof(luaFont_t) );
		std::memset( font, 0, sizeof(*font) );
		font->index = fontIndex;
		font->handle = MenuFontToHandle( fontIndex );
		lua_pushvalue( L, 2 );
		font->reference = luaL_ref( L, LUA_REGISTRYINDEX );
		//lua_pop( L, 1 );

		luaL_getmetatable( L, FONT_META );
		lua_setmetatable( L, -2 );
	}

	// ensure the value at the specified index is a valid Font object,
	// return the instance if it is, otherwise return NULL.
	luaFont_t *CheckFont( lua_State *L, int idx ) {
		void *ud = luaL_checkudata( L, idx, FONT_META );
		luaL_argcheck( L, ud != NULL, 1, "'Font' expected" );
		return (luaFont_t *)ud;
	}

	int Font_GetMetaTable( lua_State *L ) {
		luaL_getmetatable( L, FONT_META );
		return 1;
	}

	static int Font_GetHandle( lua_State *L, luaFont_t *font ) {
		lua_pushinteger( L, font->handle );
		return 1;
	}

	static void Font_SetHandle( lua_State *L, luaFont_t *font ) {
		font->handle = luaL_checkinteger( L, 3 );
	}

	static int Font_GetIndex( lua_State *L, luaFont_t *font ) {
		lua_pushinteger( L, font->index );
		return 1;
	}

	static void Font_SetIndex( lua_State *L, luaFont_t *font ) {
		font->index = luaL_checkinteger( L, 3 );

		// retrieve the font handle, though perhaps we want a pointer that will be modified elsewhere? (e.g. when new
		//	font assets are loaded
		font->handle = MenuFontToHandle( font->index );
	}

	static const fontProperty_t fontProperties[] = {
		{
			"handle",
			Font_GetHandle,
			Font_SetHandle
		},
		{
			"index",
			Font_GetIndex,
			Font_SetIndex
		}
	};
	static const size_t numFontProperties = ARRAY_LEN( fontProperties );

	static int Font_Index( lua_State *L ) {
		luaFont_t *font = CheckFont( L, 1 );
		const char *key = lua_tostring( L, 2 );
		int returnValues = 0;

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );
		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		// assume it's a field
		const fontProperty_t *property = (fontProperty_t *)bsearch( key, fontProperties, numFontProperties,
			sizeof(fontProperty_t), FontPropertyCompare
		);
		if ( property ) {
			if ( property->Get ) {
				returnValues += property->Get( L, font );
			}
		}
		else {
			lua_pushnil( L );
			returnValues++;
		}

		return returnValues;
	}

	static int Font_NewIndex( lua_State *L ) {
		luaFont_t *font = CheckFont( L, 1 );
		const char *key = lua_tostring( L, 2 );

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );

		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		// assume it's a field
		const fontProperty_t *property = (fontProperty_t *)bsearch(key, fontProperties, numFontProperties,
			sizeof(fontProperty_t), FontPropertyCompare
		);
		if ( property ) {
			if ( property->Set ) {
				property->Set( L, font );
			}
		}
		else {
			// ...
		}

		return 0;
	}

	//Func: RegisterFont( fontIndex )
	//Retn: Font object
	int RegisterFont( lua_State *L ) {
		int fontIndex = luaL_checkinteger( L, 1 );
		Font_CreateRef( L, fontIndex );
		return 1;
	}

	//Func: Font1 == Font2
	//Retn: boolean value of whether Font1 is the same font as Font2
	static int Font_Equals( lua_State *L ) {
		luaFont_t *f1 = CheckFont( L, 1 );
		luaFont_t *f2 = CheckFont( L, 2 );
		lua_pushboolean( L, (f1->index == f2->index) ? 1 : 0 );
		return 1;
	}

	//Func: tostring( Font )
	//Retn: string representing the Font instance (for debug/error messages)
	static int Font_ToString( lua_State *L ) {
		luaFont_t *font = CheckFont( L, 1 );
		lua_pushfstring( L, "Font(%d)", font->index );
		return 1;
	}

	static const struct luaL_Reg fontMeta[] = {
		{ "__index", Font_Index },
		{ "__newindex", Font_NewIndex },
		{ "__eq", Font_Equals },
		{ "__tostring", Font_ToString },
		{ NULL, NULL }
	};

	// register the Font class for Lua
	void Register_Font( lua_State *L ) {
		luaL_newmetatable( L, FONT_META ); // create metatable for Font class, push on stack

		// lua won't attempt to directly index userdata, only via metatables
		// make this metatable's __index loopback to itself so can index the object directly
		lua_pushstring( L, "__index" );
		lua_pushvalue( L, -2 ); // re-push metatable to top of stack
		lua_settable( L, -3 ); // metatable.__index = metatable

		// fill metatable with fields
		for ( const luaL_Reg *r = fontMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}

		lua_pop( L, -1 ); // pop the Font class metatable from the stack
	}

} // namespace JPLua

#endif // JPLUA
