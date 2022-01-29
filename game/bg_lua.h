#pragma once

#ifdef JPLUA
#include "game/bg_luacvar.h"
#include "game/bg_luaentity.h"
#include "game/bg_luaevent.h"
#include "game/bg_luafs.h"
#include "game/bg_lualogger.h"
#include "game/bg_luaplayer.h"
#include "game/bg_luaserialiser.h"
#include "game/bg_luavector.h"
#include "game/bg_luasocket.h"

#if defined(PROJECT_GAME)
	#include "game/g_luasql.h"
	#include "game/g_luaweapon.h"
#elif defined(PROJECT_CGAME)
	#include "cgame/cg_luafont.h"
	#include "cgame/cg_luainterface.h"
	#include "cgame/cg_lualocalentity.h"
	#include "cgame/cg_luarefentity.h"
	#include "cgame/cg_luaserver.h"
	#include "cgame/cg_luatextbox.h"
#endif

#include "semver/semver.h"

namespace JPLua {

	struct plugin_t {
		bool enabled;

		char name[32], longname[32];
		semver_t version, requiredJPLuaVersion;

		// lua handles, internal
		int handle;
		int eventListeners[JPLUA_EVENT_MAX]; // references to listener functions in lua stored in the registry

		plugin_t *next;
	};

	// public API, no references to JPLua or Lua internals/types
	void Init( void );
	bool IsInitialised( void );
	void Shutdown( qboolean restart );
	qboolean IteratePlugins( plugin_t **plugin, bool ifActive = true ); // FIXME: hide type of plugin_t?
	qboolean IteratePluginsTemp( plugin_t **plugin, bool ifActive = true ); // FIXME: hide type of plugin_t?
	void ListPlugins( void );
	bool EnablePlugin( plugin_t *plugin );
	void DisablePlugin( plugin_t *plugin );
	plugin_t *FindPlugin( const char * const pluginName );
	void UpdateAutoload( void );
	const char *DoString( const char *str );

} // namespace JPLua

#endif // JPLUA
