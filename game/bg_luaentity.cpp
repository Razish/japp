#if defined(PROJECT_GAME)
#include "g_local.h"
#include "g_admin.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#include "tr_types.h"
#endif
#include "bg_lua.h"
#define MAX_MISC_ENTS	4000

#if defined(PROJECT_CGAME)
extern refEntity_t	MiscEnts[MAX_MISC_ENTS];
extern float		Radius[MAX_MISC_ENTS];
extern float		zOffset[MAX_MISC_ENTS];
extern int			NumMiscEnts;
#endif

#ifdef JPLUA

static const char ENTITY_META[] = "Entity.meta";
static const char REFENTITY_META[] = "RefEntity.meta";

#if defined(PROJECT_GAME)
static jpluaEntity_t *ents = g_entities;
#elif defined(PROJECT_CGAME)
static jpluaEntity_t *ents = cg_entities;
#endif

void JPLua_Entity_CreateRef(lua_State *L, jpluaEntity_t *ent) {
	jplua_entity_t *data = NULL;
#ifdef PROJECT_GAME
	if (ent->inuse) {
		data = (jplua_entity_t *)lua_newuserdata(L, sizeof(jplua_entity_t));
		if (ent->client && ent->s.eType == ET_PLAYER && ent->ID == 0) { // Player entities are created in engine :(
				ent->ID = ent->s.number;
		}
			//data->id = ent->ID;
			data->id = ent->s.number;
	}
	else {
		lua_pushnil(L);
	}
#elif defined PROJECT_CGAME
	if (ent){ /// : dddd
		data = (jplua_entity_t *)lua_newuserdata(L, sizeof(jplua_entity_t));
		data->id = ent->currentState.number;
	}
#endif
	luaL_getmetatable(L, ENTITY_META);
	lua_setmetatable(L, -2);
}
#ifdef PROJECT_CGAME
void JPLua_RefEntity_CreateRef(lua_State *L, refEntity_t *ent) {
	jplua_entity_t *data = NULL;
	data = (jplua_entity_t *)lua_newuserdata(L, sizeof(jplua_entity_t));
	data->id = ent->id;
	luaL_getmetatable(L, REFENTITY_META);
	lua_setmetatable(L, -2);
}
#endif

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

#ifdef PROJECT_CGAME
refEntity_t *JPLua_CheckRefEntity(lua_State *L, int idx) {
	refEntity_t *ent = NULL;
	jplua_entity_t *data;
	void *ud = luaL_checkudata(L, idx, REFENTITY_META);
	luaL_argcheck(L, ud != NULL, 1, "'RefEntity' expected");
	data = (jplua_entity_t *)ud;
	ent = &MiscEnts[data->id];
	return ent;
}
#endif

static int JPLua_Entity_Equals(lua_State *L) {
	jpluaEntity_t *e1 = JPLua_CheckEntity(L, 1), *e2 = JPLua_CheckEntity(L, 2);
#ifdef PROJECT_GAME
	lua_pushboolean(L, (e1->s.number == e2->s.number) ? 1 : 0);
#elif defined PROJECT_CGAME
	lua_pushboolean(L, (e1->currentState.number == e2->currentState.number) ? 1 : 0);
#endif
	return 1;
}

static int JPLua_Entity_ToString(lua_State *L) {
	jpluaEntity_t *entity = JPLua_CheckEntity(L, 1);
#ifdef PROJECT_GAME
	lua_pushfstring(L, "Entity(%d)", entity->s.number);
#elif defined PROJECT_CGAME
	lua_pushfstring(L, "Entity(%d)", entity->currentState.number);
#endif
	return 1;
}

#ifdef PROJECT_CGAME
static int JPLua_RefEntity_Equals(lua_State *L){
	refEntity_t *e1 = JPLua_CheckRefEntity(L, 1), *e2 = JPLua_CheckRefEntity(L, 2);
	lua_pushboolean(L, (e1->id == e2->id) ? 1 : 0);
	return 1;
}

