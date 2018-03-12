#if defined(PROJECT_GAME)
#include "g_local.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#include "cg_notify.h"
#endif
#include "ui/ui_fonts.h"

#include "bg_luainternal.h"

#ifdef JPLUA

#define __STDC_FORMAT_MACROS // older compilers need this
#include <inttypes.h>
#include <unordered_map>
#include <vector>
#include "cJSON/cJSON.h"

extern int lastluaid;

namespace JPLua {

	const char *baseDir = "lua/";
	#if defined(PROJECT_GAME)
	const char *pluginDir = "lua/sv/";
	#elif defined(PROJECT_CGAME)
	const char *pluginDir = "lua/cl/";
	#endif

	luaState_t ls;
	static semver_t jpluaVersion;

	#if defined(PROJECT_GAME)
		extern std::unordered_map<int, luaWeapon_t> weaponCallbacks;
		std::unordered_map<std::string, command_t> clientCommands;
	#elif defined(PROJECT_CGAME)
		std::unordered_map<std::string, command_t> consoleCommands;
	#endif
	std::unordered_map<std::string, command_t> serverCommands;

	// parsed out from cvar setting
	static struct autoload_t {
		bool all;
		std::unordered_map<std::string, bool> plugins;
	} autoload;

	void UpdateAutoload( void ) {
	#if defined(PROJECT_GAME)
		char *autoloadStr = g_jpluaAutoload.string;
	#elif defined(PROJECT_CGAME)
		char *autoloadStr = cg_jpluaAutoload.string;
	#endif

		autoload.plugins.clear();

		if ( !strcmp( autoloadStr, "1" ) ) {
			autoload.all = true;
			return;
		}
		else if ( !strcmp( autoloadStr, "0" ) ) {
			autoload.all = false;
			return;
		}
		autoload.all = false;
		const char *delim = " ";
		for ( char *p = strtok( autoloadStr, delim ); p; p = strtok( NULL, delim ) ) {
			autoload.plugins[p] = true;
		}
	}

	void ListPlugins( void ) {
		plugin_t *plugin = nullptr;
		while ( IteratePluginsTemp( &plugin, false ) ) {
			if ( plugin->enabled ) {
				trap->Print( S_COLOR_WHITE "  %s " S_COLOR_GREEN "v%i.%i.%i " S_COLOR_WHITE " (%s)\n",
					plugin->longname, plugin->version.major, plugin->version.minor, plugin->version.patch, plugin->name
				);
			}
			else {
				trap->Print( S_COLOR_GREY "  %s\n", plugin->name );
			}
		}
	}

	plugin_t *FindPlugin( const char * const pluginName ) {
		plugin_t *plugin = nullptr;
		while ( IteratePlugins( &plugin, false ) ) {
			if ( !Q_stricmp( plugin->name, pluginName ) ) {
				return plugin;
			}
		}
		return nullptr;
	}

	qboolean IteratePlugins( plugin_t **plugin, bool ifActive ) {
		// ensure plugin exists
		if ( !*plugin ) {
			if ( ls.plugins ) {
				*plugin = ls.plugins;
			}
			else {
				*plugin = nullptr;
				return qfalse;
			}
		}
		else {
			*plugin = (*plugin)->next;
		}

		// skip over disabled plugins if we don't want them
		while ( ifActive && *plugin && !(*plugin)->enabled ) {
			*plugin = (*plugin)->next;
		}

		ls.currentPlugin = *plugin;

		return (*plugin != NULL) ? qtrue : qfalse;
	}

	// stateless version of the above - does not set ls.currentPlugin
	qboolean IteratePluginsTemp( plugin_t **plugin, bool ifActive ) {
		// ensure plugin exists
		if ( !*plugin ) {
			if ( ls.plugins ) {
				*plugin = ls.plugins;
			}
			else {
				*plugin = nullptr;
				return qfalse;
			}
		}
		else {
			*plugin = (*plugin)->next;
		}

		// skip over disabled plugins if we don't want them
		while ( ifActive && *plugin && !(*plugin)->enabled ) {
			*plugin = (*plugin)->next;
		}

		return (*plugin != NULL) ? qtrue : qfalse;
	}

	const char *DoString( const char *str ) {
		if ( luaL_dostring( ls.L, str ) != 0 ) {
			const char *errorMsg = lua_tostring( ls.L, -1 );
			trap->Print( S_COLOR_RED "Lua Error: " S_COLOR_WHITE "%s\n", errorMsg );
			return errorMsg;
		}
		return nullptr;
	}

	void DPrintf( const char *msg, ... ) {
	#ifdef JPLUA_DEBUG
		va_list argptr;
		char text[1024] = {0};

		va_start( argptr, msg );
		Q_vsnprintf( text, sizeof( text ), msg, argptr );
		va_end( argptr );

		trap->Print( "%s", text );
	#endif
	}

	void Util_ArgAsString( lua_State *L, char *out, int bufsize ) {
		int args = lua_gettop( L );
		const char *res;
		int i;

		// Lets do this a lil different, concat all args and use that as the message ^^
		Push_ToString( L ); // Ref to tostring (instead of a global lookup, in case someone changes it)

		for ( i = 1; i <= args; i++ ) {
			lua_pushvalue( L, -1 );
			lua_pushvalue( L, i );
			lua_call( L, 1, 1 ); // Assume this will never error out
			res = lua_tostring( L, -1 );
			if ( res ) {
				Q_strcat( out, bufsize, res );
			}
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );

		return;
	}

	static int Export_Print( lua_State *L ) {
		char buf[1024] = { 0 };
		//TODO: print buffering?

		Util_ArgAsString( L, buf, sizeof(buf) );
	#ifdef PROJECT_GAME
		if ( lastluaid == -1 ) {
	#endif
			trap->Print( "%s\n", buf );
	#ifdef PROJECT_GAME
		}
		else {
			trap->SendServerCommand( lastluaid, va( "print \"%s\n\"", buf ) );
		}
	#endif
		return 0;
	}

	// lua calls this if it panics, it'll then terminate the server with exit( EXIT_FAILURE )
	// this error should never happen in a release build
	static int Error( lua_State *L ) {
		trap->Print( S_COLOR_RED "*************** JA++ LUA ERROR ***************\n" );
		trap->Print( S_COLOR_RED "unprotected error in call to Lua API (%s)\n", lua_tostring( L, -1 ) );
		return 0;
	}

	qboolean Call( lua_State *L, int argCount, int resCount ) {
		if ( lua_pcall( L, argCount, resCount, 0 ) ) {
			trap->Print( S_COLOR_GREEN "JPLua " S_COLOR_RED "Error: %s\n", lua_tostring( L, -1 ) );
			lua_pop( L, 1 );

			//TODO: disable current plugin..?
			return qfalse;
		}
		return qtrue;
	}

	#define JPLUA_LOAD_CHUNKSIZE (1024)
	typedef struct gfd_s {// ls.File Data
		fileHandle_t f;
		int dataRemaining;
		char buff[JPLUA_LOAD_CHUNKSIZE];
	} gfd_t;

	// called by the loader, never access it directly
	static const char *LoadFile_Reader( lua_State *L, void *ud, size_t *sz ) {
		gfd_t *gfd = (gfd_t *)ud;

		if ( !gfd->dataRemaining ) {
			return NULL;
		}

		if ( gfd->dataRemaining >= JPLUA_LOAD_CHUNKSIZE ) {
			trap->FS_Read( gfd->buff, JPLUA_LOAD_CHUNKSIZE, gfd->f );
			gfd->dataRemaining -= JPLUA_LOAD_CHUNKSIZE;
			*sz = JPLUA_LOAD_CHUNKSIZE;
			return gfd->buff;
		}
		else {
			trap->FS_Read( gfd->buff, gfd->dataRemaining, gfd->f );
			*sz = gfd->dataRemaining;
			gfd->dataRemaining = 0;
			return gfd->buff;
		}
	}

