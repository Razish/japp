#include "g_local.h"
#include "g_lua.h"

#ifdef JPLUA

static const char PLAYER_META[] = "Player.meta";

//Func: GetPlayer( clientNum )
//Retn: Player object
int JPLua_GetPlayer( lua_State *L ) {
	int clientNum;

	if ( lua_type( L, 1 ) == LUA_TNUMBER )
		clientNum = lua_tointeger( L, 1 );

	else if ( lua_type( L, 1 ) == LUA_TSTRING ) {
		const char *name = lua_tostring( L, 1 );
		clientNum = G_ClientFromString( NULL, name, FINDCL_SUBSTR );
		if ( clientNum == -1 ) {
			lua_pushnil( L );
			return 1;
		}
	}

	else {
		lua_pushnil( L );
		return 1;
	}

	JPLua_Player_CreateRef( L, clientNum );
	return 1;
}

//Func: tostring( Player )
//Retn: string representing the Player instance (for debug/error messages)
static int JPLua_Player_ToString( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushfstring( L, "Player(%d)", player->clientNum );
	return 1;
}

//Func: Player:GetID()
//Retn: integer of the client's ID
static int JPLua_Player_GetID( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, player->clientNum );
	return 1;
}

//Func: Player:GetName()
//Retn: string of the Player's name
static int JPLua_Player_GetName( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushstring( L, level.clients[player->clientNum].pers.netname );
	return 1;
}

//Func: Player:GetHealth()
//Retn: integer amount of the Player's health
static int JPLua_Player_GetHealth( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.stats[STAT_HEALTH] );
	return 1;
}

//Func: Player:GetArmor()
//Retn: integer amount of the Player's armor
static int JPLua_Player_GetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.stats[STAT_ARMOR] );
	return 1;
}

//Func: Player:GetForce()
//Retn: integer amount of Player's force mana points
static int JPLua_Player_GetForce( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.fd.forcePower );
	return 1;
}

//Func: Player:GetWeapon()
//Retn: integer index of the weapon the Player has selected (See: 'Weapons' table)
static int JPLua_Player_GetWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.weapon );
	return 1;
}

//Func: Player:GetAmmo([weapon])
//Retn: integer amount of the ammo for either the specified weapon, or the weapon the player has selected
static int JPLua_Player_GetAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int weapon = level.clients[player->clientNum].ps.weapon;

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

	lua_pushinteger( L, level.clients[player->clientNum].ps.ammo[weaponData[weapon].ammoIndex] );
	return 1;
}

//Func: Player:GetMaxAmmo([weapon])
//Retn: integer amount of the max ammo for either the specified weapon, or the weapon the player has selected
static int JPLua_Player_GetMaxAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int weapon = level.clients[player->clientNum].ps.weapon;

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

	lua_pushinteger( L, ammoData[weaponData[weapon].ammoIndex].max );
	return 1;
}

//Func: Player:GetSaberStyle()
//Retn: integer index of the saber style the Player is using (See: 'SaberStyle' table)
static int JPLua_Player_GetSaberStyle( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.fd.saberDrawAnimLevel );
	return 1;
}

//Func: Player:GetEFlags()
//Retn: bit-mask of various entity flags (See: 'EFlags' table)
static int JPLua_Player_GetEFlags( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.eFlags );
	return 1;
}

//Func: Player:GetEFlags2()
//Retn: bit-mask of various entity flags (See: 'EFlags2' table)
static int JPLua_Player_GetEFlags2( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.eFlags2 );
	return 1;
}

//Func: Player:GetDuelingPartner()
//Retn: nil if Player is not dueling
//		Player object of the client Player is dueling
static int JPLua_Player_GetDuelingPartner( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( !level.clients[player->clientNum].ps.duelInProgress )
		lua_pushnil( L );
	else
		JPLua_Player_CreateRef( L, level.clients[player->clientNum].ps.duelIndex );

	return 1;
}

//Func: Player:GetLocation()
//Retn: string of the nearest target_location
extern const char *CG_GetLocationString( const char *loc ); // cg_main.c
static int JPLua_Player_GetLocation( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	char location[64];
	if ( Team_GetLocationMsg( &g_entities[player->clientNum], location, sizeof( location ) ) )
		lua_pushstring( L, location );
	else
		lua_pushnil( L );

	return 1;
}

//Func: Player:GetAnimations()
//Retn: Table of Legs/Torso anims and Legs/Torso timers
static int JPLua_Player_GetAnimations( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "torsoAnim" ); lua_pushnumber( L, level.clients[player->clientNum].ps.torsoAnim ); lua_settable( L, top );
	lua_pushstring( L, "torsoTimer" ); lua_pushnumber( L, level.clients[player->clientNum].ps.torsoTimer ); lua_settable( L, top );
	lua_pushstring( L, "legsAnim" ); lua_pushnumber( L, level.clients[player->clientNum].ps.legsAnim ); lua_settable( L, top );
	lua_pushstring( L, "legsTimer" ); lua_pushnumber( L, level.clients[player->clientNum].ps.legsTimer ); lua_settable( L, top );
	return 1;
}

