#pragma once

#ifdef JPLUA

namespace JPLua {

#ifdef JPLUA_INTERNALS
	int GetServer( lua_State *L );
	void Register_Server( lua_State *L );
#endif // JPLUA_INTERNALS

} // namespace JPLUA

#endif // JPLUA
