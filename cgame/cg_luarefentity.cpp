#include "cg_local.h"
#include "bg_luainternal.h"
#if defined(PROJECT_CGAME) || defined(JPLUA)
namespace JPLua {
static const char REFENTITY_META[] = "RefEntity.meta";

void RefEntity_CreateRef(lua_State *L, refEntity_t *ent) {
    jplua_refentity_t *data = NULL;
    if (ent) { /// : dddd
        data = (jplua_refentity_t *)lua_newuserdata(L, sizeof(jplua_refentity_t));
        data->p = ent;
    }
    luaL_getmetatable(L, REFENTITY_META);
    lua_setmetatable(L, -2);
}

refEntity_t *CheckRefEntity(lua_State *L, int idx) {
    refEntity_t *ent = NULL;
    jplua_refentity_t *data;
    void *ud = luaL_checkudata(L, idx, REFENTITY_META);
    luaL_argcheck(L, ud != NULL, 1, "'RefEntity' expected");
    data = (jplua_refentity_t *)ud;
    ent = data->p;
    return ent;
}

static int RefEntity_Equals(lua_State *L) {
    const refEntity_t *e1 = CheckRefEntity(L, 1), *e2 = CheckRefEntity(L, 2);
    lua_pushboolean(L, (e1 == e2) ? 1 : 0);
    return 1;
}

static int RefEntity_ToString(lua_State *L) {
    lua_pushstring(L, "RefEntity");
    return 1;
}

int RefEntity_Create(lua_State *L) {
    refEntity_t *ent = (refEntity_t *)malloc(sizeof(refEntity_t));
    memset(ent, 0, sizeof(refEntity_t));
    RefEntity_CreateRef(L, ent);
    return 1;
}

static int RefEntity_AddToScene(lua_State *L) {
    refEntity_t *ent = CheckRefEntity(L, 1);
    trap->R_AddRefEntityToScene(ent);
    return 0;
}

static int RefEntity_Free(lua_State *L) {
    refEntity_t *ent = CheckRefEntity(L, 1);
    free(ent);
    return 0;
}

static void RefEntity_SetReType(lua_State *L, refEntity_t *ent) { ent->reType = (refEntityType_t)luaL_checkinteger(L, 3); }

static int RefEntity_GetReType(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->reType);
    return 1;
}

static void RefEntity_SetRenderFx(lua_State *L, refEntity_t *ent) { ent->renderfx = luaL_checkinteger(L, 3); }

static int RefEntity_GetRenderFx(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->renderfx);
    return 1;
}

static void RefEntity_SetHModel(lua_State *L, refEntity_t *ent) { ent->hModel = (qhandle_t)luaL_checkinteger(L, 3); }

static int RefEntity_GetHModel(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->hModel);
    return 1;
}

static void RefEntity_SetOrigin(lua_State *L, refEntity_t *ent) {
    vector3 *org = CheckVector(L, 3);
    VectorCopy(org, &ent->origin);
}

static int RefEntity_GetOrigin(lua_State *L, refEntity_t *ent) {
    Vector_CreateRef(L, &ent->origin);
    return 1;
}

static void RefEntity_SetOldOrigin(lua_State *L, refEntity_t *ent) {
    vector3 *org = CheckVector(L, 3);
    VectorCopy(org, &ent->oldorigin);
}

static int RefEntity_GetOldOrigin(lua_State *L, refEntity_t *ent) {
    Vector_CreateRef(L, &ent->oldorigin);
    return 1;
}

static void RefEntity_SetCustomShader(lua_State *L, refEntity_t *ent) { ent->customShader = (qhandle_t)luaL_checkinteger(L, 3); }

static int RefEntity_GetCustomShader(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->customShader);
    return 1;
}

