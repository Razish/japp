#include "cg_local.h"
#include "bg_luainternal.h"

#ifdef JPLUA

namespace JPLua {

static const char LOCALENTITY_META[] = "LocalEntity.meta";

void LocalEntity_CreateRef(lua_State *L, localEntity_t *ent) {
    luaLocalEntity_t *data = NULL;
    if (ent) {
        data = (luaLocalEntity_t *)lua_newuserdata(L, sizeof(luaLocalEntity_t));
        data->id = ent->id;
    }
    luaL_getmetatable(L, LOCALENTITY_META);
    lua_setmetatable(L, -2);
}

localEntity_t *CheckLocalEntity(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, LOCALENTITY_META);
    luaL_argcheck(L, ud != NULL, 1, "'LocalEntity' expected");
    luaLocalEntity_t *data = (luaLocalEntity_t *)ud;

    return CG_FindLocalEntity(data->id);
}

static int LocalEntity_Equals(lua_State *L) {
    const localEntity_t *e1 = CheckLocalEntity(L, 1);
    const localEntity_t *e2 = CheckLocalEntity(L, 2);
    lua_pushboolean(L, (e1->id == e2->id) ? 1 : 0);
    return 1;
}

static int LocalEntity_ToString(lua_State *L) {
    const localEntity_t *entity = CheckLocalEntity(L, 1);
    lua_pushfstring(L, "LocalEntity(%d)", entity->id);
    return 1;
}

int LocalEntity_Create(lua_State *L) {
    localEntity_t *ent = CG_AllocLocalEntity();
    LocalEntity_CreateRef(L, ent);

    return 1;
}

static void LocalEntity_SetType(lua_State *L, localEntity_t *ent) { ent->leType = (leType_e)luaL_checkinteger(L, 3); }

static int LocalEntity_GetType(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->leType);
    return 1;
}

static void LocalEntity_SetFlags(lua_State *L, localEntity_t *ent) { ent->leFlags = luaL_checkinteger(L, 3); }

static int LocalEntity_GetFlags(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->leFlags);
    return 1;
}

static void LocalEntity_SetLifeTime(lua_State *L, localEntity_t *ent) { ent->lifeRate = luaL_checkinteger(L, 3); }

static int LocalEntity_GetLifeTime(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->lifeRate);
    return 1;
}

static void LocalEntity_SetFadeInTime(lua_State *L, localEntity_t *ent) { ent->fadeInTime = luaL_checkinteger(L, 3); }

static int LocalEntity_GetFadeInTime(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->fadeInTime);
    return 1;
}

#if 0
	static void LocalEntity_SetTrAngles( lua_State *L, localEntity_t *ent ) { //epiccheck
	}

	static void LocalEntity_SetTrPos( lua_State *L, localEntity_t *ent ) { //epiccheck
	}

	static int LocalEntity_GetTrPos( lua_State *L, localEntity_t *ent ) {
		return 0;
	}
#endif

static void LocalEntity_SetBFactor(lua_State *L, localEntity_t *ent) { ent->bounceFactor = luaL_checknumber(L, 3); }

static int LocalEntity_GetBFactor(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->bounceFactor);
    return 1;
}

static void LocalEntity_SetBSound(lua_State *L, localEntity_t *ent) { ent->bounceSound = luaL_checkinteger(L, 3); }

static int LocalEntity_GetBSound(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->bounceSound);
    return 1;
}

static void LocalEntity_SetAlpha(lua_State *L, localEntity_t *ent) { ent->alpha = luaL_checknumber(L, 3); }

static int LocalEntity_GetAlpha(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->alpha);
    return 1;
}

static void LocalEntity_SetDAlpha(lua_State *L, localEntity_t *ent) { ent->dalpha = luaL_checknumber(L, 3); }

static int LocalEntity_GetDAlpha(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->dalpha);
    return 1;
}

static void LocalEntity_SetForceAlpha(lua_State *L, localEntity_t *ent) { ent->forceAlpha = luaL_checkinteger(L, 3); }

static int LocalEntity_GetForceAlpha(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->forceAlpha);
    return 1;
}

static void LocalEntity_SetColor(lua_State *L, localEntity_t *ent) { ReadFloats(ent->color, 4, L, 3); }

static int LocalEntity_GetColor(lua_State *L, localEntity_t *ent) {
    lua_newtable(L);
    int top = lua_gettop(L);
    lua_pushnumber(L, ent->color[0]);
    lua_settable(L, top);
    lua_pushnumber(L, ent->color[1]);
    lua_settable(L, top);
    lua_pushnumber(L, ent->color[2]);
    lua_settable(L, top);
    lua_pushnumber(L, ent->color[3]);
    lua_settable(L, top);
    return 1;
}

static void LocalEntity_SetRadius(lua_State *L, localEntity_t *ent) { ent->radius = luaL_checknumber(L, 3); }

static int LocalEntity_GetRadius(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->radius);
    return 1;
}

static void LocalEntity_SetLight(lua_State *L, localEntity_t *ent) { ent->light = luaL_checknumber(L, 3); }

static int LocalEntity_GetLight(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->light);
    return 1;
}

static void LocalEntity_SetLightColor(lua_State *L, localEntity_t *ent) {
    vector3 *vec = CheckVector(L, 3);
    VectorCopy(vec, &ent->lightColor);
}