	// Loads a file using JA's FS functions, only use THIS to load files into lua!
	char* LoadFile( lua_State *L, const char *file ) {
		fileHandle_t f = 0;
		int len = trap->FS_Open( file, &f, FS_READ );
		gfd_t gfd;
		int status;
		static char err[128];

		// file doesn't exist
		if ( !f || len <= 0 ) {
			//trap->Print( "LoadFile: Failed to load %s, file doesn't exist\n", file );
			Q_strncpyz(err, va("LoadFile: Failed to load %s, file doesn't exist\n", file), sizeof(err));
			return err;
		}
		gfd.f = f;
		gfd.dataRemaining = len;

		status = (lua_load( L, LoadFile_Reader, &gfd, va( "@%s", file ), NULL ) || lua_pcall( L, 0, 0, 0 ));
		if ( status ) {
			//trap->Print( "LoadFile: Failed to load %s: %s\n", file, lua_tostring( L, -1 ) );
			Q_strncpyz(err, lua_tostring(L, -1), sizeof(err));
			lua_pop( L, 1 );
			trap->FS_Close(f);
			return err;
		}

		trap->FS_Close( f );
		return NULL;
	}

	int StackDump( lua_State *L ) {
		int i, top = lua_gettop( L );

		// repeat for each level
		for ( i = 1; i <= top; i++ ) {
			int t = lua_type( L, i );
			switch ( t ) {
			case LUA_TSTRING:
				trap->Print( "`%s'", lua_tostring( L, i ) );
				break;
			case LUA_TBOOLEAN:
				trap->Print( lua_toboolean( L, i ) ? "true" : "false" );
				break;
			case LUA_TNUMBER:
				trap->Print( LUA_NUMBER_FMT, lua_tonumber( L, i ) );
				break;
			default:
				trap->Print( "%s", lua_typename( L, t ) );
				break;
			}
			trap->Print( "  " );
		}
		trap->Print( "\n" );

		return 0;
	}

	// Framework functions constants
	typedef enum jpLuaConsts_e {
		JPLUA_FRAMEWORK_TOSTRING,
		JPLUA_FRAMEWORK_PAIRS,
		JPLUA_FRAMEWORK_MAX,
	} jpLuaConsts_t;

	// contains lua references to framework functions, if any of these are 0 after init, we have a serious problem
	static int framework[JPLUA_FRAMEWORK_MAX];

	int Push_ToString( lua_State *L ) {
		lua_rawgeti( L, LUA_REGISTRYINDEX, framework[JPLUA_FRAMEWORK_TOSTRING] );
		return 1;
	}

	int Push_Pairs( lua_State *L ) {
		lua_rawgeti( L, LUA_REGISTRYINDEX, framework[JPLUA_FRAMEWORK_PAIRS] );
		return 1;
	}

	static const char *colourComponents[] = { "r", "g", "b", "a" };
	void ReadColour( float *out, int numComponents, lua_State *L, int idx ) {
		for ( int i = 0; i < numComponents && i < ARRAY_LEN(colourComponents); i++ ) {
			lua_getfield( L, idx, colourComponents[i] );
			out[i] = lua_tonumber( L, -1 );
			lua_pop(L, 1);
		}
	}