static int JPLua_RefEntity_ToString(lua_State *L){
	refEntity_t *ent = JPLua_CheckRefEntity(L, 1);
	lua_pushfstring(L, "RefEntity(%d)", ent->id);
	return 1;
}
#endif

int JPLua_Entity_Get(lua_State *L){
	int num;
	if (lua_type(L, 1) == LUA_TNUMBER) {
		num = lua_tointeger(L, 1);
		if (num < 0 || num >= MAX_GENTITIES) {
			lua_pushnil(L);
		}
		JPLua_Entity_CreateRef(L, &ents[num]);
	}
	else{
		int idx, i;
		lua_newtable(L);
#ifdef PROJECT_GAME
		for (i = 0, idx = 1; i < level.num_entities; i++) {
			if (&ents[i].inuse) {
				JPLua_Entity_CreateRef(L, &ents[i]);
				lua_rawseti(L, -2, idx++);
			}
		}
#elif defined PROJECT_CGAME
		for (i = 0, idx = 1; i < cg.snap->numEntities; i++) {
			if (&ents[i]) {
				JPLua_Entity_CreateRef(L, &ents[i]);
				lua_rawseti(L, -2, idx++);
			}
		}
#endif
	}
	return 1;
}

#ifdef PROJECT_GAME
int JPLua_Entity_Create(lua_State *L){
	const char *type = lua_tostring(L, 1);
	gentity_t *ent = G_Spawn();
	if (!ent){
		lua_pushnil(L);
		return 0;
	}
	vector3 z = { 0, 0, 0 };
	ent->classname = type;
	VectorCopy( &z, &ent->s.origin );
	ent->jpSpawned = qfalse;
	JPLua_Entity_CreateRef(L, ent);
	return 1;
}
#endif

#ifdef PROJECT_CGAME
int JPLua_Entity_CreateRefEntity(lua_State *L){
	int type = lua_tointeger(L, 1);
	const vector3 *origin = JPLua_CheckVector(L,2);
	const vector3 *scale = JPLua_CheckVector(L,3);
	const vector3 *angle = JPLua_CheckVector(L,4);
	const int zoff = luaL_checkinteger(L,5);

	if ( NumMiscEnts >= MAX_MISC_ENTS ) {
		return 0;
	}

	refEntity_t *RefEnt;
	float *radius, *zOff;
	int modelIndex;
	vector3 mins, maxs;

	radius = &Radius[NumMiscEnts];
	zOff = &zOffset[NumMiscEnts];
	RefEnt = &MiscEnts[NumMiscEnts];
	RefEnt->id = NumMiscEnts++;

	RefEnt->reType = (refEntityType_t)type;
	RefEnt->frame = 0;
	trap->R_ModelBounds(modelIndex, &mins, &maxs);

	VectorCopy(origin, &RefEnt->origin);
	VectorCopy(origin, &RefEnt->lightingOrigin);

	if (angle){
		AnglesToAxis(angle, RefEnt->axis);
	}
	ScaleModelAxis(RefEnt);

	JPLua_RefEntity_CreateRef(L, RefEnt);
	return 1;
}
#endif

static int JPLua_Entity_GetID(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	lua_pushinteger(L, ent->s.number);
#elif defined PROJECT_CGAME
	lua_pushinteger(L, ent->currentState.number);
#endif
	return 1;
}

static int JPLua_Entity_ToPlayer(lua_State *L, jpluaEntity_t *ent){
	if (!ent) {
		lua_pushnil(L);
		return 1;
	}
#ifdef PROJECT_GAME
	if (ent->client && ent->s.eType == ET_PLAYER) {
		JPLua_Player_CreateRef(L, ent->s.number);
	}
#elif defined PROJECT_CGAME
	if (ent->currentState.eType == ET_PLAYER && ent->playerState){
		JPLua_Player_CreateRef(L, ent->currentState.number);
	}
#endif
	else {
		lua_pushnil(L);
	}
	return 1;
}
#ifdef PROJECT_GAME
static int JPLua_Entity_GetLinked(lua_State *L, jpluaEntity_t *ent){
		lua_pushboolean(L, ent->r.linked);
	return 1;
}

