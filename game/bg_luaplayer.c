#if defined(_GAME)
#include "g_local.h"
#elif defined(_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

static const char PLAYER_META[] = "Player.meta";

//Func: GetPlayer( clientNum )
//Retn: Player object
int JPLua_GetPlayer( lua_State *L ) {
#if defined(_GAME)
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
#elif defined(_CGAME)
	int num = -1;
	unsigned int clientsFound = 0;

	if ( lua_type( L, 1 ) == LUA_TNUMBER )
		num = lua_tointeger( L, 1 );

	else if ( lua_type( L, 1 ) == LUA_TSTRING ) {
		const char *name = lua_tostring( L, 1 );
		int numFound = 0;
		int i = 0;
		//RAZTODO: copy G_ClientFromString
		for ( i = 0; i < cgs.maxclients; i++ ) {
			char nameClean[36], nameClean2[36];

			if ( !cgs.clientinfo[i].infoValid )
				continue;

			Q_strncpyz( nameClean, cgs.clientinfo[i].name, sizeof(nameClean) );
			Q_strncpyz( nameClean2, name, sizeof(nameClean2) );
			Q_CleanString( nameClean, STRIP_COLOUR );
			Q_CleanString( nameClean2, STRIP_COLOUR );
			if ( !Q_stricmp( nameClean, nameClean2 ) ) {
				num = i;
				clientsFound |= (1 << i);
				numFound++;
			}
		}

		if ( numFound > 1 ) {
			int top = 0;
			lua_pushnil( L );
			lua_pushstring( L, "Multiple matches" );

			lua_newtable( L );
			top = lua_gettop( L );
			for ( i = 0; i < cgs.maxclients; i++ ) {
				if ( clientsFound & (1 << i) ) {
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
#endif
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

//Func: Player:GetAmmo([weapon])
//Retn: integer amount of the ammo for either the specified weapon, or the weapon the player has selected
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	gentity_t *ent = g_entities + player->clientNum;
	int weapon = ent->client->ps.weapon;
#elif defined(_CGAME)
	int weapon = cg.predictedPlayerState.weapon;
#endif

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.ammo[weaponData[weapon].ammoIndex] );
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.ammo[weaponData[weapon].ammoIndex] );
#endif
	return 1;
}

//Func: Player:GetAngles()
//Retn: Vector3 of pitch/yaw/roll view angles
static int JPLua_Player_GetAngles( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	vector3 *angles = &g_entities[player->clientNum].client->ps.viewangles;
#elif defined(_CGAME)
	vector3 *angles = (player->clientNum == cg.clientNum) ? &cg.predictedPlayerState.viewangles : &cg_entities[player->clientNum].lerpAngles;
#endif

	JPLua_Vector_CreateRef( L, angles->pitch, angles->yaw, angles->roll );

	return 1;
}

//Func: Player:GetAnimations()
//Retn: Table of Legs/Torso anims and Legs/Torso timers
static int JPLua_Player_GetAnimations( lua_State *L ) {
	int top, torsoAnim = 0, torsoTimer = 0, legsAnim = 0, legsTimer = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	playerState_t *ps = &g_entities[player->clientNum].client->ps;
#elif defined(_CGAME)
	playerState_t *ps = &cg.predictedPlayerState;
	entityState_t *es = &cg_entities[player->clientNum].currentState;
#endif

#ifdef _CGAME
	if ( player->clientNum == cg.clientNum ) {
#endif
		torsoAnim = ps->torsoAnim;
		torsoTimer = ps->torsoTimer;
		legsAnim = ps->legsAnim;
		legsTimer = ps->legsTimer;
#ifdef _CGAME
	}
	else {
		torsoAnim = es->torsoAnim;
		legsAnim = es->legsAnim;
	}
#endif

	lua_newtable( L );
	top = lua_gettop( L );

	lua_pushstring( L, "torsoAnim" );	lua_pushnumber( L, torsoAnim );		lua_settable( L, top );
	lua_pushstring( L, "torsoTimer" );	lua_pushnumber( L, torsoTimer );	lua_settable( L, top );
	lua_pushstring( L, "legsAnim" );	lua_pushnumber( L, legsAnim );		lua_settable( L, top );
	lua_pushstring( L, "legsTimer" );	lua_pushnumber( L, legsTimer );		lua_settable( L, top );

	return 1;
}

//Func: Player:GetArmor()
//Retn: integer amount of the Player's armor
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	playerState_t *ps = &g_entities[player->clientNum].client->ps;
#elif defined(_CGAME)
	playerState_t *ps = &cg.predictedPlayerState;
#endif

#ifdef _CGAME
	if ( player->clientNum == cg.clientNum )
#endif
		lua_pushinteger( L, ps->stats[STAT_ARMOR] );
#ifdef _CGAME
	else
		lua_pushnil( L );
#endif
	return 1;
}

#ifdef _CGAME
//Func: Player:GetClientInfo()
//Retn: key/value table of the player's Clientinfo
static int JPLua_Player_GetClientInfo( lua_State *L ) {
	int top = 0, i = 0;
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	clientInfo_t *ci = &cgs.clientinfo[player->clientNum];
	entityState_t *es = &cg_entities[player->clientNum].currentState;
	score_t	*score = NULL; // for ping and other scoreboard information

	for ( i = 0; i < cg.numScores; i++ ) {
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
		lua_pushstring( L, "ping" ); lua_pushinteger( L, (player->clientNum == cg.clientNum) ? cg.snap->ping : score->ping ); lua_settable( L, top );
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

	lua_pushstring( L, "rgb1" ); JPLua_Vector_CreateRef( L, ci->rgb1.r, ci->rgb1.g, ci->rgb1.b ); lua_settable( L, top );
	lua_pushstring( L, "rgb2" ); JPLua_Vector_CreateRef( L, ci->rgb2.r, ci->rgb2.g, ci->rgb2.b ); lua_settable( L, top );
	lua_pushstring( L, "skinRGB" ); JPLua_Vector_CreateRef( L, es->customRGBA[0], es->customRGBA[1], es->customRGBA[2] ); lua_settable( L, top );

	return 1;
}
#endif

//Func: Player:GetDuelingPartner()
//Retn: nil if Player is not dueling, or on client if Player:GetID() != self:GetID() due to lack of reliable information
//		Player object of the client Player is dueling
static int JPLua_Player_GetDuelingPartner( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	playerState_t *ps = &g_entities[player->clientNum].client->ps;
	if ( ps->duelInProgress )
		JPLua_Player_CreateRef( L, ps->duelIndex );
	else
		lua_pushnil( L );
#elif defined(_CGAME)
	if ( !cg_entities[player->clientNum].currentState.bolt1 || player->clientNum != cg.clientNum )
		lua_pushnil( L );
	else
		JPLua_Player_CreateRef( L, cg.predictedPlayerState.duelIndex );
#endif

	return 1;
}

//Func: Player:GetEFlags()
//Retn: bit-mask of various entity flags (See: 'EFlags' table)
static int JPLua_Player_GetEFlags( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.eFlags );
#elif defined(_CGAME)
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.eFlags );
#endif
	return 1;
}

