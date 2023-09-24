#include "g_local.h"
#include "cJSON/cJSON.h"
#include "qcommon/md5.h"

// session data is the only data that stays persistant across level loads and tournament restarts.

// called on game shutdown
void G_WriteClientSessionData(const gclient_t *client) {
    const clientSession_t *sess = &client->sess;
    cJSON *root;
    fileHandle_t f;
    char fileName[MAX_QPATH] = {};

    Com_sprintf(fileName, sizeof(fileName), "session/client%02i.json", (int)(client - level.clients));
    Com_Printf("Writing session file %s\n", fileName);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "sessionTeam", sess->sessionTeam);
    cJSON_AddNumberToObject(root, "spectatorTime", sess->spectatorTime);
    cJSON_AddNumberToObject(root, "spectatorState", sess->spectatorState);
    cJSON_AddNumberToObject(root, "spectatorClient", sess->spectatorClient);
    cJSON_AddNumberToObject(root, "wins", sess->wins);
    cJSON_AddNumberToObject(root, "losses", sess->losses);
    cJSON_AddNumberToObject(root, "setForce", sess->setForce);
    cJSON_AddNumberToObject(root, "saberLevel", sess->saberLevel);
    cJSON_AddNumberToObject(root, "selectedFP", sess->selectedFP);
    cJSON_AddNumberToObject(root, "duelTeam", sess->duelTeam);
    cJSON_AddNumberToObject(root, "siegeDesiredTeam", sess->siegeDesiredTeam);
    cJSON_AddStringToObject(root, "siegeClass", *sess->siegeClass ? sess->siegeClass : "none");
    cJSON_AddStringToObject(root, "IP", sess->IP);
    if (client->pers.adminUser) {
        char checksum[33] = {};
        char combined[MAX_STRING_CHARS] = {};
        Com_sprintf(combined, sizeof(combined), "%s%s", client->pers.adminUser->user, client->pers.adminUser->password);
        Crypto::ChecksumMD5(combined, strlen(combined), checksum);
        cJSON_AddStringToObject(root, "admin", checksum);
    }
    cJSON_AddNumberToObject(root, "empowered", !!client->pers.adminData.empowered);
    cJSON_AddNumberToObject(root, "merc", !!client->pers.adminData.merc);
    cJSON_AddNumberToObject(root, "silenced", !!client->pers.adminData.silenced);
    cJSON_AddNumberToObject(root, "slept", !!client->pers.adminData.isSlept);
    cJSON_AddNumberToObject(root, "tempprivs", !!client->pers.tempprivs);

    trap->FS_Open(fileName, &f, FS_WRITE);

    Q_FSWriteJSON(root, f);
}

// called on a reconnect
void G_ReadClientSessionData(gclient_t *client) {
    clientSession_t *sess = &client->sess;
    cJSON *root = NULL, *object = NULL;
    char fileName[MAX_QPATH] = {};
    char *buffer = NULL;
    fileHandle_t f = NULL_FILE;
    unsigned int len = 0;
    const char *tmp = NULL;

    Com_sprintf(fileName, sizeof(fileName), "session/client%02i.json", (int)(client - level.clients));
    len = trap->FS_Open(fileName, &f, FS_READ);

    // no file
    if (!f || !len || len == -1) {
        trap->FS_Close(f);
        return;
    }

    buffer = (char *)malloc(len + 1);
    if (!buffer) {
        trap->FS_Close(f);
        return;
    }

    trap->FS_Read(buffer, len, f);
    trap->FS_Close(f);
    buffer[len] = '\0';

    // read buffer
    root = cJSON_Parse(buffer);
    free(buffer);

    if (!root) {
        Com_Printf("G_ReadSessionData(%02i): could not parse session data\n", (int)(client - level.clients));
        return;
    }

    if ((object = cJSON_GetObjectItem(root, "sessionTeam"))) {
        sess->sessionTeam = (team_t)object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "spectatorTime"))) {
        sess->spectatorTime = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "spectatorState"))) {
        sess->spectatorState = (spectatorState_t)object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "spectatorClient"))) {
        sess->spectatorClient = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "wins"))) {
        sess->wins = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "losses"))) {
        sess->losses = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "setForce"))) {
        sess->setForce = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "saberLevel"))) {
        sess->saberLevel = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "selectedFP"))) {
        sess->selectedFP = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "duelTeam"))) {
        sess->duelTeam = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "siegeDesiredTeam"))) {
        sess->siegeDesiredTeam = object->valueint;
    }

    if ((object = cJSON_GetObjectItem(root, "siegeClass"))) {
        if ((tmp = object->valuestring)) {
            Q_strncpyz(sess->siegeClass, tmp, sizeof(sess->siegeClass));
        }
    }
    if ((object = cJSON_GetObjectItem(root, "IP"))) {
        if ((tmp = object->valuestring)) {
            Q_strncpyz(sess->IP, tmp, sizeof(sess->IP));
        }
    }
    if ((object = cJSON_GetObjectItem(root, "admin"))) {
        if ((tmp = object->valuestring)) {
            client->pers.adminUser = AM_ChecksumLogin(tmp);
        }
    }

    if ((object = cJSON_GetObjectItem(root, "empowered"))) {
        client->pers.adminData.empowered = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "merc"))) {
        client->pers.adminData.merc = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "silenced"))) {
        client->pers.adminData.silenced = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "slept"))) {
        client->pers.adminData.isSlept = object->valueint;
    }
    if ((object = cJSON_GetObjectItem(root, "tempprivs"))) {
        client->pers.tempprivs = object->valueint;
    }

    client->ps.fd.saberAnimLevel = sess->saberLevel;
    client->ps.fd.saberDrawAnimLevel = sess->saberLevel;
    client->ps.fd.forcePowerSelected = sess->selectedFP;

    cJSON_Delete(root);
    root = NULL;
}