static void JPLua_Entity_SetLinked(lua_State *L, jpluaEntity_t *ent){
	qboolean value = lua_toboolean(L, 3);
	if (value && !ent->r.linked && (!ent->client || !ent->NPC)){
		trap->LinkEntity((sharedEntity_t *)ent);
	}
	else if (!value && ent->r.linked && (!ent->client || !ent->NPC)){
		trap->UnlinkEntity((sharedEntity_t *)ent);
	}
}


static int JPLua_Entity_GetUsable(lua_State *L, jpluaEntity_t *ent){
	if ((ent->spawnflags & MOVER_PLAYER_USE) && (ent->r.svFlags & SVF_PLAYER_USABLE)){
		lua_pushboolean(L, 1);
	}
	else{
		lua_pushboolean(L, 2);
	}
	return 1;
}

static void JPLua_Entity_SetUsable(lua_State *L, jpluaEntity_t *ent){
	if (ent->client || ent->NPC) return;
	int value = lua_toboolean(L, 3);
	if (value) {
		ent->spawnflags |= MOVER_PLAYER_USE;
		ent->r.svFlags |= SVF_PLAYER_USABLE;

	}
	else {
		ent->spawnflags &= MOVER_PLAYER_USE;
		ent->r.svFlags &= ~SVF_PLAYER_USABLE;
	}
}

static int JPLua_Entity_GetTouchable(lua_State *L, jpluaEntity_t *ent){
	if (ent->r.contents & CONTENTS_TRIGGER){
		lua_pushboolean(L, 1);
	}
	else{
		lua_pushboolean(L, 0);
	}
	return 1;
}

static void JPLua_Entity_SetTouchable(lua_State *L, jpluaEntity_t *ent){
	if (ent->client || ent->NPC) return;
	int value = lua_toboolean(L, 3);
	if (value) {
		ent->r.contents |= CONTENTS_TRIGGER;
	}
	else {
		ent->r.contents &= CONTENTS_TRIGGER;
	}
	return;
}
#endif

static int JPLua_Entity_GetPosition(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	vector3 pos = ent->r.currentOrigin;
#elif defined PROJECT_CGAME
	vector3 pos = ent->currentState.origin;
#endif
	JPLua_Vector_CreateRef(L, pos.x, pos.y, pos.z);
	return 1;
}

static void JPLua_Entity_SetPosition(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	vector3 *pos = JPLua_CheckVector(L, 3);
	if (ent->r.linked) {
		trap->UnlinkEntity((sharedEntity_t *)ent);
	}
	G_SetOrigin(ent, pos);
	trap->LinkEntity((sharedEntity_t *)ent);
#endif
	return;
}

static int JPLua_Entity_GetAngles(lua_State *L, jpluaEntity_t *ent){
	vector3 orig;
#ifdef PROJECT_GAME
	orig = ent->r.currentAngles;
#elif defined PROJECT_CGAME
	orig = ((int)(ent - ents) == cg.clientNum)
		? cg.predictedPlayerState.origin
		: ent->currentState.pos.trBase;
#endif
	JPLua_Vector_CreateRef(L, orig.x, orig.y, orig.z);
	return 1;
}

static void JPLua_Entity_SetAngles(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	vector3 *ang = JPLua_CheckVector(L, 3);
	if (ent->r.linked) {
		trap->UnlinkEntity((sharedEntity_t *)ent);
	}
	if (ent->client)
	{
		SetClientViewAngle(ent, ang);
	}
	else
	{
		G_SetAngles(ent, ang);
	}
	trap->LinkEntity((sharedEntity_t *)ent);
#endif
	return;
}
#ifdef PROJECT_GAME
static int JPLua_Entity_GetClassName(lua_State *L, jpluaEntity_t *ent){
	lua_pushstring(L, ent->classname);
	return 1;
}
#endif

