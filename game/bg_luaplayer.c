#if defined(_GAME)
#include "g_local.h"
#include "g_admin.h"
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
#endif

	int top, i;
#if defined(_GAME)
	lua_newtable( L );
	top = lua_gettop( L );
	for (i = 1;i < WP_NUM_WEAPONS; i++){
		lua_pushinteger( L, i); lua_pushinteger( L, ent->client->ps.ammo[weaponData[i].ammoIndex] ); lua_settable( L, top );
	}
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum ){
		lua_newtable( L );
		top = lua_gettop( L );
		for (i = 1;i < WP_NUM_WEAPONS; i++){
			lua_pushinteger( L, i); lua_pushinteger( L, cg.predictedPlayerState.ammo[weaponData[i].ammoIndex] ); lua_settable( L, top );
		}
	}else{
		lua_pushnil(L);
	}
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
	int top, i;

#if defined(_GAME)
	lua_newtable( L );
	top = lua_gettop( L );
	for (i = 1;i < WP_NUM_WEAPONS; i++){
		lua_pushinteger( L, i); lua_pushinteger( L, ammoMax[i] ); lua_settable( L, top );
	}
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum ){
		lua_newtable( L );
		top = lua_gettop( L );
		for (i = 1;i < WP_NUM_WEAPONS; i++){
			lua_pushinteger( L, i); lua_pushinteger( L, ammoMax[weaponData[i].ammoIndex] ); lua_settable( L, top );
		}
	}else{
		lua_pushnil(L);
	}
#endif

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

//Func: Player:GetTeam()
//Retn: integer of the player's team
static int JPLua_Player_GetTeam( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	lua_pushinteger( L, g_entities[player->clientNum].client->sess.sessionTeam );
#elif defined(_CGAME)
	if ( player->clientNum == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.persistant[PERS_TEAM] );
	}
	else {
		lua_pushnil( L );
	}
#endif
	return 1;
}

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
//Func: Player:Give( char type, int id, int amount )
//Retn: N/A
static int JPLua_Player_Give( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	const char *type = luaL_checkstring(L,2);
	int id = luaL_checkinteger( L, 3 );
	int value = lua_tointeger(L, 4);

	if (!Q_stricmp( type, "weapon" )){
		if (id == -1){
			ent->client->ps.stats[STAT_WEAPONS] = ((1 << LAST_USEABLE_WEAPON) - 1) & ~1;
		}
		if ( id <= WP_NONE ||id >= WP_NUM_WEAPONS ){
			return 0;
		}

		ent->client->ps.stats[STAT_WEAPONS] |= (1 << id);
	}else if (!Q_stricmp( type, "powerup" )){
		if ( id <= PW_NONE ||id >= PW_NUM_POWERUPS ){
			return 0;
		}

		ent->client->ps.powerups[id] = level.time + value;
	}else if (!Q_stricmp( type, "item" )){
		if (id == -1){
			for (int i = 0; i < HI_NUM_HOLDABLE; i++ ){
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			}
		}
		if ( id <= HI_NONE ||id >= HI_NUM_HOLDABLE ){
			return 0;
		}

		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << id);
	
	}else if (!Q_stricmp( type, "ammo" )){
		if (id == -1){
			for (int i = 0; i < AMMO_MAX; i++ ) {
				ent->client->ps.ammo[i] = ammoMax[i];
			}
		}
		if ( id <= 0 ||id >= AMMO_MAX ){
			return 0;
		}

		ent->client->ps.ammo[id] = value;
	}

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
	}
	else {
		lua_pushboolean( L, 0 );
	}

	return 1;
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
	uint32_t bit = luaL_checkinteger( L, 2 );
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
	uint32_t bit = luaL_checkinteger( L, 2 );
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
	uint32_t bit = luaL_checkinteger( L, 2 );
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