	void ReadFloats( float *out, int numComponents, lua_State *L, int idx ) {
		if ( lua_type( L, idx ) != LUA_TTABLE ) {
			trap->Print( "ReadFloats failed, not a table\n" );
			StackDump( L );
			return;
		}

		lua_pushnil( L );
		for ( int i = 0; i < numComponents && lua_next( L, idx ); i++ ) {
			out[i] = lua_tonumber( L, -1 );
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
	}

	void PushInfostring( lua_State *L, const char *info ) {
		const char *s;
		infoPair_t ip;
		int top = 0;

		lua_newtable( L );
		top = lua_gettop( L );

		//RAZTODO: cache userinfo somehow :/
		s = info;
		while ( s ) {
			if ( !Info_NextPair( &s, &ip ) ) {
				break;
			}

			lua_pushstring( L, ip.key ); lua_pushstring( L, ip.value ); lua_settable( L, top );
		}
	}

	void PopInfostring( lua_State *L, char *info ) {
		lua_pushnil( L );
		while ( lua_next( L, -2 ) ) {
			Info_SetValueForKey( info, lua_tostring( L, -2 ), lua_tostring( L, -1 ) );
			lua_pop( L, 1 );
		}
		lua_pop( L, 1 );
	}

	int traceback( lua_State *L ) {
		luaL_traceback( L, L, nullptr, 1 );
		Export_Print( L );
		return 1;
	}

	static int Export_Require( lua_State *L ) {
		const char *path = va( "%s%s", ls.initialised ? pluginDir : baseDir, lua_tostring( L, 1 ) );
		const char *err = NULL;
		if ((err = LoadFile(L, path))){
			trap->Print(va(S_COLOR_GREEN "JPLua:" S_COLOR_RED " Failed to load: %s\n"), lua_tostring(L, 1));
			trap->Print(va("   %s\n", err));
		}
		return 0;
	}
	static int RegisterPlugin( lua_State *L ) {
		Q_strncpyz( ls.currentPlugin->longname, lua_tostring( L, 1 ), sizeof(ls.currentPlugin->longname) );
		Q_CleanString( ls.currentPlugin->longname, STRIP_COLOUR );

		// parse out the semantic version
		char versionStr[16];
		if ( lua_isstring( L, 2 ) ) {
			Q_strncpyz( versionStr, lua_tostring( L, 2 ), sizeof(versionStr) );
			Q_CleanString( versionStr, STRIP_COLOUR );
			semver_parse( versionStr, &ls.currentPlugin->version );
		}

		if ( lua_isstring( L, 3 ) ) {
			Q_strncpyz( versionStr, lua_tostring( L, 3 ), sizeof(versionStr) );
			Q_CleanString( versionStr, STRIP_COLOUR );
			semver_parse( versionStr, &ls.currentPlugin->requiredJPLuaVersion );
		}

		lua_newtable( L );
		int top = lua_gettop( L );

		lua_pushstring( L, "name" );
			lua_pushstring( L, ls.currentPlugin->longname );
			lua_settable( L, top );
		lua_pushstring( L, "version" );
		{
			lua_newtable( L );
			int top2 = lua_gettop( L );
			lua_pushstring( L, "major" );
				lua_pushinteger( L, ls.currentPlugin->version.major );
				lua_settable( L, top2 );
			lua_pushstring( L, "minor" );
				lua_pushinteger( L, ls.currentPlugin->version.minor );
				lua_settable( L, top2 );
			lua_pushstring( L, "patch" );
				lua_pushinteger( L, ls.currentPlugin->version.patch );
				lua_settable( L, top2 );

			lua_settable( L, top );
		}
		lua_pushstring( L, "dirname" );
			lua_pushstring( L, ls.currentPlugin->name );
			lua_settable( L, top );

		if ( semver_gt( ls.currentPlugin->requiredJPLuaVersion, jpluaVersion ) ) {
			luaO_pushfstring( L, S_COLOR_RED " %s requires JPLua v%d.%d.%d, you have JPLua v%d.%d.%d\n",
				ls.currentPlugin->name, ls.currentPlugin->requiredJPLuaVersion.major,
				ls.currentPlugin->requiredJPLuaVersion.minor, ls.currentPlugin->requiredJPLuaVersion.patch,
				jpluaVersion.major, jpluaVersion.minor, jpluaVersion.patch
			);
			luaD_throw( L, LUA_ERRRUN );
		}
		//save in the registry, but push on stack again straight away
		ls.currentPlugin->handle = luaL_ref( L, LUA_REGISTRYINDEX );
		lua_rawgeti( L, LUA_REGISTRYINDEX, ls.currentPlugin->handle );

		return 1;
	}

	bool EnablePlugin( plugin_t *plugin ) {
		if ( plugin->enabled ) {
			trap->Print( S_COLOR_YELLOW "plugin '%s' already loaded\n", plugin->name );
			return true;
		}

		// save the current plugin
		plugin_t *current = ls.currentPlugin;
		ls.currentPlugin = plugin;
		const char *err;
		if ( (err = LoadFile(ls.L, va("%s%s/%s", pluginDir, plugin->name, "plugin" JPLUA_EXTENSION) )) ){
			trap->Print(S_COLOR_RED "  failed %s " S_COLOR_GREEN "v%i.%i.%i " S_COLOR_WHITE " (%s):\n", plugin->longname,
				plugin->version.major, plugin->version.minor, plugin->version.patch, plugin->name
				);
			trap->Print(va("   %s\n",err));
			ls.currentPlugin = current;
			return false;
		}
		ls.currentPlugin = current;
		trap->Print( S_COLOR_CYAN "  loaded %s " S_COLOR_GREEN "v%i.%i.%i " S_COLOR_WHITE " (%s)\n", plugin->longname,
			plugin->version.major, plugin->version.minor, plugin->version.patch, plugin->name
		);
		plugin->enabled = true;

		return true;
	}

	void DisablePlugin( plugin_t *plugin ) {
		if ( !plugin->enabled ) {
			trap->Print( S_COLOR_YELLOW "plugin '%s' already unloaded\n", plugin->name );
		}

		// call the unload event
		if ( plugin->eventListeners[JPLUA_EVENT_UNLOAD] ) {
			// save the current plugin
			plugin_t *current = ls.currentPlugin;
			ls.currentPlugin = plugin;
			lua_rawgeti( ls.L, LUA_REGISTRYINDEX, plugin->eventListeners[JPLUA_EVENT_UNLOAD] );
			lua_pushboolean( ls.L, qfalse );
			Call( ls.L, 1, 0 );
			ls.currentPlugin = current;
		}
		std::vector<std::string> todelete; // TODO: Find another safe way to remove commands?
#ifdef PROJECT_GAME
		for (auto row : clientCommands){
			command_t* cmd = &row.second;
			if (cmd->owner == plugin){
				luaL_unref(ls.L, LUA_REGISTRYINDEX, cmd->handle);
				todelete.push_back(row.first);
				continue;
			}
		}
		for (auto row : todelete){ //messy
			clientCommands.erase(row);
		}
		todelete.clear();
#else
		for (auto row : consoleCommands){
			command_t* cmd = &row.second;
			if (cmd->owner == plugin){
				luaL_unref(ls.L, LUA_REGISTRYINDEX, cmd->handle);
				todelete.push_back(row.first);
				continue;
			}
		}
		for(auto row : todelete){ //messy
			consoleCommands.erase(row);
		}
		todelete.clear();
#endif
		for (auto row : serverCommands){
			command_t* cmd = &row.second;
			if (cmd->owner == plugin){
				luaL_unref(ls.L, LUA_REGISTRYINDEX, cmd->handle);
				todelete.push_back(row.first);
				continue;
			}
		}
		for (auto row : todelete){ //messy
			serverCommands.erase(row);
		}
		todelete.clear();

		luaL_unref( ls.L, LUA_REGISTRYINDEX, plugin->handle );
		trap->Print( S_COLOR_CYAN "  unloaded %s " S_COLOR_GREEN "v%i.%i.%i " S_COLOR_WHITE " (%s)\n", plugin->longname,
			plugin->version.major, plugin->version.minor, plugin->version.patch, plugin->name
		);
		plugin->enabled = false;
	}

	static void TrackPlugin( const char *pluginName, const char *fileName ) {
		if ( FindPlugin( pluginName ) ) {
			trap->Print( S_COLOR_YELLOW "  plugin %s already tracked\n", pluginName );
			return;
		}

		plugin_t *plugin = (plugin_t *)malloc( sizeof(plugin_t) );
		memset( plugin, 0, sizeof(plugin_t) );
		Q_strncpyz( plugin->longname, "<null>", sizeof(plugin->longname) );
		Q_strncpyz( plugin->name, pluginName, sizeof(plugin->name) );
		plugin->enabled = false;

		plugin->next = ls.plugins;
		if ( ls.currentPlugin == ls.plugins ) {
			ls.currentPlugin = plugin;
		}
		ls.plugins = plugin;
	}

	//FIXME: add UntrackPlugin again
#if 0
	static void UntrackPlugin( plugin_t *plugin ) {
		plugin_t *p = ls.plugins, *prev = nullptr;
		while ( p ) {
			if ( p == plugin ) {
				trap->Print( S_COLOR_CYAN "Untracking plugin " S_COLOR_YELLOW "%s (%s)\n", p->longname, p->name );
				if ( p->enabled && p->eventListeners[JPLUA_EVENT_UNLOAD] ) {
					//NOTE: this is necessary, for unusual reasons
					// a lua event may trigger the unload event of this plugin
					//	utility functions may then wish to know the currently-executing plugin
					// after the other plugin triggers the unload, any functions it executes afterward must still know
					//	the currently executing plugin, so we must preserve it during the unload event
					// the other reason to switch it back is that this plugin no longer exists
					// there is no concern in the case of no currently executing plugin
					plugin_t *oldPlugin = ls.currentPlugin;
					ls.currentPlugin = p;

					lua_rawgeti( ls.L, LUA_REGISTRYINDEX, p->eventListeners[JPLUA_EVENT_UNLOAD] );
					lua_pushboolean( ls.L, qfalse );
					Call( ls.L, 1, 0 );

					luaL_unref( ls.L, LUA_REGISTRYINDEX, p->handle );

					ls.currentPlugin = oldPlugin;
				}

				if ( ls.currentPlugin == p ) {
					ls.currentPlugin = p->next;
				}
				if ( ls.plugins == p ) {
					ls.plugins = p->next;
				}

				if ( prev ) {
					plugin_t *next = p->next;
					free( p );
					p = nullptr;
					prev->next = next;
				}
				else {
					free( p );
					p = nullptr;
				}
				break;
			}

			prev = p;
			p = p->next;
		}
		//TODO: make sure plugins and currentPlugin are still valid?
	}
#endif

	static int System_Index( lua_State *L ) {
		const char *key = lua_tostring( L, 2 );

		// see if this key is a function/constant in the metatable
		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );
		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		if ( !strcmp( key, "isClient" ) ) {
		#if defined(PROJECT_GAME)
			lua_pushboolean( L, 0 );
		#elif defined(PROJECT_CGAME)
			lua_pushboolean( L, 1 );
		#else
			lua_pushnil( L );
		#endif
		}
		else if ( !strcmp( key, "isServer" ) ) {
		#if defined(PROJECT_GAME)
			lua_pushboolean( L, 1 );
		#elif defined(PROJECT_CGAME)
			lua_pushboolean( L, 0 );
		#else
			lua_pushnil( L );
		#endif
		}
		else if ( !strcmp( key, "version" ) ) {
			lua_newtable( L );
			int top = lua_gettop( L );

			lua_pushstring( L, "major" );
				lua_pushinteger( L, jpluaVersion.major );
				lua_settable( L, top );
			lua_pushstring( L, "minor" );
				lua_pushinteger( L, jpluaVersion.minor );
				lua_settable( L, top );
			lua_pushstring( L, "patch" );
				lua_pushinteger( L, jpluaVersion.patch );
				lua_settable( L, top );
		}
		else if (!strcmp(key, "plugins")) {
			lua_newtable(L);
			int top = lua_gettop(L), top2, count = 1;
			plugin_t *plugin = NULL;
			char buf[16] = { '\0' };
			while (IteratePluginsTemp(&plugin, false)){
				lua_pushinteger(L, count++);
				lua_newtable(L);
				top2 = lua_gettop(L);
				lua_pushstring(L, "enabled"); lua_pushboolean(L, plugin->enabled); lua_settable(L, top2);
				lua_pushstring(L, "name"); lua_pushstring(L, plugin->name); lua_settable(L, top2);
				lua_pushstring(L, "longname"); lua_pushstring(L, plugin->longname); lua_settable(L, top2);
				semver_render(&plugin->requiredJPLuaVersion, buf);
				lua_pushstring(L, "requiredJPLuaversion"); lua_pushstring(L, buf); lua_settable(L, top2);
				semver_render(&plugin->version, buf);
				lua_pushstring(L, "requiredJPLuaversion"); lua_pushstring(L, buf); lua_settable(L, top2);

				lua_settable(L, top);
			}
		}
		else {
			lua_pushnil( L );
		}

		return 1;
	}
	static int System_NewIndex( lua_State *L ) {
		return luaL_error( L, "Attempt to modify read-only data (JPLua)" );
	}
	static const struct luaL_Reg systemMeta[] = {
		{ "__index", System_Index },
		{ "__newindex", System_NewIndex },
		{ NULL, NULL }
	};
	static void Register_System( lua_State *L ) {
		const luaL_Reg *r;

		// register the metatable
		luaL_newmetatable( L, "JPLua.Meta" );
		for ( r = systemMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}
		lua_pop( L, -1 );

		// create a global table named JPLua
		lua_newtable( L );
		luaL_getmetatable( L, "JPLua.Meta" );
		lua_setmetatable( L, -2 );
		lua_setglobal( L, "JPLua" );
	}