// called on a first-time connect
void G_InitClientSessionData(gclient_t *client, char *userinfo, qboolean isBot) {
    clientSession_t *sess = &client->sess;
    const char *value;

    client->sess.siegeDesiredTeam = TEAM_FREE;

    // initial team determination
    if (level.gametype >= GT_TEAM) {
        if (g_teamAutoJoin.integer && !(g_entities[client - level.clients].r.svFlags & SVF_BOT)) {
            sess->sessionTeam = PickTeam(-1);
            BroadcastTeamChange(client, -1);
        } else {
            // always spawn as spectator in team games
            if (!isBot) {
                sess->sessionTeam = TEAM_SPECTATOR;
            } else {
                // bots choose their team on creation
                value = Info_ValueForKey(userinfo, "team");
                if (value[0] == 'r' || value[0] == 'R') {
                    sess->sessionTeam = TEAM_RED;
                } else if (value[0] == 'b' || value[0] == 'B') {
                    sess->sessionTeam = TEAM_BLUE;
                } else {
                    sess->sessionTeam = PickTeam(-1);
                }
                BroadcastTeamChange(client, -1);
            }
        }
    } else {
        value = Info_ValueForKey(userinfo, "team");
        if (value[0] == 's') {
            // a willing spectator, not a waiting-in-line
            sess->sessionTeam = TEAM_SPECTATOR;
        } else {
            switch (level.gametype) {
            default:
            case GT_FFA:
            case GT_HOLOCRON:
            case GT_JEDIMASTER:
            case GT_SINGLE_PLAYER:
                if (g_maxGameClients.integer > 0 && level.numNonSpectatorClients >= g_maxGameClients.integer) {
                    sess->sessionTeam = TEAM_SPECTATOR;
                } else if (g_teamAutoJoin.integer == 2) {
                    // force joining in all gametypes
                    sess->sessionTeam = TEAM_FREE;
                } else if (!isBot) {
                    sess->sessionTeam = TEAM_SPECTATOR;
                } else {
                    // bots automatically join the game
                    sess->sessionTeam = TEAM_FREE;
                }
                break;
            case GT_DUEL:
                // if the game is full, go into a waiting mode
                if (level.numNonSpectatorClients >= 2) {
                    sess->sessionTeam = TEAM_SPECTATOR;
                } else {
                    sess->sessionTeam = TEAM_FREE;
                }
                break;
            case GT_POWERDUEL: {
                int loners = 0, doubles = 0;

                G_PowerDuelCount(&loners, &doubles, qtrue);

                if (!doubles || loners > (doubles / 2)) {
                    sess->duelTeam = DUELTEAM_DOUBLE;
                } else {
                    sess->duelTeam = DUELTEAM_LONE;
                }
                sess->sessionTeam = TEAM_SPECTATOR;
            } break;
            }
        }
    }

    if (sess->sessionTeam == TEAM_SPECTATOR) {
        sess->spectatorState = SPECTATOR_FREE;
    } else {
        sess->spectatorState = SPECTATOR_NOT;
    }

    sess->spectatorTime = level.time;
    sess->siegeClass[0] = '\0';

    G_WriteClientSessionData(client);
}

static const char *metaFileName = "session/meta.json";

void G_ReadSessionData(void) {
    char *buffer = NULL;
    fileHandle_t f = NULL_FILE;
    unsigned int len = 0u;
    cJSON *root;

    trap->Print("G_ReadSessionData: reading %s...", metaFileName);
    len = trap->FS_Open(metaFileName, &f, FS_READ);

    // no file
    if (!f || !len || len == -1) {
        trap->Print("failed to open file, clearing session data...\n");
        level.newSession = qtrue;
        return;
    }

    buffer = (char *)malloc(len + 1);
    if (!buffer) {
        trap->Print("failed to allocate buffer, clearing session data...\n");
        level.newSession = qtrue;
        return;
    }

    trap->FS_Read(buffer, len, f);
    trap->FS_Close(f);
    buffer[len] = '\0';

    // read buffer
    root = cJSON_Parse(buffer);

    // if the gametype changed since the last session, don't use any client sessions
    if (level.gametype != cJSON_GetObjectItem(root, "gametype")->valueint) {
        level.newSession = qtrue;
        trap->Print("gametype changed, clearing session data...");
    }

    free(buffer);
    cJSON_Delete(root);
    root = NULL;
    trap->Print("done\n");
}

void G_WriteSessionData(void) {
    int i;
    fileHandle_t f;
    const gclient_t *client = NULL;
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "gametype", level.gametype);

    trap->Print("G_WriteSessionData: writing %s...", metaFileName);
    trap->FS_Open(metaFileName, &f, FS_WRITE);

    Q_FSWriteJSON(root, f);

    for (i = 0, client = level.clients; i < level.maxclients; i++, client++) {
        if (client->pers.connected == CON_CONNECTED) {
            G_WriteClientSessionData(client);
        }
    }

    trap->Print("done\n");
}