static int LocalEntity_GetLightColor(lua_State *L, localEntity_t *ent) {
    Vector_CreateRef(L, &ent->lightColor);
    return 1;
}

static void LocalEntity_SetBSndType(lua_State *L, localEntity_t *ent) { ent->leBounceSoundType = (leBounceSoundType_e)luaL_checkinteger(L, 3); }

static int LocalEntity_GetBSndType(lua_State *L, localEntity_t *ent) {
    lua_pushinteger(L, ent->leBounceSoundType);
    return 1;
}

static void LocalEntity_Line_SetWidth(lua_State *L, localEntity_t *ent) { ent->data.line.width = luaL_checknumber(L, 3); }

static void LocalEntity_Line_SetDWidth(lua_State *L, localEntity_t *ent) { ent->data.line.dwidth = luaL_checknumber(L, 3); }

static int LocalEntity_Line_GetWidth(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->data.line.width);
    return 1;
}

static int LocalEntity_Line_GetDWidth(lua_State *L, localEntity_t *ent) {
    lua_pushnumber(L, ent->data.line.dwidth);
    return 1;
}

static int LocalEntity_GetRefEntity(lua_State *L, localEntity_t *ent) {
    RefEntity_CreateRef(L, &ent->refEntity);
    return 1;
}

static int LocalEntity_Free(lua_State *L) {
    CG_FreeLocalEntity(CheckLocalEntity(L, 1));
    return 0;
}
typedef int (*getFunc_le)(lua_State *L, localEntity_t *ent);
typedef void (*setFunc_le)(lua_State *L, localEntity_t *ent);
struct le_prop_t {
    const char *name;
    getFunc_le Get;
    setFunc_le Set;
};

static const le_prop_t localEntityProperties[] = {
    {"alpha", LocalEntity_GetAlpha, LocalEntity_SetAlpha},
    {"bounceFactor", LocalEntity_GetBFactor, LocalEntity_SetBFactor},
    {"bounceSound", LocalEntity_GetBSound, LocalEntity_SetBSound},
    {"bounceSoundType", LocalEntity_GetBSndType, LocalEntity_SetBSndType},
    {"color", LocalEntity_GetColor, LocalEntity_SetColor},
    {"dalpha", LocalEntity_GetDAlpha, LocalEntity_SetDAlpha},
    {"fadeintime", LocalEntity_GetFadeInTime, LocalEntity_SetFadeInTime},
    {"flags", LocalEntity_GetFlags, LocalEntity_SetFlags},
    {"forcealpha", LocalEntity_GetForceAlpha, LocalEntity_SetForceAlpha},
    {
        "lifetime",
        LocalEntity_GetLifeTime,
        LocalEntity_SetLifeTime,
    },
    {
        "light",
        LocalEntity_GetLight,
        LocalEntity_SetLight,
    },
    {
        "lightcolor",
        LocalEntity_GetLightColor,
        LocalEntity_SetLightColor,
    },
    {
        "line_dwidth",
        LocalEntity_Line_GetDWidth,
        LocalEntity_Line_SetDWidth,
    },
    {
        "line_width",
        LocalEntity_Line_GetWidth,
        LocalEntity_Line_SetWidth,
    },
    {
        "radius",
        LocalEntity_GetRadius,
        LocalEntity_SetRadius,
    },
    {
        "refentity", LocalEntity_GetRefEntity,
        nullptr, // Epic: Set?
    },
    {"type", LocalEntity_GetType, LocalEntity_SetType},

};
static const size_t numLocalEntityProperties = ARRAY_LEN(localEntityProperties);

static int LocalEntity_Index(lua_State *L) {
    localEntity_t *ent = CheckLocalEntity(L, 1);
    const char *key = luaL_checkstring(L, 2);
    int returnValues = 0;

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const le_prop_t *property = (le_prop_t *)bsearch(key, localEntityProperties, numLocalEntityProperties, sizeof(le_prop_t), EntityPropertyCompare);
    if (property) {
        if (property->Get) {
            returnValues += property->Get(L, ent);
        }
    } else {
        lua_pushnil(L);
        returnValues++;
    }

    return returnValues;
}

static int LocalEntity_NewIndex(lua_State *L) {
    localEntity_t *ent = CheckLocalEntity(L, 1);
    const char *key = luaL_checkstring(L, 2);

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);

    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const le_prop_t *property = (le_prop_t *)bsearch(key, localEntityProperties, numLocalEntityProperties, sizeof(le_prop_t), EntityPropertyCompare);
    if (property) {
        if (property->Set) {
            property->Set(L, ent);
        }
    } else {
        // ...
    }

    return 0;
}

static const struct luaL_Reg localentity_meta[] = {{"__index", LocalEntity_Index},       {"__newindex", LocalEntity_NewIndex}, {"__eq", LocalEntity_Equals},
                                                   {"__tostring", LocalEntity_ToString}, {"Free", LocalEntity_Free},           {NULL, NULL}};

void Register_LocalEntity(lua_State *L) {
    luaL_newmetatable(L, LOCALENTITY_META);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (const luaL_Reg *r = localentity_meta; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    lua_pop(L, -1);
}

} // namespace JPLua

#endif // JPLUA
