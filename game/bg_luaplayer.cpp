#if defined(_GAME)
#include "g_local.h"
#include "g_admin.h"
#elif defined(_CGAME)
#include "cg_local.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

static const char PLAYER_META[] = "Player.meta";

#if defined(_GAME)
	typedef gentity_t jpluaEntity_t;
	jpluaEntity_t *ents = g_entities;
#elif defined(_CGAME)
	typedef centity_t jpluaEntity_t;
	jpluaEntity_t *ents = cg_entities;
#endif

// Push a Player instance for a client number onto the stack
void JPLua_Player_CreateRef( lua_State *L, int num ) {
	jplua_player_t *player = NULL;

#if defined(_GAME)
	if ( num < 0 || num >= level.maxclients || !g_entities[num].inuse ) {
#elif defined(_CGAME)
	if ( num < 0 || num >= cgs.maxclients || !cgs.clientinfo[num].infoValid ) {
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

#if defined(_GAME)
static int JPLua_Player_GetAdminPrivs( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent->client->pers.adminUser ) {
		lua_pushinteger( L, ent->client->pers.adminUser->privileges );
	}
	else {
		lua_pushinteger( L, ent->client->pers.tempprivs );
	}
	return 1;
}
#endif

static int JPLua_Player_GetAmmo( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: GetAmmo
	return 0;
}

static int JPLua_Player_GetAngles( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const vector3 *angles = &ent->client->ps.viewangles;
#elif defined(_CGAME)
	const vector3 *angles = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState.viewangles
		: &ent->lerpAngles;
#endif

	JPLua_Vector_CreateRef( L, angles->x, angles->y, angles->z );
	return 1;
}

static int JPLua_Player_GetArmor( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.stats[STAT_ARMOR] );
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.stats[STAT_ARMOR] );
	}
	else {
		lua_pushnil( L );
	}
#endif

	return 1;
}

static int JPLua_Player_GetDuelingPartner( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	if ( ent->client->ps.duelInProgress ) {
		JPLua_Player_CreateRef( L, ent->client->ps.duelIndex );
	}
	else {
		lua_pushnil( L );
	}
#elif defined(_CGAME)
	//TODO: GetDuelingPartner
#endif
	return 1;
}

static int JPLua_Player_GetEFlags( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const playerState_t *ps = &ent->client->ps;
#else
	const playerState_t *ps = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState
		: nullptr;
#endif

	if ( ps ) {
		lua_pushinteger( L, ps->eFlags );
	}
	else {
		lua_pushnil( L );
	}

	return 1;
}

static int JPLua_Player_GetEFlags2( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const playerState_t *ps = &ent->client->ps;
#else
	const playerState_t *ps = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState
		: nullptr;
#endif

	if ( ps ) {
		lua_pushinteger( L, ps->eFlags2 );
	}
	else {
		lua_pushnil( L );
	}

	return 1;
}

#if defined(_GAME)
static int JPLua_Player_GetFlags( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, ent->flags );
	return 1;
}
#endif

static int JPLua_Player_GetForce( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const playerState_t *ps = &ent->client->ps;
#else
	const playerState_t *ps = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState
		: nullptr;
#endif

	if ( ps ) {
		lua_pushinteger( L, ps->fd.forcePower );
	}
	else {
		lua_pushnil( L );
	}
	return 1;
}

static int JPLua_Player_GetHealth( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.stats[STAT_HEALTH] );
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.stats[STAT_HEALTH] );
	}
	else {
		if ( cgs.clientinfo[(int)(ent - ents)].team == cg.predictedPlayerState.persistant[PERS_TEAM] ) {
			lua_pushinteger( L, ent->currentState.health );
		}
		else {
			lua_pushnil( L );
		}
	}
#endif
	return 1;
}

static int JPLua_Player_GetID( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, (int)(ent - ents) );
	return 1;
}

#if defined(_GAME)
static int JPLua_Player_GetAdmin( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->client->pers.adminUser ? 1 : 0 );
	return 1;
}
#endif