static void RefEntity_SetShaderRGBA(lua_State *L, refEntity_t *ent) {
    if (lua_type(L, 3) != LUA_TTABLE) {
        trap->Print("JPLua_RefEntity_SetShaderRGBA failed, not a table\n");
        StackDump(L);
        return;
    }

    lua_pushnil(L);
    for (int i = 0; i < 4 && lua_next(L, 3); i++) {
        ent->shaderRGBA[i] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

} // //

static int RefEntity_GetShaderRGBA(lua_State *L, refEntity_t *ent) {
    lua_newtable(L);
    int top = lua_gettop(L);
    lua_pushinteger(L, ent->shaderRGBA[0]);
    lua_settable(L, top);
    lua_pushinteger(L, ent->shaderRGBA[1]);
    lua_settable(L, top);
    lua_pushinteger(L, ent->shaderRGBA[2]);
    lua_settable(L, top);
    lua_pushinteger(L, ent->shaderRGBA[3]);
    lua_settable(L, top);
    return 1;
} //

static void RefEntity_SetShaderTexCoord(lua_State *L, refEntity_t *ent) {}

static int RefEntity_GetShaderTexCoord(lua_State *L, refEntity_t *ent) {
    lua_pushnil(L);
    return 1; // FIXME
}

static void RefEntity_SetRadius(lua_State *L, refEntity_t *ent) { ent->radius = luaL_checknumber(L, 3); }

static int RefEntity_GetRadius(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->radius);
    return 1;
}

static void RefEntity_SetFrame(lua_State *L, refEntity_t *ent) { // also used as MODEL_BEAM's diameter
    ent->frame = luaL_checkinteger(L, 3);
}

static int RefEntity_GetFrame(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->frame);
    return 1;
}

static void RefEntity_SetLightOrigin(lua_State *L, refEntity_t *ent) {
    vector3 *org = CheckVector(L, 3);
    VectorCopy(org, &ent->lightingOrigin);
}

static int RefEntity_GetLightOrigin(lua_State *L, refEntity_t *ent) {
    Vector_CreateRef(L, &ent->lightingOrigin);
    return 1;
}

static void RefEntity_SetShadowPlane(lua_State *L, refEntity_t *ent) { ent->shadowPlane = luaL_checknumber(L, 3); }

static int RefEntity_GetShadowPlane(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->shadowPlane);
    return 1;
}

static void RefEntity_SetSkinNum(lua_State *L, refEntity_t *ent) { ent->skinNum = luaL_checkinteger(L, 3); }

static int RefEntity_GetSkinNum(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->skinNum);
    return 1;
}

static void RefEntity_SetCustomSkin(lua_State *L, refEntity_t *ent) { ent->customSkin = (qhandle_t)luaL_checkinteger(L, 3); }

static int RefEntity_GetCustomSkin(lua_State *L, refEntity_t *ent) {
    lua_pushinteger(L, ent->customSkin);
    return 1;
}

static void RefEntity_Line_SetWidth(lua_State *L, refEntity_t *ent) { ent->data.line.width = luaL_checknumber(L, 3); }

static int RefEntity_Line_GetWidth(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->data.line.width);
    return 1;
}

static void RefEntity_Line_SetWidth2(lua_State *L, refEntity_t *ent) { ent->data.line.width2 = luaL_checknumber(L, 3); }

static int RefEntity_Line_GetWidth2(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->data.line.width2);
    return 1;
}

static void RefEntity_Line_SetStScale(lua_State *L, refEntity_t *ent) { ent->data.line.stscale = luaL_checknumber(L, 3); }

static int RefEntity_Line_GetStScale(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->data.line.stscale);
    return 1;
}

static void RefEntity_SetEndTime(lua_State *L, refEntity_t *ent) { ent->endTime = luaL_checknumber(L, 3); }

static int RefEntity_GetEndTime(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->endTime);
    return 1;
}

static void RefEntity_SetSaberLength(lua_State *L, refEntity_t *ent) { ent->saberLength = luaL_checknumber(L, 3); }

static int RefEntity_GetSaberLength(lua_State *L, refEntity_t *ent) {
    lua_pushnumber(L, ent->saberLength);
    return 1;
}

static void RefEntity_SetAngles(lua_State *L, refEntity_t *ent) {
    vector3 *vec = CheckVector(L, 3);
    VectorCopy(vec, &ent->angles);
}

static int RefEntity_GetAngles(lua_State *L, refEntity_t *ent) {
    Vector_CreateRef(L, &ent->angles);
    return 1;
}

