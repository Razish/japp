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

//Func: Player1 == Player2
//Retn: boolean value of whether Player1 is the same client as Player2
static int JPLua_Player_Equals( lua_State *L ) {
	jplua_player_t *p1 = JPLua_CheckPlayer( L, 1 ), *p2 = JPLua_CheckPlayer( L, 2 );
	lua_pushboolean( L, (p1->clientNum == p2->clientNum) ? 1 : 0 );
	return 1;
}

//Func: tostring( Player )
//Retn: string representing the Player instance (for debug/error messages)
static int JPLua_Player_ToString( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushfstring( L, "Player(%d)", player->clientNum );
	return 1;
}

//Func: Player:GetAngles()
//Retn: Table of pitch/yaw/roll view angles
static int JPLua_Player_GetAngles( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "pitch" );	lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.pitch );	lua_settable( L, top );
	lua_pushstring( L, "yaw" );		lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.yaw);		lua_settable( L, top );
	lua_pushstring( L, "roll" );	lua_pushnumber( L, level.clients[player->clientNum].ps.viewangles.roll );	lua_settable( L, top );

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

//Func: Player:GetAnimations()
//Retn: Table of Legs/Torso anims and Legs/Torso timers
static int JPLua_Player_GetAnimations( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	playerState_t *ps = &g_entities[player->clientNum].client->ps;

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "torsoAnim" ); lua_pushnumber( L, ps->torsoAnim ); lua_settable( L, top );
	lua_pushstring( L, "torsoTimer" ); lua_pushnumber( L, ps->torsoTimer ); lua_settable( L, top );
	lua_pushstring( L, "legsAnim" ); lua_pushnumber( L, ps->legsAnim ); lua_settable( L, top );
	lua_pushstring( L, "legsTimer" ); lua_pushnumber( L, ps->legsTimer ); lua_settable( L, top );

	return 1;
}

//Func: Player:GetArmor()
//Retn: integer amount of the Player's armor
static int JPLua_Player_GetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.stats[STAT_ARMOR] );
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

//Func: Player:GetForce()
//Retn: integer amount of Player's force mana points
static int JPLua_Player_GetForce( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.fd.forcePower );
	return 1;
}

//Func: Player:GetHealth()
//Retn: integer amount of the Player's health
static int JPLua_Player_GetHealth( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.stats[STAT_HEALTH] );
	return 1;
}

//Func: Player:GetID()
//Retn: integer of the client's ID
static int JPLua_Player_GetID( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, player->clientNum );
	return 1;
}

//Func: Player:GetLocation()
//Retn: string of the nearest target_location
static int JPLua_Player_GetLocation( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	char location[64];
	if ( Team_GetLocationMsg( &g_entities[player->clientNum], location, sizeof( location ) ) )
		lua_pushstring( L, location );
	else
		lua_pushnil( L );

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

//Func: Player:GetName()
//Retn: string of the Player's name
static int JPLua_Player_GetName( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushstring( L, level.clients[player->clientNum].pers.netname );
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

//Func: Player:GetSaberStyle()
//Retn: integer index of the saber style the Player is using (See: 'SaberStyle' table)
static int JPLua_Player_GetSaberStyle( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.fd.saberDrawAnimLevel );
	return 1;
}

//Func: Player:GetScore()
//Retn: integer of the player's score
static int JPLua_Player_GetScore( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.persistant[PERS_SCORE] );
	return 1;
}


//Func: Player:GetUserinfo()
//Retn: Table of userinfo keys/values
static int JPLua_Player_GetUserinfo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	char userinfo[MAX_INFO_STRING];

	trap->GetUserinfo( player->clientNum, userinfo, sizeof( userinfo ) );

	JPLua_PushInfostring( L, userinfo );

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

//Func: Player:GetWeapon()
//Retn: integer index of the weapon the Player has selected (See: 'Weapons' table)
static int JPLua_Player_GetWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, level.clients[player->clientNum].ps.weapon );
	return 1;
}