//Func: Player:SetArmor()
//Retn: N/A
static int JPLua_Player_SetArmor( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int armour = luaL_checkinteger( L, 3 );

	if ( armour < 0 )
		armour = 0;

	g_entities[player->clientNum].client->ps.stats[STAT_ARMOR] = armour;
#endif
	return 0;
}


//Func: Player:SetEFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetEFlag( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = luaL_checkinteger( L, 3 );
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
#endif
	return 0;
}

//Func: Player:SetEFlag2( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetEFlag2( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = luaL_checkinteger( L, 3 );
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
#endif
	return 0;
}

#ifdef _GAME
//Func: Player:SetFlag( integer bit )
//Retn: string errorMsg if it failed, otherwise nothing
static int JPLua_Player_SetFlag( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	uint32_t bit = luaL_checkinteger( L, 3 );
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

//Func: Player:SetForce( integer points )
//Retn: N/A
static int JPLua_Player_SetForce( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int points = luaL_checkinteger( L, 3 );

	g_entities[player->clientNum].client->ps.fd.forcePower = points;
#endif
	return 0;
}

//Func: Player:SetHealth()
//Retn: N/A
static int JPLua_Player_SetHealth( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	int hp = luaL_checkinteger( L, 3 );
	gentity_t *ent = &g_entities[player->clientNum];

	if ( hp < 1 )
		return 0;

	ent->health = ent->client->ps.stats[STAT_HEALTH] = hp;
#endif
	return 0;
}

//Func: Player:SetName( string name )
//Retn: N/A
static int JPLua_Player_SetName( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *name = luaL_checkstring( L, 3 );
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

	if ( CheckDuplicateName( player->clientNum ) ) {
		Q_strncpyz( ent->client->pers.netnameClean, ent->client->pers.netname, sizeof(ent->client->pers.netnameClean) );
		Q_CleanString( ent->client->pers.netnameClean, STRIP_COLOUR );
	}

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
#endif
	return 0;
}

#ifdef _GAME
//Func: Player:SetScore()
//Retn: N/A
static int JPLua_Player_SetScore( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );

	g_entities[player->clientNum].client->ps.persistant[PERS_SCORE] = luaL_checkinteger( L, 3 );
	return 0;
}
#endif

#ifdef _GAME
//Func: Player:SetSpeed( integer speed )
//Retn: N/A
static int JPLua_Player_SetSpeed( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	ent->client->pers.speed = luaL_checkinteger( L, 3 );
	return 0;
}
#endif

//Func: Player:SetTeam( string team )
//Retn: N/A
static int JPLua_Player_SetTeam( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *team = luaL_checkstring( L, 3 );

	SetTeam( &g_entities[player->clientNum], team, qtrue );
#endif
	return 0;
}

//Func: Player:SetVelocity( Vector3 velocity )
//Retn: N/A
static int JPLua_Player_SetVelocity( lua_State *L ) {
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 *v = JPLua_CheckVector( L, 3 );
	gentity_t *ent = &g_entities[player->clientNum];

	VectorCopy( v, &ent->client->ps.velocity );
#endif
	return 0;
}

#ifdef _GAME
//Func: Player:Take( integer weaponID )
//Retn: N/A
static int JPLua_Player_Take( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	const char *type = luaL_checkstring( L, 2 );
	int id = luaL_checkinteger( L, 3 );
	int i, newWeapon = -1, selectedWeapon = ent->client->ps.weapon;

	if (Q_stricmp(type, "weapon")){

		if ( id <= WP_NONE || id >= WP_NUM_WEAPONS )
			return 0;

		ent->client->ps.stats[STAT_WEAPONS] &= ~(1 << id);

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
	}else if (Q_stricmp(type, "powerup")){
		if ( id <= PW_NONE ||id >= PW_NUM_POWERUPS ){
			return 0;
		}

		ent->client->ps.powerups[id] = 0;
	}else if (Q_stricmp(type, "item")){
		if ( id <= HI_NONE ||id >= HI_NUM_HOLDABLE ){
			return 0;
		}
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << id);
	}
	return 0;
}

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