static void RefEntity_SetModelScale(lua_State *L, refEntity_t *ent) {
    vector3 *vec = CheckVector(L, 3);
    VectorCopy(vec, &ent->modelScale);
}

static int RefEntity_GetModelScale(lua_State *L, refEntity_t *ent) {
    Vector_CreateRef(L, &ent->modelScale);
    return 1;
}

typedef int (*getFunc_re)(lua_State *L, refEntity_t *ent);
typedef void (*setFunc_re)(lua_State *L, refEntity_t *ent);
typedef struct re_prop_s {
    const char *name;
    getFunc_re Get;
    setFunc_re Set;
} re_prop_t;

static const re_prop_t refEntityProperties[] = {
    {"angles", RefEntity_GetAngles, RefEntity_SetAngles},
    {"customShader", RefEntity_GetCustomShader, RefEntity_SetCustomShader},
    {"customSkin", RefEntity_GetCustomSkin, RefEntity_SetCustomSkin},
    {"endTime", RefEntity_GetEndTime, RefEntity_SetEndTime},
    {"frame", RefEntity_GetFrame, RefEntity_SetFrame},
    {"hModel", RefEntity_GetHModel, RefEntity_SetHModel},
    {"lightOrigin", RefEntity_GetLightOrigin, RefEntity_SetLightOrigin},
    {"line_width", RefEntity_Line_GetWidth, RefEntity_Line_SetWidth},
    {"line_width2", RefEntity_Line_GetWidth2, RefEntity_Line_SetWidth2},
    {"line_stscale", RefEntity_Line_GetStScale, RefEntity_Line_SetStScale},
    {"modelScale", RefEntity_GetModelScale, RefEntity_SetModelScale},
    {"origin", RefEntity_GetOrigin, RefEntity_SetOrigin},
    {"oldOrigin", RefEntity_GetOldOrigin, RefEntity_SetOldOrigin},
    {"radius", RefEntity_GetRadius, RefEntity_SetRadius},
    {"renderFx", RefEntity_GetRenderFx, RefEntity_SetRenderFx},
    {"reType", RefEntity_GetReType, RefEntity_SetReType},
    {"saberLength", RefEntity_GetSaberLength, RefEntity_SetSaberLength},
    {"shaderRGBA", RefEntity_GetShaderRGBA, RefEntity_SetShaderRGBA},
    {"shaderTexCoord", RefEntity_GetShaderTexCoord, RefEntity_SetShaderTexCoord},
    {"shadowPlane", RefEntity_GetShadowPlane, RefEntity_SetShadowPlane},
    {"skinNum", RefEntity_GetSkinNum, RefEntity_SetSkinNum},
};

static const size_t numRefEntityProperties = ARRAY_LEN(refEntityProperties);

static int RefEntity_Index(lua_State *L) {
    refEntity_t *ent = CheckRefEntity(L, 1);
    const char *key = luaL_checkstring(L, 2);
    int returnValues = 0;

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const re_prop_t *property = (re_prop_t *)bsearch(key, refEntityProperties, numRefEntityProperties, sizeof(re_prop_t), EntityPropertyCompare);
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

static int RefEntity_NewIndex(lua_State *L) {
    refEntity_t *ent = CheckRefEntity(L, 1);
    const char *key = luaL_checkstring(L, 2);

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);

    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const re_prop_t *property = (re_prop_t *)bsearch(key, refEntityProperties, numRefEntityProperties, sizeof(re_prop_t), EntityPropertyCompare);
    if (property) {
        if (property->Set) {
            property->Set(L, ent);
        }
    } else {
        // ...
    }

    return 0;
}

static const struct luaL_Reg refentity_meta[] = {
    {"__index", RefEntity_Index}, {"__newindex", RefEntity_NewIndex},   {"__eq", RefEntity_Equals}, {"__tostring", RefEntity_ToString},
    {"__gc", RefEntity_Free},     {"AddToScene", RefEntity_AddToScene}, {"Free", RefEntity_Free},   {NULL, NULL}};

void Register_RefEntity(lua_State *L) {
    const luaL_Reg *r;

    luaL_newmetatable(L, REFENTITY_META);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    for (r = refentity_meta; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    lua_pop(L, -1);
}
} // namespace JPLua
#endif
