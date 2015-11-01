#pragma once

#ifdef JPLUA

namespace JPLua {

	// Server instance userdata type
	struct luaServer_t {
		uint32_t dummy;
	};

#ifdef JPLUA_INTERNALS
	int GetServer( lua_State *L );
	void Register_Server( lua_State *L );
	struct serverProperty_t {
		const char		*name;
		int				(*Get)( lua_State *L );
		void			(*Set)( lua_State *L );
	};
#endif // JPLUA_INTERNALS

} // namespace JPLUA

#endif // JPLUA