static int JPLua_Player_Sleep(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	qboolean value = lua_toboolean(L,3);
	if (value){
		G_SleepClient(ent->client);
	}else{
		G_WakeClient(ent->client);
	}
#endif
	return 0;
}

static int JPLua_Player_Ghost(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	qboolean value = lua_toboolean(L, 3);
	if (value){
		ent->client->pers.adminData.isGhost = qtrue;
		ent->r.contents = CONTENTS_BODY;
		ent->clipmask = 267009/*CONTENTS_SOLID*/;
		ent->client->ps.fd.forcePowersKnown |= (1 << NUM_FORCE_POWERS); // JA++ client prediction
	}else{
		ent->client->pers.adminData.isGhost = qfalse;
		ent->r.contents = CONTENTS_SOLID;
		ent->clipmask = CONTENTS_SOLID | CONTENTS_BODY;
		ent->client->ps.fd.forcePowersKnown &= ~(1 << NUM_FORCE_POWERS); // JA++ client prediction
	}
	trap->LinkEntity( (sharedEntity_t *)ent );
#endif
	return 0;
}

static int JPLua_Player_Grant(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	int value = luaL_checkinteger(L, 3);
	ent->client->pers.tempprivs = value;
#endif
	return 0;
}

static int JPLua_Player_Empower(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	qboolean value = lua_toboolean(L, 3);
	if (value){
		ent->client->pers.adminData.empowered = qtrue;
		Empower_On(ent);
	}else{
		ent->client->pers.adminData.empowered = qfalse;
		Empower_Off(ent);
	}
#endif
	return 0;
}

static int JPLua_Player_Merc(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	qboolean value = lua_toboolean(L, 3);
	if (value){
		ent->client->pers.adminData.merc = qtrue;
		Merc_On(ent);
	}else{
		ent->client->pers.adminData.merc = qfalse;
		Merc_Off(ent);
	}
#endif
	return 0;
}

static int JPLua_Player_Slap(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	Slap(ent);
#endif
	return 0;
}

static int JPLua_Player_Silence(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	qboolean value = lua_toboolean(L, 3);
	if (value){
		ent->client->pers.adminData.silenced = qtrue;
	}else{
		ent->client->pers.adminData.silenced = qfalse;
	}
#endif
	return 0;
}

static int JPLua_Player_Protect(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	int value = lua_toboolean(L, 3);
	if (value >= 1){
		ent->client->ps.eFlags ^= EF_INVULNERABLE;
		ent->client->invulnerableTimer = value;
	}else{
		ent->client->ps.eFlags &= ~EF_INVULNERABLE;
		ent->client->invulnerableTimer = 0;
	}
#endif
	return 0;
}