//Func: Player:GetEFlags2()
//Retn: bit-mask of various entity flags (See: 'EFlags2' table)
static int JPLua_Player_GetEFlags2( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.eFlags2 );
#elif defined(_CGAME)
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.eFlags2 );
#endif
	return 1;
}

#ifdef _GAME
//Func: Player:GetFlags()
//Retn: bit-mask of various entity flags (See 'Flags' table)
static int JPLua_Player_GetFlags( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, g_entities[player->clientNum].flags );
	return 1;
}
#endif

//Func: Player:GetForce()
//Retn: integer amount of Player's force mana points
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetForce( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.fd.forcePower );
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.fd.forcePower );
	else
		lua_pushnil( L );
#endif
	return 1;
}

//Func: Player:GetHealth()
//Retn: integer amount of the Player's health
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetHealth( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.stats[STAT_HEALTH] );
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.stats[STAT_HEALTH] );
	else
		lua_pushnil( L );
#endif
	return 1;
}

//Func: Player:GetID()
//Retn: integer of the client's ID
static int JPLua_Player_GetID( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, player->clientNum );
	return 1;
}

#ifdef _CGAME
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
#endif

//Func: Player:GetLocation()
//Retn: string of the nearest target_location
static int JPLua_Player_GetLocation( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	char location[64];
	if ( Team_GetLocationMsg( &g_entities[player->clientNum], location, sizeof(location) ) )
		lua_pushstring( L, location );
	else
		lua_pushnil( L );
#elif defined(_CGAME)
	lua_pushstring( L, CG_GetLocationString( CG_ConfigString( CS_LOCATIONS + cgs.clientinfo[player->clientNum].location ) ) );
#endif

	return 1;
}

