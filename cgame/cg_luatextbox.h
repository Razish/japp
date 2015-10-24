#pragma once

#ifdef JPLUA

#include "cg_luafont.h"

namespace JPLua {

	// TextBox instance userdata type
	struct luaTextBox_t {
		luaFont_t	*font; // from RegisterFont( int fontIndex );
		float		scale;
		std::string	text; // std::string?
		vector4		colour;
		int			style;
		bool		centered;
	};

#ifdef JPLUA_INTERNALS
	struct textBoxProperty_t {
		const char		*name;
		int				(*Get)( lua_State *L, luaTextBox_t *box );
		void			(*Set)( lua_State *L, luaTextBox_t *box );
	};
	int textBoxPropertyCompare( const void *a, const void *b );

	//internal: push a TextBox object on the stack
	void TextBox_CreateRef( lua_State *L, int fontIndex );

	// return a TextBox object
	int CreateTextBox( lua_State *L );
	luaTextBox_t *CheckTextBox( lua_State *L, int idx );

	// register the TextBox metatable
	void Register_TextBox( lua_State *L );
#endif // JPLUA_INTERNALS

} // namespace JPLUA

#endif // JPLUA