	static void LoadPluginDir( qboolean inPK3 ) {
		static char folderList[16384];
		char *folderName = folderList;
		memset( folderList, 0, sizeof(folderList) );

		int numFolders = trap->FS_GetFileList( pluginDir, inPK3 ? "" : "/", folderList, sizeof(folderList) );
		for ( int i = 0; i < numFolders; i++ ) {
			size_t skipLenFolder = inPK3 ? 1 : 0, folderLen = 0;
			qboolean skip = qfalse;
			char *s;
			plugin_t *plugin = NULL;

			if ( folderName[0] == '.' ) {
				skip = qtrue;
			}

			// check for loading the same plugin twice
			// this can happen when listing plugins outside of PK3s when plugins have written files using the Serialiser
			// use temp iterator because we don't want to modify any state
			while ( IteratePluginsTemp( &plugin ) ) {
				if ( !Q_stricmp( folderName, plugin->name ) ) {
					skip = qtrue;
				}
			}

			if ( (s = (char *)Q_strchrs( folderName, "/\\" )) ) {
				if ( !s[1] )
					skip = qtrue;
				*s = '\0';
				skipLenFolder = strlen( ++s ) + 1;
			}
			folderLen = strlen( folderName ) + 1;
			if ( !skip ) {
				static char fileList[16384];
				char *fileName = fileList;
				memset( fileList, 0, sizeof(fileList) );

				int numFiles = trap->FS_GetFileList( va( "%s%s", pluginDir, folderName ), JPLUA_EXTENSION, fileList, sizeof(fileList) );
				for ( int j = 0; j < numFiles; j++ ) {
					size_t skipLenFile = inPK3 ? 1 : 0, fileLen = 0;
					if ( (s = (char *)Q_strchrs( fileName, "/\\" )) ) {
						*s = '\0';
						skipLenFile = strlen( ++s ) + 1;
					}
					fileLen = strlen( fileName ) + 1;
					if ( !Q_stricmp( fileName, "plugin" JPLUA_EXTENSION ) ) {
						TrackPlugin( folderName, fileName );
						break;
					}
					fileName += fileLen + skipLenFile;
				}
			}
			folderName += folderLen + skipLenFolder;
		}
	}

	static void PostInit( lua_State *L ) {
		const char *err = NULL;
		Register_System( L );
		const char *fileName = va( "%sinit" JPLUA_EXTENSION, baseDir );
		if ( (err = LoadFile( L, fileName )) ) {
			trap->Print( S_COLOR_GREEN "JPLua:" S_COLOR_RED " Failed to load main scripts: %s\n", fileName );
			trap->Print( "   %s\n", err );
			Shutdown( qfalse );
			return;
		}
		ls.initialised = qtrue;

		//TODO: handle duplicates, i.e. one in pk3, one out of pk3
		LoadPluginDir( qtrue );
		LoadPluginDir( qfalse );

		plugin_t *plugin = nullptr;
		while ( IteratePlugins( &plugin, false ) ) {
			if (autoload.all){
				EnablePlugin( plugin );
				continue;
			}
			if ( autoload.plugins[plugin->name] ) {
				EnablePlugin( plugin );
			}
		}
	}

	#ifdef PROJECT_GAME
	static int Export_AddClientCommand( lua_State *L ) {
		StackCheck st( L );

		const char *name = luaL_checkstring( L, 1 );
		command_t &cmd = clientCommands[name];

		if ( cmd.handle ) {
			// already exists
			trap->Print( "JPlua: AddClientCommand(%s) failed, command already exists. Remove command first\n", name );
			return 0;
		}

		const int top = lua_gettop( L );
		int typeArg2 = lua_type( L, 2 );

		if ( top == 2 ) {
			if ( typeArg2 == LUA_TFUNCTION ) {
				cmd.handle = luaL_ref( L, LUA_REGISTRYINDEX );
				cmd.owner = ls.currentPlugin;
			}
			else {
				trap->Print( "JPLua AddClientCommand(%s) failed, function signature invalid. Is it up to date?\n", name );
				return 0;
			}
		}
		else {
			trap->Print( "JPLua AddClientCommand(%s) failed, too many arguments\n", name );
			return 0;
		}

		return 0;
	}

	#endif

	#ifdef PROJECT_CGAME
	static int Export_AddConsoleCommand( lua_State *L ) {
		StackCheck st( L );

		const char *name = luaL_checkstring( L, 1 );
		command_t &cmd = consoleCommands[name];
		if ( cmd.handle ) {
			// already exists
			trap->Print( "JPlua: AddConsoleCommand(%s) failed, command already exists. Remove command first\n", name );
			return 0;
		}

		const int top = lua_gettop( L );
		int typeArg2 = lua_type( L, 2 );
		if ( top == 1 || (top == 2 && typeArg2 == LUA_TNIL) ) {
			// add to autocomplete list
			trap->AddCommand( name );
		}
		else if ( top == 2 ) {
			if ( typeArg2 == LUA_TFUNCTION ) {
				trap->AddCommand( name );
				cmd.handle = luaL_ref( L, LUA_REGISTRYINDEX );
				cmd.owner = ls.currentPlugin;
			}
			else {
				trap->Print( "JPLua AddConsoleCommand(%s) failed, function signature invalid. Is it up to date?\n", name );
				return 0;
			}
		}
		else {
			trap->Print( "JPLua AddConsoleCommand(%s) failed, too many arguments\n", name );
			return 0;
		}

		return 0;
	}
	#endif

	static int Export_AddServerCommand( lua_State *L ) {
		StackCheck st( L );

		const char *name = luaL_checkstring( L, 1 );
		command_t &cmd = serverCommands[name];
		if ( cmd.handle ) {
			// already exists
			trap->Print( "JPlua: AddServerCommand(%s) failed, command already exists. Remove command first\n", name );
		}

		if ( lua_type( L, 2 ) != LUA_TFUNCTION ) {
			trap->Print( "JPLua AddServerCommand(%s) failed, function signature invalid. Is it up to date?\n", name );
			return 0;
		}

		cmd.handle = luaL_ref( L, LUA_REGISTRYINDEX );
		cmd.owner = ls.currentPlugin;

		return 0;
	}