//Func: Player:GetMaxAmmo([weapon])
//Retn: integer amount of the max ammo for either the specified weapon, or the weapon the player has selected
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetMaxAmmo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	int weapon = g_entities[player->clientNum].client->ps.weapon;
#elif defined(_CGAME)
	int weapon = cg.predictedPlayerState.weapon;
#endif

	if ( lua_gettop( L ) == 2 )
		weapon = lua_tointeger( L, 2 );

#ifdef _CGAME
	if ( player->clientNum != cg.clientNum )
		lua_pushnil( L );
#endif

	lua_pushinteger( L, ammoData[weaponData[weapon].ammoIndex].max );

	return 1;
}

//Func: Player:GetName()
//Retn: string of the Player's name
static int JPLua_Player_GetName( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushstring( L, g_entities[player->clientNum].client->pers.netname );
#elif defined(_CGAME)
	lua_pushstring( L, cgs.clientinfo[player->clientNum].name );
#endif
	return 1;
}

//Func: Player:GetPosition()
//Retn: Vector3 of x,y,z position
static int JPLua_Player_GetPosition( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	vector3 *pos = &g_entities[player->clientNum].client->ps.origin;
#elif defined(_CGAME)
	vector3 *pos = (player->clientNum == cg.clientNum) ? &cg.predictedPlayerState.origin : &cg_entities[player->clientNum].currentState.pos.trBase;
#endif

	JPLua_Vector_CreateRef( L, pos->x, pos->y, pos->z );

	return 1;
}

//Func: Player:GetSaberStyle()
//Retn: integer index of the saber style the Player is using (See: 'SaberStyle' table)
//		on client, nil if Player:GetID() != self:GetID() due to lack of reliable information
static int JPLua_Player_GetSaberStyle( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.fd.saberDrawAnimLevel );
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum )
		lua_pushinteger( L, cg.predictedPlayerState.fd.saberDrawAnimLevel );
	else
		lua_pushnil( L );
#endif

	return 1;
}

#ifdef _GAME
//Func: Player:GetScore()
//Retn: integer of the player's score
static int JPLua_Player_GetScore( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.persistant[PERS_SCORE] );
	return 1;
}
#endif

#ifdef _GAME
//Func: Player:GetUserinfo()
//Retn: Table of userinfo keys/values
static int JPLua_Player_GetUserinfo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	char userinfo[MAX_INFO_STRING];

	trap->GetUserinfo( player->clientNum, userinfo, sizeof(userinfo) );

	JPLua_PushInfostring( L, userinfo );

	return 1;
}
#endif

//Func: Player:GetVelocity()
//Retn: Vector3 of x,y,z velocity
static int JPLua_Player_GetVelocity( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	vector3 *vel = &g_entities[player->clientNum].client->ps.velocity;
#elif defined(_CGAME)
	vector3 *vel = (player->clientNum == cg.clientNum) ? &cg.predictedPlayerState.velocity : &cg_entities[player->clientNum].currentState.pos.trDelta;
#endif

	JPLua_Vector_CreateRef( L, vel->x, vel->y, vel->z );

	return 1;
}

//Func: Player:GetWeapon()
//Retn: integer index of the weapon the Player has selected (See: 'Weapons' table)
static int JPLua_Player_GetWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->ps.weapon );
#elif defined(_CGAME)
	lua_pushinteger( L, cg_entities[player->clientNum].currentState.weapon );
#endif

	return 1;
}

#ifdef _GAME
//Func: Player:GiveWeapon( integer weaponID )
//Retn: N/A
static int JPLua_Player_GiveWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int wp = luaL_checkinteger( L, 2 );

	if ( wp <= WP_NONE || wp >= WP_NUM_WEAPONS )
		return 0;

	g_entities[player->clientNum].client->ps.stats[STAT_WEAPONS] |= (1 << wp);

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:IsAdmin()
//Retn: boolean loggedIn, string adminUser, integer adminPermissions
static int JPLua_Player_IsAdmin( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gclient_t *cl = g_entities[player->clientNum].client;

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
#endif

//Func: Player:IsAlive()
//Retn: boolean expressing whether the Player is alive
static int JPLua_Player_IsAlive( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

#if defined(_GAME)
	if ( g_entities[player->clientNum].client->ps.stats[STAT_HEALTH] > 0
		&& !(g_entities[player->clientNum].client->ps.eFlags & EF_DEAD)
		&& g_entities[player->clientNum].client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		lua_pushboolean( L, 1 );
		return 1;
	}
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum ) {
		if ( cg.predictedPlayerState.stats[STAT_HEALTH] > 0 && !(cg.predictedPlayerState.eFlags & EF_DEAD) && cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			lua_pushboolean( L, 1 );
			return 1;
		}
	}
	else if ( !(cg_entities[player->clientNum].currentState.eFlags & EF_DEAD) && cgs.clientinfo[player->clientNum].team != TEAM_SPECTATOR ) {
		lua_pushboolean( L, 1 );
		return 1;
	}
#endif

	lua_pushboolean( L, 0 );
	return 1;
}

