#if defined(PROJECT_GAME)
#include "g_local.h"
#include "g_admin.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#include "tr_types.h"
#endif
#include "bg_lua.h"

#ifdef JPLUA

static const char ENTITY_META[] = "Entity.meta";

#if defined(PROJECT_GAME)
static jpluaEntity_t *ents = g_entities;
#elif defined(PROJECT_CGAME)
static jpluaEntity_t *ents = cg_entities;
#endif

void JPLua_Entity_CreateRef( lua_State *L, jpluaEntity_t *ent ) {
	jplua_entity_t *data = NULL;
#ifdef PROJECT_GAME
	if ( ent->inuse ) {
		data = (jplua_entity_t *)lua_newuserdata( L, sizeof(jplua_entity_t) );
		if ( ent->client && ent->s.eType == ET_PLAYER && ent->ID == 0 ) {
			// Player entities are created in engine :(
			ent->ID = ent->s.number;
		}
		data->id = ent->s.number;
	}
	else {
		lua_pushnil( L );
	}
#elif defined PROJECT_CGAME
	if ( ent ){ /// : dddd
		data = (jplua_entity_t *)lua_newuserdata( L, sizeof(jplua_entity_t) );
		data->id = ent->currentState.number;
	}
#endif
	luaL_getmetatable( L, ENTITY_META );
	lua_setmetatable( L, -2 );
}

jpluaEntity_t *JPLua_CheckEntity(lua_State *L, int idx) {
	jpluaEntity_t *ent = NULL;
	jplua_entity_t *data;
	void *ud = luaL_checkudata(L, idx, ENTITY_META);
	luaL_argcheck(L, ud != NULL, 1, "'Entity' expected");
	data = (jplua_entity_t *)ud;
	ent = &ents[data->id];
#ifdef PROJECT_GAME
	if (!ent->inuse)
		return NULL;
	if (ent->s.number != data->id)
		return NULL;
#elif defined PROJECT_CGAME
	if (ent->currentState.number != data->id)
		return NULL;
#endif
	return ent;
}

static int JPLua_Entity_Equals( lua_State *L ) {
	const jpluaEntity_t *e1 = JPLua_CheckEntity( L, 1 ), *e2 = JPLua_CheckEntity( L, 2 );

#ifdef PROJECT_GAME
	lua_pushboolean( L, (e1->s.number == e2->s.number) ? 1 : 0);
#elif defined PROJECT_CGAME
	lua_pushboolean( L, (e1->currentState.number == e2->currentState.number) ? 1 : 0 );
#endif

	return 1;
}

static int JPLua_Entity_ToString( lua_State *L ) {
	const jpluaEntity_t *entity = JPLua_CheckEntity( L, 1 );

#ifdef PROJECT_GAME
	lua_pushfstring( L, "Entity(%d)", entity->s.number );
#elif defined PROJECT_CGAME
	lua_pushfstring( L, "Entity(%d)", entity->currentState.number );
#endif

	return 1;
}

int JPLua_Entity_Get( lua_State *L ) {
	if ( lua_type( L, 1 ) == LUA_TNUMBER ) {
		int num = lua_tointeger( L, 1 );
		if ( num < 0 || num >= MAX_GENTITIES ) {
			lua_pushnil( L );
		}
		JPLua_Entity_CreateRef( L, &ents[num] );
	}
	else {
		lua_pushnil( L );
	}
	return 1;
}

