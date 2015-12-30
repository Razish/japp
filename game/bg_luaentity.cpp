#if defined(PROJECT_GAME)
#include "g_local.h"
#include "g_admin.h"
#elif defined(PROJECT_CGAME)
#include "cg_local.h"
#include "tr_types.h"
#endif
#include "bg_luainternal.h"

#ifdef JPLUA

void BG_ParseField( const BG_field_t *l_fields, int numFields, const char *key, const char *value, byte *ent );
extern BG_field_t fields[88];

namespace JPLua {

	static const char ENTITY_META[] = "Entity.meta";

#if defined(PROJECT_GAME)
	static jpluaEntity_t *ents = g_entities;
#elif defined(PROJECT_CGAME)
	static jpluaEntity_t *ents = cg_entities;
#endif

	int EntityPropertyCompare( const void *a, const void *b ) {
		return strcmp( (const char *)a, ((entityProperty_t *)b)->name );
	}

	void Entity_CreateRef( lua_State *L, jpluaEntity_t *ent ) {
		luaEntity_t *data = NULL;
#ifdef PROJECT_GAME
		if (ent && ent->inuse ) {
			data = (luaEntity_t *)lua_newuserdata( L, sizeof(luaEntity_t) );
			if ( ent->client && ent->s.eType == ET_PLAYER && ent->ID == 0 ) {
				// Player entities are created in engine :(
				ent->ID = ent->s.number;
			}
			data->id = ent->s.number;
		}
		else {
			lua_pushnil( L );
			return;
		}
#elif defined PROJECT_CGAME
		if ( ent ){ /// : dddd
			data = (luaEntity_t *)lua_newuserdata( L, sizeof(luaEntity_t) );
			data->id = ent->currentState.number;
		}
#endif
		luaL_getmetatable( L, ENTITY_META );
		lua_setmetatable( L, -2 );
	}

