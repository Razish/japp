#include "cg_local.h"
#include "cg_lua.h"

#ifdef JPLUA

static const char PLAYER_META[] = "Player.meta";

//Func: GetPlayer(clientNum)
//Retn: Player object if their clientinfo is valid, and clientNum is between [0, MAX_CLIENTS-1]
int JPLua_GetPlayer( lua_State *L ) {
	int num = -1;
	unsigned int clientsFound = 0;

	if ( lua_type( L, 1 ) == LUA_TNUMBER )
		num = lua_tointeger( L, 1 );

	else if ( lua_type( L, 1 ) == LUA_TSTRING ) {
		const char *name = lua_tostring( L, 1 );
		int numFound = 0;
		int i=0;
		//RAZTODO: copy G_ClientFromString
		for ( i=0; i<cgs.maxclients; i++ ) {
			char nameClean[36] = {0};
			char nameClean2[36] = {0};
			if ( !cgs.clientinfo[i].infoValid )
				continue;
			Q_strncpyz( nameClean, cgs.clientinfo[i].name, sizeof( nameClean ) );
			Q_strncpyz( nameClean2, name, sizeof( nameClean2 ) );
			Q_CleanString( nameClean, STRIP_COLOUR );
			Q_CleanString( nameClean2, STRIP_COLOUR );
			if ( !Q_stricmp( nameClean, nameClean2 ) ) {
				num = i;
				clientsFound |= (1<<i);
				numFound++;
			}
		}

		if ( numFound > 1 ) {
			int top=0;
			lua_pushnil( L );
			lua_pushstring( L, "Multiple matches" );
			
			lua_newtable( L );
			top = lua_gettop( L );
			for ( i=0; i<cgs.maxclients; i++ ) {
				if ( clientsFound & (1<<i) ) {
					lua_pushnumber( L, i );
					JPLua_Player_CreateRef( L, i );
					lua_settable( L, top );
				}
			}
			return 3;
		}
		else if ( !numFound ) {
			lua_pushnil( L );
			return 1;
		}
	}

	else //if ( lua_type( L, 1 ) == LUA_TNIL )
		num = cg.clientNum;

	JPLua_Player_CreateRef( L, num );
	return 1;
}

//Func: tostring(Player)
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
	lua_pushstring( L, cgs.clientinfo[player->clientNum].name );
	return 1;
}

//Func: Player:GetHealth()
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer amount of the Player's health
static int JPLua_Player_GetHealth( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.stats[STAT_HEALTH] );
	else
		lua_pushnil( L );
	return 1;
}

//Func: Player:GetLastPickup()
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		string of the most recent pickup's name
static int JPLua_Player_GetLastPickup( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( player->clientNum == cg.clientNum )
		lua_pushstring( L, CG_GetStringEdString( "SP_INGAME", bg_itemlist[cg.itemPickup].classname ) );
	else
		lua_pushnil( L );
	return 1;
}

//Func: Player:GetArmor()
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer amount of the Player's armor
static int JPLua_Player_GetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.stats[STAT_ARMOR] );
	else
		lua_pushnil( L );
	return 1;
}

//Func: Player:GetForce()
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer amount of Player's force mana points
static int JPLua_Player_GetForce( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.fd.forcePower );
	else
		lua_pushnil( L );
	return 1;
}

//Func: Player:GetWeapon()
//Retn: integer index of the weapon the Player has selected (See: 'Weapons' table)
static int JPLua_Player_GetWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.weapon );
	return 1;
}

//Func: Player:GetAmmo([weapon])
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer amount of the ammo for either the specified weapon, or the weapon the player has selected
static int JPLua_Player_GetAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int weapon = cg.predictedPlayerState.weapon;

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.ammo[weaponData[weapon].ammoIndex] );
	return 1;
}

//Func: Player:GetMaxAmmo([weapon])
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer amount of the max ammo for either the specified weapon, or the weapon the player has selected
static int JPLua_Player_GetMaxAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int weapon = cg.predictedPlayerState.weapon;

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, ammoData[weaponData[weapon].ammoIndex].max );
	return 1;
}

//Func: Player:GetSaberStyle()
//Retn: nil if Player:GetID() != self:GetID() due to lack of reliable information
//		integer index of the saber style the Player is using (See: 'SaberStyle' table)
static int JPLua_Player_GetSaberStyle( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.fd.saberDrawAnimLevel );
	else
		lua_pushnil( L );
	return 1;
}

//Func: Player:GetEFlags()
//Retn: bit-mask of various entity flags (See: 'EFlags' table)
static int JPLua_Player_GetEFlags( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.eFlags );
	return 1;
}

//Func: Player:GetEFlags2()
//Retn: bit-mask of various entity flags (See: 'EFlags2' table)
static int JPLua_Player_GetEFlags2( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.eFlags2 );
	return 1;
}

//Func: Player:GetDuelingPartner()
//Retn: nil if Player is not dueling, or Player:GetID() != self:GetID() due to lack of reliable information
//		Player object of the client Player is dueling
static int JPLua_Player_GetDuelingPartner( lua_State *L )
{
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	if ( !cg_entities[player->clientNum].currentState.bolt1 || player->clientNum != cg.clientNum )
		lua_pushnil( L );
	else
		JPLua_Player_CreateRef( L, cg.predictedPlayerState.duelIndex );

	return 1;
}