#ifdef PROJECT_GAME
int JPLua_Entity_Create( lua_State *L ) {
	gentity_t *ent;

	if (lua_type(L, 1) != LUA_TTABLE) {
		trap->Print("JPLua_Entity_Create failed, not a table\n");
		return 0;
	}

	level.manualSpawning = qtrue;
	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	lua_pushnil(L);
	const char *value, *key;
	for (int i = 0; lua_next(L, 1); i++){
		if (lua_type(L, -2) != LUA_TSTRING) continue; // key can be only string 
		switch (lua_type(L, -1)){
		case LUA_TNONE:
		case LUA_TNIL:
		case LUA_TBOOLEAN:
			continue;
		case LUA_TNUMBER:
			key = luaL_checkstring(L, -2);
			value = luaL_checkstring(L, -1);
			break;
		case LUA_TSTRING:
			key = luaL_checkstring(L, -2);
			value = luaL_checkstring(L, -1);
			break;
		case LUA_TTABLE:
		case LUA_TFUNCTION:
		case LUA_TUSERDATA: /// Vector...Player...Entity...etc
			key = lua_tostring(L, -2);

			if (lua_getmetatable(L, -1)){ // Get Userdata metatable
				luaL_getmetatable(L, "Vector.meta"); // Get metatable for comparing
				JPLua_StackDump(L);
				if (lua_rawequal(L, -1, -2)){
					vector3 *vector = JPLua_CheckVector(L, -3);
					value = va("%.0f %.0f %.0f", vector->x, vector->y, vector->z);
					lua_pop(L, 2);
					break;
				}
				lua_pop(L, 2); // Pop metatables from stack
			}
		case LUA_TTHREAD:
		default:
			lua_pop(L, 1);
			continue;
		}

		level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken(key);
		level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken(value);

		level.numSpawnVars++;
		lua_pop(L, 1);
	}
	ent = G_SpawnGEntityFromSpawnVars(qfalse);

	level.manualSpawning = qfalse;

	if (!ent){
		lua_pushnil(L);
		return 1;
	}
	JPLua_Entity_CreateRef(L, ent);
	return 1;
}
#endif

static int JPLua_Entity_GetID( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
	lua_pushinteger( L, ent->s.number );
#elif defined PROJECT_CGAME
	lua_pushinteger( L, ent->currentState.number );
#endif

	return 1;
}

static int JPLua_Entity_ToPlayer( lua_State *L, jpluaEntity_t *ent ) {
	if ( !ent ) {
		lua_pushnil( L );
		return 1;
	}

#if defined(PROJECT_GAME)
	if ( ent->client && ent->s.eType == ET_PLAYER ) {
		JPLua_Player_CreateRef( L, ent->s.number );
	}
#elif defined(PROJECT_CGAME)
	if ( ent->currentState.eType == ET_PLAYER ) {
		JPLua_Player_CreateRef( L, ent->currentState.number );
	}
#endif
	else {
		lua_pushnil( L );
	}

	return 1;
}