static int JPLua_Entity_GetModel(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	char buff[1024];
	if (ent->client || ent->NPC) return 0;
	switch (ent->s.eType) {
	case ET_GENERAL:
		trap->GetConfigstring(CS_MODELS + ent->s.modelindex, buff, 1024);
		lua_pushstring(L, buff);
		break;
	case ET_PLAYER:

		break;
	case ET_TELEPORT_TRIGGER:
		if (ent->s.solid == SOLID_BMODEL) {
			// We got a bmodel here
			lua_pushstring(L, va("*%i", ent->s.modelindex));
		}
		else {
			trap->GetConfigstring(CS_MODELS + ent->s.modelindex, buff, 1024);
			lua_pushstring(L, buff);
		}
		break;
	default:
		lua_pushnil(L);
		break;
	}
#elif defined PROJECT_CGAME
	if (ent->currentState.eType == ET_PLAYER){
		const char *configstring = CG_ConfigString( ent->currentState.number + CS_PLAYERS ), *value;
		value = Info_ValueForKey( configstring, "model" );
		lua_pushstring(L, value);
	}
	else{
		const char *model = CG_ConfigString(CS_MODELS + ent->currentState.modelindex);
		lua_pushstring(L, model);
	}
#endif

	return 1;
}

static void JPLua_Entity_SetModel(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	const char *model = luaL_checkstring(L, 3);
	if (ent->client || ent->NPC) return;
	switch (ent->s.eType) {
	case ET_GENERAL:
		ent->s.modelindex = G_ModelIndex(model);
		break;
	case ET_MOVER:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		trap->UnlinkEntity((sharedEntity_t *)ent);
		if (model[0] == '*') {
			trap->SetBrushModel((sharedEntity_t *)ent, model);
			trap->LinkEntity((sharedEntity_t *)ent);
		}
		else {
			if (ent->s.solid == SOLID_BMODEL) {
				ent->s.solid = 0;
			}
			ent->s.modelindex = G_ModelIndex(model);
			trap->LinkEntity((sharedEntity_t *)ent);
		}
		break;
	default:
		lua_pushnil(L);
		break;
	}
#endif
	return;
}

#ifdef PROJECT_GAME
static int JPLua_Entity_GetClipmask(lua_State *L, jpluaEntity_t *ent){
	lua_pushinteger(L, ent->clipmask);
	return 1;
}

static void JPLua_Entity_SetClipmask(lua_State *L, jpluaEntity_t *ent){
	int value = lua_tointeger(L, 3);
	ent->clipmask = value;
	return;
}

extern void CacheChunkEffects(material_t material);
static int JPLua_Entity_GetMaterial(lua_State *L, jpluaEntity_t *ent){
	lua_pushinteger(L, ent->material);
	return 1;
}
static void JPLua_Entity_SetMaterial(lua_State *L, jpluaEntity_t *ent){
	int material = lua_tointeger(L, 3);
	ent->material = (material_t)material;
	CacheChunkEffects(ent->material);
	return;
}

static int JPLua_Entity_GetBreakable(lua_State *L, jpluaEntity_t *ent){
	lua_pushboolean(L,ent->takedamage);
	return 1;
}

static void JPLua_Entity_SetBreakable(lua_State *L, jpluaEntity_t *ent){
	ent->takedamage = qtrue;
	return;
}
#endif

static int JPLua_Entity_GetHealth(lua_State *L, jpluaEntity_t *ent){
	if (!ent) return 0;
#ifdef PROJECT_GAME
	lua_pushinteger(L, ent->health);
#elif defined PROJECT_CGAME
	lua_pushinteger(L, ent->currentState.health);
#endif
	return 1;
}

static void JPLua_Entity_SetHealth(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	int health = lua_tointeger(L,3);
	if (!ent) return;
	ent->health = health;
#endif
	return;
}