//Func: Player:GetLocation()
//Retn: string of the nearest target_location
extern const char *CG_GetLocationString( const char *loc ); // cg_main.c
static int JPLua_Player_GetLocation( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushstring( L, CG_GetLocationString( CG_ConfigString( CS_LOCATIONS+cgs.clientinfo[player->clientNum].location ) ) );

	return 1;
}

//Func: Player:GetClientInfo()
//Retn: key/value table of the player's Clientinfo
static int JPLua_Player_GetClientInfo( lua_State *L ) {
	int top=0, top2=0, i=0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	clientInfo_t *ci = &cgs.clientinfo[player->clientNum];
	entityState_t *es = &cg_entities[player->clientNum].currentState;
	score_t	*score = NULL; // for ping and other scoreboard information

	for ( i=0; i<cg.numScores; i++ ) {
		if ( cg.scores[i].client == player->clientNum ) {
			score = &cg.scores[i];
			break;
		}
	}
	
	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "saberName" ); lua_pushstring( L, ci->saberName ); lua_settable( L, top );
	lua_pushstring( L, "saber2Name" ); lua_pushstring( L, ci->saber2Name ); lua_settable( L, top );
	lua_pushstring( L, "name" ); lua_pushstring( L, ci->name ); lua_settable( L, top );
	lua_pushstring( L, "team" ); lua_pushinteger( L, ci->team ); lua_settable( L, top );
	lua_pushstring( L, "duelTeam" ); lua_pushinteger( L, ci->duelTeam ); lua_settable( L, top );
	lua_pushstring( L, "botSkill" ); lua_pushinteger( L, ci->botSkill ); lua_settable( L, top );
	lua_pushstring( L, "icolor1" ); lua_pushinteger( L, ci->icolor1 ); lua_settable( L, top );
	lua_pushstring( L, "icolor2" ); lua_pushinteger( L, ci->icolor2 ); lua_settable( L, top );
	lua_pushstring( L, "score" ); lua_pushinteger( L, ci->score ); lua_settable( L, top );
	lua_pushstring( L, "modelName" ); lua_pushstring( L, ci->modelName ); lua_settable( L, top );
	lua_pushstring( L, "skinName" ); lua_pushstring( L, ci->skinName ); lua_settable( L, top );
	lua_pushstring( L, "deferred" ); lua_pushboolean( L, !!ci->deferred ); lua_settable( L, top );
	lua_pushstring( L, "gender" ); lua_pushinteger( L, ci->gender ); lua_settable( L, top );
	if ( score ) {
		lua_pushstring( L, "ping" ); lua_pushinteger( L, (player->clientNum==cg.clientNum) ? cg.snap->ping : score->ping ); lua_settable( L, top );
		lua_pushstring( L, "time" ); lua_pushinteger( L, score->time ); lua_settable( L, top );
		lua_pushstring( L, "deaths" ); lua_pushinteger( L, score->deaths ); lua_settable( L, top );
	}
	else {
		lua_pushstring( L, "ping" );
		if ( player->clientNum == cg.clientNum )
			lua_pushinteger( L, cg.snap->ping );
		else
			lua_pushnil( L );
		lua_settable( L, top );
		lua_pushstring( L, "time" ); lua_pushnil( L ); lua_settable( L, top );
		lua_pushstring( L, "deaths" ); lua_pushnil( L ); lua_settable( L, top );
	}

	lua_pushstring( L, "rgb1" );
		lua_newtable( L ); top2 = lua_gettop( L );
		lua_pushstring( L, "r" ); lua_pushnumber( L, ci->rgb1.r ); lua_settable( L, top2 );
		lua_pushstring( L, "g" ); lua_pushnumber( L, ci->rgb1.g ); lua_settable( L, top2 );
		lua_pushstring( L, "b" ); lua_pushnumber( L, ci->rgb1.b ); lua_settable( L, top2 );
	lua_settable( L, top );

	lua_pushstring( L, "rgb2" );
		lua_newtable( L ); top2 = lua_gettop( L );
		lua_pushstring( L, "r" ); lua_pushnumber( L, ci->rgb2.r ); lua_settable( L, top2 );
		lua_pushstring( L, "g" ); lua_pushnumber( L, ci->rgb2.g ); lua_settable( L, top2 );
		lua_pushstring( L, "b" ); lua_pushnumber( L, ci->rgb2.b ); lua_settable( L, top2 );
	lua_settable( L, top );

	lua_pushstring( L, "skinRGB" );
		lua_newtable( L ); top2 = lua_gettop( L );
		lua_pushstring( L, "r" ); lua_pushnumber( L, es->customRGBA[0] ); lua_settable( L, top2 );
		lua_pushstring( L, "g" ); lua_pushnumber( L, es->customRGBA[1] ); lua_settable( L, top2 );
		lua_pushstring( L, "b" ); lua_pushnumber( L, es->customRGBA[2] ); lua_settable( L, top2 );
	lua_settable( L, top );
	
	return 1;
}
//[ASTRAL-START]
//Func: Player:GetAnimations()
//Retn: Table of Legs/Torso anims and Legs/Torso timers
static int JPLua_Player_GetAnimations( lua_State *L ) {
	int top = 0;
	int torsoAnim = 0;
	int torsoTimer = 0;
	int legsAnim = 0;
	int legsTimer = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	
	if ( player->clientNum == cg.clientNum ) {
		torsoAnim = cg.predictedPlayerState.torsoAnim;
		torsoTimer = cg.predictedPlayerState.torsoTimer;
		legsAnim = cg.predictedPlayerState.legsAnim;
		legsTimer = cg.predictedPlayerState.legsTimer;
	}
	else {
		torsoAnim = cg_entities[player->clientNum].currentState.torsoAnim;
		legsAnim = cg_entities[player->clientNum].currentState.legsAnim;
	}
		
	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "torsoAnim" ); lua_pushnumber( L, torsoAnim ); lua_settable( L, top );
	lua_pushstring( L, "torsoTimer" ); lua_pushnumber( L, torsoTimer ); lua_settable( L, top );
	lua_pushstring( L, "legsAnim" ); lua_pushnumber( L, legsAnim ); lua_settable( L, top );
	lua_pushstring( L, "legsTimer" ); lua_pushnumber( L, legsTimer ); lua_settable( L, top );
	return 1;
}
//[ASTRAL-END]
//Func: Player:GetPosition()
//Retn: Table of x/y/z position
static int JPLua_Player_GetPosition( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 position = { 0.0f };
	
	if ( player->clientNum == cg.clientNum )
		VectorCopy( &cg.predictedPlayerState.origin, &position );
	else
		VectorCopy( &cg_entities[player->clientNum].currentState.pos.trBase, &position );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, position.x ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, position.y ); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, position.z ); lua_settable( L, top );
	return 1;
}