//Func: Player:GetPosition()
//Retn: Table of x/y/z position
static int JPLua_Player_GetPosition( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, level.clients[player->clientNum].ps.origin.x ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, level.clients[player->clientNum].ps.origin.y ); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, level.clients[player->clientNum].ps.origin.z ); lua_settable( L, top );
	return 1;
}

//Func: Player:GetAngles()
//Retn: Table of pitch/yaw/roll view angles
static int JPLua_Player_GetAngles( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.pitch ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.yaw); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.roll ); lua_settable( L, top );
	return 1;
}

//Func: Player:GetVelocity()
//Retn: Table of x/y/z velocity
static int JPLua_Player_GetVelocity( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, level.clients[player->clientNum].ps.velocity.x ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, level.clients[player->clientNum].ps.velocity.y ); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, level.clients[player->clientNum].ps.velocity.z ); lua_settable( L, top );
	return 1;
}

//Func: Player:IsWeaponHolstered()
//Retn: integer expressing the holstered state of the saber
//		0 - all applicable sabers are activated
//		1 - using dual/staff and only one blade is activated
//		2 - both blades are off
static int JPLua_Player_IsWeaponHolstered( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, g_entities[player->clientNum].s.saberHolstered );
	return 1;
}

//Func: Player:IsAlive()
//Retn: boolean expressing whether the Player is alive
static int JPLua_Player_IsAlive( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( level.clients[player->clientNum].ps.stats[STAT_HEALTH] > 0 && !(level.clients[player->clientNum].ps.eFlags & EF_DEAD)
		&& level.clients[player->clientNum].ps.persistant[PERS_TEAM] != TEAM_SPECTATOR )
	{
		lua_pushboolean( L, 1 );
		return 1;
	}

	lua_pushboolean( L, 0 );
	return 1;
}

//Func: Player:IsBot()
//Retn: boolean expressing whether the Player is a bot
static int JPLua_Player_IsBot( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushboolean( L, !!(g_entities[player->clientNum].r.svFlags & SVF_BOT) );
	return 1;
}

// Push a Player instance for a client number onto the stack
void JPLua_Player_CreateRef( lua_State *L, int num ) {
	jplua_player_t *player = NULL;

	if ( num < 0 || num >= MAX_CLIENTS || !g_entities[num].inuse ) {
		lua_pushnil( L );
		return;
	}

	player = (jplua_player_t *)lua_newuserdata( L, sizeof( jplua_player_t ) );
	player->clientNum = num;

	luaL_getmetatable( L, PLAYER_META );
	lua_setmetatable( L, -2 );
}

// Ensure the value at the specified index is a valid Player instance,
// Return the instance if it is, otherwise return NULL.
jplua_player_t *JPLua_CheckPlayer( lua_State *L, int idx ) {
	void *ud = luaL_checkudata( L, idx, PLAYER_META );
	luaL_argcheck( L, ud != NULL, 1, "'Player' expected" );
	return (jplua_player_t *)ud;
}

static const struct luaL_Reg jplua_player_meta[] = {
	{ "__tostring",			JPLua_Player_ToString },
	{ "GetID",				JPLua_Player_GetID },
	{ "GetName",			JPLua_Player_GetName },
	{ "GetHealth",			JPLua_Player_GetHealth },
	{ "GetArmor",			JPLua_Player_GetArmor },
	{ "GetForce",			JPLua_Player_GetForce },
	{ "GetWeapon",			JPLua_Player_GetWeapon },
	{ "GetAmmo",			JPLua_Player_GetAmmo },
	{ "GetMaxAmmo",			JPLua_Player_GetMaxAmmo },
	{ "GetSaberStyle",		JPLua_Player_GetSaberStyle },
	{ "GetEFlags",			JPLua_Player_GetEFlags },
	{ "GetEFlags2",			JPLua_Player_GetEFlags2 },
	{ "GetDuelingPartner",	JPLua_Player_GetDuelingPartner },
	{ "GetLocation",		JPLua_Player_GetLocation },
	{ "GetAnimations",		JPLua_Player_GetAnimations },
	{ "GetPosition",		JPLua_Player_GetPosition },
	{ "GetAngles",			JPLua_Player_GetAngles },
	{ "GetVelocity",		JPLua_Player_GetVelocity },
	{ "IsWeaponHolstered",	JPLua_Player_IsWeaponHolstered },
	{ "IsAlive",			JPLua_Player_IsAlive },
	{ "IsBot",				JPLua_Player_IsBot },

	{ NULL, NULL }
};

// Register the Player class for Lua
void JPLua_Register_Player( lua_State *L ) {
	luaL_newmetatable( L, PLAYER_META ); // Create metatable for Player class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	luaL_register( L, NULL, jplua_player_meta ); // Fill metatable with fields
	lua_pop( L, -1 ); // Pop the Player class metatable from the stack
}

#endif // JPLUA