#ifdef PROJECT_GAME
static int JPLua_Entity_GetLinked( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->r.linked );
	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetLinked( lua_State *L, jpluaEntity_t *ent ) {
	qboolean value = lua_toboolean( L, 3 );
	if ( value && !ent->r.linked && (!ent->client || !ent->NPC) ) {
		trap->LinkEntity( (sharedEntity_t *)ent );
	}
	else if ( !value && ent->r.linked && (!ent->client || !ent->NPC) ) {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetUseable( lua_State *L, jpluaEntity_t *ent ) {
	if ( (ent->spawnflags & MOVER_PLAYER_USE) && (ent->r.svFlags & SVF_PLAYER_USABLE) ) {
		lua_pushboolean( L, 1 );
	}
	else {
		lua_pushboolean( L, 0 );
	}

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetUseable( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent->client || ent->NPC ) {
		return;
	}
	qboolean value = lua_toboolean( L, 3 );
	if ( value ) {
		ent->spawnflags |= MOVER_PLAYER_USE;
		ent->r.svFlags |= SVF_PLAYER_USABLE;

	}
	else {
		ent->spawnflags &= ~MOVER_PLAYER_USE;
		ent->r.svFlags &= ~SVF_PLAYER_USABLE;
	}
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetTouchable( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent->r.contents & CONTENTS_TRIGGER ) {
		lua_pushboolean( L, 1 );
	}
	else {
		lua_pushboolean( L, 0 );
	}

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetTouchable( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent->client || ent->NPC ) {
		return;
	}

	qboolean value = lua_toboolean( L, 3 );
	if ( value ) {
		ent->r.contents |= CONTENTS_TRIGGER;
	}
	else {
		ent->r.contents &= ~CONTENTS_TRIGGER;
	}
}
#endif

static int JPLua_Entity_GetPosition( lua_State *L, jpluaEntity_t *ent ) {
#if defined(PROJECT_GAME)
	const vector3 *pos = &ent->r.currentOrigin;
#elif defined(PROJECT_CGAME)
	//JAPPFIXME: this is not a reliable way to get the position of any entity
	const vector3 *pos = &ent->lerpOrigin;
#endif
	JPLua_Vector_CreateRef( L, pos->x, pos->y, pos->z );

	return 1;
}

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetPosition( lua_State *L, jpluaEntity_t *ent ) {
	vector3 *pos = JPLua_CheckVector( L, 3 );
	if ( ent->r.linked ) {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}
	G_SetOrigin( ent, pos );
	trap->LinkEntity( (sharedEntity_t *)ent );
}
#endif

static int JPLua_Entity_GetAngles( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
	const vector3 *angles = &ent->r.currentAngles;
#elif defined PROJECT_CGAME
	const vector3 *angles = ((int)(ent - ents) == cg.clientNum)
		? &cg.predictedPlayerState.viewangles
		: &ent->lerpAngles;
#endif
	JPLua_Vector_CreateRef( L, angles->x, angles->y, angles->z );

	return 1;
}

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetAngles( lua_State *L, jpluaEntity_t *ent ) {
	vector3 *ang = JPLua_CheckVector( L, 3 );

	// unlink
	if ( ent->r.linked ) {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
	}

	// set angles
	if ( ent->client ) {
		SetClientViewAngle( ent, ang );
	}
	else {
		G_SetAngles( ent, ang );
	}

	// re-link
	trap->LinkEntity( (sharedEntity_t *)ent );
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetClassName( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushstring( L, ent->classname );
	return 1;
}
#endif

static int JPLua_Entity_GetModel( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
	char buff[1024] = {};
	if ( ent->client || ent->NPC ) {
		lua_pushnil( L );
		return 1;
	}

	switch ( ent->s.eType ) {
	case ET_GENERAL: {
		trap->GetConfigstring( CS_MODELS + ent->s.modelindex, buff, sizeof(buff) );
		lua_pushstring( L, buff );
	} break;

	case ET_PLAYER: {
	} break;

	case ET_TELEPORT_TRIGGER: {
		if ( ent->s.solid == SOLID_BMODEL ) {
			// we got a bmodel here
			lua_pushstring( L, va( "*%i", ent->s.modelindex ) );
		}
		else {
			trap->GetConfigstring( CS_MODELS + ent->s.modelindex, buff, sizeof(buff) );
			lua_pushstring( L, buff );
		}
	} break;

	default: {
		lua_pushnil( L );
	} break;

	}

#elif defined(PROJECT_CGAME)
	if ( ent->currentState.eType == ET_PLAYER ) {
		const char *configstring = CG_ConfigString( ent->currentState.number + CS_PLAYERS );
		const char *value = Info_ValueForKey( configstring, "model" );
		lua_pushstring( L, value );
	}
	else {
		const char *model = CG_ConfigString( CS_MODELS + ent->currentState.modelindex );
		lua_pushstring( L, model );
	}
#endif

	return 1;
}

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetModel( lua_State *L, jpluaEntity_t *ent ) {
	const char *model = luaL_checkstring( L, 3 );
	if ( ent->client || ent->NPC ) {
		return;
	}

	switch ( ent->s.eType ) {

	case ET_GENERAL: {
		ent->s.modelindex = G_ModelIndex( model );
		G_GetModelBounds(model, &ent->r.mins, &ent->r.maxs);
		ent->r.contents |= CONTENTS_SOLID;
		ent->clipmask |= MASK_SOLID;
	} break;

	case ET_MOVER:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER: {
		trap->UnlinkEntity( (sharedEntity_t *)ent );
		if ( model[0] == '*' ) {
			trap->SetBrushModel( (sharedEntity_t *)ent, model );
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
		else {
			if ( ent->s.solid == SOLID_BMODEL ) {
				ent->s.solid = 0;
			}
			ent->s.modelindex = G_ModelIndex( model );
			trap->LinkEntity( (sharedEntity_t *)ent );
		}
	} break;
	default: {
		lua_pushnil( L );
	} break;

	}
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetClipmask( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, ent->clipmask );

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetClipmask( lua_State *L, jpluaEntity_t *ent ){
	int value = lua_tointeger( L, 3 );
	ent->clipmask = value;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetMaterial( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, ent->material );

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetMaterial( lua_State *L, jpluaEntity_t *ent ) {
	int material = lua_tointeger( L, 3 );
	ent->material = (material_t)material;
	CacheChunkEffects( ent->material );
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetBreakable( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushboolean( L, ent->takedamage );

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetBreakable( lua_State *L, jpluaEntity_t *ent ) {
	qboolean value = lua_toboolean( L, 3 );
	ent->takedamage = value;
}
#endif

static int JPLua_Entity_GetHealth( lua_State *L, jpluaEntity_t *ent ) {
	if ( ent ) {
#if defined(PROJECT_GAME)
		lua_pushinteger( L, ent->health );
#elif defined(PROJECT_CGAME)
		lua_pushinteger( L, ent->currentState.health );
#endif
	}
	else {
		lua_pushnil( L );
	}

	return 1;
}

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetHealth( lua_State *L, jpluaEntity_t *ent ) {
	if ( !ent ) {
		return;
	}

	int health = lua_tointeger( L, 3 );
	ent->health = health;
}
#endif

static int JPLua_Entity_GetBounds(lua_State *L, jpluaEntity_t *ent){
	vector3 mins, maxs;

#if defined(PROJECT_GAME)
	VectorCopy( &ent->r.mins, &mins );
	VectorCopy( &ent->r.maxs, &maxs );
#elif defined(PROJECT_CGAME)
	trap->R_ModelBounds( ent->currentState.modelindex, &mins, &maxs );
#endif

	JPLua_Vector_CreateRef( L, mins.x, mins.y, mins.z );
	JPLua_Vector_CreateRef( L, maxs.x, maxs.y, maxs.z );

	return 2;
}

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetBounds( lua_State *L, jpluaEntity_t *ent ) {
	lua_getfield( L, 3, "mins" );
	vector3 *mins = JPLua_CheckVector( L, -1 );
	lua_getfield( L, 3, "maxs" );
	vector3 *maxs = JPLua_CheckVector( L, -1 );

	//JAPPFIXME: is this all that's required? no re-linking?
	trap->UnlinkEntity((sharedEntity_t*)ent);
	VectorCopy( mins, &ent->r.mins );
	VectorCopy( maxs, &ent->r.maxs );
	trap->LinkEntity((sharedEntity_t*)ent);
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetContents( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, ent->r.contents );
	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetContents( lua_State *L, jpluaEntity_t *ent ) {
	int value = lua_tointeger( L, 3 );
	ent->r.contents = value;
}
#endif

static int JPLua_Entity_IsNPC( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
	lua_pushboolean( L, (ent->s.eType == ET_NPC) ? 1 : 0 );
#elif defined PROJECT_CGAME
	lua_pushboolean( L, (ent->currentState.eType == ET_NPC) ? 1 : 0 );
#endif

	return 1;
}

#if defined(PROJECT_GAME)
static int JPLua_Entity_GetSpawnFlags( lua_State *L, jpluaEntity_t *ent ) {
	lua_pushinteger( L, ent->spawnflags );

	return 1;
}
#endif

#if defined(PROJECT_GAME)
static void JPLua_Entity_SetSpawnFlags( lua_State *L, jpluaEntity_t *ent ) {
	int value = lua_tointeger( L, 3 );
	ent->spawnflags = value;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetThinkFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_think = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetReachedFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_reached = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetBlockedFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_blocked = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetTouchFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_touch = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetUseFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_use = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetPainFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_pain = luaL_ref( L, LUA_REGISTRYINDEX );
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_SetDieFunction( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
		ent->lua_die = luaL_ref(L, LUA_REGISTRYINDEX);
		ent->uselua = qtrue;
	}

	return 0;
}
#endif

static const luaProperty_t entityProperties[] = {
	{
		"angles",
		JPLua_Entity_GetAngles,
#if defined(PROJECT_GAME)
		JPLua_Entity_SetAngles
#elif defined(PROJECT_CGAME)
		nullptr
#endif
	},

	{
		"bounds",
		JPLua_Entity_GetBounds,
#if defined(PROJECT_GAME)
		JPLua_Entity_SetBounds
#elif defined(PROJECT_CGAME)
		nullptr
#endif
	},

#if defined(PROJECT_GAME)
	{
		"breakable",
		JPLua_Entity_GetBreakable,
		JPLua_Entity_SetBreakable
	},
#endif

#if defined(PROJECT_GAME)
	{
		"classname",
		JPLua_Entity_GetClassName,
		NULL
	},
#endif

#if defined(PROJECT_GAME)
	{
		"clipmask",
		JPLua_Entity_GetClipmask,
		JPLua_Entity_SetClipmask
	},
#endif

#if defined(PROJECT_GAME)
	{
		"contents",
		JPLua_Entity_GetContents,
		JPLua_Entity_SetContents
	},
#endif

	{
		"health",
		JPLua_Entity_GetHealth,
#if defined(PROJECT_GAME)
		JPLua_Entity_SetHealth
#elif defined(PROJECT_CGAME)
		nullptr
#endif
	},

	{
		"id",
		JPLua_Entity_GetID,
		NULL
	},

	{
		"isNPC",
		JPLua_Entity_IsNPC,
		NULL
	},

#if defined(PROJECT_GAME)
	{
		"linked",
		JPLua_Entity_GetLinked,
		JPLua_Entity_SetLinked
	},
#endif

#if defined(PROJECT_GAME)
	{
		"material",
		JPLua_Entity_GetMaterial,
		JPLua_Entity_SetMaterial
	},
#endif

	{
		"model",
		JPLua_Entity_GetModel,
#if defined(PROJECT_GAME)
		JPLua_Entity_SetModel
#elif defined(PROJECT_CGAME)
		nullptr
#endif
	},

	{
		"player",
		JPLua_Entity_ToPlayer,
		NULL
	},

	{
		"position",
		JPLua_Entity_GetPosition,
#if defined(PROJECT_GAME)
		JPLua_Entity_SetPosition
#elif defined(PROJECT_CGAME)
		nullptr
#endif
	},

#if defined(PROJECT_GAME)
	{
		"spawnflags",
		JPLua_Entity_GetSpawnFlags,
		JPLua_Entity_SetSpawnFlags
	},
#endif

#if defined(PROJECT_GAME)
	{
		"touchable",
		JPLua_Entity_GetTouchable,
		JPLua_Entity_SetTouchable
	},
#endif

#if defined(PROJECT_GAME)
	{
		"usable",
		JPLua_Entity_GetUseable,
		JPLua_Entity_SetUseable
	},
#endif
};

static const size_t numEntityProperties = ARRAY_LEN( entityProperties );

static int JPLua_Entity_Index( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	const char *key = lua_tostring( L, 2 );
	int returnValues = 0;

	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );
	if ( !lua_isnil( L, -1) ) {
		return 1;
	}

	// assume it's a field
	const luaProperty_t *property = (luaProperty_t *)bsearch(
		key,
		entityProperties,
		numEntityProperties,
		sizeof(luaProperty_t),
		propertycmp
	);
	if ( property ) {
		if ( property->Get ) {
			returnValues += property->Get( L, ent );
		}
	}
	else {
		lua_pushnil( L );
		returnValues++;
	}

	return returnValues;
}

static int JPLua_Entity_NewIndex( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	const char *key = lua_tostring( L, 2 );

	lua_getmetatable( L, 1 );
	lua_getfield( L, -1, key );

	if ( !lua_isnil( L, -1 ) ) {
		return 1;
	}

	// assume it's a field
	const luaProperty_t *property = (luaProperty_t *)bsearch(
		key,
		entityProperties,
		numEntityProperties,
		sizeof(luaProperty_t),
		propertycmp
	);
	if ( property ) {
		if ( property->Set ) {
			property->Set( L, ent );
		}
	}
	else {
		// ...
	}

	return 0;
}

#ifdef PROJECT_GAME
void JPLua_Entity_CallFunction( gentity_t *ent, jplua_entityFunc_t funcID, intptr_t arg1, intptr_t arg2, intptr_t arg3,
	intptr_t arg4 )
{
	if ( ent->uselua ) {
		lua_State *L = JPLua.state;
		switch ( funcID ) {

		case JPLUA_ENTITY_THINK: {
			if ( ent->lua_think ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_think );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Call( L, 1, 0 );
			}
		} break;

		case JPLUA_ENTITY_REACHED: {
			if ( ent->lua_reached ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_reached );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Call( L, 1, 0 );
			}
		} break;

		case JPLUA_ENTITY_BLOCKED: {
			if ( ent->lua_blocked ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_blocked );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg1 );
				JPLua_Call( L, 2, 0 );
				break;
			}
		} break;

		case JPLUA_ENTITY_TOUCH: {
			if ( ent->lua_touch ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_touch );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg1 );
				trace_t *tr = (trace_t *)arg2;

				lua_newtable( L );
				int top = lua_gettop( L );

				lua_pushstring( L, "allsolid" );
					lua_pushboolean( L, !!tr->allsolid );
				lua_settable( L, top );

				lua_pushstring( L, "startsolid" );
					lua_pushboolean( L, !!tr->startsolid );
				lua_settable( L, top );

				lua_pushstring( L, "entityNum" );
					lua_pushinteger( L, tr->entityNum );
				lua_settable( L, top );

				lua_pushstring( L, "fraction" );
					lua_pushnumber( L, tr->fraction );
				lua_settable( L, top );

				lua_pushstring( L, "endpos" );
					lua_newtable( L );
					int top2 = lua_gettop( L );

					lua_pushstring( L, "x" );
						lua_pushnumber( L, tr->endpos.x );
					lua_settable( L, top2 );

					lua_pushstring( L, "y" );
						lua_pushnumber( L, tr->endpos.y );
					lua_settable( L, top2 );

					lua_pushstring( L, "z" );
						lua_pushnumber( L, tr->endpos.z );
					lua_settable( L, top2 );
				lua_settable( L, top );

				lua_pushstring( L, "plane" );
					lua_newtable( L );
					top2 = lua_gettop( L );

					lua_pushstring( L, "normal" );
						lua_newtable( L );
						int top3 = lua_gettop( L );

						lua_pushstring( L, "x" );
							lua_pushnumber( L, tr->plane.normal.x );
						lua_settable( L, top3 );

						lua_pushstring( L, "y" );
							lua_pushnumber( L, tr->plane.normal.y );
						lua_settable( L, top3 );

						lua_pushstring( L, "z" );
							lua_pushnumber( L, tr->plane.normal.z );
						lua_settable( L, top3 );
					lua_settable( L, top2 );

					lua_pushstring( L, "dist" );
						lua_pushnumber( L, tr->plane.dist );
					lua_settable( L, top2 );

					lua_pushstring( L, "type" );
						lua_pushinteger( L, tr->plane.type );
					lua_settable( L, top2 );

					lua_pushstring( L, "signbits" );
						lua_pushinteger( L, tr->plane.signbits );
					lua_settable( L, top2 );
				lua_settable( L, top );

				lua_pushstring( L, "surfaceFlags" );
					lua_pushinteger( L, tr->surfaceFlags );
				lua_settable( L, top );

				lua_pushstring( L, "contents" );
					lua_pushinteger( L, tr->contents );
				lua_settable( L, top );

				JPLua_Call( L, 3, 0 );
				break;
			}
		} break;

		case JPLUA_ENTITY_USE: {
			if ( ent->lua_use ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_use );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg1 );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg2 );
				JPLua_Call( L, 3, 0 );
				break;
			}
		} break;

		case JPLUA_ENTITY_PAIN: {
			if ( ent->lua_pain ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_pain );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg1 );
				lua_pushinteger( L, (int)arg2 );
				JPLua_Call( L, 3, 0 );
				break;
			}
		} break;

		case JPLUA_ENTITY_DIE: {
			if ( ent->lua_die ) {
				lua_rawgeti( L, LUA_REGISTRYINDEX, ent->lua_die );
				JPLua_Entity_CreateRef( L, ent );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg1 );
				JPLua_Entity_CreateRef( L, (gentity_t *)arg2 );
				lua_pushinteger( L, (int)arg3 );
				lua_pushinteger( L, (int)arg4 );
				JPLua_Call( L, 5, 0 );
				break;
			}
		} break;

		default: {
			// ...
		} break;
		}
	}
}

