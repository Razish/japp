#pragma once

#ifdef JPLUA
#include "bg_luacvar.h"
#include "bg_luaentity.h"
#include "bg_luaevent.h"
#include "bg_luafs.h"
#include "bg_lualogger.h"
#include "bg_luaplayer.h"
#include "bg_luaserialiser.h"
#include "bg_luavector.h"
#include "bg_luasocket.h"

#if defined(PROJECT_GAME)
	#include "g_luasql.h"
	#include "g_luaweapon.h"
#elif defined(PROJECT_CGAME)
	#include "cg_luafont.h"
	#include "cg_luainterface.h"
	#include "cg_lualocalentity.h"
	#include "cg_luarefentity.h"
	#include "cg_luaserver.h"
	#include "cg_luatextbox.h"
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