static int JPLua_Player_GetAlive( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	if ( ent->client->ps.stats[STAT_HEALTH] > 0
		&& !(ent->client->ps.eFlags & EF_DEAD)
		&& ent->client->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR
		&& ent->client->tempSpectate < level.time )
	{
		lua_pushboolean( L, 1 );
		return 1;
	}
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		if ( cg.predictedPlayerState.stats[STAT_HEALTH] > 0
			&& !(cg.predictedPlayerState.eFlags & EF_DEAD)
			&& cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR )
		{
			lua_pushboolean( L, 1 );
			return 1;
		}
	}
	else if ( !(ent->currentState.eFlags & EF_DEAD)
		&& cgs.clientinfo[(int)(ent - ents)].team != TEAM_SPECTATOR )
	{
		lua_pushboolean( L, 1 );
		return 1;
	}
#endif

	lua_pushboolean( L, 0 );
	return 1;
}

static int JPLua_Player_GetBot( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushboolean( L, !!(ent->r.svFlags & SVF_BOT) );
#elif defined(_CGAME)
	lua_pushboolean( L, !!(cgs.clientinfo[(int)(ent - ents)].botSkill != -1) );
#endif
	return 1;
}

static int JPLua_Player_GetDueling( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushboolean( L, ent->client->ps.duelInProgress ? 1 : 0 );
#else
	//TODO: GetDueling
#endif
	return 1;
}

#if defined(_GAME)
static int JPLua_Player_GetEmpowered( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->client->pers.adminData.empowered ? 1 : 0 );
	return 1;
}
#endif

#if defined(_GAME)
static int JPLua_Player_GetGhost( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->client->pers.adminData.isGhost ? 1 : 0 );
	return 1;
}
#endif

static int JPLua_Player_GetHolstered( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	entityState_t *es = &ent->s;
#elif defined(_CGAME)
	entityState_t *es = &ent->currentState;
#endif
	lua_pushboolean( L, es->saberHolstered );
	return 1;
}

#if defined(_GAME)
static int JPLua_Player_GetMerced( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->client->pers.adminData.merc ? 1 : 0 );
	return 1;
}
#endif

static int JPLua_Player_GetProtected( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	entityState_t *es = &ent->s;
#elif defined(_CGAME)
	entityState_t *es = &ent->currentState;
#endif
	lua_pushboolean( L, (es->eFlags & EF_INVULNERABLE) ? 1 : 0 );
	return 1;
}

#if defined(_GAME)
static int JPLua_Player_GetSilenced( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: GetSilenced
	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_GetSlept( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->client->pers.adminData.isSlept ? 1 : 0 );
	return 1;
}
#endif

static int JPLua_Player_GetUnderwater( lua_State *L, jpluaEntity_t *ent ) {
	qboolean underwater = qfalse;
#if defined(_GAME)
	if ( ent->waterlevel == 3 ) {
		underwater = qtrue;
	}
#elif defined(_CGAME)
	const vector3 *pos = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState.origin
		: &ent->currentState.pos.trBase; // not cent->lerpOrigin?
	if ( CG_PointContents( pos, -1 ) & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA) ) {
		underwater = qtrue;
	}
#endif
	lua_pushboolean( L, underwater );
	return 1;
}

#if defined(_CGAME)
static int JPLua_Player_GetLastPickup( lua_State *L, jpluaEntity_t *ent ) {
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushstring( L, CG_GetStringEdString( "SP_INGAME", bg_itemlist[cg.itemPickup].classname ) );
	}
	else {
		lua_pushnil( L );
	}
	return 1;
}
#endif

static int JPLua_Player_GetLegsAnim( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	playerState_t *ps = &g_entities[(int)(ent - ents)].client->ps;
#elif defined(_CGAME)
	playerState_t *ps = &cg.predictedPlayerState;
	entityState_t *es = &cg_entities[(int)(ent - ents)].currentState;
#endif

#if defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
#endif
		lua_pushinteger( L, ps->legsAnim );
#if defined(_CGAME)
	}
	else {
		lua_pushinteger( L, es->legsAnim );
	}
#endif

	return 1;
}