#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_Free( lua_State *L ) {
	jpluaEntity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( !ent->client ) {
		// can't free client entity
		G_FreeEntity( ent );
	}

	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_Use( lua_State *L ) {
	gentity_t *ent = JPLua_CheckEntity(L, 1);
	GlobalUse(ent, ent, ent);
	return 0;
}
#endif

#if defined(PROJECT_GAME)
static int JPLua_Entity_PlaySound( lua_State *L ) {
	gentity_t *ent = JPLua_CheckEntity( L, 1 );
	if ( ent ) {
		G_EntitySound( ent, lua_tointeger( L, 2 ), lua_tointeger( L, 3 ) );
	}
	return 0;
}
#endif
#if defined(PROJECT_GAME)
static int JPLua_Entity_GetBoneVector(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	const char *bone = luaL_checkstring(L, 2);
	mdxaBone_t	boltMatrix;
	vector3     origin, angle;
	if (ent){
		int bolt = trap->G2API_AddBolt(ent->ghoul2, 0, bone);
		if (bolt == -1) {
			trap->Print("^2JPLua:^1Bone %s not found\n", bone);
		}
		VectorSet(&angle, 0, ent->client->ps.viewangles.yaw, 0);
		trap->G2API_GetBoltMatrix(ent->ghoul2, 0, bolt, &boltMatrix, &angle, &ent->r.currentOrigin, level.time, NULL, &ent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, &origin);
		JPLua_Vector_CreateRef(L, &origin);
		return 1;
	}
	return 0;
}
#endif
#if defined(PROJECT_GAME)
static int JPLua_Entity_Scale(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	vector3 *vec = JPLua_CheckVector(L, 2), newmins, newmaxs;
	if (!ent) return 0;
	trap->UnlinkEntity((sharedEntity_t *)ent);
	VectorCopy(vec, &ent->modelScale);
	for (int i = 0; i < 3; i++){
		newmins.raw[i] = ent->r.mins.raw[i] * vec->raw[i];
		newmaxs.raw[i] = ent->r.maxs.raw[i] * vec->raw[i];
	}
	VectorCopy(&newmins, &ent->r.mins);
	VectorCopy(&newmaxs, &ent->r.maxs);
	trap->LinkEntity((sharedEntity_t *)ent);
	return 0;
}
#endif