//Func: Player:GiveWeapon( integer weaponID )
//Retn: N/A
static int JPLua_Player_GiveWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int wp = luaL_checkinteger( L, 2 );

	if ( wp <= WP_NONE || wp >= WP_NUM_WEAPONS )
		return 0;

	level.clients[player->clientNum].ps.stats[STAT_WEAPONS] |= (1<<wp);

	return 0;
}

//Func: Player:IsAdmin()
//Retn: boolean loggedIn, string adminUser, integer adminPermissions
static int JPLua_Player_IsAdmin( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gclient_t *cl = &level.clients[player->clientNum];

	if ( cl->pers.adminUser ) {
		lua_pushboolean( L, 1 );
		lua_pushstring( L, cl->pers.adminUser->user );
		lua_pushinteger( L, cl->pers.adminUser->privileges );
	}
	else {
		lua_pushboolean( L, 0 );
		lua_pushnil( L );
		lua_pushnil( L );
	}

	return 3;
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

//Func: Player:Kick( [string reason] )
//Retn: N/A
static int JPLua_Player_Kick( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *reason = lua_tostring( L, 2 );

	trap->DropClient( player->clientNum, reason ? reason : "was kicked" );

	return 0;
}

//Func: Player:Kill()
//Retn: N/A
static int JPLua_Player_Kill( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );

	return 0;
}