	#ifdef PROJECT_CGAME
	static int Export_DrawPic( lua_State *L ) {
		const float x = luaL_checknumber( L, 1 );
		const float y = luaL_checknumber( L, 2 );
		const float w = luaL_checknumber( L, 3 );
		const float h = luaL_checknumber( L, 4 );

		vector4 colour;
		if ( lua_type( L, 5 ) == LUA_TTABLE ) {
			ReadFloats( colour.raw, 4, L, 5 );
		}
		else {
			VectorSet4( &colour, 1.0f, 1.0f, 1.0f, 1.0f );
		}

		qhandle_t shader = luaL_checkinteger( L, 6 );

		trap->R_SetColor( &colour );
			CG_DrawPic( x, y, w, h, shader );
		trap->R_SetColor( NULL );

		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_DrawRect( lua_State *L ) {
		vector4 colour = { 1.0f };

		ReadFloats( colour.raw, 4, L, 5 );

		CG_FillRect( (float)lua_tonumber( L, 1 ), (float)lua_tonumber( L, 2 ), (float)lua_tonumber( L, 3 ),
			(float)lua_tonumber( L, 4 ), &colour );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_DrawRotatedPic( lua_State *L ) {
		vector4 colour;
		int x, y, w, h, shader;
		float angle;

		x = lua_tointeger( L, 1 );
		y = lua_tointeger( L, 2 );
		w = lua_tointeger( L, 3 );
		h = lua_tointeger( L, 4 );
		angle = lua_tonumber( L, 5 );
		ReadFloats( colour.raw, 4, L, 6 );
		shader = lua_tointeger( L, 7 );

		trap->R_SetColor( &colour );
		CG_DrawRotatePic2( x, y, w, h, angle, shader );
		trap->R_SetColor( NULL );

		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_DrawText( lua_State *L ) {
		float x = luaL_checknumber( L, 1 );
		float y = luaL_checknumber( L, 2 );
		const char *text = luaL_checkstring( L, 3 );
		vector4 colour = { 1.0f };
		ReadFloats( colour.raw, 4, L, 4 );
		float scale = luaL_checknumber( L, 5 );
		int style = luaL_checkinteger( L, 6 );
		int iMenuFont = luaL_checkinteger( L, 7 );
		int customFont = luaL_checkinteger( L, 8 );

		const Font font( iMenuFont, scale, customFont );
		font.Paint( x, y, text, &colour, style );

		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_Font_StringHeightPixels( lua_State *L ) {
		const char *text = luaL_checkstring( L, 1 );
		float scale = luaL_checknumber( L, 2 );
		qhandle_t fontHandle = luaL_checkinteger( L, 3 );
		int customFont = 0;
		if ( lua_isboolean( L, 4 ) ) {
			customFont = lua_toboolean( L, 4 );
		}

		if ( !text ) {
			lua_pushnil( L );
		}
		else {
			const Font font( fontHandle, scale, customFont );
			lua_pushnumber( L, font.Height( text ) );
		}
		return 1;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_Font_StringLengthPixels( lua_State *L ) {
		const char *text = luaL_checkstring( L, 1 );
		float scale = luaL_checknumber( L, 2 );
		qhandle_t fontHandle = luaL_checkinteger( L, 3 );
		int customFont = 0;
		if ( lua_isboolean( L, 4 ) ) {
			customFont = lua_toboolean( L, 4 );
		}

		if ( !text ) {
			lua_pushnil( L );
		}
		else {
			const Font font( fontHandle, scale, customFont );
			lua_pushnumber( L, font.Width( text ) );
		}
		return 1;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_GetFPS( lua_State *L ) {
		lua_pushinteger( L, cg.japp.fps );
		return 1;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_GetKeyCatcher( lua_State *L ) {
		lua_pushinteger( L, trap->Key_GetCatcher() );
		return 1;
	}
	#endif

	static int Export_GetFlagStatus( lua_State *L ) {
	#if defined(PROJECT_GAME)
		//TODO: GetFlagStatus on server
		lua_pushnil( L );
		lua_pushnil( L );
	#elif defined(PROJECT_CGAME)
		lua_pushinteger( L, (lua_Integer)CG_GetFlagStatus( TEAM_RED ) );
		lua_pushinteger( L, (lua_Integer)CG_GetFlagStatus( TEAM_BLUE ) );
	#endif
		return 2;
	}

	static int Export_GetGametype( lua_State *L ) {
	#if defined(PROJECT_GAME)
		lua_pushinteger( L, level.gametype );
	#elif defined(PROJECT_CGAME)
		lua_pushinteger( L, cgs.gametype );
	#endif

		return 1;
	}

	static int Export_GetMap( lua_State *L ) {
	#if defined(PROJECT_GAME)
		char mapname[MAX_CVAR_VALUE_STRING];
		COM_StripExtension( level.rawmapname, mapname, sizeof(mapname) );
		lua_pushstring( L, mapname );
	#elif defined(PROJECT_CGAME)
		lua_pushstring( L, cgs.mapnameClean );
	#endif

		return 1;
	}

	static int Export_GetMapTime( lua_State *L ) {
	#if defined(PROJECT_GAME)

		int msec = level.time - level.startTime;
		int seconds = msec / 1000;
		int mins = seconds / 60;
		seconds %= 60;
		//	msec %= 1000;
		lua_pushstring( L, va( "%02i:%02i", mins, seconds ) );
		return 1;

	#elif defined(PROJECT_CGAME)

		int msec = 0, secs = 0, mins = 0, limitSec = cgs.timelimit * 60;

		msec = cg.time - cgs.levelStartTime;
		secs = msec / 1000;
		mins = secs / 60;

		if ( cgs.timelimit && (cg_drawTimer.bits & DRAWTIMER_COUNTDOWN) ) {// count down
			msec = limitSec * 1000 - (msec);
			secs = msec / 1000;
			mins = secs / 60;
		}

		secs %= 60;
		msec %= 1000;

		lua_pushinteger( L, mins );
		lua_pushinteger( L, secs );
		lua_pushinteger( L, msec );
		return 3;

	#endif
	}

	#ifdef PROJECT_CGAME
	static int Export_GetMousePos( lua_State *L ) {
		if ( trap->Key_GetCatcher() & KEYCATCH_CGAME ) {
			int top;
			lua_newtable( L );
			top = lua_gettop( L );
			lua_pushstring( L, "x" ); lua_pushinteger( L, cgs.cursorX ); lua_settop( L, top );
			lua_pushstring( L, "y" ); lua_pushinteger( L, cgs.cursorY ); lua_settop( L, top );
		}
		else
			lua_pushnil( L );

		return 1;
	}
	#endif

	#ifdef PROJECT_GAME
	//TODO: client-side version of this
	static int Export_GetPlayers( lua_State *L ) {
		int top, i = 1, clNum;
		gentity_t *ent;

		lua_newtable( L );
		top = lua_gettop( L );

		for ( ent = g_entities, clNum = 0; clNum < level.maxclients; ent++, clNum++ ) {
			if ( !ent->inuse || ent->client->pers.connected == CON_DISCONNECTED )
				continue;
			lua_pushnumber( L, i++ );
			Player_CreateRef( L, clNum );
			lua_settable( L, top );
		}

		return 1;
	}
	#endif

	static int Export_GetRealTime( lua_State *L ) {
		lua_pushinteger( L, trap->Milliseconds() );
		return 1;
	}

#if defined(PROJECT_CGAME)
	static int Export_GetScores( lua_State *L ) {
		lua_pushinteger( L, cgs.scores1 );
		lua_pushinteger( L, cgs.scores2 );
		return 2;
	}
#endif

	static int Export_GetTime( lua_State *L ) {
	#if defined(PROJECT_GAME)
		lua_pushinteger( L, level.time );
	#elif defined(PROJECT_CGAME)
		lua_pushinteger( L, cg.time );
	#endif
		return 1;
	}

	int Export_Trace( lua_State *L ) {
		trace_t tr;
		vector3 *start, *end, mins, maxs;
		float size;
		int skipNumber, mask;
		int top, top2;

		start = CheckVector(L, 1);

		size = lua_tonumber( L, 2 ) / 2.0f;
		VectorSet( &mins, size, size, size );
		VectorScale( &mins, -1.0f, &maxs );

		end = CheckVector(L, 3);

		skipNumber = lua_tointeger( L, 4 );
		mask = lua_tointeger( L, 5 );

	#if defined(PROJECT_GAME)
		trap->Trace( &tr, start, &mins, &maxs, end, skipNumber, mask, qfalse, 0, 0 );
	#elif defined(PROJECT_CGAME)
		CG_Trace( &tr, start, &mins, &maxs, end, skipNumber, mask );
	#endif

		lua_newtable( L );
		top = lua_gettop( L );
		lua_pushstring( L, "allsolid" ); lua_pushboolean( L, !!tr.allsolid ); lua_settable( L, top );
		lua_pushstring( L, "startsolid" ); lua_pushboolean( L, !!tr.startsolid ); lua_settable( L, top );
		lua_pushstring( L, "entityNum" ); lua_pushinteger( L, tr.entityNum ); lua_settable( L, top );
		lua_pushstring( L, "fraction" ); lua_pushnumber( L, tr.fraction ); lua_settable( L, top );

		lua_pushstring( L, "endpos" );Vector_CreateRef(L, &tr.endpos);lua_settable( L, top );

		lua_pushstring( L, "plane" );lua_newtable( L ); top2 = lua_gettop( L );
			lua_pushstring( L, "normal" );Vector_CreateRef(L, &tr.plane.normal);lua_settable( L, top2 );
			lua_pushstring( L, "dist" ); lua_pushnumber( L, tr.plane.dist ); lua_settable( L, top2 );
			lua_pushstring( L, "type" ); lua_pushinteger( L, tr.plane.type ); lua_settable( L, top2 );
			lua_pushstring( L, "signbits" ); lua_pushinteger( L, tr.plane.signbits ); lua_settable( L, top2 );
		lua_settable( L, top );

		lua_pushstring( L, "surfaceFlags" ); lua_pushinteger( L, tr.surfaceFlags ); lua_settable( L, top );
		lua_pushstring( L, "contents" ); lua_pushinteger( L, tr.contents ); lua_settable( L, top );
		return 1;
	}

	#ifdef PROJECT_CGAME
	static int Export_RegisterShader( lua_State *L ) {
		const char *shaderName = luaL_checkstring( L, 1 );
		const bool noMip = lua_toboolean( L, 2 );

		qhandle_t handle = NULL_HANDLE;
		if ( noMip ) {
			handle = trap->R_RegisterShaderNoMip( shaderName );
		}
		else {
			handle = trap->R_RegisterShader( shaderName );
		}

		lua_pushinteger( L, handle );

		return 1;
	}
	#endif

	static int Export_RegisterSound( lua_State *L ) {
#ifdef PROJECT_CGAME
		lua_pushinteger( L, trap->S_RegisterSound( lua_tostring( L, 1 ) ) );
#elif defined PROJECT_GAME
		lua_pushinteger(L, G_SoundIndex( lua_tostring( L, 1 ) ) );
#endif
		return 1;
	}

	#ifdef PROJECT_CGAME
	static int Export_RemapShader( lua_State *L ) {
		trap->R_RemapShader( lua_tostring( L, 1 ), lua_tostring( L, 2 ), lua_tostring( L, 3 ) );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_SendChatText( lua_State *L ) {
		char text[MAX_SAY_TEXT] = { 0 };

		if ( !cg_teamChatsOnly.integer ) {
			Q_strncpyz( text, lua_tostring( L, -1 ), MAX_SAY_TEXT );
			lua_pop( L, 1 );
			CG_LogPrintf( cg.log.console, va( "%s\n", text ) );
			if ( cg_newChatbox.integer )
				CG_ChatboxAddMessage( text, qfalse, "normal" );
			else
				CG_ChatBox_AddString( text );
		}

		return 0;
	}
	#endif

	static int Export_SendConsoleCommand( lua_State *L ) {
	#if defined(PROJECT_GAME)
		int target = luaL_checkinteger( L, 1 );
		const char *cmd = luaL_checkstring( L, 2 );
		trap->SendConsoleCommand( target, va( "%s\n", cmd ) );
	#elif defined(PROJECT_CGAME)
		const char *cmd = luaL_checkstring( L, 1 );
		trap->SendConsoleCommand( va( "%s\n", cmd ) );
	#endif
		return 0;
	}

	#ifdef PROJECT_GAME
	static int Export_SendReliableCommand( lua_State *L ) {
		trap->SendServerCommand( lua_tointeger( L, 1 ), lua_tostring( L, 2 ) );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_SendServerCommand( lua_State *L ) {
		trap->SendClientCommand( lua_tostring( L, 1 ) );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_SendNotification( lua_State *L ) {
		const char *title = luaL_checkstring( L, 1 );
		const char *msg = luaL_checkstring( L, 2 );
		const uint32_t timeout = luaL_checkinteger( L, 3 );
		const char *iconName = luaL_checkstring( L, 4 );

		bool success = CG_NotifySend( title, msg, timeout, iconName );
		lua_pushboolean( L, success );
		return 1;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_SetKeyCatcher( lua_State *L ) {
		uint32_t catcherMask = (uint32_t)lua_tointeger( L, 1 );
		trap->Key_SetCatcher( catcherMask );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_SetMousePos( lua_State *L ) {
		cgs.cursorX = lua_tointeger( L, 1 );
		cgs.cursorY = lua_tointeger( L, 2 );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_StartLocalSound( lua_State *L ) {
		trap->S_StartLocalSound( lua_tonumber( L, 1 ), lua_tonumber( L, 2 ) );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	int Export_TestLine( lua_State *L ) {
		vector3 start, end;
		float radius;
		int time;
		uint32_t color;

		lua_getfield( L, 1, "x" ); start.x = lua_tonumber( L, -1 );
		lua_getfield( L, 1, "y" ); start.y = lua_tonumber( L, -1 );
		lua_getfield( L, 1, "z" ); start.z = lua_tonumber( L, -1 );

		time = lua_tonumber( L, 3 );
		color = lua_tonumber( L, 4 );
		radius = lua_tonumber( L, 5 );
		lua_getfield( L, 2, "x" ); end.x = lua_tonumber( L, -1 );
		lua_getfield( L, 2, "y" ); end.y = lua_tonumber( L, -1 );
		lua_getfield( L, 2, "z" ); end.z = lua_tonumber( L, -1 );

		CG_TestLine( &start, &end, time, color, radius );

		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int Export_WorldCoordToScreenCoord( lua_State *L ) {
		vector3 *v = CheckVector( L, 1 );
		float x, y;
		if ( CG_WorldCoordToScreenCoordFloat( v, &x, &y ) ) {
			lua_pushnumber( L, x );
			lua_pushnumber( L, y );
			return 2;
		}
		else {
			lua_pushnil( L );
			return 1;
		}
	}
	#endif

	#ifdef PROJECT_GAME
	static int ConnectToDB( lua_State *L ) {
		StackCheck st( L );

	#if defined(NO_SQL)
		lua_pushnil( L );
		return 1;
	#else
		int type = luaL_checkinteger( L, 1 );
		switch ( type ) {
		case 1: ///MySQL
			return MySQL_Open( L );
		case 2: ///SQLite
			return SQLite_Open( L );
		default:
			lua_pushnil( L );
			return 1;
		}
	#endif
	}
	#endif
	static int GetConfigString( lua_State *L ) {
		int type = luaL_checkinteger( L, 1 );

	#ifdef PROJECT_CGAME
		const char *info = CG_ConfigString( type );
	#elif defined (PROJECT_GAME)
		char info[MAX_INFO_STRING] = {};
		trap->GetConfigstring( type, info, sizeof(info) );
	#endif

		if ( !info[0] ) {
			lua_pushnil( L );
			return 1;
		}

		PushInfostring( L, info );
		return 1;
	}

	#ifdef PROJECT_GAME
	static int SetConfigString( lua_State *L ) {
		int type = luaL_checkinteger( L, 1 );
		const char *value = luaL_checkstring( L, 2 );
		trap->SetConfigstring( type, value );
		return 0;
	}
	#endif

	#ifdef PROJECT_CGAME
	static int GetGLConfig( lua_State *L ) {
		glconfig_t config;
		trap->GetGlconfig( &config );

		lua_newtable( L );
		int top = lua_gettop( L );
		lua_pushstring( L, "renderer" );
			lua_pushstring( L, config.renderer_string );
			lua_settable( L, top );
		lua_pushstring( L, "vendor" );
			lua_pushstring( L, config.vendor_string );
			lua_settable( L, top );
		lua_pushstring( L, "version" );
			lua_pushstring( L, config.version_string );
			lua_settable( L, top );
		lua_pushstring( L, "extensions" );
			lua_pushstring( L, config.extensions_string );
			lua_settable( L, top );
		lua_pushstring( L, "colorbits" );
			lua_pushinteger( L, config.colorBits );
			lua_settable( L, top );
		lua_pushstring( L, "depthbits" );
			lua_pushinteger( L, config.depthBits );
			lua_settable( L, top );
		lua_pushstring( L, "stencilBits" );
			lua_pushinteger( L, config.stencilBits );
			lua_settable( L, top );
		lua_pushstring( L, "width" );
			lua_pushinteger( L, config.vidWidth );
			lua_settable( L, top );
		lua_pushstring( L, "height" );
			lua_pushinteger( L, config.vidHeight );
			lua_settable( L, top );
		lua_pushstring( L, "frequency" );
			lua_pushinteger( L, config.displayFrequency );
			lua_settable( L, top );

		lua_pushstring( L, "fullscreen" );
			lua_pushboolean( L, config.isFullscreen );
			lua_settable( L, top );

		return 1;
	}
	#endif

	#ifdef PROJECT_GAME
	static int GetModelBounds( lua_State *L ){
		const char *name = luaL_checkstring( L, 1 );

		vector3 mins, maxs;
		G_GetModelBounds( name, &mins, &maxs );
		Vector_CreateRef( L, mins.x, mins.y, mins.z );
		Vector_CreateRef( L, maxs.x, maxs.y, maxs.z );

		return 2;
	}

	static int EntitiesInRadius( lua_State *L ){
		vector3 *origin = CheckVector( L, 1 );
		float radius = luaL_checknumber( L, 2 );
		gentity_t	*entity_list[MAX_GENTITIES];

		int count =	G_RadiusList( origin, radius, NULL, qtrue, entity_list );

		lua_newtable( L );
		int top = lua_gettop( L );

		for ( int i = 0; i < count; i++ ) {
			lua_pushinteger( L, i + 1 );
			Entity_CreateRef( L, entity_list[i] );
			lua_settable( L, top );
		}
		return 1;
	}

	static int EntitiesInBox( lua_State *L ) {
		vector3 *mins = CheckVector( L, 1 );
		vector3 *maxs = CheckVector( L, 2 );
		int entityList[MAX_GENTITIES], numListedEntities;
		numListedEntities = trap->EntitiesInBox(mins, maxs, entityList, MAX_GENTITIES);

		lua_newtable( L );
		int top = lua_gettop( L );

		for ( int i = 0; i < numListedEntities; i++ ) {
			gentity_t *ent = &g_entities[entityList[i]];
			lua_pushinteger( L, i + 1 );
			Entity_CreateRef( L, ent );
			lua_settable( L, top );
		}
		return 1;
	}

	//TODO: client-side version
	static int ScreenShake( lua_State *L ) {
		vector3 *origin = CheckVector( L, 1 );
		float intense = luaL_checknumber( L, 2 );
		int duration = luaL_checkinteger( L, 3 );
		qboolean global = lua_toboolean( L, 4 );

		G_ScreenShake( origin, NULL, intense, duration, global );
		return 0;
	}
	#endif


	#ifdef PROJECT_CGAME
	static int Export_IsKeyDown( lua_State *L ){
		int key = luaL_checkinteger( L, 1 );
		lua_pushboolean(L, trap->Key_IsDown( key ) );
		return 1;
	}

	#endif

	#ifdef PROJECT_CGAME
	static int Export_OpenURL( lua_State *L ){
		const char *url = luaL_checkstring(L, 1);
		Q_OpenURL(url);
		return 0;
	}
	#endif

#ifdef PROJECT_GAME
	static int TeamLock( lua_State *L ) {
		int team = luaL_checkinteger( L, 1 );
		luaL_checktype( L, 2, LUA_TBOOLEAN );
		qboolean enabled = lua_toboolean( L, 2 );

		if ( team >= TEAM_FREE && team < TEAM_NUM_TEAMS ) {
			level.lockedTeams[team] = enabled;
		}
		else {
			return luaL_error( L, "team ID %i not in range [%i,%i]", team, TEAM_FREE, TEAM_NUM_TEAMS - 1 );
		}

		return 0;
	}

#endif

	static const importTable_t imports[] = {
	#ifdef PROJECT_GAME
		{ "AddClientCommand", Export_AddClientCommand }, // AddClientCommand( string cmd )
	#endif
	#ifdef PROJECT_CGAME
		{ "AddConsoleCommand", Export_AddConsoleCommand }, // AddConsoleCommand( string cmd )
	#endif
		{ "AddListener", Event_AddListener }, // AddListener( string name, function listener )
		{ "AddServerCommand", Export_AddServerCommand }, // AddServerCommand( string cmd )
	#ifdef PROJECT_GAME
		{ "ConnectToDB", ConnectToDB }, // ConnectToDB ( int type(1 - MySQL , 2 - SQLite), ...) // SQLite (type, string path) || MySQL ( type, string host, string user, string db, string password, int port )
	#endif
		{ "CreateCvar", CreateCvar }, // Cvar CreateCvar( string name [, string value [, integer flags] ] )
	#ifdef PROJECT_GAME
		{ "CreateEntity", Entity_Create },
	#endif
	#ifdef PROJECT_CGAME
		{ "CreateMenu", Interface_CreateMenu },
		{ "CreateLocalEntity", LocalEntity_Create },
		{ "CreateRefEntity", RefEntity_Create },
		{ "DrawPic", Export_DrawPic }, // DrawPic( float x, float y, float width, float height, table { float r, float g, float b, float a }, integer shaderHandle )
		{ "DrawRect", Export_DrawRect }, // DrawRect( float x, float y, float width, float height, table { float r, float g, float b, float a } )
		{ "DrawRotatedPic", Export_DrawRotatedPic }, // DrawPic( float x, float y, float width, float height, float angle, table { float r, float g, float b, float a }, integer shaderHandle )
		{ "DrawText", Export_DrawText }, // DrawText( float x, float y, string text, table { float r, float g, float b, float a }, float scale, integer fontStyle, integer fontIndex )
		{ "Font_StringHeightPixels", Export_Font_StringHeightPixels }, // integer Font_StringHeightPixels( integer fontHandle, float scale )
		{ "Font_StringLengthPixels", Export_Font_StringLengthPixels }, // integer Font_StringLengthPixels( string str, integer fontHandle, float scale )
	#endif
	#ifdef PROJECT_GAME
		{ "EntitiesInBox", EntitiesInBox },
		{ "EntitiesInRadius", EntitiesInRadius },
		{ "FindEntityByClassname", FindEntityByClassName },
	#endif
		{ "GetEntity", Entity_Get }, // GetEntity(num) or GetEntity() for full list
		{ "GetEntityTable", Entity_GetMetaTable }, // Entity.meta GetEntityTable()
		{ "GetFileList", File_GetFileList}, // table GetFileList(string path, string extension)
	#ifdef PROJECT_CGAME
		{ "GetFPS", Export_GetFPS }, // integer GetFPS()
	#endif
		{ "GetConfigString", GetConfigString }, // table GetConfigString()
		{ "GetCvar", GetCvar }, // Cvar GetCvar( string name )
		{ "GetFlagStatus", Export_GetFlagStatus }, // FlagStatus GetFlagStatus( integer team )
		{ "GetGametype", Export_GetGametype }, // integer GetGametype()
	#ifdef PROJECT_CGAME
		{ "GetGLConfig", GetGLConfig }, // table GetGLConfig()
		{ "GetKeyCatcher", Export_GetKeyCatcher }, // integer GetKeyCatcher()
	#endif
		{ "GetLogger", GetLogger }, // Logger GetLogger( string filename )
		{ "GetMap", Export_GetMap }, // string GetMap()
		{ "GetMapTime", Export_GetMapTime }, // string GetMapTime()
	#ifdef PROJECT_CGAME
		{ "GetMenu", Interface_GetMenu },
	#endif
	#ifdef PROJECT_GAME
		{ "GetModelBounds", GetModelBounds },
	#endif
	#ifdef PROJECT_CGAME
		{ "GetMousePos", Export_GetMousePos }, // [{x,y}, nil] GetMousePos
	#endif
		{ "GetPlayer", GetPlayer }, // Player GetPlayer( integer clientNum )
	#ifdef PROJECT_GAME
		{ "GetPlayers", Export_GetPlayers }, // {Player,...} GetPlayers()
	#endif
		{ "GetPlayerTable", Player_GetMetaTable }, // Player.meta GetPlayerTable()
		{ "GetRealTime", Export_GetRealTime }, // integer GetRealTime()
		{ "GetSerialiser", GetSerialiser }, // Serialiser GetSerialiser( string fileName )
	#ifdef PROJECT_CGAME
		{ "GetServer", GetServer }, // Server GetServer()
		{ "GetScores", Export_GetScores }, // integer, integer GetScores()
	#endif
		{ "GetTime", Export_GetTime }, // integer GetTime()
	#ifdef PROJECT_CGAME

		{ "IsKeyDown", Export_IsKeyDown }, // boolean IsKeyDown( integer )
	#endif
	#ifdef PROJECT_GAME
		{ "TeamLock", TeamLock},
	#endif
		{ "OpenFile", File_Open},
#ifdef PROJECT_CGAME
		{ "OpenURL", Export_OpenURL},
#endif
		{ "RayTrace", Export_Trace }, // traceResult Trace( stuff )
	#ifdef PROJECT_CGAME
		{ "RegisterFont", RegisterFont }, // RegisterFont( string name)
		{ "RegisterShader", Export_RegisterShader }, // integer RegisterShader( string path )
#endif
		{ "RegisterSound", Export_RegisterSound }, // integer RegisterSound( string path )
		{ "RegisterPlugin", RegisterPlugin }, // plugin RegisterPlugin( string name, string version )
	#ifdef PROJECT_CGAME
		{ "RemapShader", Export_RemapShader }, // RemapShader( string oldshader, string newshader, string timeoffset )
	#endif
		{ "RemoveListener", Event_RemoveListener }, // RemoveListener( string name )
	#ifdef PROJECT_GAME
		{ "ScreenShake", ScreenShake },
	#endif
	#ifdef PROJECT_CGAME
		{ "SendChatText", Export_SendChatText }, // SendChatText( string text )
	#endif
		{ "SendConsoleCommand", Export_SendConsoleCommand }, // SendConsoleCommand( string command )
	#ifdef PROJECT_CGAME
		{ "SendNotification", Export_SendNotification }, // SendNotification( string title, string message, integer timeout, string iconName )
		{ "SendServerCommand", Export_SendServerCommand }, // SendServerCommand( string command )
	#endif
	#ifdef PROJECT_GAME
		{ "SendReliableCommand", Export_SendReliableCommand }, // SendReliableCommand( integer clientNum, string cmd )
		{ "SetConfigString", SetConfigString}, // SetConfigString(integer type ( CS_SYSTEMINFO + i ) , value
	#endif
	#ifdef PROJECT_CGAME
		{ "SetKeyCatcher", Export_SetKeyCatcher }, // SetKeyCatcher( integer catcherMask )
		{ "SetMousePos", Export_SetMousePos }, // SetMousePos( integer x, integer y )
	#endif
	#ifdef PROJECT_GAME
		{ "SetWeaponAltFireFunc", Weapon_SetAltFireFunction },
		{ "SetWeaponFireFunc", Weapon_SetFireFunction },
	#endif
		{ "StackDump", StackDump },
	#ifdef PROJECT_CGAME
		{ "StartLocalSound", Export_StartLocalSound }, // StartLocalSound( integer soundHandle, integer channelNum )
		{ "TestLine", Export_TestLine }, // traceResult Trace( stuff )
		{ "TextBox", CreateTextBox }, // TextBox TextBox( integer fontIndex )
	#endif
		{ "Vector3", GetVector3 }, // Vector Vector3( [float x, float y, float z] )
	#ifdef PROJECT_CGAME
		{ "WorldCoordToScreenCoord", Export_WorldCoordToScreenCoord }, // { float x, float y } WorldCoordToScreenCoord( Vector3 pos )
	#endif
	};
	static const size_t cimportsSize = ARRAY_LEN( imports );

	#endif // JPLUA

	// initialise the JPLua system
	void Init( void ) {
	#ifdef JPLUA
	#if defined(PROJECT_GAME)
		if ( !g_jplua.integer ) {
	#elif defined(PROJECT_CGAME)
		if ( !cg_jplua.integer ) {
	#endif
			return;
		}
		//Initialise and load base libraries
		memset( framework, -1, sizeof(framework) );
		ls.L = luaL_newstate();
		if ( !ls.L ) {
			//TODO: Fail gracefully
			return;
		}

		// set the JPLua version
		semver_parse( "13.6.1", &jpluaVersion );

		// set the callback in case of an error
		lua_atpanic( ls.L, Error );
		luaL_openlibs( ls.L );
		luaopen_string( ls.L );

		// get rid of libraries we don't need
		// no need for the package library
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, LUA_LOADLIBNAME );

		// we use JKA engine facilities for file-handling
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, LUA_IOLIBNAME );

		// there are some things in the base library that we don't want
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, "dofile" );
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, "loadfile" );
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, "load" );
		lua_pushnil( ls.L );
			lua_setglobal( ls.L, "loadstring" );

		// some libraries we want, but not certain elements in them
		// the OS library has dangerous access to the system, remove some parts of it
		lua_getglobal( ls.L, LUA_OSLIBNAME );
			lua_pushstring( ls.L, "execute" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
			lua_pushstring( ls.L, "exit" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
			lua_pushstring( ls.L, "remove" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
			lua_pushstring( ls.L, "rename" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
			lua_pushstring( ls.L, "setlocale" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
			lua_pushstring( ls.L, "tmpname" );
				lua_pushnil( ls.L );
				lua_settable( ls.L, -3 );
		lua_pop( ls.L, 1 );

		// redefine global functions
		lua_pushcclosure( ls.L, Export_Print, 0 );
			lua_setglobal( ls.L, "print" );
		lua_pushcclosure( ls.L, Export_Require, 0 );
			lua_setglobal( ls.L, "require" );

		for ( int i = 0; i < cimportsSize; i++ ) {
			lua_register( ls.L, imports[i].name, imports[i].function );
		}

		// register our classes
		Register_Player( ls.L );
		Register_Entity( ls.L );
	#ifdef PROJECT_CGAME
		Register_Server( ls.L );
		Register_Menu( ls.L );
		Register_Item( ls.L );
		Register_RefEntity( ls.L );
		Register_LocalEntity( ls.L );
		Register_Font( ls.L );
		Register_TextBox( ls.L );
	#endif
		Register_Cvar( ls.L );
		Register_Logger( ls.L );
		Register_Serialiser( ls.L );
		Register_Vector( ls.L );
		Register_File( ls.L );
	#ifdef PROJECT_GAME
		#if !defined(NO_SQL)
			Register_MySQL( ls.L );
			Register_SQLite( ls.L );
		#endif
	#endif

		// framework initialisation
		lua_getglobal( ls.L, "tostring" );
		framework[JPLUA_FRAMEWORK_TOSTRING] = luaL_ref( ls.L, LUA_REGISTRYINDEX );
		lua_getglobal( ls.L, "pairs" );
		framework[JPLUA_FRAMEWORK_PAIRS] = luaL_ref( ls.L, LUA_REGISTRYINDEX );

		for ( int i = 0; i < JPLUA_FRAMEWORK_MAX; i++ ) {
			if ( framework[i] < 0 ) {
				Com_Error( ERR_FATAL, "FATAL ERROR: Could not properly initialize the JPLua framework!\n" );
			}
		}

		// call our base scripts
		PostInit( ls.L );
	#endif
	}

	bool IsInitialised( void ) {
		return ls.L != nullptr;
	}

	void Shutdown( qboolean restart ) {
	#ifdef JPLUA
		if ( ls.L ) {
			plugin_t *nextPlugin = ls.plugins;

			Event_Shutdown( restart );
#ifdef PROJECT_GAME
			for ( auto &ent : g_entities ) {
				ent.uselua = false;
			}
#endif

			ls.currentPlugin = ls.plugins;
			while ( nextPlugin ) {
				for ( int i = JPLUA_EVENT_UNLOAD; i < JPLUA_EVENT_MAX; i++ ) {
					if ( ls.currentPlugin->eventListeners[i] ) {
						luaL_unref( ls.L, LUA_REGISTRYINDEX, ls.currentPlugin->eventListeners[i] );
					}
				}
				luaL_unref( ls.L, LUA_REGISTRYINDEX, ls.currentPlugin->handle );
				nextPlugin = ls.currentPlugin->next;

				free( ls.currentPlugin );
				ls.currentPlugin = nextPlugin;
			}

	#ifdef PROJECT_GAME
			for ( auto &row : clientCommands ) {
				luaL_unref( ls.L, LUA_REGISTRYINDEX, row.second.handle );
			}
			clientCommands.clear();
			weaponCallbacks.clear();
	#elif defined PROJECT_CGAME
			for ( auto& row : consoleCommands ) {
				luaL_unref( ls.L, LUA_REGISTRYINDEX, row.second.handle );
			}
			consoleCommands.clear();
	#endif
			for ( auto &row : serverCommands ) {
				luaL_unref( ls.L, LUA_REGISTRYINDEX, row.second.handle );
			}
			serverCommands.clear();

			ls.plugins = ls.currentPlugin = NULL;

			lua_close( ls.L );
			ls.L = NULL;
			ls.initialised = qfalse;
		}
	#endif
	}

} // namespace JPLua