//Func: Player:GetAngles()
//Retn: Table of pitch/yaw/roll view angles
static int JPLua_Player_GetAngles( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 angles = { 0.0f };
	
	if ( player->clientNum == cg.clientNum )
		VectorCopy( &cg.predictedPlayerState.viewangles, &angles );
	else
		VectorCopy( &cg_entities[player->clientNum].lerpAngles, &angles );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, angles.pitch ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, angles.yaw); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, angles.roll ); lua_settable( L, top );
	return 1;
}

//Func: Player:GetVelocity()
//Retn: Table of x/y/z velocity
static int JPLua_Player_GetVelocity( lua_State *L ) {
	int top = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 velocity = { 0.0f };
	
	if ( player->clientNum == cg.clientNum )
		VectorCopy( &cg.predictedPlayerState.velocity, &velocity );
	else
		VectorCopy( &cg_entities[player->clientNum].currentState.pos.trDelta, &velocity );

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "x" ); lua_pushnumber( L, velocity.x ); lua_settable( L, top );
	lua_pushstring( L, "y" ); lua_pushnumber( L, velocity.y ); lua_settable( L, top );
	lua_pushstring( L, "z" ); lua_pushnumber( L, velocity.z ); lua_settable( L, top );
	return 1;
}

//Func: Player:IsWeaponHolstered()
//Retn: integer expressing the holstered state of the saber
//		0 - all applicable sabers are activated
//		1 - using dual/staff and only one blade is activated
//		2 - both blades are off
static int JPLua_Player_IsWeaponHolstered( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.saberHolstered );
	return 1;
}

//Func: Player:IsAlive()
//Retn: boolean expressing whether the Player is alive
static int JPLua_Player_IsAlive( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	//RAZFIXME: Check clientNum bounds
	if ( player->clientNum == cg.clientNum ) {
		if ( cg.predictedPlayerState.stats[STAT_HEALTH] > 0 && !(cg.predictedPlayerState.eFlags & EF_DEAD) && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			lua_pushboolean( L, 1 );
			return 1;
		}
	}
	else {
		if ( !(cg_entities[player->clientNum].currentState.eFlags & EF_DEAD) && cgs.clientinfo[player->clientNum].team != TEAM_SPECTATOR ) {
			lua_pushboolean( L, 1 );
			return 1;
		}
	}

	lua_pushboolean( L, 0 );
	return 1;
}

//Func: Player:IsBot()
//Retn: boolean expressing whether the Player is a bot
static int JPLua_Player_IsBot( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushboolean( L, !!(cgs.clientinfo[player->clientNum].botSkill != -1) );
	return 1;
}

// Push a Player instance for a client number onto the stack
void JPLua_Player_CreateRef( lua_State *L, int num ) {
	jplua_player_t *player = NULL;

	if ( num < 0 || num >= MAX_CLIENTS || !cgs.clientinfo[num].infoValid ) {
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
	{ "GetLastPickup",		JPLua_Player_GetLastPickup },
	{ "GetLocation",		JPLua_Player_GetLocation },
	{ "GetClientInfo",		JPLua_Player_GetClientInfo },
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