//Func: Player:RemoveEntityFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_RemoveEntityFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i=0; i<32; i++ ) {
		if ( bit & (1<<i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits removed (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags &= ~bit;

	return 0;
}

//Func: Player:SetArmor()
//Retn: N/A
static int JPLua_Player_SetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int armour = lua_tointeger( L, 2 );

	if ( armour < 0 )
		armour = 0;

	level.clients[player->clientNum].ps.stats[STAT_ARMOR] = armour;

	return 0;
}


//Func: Player:SetEntityFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetEntityFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i=0; i<32; i++ ) {
		if ( bit & (1<<i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits set (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags |= bit;

	return 0;
}

//Func: Player:SetHealth()
//Retn: N/A
static int JPLua_Player_SetHealth( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int hp = lua_tointeger( L, 2 );
	gentity_t *ent = &g_entities[player->clientNum];

	if ( hp < 1 )
		return 0;

	ent->health = ent->client->ps.stats[STAT_HEALTH] = hp;

	return 0;
}

//Func: Player:SetScore()
//Retn: N/A
static int JPLua_Player_SetScore( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	g_entities[player->clientNum].client->ps.persistant[PERS_SCORE] = lua_tointeger( L, 2 );

	return 0;
}



//Func: Player:SetVelocity( x, y, z )
//Retn: N/A
static int JPLua_Player_SetVelocity( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	lua_getfield( L, 2, "x" );	ent->playerState->velocity.x = lua_tonumber( L, -1 ); //x
	lua_getfield( L, 2, "y" );	ent->playerState->velocity.y = lua_tonumber( L, -1 ); //y
	lua_getfield( L, 2, "z" );	ent->playerState->velocity.z = lua_tonumber( L, -1 ); //z

	return 0;
}

//Func: Player:TakeWeapon( integer weaponID )
//Retn: N/A
static int JPLua_Player_TakeWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	int wp = luaL_checkinteger( L, 2 );
	int i, newWeapon = -1, selectedWeapon = ent->client->ps.weapon;

	if ( wp <= WP_NONE || wp >= WP_NUM_WEAPONS )
		return 0;

	ent->client->ps.stats[STAT_WEAPONS] &= ~(1<<wp);

	for ( i=WP_SABER; i<WP_NUM_WEAPONS; i++ ) {
		if ( ent->client->ps.stats[STAT_WEAPONS] & (1<<i) ) {
			newWeapon = i;
			break;
		}
	}

	if ( newWeapon == WP_NUM_WEAPONS ) {
		for ( i=WP_STUN_BATON; i<WP_SABER; i++ ) {
			if ( ent->client->ps.stats[STAT_WEAPONS] & (1<<i) ) {
				newWeapon = i;
				break;
			}
		}
		if ( newWeapon == WP_SABER )
			newWeapon = WP_NONE;
	}

	ent->client->ps.weapon = (newWeapon == -1) ? 0 : newWeapon;

	G_AddEvent( ent, EV_NOAMMO, selectedWeapon );

	return 0;
}

//Func: Player:Teleport( table position{x, y, z}, table angles{pitch, yaw, roll} )
//Retn: N/A
static int JPLua_Player_Teleport( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	vector3 pos, angles;

	// get position
	lua_getfield( L, 2, "x" );	pos.x = lua_tonumber( L, -1 );
	lua_getfield( L, 2, "y" );	pos.y = lua_tonumber( L, -1 );
	lua_getfield( L, 2, "z" );	pos.z = lua_tonumber( L, -1 );

	// get angles
	lua_getfield( L, 3, "pitch" );	angles.pitch = lua_tonumber( L, -1 );
	lua_getfield( L, 3, "yaw" );	angles.yaw = lua_tonumber( L, -1 );
	lua_getfield( L, 3, "roll" );	angles.roll = lua_tonumber( L, -1 );

	TeleportPlayer( ent, &pos, &angles );

	return 0;
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
	{ "__eq",				JPLua_Player_Equals },
	{ "__tostring",			JPLua_Player_ToString },
	{ "GetAmmo",			JPLua_Player_GetAmmo },
	{ "GetAngles",			JPLua_Player_GetAngles },
	{ "GetAnimations",		JPLua_Player_GetAnimations },
	{ "GetArmor",			JPLua_Player_GetArmor },
	{ "GetDuelingPartner",	JPLua_Player_GetDuelingPartner },
	{ "GetEFlags",			JPLua_Player_GetEFlags },
	{ "GetEFlags2",			JPLua_Player_GetEFlags2 },
	{ "GetForce",			JPLua_Player_GetForce },
	{ "GetHealth",			JPLua_Player_GetHealth },
	{ "GetID",				JPLua_Player_GetID },
	{ "GetLocation",		JPLua_Player_GetLocation },
	{ "GetMaxAmmo",			JPLua_Player_GetMaxAmmo },
	{ "GetName",			JPLua_Player_GetName },
	{ "GetPosition",		JPLua_Player_GetPosition },
	{ "GetSaberStyle",		JPLua_Player_GetSaberStyle },
	{ "GetScore",			JPLua_Player_GetScore },
	{ "GetUserinfo",		JPLua_Player_GetUserinfo },
	{ "GetVelocity",		JPLua_Player_GetVelocity },
	{ "GetWeapon",			JPLua_Player_GetWeapon },
	{ "GiveWeapon",			JPLua_Player_GiveWeapon },
	{ "IsAdmin",			JPLua_Player_IsAdmin },
	{ "IsAlive",			JPLua_Player_IsAlive },
	{ "IsBot",				JPLua_Player_IsBot },
	{ "IsWeaponHolstered",	JPLua_Player_IsWeaponHolstered },
	{ "Kick",				JPLua_Player_Kick },
	{ "Kill",				JPLua_Player_Kill },
	{ "RemoveEntityFlag",	JPLua_Player_RemoveEntityFlag },
	{ "SetArmor",			JPLua_Player_SetArmor },
	{ "SetEntityFlag",		JPLua_Player_SetEntityFlag },
	{ "SetHealth",			JPLua_Player_SetHealth },
	{ "SetScore",			JPLua_Player_SetScore },
	{ "SetVelocity",		JPLua_Player_SetVelocity },
	{ "TakeWeapon",			JPLua_Player_TakeWeapon },
	{ "Teleport",			JPLua_Player_Teleport },

	{ NULL, NULL }
};

// Register the Player class for Lua
void JPLua_Register_Player( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, PLAYER_META ); // Create metatable for Player class, push on stack

	// Lua won't attempt to directly index userdata, only via metatables
	// Make this metatable's __index loopback to itself so can index the object directly
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 ); // Re-push metatable to top of stack
	lua_settable( L, -3 ); // metatable.__index = metatable

	// fill metatable with fields
#if LUA_VERSION_NUM > 501
	for ( r=jplua_player_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}
#else
	luaL_register( L, NULL, jplua_player_meta );
#endif

	lua_pop( L, -1 ); // Pop the Player class metatable from the stack
}

#endif // JPLUA