static const struct luaL_Reg jplua_entity_meta[] = {
	{ "__index", JPLua_Entity_Index },
	{ "__newindex", JPLua_Entity_NewIndex },
	{ "__eq", JPLua_Entity_Equals },
	{ "__tostring", JPLua_Entity_ToString },
#ifdef PROJECT_GAME
	{ "SetThinkFunction", JPLua_Entity_SetThinkFunction },
	{ "SetReachedFunction", JPLua_Entity_SetReachedFunction },
	{ "SetBlockedFunction", JPLua_Entity_SetBlockedFunction },
	{ "SetTouchFunction", JPLua_Entity_SetTouchFunction },
	{ "SetUseFunction", JPLua_Entity_SetUseFunction },
	{ "SetPainFunction", JPLua_Entity_SetPainFunction },
	{ "SetDieFunction", JPLua_Entity_SetDieFunction },
	{ "Free", JPLua_Entity_Free },
	{ "Use", JPLua_Entity_Use },
	{ "PlaySound", JPLua_Entity_PlaySound },
	{ "GetBoneVector", JPLua_Entity_GetBoneVector },
	{ "Scale", JPLua_Entity_Scale },
#endif
	{ NULL, NULL }
};

void JPLua_Register_Entity( lua_State *L ) {
	const luaL_Reg *r;

	luaL_newmetatable( L, ENTITY_META );
	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 );
	lua_settable( L, -3 );

	for ( r = jplua_entity_meta; r->name; r++ ) {
		lua_pushcfunction( L, r->func );
		lua_setfield( L, -2, r->name );
	}

	lua_pop( L, -1 );
}

#endif //JPLUA