//Func: Player:IsBot()
//Retn: boolean expressing whether the Player is a bot
static int JPLua_Player_IsBot( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

#if defined(_GAME)
	lua_pushboolean( L, !!(g_entities[player->clientNum].r.svFlags & SVF_BOT) );
#elif defined(_CGAME)
	lua_pushboolean( L, !!(cgs.clientinfo[player->clientNum].botSkill != -1) );
#endif

	return 1;
}

//Func: Player:IsUnderwater()
//Retn: boolean expressing if the player is underwater
static int JPLua_Player_IsUnderwater( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	qboolean underwater = qfalse;
#if defined(_GAME)
	if ( g_entities[player->clientNum].waterlevel == 3 )
		underwater = qtrue;
#elif defined(_CGAME)
	vector3 *pos = (player->clientNum == cg.clientNum) ? &cg.predictedPlayerState.origin
		: &cg_entities[player->clientNum].currentState.pos.trBase; // not cent->lerpOrigin?
	if ( CG_PointContents( pos, -1 ) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA) )
		underwater = qtrue;
#endif

	lua_pushboolean( L, underwater ? 1 : 0 );

	return 1;
}

//Func: Player:IsWeaponHolstered()
//Retn: integer expressing the holstered state of the saber
//		0 - all applicable sabers are activated
//		1 - using dual/staff and only one blade is activated
//		2 - both blades are off
static int JPLua_Player_IsWeaponHolstered( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	entityState_t *es = &g_entities[player->clientNum].s;
#elif defined(_CGAME)
	entityState_t *es = &cg_entities[player->clientNum].currentState;
#endif

	lua_pushinteger( L, es->saberHolstered );

	return 1;
}

#ifdef _GAME
//Func: Player:Kick( [string reason] )
//Retn: N/A
static int JPLua_Player_Kick( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *reason = lua_tostring( L, 2 );

	trap->DropClient( player->clientNum, reason ? reason : "was kicked" );

	return 0;
}
#endif

#ifdef _GAME
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
#endif

#ifdef _GAME
//Func: Player:OnSameTeam( Player p2 )
//Retn: boolean expressing whether the specified player is on this player's team
static int JPLua_Player_OnSameTeam( lua_State *L ) {
	jplua_player_t *self = JPLua_CheckPlayer( L, 1 ), *other = JPLua_CheckPlayer( L, 2 );
	lua_pushboolean( L, OnSameTeam( &g_entities[self->clientNum], &g_entities[other->clientNum] ) ? 1 : 0 );
	return 1;
}
#endif

#ifdef _CGAME
//Func: Player:OnSameTeam()
//Retn: boolean expressing whether this player is on your team
static int JPLua_Player_OnSameTeam( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 2 );
	const clientInfo_t *ci = &cgs.clientinfo[player->clientNum];

	if ( cgs.gametype >= GT_TEAM && ci->team == cgs.clientinfo[cg.clientNum].team )
		lua_pushboolean( L, 1 );
	else if ( cgs.gametype == GT_POWERDUEL && ci->duelTeam == cgs.clientinfo[cg.clientNum].duelTeam )
		lua_pushboolean( L, 1 );
	else
		lua_pushboolean( L, 0 );

	return 1;
}
#endif