static int JPLua_Entity_GetBounds(lua_State *L, jpluaEntity_t *ent){
	vector3 mins, maxs;
#ifdef PROJECT_GAME
	mins = ent->r.mins;
	maxs = ent->r.maxs;
	JPLua_Vector_CreateRef(L, mins.x, mins.y, mins.z);
	JPLua_Vector_CreateRef(L, maxs.x, maxs.y, maxs.z);
#elif defined PROJECT_CGAME
	trap->R_ModelBounds(ent->currentState.modelindex, &mins, &maxs);
#endif
	return 2;
}

static void JPLua_Entity_SetBounds(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	lua_getfield(L, 3, "min");
	vector3 *mins = JPLua_CheckVector(L, -1);
	lua_getfield(L, 3, "max");
	vector3 *maxs = JPLua_CheckVector(L, -1);

	VectorCopy(mins, &ent->r.mins);
	VectorCopy(maxs, &ent->r.maxs);
#endif
	return;
}

#ifdef PROJECT_GAME
static int JPLua_Entity_GetContents(lua_State *L, jpluaEntity_t *ent){
	lua_pushinteger(L, ent->r.contents);
	return 1;
}

static void JPLua_Entity_SetContents(lua_State *L, jpluaEntity_t *ent){
	int value = lua_tointeger(L, 3);
	ent->r.contents = value;
	return;
}
#endif

static int JPLua_Entity_IsNPC(lua_State *L, jpluaEntity_t *ent){
#ifdef PROJECT_GAME
	lua_pushboolean(L, (ent->s.eType == ET_NPC) ? 1 : 0);
#elif defined PROJECT_CGAME
	lua_pushboolean(L, (ent->currentState.eType == ET_NPC) ? 1 : 0);
#endif
	return 1;
}

#ifdef PROJECT_GAME
static int JPLua_Entity_GetSpawnFlags(lua_State *L, jpluaEntity_t *ent){
	lua_pushinteger(L, ent->spawnflags);
	return 1;
}

static void JPLua_Entity_SetSpawnFlags(lua_State *L, jpluaEntity_t *ent){
	int value = lua_tointeger(L, 3);
	ent->spawnflags = value;
	return;
}
#endif

#ifdef PROJECT_GAME
static int JPLua_Entity_SetThinkFunction(lua_State *L){
	jpluaEntity_t *ent = &ents[JPLua_CheckEntity(L, 1)->s.number];
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_think = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetReachedFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_reached = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetBlockedFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_blocked = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetTouchFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_touch = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetUseFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_use = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetPainFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_pain = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}

static int JPLua_Entity_SetDieFunction(lua_State *L){
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	if (lua_type(L, 2) == LUA_TFUNCTION){
		ent->lua_die = luaL_ref(L, LUA_REGISTRYINDEX);
		if (!ent->uselua){
			ent->uselua = qtrue;
		}
	}
	return 0;
}
#endif
static const luaProperty_t entityProperties[] = {
	{ "angles", JPLua_Entity_GetAngles, JPLua_Entity_SetAngles },
	{ "bounds", JPLua_Entity_GetBounds, JPLua_Entity_SetBounds },
#ifdef PROJECT_GAME
	{ "breakable", JPLua_Entity_GetBreakable, JPLua_Entity_SetBreakable },
	{ "classname", JPLua_Entity_GetClassName, NULL },
	{ "clipmask", JPLua_Entity_GetClipmask, JPLua_Entity_SetClipmask },
	{ "contents", JPLua_Entity_GetContents, JPLua_Entity_SetContents },
#endif
	{ "health", JPLua_Entity_GetHealth, JPLua_Entity_SetHealth },
	{ "id", JPLua_Entity_GetID, NULL },
	{ "isnpc", JPLua_Entity_IsNPC, NULL },
#ifdef PROJECT_GAME
	{ "link", JPLua_Entity_GetLinked, JPLua_Entity_SetLinked },
	{ "material", JPLua_Entity_GetMaterial, JPLua_Entity_SetMaterial },
#endif
	{ "model", JPLua_Entity_GetModel, JPLua_Entity_SetModel },
	{ "player", JPLua_Entity_ToPlayer, NULL },
	{ "position", JPLua_Entity_GetPosition, JPLua_Entity_SetPosition },
#ifdef PROJECT_GAME
	{ "spawnflags", JPLua_Entity_GetSpawnFlags, JPLua_Entity_SetSpawnFlags },
	{ "touchable", JPLua_Entity_GetTouchable, JPLua_Entity_SetTouchable },
	{ "usable", JPLua_Entity_GetUsable, JPLua_Entity_SetUsable },
#endif
};

