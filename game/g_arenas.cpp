// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// g_arenas.c
//

#include "g_local.h"

void UpdateTournamentInfo(void) {
    int i;
    gentity_t *player;
    int playerClientNum;
    int n, accuracy, perfect, msglen;
    int buflen;
    int score1, score2;
    qboolean won;
    char buf[32];
    char msg[MAX_STRING_CHARS];

    // find the real player
    player = NULL;
    for (i = 0; i < level.maxclients; i++) {
        player = &g_entities[i];
        if (!player->inuse) {
            continue;
        }
        if (!(player->r.svFlags & SVF_BOT)) {
            break;
        }
    }
    // this should never happen!
    if (!player || i == level.maxclients) {
        return;
    }
    playerClientNum = i;

    CalculateRanks();

    if (level.clients[playerClientNum].sess.sessionTeam == TEAM_SPECTATOR) {
        Com_sprintf(msg, sizeof(msg), "postgame %i %i 0 0 0 0 0 0 0 0 0 0 0", level.numNonSpectatorClients, playerClientNum);
    } else {
        if (player->client->accuracy_shots) {
            accuracy = player->client->accuracy_hits * 100 / player->client->accuracy_shots;
        } else {
            accuracy = 0;
        }
        won = qfalse;
        if (level.gametype >= GT_CTF) {
            score1 = level.teamScores[TEAM_RED];
            score2 = level.teamScores[TEAM_BLUE];
            if (level.clients[playerClientNum].sess.sessionTeam == TEAM_RED) {
                won = (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]);
            } else {
                won = (level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED]);
            }
        } else {
            if (&level.clients[playerClientNum] == &level.clients[level.sortedClients[0]]) {
                won = qtrue;
                score1 = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
                score2 = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];
            } else {
                score2 = level.clients[level.sortedClients[0]].ps.persistant[PERS_SCORE];
                score1 = level.clients[level.sortedClients[1]].ps.persistant[PERS_SCORE];
            }
        }
        if (won && player->client->ps.persistant[PERS_KILLED] == 0) {
            perfect = 1;
        } else {
            perfect = 0;
        }
        Com_sprintf(msg, sizeof(msg), "postgame %i %i %i %i %i %i %i %i %i %i %i %i %i %i", level.numNonSpectatorClients, playerClientNum, accuracy,
                    player->client->ps.persistant[PERS_IMPRESSIVE_COUNT], player->client->ps.persistant[PERS_EXCELLENT_COUNT],
                    player->client->ps.persistant[PERS_DEFEND_COUNT], player->client->ps.persistant[PERS_ASSIST_COUNT],
                    player->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT], player->client->ps.persistant[PERS_SCORE], perfect, score1, score2, level.time,
                    player->client->ps.persistant[PERS_CAPTURES]);
    }

    msglen = strlen(msg);
    for (i = 0; i < level.numNonSpectatorClients; i++) {
        n = level.sortedClients[i];
        Com_sprintf(buf, sizeof(buf), " %i %i %i", n, level.clients[n].ps.persistant[PERS_RANK], level.clients[n].ps.persistant[PERS_SCORE]);
        buflen = strlen(buf);
        if (msglen + buflen + 1 >= sizeof(msg)) {
            break;
        }
        Q_strcat(msg, sizeof(msg), buf);
    }
    trap->SendConsoleCommand(EXEC_APPEND, msg);
}