#ifdef _GAME
//Func: Player:RemoveEFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_RemoveEFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits removed (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags &= ~bit;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:RemoveEFlag2( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_RemoveEFlag2( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits removed (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags2 &= ~bit;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:RemoveFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_RemoveFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits removed (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].flags &= ~bit;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetArmor()
//Retn: N/A
static int JPLua_Player_SetArmor( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int armour = lua_tointeger( L, 2 );

	if ( armour < 0 )
		armour = 0;

	g_entities[player->clientNum].client->ps.stats[STAT_ARMOR] = armour;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetEFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetEFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits set (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags |= bit;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetEFlag2( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetEFlag2( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits set (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].client->ps.eFlags2 |= bit;

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = lua_tointeger( L, 2 );
	int i, found = 0;

	for ( i = 0; i<32; i++ ) {
		if ( bit & (1 << i) )
			found++;
	}

	if ( found > 1 ) {
		lua_pushfstring( L, "too many bits set (%i)\n", found );
		return 1;
	}

	g_entities[player->clientNum].flags |= bit;

	return 0;
}
#endif

#ifdef _GAME
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
#endif

#ifdef _GAME
//Func: Player:SetName( string name, [boolean announce] )
//Retn: N/A
static int JPLua_Player_SetName( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *name = lua_tostring( L, 2 );
	char info[MAX_INFO_STRING], oldName[MAX_NETNAME];
	gentity_t *ent = &g_entities[player->clientNum];

	if ( !name || !*name || strlen( name ) >= MAX_NETNAME )
		return 0;

	Q_strncpyz( oldName, ent->client->pers.netname, sizeof(oldName) );

	ClientCleanName( name, ent->client->pers.netname, sizeof(ent->client->pers.netname) );

	if ( !strcmp( oldName, ent->client->pers.netname ) )
		return 0;

	Q_strncpyz( ent->client->pers.netnameClean, ent->client->pers.netname, sizeof(ent->client->pers.netnameClean) );
	Q_CleanString( ent->client->pers.netnameClean, STRIP_COLOUR );

	// update clientinfo
	trap->GetConfigstring( CS_PLAYERS + player->clientNum, info, sizeof(info) );
	Info_SetValueForKey( info, "n", name );
	trap->SetConfigstring( CS_PLAYERS + player->clientNum, info );

	// update userinfo (in engine)
	trap->GetUserinfo( player->clientNum, info, sizeof(info) );
	Info_SetValueForKey( info, "name", name );
	trap->SetUserinfo( player->clientNum, info );

	// announce if requested
	if ( lua_toboolean( L, 3 ) == 1 ) {
		trap->SendServerCommand( -1, va( "print \"%s"S_COLOR_WHITE" %s %s\n\"", oldName, G_GetStringEdString( "MP_SVGAME",
			"PLRENAME" ), ent->client->pers.netname ) );
	}

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetScore()
//Retn: N/A
static int JPLua_Player_SetScore( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	g_entities[player->clientNum].client->ps.persistant[PERS_SCORE] = lua_tointeger( L, 2 );

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetSpeed( integer speed )
//Retn: N/A
static int JPLua_Player_SetSpeed( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	ent->client->pers.speed = luaL_checkinteger( L, 2 );

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetTeam( string team )
//Retn: N/A
static int JPLua_Player_SetTeam( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *team = luaL_checkstring( L, 2 );

	SetTeam( &g_entities[player->clientNum], team, qtrue );

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetVelocity( Vector3 velocity )
//Retn: N/A
static int JPLua_Player_SetVelocity( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 *v = JPLua_CheckVector( L, 2 );
	gentity_t *ent = &g_entities[player->clientNum];

	VectorCopy( v, &ent->client->ps.velocity );

	return 0;
}
#endif

#ifdef _GAME
//Func: Player:TakeWeapon( integer weaponID )
//Retn: N/A
static int JPLua_Player_TakeWeapon( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	int wp = luaL_checkinteger( L, 2 );
	int i, newWeapon = -1, selectedWeapon = ent->client->ps.weapon;

	if ( wp <= WP_NONE || wp >= WP_NUM_WEAPONS )
		return 0;

	ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << wp);

	for ( i = WP_SABER; i < WP_NUM_WEAPONS; i++ ) {
		if ( ent->client->ps.stats[STAT_WEAPONS] & (1 << i) ) {
			newWeapon = i;
			break;
		}
	}

	if ( newWeapon == WP_NUM_WEAPONS ) {
		for ( i = WP_STUN_BATON; i < WP_SABER; i++ ) {
			if ( ent->client->ps.stats[STAT_WEAPONS] & (1 << i) ) {
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
#endif

#ifdef _GAME
//Func: Player:Teleport( table position{x, y, z}, table angles{pitch, yaw, roll} )
//Retn: N/A
static int JPLua_Player_Teleport( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	vector3 *pos = JPLua_CheckVector( L, 2 ), *angles = JPLua_CheckVector( L, 3 );

	TeleportPlayer( ent, pos, angles );

	return 0;
}
#endif

// Push a Player instance for a client number onto the stack
void JPLua_Player_CreateRef( lua_State *L, int num ) {
	jplua_player_t *player = NULL;

#if defined(_GAME)
	if ( num < 0 || num >= MAX_CLIENTS || !g_entities[num].inuse ) {
#elif defined(_CGAME)
	if ( num < 0 || num >= MAX_CLIENTS || !cgs.clientinfo[num].infoValid ) {
#endif
		lua_pushnil( L );
		return;
	}

	player = (jplua_player_t *)lua_newuserdata( L, sizeof(jplua_player_t) );
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

int JPLua_Player_GetMetaTable( lua_State *L ) {
	luaL_getmetatable( L, PLAYER_META );
	return 1;
}

static const struct luaL_Reg jplua_player_meta[] = {
	{ "__eq", JPLua_Player_Equals },
	{ "__tostring", JPLua_Player_ToString },
	{ "GetAmmo", JPLua_Player_GetAmmo },
	{ "GetAngles", JPLua_Player_GetAngles },
	{ "GetAnimations", JPLua_Player_GetAnimations },
	{ "GetArmor", JPLua_Player_GetArmor },
#ifdef _CGAME
	{ "GetClientInfo", JPLua_Player_GetClientInfo },
#endif
	{ "GetDuelingPartner", JPLua_Player_GetDuelingPartner },
	{ "GetEFlags", JPLua_Player_GetEFlags },
	{ "GetEFlags2", JPLua_Player_GetEFlags2 },
#ifdef _GAME
	{ "GetFlags", JPLua_Player_GetFlags },
#endif
	{ "GetForce", JPLua_Player_GetForce },
	{ "GetHealth", JPLua_Player_GetHealth },
	{ "GetID", JPLua_Player_GetID },
#ifdef _CGAME
	{ "GetLastPickup", JPLua_Player_GetLastPickup },
#endif
	{ "GetLocation", JPLua_Player_GetLocation },
	{ "GetMaxAmmo", JPLua_Player_GetMaxAmmo },
	{ "GetName", JPLua_Player_GetName },
	{ "GetPosition", JPLua_Player_GetPosition },
	{ "GetSaberStyle", JPLua_Player_GetSaberStyle },
#ifdef _GAME
	{ "GetScore", JPLua_Player_GetScore },
	{ "GetUserinfo", JPLua_Player_GetUserinfo },
#endif
	{ "GetVelocity", JPLua_Player_GetVelocity },
	{ "GetWeapon", JPLua_Player_GetWeapon },
#ifdef _GAME
	{ "GiveWeapon", JPLua_Player_GiveWeapon },
	{ "IsAdmin", JPLua_Player_IsAdmin },
#endif
	{ "IsAlive", JPLua_Player_IsAlive },
	{ "IsBot", JPLua_Player_IsBot },
	{ "IsUnderwater", JPLua_Player_IsUnderwater },
	{ "IsWeaponHolstered", JPLua_Player_IsWeaponHolstered },
#ifdef _GAME
	{ "Kick", JPLua_Player_Kick },
	{ "Kill", JPLua_Player_Kill },
#endif
	{ "OnSameTeam", JPLua_Player_OnSameTeam },
#ifdef _GAME
	{ "RemoveEFlag", JPLua_Player_RemoveEFlag },
	{ "RemoveEFlag2", JPLua_Player_RemoveEFlag2 },
	{ "RemoveFlag", JPLua_Player_RemoveFlag },
	{ "SetArmor", JPLua_Player_SetArmor },
	{ "SetEFlag", JPLua_Player_SetEFlag },
	{ "SetEFlag2", JPLua_Player_SetEFlag2 },
	{ "SetFlag", JPLua_Player_SetFlag },
	{ "SetHealth", JPLua_Player_SetHealth },
	{ "SetName", JPLua_Player_SetName },
	{ "SetScore", JPLua_Player_SetScore },
	{ "SetSpeed", JPLua_Player_SetSpeed },
	{ "SetTeam", JPLua_Player_SetTeam },
	{ "SetVelocity", JPLua_Player_SetVelocity },
	{ "TakeWeapon", JPLua_Player_TakeWeapon },
	{ "Teleport", JPLua_Player_Teleport },
#endif
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
	for ( r = jplua_player_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 ); // Pop the Player class metatable from the stack
}

#endif // JPLUA