	jpluaEntity_t *CheckEntity(lua_State *L, int idx) {
		jpluaEntity_t *ent = NULL;
		luaEntity_t *data;
		void *ud = luaL_checkudata(L, idx, ENTITY_META);
		luaL_argcheck(L, ud != NULL, 1, "'Entity' expected");
		data = (luaEntity_t *)ud;
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

	static int Entity_Equals( lua_State *L ) {
		const jpluaEntity_t *e1 = CheckEntity( L, 1 ), *e2 = CheckEntity( L, 2 );

#ifdef PROJECT_GAME
		lua_pushboolean( L, (e1->s.number == e2->s.number) ? 1 : 0);
#elif defined PROJECT_CGAME
		lua_pushboolean( L, (e1->currentState.number == e2->currentState.number) ? 1 : 0 );
#endif

		return 1;
	}

	static int Entity_ToString( lua_State *L ) {
		const jpluaEntity_t *entity = CheckEntity( L, 1 );

#ifdef PROJECT_GAME
		lua_pushfstring( L, "Entity(%d)", entity->s.number );
#elif defined PROJECT_CGAME
		lua_pushfstring( L, "Entity(%d)", entity->currentState.number );
#endif

		return 1;
	}

	int Entity_Get( lua_State *L ) {
		if ( lua_type( L, 1 ) == LUA_TNUMBER ) {
			int num = lua_tointeger( L, 1 );
			if ( num < 0 || num >= MAX_GENTITIES ) {
				lua_pushnil( L );
				return 1;
			}
			Entity_CreateRef( L, &ents[num] );
		}
		else {
			lua_pushnil( L );
		}
		return 1;
	}

#if defined(PROJECT_GAME)
	int FindEntityByClassName( lua_State *L ) {
		const char *match = luaL_checkstring( L, 1 );
		gentity_t *list[MAX_GENTITIES], *found = g_entities;

		int count = 0;
		while ( (found = G_Find( found, FOFS( classname ), match )) != NULL ) {
			list[count++] = found;
		}
		if ( count != 0 ) {
			lua_newtable( L );
			int top = lua_gettop( L );
			for ( int i = 0; i < count; i++ ) {
				lua_pushinteger( L, i + 1 );
				Entity_CreateRef( L, list[count] );
				lua_settable( L, top );
			}
			return 1;
		}
		return 0;
	}
#endif

#ifdef PROJECT_GAME
	int Entity_Create( lua_State *L ) {
		gentity_t *ent;
		if ( lua_gettop(L) == 0  ) { // kill me please
			ent = G_Spawn();
		}else if( !lua_type(L, 1) == LUA_TTABLE){
			trap->Print( "Entity_Create failed, not a table\n" );
			return 0;
		}else{
			level.manualSpawning = qtrue;
			level.numSpawnVars = 0;
			level.numSpawnVarChars = 0;

			lua_pushnil( L );
			const char *value, *key;
			for ( int i = 0; lua_next( L, 1 ); i++ ) {
				if ( lua_type( L, -2 ) != LUA_TSTRING ) {
					continue; // key can be only string
				}
				switch ( lua_type( L, -1 ) ) {

				case LUA_TNONE:
				case LUA_TNIL:
				case LUA_TBOOLEAN: {
					continue;
				} break;

				case LUA_TSTRING:
				case LUA_TNUMBER: {
					key = luaL_checkstring(L, -2);
					value = luaL_checkstring(L, -1);
				} break;

				case LUA_TTABLE:
				case LUA_TFUNCTION:
				case LUA_TUSERDATA: {
					// Vector, Player, Entity etc
					key = lua_tostring( L, -2 );

					if ( lua_getmetatable( L, -1 ) ) {
						// get userdata metatable
						luaL_getmetatable( L, "Vector.meta" ); // get metatable for comparing
						if ( lua_rawequal( L, -1, -2 ) ) {
							vector3 *vector = CheckVector( L, -3 );
							value = va( "%.0f %.0f %.0f", vector->x, vector->y, vector->z );
							lua_pop( L, 2 );
							break;
						}
						lua_pop( L, 2 ); // pop metatable from stack
					}
				} // fall through

				case LUA_TTHREAD:
				default: {
					lua_pop( L, 1 );
					continue;
				} break;

				}

				level.spawnVars[level.numSpawnVars][0] = G_AddSpawnVarToken( key );
				level.spawnVars[level.numSpawnVars][1] = G_AddSpawnVarToken( value );

				level.numSpawnVars++;
				lua_pop( L, 1 );
			}
			ent = G_SpawnGEntityFromSpawnVars( qfalse );

			level.manualSpawning = qfalse;
		}
		if ( !ent ) {
			lua_pushnil( L );
			return 1;
		}
		Entity_CreateRef( L, ent );
		return 1;
	}
#endif

	static int Entity_GetID( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
		lua_pushinteger( L, ent->s.number );
#elif defined PROJECT_CGAME
		lua_pushinteger( L, ent->currentState.number );
#endif

		return 1;
	}

	static int Entity_ToPlayer( lua_State *L, jpluaEntity_t *ent ) {
		if ( !ent ) {
			lua_pushnil( L );
			return 1;
		}

#if defined(PROJECT_GAME)
		if ( ent->client && ent->s.eType == ET_PLAYER ) {
			Player_CreateRef( L, ent->s.number );
		}
#elif defined(PROJECT_CGAME)
		if ( ent->currentState.eType == ET_PLAYER ) {
			Player_CreateRef( L, ent->currentState.number );
		}
#endif
		else {
			lua_pushnil( L );
		}

		return 1;
	}

#ifdef PROJECT_GAME
	static int Entity_GetLinked( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushboolean( L, ent->r.linked );
		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetLinked( lua_State *L, jpluaEntity_t *ent ) {
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
	static int Entity_GetUseable( lua_State *L, jpluaEntity_t *ent ) {
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
	static void Entity_SetUseable( lua_State *L, jpluaEntity_t *ent ) {
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
	static int Entity_GetTouchable( lua_State *L, jpluaEntity_t *ent ) {
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
	static void Entity_SetTouchable( lua_State *L, jpluaEntity_t *ent ) {
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

	static int Entity_GetPosition( lua_State *L, jpluaEntity_t *ent ) {
#if defined(PROJECT_GAME)
		const vector3 *pos = &ent->r.currentOrigin;
#elif defined(PROJECT_CGAME)
		//JAPPFIXME: this is not a reliable way to get the position of any entity
		const vector3 *pos = &ent->lerpOrigin;
#endif
		Vector_CreateRef( L, pos->x, pos->y, pos->z );

		return 1;
	}

#if defined(PROJECT_GAME)
	static void Entity_SetPosition( lua_State *L, jpluaEntity_t *ent ) {
		vector3 *pos = CheckVector( L, 3 );
		if ( ent->r.linked ) {
			trap->UnlinkEntity( (sharedEntity_t *)ent );
		}
		G_SetOrigin( ent, pos );
		trap->LinkEntity( (sharedEntity_t *)ent );
	}
#endif

	static int Entity_GetAngles( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
		const vector3 *angles = &ent->r.currentAngles;
#elif defined PROJECT_CGAME
		const vector3 *angles = ((int)(ent - ents) == cg.clientNum)
			? &cg.predictedPlayerState.viewangles
			: &ent->lerpAngles;
#endif
		Vector_CreateRef( L, angles->x, angles->y, angles->z );

		return 1;
	}

#if defined(PROJECT_GAME)
	static void Entity_SetAngles( lua_State *L, jpluaEntity_t *ent ) {
		vector3 *ang = CheckVector( L, 3 );

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
	static int Entity_GetClassName( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushstring( L, ent->classname );
		return 1;
	}
	
	static void Entity_SetClassName( lua_State *L, jpluaEntity_t *ent ) {
		const char *name = lua_tostring(L,3);
		ent->classname = BG_StringAlloc(name);
	}
#endif

	static int Entity_GetModel( lua_State *L, jpluaEntity_t *ent ) {
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
	static void Entity_SetModel( lua_State *L, jpluaEntity_t *ent ) {
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
	static int Entity_GetClipmask( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushinteger( L, ent->clipmask );

		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetClipmask( lua_State *L, jpluaEntity_t *ent ){
		int value = lua_tointeger( L, 3 );
		ent->clipmask = value;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_GetMaterial( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushinteger( L, ent->material );

		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetMaterial( lua_State *L, jpluaEntity_t *ent ) {
		int material = lua_tointeger( L, 3 );
		ent->material = (material_t)material;
		CacheChunkEffects( ent->material );
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_GetBreakable( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushboolean( L, ent->takedamage );

		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetBreakable( lua_State *L, jpluaEntity_t *ent ) {
		qboolean value = lua_toboolean( L, 3 );
		ent->takedamage = value;
	}
#endif

	static int Entity_GetHealth( lua_State *L, jpluaEntity_t *ent ) {
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
	static void Entity_SetHealth( lua_State *L, jpluaEntity_t *ent ) {
		if ( !ent ) {
			return;
		}

		int health = lua_tointeger( L, 3 );
		ent->health = health;
	}
#endif

	static int Entity_GetBounds(lua_State *L, jpluaEntity_t *ent){
		vector3 mins, maxs;

#if defined(PROJECT_GAME)
		VectorCopy( &ent->r.mins, &mins );
		VectorCopy( &ent->r.maxs, &maxs );
#elif defined(PROJECT_CGAME)
		trap->R_ModelBounds( ent->currentState.modelindex, &mins, &maxs );
#endif
		lua_newtable(L); int top = lua_gettop(L);
		lua_pushstring(L, "mins");Vector_CreateRef( L, &mins);lua_settable(L, top);
		lua_pushstring(L, "maxs");Vector_CreateRef( L, &maxs);lua_settable(L, top);

		return 1;
	}

#if defined(PROJECT_GAME)
	static void Entity_SetBounds( lua_State *L, jpluaEntity_t *ent ) {
		lua_getfield( L, 3, "mins" );
		vector3 *mins = CheckVector( L, -1 );
		lua_pop(L, 1);
		lua_getfield( L, 3, "maxs" );
		vector3 *maxs = CheckVector( L, -1 );
		lua_pop(L, 1);

		trap->UnlinkEntity((sharedEntity_t*)ent);
		VectorCopy( mins, &ent->r.mins );
		VectorCopy( maxs, &ent->r.maxs );
		trap->LinkEntity((sharedEntity_t*)ent);
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_GetContents( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushinteger( L, ent->r.contents );
		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetContents( lua_State *L, jpluaEntity_t *ent ) {
		int value = lua_tointeger( L, 3 );
		ent->r.contents = value;
	}
#endif

	static int Entity_IsNPC( lua_State *L, jpluaEntity_t *ent ) {
#ifdef PROJECT_GAME
		lua_pushboolean( L, (ent->s.eType == ET_NPC) ? 1 : 0 );
#elif defined PROJECT_CGAME
		lua_pushboolean( L, (ent->currentState.eType == ET_NPC) ? 1 : 0 );
#endif

		return 1;
	}

#if defined(PROJECT_GAME)
	static int Entity_GetSpawnFlags( lua_State *L, jpluaEntity_t *ent ) {
		lua_pushinteger( L, ent->spawnflags );

		return 1;
	}
#endif

#if defined(PROJECT_GAME)
	static void Entity_SetSpawnFlags( lua_State *L, jpluaEntity_t *ent ) {
		int value = lua_tointeger( L, 3 );
		ent->spawnflags = value;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetThinkFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_think = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetReachedFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_reached = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetBlockedFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_blocked = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetTouchFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_touch = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetUseFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_use = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetPainFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_pain = luaL_ref( L, LUA_REGISTRYINDEX );
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined(PROJECT_GAME)
	static int Entity_SetDieFunction( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if ( lua_type( L, 2 ) == LUA_TFUNCTION ) {
			ent->lua_die = luaL_ref(L, LUA_REGISTRYINDEX);
			ent->uselua = qtrue;
		}

		return 0;
	}
#endif

#if defined (PROJECT_GAME)
	static int Entity_GetLight(lua_State *L, jpluaEntity_t *ent){
		int light = ent->s.constantLight;
		int r, g, b, i;
		r = light & 255;
		g = (light >> 8) & 255;
		b = (light >> 16) & 255;
		i = ((light >> 24) & 255) * 4;

		lua_newtable(L);
		int top = lua_gettop(L);

		lua_pushstring(L, "r"); lua_pushnumber(L, r); lua_settable(L, top);
		lua_pushstring(L, "g"); lua_pushnumber(L, g); lua_settable(L, top);
		lua_pushstring(L, "b"); lua_pushnumber(L, b); lua_settable(L, top);
		lua_pushstring(L, "intensity"); lua_pushnumber(L, i); lua_settable(L, top);
		return 1;
	}

	static void Entity_SetLight(lua_State *L, jpluaEntity_t *ent){
		float rgbi[4];
		int		r, g, b, i;
		ReadColour(rgbi, 4, L, 3);

		r = rgbi[0] * 255;
		if (r > 255) {
			r = 255;
		}
		g = rgbi[1] * 255;
		if (g > 255) {
			g = 255;
		}
		b = rgbi[2] * 255;
		if (b > 255) {
			b = 255;
		}
		i = rgbi[3] / 4;
		if (i > 255) {
			i = 255;
		}
		ent->s.constantLight = r | (g << 8) | (b << 16) | (i << 24);
		trap->LinkEntity((sharedEntity_t *)ent);
	}

#endif

#ifdef PROJECT_GAME
	static int Entity_GetTrajectory(lua_State *L, jpluaEntity_t *ent){
		lua_newtable(L);
		int top = lua_gettop(L);

		lua_pushstring(L, "trType"); lua_pushinteger(L, ent->s.pos.trType); lua_settable(L, top);
		lua_pushstring(L, "trTime"); lua_pushinteger(L, ent->s.pos.trTime); lua_settable(L, top);
		lua_pushstring(L, "trDuration"); lua_pushinteger(L, ent->s.pos.trDuration); lua_settable(L, top);
		lua_pushstring(L, "trBase"); Vector_CreateRef(L, &ent->s.pos.trBase); lua_settable(L, top);
		lua_pushstring(L, "trDelta"); Vector_CreateRef(L, &ent->s.pos.trDelta); lua_settable(L, top);
		return 1;
	}

	static void Entity_SetTrajectory(lua_State *L, jpluaEntity_t *ent){
		if ( lua_type( L, 3) != LUA_TTABLE ) {
			trap->Print( "JPLua::Entity_SetTrajectory failed, not a table\n" );
			return;
		}
		lua_getfield( L, 3, "trType" );
			ent->s.pos.trType = (trType_t)lua_tointeger(L, -1);
			lua_pop(L,1);
		lua_getfield( L, 3, "trTime" );
			ent->s.pos.trTime = lua_tointeger(L, -1);
			lua_pop(L,1);
		lua_getfield( L, 3, "trDuration" );
			ent->s.pos.trDuration = lua_tointeger(L, -1);
			lua_pop(L,1);
		lua_getfield( L, 3, "trBase" );
		VectorCopy(CheckVector(L,-1), &ent->s.pos.trBase);
			lua_pop(L,1);
		lua_getfield( L, 3, "trDelta" );
			VectorCopy(CheckVector(L,-1), &ent->s.pos.trDelta);
			lua_pop(L,1);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetFlags(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->flags);
		return 1;
	}

	static void Entity_SetFlags(lua_State *L, jpluaEntity_t *ent){
		ent->flags = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetDamage(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->damage);
		return 1;
	}

	static void Entity_SetDamage(lua_State *L, jpluaEntity_t *ent){
		ent->damage = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetEType(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->s.eType);
		return 1;
	}

	static void Entity_SetEType(lua_State *L, jpluaEntity_t *ent){
		ent->s.eType = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetSVFlags(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->r.svFlags);
		return 1;
	}

	static void Entity_SetSVFlags(lua_State *L, jpluaEntity_t *ent){
		ent->r.svFlags = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetWeapon(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->s.weapon);
		return 1;
	}

	static void Entity_SetWeapon(lua_State *L, jpluaEntity_t *ent){
		ent->s.weapon = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetParent(lua_State *L, jpluaEntity_t *ent){
		Entity_CreateRef(L, ent->parent);
		return 1;
	}

	static void Entity_SetParent(lua_State *L, jpluaEntity_t *ent){
		gentity_t *parent = CheckEntity(L, 3);
		ent->parent = parent;
		ent->r.ownerNum = parent->s.number;
	}
#endif
#ifdef PROJECT_GAME
	static int Entity_GetNextthink(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->nextthink);
		return 1;
	}

	static void Entity_SetNextthink(lua_State *L, jpluaEntity_t *ent){
		ent->nextthink = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetPos_2(lua_State *L, jpluaEntity_t *ent){ //crying(
		lua_newtable(L);
		int top = lua_gettop(L);
		lua_pushstring(L, "pos1");Vector_CreateRef(L, &ent->pos1);lua_settable(L, top);
		lua_pushstring(L, "pos2");Vector_CreateRef(L, &ent->pos2);lua_settable(L, top);
		lua_pushstring(L, "pos3");Vector_CreateRef(L, &ent->pos3);lua_settable(L, top);
		return 1;
	}

	static void Entity_SetPos_2(lua_State *L, jpluaEntity_t *ent){
		if ( lua_type( L, 3) != LUA_TTABLE ) {
			trap->Print( "JPLua::Entity_SetPos_2 failed, not a table\n" );
			return;
		}
		lua_getfield( L, 3, "pos1" );
		VectorCopy(CheckVector(L,-1), &ent->pos1);
		lua_pop(L,1);
		lua_getfield( L, 3, "pos2" );
		VectorCopy(CheckVector(L,-1), &ent->pos2);
		lua_pop(L,1);
		lua_getfield( L, 3, "pos3" );
		VectorCopy(CheckVector(L,-1), &ent->pos3);
		lua_pop(L,1);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetPhysicsObject(lua_State *L, jpluaEntity_t *ent){
		lua_pushboolean(L, ent->physicsObject);
		return 1;
	}

	static void Entity_SetPhysicsObject(lua_State *L, jpluaEntity_t *ent){
		ent->physicsObject = lua_toboolean(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetBounceCount(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->bounceCount);
		return 1;
	}
	
	static void Entity_SetBounceCount(lua_State *L, jpluaEntity_t *ent){
		ent->bounceCount = lua_tointeger(L, 3);
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_GetMOD(lua_State *L, jpluaEntity_t *ent){
		lua_pushinteger(L, ent->methodOfDeath);
		lua_pushinteger(L, ent->splashMethodOfDeath);
		return 2;
	}
	
	static void Entity_SetMOD(lua_State *L, jpluaEntity_t *ent){
		if ( lua_type( L, 3) != LUA_TTABLE ) {
			trap->Print( "JPLua::Entity_GetMOD failed, not a table\n" );
			return;
		}
		lua_getfield( L, 3, "methodOfDeath" );
			ent->methodOfDeath = lua_tointeger(L, 3);
			lua_pop(L,1);
		lua_getfield( L, 3, "splashMethodOfDeath" );
			ent->splashMethodOfDeath = lua_tointeger(L, 3);
			lua_pop(L,1);
	}

#endif
	static const entityProperty_t entityProperties[] = {
		{
			"angles",
			Entity_GetAngles,
#if defined(PROJECT_GAME)
			Entity_SetAngles
#elif defined(PROJECT_CGAME)
			nullptr
#endif
		},
		
#if defined(PROJECT_GAME)
		{
			"bounceCount",
			Entity_GetBounceCount,
			Entity_SetBounceCount
		},
#endif

		{
			"bounds",
			Entity_GetBounds,
#if defined(PROJECT_GAME)
			Entity_SetBounds
#elif defined(PROJECT_CGAME)
			nullptr
#endif
		},

#if defined(PROJECT_GAME)
		{
			"breakable",
			Entity_GetBreakable,
			Entity_SetBreakable
		},
#endif

#if defined(PROJECT_GAME)
		{
			"classname",
			Entity_GetClassName,
			Entity_SetClassName
		},
#endif

#if defined(PROJECT_GAME)
		{
			"clipmask",
			Entity_GetClipmask,
			Entity_SetClipmask
		},
#endif

#if defined(PROJECT_GAME)
		{
			"contents",
			Entity_GetContents,
			Entity_SetContents
		},
#endif

#if defined(PROJECT_GAME)
		{
			"damage",
			Entity_GetDamage,
			Entity_SetDamage
		},
#endif

#if defined(PROJECT_GAME)
		{
			"eType",
			Entity_GetEType,
			Entity_SetEType
		},
#endif

#if defined(PROJECT_GAME)
		{
			"flags",
			Entity_GetFlags,
			Entity_SetFlags
		},
#endif
		{
			"health",
			Entity_GetHealth,
#if defined(PROJECT_GAME)
			Entity_SetHealth
#elif defined(PROJECT_CGAME)
			nullptr
#endif
		},

		{
			"id",
			Entity_GetID,
			NULL
		},

		{
			"isNPC",
			Entity_IsNPC,
			NULL
		},

#if defined(PROJECT_GAME)
		{
			"light",
			Entity_GetLight,
			Entity_SetLight
		},
		{
			"linked",
			Entity_GetLinked,
			Entity_SetLinked
		},
#endif

#if defined(PROJECT_GAME)
		{
			"material",
			Entity_GetMaterial,
			Entity_SetMaterial
		},
#endif

#if defined(PROJECT_GAME)
		{
			"methodOfDeath",
			Entity_GetMOD,
			Entity_SetMOD
		},
#endif

		{
			"model",
			Entity_GetModel,
#if defined(PROJECT_GAME)
			Entity_SetModel
#elif defined(PROJECT_CGAME)
			nullptr
#endif
		},
#if defined(PROJECT_GAME)
		{
			"nextthink",
			Entity_GetNextthink,
			Entity_SetNextthink
		},
#endif
#if defined(PROJECT_GAME)
		{
			"parent",
			Entity_GetParent,
			Entity_SetParent
			},
#endif

#if defined(PROJECT_GAME)
		{
			"physicsObject",
			Entity_GetPhysicsObject,
			Entity_SetPhysicsObject
		},
#endif

		{
			"player",
			Entity_ToPlayer,
			NULL
		},

		{
			"position",
			Entity_GetPosition,
#if defined(PROJECT_GAME)
			Entity_SetPosition
#elif defined(PROJECT_CGAME)
			nullptr
#endif
		},
#if defined(PROJECT_GAME)
		{
			"position_2",
			Entity_GetPos_2,
			Entity_SetPos_2
		},
#endif

#if defined(PROJECT_GAME)
		{
			"spawnflags",
			Entity_GetSpawnFlags,
			Entity_SetSpawnFlags
		},
#endif

#if defined(PROJECT_GAME)
		{
			"svFlags",
			Entity_GetSVFlags,
			Entity_SetSVFlags
		},
#endif

#if defined(PROJECT_GAME)
		{
			"touchable",
			Entity_GetTouchable,
			Entity_SetTouchable
		},
#endif

#if defined(PROJECT_GAME)
		{
			"trajectory",
			Entity_GetTrajectory,
			Entity_SetTrajectory
		},
#endif

#if defined(PROJECT_GAME)
		{
			"usable",
			Entity_GetUseable,
			Entity_SetUseable
		},
#endif
#if defined(PROJECT_GAME)
		{
			"weapon",
			Entity_GetWeapon,
			Entity_SetWeapon
		},
#endif
	};

	static const size_t numEntityProperties = ARRAY_LEN( entityProperties );

	static int Entity_Index( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		const char *key = lua_tostring( L, 2 );
		int returnValues = 0;

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );
		if ( !lua_isnil( L, -1) ) {
			return 1;
		}

		// assume it's a field
		const entityProperty_t *property = (entityProperty_t *)bsearch(
			key,
			entityProperties,
			numEntityProperties,
			sizeof(entityProperty_t),
			EntityPropertyCompare
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

	static int Entity_NewIndex( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		const char *key = lua_tostring( L, 2 );

		lua_getmetatable( L, 1 );
		lua_getfield( L, -1, key );

		if ( !lua_isnil( L, -1 ) ) {
			return 1;
		}

		// assume it's a field
		const entityProperty_t *property = (entityProperty_t *)bsearch(
			key,
			entityProperties,
			numEntityProperties,
			sizeof(entityProperty_t),
			EntityPropertyCompare
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


#if defined(PROJECT_GAME)
	static int Entity_Free( lua_State *L ) {
		jpluaEntity_t *ent = CheckEntity( L, 1 );
		if (ent && !ent->client ) {
			// can't free client entity
			G_FreeEntity( ent );
		}

		return 0;
	}

	static int Entity_Use( lua_State *L ) {
		gentity_t *ent = CheckEntity(L, 1);
		if (!ent) return 0;
		GlobalUse(ent, ent, ent);
		return 0;
	}

	static int Entity_PlaySound( lua_State *L ) {
		gentity_t *ent = CheckEntity( L, 1 );
		if ( ent ) {
			G_EntitySound( ent, lua_tointeger( L, 2 ), lua_tointeger( L, 3 ) );
		}
		return 0;
	}
#endif

	static int Entity_GetBoneVector(lua_State *L){
		jpluaEntity_t *ent = CheckEntity(L, 1);
		const char *bone = luaL_checkstring(L, 2);
		mdxaBone_t	boltMatrix;
		vector3     origin, angle;
		if (ent){
			int bolt = trap->G2API_AddBolt(ent->ghoul2, 0, bone);
			if (bolt == -1) {
				trap->Print("^2ls.^1Bone %s not found\n", bone);
				return 0;
			}
#ifdef PROJECT_GAME
			VectorSet(&angle, 0, ent->client->ps.viewangles.yaw, 0);
			trap->G2API_GetBoltMatrix(ent->ghoul2, 0, bolt, &boltMatrix, &angle, &ent->r.currentOrigin, level.time, NULL, &ent->modelScale);
#else
			VectorSet(&angle, 0,((int)(ent - ents) == cg.clientNum)
				? cg.predictedPlayerState.viewangles.yaw
				: ent->lerpAngles.yaw, 0);
			trap->G2API_GetBoltMatrix(ent->ghoul2, 0, bolt, &boltMatrix, &angle, &ent->lerpOrigin, cg.time, cgs.gameModels, &ent->modelScale);
#endif
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, &origin);
			Vector_CreateRef(L, &origin);
			return 1;
		}
		return 0;
	}

#ifdef PROJECT_GAME
	static int Entity_Scale(lua_State *L){
		jpluaEntity_t *ent = CheckEntity(L, 1);
		int value = luaL_checkinteger(L, 2);
		if (!ent) return 0;

		ent->s.iModelScale = Q_clampi(0, value, 1023);

		if (ent->client)
		ent->client->ps.iModelScale = ent->s.iModelScale;

		float fScale = ent->s.iModelScale / 100.0f;
		ent->modelScale.x = ent->modelScale.y = ent->modelScale.z = fScale;
		VectorScale(&ent->r.mins, fScale, &ent->r.mins);
		VectorScale(&ent->r.maxs, fScale, &ent->r.maxs);

		trap->LinkEntity((sharedEntity_t *)ent);
		G_CheckInSolid(ent, qtrue); // check
		return 0;
	}

	static int Entity_SetVar(lua_State *L){
		jpluaEntity_t *ent = CheckEntity(L, 1);
		if (!ent) return 0;
		const char *key = luaL_checkstring(L, 2);
		const char *value = luaL_checkstring(L, 3);
		BG_ParseField(fields, ARRAY_LEN(fields), key, value, (byte *)ent);
		return 0;
	}

	static int spawncmp( const void *a, const void *b ) {
		return Q_stricmp( (const char *)a, ((BG_field_t*)b)->name );
	}


	static int Entity_GetVar(lua_State *L){
		byte *ent = (byte *)CheckEntity(L, 1);
		if (!ent) return 0;
		const char *key = luaL_checkstring(L,2);

		const BG_field_t *f = (BG_field_t *)bsearch( key, fields, ARRAY_LEN(fields), sizeof(BG_field_t), spawncmp );

		if ( f ) {
			switch ( f->type ) {
			case F_LSTRING:
				lua_pushstring(L, *(char **)(ent + f->ofs));
				break;
			case F_VECTOR:
				Vector_CreateRef(L, ((float *)(ent + f->ofs))[0], ((float *)(ent + f->ofs))[1], ((float *)(ent + f->ofs))[2]);
				break;
			case F_INT:
				lua_pushinteger(L, *(int *)(ent + f->ofs));
				break;
			case F_FLOAT:
				lua_pushnumber(L, *(float *)(ent + f->ofs));
				break;
			case F_IGNORE:
			default:
				return 0;
			}
			return 1;
		}
		return 0;
	}
#endif

#ifdef PROJECT_GAME
	static int Entity_RunObject(lua_State *L){
		gentity_t *ent = CheckEntity(L, 1);
		if (ent){
			G_RunObject( ent );
		}
		return 0;
	}
#endif
	static const struct luaL_Reg entityMeta[] = {
		{ "__index", Entity_Index },
		{ "__newindex", Entity_NewIndex },
		{ "__eq", Entity_Equals },
		{ "__tostring", Entity_ToString },
#if defined(PROJECT_GAME)
		{ "SetThinkFunction", Entity_SetThinkFunction },
		{ "SetReachedFunction", Entity_SetReachedFunction },
		{ "SetBlockedFunction", Entity_SetBlockedFunction },
		{ "SetTouchFunction", Entity_SetTouchFunction },
		{ "SetUseFunction", Entity_SetUseFunction },
		{ "SetPainFunction", Entity_SetPainFunction },
		{ "SetDieFunction", Entity_SetDieFunction },
		{ "Free", Entity_Free },
		{ "Use", Entity_Use },
		{ "PlaySound", Entity_PlaySound },
#endif
		{ "GetBoneVector", Entity_GetBoneVector },
#if defined(PROJECT_GAME)
		{ "Scale", Entity_Scale },
		{ "SetVar", Entity_SetVar },
		{ "GetVar", Entity_GetVar },
		{ "RunObject", Entity_RunObject},
#endif
		{ NULL, NULL }
	};

	void Register_Entity( lua_State *L ) {
		const luaL_Reg *r;

		luaL_newmetatable( L, ENTITY_META );
		lua_pushstring( L, "__index" );
		lua_pushvalue( L, -2 );
		lua_settable( L, -3 );

		for ( r = entityMeta; r->name; r++ ) {
			lua_pushcfunction( L, r->func );
			lua_setfield( L, -2, r->name );
		}

		lua_pop( L, -1 );
	}

#endif //JPLUA

#ifdef PROJECT_GAME
	void Entity_CallFunction( gentity_t *ent, entityFunc_t funcID, intptr_t arg1, intptr_t arg2,
		intptr_t arg3, intptr_t arg4 )
	{
#ifdef JPLUA
		if (ent->uselua) {
			lua_State *L = ls.L;
			switch (funcID) {

			case JPLUA_ENTITY_THINK: {
				if (ent->lua_think) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_think);
					Entity_CreateRef(L, ent);
					Call(L, 1, 0);
				}
			} break;

			case JPLUA_ENTITY_REACHED: {
				if (ent->lua_reached) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_reached);
					Entity_CreateRef(L, ent);
					Call(L, 1, 0);
				}
			} break;

			case JPLUA_ENTITY_BLOCKED: {
				if (ent->lua_blocked) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_blocked);
					Entity_CreateRef(L, ent);
					Entity_CreateRef(L, (gentity_t *)arg1);
					Call(L, 2, 0);
					break;
				}
			} break;

			case JPLUA_ENTITY_TOUCH: {
				if (ent->lua_touch) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_touch);
					Entity_CreateRef(L, ent);
					Entity_CreateRef(L, (gentity_t *)arg1);
					trace_t *tr = (trace_t *)arg2;

					lua_newtable(L);
					int top = lua_gettop(L);

					lua_pushstring(L, "allsolid");
					lua_pushboolean(L, !!tr->allsolid);
					lua_settable(L, top);

					lua_pushstring(L, "startsolid");
					lua_pushboolean(L, !!tr->startsolid);
					lua_settable(L, top);

					lua_pushstring(L, "entityNum");
					lua_pushinteger(L, tr->entityNum);
					lua_settable(L, top);

					lua_pushstring(L, "fraction");
					lua_pushnumber(L, tr->fraction);
					lua_settable(L, top);

					lua_pushstring(L, "endpos");
					lua_newtable(L);
					int top2 = lua_gettop(L);

					lua_pushstring(L, "x");
					lua_pushnumber(L, tr->endpos.x);
					lua_settable(L, top2);

					lua_pushstring(L, "y");
					lua_pushnumber(L, tr->endpos.y);
					lua_settable(L, top2);

					lua_pushstring(L, "z");
					lua_pushnumber(L, tr->endpos.z);
					lua_settable(L, top2);
					lua_settable(L, top);

					lua_pushstring(L, "plane");
					lua_newtable(L);
					top2 = lua_gettop(L);

					lua_pushstring(L, "normal");
					lua_newtable(L);
					int top3 = lua_gettop(L);

					lua_pushstring(L, "x");
					lua_pushnumber(L, tr->plane.normal.x);
					lua_settable(L, top3);

					lua_pushstring(L, "y");
					lua_pushnumber(L, tr->plane.normal.y);
					lua_settable(L, top3);

					lua_pushstring(L, "z");
					lua_pushnumber(L, tr->plane.normal.z);
					lua_settable(L, top3);
					lua_settable(L, top2);

					lua_pushstring(L, "dist");
					lua_pushnumber(L, tr->plane.dist);
					lua_settable(L, top2);

					lua_pushstring(L, "type");
					lua_pushinteger(L, tr->plane.type);
					lua_settable(L, top2);

					lua_pushstring(L, "signbits");
					lua_pushinteger(L, tr->plane.signbits);
					lua_settable(L, top2);
					lua_settable(L, top);

					lua_pushstring(L, "surfaceFlags");
					lua_pushinteger(L, tr->surfaceFlags);
					lua_settable(L, top);

					lua_pushstring(L, "contents");
					lua_pushinteger(L, tr->contents);
					lua_settable(L, top);

					Call(L, 3, 0);
					break;
				}
			} break;

			case JPLUA_ENTITY_USE: {
				if (ent->lua_use) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_use);
					Entity_CreateRef(L, ent);
					Entity_CreateRef(L, (gentity_t *)arg1);
					Entity_CreateRef(L, (gentity_t *)arg2);
					Call(L, 3, 0);
					break;
				}
			} break;

			case JPLUA_ENTITY_PAIN: {
				if (ent->lua_pain) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_pain);
					Entity_CreateRef(L, ent);
					Entity_CreateRef(L, (gentity_t *)arg1);
					lua_pushinteger(L, (int)arg2);
					Call(L, 3, 0);
					break;
				}
			} break;

			case JPLUA_ENTITY_DIE: {
				if (ent->lua_die) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, ent->lua_die);
					Entity_CreateRef(L, ent);
					Entity_CreateRef(L, (gentity_t *)arg1);
					Entity_CreateRef(L, (gentity_t *)arg2);
					lua_pushinteger(L, (int)arg3);
					lua_pushinteger(L, (int)arg4);
					Call(L, 5, 0);
					break;
				}
			} break;

			default: {
				// ...
			} break;
			}
		}
#endif // JPLUA
	}

#endif // PROJECT_GAME

} // namespace JPLua