static const size_t numEntityProperties = ARRAY_LEN(entityProperties);

static int JPLua_Entity_Index(lua_State *L) {
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	const char *key = lua_tostring(L, 2);
	int returnValues = 0;

	lua_getmetatable(L, 1);
	lua_getfield(L, -1, key);
	if (!lua_isnil(L, -1)) {
		return 1;
	}

	// assume it's a field
	const luaProperty_t *property = (luaProperty_t *)bsearch(key, entityProperties, numEntityProperties,
		sizeof(luaProperty_t), propertycmp
		);
	if (property) {
		if (property->Get) {
			returnValues += property->Get(L, ent);
		}
	}
	else {
		lua_pushnil(L);
		returnValues++;
	}

	return returnValues;
}

static int JPLua_Entity_NewIndex(lua_State *L) {
	jpluaEntity_t *ent = JPLua_CheckEntity(L, 1);
	const char *key = lua_tostring(L, 2);

	lua_getmetatable(L, 1);
	lua_getfield(L, -1, key);

	if (!lua_isnil(L, -1)) {
		return 1;
	}

	// assume it's a field
	const luaProperty_t *property = (luaProperty_t *)bsearch(key, entityProperties, numEntityProperties,
		sizeof(luaProperty_t), propertycmp
		);
	if (property) {
		if (property->Set) {
			property->Set(L, ent);
		}
	}
	else {
	}

	return 0;
}

#ifdef PROJECT_GAME
void JPLua_Entity_CallFunction(gentity_t *ent,int id,void *arg1, void *arg2, void *arg3, void *arg4){
	if (ent->uselua){
		lua_State *L = JPLua.state;
		switch(id){
		case JPLUA_ENTITY_THINK:
			if (ent->lua_think != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_think);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Call(L, 1, 0);
				break;
			}
		case JPLUA_ENTITY_REACHED:
			if (ent->lua_reached != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_reached);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Call(L, 1, 0);
				break;
			}
		case JPLUA_ENTITY_BLOCKED:
			if (ent->lua_blocked != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_blocked);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg1);
				JPLua_Call(L, 2, 0);
				break;
			}
		case JPLUA_ENTITY_TOUCH:
			if (ent->lua_touch != 0){
				int top, top2, top3;
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_touch);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg1);
				trace_t *tr = (trace_t *)arg2;
				lua_newtable(L);
				top = lua_gettop(L);
				lua_pushstring(L, "allsolid"); lua_pushboolean(L, !!tr->allsolid); lua_settable(L, top);
				lua_pushstring(L, "startsolid"); lua_pushboolean(L, !!tr->startsolid); lua_settable(L, top);
				lua_pushstring(L, "entityNum"); lua_pushinteger(L, tr->entityNum); lua_settable(L, top);
				lua_pushstring(L, "fraction"); lua_pushnumber(L, tr->fraction); lua_settable(L, top);

				lua_pushstring(L, "endpos");
				lua_newtable(L); top2 = lua_gettop(L);
				lua_pushstring(L, "x"); lua_pushnumber(L, tr->endpos.x); lua_settable(L, top2);
				lua_pushstring(L, "y"); lua_pushnumber(L, tr->endpos.y); lua_settable(L, top2);
				lua_pushstring(L, "z"); lua_pushnumber(L, tr->endpos.z); lua_settable(L, top2);
				lua_settable(L, top);

				lua_pushstring(L, "plane");
				lua_newtable(L); top2 = lua_gettop(L);
				lua_pushstring(L, "normal");
				lua_newtable(L); top3 = lua_gettop(L);
				lua_pushstring(L, "x"); lua_pushnumber(L, tr->plane.normal.x); lua_settable(L, top3);
				lua_pushstring(L, "y"); lua_pushnumber(L, tr->plane.normal.y); lua_settable(L, top3);
				lua_pushstring(L, "z"); lua_pushnumber(L, tr->plane.normal.z); lua_settable(L, top3);
				lua_settable(L, top2);
				lua_pushstring(L, "dist"); lua_pushnumber(L, tr->plane.dist); lua_settable(L, top2);
				lua_pushstring(L, "type"); lua_pushinteger(L, tr->plane.type); lua_settable(L, top2);
				lua_pushstring(L, "signbits"); lua_pushinteger(L, tr->plane.signbits); lua_settable(L, top2);
				lua_settable(L, top);

				lua_pushstring(L, "surfaceFlags"); lua_pushinteger(L, tr->surfaceFlags); lua_settable(L, top);
				lua_pushstring(L, "contents"); lua_pushinteger(L, tr->contents); lua_settable(L, top);

				JPLua_Call(L, 3, 0);
				break;
			}
		case JPLUA_ENTITY_USE:
			if (ent->lua_use != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_use);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg1);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg2);
				JPLua_Call(L, 3, 0);
				break;
			}
		case JPLUA_ENTITY_PAIN:
			if (ent->lua_pain != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_pain);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg1);
				lua_pushinteger(L, (int)arg2);
				JPLua_Call(L, 3, 0);
				break;
			}
		case JPLUA_ENTITY_DIE:
			if (ent->lua_die != 0){
				lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_die);
				JPLua_Entity_CreateRef(L, ent);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg1);
				JPLua_Entity_CreateRef(L, (gentity_t *)arg2);
				lua_pushinteger(L, (int)arg3);
				lua_pushinteger(L, (int)arg4);
				JPLua_Call(L, 5, 0);
				break;
			}
		default:
			return;
		}
	}
}