static int JPLua_Player_GetLocation( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	char location[64];
	if ( Team_GetLocationMsg( ent, location, sizeof(location) ) ) {
		lua_pushstring( L, location );
	}
	else {
		lua_pushnil( L );
	}
#elif defined(_CGAME)
	lua_pushstring( L,
		CG_GetLocationString( CG_ConfigString( CS_LOCATIONS + cgs.clientinfo[(int)(ent - ents)].location ) )
	);
#endif

	return 1;
}

static int JPLua_Player_GetMaxAmmo( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_newtable( L );
	int top = lua_gettop( L );
	for ( int i = 1; i < WP_NUM_WEAPONS; i++ ) {
		lua_pushinteger( L, i );
		lua_pushinteger( L, ammoMax[weaponData[i].ammoIndex] );
		lua_settable( L, top );
	}
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_newtable( L );
		int top = lua_gettop( L );
		for ( int i = 1; i < WP_NUM_WEAPONS; i++ ) {
			lua_pushinteger( L, i );
			lua_pushinteger( L, ammoMax[weaponData[i].ammoIndex] );
			lua_settable( L, top );
		}
	}
	else {
		lua_pushnil( L );
	}
#endif

	return 1;
}

static int JPLua_Player_GetName( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushstring( L, ent->client->pers.netname );
#elif defined(_CGAME)
	lua_pushstring( L, cgs.clientinfo[(int)(ent - ents)].name );
#endif
	return 1;
}

static int JPLua_Player_GetPosition( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const vector3 *pos = &ent->client->ps.origin;
#elif defined(_CGAME)
	const vector3 *pos = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState.origin
		: &ent->currentState.pos.trBase; // not cent->lerpOrigin?
#endif

	JPLua_Vector_CreateRef( L, pos->x, pos->y, pos->z );
	return 1;
}

static int JPLua_Player_GetSaberStyle( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.fd.saberDrawAnimLevel );
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.fd.saberDrawAnimLevel );
	}
	else {
		lua_pushnil( L );
	}
#endif

	return 1;
}

static int JPLua_Player_GetScore( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.persistant[PERS_SCORE] );
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.persistant[PERS_SCORE] );
	}
	else {
		lua_pushinteger( L, cg.scores[cgs.clientinfo[(int)(ent - ents)].score].score );
	}
#endif
	return 1;
}

static int JPLua_Player_GetTeam( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->sess.sessionTeam );
#elif defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
		lua_pushinteger( L, cg.predictedPlayerState.persistant[PERS_TEAM] );
	}
	else {
		lua_pushinteger( L, cgs.clientinfo[(int)(ent - ents)].team );
	}
#endif
	return 1;
}

static int JPLua_Player_GetTorsoAnim( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	playerState_t *ps = &g_entities[(int)(ent - ents)].client->ps;
#elif defined(_CGAME)
	playerState_t *ps = &cg.predictedPlayerState;
	entityState_t *es = &cg_entities[(int)(ent - ents)].currentState;
#endif

#if defined(_CGAME)
	if ( (int)(ent - ents) == cg.clientNum ) {
#endif
		lua_pushinteger( L, ps->torsoAnim );
#if defined(_CGAME)
	}
	else {
		lua_pushinteger( L, es->torsoAnim );
	}
#endif
	return 1;
}

static int JPLua_Player_GetVelocity( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	const vector3 *vel = &ent->client->ps.velocity;
#elif defined(_CGAME)
	const vector3 *vel = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState.velocity
		: &ent->currentState.pos.trDelta;
#endif

	JPLua_Vector_CreateRef( L, vel->x, vel->y, vel->z );
	return 1;
}

static int JPLua_Player_GetWeapon( lua_State *L, jpluaEntity_t *ent ) {
#if defined(_GAME)
	lua_pushinteger( L, ent->client->ps.weapon );
#elif defined(_CGAME)
	lua_pushinteger( L, ent->currentState.weapon );
#endif
	return 1;
}

