#include "cg_local.h"
#include "bg_luainternal.h"

#ifdef JPLUA

namespace JPLua {

static const char SERVER_META[] = "Server.meta";

// Func: GetServer()
// Retn: Server object if we have a valid snapshot
//		nil if we do not have a valid snapshot
int GetServer(lua_State *L) {
    if (cg.snap) {
        luaServer_t *server = (luaServer_t *)lua_newuserdata(L, sizeof(luaServer_t));
        server->dummy = 0u;
        luaL_getmetatable(L, SERVER_META);
        lua_setmetatable(L, -2);
    } else
        lua_pushnil(L);

    return 1;
}

// Ensure the value at the specified index is a valid Server instance,
// Return the instance if it is, otherwise return NULL.
luaServer_t *CheckServer(lua_State *L, int idx) {
    void *ud = luaL_checkudata(L, idx, SERVER_META);
    luaL_argcheck(L, ud != NULL, 1, "'Server' expected");
    return (luaServer_t *)ud;
}

static int Server_GetCInfo(lua_State *L) {
    lua_pushunsigned(L, cgs.japp.jp_cinfo);
    return 1;
}

static const serverProperty_t serverProperties[] = {
    {"cinfo", Server_GetCInfo, nullptr},
};
static const size_t numServerProperties = ARRAY_LEN(serverProperties);

static int ServerPropertyCompare(const void *a, const void *b) { return strcmp((const char *)a, ((serverProperty_t *)b)->name); }

static int Server_Index(lua_State *L) {
    Q_UNUSED
    luaServer_t *server = CheckServer(L, 1);
    const char *key = lua_tostring(L, 2);
    int returnValues = 0;

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);
    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const serverProperty_t *property = (serverProperty_t *)bsearch(key, serverProperties, numServerProperties, sizeof(serverProperty_t), ServerPropertyCompare);
    if (property) {
        if (property->Get) {
            returnValues += property->Get(L);
        }
    } else {
        lua_pushnil(L);
        returnValues++;
    }

    return returnValues;
}

static int Server_NewIndex(lua_State *L) {
    Q_UNUSED
    luaServer_t *server = CheckServer(L, 1);
    const char *key = lua_tostring(L, 2);

    lua_getmetatable(L, 1);
    lua_getfield(L, -1, key);

    if (!lua_isnil(L, -1)) {
        return 1;
    }

    // assume it's a field
    const serverProperty_t *property = (serverProperty_t *)bsearch(key, serverProperties, numServerProperties, sizeof(serverProperty_t), ServerPropertyCompare);
    if (property) {
        if (property->Set) {
            property->Set(L);
        }
    } else {
    }

    return 0;
}

// Func: tostring(Server)
// Retn: string representing the Server instance (for debug/error messages)
static int Server_ToString(lua_State *L) {
    lua_pushfstring(L, "Server(%s)", cgs.japp.serverName);
    return 1;
}

// Func: Server:GetName()
// Retn: string of the server's sv_hostname
static int Server_GetName(lua_State *L) {
    lua_pushstring(L, cgs.japp.serverName);
    return 1;
}

// Func: Server:GetSSF()
// Retn: 32bit integer bit-field of the server's support flags
static int Server_GetSSF(lua_State *L) {
    lua_pushinteger(L, cg.japp.SSF);
    return 1;
}

// Func: Server:GetPlayers()
// Retn: Indexed table of Player objects ordered by clientNum
static int Server_GetPlayers(lua_State *L) {
    int top, i = 1, clientNum;

    lua_newtable(L);
    top = lua_gettop(L);

    for (clientNum = 0; clientNum < MAX_CLIENTS; clientNum++) {
        if (cgs.clientinfo[clientNum].infoValid) {
            lua_pushnumber(L, i++);
            Player_CreateRef(L, clientNum);
            lua_settable(L, top);
        }
    }

    return 1;
}

static const struct luaL_Reg serverMeta[] = {{"__index", Server_Index},
                                             {"__newindex", Server_NewIndex},
                                             {"__tostring", Server_ToString},
                                             {"GetName", Server_GetName},
                                             {"GetSSF", Server_GetSSF},
                                             {"GetPlayers", Server_GetPlayers},
                                             {NULL, NULL}};

// Register the Server class for Lua
void Register_Server(lua_State *L) {
    luaL_newmetatable(L, SERVER_META); // Create metatable for Server class, push on stack

    // Lua won't attempt to directly index userdata, only via metatables
    // Make this metatable's __index loopback to itself so can index the object directly
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // Re-push metatable to top of stack
    lua_settable(L, -3);  // metatable.__index = metatable

    // fill metatable with fields
    for (const luaL_Reg *r = serverMeta; r->name; r++) {
        lua_pushcfunction(L, r->func);
        lua_setfield(L, -2, r->name);
    }

    lua_pop(L, -1); // Pop the Server class metatable from the stack
}

} // namespace JPLua

#endif // JPLUA