#endif

#ifdef PROJECT_GAME
static int JPLua_Entity_Free(lua_State *L){
	jpluaEntity_t *ent =JPLua_CheckEntity(L, 1);
	if (ent->client){
		return 0; /// Cannot free client entity
	}
	G_FreeEntity(ent);
	return 0;
}

static int JPLua_Entity_Use(lua_State *L){


	return 0;
}

static int JPLua_Entity_PlaySound(lua_State *L){
	gentity_t *ent = JPLua_CheckEntity(L, 1);
	if (!ent) return 0;
	G_EntitySound(ent, lua_tointeger(L, 2), lua_tointeger(L, 3));
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
#endif
	{ NULL, NULL }
};

#ifdef PROJECT_CGAME

static int JPLua_RefEntity_Free(lua_State *L){
	refEntity_t *ent = JPLua_CheckRefEntity(L, 1);
	return 0;
}

static int JPLua_RefEntity_AddToScene(lua_State *L){
	refEntity_t *ent = JPLua_CheckRefEntity(L, 1);
	SE_R_AddRefEntityToScene(ent, MAX_CLIENTS);
	return 0;
}

static const struct luaL_Reg jplua_refentity_meta[] = {
	{ "__eq", JPLua_RefEntity_Equals },
	{ "__tostring", JPLua_RefEntity_ToString },
#ifdef PROJECT_GAME
	{ "Free", JPLua_RefEntity_Free },
	//{ "PlaySound", JPLua_RefEntity_PlaySound },
#endif
	{ NULL, NULL }
};
#endif
void JPLua_Register_Entity(lua_State *L) {
	const luaL_Reg *r;

	luaL_newmetatable(L, ENTITY_META);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	for (r = jplua_entity_meta; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	lua_pop(L, -1);

#ifdef PROJECT_CGAME
	luaL_newmetatable(L, REFENTITY_META);
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);

	for (r = jplua_refentity_meta; r->name; r++) {
		lua_pushcfunction(L, r->func);
		lua_setfield(L, -2, r->name);
	}

	lua_pop(L, -1);
#endif
}

#endif