#if defined(_GAME)
static void JPLua_Player_SetAdminPrivs( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent->client->pers.adminUser ) {
		// ...
	}
	else {
		ent->client->pers.tempprivs = luaL_checkinteger( L, 3 );
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetAngles( lua_State *L, jpluaEntity_t *ent ) {
	const vector3 *vec = JPLua_CheckVector( L, 3 );
	VectorCopy( vec, &ent->client->ps.viewangles );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetArmor( lua_State *L, jpluaEntity_t *ent ) {
	ent->client->ps.stats[STAT_ARMOR] = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetEFlags( lua_State *L, jpluaEntity_t *ent ) {
	ent->client->ps.eFlags = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetEFlags2( lua_State *L, jpluaEntity_t *ent ) {
	ent->client->ps.eFlags2 = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetFlags( lua_State *L, jpluaEntity_t *ent ) {
	ent->flags = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetForce( lua_State *L, jpluaEntity_t *ent ) {
	ent->client->ps.fd.forcePower = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetHealth( lua_State *L, jpluaEntity_t *ent ) {
	ent->health = ent->client->ps.stats[STAT_HEALTH] = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetEmpowered( lua_State *L, jpluaEntity_t *ent ) {
	bool doEmpower = luaL_checkinteger( L, 3 ) ? true : false;
	if ( doEmpower ) {
		if ( !ent->client->pers.adminData.empowered ) {
			Empower_On( ent );
		}
	}
	else {
		if ( ent->client->pers.adminData.empowered ) {
			Empower_Off( ent );
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetGhost( lua_State *L, jpluaEntity_t *ent ) {
	bool doGhost = luaL_checkinteger( L, 3 ) ? true : false;
	if ( doGhost ) {
		if ( !ent->client->pers.adminData.isGhost ) {
			Ghost_On( ent );
		}
	}
	else {
		if ( ent->client->pers.adminData.isGhost ) {
			Ghost_Off( ent );
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetHolstered( lua_State *L, jpluaEntity_t *ent ) {
	ent->client->ps.saberHolstered = luaL_checkinteger( L, 3 );
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetMerced( lua_State *L, jpluaEntity_t *ent ) {
	bool doMerc = luaL_checkinteger( L, 3 ) ? true : false;
	if ( doMerc ) {
		if ( !ent->client->pers.adminData.merc ) {
			Merc_On( ent );
		}
	}
	else {
		if ( ent->client->pers.adminData.merc ) {
			Merc_Off( ent );
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetProtected( lua_State *L, jpluaEntity_t *ent ) {
	const bool isProtected = (ent->client->ps.eFlags & EF_INVULNERABLE);
	bool doProtect = luaL_checkinteger( L, 3 );
	if ( doProtect ) {
		if ( !isProtected ) {
			ent->client->ps.eFlags |= EF_INVULNERABLE;
			ent->client->invulnerableTimer = INT32_MAX;
		}
	}
	else {
		if ( isProtected ) {
			ent->client->ps.eFlags &= ~EF_INVULNERABLE;
			ent->client->invulnerableTimer = level.time;
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetSilenced( lua_State *L, jpluaEntity_t *ent ) {
	bool doSilence = luaL_checkinteger( L, 3 ) ? true : false;
	if ( doSilence ) {
		if ( !ent->client->pers.adminData.silenced ) {
			ent->client->pers.adminData.silenced = true;
		}
	}
	else {
		if ( ent->client->pers.adminData.silenced ) {
			ent->client->pers.adminData.silenced = false;
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetSlept( lua_State *L, jpluaEntity_t *ent ) {
	bool doSleep = luaL_checkinteger( L, 3 ) ? true : false;
	if ( doSleep ) {
		if ( !ent->client->pers.adminData.isSlept ) {
			G_SleepClient( ent->client );
		}
	}
	else {
		if ( ent->client->pers.adminData.isSlept ) {
			G_WakeClient( ent->client );
		}
	}
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetLegsAnim( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetLegsAnim
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetName( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetName
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetPosition( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetPosition
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetSaberStyle( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetSaberStyle
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetScore( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetScore
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetTeam( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetTeam
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetTorsoAnim( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetTorsoAnim
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetVelocity( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetVelocity
}
#endif

#if defined(_GAME)
static void JPLua_Player_SetWeapon( lua_State *L, jpluaEntity_t *ent ) {
	//TODO: SetWeapon
}
#endif

static int JPLua_Player_GetJetpackFuel( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	lua_pushinteger(L, ent->client->ps.jetpackFuel);
#elif defined (_CGAME)
	lua_pushinteger(L, cg.predictedPlayerState.jetpackFuel);
#endif
	return 1;
}

static void JPLua_Player_SetJetpackFuel( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	ent->client->ps.jetpackFuel = luaL_checkinteger(L,3);
#endif
}

static int JPLua_Player_GetCloakFuel( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	lua_pushinteger(L, ent->client->ps.cloakFuel);
#elif defined (_CGAME)
	lua_pushinteger(L, cg.predictedPlayerState.cloakFuel);
#endif
	return 1;
}

static void JPLua_Player_SetCloakFuel( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	ent->client->ps.cloakFuel = luaL_checkinteger(L,3);
#endif
}

static int JPLua_Player_IsCloaked( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	if ( ent->client->ps.powerups[PW_CLOAKED] ){
#elif defined (_CGAME)
	if (cg.predictedPlayerState.powerups[PW_CLOAKED]){
#endif
		lua_pushboolean(L, qtrue);
	}else{
		lua_pushboolean(L, qfalse);
	}
	return 1;
}

#ifdef _GAME
void Jedi_Cloak(gentity_t *self);
void Jedi_Decloak(gentity_t *self);
#endif

static int JPLua_Player_SetCloak( lua_State *L, jpluaEntity_t *ent ){
#ifdef _GAME
	qboolean value = lua_toboolean(L,3);

	if (value && !ent->client->ps.powerups[PW_CLOAKED]){
		Jedi_Cloak( ent );
	}else{
		Jedi_Decloak( ent );
	}
#endif
	return 0;
}

typedef int (*getFunc_t)( lua_State *L, jpluaEntity_t *ent );
typedef void (*setFunc_t)( lua_State *L, jpluaEntity_t *ent );

struct playerProperty_t {
	const char		*name;
	getFunc_t		Get;
	setFunc_t		Set;
};

static const playerProperty_t playerProperties [] = {
#if defined(_GAME)
	{
		"adminPrivileges",
		JPLua_Player_GetAdminPrivs,
		JPLua_Player_SetAdminPrivs
	},
#endif
	{
		"ammo",
		JPLua_Player_GetAmmo,
		nullptr
	},
	{
		"angles",
		JPLua_Player_GetAngles,
#if defined(_GAME)
		JPLua_Player_SetAngles
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"armor",
		JPLua_Player_GetArmor,
#if defined(_GAME)
		JPLua_Player_SetArmor
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"cloakfuel",
		JPLua_Player_GetCloakFuel,
		JPLua_Player_SetCloakFuel,
	},
	{
		"duelPartner",
		JPLua_Player_GetDuelingPartner,
		nullptr
	},
	{
		"eFlags",
		JPLua_Player_GetEFlags,
#if defined(_GAME)
		JPLua_Player_SetEFlags
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"eFlags2",
		JPLua_Player_GetEFlags2,
#if defined(_GAME)
		JPLua_Player_SetEFlags2
#elif defined(_CGAME)
		nullptr
#endif
	},
#if defined(_GAME)
	//TODO: move to entity object
	{
		"flags",
		JPLua_Player_GetFlags,
		JPLua_Player_SetFlags
	},
#endif
	{
		"force",
		JPLua_Player_GetForce,
#if defined(_GAME)
		JPLua_Player_SetForce
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"health",
		JPLua_Player_GetHealth,
#if defined(_GAME)
		JPLua_Player_SetHealth
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"id",
		JPLua_Player_GetID,
		NULL
	},
#if defined(_GAME)
	{
		"isAdmin",
		JPLua_Player_GetAdmin,
		NULL
	},
#endif
	{
		"isAlive",
		JPLua_Player_GetAlive,
		NULL
	},
	{
		"isBot",
		JPLua_Player_GetBot,
		NULL
	},
	{
		"isCloaked",
		JPLua_Player_IsCloaked,
		NULL
	},
	{
		"isDueling",
		JPLua_Player_GetDueling,
		NULL
	},
#if defined(_GAME)
	{
		"isEmpowered",
		JPLua_Player_GetEmpowered,
		JPLua_Player_SetEmpowered
	},
#endif
#if defined(_GAME)
	{
		"isGhosted",
		JPLua_Player_GetGhost,
		JPLua_Player_SetGhost
	},
#endif
	{
		"isHolstered",
		JPLua_Player_GetHolstered,
#if defined(_GAME)
		JPLua_Player_SetHolstered
#elif defined(_CGAME)
		nullptr
#endif
	},
#if defined(_GAME)
	{
		"isMerc",
		JPLua_Player_GetMerced,
		JPLua_Player_SetMerced
	},
#endif
	{
		"isProtected",
		JPLua_Player_GetProtected,
#if defined(_GAME)
		JPLua_Player_SetProtected
#elif defined(_CGAME)
		nullptr
#endif
	},
#if defined(_GAME)
	{
		"isSilenced",
		JPLua_Player_GetSilenced,
		JPLua_Player_SetSilenced
	},
#endif
#if defined(_GAME)
	{
		"isSlept",
		JPLua_Player_GetSlept,
		JPLua_Player_SetSlept
	},
#endif
	{
		"isUnderwater",
		JPLua_Player_GetUnderwater,
		NULL
	},
	{   "jetpackfuel",
		JPLua_Player_GetJetpackFuel,
		JPLua_Player_SetJetpackFuel,
	},
	{
		"lastPickup",
#if defined(_GAME)
		nullptr,
#elif defined(_CGAME)
		JPLua_Player_GetLastPickup,
#endif
		NULL
	},
	{
		"legsAnim",
		JPLua_Player_GetLegsAnim,
#if defined(_GAME)
		JPLua_Player_SetLegsAnim
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"location",
		JPLua_Player_GetLocation,
		NULL
	},
	{
		"maxAmmo",
		JPLua_Player_GetMaxAmmo,
		NULL
	},
	{
		"name",
		JPLua_Player_GetName,
#if defined(_GAME)
		JPLua_Player_SetName
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"position",
		JPLua_Player_GetPosition,
#if defined(_GAME)
		JPLua_Player_SetPosition
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"saberStyle",
		JPLua_Player_GetSaberStyle,
#if defined(_GAME)
		JPLua_Player_SetSaberStyle
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"score",
		JPLua_Player_GetScore,
#if defined(_GAME)
		JPLua_Player_SetScore
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"team",
		JPLua_Player_GetTeam,
#if defined(_GAME)
		JPLua_Player_SetTeam
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"torsoAnim",
		JPLua_Player_GetTorsoAnim,
#if defined(_GAME)
		JPLua_Player_SetTorsoAnim
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"velocity",
		JPLua_Player_GetVelocity,
#if defined(_GAME)
		JPLua_Player_SetVelocity
#elif defined(_CGAME)
		nullptr
#endif
	},
	{
		"weapon",
		JPLua_Player_GetWeapon,
#if defined(_GAME)
		JPLua_Player_SetWeapon
#elif defined(_CGAME)
		nullptr
#endif
	},
};
static const size_t numPlayerProperties = ARRAY_LEN( playerProperties );

static int propertycmp( const void *a, const void *b ) {
	return strcmp( (const char *)a, ((playerProperty_t *)b)->name );
}

static int JPLua_Player_Index( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *key = lua_tostring( L, 2 );
	int returnValues = 0;

	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );
	if ( !lua_isnil( L, -1 ) ) {
		return 1;
	}

	// assume it's a field
	const playerProperty_t *property = (playerProperty_t *)bsearch( key, playerProperties, numPlayerProperties,
		sizeof( playerProperty_t ), propertycmp
	);
	if ( property ) {
		if ( property->Get ) {
			returnValues += property->Get( L, ents + player->clientNum );
		}
	}
	else {
		lua_pushnil( L );
		returnValues++;
	}

	return returnValues;
}

static int JPLua_Player_NewIndex( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *key = lua_tostring( L, 2 );

	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );

	if ( !lua_isnil( L, -1 ) ) {
		return 1;
	}

	// assume it's a field
	const playerProperty_t *property = (playerProperty_t *)bsearch( key, playerProperties, numPlayerProperties,
		sizeof( playerProperty_t ), propertycmp
	);
	if ( property ) {
		if ( property->Set ) {
			property->Set( L, ents + player->clientNum );
		}
	}
	else {
	}

	return 0;
}

//Func: GetPlayer( clientNum )
//Retn: Player object
int JPLua_GetPlayer( lua_State *L ) {
#if defined(_GAME)
	int clientNum;

	if ( lua_type( L, 1 ) == LUA_TNUMBER ) {
		clientNum = lua_tointeger( L, 1 );
	}

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
	uint32_t clientsFound = 0;

	if ( lua_type( L, 1 ) == LUA_TNUMBER ) {
		num = lua_tointeger( L, 1 );
	}

	else if ( lua_type( L, 1 ) == LUA_TSTRING ) {
		const char *name = lua_tostring( L, 1 );
		int numFound = 0;
		//RAZTODO: copy G_ClientFromString
		for ( int i = 0; i < cgs.maxclients; i++ ) {
			char nameClean[MAX_NETNAME], nameClean2[MAX_NETNAME];

			if ( !cgs.clientinfo[i].infoValid ) {
				continue;
			}

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
			for ( int i = 0; i < cgs.maxclients; i++ ) {
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

	else {//if ( lua_type( L, 1 ) == LUA_TNIL )
		num = cg.clientNum;
	}

	JPLua_Player_CreateRef( L, num );
	return 1;
#endif
}

//Func: Player1 == Player2
//Retn: boolean value of whether Player1 is the same client as Player2
static int JPLua_Player_Equals( lua_State *L ) {
	jplua_player_t *p1 = JPLua_CheckPlayer( L, 1 );
	jplua_player_t *p2 = JPLua_CheckPlayer( L, 2 );
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

#if defined(_CGAME)
static int JPLua_Player_GetClientInfo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	clientInfo_t *ci = &cgs.clientinfo[player->clientNum];
	entityState_t *es = &cg_entities[player->clientNum].currentState;

	score_t	*score = NULL; // for ping and other scoreboard information
	for ( int i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == player->clientNum ) {
			score = &cg.scores[i];
			break;
		}
	}

	lua_newtable( L );
	int top = lua_gettop( L );

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

#if defined(_GAME)
static int JPLua_Player_GetAdminData( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	lua_newtable( L );
	int top = lua_gettop( L );
	lua_pushstring( L, "login" ); lua_pushstring( L, ent->client->pers.adminUser->user ); lua_settable( L, top );
	lua_pushstring( L, "password" ); lua_pushstring(L, ent->client->pers.adminUser->password ); lua_settable( L, top );
	lua_pushstring( L, "privileges" ); lua_pushinteger(L, ent->client->pers.adminUser->privileges ); lua_settable( L, top );
	lua_pushstring( L, "loginmsg" ); lua_pushstring(L, ent->client->pers.adminUser->loginMsg ); lua_settable( L, top );
	lua_pushstring( L, "rank" ); lua_pushinteger(L, ent->client->pers.adminUser->rank ); lua_settable( L, top );
	lua_pushstring( L, "logineffect" ); lua_pushinteger(L, ent->client->pers.adminUser->logineffect ); lua_settable( L, top );

	return 1;
}
#endif

#if defined(_GAME)
static int JPLua_Player_GetUserinfo( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	char userinfo[MAX_INFO_STRING];
	const char *info;
	const char *name = lua_tostring(L, 2);
	if (lua_isnil(L, 2) || !Q_stricmp(name, "") || !Q_stricmp(name, "-1") || !name ) {
		trap->GetUserinfo(player->clientNum, userinfo, sizeof(userinfo));
		JPLua_PushInfostring(L, userinfo);
		return 1;
	}
	trap->GetUserinfo( player->clientNum, userinfo, sizeof(userinfo) );
	info = Info_ValueForKey(userinfo, name);

	lua_pushstring(L, info);
	return 1;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Give( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	const char *type = luaL_checkstring( L, 2 );
	int id = luaL_checkinteger( L, 3 );
	int value = lua_tointeger( L, 4 );

	if ( !Q_stricmp( type, "weapon" ) ) {
		if ( id == -1 ) {
			ent->client->ps.stats[STAT_WEAPONS] = ((1 << LAST_USEABLE_WEAPON) - 1) & ~1;
		}
		if ( id <= WP_NONE || id >= WP_NUM_WEAPONS ) {
			return 0;
		}

		ent->client->ps.stats[STAT_WEAPONS] |= (1 << id);
	}
	else if ( !Q_stricmp( type, "powerup" ) ) {
		if ( id <= PW_NONE || id >= PW_NUM_POWERUPS ) {
			return 0;
		}

		ent->client->ps.powerups[id] = level.time + value;
	}
	else if ( !Q_stricmp( type, "item" ) ) {
		if ( id == -1 ) {
			for ( int i = 0; i < HI_NUM_HOLDABLE; i++ ) {
				ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << i);
			}
		}
		if ( id <= HI_NONE || id >= HI_NUM_HOLDABLE ) {
			return 0;
		}

		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << id);
	}
	else if ( !Q_stricmp( type, "ammo" ) ) {
		if ( id == -1 ) {
			for ( int i = 0; i < AMMO_MAX; i++ ) {
				ent->client->ps.ammo[i] = ammoMax[i];
			}
		}
		if ( id <= 0 || id >= AMMO_MAX ) {
			return 0;
		}

		ent->client->ps.ammo[id] = value;
	}

	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Kick( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	const char *reason = lua_tostring( L, 2 );

	trap->DropClient( player->clientNum, reason ? reason : "was kicked" );

	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Kill( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = -999;
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );

	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Slap( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer(L, 1);
	gentity_t *ent = &g_entities[player->clientNum];

	Slap(ent);

	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Take( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	const char *type = luaL_checkstring( L, 2 );
	int id = luaL_checkinteger( L, 3 );
	int i, newWeapon = -1, selectedWeapon = ent->client->ps.weapon;

	if ( !Q_stricmp( type, "weapon" ) ) {
		if ( id <= WP_NONE || id >= WP_NUM_WEAPONS ) {
			return 0;
		}

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
			if ( newWeapon == WP_SABER ) {
				newWeapon = WP_NONE;
			}
		}

		ent->client->ps.weapon = (newWeapon == -1) ? 0 : newWeapon;

		G_AddEvent( ent, EV_NOAMMO, selectedWeapon );
	}

	else if ( !Q_stricmp( type, "powerup" ) ) {
		if ( id <= PW_NONE || id >= PW_NUM_POWERUPS ) {
			return 0;
		}

		ent->client->ps.powerups[id] = 0;
	}
	else if ( Q_stricmp( type, "item" ) ) {
		if ( id <= HI_NONE || id >= HI_NUM_HOLDABLE ) {
			return 0;
		}
		ent->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << id);
	}

	return 0;
}
#endif

#if defined(_GAME)
static int JPLua_Player_Teleport( lua_State *L ) {
	jplua_player_t *player = JPLua_CheckPlayer( L, 1 );
	gentity_t *ent = &g_entities[player->clientNum];
	vector3 *pos = JPLua_CheckVector( L, 2 );
	vector3 *angles = JPLua_CheckVector( L, 3 );

	TeleportPlayer( ent, pos, angles );

	return 0;
}
#endif

static const struct luaL_Reg jplua_player_meta[] = {
	{ "__index", JPLua_Player_Index},
	{ "__newindex", JPLua_Player_NewIndex},
	{ "__eq", JPLua_Player_Equals },
	{ "__tostring", JPLua_Player_ToString },
#if defined(_CGAME)
	{ "GetClientInfo", JPLua_Player_GetClientInfo },
#elif defined(_GAME)
	{ "GetAdminData", JPLua_Player_GetAdminData},
	{ "GetUserinfo", JPLua_Player_GetUserinfo },
	{ "Give", JPLua_Player_Give },
	{ "Kick", JPLua_Player_Kick },
	{ "Kill", JPLua_Player_Kill },
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