static int JPLua_Player_GetAdminPrivs(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	adminUser_t *user = ent->client->pers.adminUser;

	if (user){
		lua_pushinteger(L, user->privileges);
	}else if (ent->client->pers.tempprivs){
		lua_pushinteger(L, ent->client->pers.tempprivs);
	}else{
		lua_pushinteger(L, 0);
	}
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_SetPosition(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	vector3 *pos = JPLua_CheckVector( L, 3 );
	TeleportPlayer( ent, pos, &ent->client->ps.viewangles );
#endif
	return 0;
}

static int JPLua_Player_IsSleep(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushboolean(L, ent->client->pers.adminData.isSlept);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_GetSpeed(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushinteger(L,ent->client->pers.speed);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_IsSilenced(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushboolean(L, ent->client->pers.adminData.silenced);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_IsMerced(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushboolean(L, ent->client->pers.adminData.merc);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_IsProtected(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	if ((ent->client->ps.eFlags & EF_INVULNERABLE) && ent->client->invulnerableTimer >= 1 ){
		lua_pushboolean(L, qtrue);
	}else{
		lua_pushboolean(L, qfalse);
	}
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_IsEmpowered(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushboolean(L, ent->client->pers.adminData.empowered);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_IsGhost(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	lua_pushboolean(L, ent->client->pers.adminData.isGhost);
	return 1;
#else
	return 0;
#endif
}

static int JPLua_Player_SetWeapon(lua_State *L){
#ifdef _GAME
	 jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	 int id = luaL_checkinteger(L,3);
	 g_entities[player->clientNum].client->ps.weapon = id;
#endif
	 return 0;
}

static int JPLua_Player_SetAngles(lua_State *L){
#ifdef _GAME
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	vector3 *vec = JPLua_CheckVector(L,3);
	VectorCopy(vec, &g_entities[player->clientNum].client->ps.viewangles); 
#endif
	return 0;
}

static int JPLua_Player_IsDueling(lua_State *L){
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
#if defined(_GAME)
	playerState_t *ps = &g_entities[player->clientNum].client->ps;
	lua_pushboolean(L, ps->duelInProgress);
#elif defined(_CGAME)
	if ( !cg_entities[player->clientNum].currentState.bolt1 || player->clientNum != cg.clientNum )
		lua_pushboolean(L, qfalse);
	else
		lua_pushboolean(L, qtrue);
#endif
	return 1;
}

#ifdef _GAME
static int JPLua_Player_GetAdminData(lua_State *L){
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	int top, i;

	lua_newtable( L );
	top = lua_gettop( L );
	lua_pushstring( L, "login"); lua_pushstring( L, ent->client->pers.adminUser->user ); lua_settable( L, top );
	lua_pushstring(L, "password"); lua_pushstring(L, ent->client->pers.adminUser->password); lua_settable(L, top);
	lua_pushstring(L, "privileges"); lua_pushinteger(L, ent->client->pers.adminUser->privileges); lua_settable(L, top);
	lua_pushstring(L, "loginmsg"); lua_pushstring(L, ent->client->pers.adminUser->loginMsg); lua_settable(L, top);
	lua_pushstring(L, "rank"); lua_pushinteger(L, ent->client->pers.adminUser->rank); lua_settable(L, top);
	lua_pushstring(L, "logineffect"); lua_pushinteger(L, ent->client->pers.adminUser->logineffect); lua_settable(L, top);
	return 1;
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


static const struct jplua_player_func_s funcs [] = { 
	{ "alive", JPLua_Player_IsAlive, NULL },
	{ "anim", JPLua_Player_GetAnimations, NULL },
	{ "angles", JPLua_Player_GetAngles, JPLua_Player_SetAngles },
	{ "ammo", JPLua_Player_GetAmmo, NULL},
	{ "armor", JPLua_Player_GetArmor, JPLua_Player_SetArmor },
	{ "bot", JPLua_Player_IsBot, NULL },
	{ "duelpartner", JPLua_Player_GetDuelingPartner, NULL},
	{ "eflags", JPLua_Player_GetEFlags, JPLua_Player_SetEFlag },
	{ "eflags2", JPLua_Player_GetEFlags2, JPLua_Player_SetEFlag2 },
#ifdef _GAME
	{ "empower", JPLua_Player_IsEmpowered, JPLua_Player_Empower },
	{ "flags", JPLua_Player_GetFlags, JPLua_Player_SetFlag },
#endif
	{ "force", JPLua_Player_GetForce, JPLua_Player_SetForce },
#ifdef _GAME
	{ "ghost", JPLua_Player_IsGhost, JPLua_Player_Ghost },
#endif
	{ "health", JPLua_Player_GetHealth, JPLua_Player_SetHealth },
	{ "id", JPLua_Player_GetID, NULL },
#ifdef _GAME
	{ "isadmin", JPLua_Player_IsAdmin, NULL },
#endif
	{ "isdueling", JPLua_Player_IsDueling, NULL},
	{ "isholstered", JPLua_Player_IsWeaponHolstered, NULL},
#ifdef _CGAME
	{ "lastpickup", JPLua_Player_GetLastPickup, NULL},
#endif
	{ "location", JPLua_Player_GetLocation, NULL },
	{ "maxammo", JPLua_Player_GetMaxAmmo, NULL},
#ifdef _GAME
	{ "merc", JPLua_Player_IsMerced, JPLua_Player_Merc },
#endif
	{ "name", JPLua_Player_GetName, JPLua_Player_SetName },
	{ "position", JPLua_Player_GetPosition, JPLua_Player_SetPosition },
#ifdef _GAME
	{ "privileges", JPLua_Player_GetAdminPrivs, JPLua_Player_Grant },
	{ "protect", JPLua_Player_IsProtected, JPLua_Player_Protect },
#endif
	{ "saberstyle", JPLua_Player_GetSaberStyle, NULL },
#ifdef _GAME
	{ "score", JPLua_Player_GetScore, JPLua_Player_SetScore },
	{ "silence", JPLua_Player_IsSilenced, JPLua_Player_Silence },
	{ "sleep", JPLua_Player_IsSleep, JPLua_Player_Sleep },
	{ "speed", JPLua_Player_GetSpeed, JPLua_Player_SetSpeed },
#endif
	{ "team", JPLua_Player_GetTeam, JPLua_Player_SetTeam },
	{ "underwater", JPLua_Player_IsUnderwater, NULL },
	{ "velocity", JPLua_Player_GetVelocity, JPLua_Player_SetVelocity},
	{ "weapon", JPLua_Player_GetWeapon, JPLua_Player_SetWeapon },
};
static const size_t numMetaMethods = ARRAY_LEN( funcs );

static int metacmp( const void *a, const void *b ) {
	return Q_stricmp( (const char *)a, ((jplua_player_func_t*)b)->name );
}

static int JPLua_Player_Index( lua_State *L ) {
	jplua_player_func_t *method = NULL;
	const char *key = lua_tostring( L, 2 );
	int ret;

	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );
	if ( !lua_isnil( L, -1 ) )
		return 1;

	method = (jplua_player_func_t *)bsearch( key, funcs, numMetaMethods, sizeof(funcs[0]), metacmp );
	if (!method) {
		return 0;
	}
	ret = method->getfunc(L);
	return ret;

}

static int JPLua_Player_NewIndex( lua_State *L ) {
	jplua_player_func_t *method = NULL;
	const char *key = lua_tostring( L, 2 );
	int ret;

	method = (jplua_player_func_t *)bsearch( key, funcs, numMetaMethods, sizeof(funcs[0]), metacmp );
	if (!method) {
		return 0;
	}
	ret = method->setfunc(L);
	return ret;

}

static const struct luaL_Reg jplua_player_meta[] = {
	{ "__index", JPLua_Player_Index},
	{ "__newindex", JPLua_Player_NewIndex},
	{ "__eq", JPLua_Player_Equals },
	{ "__tostring", JPLua_Player_ToString },
#ifdef _CGAME
	{ "GetClientInfo", JPLua_Player_GetClientInfo },
#endif
#ifdef _GAME
	{ "GetAdminData", JPLua_Player_GetAdminData},
	{ "GetUserinfo", JPLua_Player_GetUserinfo },
	{ "Ghost", JPLua_Player_Ghost},
	{ "Give", JPLua_Player_Give },
	{ "Kick", JPLua_Player_Kick },
	{ "Kill", JPLua_Player_Kill },
	{ "RemoveEFlag", JPLua_Player_RemoveEFlag },
	{ "RemoveEFlag2", JPLua_Player_RemoveEFlag2 },
	{ "RemoveFlag", JPLua_Player_RemoveFlag },
	{ "Slap", JPLua_Player_Slap},
	{ "Take", JPLua_Player_Take },
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
