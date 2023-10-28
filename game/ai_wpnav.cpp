#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib.h"
#include "ai_main.h"

float gWPRenderTime = 0;
float gDeactivated = 0;
float gBotEdit = 0;
int gWPRenderedFrame = 0;

std::vector<wpobject_t *> gWPArray;

int gLastPrintedIndex = -1;

nodeobject_t nodetable[MAX_NODETABLE_SIZE];
int nodenum; // so we can connect broken trails

uint32_t gLevelFlags = 0;

static char *GetFlagStr(uint32_t flags) {
    static char flagstr[128];
    char *p = flagstr;

    memset(flagstr, '\0', sizeof(flagstr));

    if (!flags) {
        Q_strncpyz(flagstr, "none", sizeof(flagstr));
        return flagstr;
    }

    if (flags & WPFLAG_JUMP) {
        *p++ = 'j';
    }

    if (flags & WPFLAG_DUCK) {
        *p++ = 'd';
    }

    if (flags & WPFLAG_SNIPEORCAMPSTAND) {
        *p++ = 'c';
    }

    if (flags & WPFLAG_WAITFORFUNC) {
        *p++ = 'f';
    }

    if (flags & WPFLAG_SNIPEORCAMP) {
        *p++ = 's';
    }

    if (flags & WPFLAG_ONEWAY_FWD) {
        *p++ = 'x';
    }

    if (flags & WPFLAG_ONEWAY_BACK) {
        *p++ = 'y';
    }

    if (flags & WPFLAG_GOALPOINT) {
        *p++ = 'g';
    }

    if (flags & WPFLAG_NOVIS) {
        *p++ = 'n';
    }

    if (flags & WPFLAG_NOMOVEFUNC) {
        *p++ = 'm';
    }

    if (flags & WPFLAG_RED_FLAG) {
        const char *tmp = "red flag";
        if (p > flagstr) {
            *p++ = ' ';
        }
        Q_strncpyz(p, tmp, sizeof(flagstr) - (p - flagstr));
        p += strlen(tmp);
    }

    if (flags & WPFLAG_BLUE_FLAG) {
        const char *tmp = "blue flag";
        if (p > flagstr) {
            *p++ = ' ';
        }
        Q_strncpyz(p, tmp, sizeof(flagstr) - (p - flagstr));
        p += strlen(tmp);
    }

    if (flags & WPFLAG_SIEGE_IMPERIALOBJ) {
        const char *tmp = "saga-imp";
        if (p > flagstr) {
            *p++ = ' ';
        }
        Q_strncpyz(p, tmp, sizeof(flagstr) - (p - flagstr));
        p += strlen(tmp);
    }

    if (flags & WPFLAG_SIEGE_REBELOBJ) {
        const char *tmp = "saga-reb";
        if (p > flagstr) {
            *p++ = ' ';
        }
        Q_strncpyz(p, tmp, sizeof(flagstr) - (p - flagstr));
        p += strlen(tmp);
    }

    *p = '\0';

    if (p == flagstr) {
        Q_strncpyz(flagstr, "unknown", sizeof(flagstr));
    }

    return flagstr;
}

void G_TestLine(vector3 *start, vector3 *end, int color, int time) {
    gentity_t *te;

    te = G_TempEntity(start, EV_TESTLINE);
    VectorCopy(start, &te->s.origin);
    VectorCopy(end, &te->s.origin2);
    te->s.time2 = time;
    {
        static int r = 0;
        te->s.weapon = r; // color;
        r++;
        r &= 8;
    }
    te->r.svFlags |= SVF_BROADCAST;
}

void BotWaypointRender(void) {
    int i, n;
    int inc_checker;
    int bestindex;
    int gotbestindex;
    float bestdist;
    float checkdist;
    gentity_t *plum;
    gentity_t *viewent;
    vector3 a;

    if (!gBotEdit) {
        return;
    }

    bestindex = 0;

    if (gWPRenderTime > level.time) {
        goto checkprint;
    }

    gWPRenderTime = level.time + 100;

    i = gWPRenderedFrame;
    inc_checker = gWPRenderedFrame;

    for (int size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            plum = G_TempEntity(&gWPArray[i]->origin, EV_SCOREPLUM);
            plum->r.svFlags |= SVF_BROADCAST;
            plum->s.time = i;

            for (n = 0; n < gWPArray[i]->neighbornum; n++) {
                if (gWPArray[i]->neighbors[n].forceJumpTo && gWPArray[gWPArray[i]->neighbors[n].num]) {
                    G_TestLine(&gWPArray[i]->origin, &gWPArray[gWPArray[i]->neighbors[n].num]->origin, 0x0000ff, 5000);
                }
            }

            gWPRenderedFrame++;
        } else {
            gWPRenderedFrame = 0;
            break;
        }

        if ((i - inc_checker) > 4) {
            break; // don't render too many at once
        }
    }

    if (i >= gWPArray.size()) {
        gWPRenderTime = level.time + 1500; // wait a bit after we finish doing the whole trail
        gWPRenderedFrame = 0;
    }

checkprint:

    if (!bot_wp_info.value) {
        return;
    }

    viewent = &g_entities[0]; // only show info to the first client

    if (!viewent || !viewent->client) { // client isn't in the game yet?
        return;
    }

    bestdist = 256; // max distance for showing point info
    gotbestindex = 0;

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            VectorSubtract(&viewent->client->ps.origin, &gWPArray[i]->origin, &a);

            checkdist = VectorLength(&a);

            if (checkdist < bestdist) {
                bestdist = checkdist;
                bestindex = i;
                gotbestindex = 1;
            }
        }
    }

    if (gotbestindex && bestindex != gLastPrintedIndex) {
        const char *flagstr = GetFlagStr(gWPArray[bestindex]->flags);
        gLastPrintedIndex = bestindex;
        trap->Print(S_COLOR_YELLOW "Waypoint %i\nFlags - %i (%s) (w%f)\nOrigin - %s\n", gWPArray[bestindex]->index, gWPArray[bestindex]->flags, flagstr,
                    (double)gWPArray[bestindex]->weight, vtos(&gWPArray[bestindex]->origin));

        plum = G_TempEntity(&gWPArray[bestindex]->origin, EV_SCOREPLUM);
        plum->r.svFlags |= SVF_BROADCAST;
        plum->s.time = bestindex; // render it once
    } else if (!gotbestindex) {
        gLastPrintedIndex = -1;
    }
}

static void CreateNewWP(vector3 *origin, int flags) {
    wpobject_t *wp = (wpobject_t *)B_AllocZ(sizeof(wpobject_t));
    if (!wp) {
        trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
    }

    wp->flags = flags;
    wp->weight = 0;                         // calculated elsewhere
    wp->associated_entity = ENTITYNUM_NONE; // set elsewhere
    wp->forceJumpTo = 0;
    wp->disttonext = 0; // calculated elsewhere
    wp->index = gWPArray.size();
    VectorCopy(origin, &wp->origin);
    gWPArray.push_back(wp);
}

static void CreateNewWP_FromObject(wpobject_t *srcWP) {
    wpobject_t *destWP = (wpobject_t *)B_AllocZ(sizeof(wpobject_t));
    if (!destWP) {
        trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
    }

    destWP->flags = srcWP->flags;
    destWP->weight = srcWP->weight;
    destWP->associated_entity = srcWP->associated_entity;
    destWP->disttonext = srcWP->disttonext;
    destWP->forceJumpTo = srcWP->forceJumpTo;
    destWP->index = gWPArray.size();
    VectorCopy(&srcWP->origin, &destWP->origin);
    destWP->neighbornum = srcWP->neighbornum;

    for (int i = srcWP->neighbornum; i >= 0; i--) {
        destWP->neighbors[i].num = srcWP->neighbors[i].num;
        destWP->neighbors[i].forceJumpTo = srcWP->neighbors[i].forceJumpTo;
    }

    if (destWP->flags & WPFLAG_RED_FLAG) {
        flagRed = destWP;
        oFlagRed = flagRed;
    } else if (destWP->flags & WPFLAG_BLUE_FLAG) {
        flagBlue = destWP;
        oFlagBlue = flagBlue;
    }

    gWPArray.push_back(destWP);
}

static void RemoveWP(void) {
    if (gWPArray.size() <= 0) {
        return;
    }

    wpobject_t *back = gWPArray.back();
    if (!back) {
        return;
    }

    B_Free(back);
    gWPArray.pop_back();
}

static void RemoveWP_InTrail(int afterindex) {
    int foundindex;
    int foundanindex;

    foundindex = 0;
    foundanindex = 0;

    if (afterindex < 0 || afterindex >= gWPArray.size()) {
        trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
        return;
    }

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i]->index == afterindex) {
            foundindex = i;
            foundanindex = 1;
            break;
        }
    }

    if (!foundanindex) {
        trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
        return;
    }

    auto it = gWPArray.begin() + foundindex;
    B_Free(*it);
    gWPArray.erase(it);
}

static int CreateNewWP_InTrail(vector3 *origin, int flags, int afterindex) {
    int foundindex;
    int foundanindex;

    foundindex = 0;
    foundanindex = 0;

    if (afterindex < 0 || afterindex >= gWPArray.size()) {
        trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
        return 0;
    }

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i]->index == afterindex) {
            foundindex = i;
            foundanindex = 1;
            break;
        }
    }

    if (!foundanindex) {
        trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
        return 0;
    }

    wpobject_t *newWP = (wpobject_t *)B_AllocZ(sizeof(wpobject_t));
    newWP->flags = flags;
    newWP->weight = 0;                         // calculated elsewhere
    newWP->associated_entity = ENTITYNUM_NONE; // set elsewhere
    newWP->disttonext = 0;                     // calculated elsewhere
    newWP->forceJumpTo = 0;
    newWP->index = foundindex;
    VectorCopy(origin, &newWP->origin);
    gWPArray.emplace(gWPArray.begin() + foundindex, newWP);

    return 1;
}

static void TeleportToWP(gentity_t *pl, int afterindex) {
    int foundindex;
    int foundanindex;

    if (!pl || !pl->client) {
        return;
    }

    foundindex = 0;
    foundanindex = 0;

    if (afterindex < 0 || afterindex >= gWPArray.size()) {
        trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
        return;
    }

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i]->index == afterindex) {
            foundindex = i;
            foundanindex = 1;
            break;
        }
    }

    if (!foundanindex) {
        trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
        return;
    }

    VectorCopy(&gWPArray[foundindex]->origin, &pl->client->ps.origin);

    return;
}

static void WPFlagsModify(int wpnum, int flags) {
    if (wpnum < 0 || wpnum >= gWPArray.size() || !gWPArray[wpnum]) {
        trap->Print(S_COLOR_YELLOW "WPFlagsModify: Waypoint %i does not exist\n", wpnum);
        return;
    }

    gWPArray[wpnum]->flags = flags;
}

static int NotWithinRange(int base, int extent) {
    if (extent > base && base + 5 >= extent) {
        return 0;
    }

    if (extent < base && base - 5 <= extent) {
        return 0;
    }

    return 1;
}

static int NodeHere(vector3 *spot) {
    for (const auto &node : nodetable) {
        if ((int)node.origin.x == (int)spot->x && (int)node.origin.y == (int)spot->y) {
            if ((int)node.origin.z == (int)spot->z || ((int)node.origin.z < (int)spot->z && (int)node.origin.z + 5 > (int)spot->z) ||
                ((int)node.origin.z > (int)spot->z && (int)node.origin.z - 5 < (int)spot->z)) {
                return 1;
            }
        }
    }

    return 0;
}

static int CanGetToVector(vector3 *org1, vector3 *org2, vector3 *mins, vector3 *maxs) {
    trace_t tr;

    trap->Trace(&tr, org1, mins, maxs, org2, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

    if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid) {
        return 1;
    }

    return 0;
}

int CanGetToVectorTravel(vector3 *org1, vector3 *moveTo, vector3 *mins, vector3 *maxs) {
    trace_t tr;
    vector3 stepTo;
    vector3 stepSub;
    vector3 stepGoal;
    vector3 workingOrg;
    vector3 lastIncrement;
    vector3 finalMeasure;
    float stepSize = 0;
    float measureLength = 0;
    int didMove = 0;
    int traceMask = MASK_PLAYERSOLID;
    qboolean initialDone = qfalse;

    VectorCopy(org1, &workingOrg);
    VectorCopy(org1, &lastIncrement);

    VectorCopy(moveTo, &stepTo);
    stepTo.z = workingOrg.z;

    VectorSubtract(&stepTo, &workingOrg, &stepSub);
    stepSize = VectorLength(&stepSub); // make the step size the length of the original positions without Z

    VectorNormalize(&stepSub);

    while (!initialDone || didMove) {
        initialDone = qtrue;
        didMove = 0;

        stepGoal.x = workingOrg.x + stepSub.x * stepSize;
        stepGoal.y = workingOrg.y + stepSub.y * stepSize;
        stepGoal.z = workingOrg.z + stepSub.z * stepSize;

        trap->Trace(&tr, &workingOrg, mins, maxs, &stepGoal, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

        if (!tr.startsolid && !tr.allsolid && tr.fraction) {
            vector3 vecSub;
            VectorSubtract(&workingOrg, &tr.endpos, &vecSub);

            if (VectorLength(&vecSub) > (stepSize / 2)) {
                workingOrg.x = tr.endpos.x;
                workingOrg.y = tr.endpos.y;
                // trap->LinkEntity((sharedEntity_t *)self);
                didMove = 1;
            }
        }

        if (didMove != 1) { // stair check
            vector3 trFrom;
            vector3 trTo;
            vector3 trDir;
            vector3 vecMeasure;

            VectorCopy(&tr.endpos, &trFrom);
            trFrom.z += 16;

            VectorSubtract(/*tr.endpos*/ &stepGoal, &workingOrg, &trDir);
            VectorNormalize(&trDir);
            trTo.x = tr.endpos.x + trDir.x * 2;
            trTo.y = tr.endpos.y + trDir.y * 2;
            trTo.z = tr.endpos.z + trDir.z * 2;
            trTo.z += 16;

            VectorSubtract(&trFrom, &trTo, &vecMeasure);

            if (VectorLength(&vecMeasure) > 1) {
                trap->Trace(&tr, &trFrom, mins, maxs, &trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

                if (!tr.startsolid && !tr.allsolid && tr.fraction == 1) { // clear trace here, probably up a step
                    vector3 trDown;
                    vector3 trUp;
                    VectorCopy(&tr.endpos, &trUp);
                    VectorCopy(&tr.endpos, &trDown);
                    trDown.z -= 16;

                    trap->Trace(&tr, &trFrom, mins, maxs, &trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

                    if (!tr.startsolid && !tr.allsolid) { // plop us down on the step after moving up
                        VectorCopy(&tr.endpos, &workingOrg);
                        // trap->LinkEntity((sharedEntity_t *)self);
                        didMove = 1;
                    }
                }
            }
        }

        VectorSubtract(&lastIncrement, &workingOrg, &finalMeasure);
        measureLength = VectorLength(&finalMeasure);

        if (!measureLength) { // no progress, break out. If last movement was a sucess didMove will equal 1.
            break;
        }

        stepSize -= measureLength; // subtract the progress distance from the step size so we don't overshoot the mark.
        if (stepSize <= 0) {
            break;
        }

        VectorCopy(&workingOrg, &lastIncrement);
    }

    return didMove;
}

int ConnectTrail(int startindex, int endindex, qboolean behindTheScenes) {
    int foundit;
    int cancontinue;
    int i;
    int failsafe;
    int successnodeindex;
    int insertindex;
    int prenodestart;
    byte extendednodes[MAX_NODETABLE_SIZE]; // for storing checked nodes and not trying to extend them each a bazillion times
    float fvecmeas;
    float baseheight;
    float branchDistance;
    float maxDistFactor = 256;
    vector3 a;
    vector3 startplace, starttrace;
    vector3 mins, maxs;
    vector3 testspot;
    vector3 validspotpos;
    trace_t tr;

    branchDistance = TABLE_BRANCH_DISTANCE;

    VectorSet(&mins, -15, -15, 0);
    VectorSet(&maxs, 15, 15, 0);

    nodenum = 0;
    foundit = 0;

    i = 0;

    successnodeindex = 0;

    while (i < MAX_NODETABLE_SIZE) // clear it out before using it
    {
        nodetable[i].flags = 0;
        //		nodetable[i].index = 0;
        nodetable[i].inuse = 0;
        nodetable[i].neighbornum = 0;
        nodetable[i].origin.x = 0;
        nodetable[i].origin.y = 0;
        nodetable[i].origin.z = 0;
        nodetable[i].weight = 0;

        extendednodes[i] = 0;

        i++;
    }

    if (!behindTheScenes) {
        trap->Print(S_COLOR_YELLOW "Point %i is not connected to %i - Repairing...\n", startindex, endindex);
    }

    VectorCopy(&gWPArray[startindex]->origin, &startplace);

    VectorCopy(&startplace, &starttrace);

    starttrace.z -= 4096;

    trap->Trace(&tr, &startplace, NULL, NULL, &starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

    baseheight = startplace.z - tr.endpos.z;

    cancontinue = 1;

    VectorCopy(&startplace, &nodetable[nodenum].origin);
    nodetable[nodenum].weight = 1;
    nodetable[nodenum].inuse = 1;
    //	nodetable[nodenum].index = nodenum;
    nodenum++;

    while (nodenum < MAX_NODETABLE_SIZE && !foundit && cancontinue) {
        cancontinue = 0;
        i = 0;
        prenodestart = nodenum;

        while (i < prenodestart) {
            if (extendednodes[i] != 1) {
                VectorSubtract(&gWPArray[endindex]->origin, &nodetable[i].origin, &a);
                fvecmeas = VectorLength(&a);

                if (fvecmeas < 128 && CanGetToVector(&gWPArray[endindex]->origin, &nodetable[i].origin, &mins, &maxs)) {
                    foundit = 1;
                    successnodeindex = i;
                    break;
                }

                VectorCopy(&nodetable[i].origin, &testspot);
                testspot.x += branchDistance;

                VectorCopy(&testspot, &starttrace);

                starttrace.z -= 4096;

                trap->Trace(&tr, &testspot, NULL, NULL, &starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

                testspot.z = tr.endpos.z + baseheight;

                if (!NodeHere(&testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(&nodetable[i].origin, &testspot, &mins, &maxs)) {
                    VectorCopy(&testspot, &nodetable[nodenum].origin);
                    nodetable[nodenum].inuse = 1;
                    //					nodetable[nodenum].index = nodenum;
                    nodetable[nodenum].weight = nodetable[i].weight + 1;
                    nodetable[nodenum].neighbornum = i;
                    if ((nodetable[i].origin.z - nodetable[nodenum].origin.z) >
                        50) { // if there's a big drop, make sure we know we can't just magically fly back up
                        nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
                    }
                    nodenum++;
                    cancontinue = 1;
                }

                if (nodenum >= MAX_NODETABLE_SIZE) {
                    break; // failure
                }

                VectorCopy(&nodetable[i].origin, &testspot);
                testspot.x -= branchDistance;

                VectorCopy(&testspot, &starttrace);

                starttrace.z -= 4096;

                trap->Trace(&tr, &testspot, NULL, NULL, &starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

                testspot.z = tr.endpos.z + baseheight;

                if (!NodeHere(&testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(&nodetable[i].origin, &testspot, &mins, &maxs)) {
                    VectorCopy(&testspot, &nodetable[nodenum].origin);
                    nodetable[nodenum].inuse = 1;
                    //					nodetable[nodenum].index = nodenum;
                    nodetable[nodenum].weight = nodetable[i].weight + 1;
                    nodetable[nodenum].neighbornum = i;
                    if ((nodetable[i].origin.z - nodetable[nodenum].origin.z) >
                        50) { // if there's a big drop, make sure we know we can't just magically fly back up
                        nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
                    }
                    nodenum++;
                    cancontinue = 1;
                }

                if (nodenum >= MAX_NODETABLE_SIZE) {
                    break; // failure
                }

                VectorCopy(&nodetable[i].origin, &testspot);
                testspot.y += branchDistance;

                VectorCopy(&testspot, &starttrace);

                starttrace.z -= 4096;

                trap->Trace(&tr, &testspot, NULL, NULL, &starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

                testspot.z = tr.endpos.z + baseheight;

                if (!NodeHere(&testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(&nodetable[i].origin, &testspot, &mins, &maxs)) {
                    VectorCopy(&testspot, &nodetable[nodenum].origin);
                    nodetable[nodenum].inuse = 1;
                    //					nodetable[nodenum].index = nodenum;
                    nodetable[nodenum].weight = nodetable[i].weight + 1;
                    nodetable[nodenum].neighbornum = i;
                    if ((nodetable[i].origin.z - nodetable[nodenum].origin.z) >
                        50) { // if there's a big drop, make sure we know we can't just magically fly back up
                        nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
                    }
                    nodenum++;
                    cancontinue = 1;
                }

                if (nodenum >= MAX_NODETABLE_SIZE) {
                    break; // failure
                }

                VectorCopy(&nodetable[i].origin, &testspot);
                testspot.y -= branchDistance;

                VectorCopy(&testspot, &starttrace);

                starttrace.z -= 4096;

                trap->Trace(&tr, &testspot, NULL, NULL, &starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

                testspot.z = tr.endpos.z + baseheight;

                if (!NodeHere(&testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(&nodetable[i].origin, &testspot, &mins, &maxs)) {
                    VectorCopy(&testspot, &nodetable[nodenum].origin);
                    nodetable[nodenum].inuse = 1;
                    //					nodetable[nodenum].index = nodenum;
                    nodetable[nodenum].weight = nodetable[i].weight + 1;
                    nodetable[nodenum].neighbornum = i;
                    if ((nodetable[i].origin.z - nodetable[nodenum].origin.z) >
                        50) { // if there's a big drop, make sure we know we can't just magically fly back up
                        nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
                    }
                    nodenum++;
                    cancontinue = 1;
                }

                if (nodenum >= MAX_NODETABLE_SIZE) {
                    break; // failure
                }

                extendednodes[i] = 1;
            }

            i++;
        }
    }

    if (!foundit) {
#ifndef _DEBUG // if debug just always print this.
        if (!behindTheScenes)
#endif
        {
            trap->Print(S_COLOR_RED "Could not link %i to %i, unreachable by node branching.\n", startindex, endindex);
        }
        gWPArray[startindex]->flags |= WPFLAG_ONEWAY_FWD;
        gWPArray[endindex]->flags |= WPFLAG_ONEWAY_BACK;
        if (!behindTheScenes) {
            trap->Print(S_COLOR_YELLOW
                        "Since points cannot be connected, point %i has been flagged as only-forward and point %i has been flagged as only-backward.\n",
                        startindex, endindex);
        }

        /*while (nodenum >= 0)
        {
        if (nodetable[nodenum].origin.x || nodetable[nodenum].origin.y || nodetable[nodenum].origin.z)
        {
        CreateNewWP(nodetable[nodenum].origin, nodetable[nodenum].flags);
        }

        nodenum--;
        }*/
        // The above code transfers nodes into the "rendered" waypoint array. Strictly for debugging.

        if (!behindTheScenes) { // just use what we have if we're auto-pathing the level
            return 0;
        } else {
            vector3 endDist;
            int nCount = 0;
            int idealNode = -1;
            float bestDist = 0;
            float testDist;

            if (nodenum <= 10) { // not enough to even really bother.
                return 0;
            }

            // Since it failed, find whichever node is closest to the desired end.
            while (nCount < nodenum) {
                VectorSubtract(&nodetable[nCount].origin, &gWPArray[endindex]->origin, &endDist);
                testDist = VectorLength(&endDist);
                if (idealNode == -1) {
                    idealNode = nCount;
                    bestDist = testDist;
                    nCount++;
                    continue;
                }

                if (testDist < bestDist) {
                    idealNode = nCount;
                    bestDist = testDist;
                }

                nCount++;
            }

            if (idealNode == -1) {
                return 0;
            }

            successnodeindex = idealNode;
        }
    }

    i = successnodeindex;
    insertindex = startindex;
    failsafe = 0;
    VectorCopy(&gWPArray[startindex]->origin, &validspotpos);

    while (failsafe < MAX_NODETABLE_SIZE && i < MAX_NODETABLE_SIZE && i >= 0) {
        VectorSubtract(&validspotpos, &nodetable[i].origin, &a);
        if (!nodetable[nodetable[i].neighbornum].inuse ||
            !CanGetToVectorTravel(&validspotpos, /*nodetable[nodetable[i].neighbornum].origin*/ &nodetable[i].origin, &mins, &maxs) ||
            VectorLength(&a) > maxDistFactor ||
            (!CanGetToVectorTravel(&validspotpos, &gWPArray[endindex]->origin, &mins, &maxs) &&
             CanGetToVectorTravel(&nodetable[i].origin, &gWPArray[endindex]->origin, &mins, &maxs))) {
            nodetable[i].flags |= WPFLAG_CALCULATED;
            if (!CreateNewWP_InTrail(&nodetable[i].origin, nodetable[i].flags, insertindex)) {
                if (!behindTheScenes) {
                    trap->Print(S_COLOR_RED "Could not link %i to %i, waypoint limit hit.\n", startindex, endindex);
                }
                return 0;
            }

            VectorCopy(&nodetable[i].origin, &validspotpos);
        }

        if (i == 0) {
            break;
        }

        i = nodetable[i].neighbornum;

        failsafe++;
    }

    if (!behindTheScenes) {
        trap->Print(S_COLOR_YELLOW "Finished connecting %i to %i.\n", startindex, endindex);
    }

    return 1;
}

int OpposingEnds(int start, int end) {
    if (!gWPArray[start] || !gWPArray[end]) {
        return 0;
    }

    if ((gWPArray[start]->flags & WPFLAG_ONEWAY_FWD) && (gWPArray[end]->flags & WPFLAG_ONEWAY_BACK)) {
        return 1;
    }

    return 0;
}

int DoorBlockingSection(int start, int end) { // if a door blocks the trail, we'll just have to assume the points on each side are in visibility when it's open
    trace_t tr;
    gentity_t *testdoor;
    int start_trace_index;

    if (!gWPArray[start] || !gWPArray[end]) {
        return 0;
    }

    trap->Trace(&tr, &gWPArray[start]->origin, NULL, NULL, &gWPArray[end]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

    if (tr.fraction == 1) {
        return 0;
    }

    testdoor = &g_entities[tr.entityNum];

    if (!testdoor) {
        return 0;
    }

    if (!strstr(testdoor->classname, "func_")) {
        return 0;
    }

    start_trace_index = tr.entityNum;

    trap->Trace(&tr, &gWPArray[end]->origin, NULL, NULL, &gWPArray[start]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

    if (tr.fraction == 1) {
        return 0;
    }

    if (start_trace_index == tr.entityNum) {
        return 1;
    }

    return 0;
}

int RepairPaths(qboolean behindTheScenes) {
    vector3 a;
    float maxDistFactor = 400;

    if (gWPArray.empty()) {
        return 0;
    }

    trap->Cvar_Update(&bot_wp_distconnect);
    trap->Cvar_Update(&bot_wp_visconnect);

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i + 1]) {
            VectorSubtract(&gWPArray[i]->origin, &gWPArray[i + 1]->origin, &a);

            if (!(gWPArray[i + 1]->flags & WPFLAG_NOVIS) &&
                !(gWPArray[i + 1]->flags &
                  WPFLAG_JUMP) && // don't calculate on jump points because they might not always want to be visible (in cases of force jumping)
                !(gWPArray[i]->flags & WPFLAG_CALCULATED) && // don't calculate it again
                !OpposingEnds(i, i + 1) &&
                ((bot_wp_distconnect.value && VectorLength(&a) > maxDistFactor) ||
                 (!OrgVisible(&gWPArray[i]->origin, &gWPArray[i + 1]->origin, ENTITYNUM_NONE) && bot_wp_visconnect.value)) &&
                !DoorBlockingSection(i, i + 1)) {
                ConnectTrail(i, i + 1, behindTheScenes);
            }
        }
    }

    return 1;
}

int OrgVisibleCurve(vector3 *org1, vector3 *mins, vector3 *maxs, vector3 *org2, int ignore) {
    trace_t tr;
    vector3 evenorg1;

    VectorCopy(org1, &evenorg1);
    evenorg1.z = org2->z;

    trap->Trace(&tr, &evenorg1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);

    if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid) {
        trap->Trace(&tr, &evenorg1, mins, maxs, org1, ignore, MASK_SOLID, qfalse, 0, 0);

        if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid) {
            return 1;
        }
    }

    return 0;
}

int CanForceJumpTo(int baseindex, int testingindex, float distance) {
    float heightdif;
    vector3 xy_base, xy_test, mins, maxs;
    wpobject_t *wpBase = gWPArray[baseindex];
    wpobject_t *wpTest = gWPArray[testingindex];

    VectorSet(&mins, -15, -15, -15);
    VectorSet(&maxs, 15, 15, 15);

    if (!wpBase || !wpTest) {
        return 0;
    }

    if (distance > 400) {
        return 0;
    }

    VectorCopy(&wpBase->origin, &xy_base);
    VectorCopy(&wpTest->origin, &xy_test);

    xy_base.z = xy_test.z;

    vector3 testVec;
    VectorSubtract(&xy_base, &xy_test, &testVec);

    if (VectorLength(&testVec) > MAX_NEIGHBOR_LINK_DISTANCE) {
        return 0;
    }

    if ((int)wpBase->origin.z < (int)wpTest->origin.z) {
        heightdif = wpTest->origin.z - wpBase->origin.z;
    } else {
        return 0; // err..
    }

    if (heightdif < 128) { // don't bother..
        return 0;
    }

    if (heightdif > 512) { // too high
        return 0;
    }

    if (!OrgVisibleCurve(&wpBase->origin, &mins, &maxs, &wpTest->origin, ENTITYNUM_NONE)) {
        return 0;
    }

    if (heightdif > 400) {
        return 3;
    } else if (heightdif > 256) {
        return 2;
    } else {
        return 1;
    }
}

void CalculatePaths(void) {
    int forceJumpable;
    int maxNeighborDist = MAX_NEIGHBOR_LINK_DISTANCE;
    float nLDist;
    vector3 a;
    vector3 mins, maxs;

    if (gWPArray.empty()) {
        return;
    }

    VectorSet(&mins, -15, -15, -15);
    VectorSet(&maxs, 15, 15, 15);

    // now clear out all the neighbor data before we recalculate
    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i]->neighbornum) {
            while (gWPArray[i]->neighbornum >= 0) {
                gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = 0;
                gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
                gWPArray[i]->neighbornum--;
            }
            gWPArray[i]->neighbornum = 0;
        }
    }

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            for (int c = 0; c < size; c++) {
                if (gWPArray[c] && i != c && NotWithinRange(i, c)) {
                    VectorSubtract(&gWPArray[i]->origin, &gWPArray[c]->origin, &a);

                    nLDist = VectorLength(&a);
                    forceJumpable = CanForceJumpTo(i, c, nLDist);

                    if ((nLDist < maxNeighborDist || forceJumpable) && ((int)gWPArray[i]->origin.z == (int)gWPArray[c]->origin.z || forceJumpable) &&
                        (OrgVisibleBox(&gWPArray[i]->origin, &mins, &maxs, &gWPArray[c]->origin, ENTITYNUM_NONE) || forceJumpable)) {
                        gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = c;
                        if (forceJumpable && ((int)gWPArray[i]->origin.z != (int)gWPArray[c]->origin.z || nLDist < maxNeighborDist)) {
                            gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 999; // forceJumpable; //FJSR
                        } else {
                            gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
                        }
                        gWPArray[i]->neighbornum++;
                    }

                    if (gWPArray[i]->neighbornum >= MAX_NEIGHBOR_SIZE) {
                        break;
                    }
                }
            }
        }
    }
}

gentity_t *GetObjectThatTargets(gentity_t *ent) {
    gentity_t *next = NULL;

    if (!ent->targetname) {
        return NULL;
    }

    next = G_Find(next, FOFS(target), ent->targetname);

    if (next) {
        return next;
    }

    return NULL;
}

void CalculateSiegeGoals(void) {
    int i = 0;
    int looptracker = 0;
    int wpindex = 0;
    vector3 dif;
    gentity_t *ent;
    gentity_t *tent = NULL, *t2ent = NULL;

    while (i < level.num_entities) {
        ent = &g_entities[i];

        tent = NULL;

        if (ent && ent->classname && strcmp(ent->classname, "info_siege_objective") == 0) {
            tent = ent;
            t2ent = GetObjectThatTargets(tent);
            looptracker = 0;

            while (t2ent && looptracker < 2048) { // looptracker keeps us from getting stuck in case something is set up weird on this map
                tent = t2ent;
                t2ent = GetObjectThatTargets(tent);
                looptracker++;
            }

            if (looptracker >= 2048) { // something unpleasent has happened
                tent = NULL;
                break;
            }
        }

        if (tent && ent && tent != ent) { // tent should now be the object attached to the mission objective
            dif.x = (tent->r.absmax.x + tent->r.absmin.x) / 2;
            dif.y = (tent->r.absmax.y + tent->r.absmin.y) / 2;
            dif.z = (tent->r.absmax.z + tent->r.absmin.z) / 2;

            wpindex = GetNearestVisibleWP(&dif, tent->s.number);

            if (wpindex != -1 && gWPArray[wpindex]) { // found the waypoint nearest the center of this objective-related object
                if (ent->side == SIEGETEAM_TEAM1) {
                    gWPArray[wpindex]->flags |= WPFLAG_SIEGE_IMPERIALOBJ;
                } else {
                    gWPArray[wpindex]->flags |= WPFLAG_SIEGE_REBELOBJ;
                }

                gWPArray[wpindex]->associated_entity = tent->s.number;
            }
        }

        i++;
    }
}

float botGlobalNavWeaponWeights[WP_NUM_WEAPONS] = {
    0, // WP_NONE,

    0, // WP_STUN_BATON,
    0, // WP_MELEE
    0, // WP_SABER,				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
    0, // WP_BRYAR_PISTOL,
    3, // WP_BLASTER,
    5, // WP_DISRUPTOR,
    4, // WP_BOWCASTER,
    6, // WP_REPEATER,
    7, // WP_DEMP2,
    8, // WP_FLECHETTE,
    9, // WP_ROCKET_LAUNCHER,
    3, // WP_THERMAL,
    3, // WP_TRIP_MINE,
    3, // WP_DET_PACK,
    0  // WP_EMPLACED_GUN,
};

int GetNearestVisibleWPToItem(vector3 *org, int ignore) {
    float bestdist;
    float flLen;
    int bestindex;
    vector3 a, mins, maxs;

    bestdist = 64; // has to be less than 64 units to the item or it isn't safe enough
    bestindex = -1;

    VectorSet(&mins, -15, -15, 0);
    VectorSet(&maxs, 15, 15, 0);

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i] && gWPArray[i]->origin.z - 15 < org->z && gWPArray[i]->origin.z + 15 > org->z) {
            VectorSubtract(org, &gWPArray[i]->origin, &a);
            flLen = VectorLength(&a);

            if (flLen < bestdist && trap->InPVS(org, &gWPArray[i]->origin) && OrgVisibleBox(org, &mins, &maxs, &gWPArray[i]->origin, ignore)) {
                bestdist = flLen;
                bestindex = i;
            }
        }
    }

    return bestindex;
}

void CalculateWeightGoals(void) { // set waypoint weights depending on weapon and item placement
    int wpindex = 0;
    gentity_t *ent;
    float weight;

    trap->Cvar_Update(&bot_wp_clearweight);

    if (bot_wp_clearweight.integer) { // if set then flush out all weight/goal values before calculating them again
        for (int i = 0, size = gWPArray.size(); i < size; i++) {
            if (gWPArray[i]) {
                gWPArray[i]->weight = 0;

                if (gWPArray[i]->flags & WPFLAG_GOALPOINT) {
                    gWPArray[i]->flags &= ~WPFLAG_GOALPOINT;
                }
            }
        }
    }

    for (int i = 0; i < level.num_entities; i++) {
        ent = &g_entities[i];

        weight = 0;

        if (ent && ent->classname) {
            if (!strcmp(ent->classname, "item_seeker"))
                weight = 2;
            else if (!strcmp(ent->classname, "item_shield"))
                weight = 2;
            else if (!strcmp(ent->classname, "item_medpac"))
                weight = 2;
            else if (!strcmp(ent->classname, "item_sentry_gun"))
                weight = 2;
            else if (!strcmp(ent->classname, "item_force_enlighten_dark"))
                weight = 5;
            else if (!strcmp(ent->classname, "item_force_enlighten_light"))
                weight = 5;
            else if (!strcmp(ent->classname, "item_force_boon"))
                weight = 5;
            else if (!strcmp(ent->classname, "item_ysalimari"))
                weight = 2;
            else if (ent->item && strstr(ent->classname, "weapon_"))
                weight = botGlobalNavWeaponWeights[ent->item->giTag];
            else if (ent->item && ent->item->giType == IT_AMMO)
                weight = 3;
        }

        if (ent && weight) {
            wpindex = GetNearestVisibleWPToItem(&ent->s.pos.trBase, ent->s.number);

            if (wpindex != -1 && gWPArray[wpindex]) { // found the waypoint nearest the center of this object
                gWPArray[wpindex]->weight = weight;
                gWPArray[wpindex]->flags |= WPFLAG_GOALPOINT;
                gWPArray[wpindex]->associated_entity = ent->s.number;
            }
        }
    }
}

void CalculateJumpRoutes(void) {
    float nheightdif = 0;
    float pheightdif = 0;

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            if (gWPArray[i]->flags & WPFLAG_JUMP) {
                nheightdif = 0;
                pheightdif = 0;

                gWPArray[i]->forceJumpTo = 0;

                if (i > 0 && gWPArray[i - 1] && (gWPArray[i - 1]->origin.z + 16) < gWPArray[i]->origin.z)
                    nheightdif = (gWPArray[i]->origin.z - gWPArray[i - 1]->origin.z);

                if (i + 1 < gWPArray.size() && gWPArray[i + 1] && (gWPArray[i + 1]->origin.z + 16) < gWPArray[i]->origin.z)
                    pheightdif = (gWPArray[i]->origin.z - gWPArray[i + 1]->origin.z);

                if (nheightdif > pheightdif)
                    pheightdif = nheightdif;

                if (pheightdif) {
                    if (pheightdif > 500)
                        gWPArray[i]->forceJumpTo = 999; // FORCE_LEVEL_3; //FJSR
                    else if (pheightdif > 256)
                        gWPArray[i]->forceJumpTo = 999; // FORCE_LEVEL_2; //FJSR
                    else if (pheightdif > 128)
                        gWPArray[i]->forceJumpTo = 999; // FORCE_LEVEL_1; //FJSR
                }
            }
        }
    }
}

int LoadPathData(const char *filename) {
    int i, i_cv;
    int nei_num;

    i = 0;

    char routePath[MAX_QPATH];
    Com_sprintf(routePath, sizeof(routePath), "botroutes/%s.wnt", filename);
    fileHandle_t f;
    int len = trap->FS_Open(routePath, &f, FS_READ);
    if (!f) {
        trap->Print(S_COLOR_YELLOW "Bot route data not found for %s\n", filename);
        return 2;
    }

    char *fileContents = (char *)B_AllocZ(len);
    char currentVar[MAX_TOKEN_CHARS];

    trap->FS_Read(fileContents, len, f);

    if (fileContents[i] == 'l') { // contains a "levelflags" entry..
        char readLFlags[64];
        i_cv = 0;

        while (fileContents[i] != ' ') {
            i++;
        }
        i++;
        while (fileContents[i] != '\n') {
            readLFlags[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        readLFlags[i_cv] = 0;
        i++;

        gLevelFlags = atoi(readLFlags);
    } else {
        gLevelFlags = 0;
    }

    while (i < len) {
        i_cv = 0;

        wpobject_t thiswp = {};
        thiswp.index = 0;
        thiswp.flags = 0;
        thiswp.neighbornum = 0;
        thiswp.origin.x = 0;
        thiswp.origin.y = 0;
        thiswp.origin.z = 0;
        thiswp.weight = 0;
        thiswp.associated_entity = ENTITYNUM_NONE;
        thiswp.forceJumpTo = 0;
        thiswp.disttonext = 0;
        nei_num = 0;

        while (nei_num < MAX_NEIGHBOR_SIZE) {
            thiswp.neighbors[nei_num].num = 0;
            thiswp.neighbors[nei_num].forceJumpTo = 0;

            nei_num++;
        }

        while (fileContents[i] != ' ') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.index = atoi(currentVar);

        i_cv = 0;
        i++;

        while (fileContents[i] != ' ') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.flags = atoi(currentVar);

        i_cv = 0;
        i++;

        while (fileContents[i] != ' ') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.weight = atof(currentVar);

        i_cv = 0;
        i++;
        i++;

        while (fileContents[i] != ' ') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.origin.x = atof(currentVar);

        i_cv = 0;
        i++;

        while (fileContents[i] != ' ') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.origin.y = atof(currentVar);

        i_cv = 0;
        i++;

        while (fileContents[i] != ')') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.origin.z = atof(currentVar);

        i += 4;

        while (fileContents[i] != '}') {
            i_cv = 0;
            while (fileContents[i] != ' ' && fileContents[i] != '-') {
                currentVar[i_cv] = fileContents[i];
                i_cv++;
                i++;
            }
            currentVar[i_cv] = '\0';

            thiswp.neighbors[thiswp.neighbornum].num = atoi(currentVar);

            if (fileContents[i] == '-') {
                i_cv = 0;
                i++;

                while (fileContents[i] != ' ') {
                    currentVar[i_cv] = fileContents[i];
                    i_cv++;
                    i++;
                }
                currentVar[i_cv] = '\0';

                thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 999; // atoi(currentVar); //FJSR
            } else {
                thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 0;
            }

            thiswp.neighbornum++;

            i++;
        }

        i_cv = 0;
        i++;
        i++;

        while (fileContents[i] != '\n') {
            currentVar[i_cv] = fileContents[i];
            i_cv++;
            i++;
        }
        currentVar[i_cv] = '\0';

        thiswp.disttonext = atof(currentVar);

        CreateNewWP_FromObject(&thiswp);
        i++;
    }

    B_Free(fileContents);

    trap->FS_Close(f);

    if (level.gametype == GT_SIEGE) {
        CalculateSiegeGoals();
    }

    CalculateWeightGoals();
    // calculate weights for idle activity goals when
    // the bot has absolutely nothing else to do

    CalculateJumpRoutes();
    // Look at jump points and mark them as requiring
    // force jumping as needed

    return 1;
}

void FlagObjects(void) {
    int bestindex = 0, found = 0;
    float bestdist = 999999, tlen = 0;
    gentity_t *flag_red, *flag_blue, *ent;
    vector3 a, mins, maxs;
    trace_t tr;

    flag_red = NULL;
    flag_blue = NULL;

    VectorSet(&mins, -15, -15, -5);
    VectorSet(&maxs, 15, 15, 5);

    for (int i = 0; i < level.num_entities; i++) {
        ent = &g_entities[i];

        if (G_IsValidEntity(ent) && ent->classname) {
            if (!flag_red && strcmp(ent->classname, "team_CTF_redflag") == 0)
                flag_red = ent;
            else if (!flag_blue && strcmp(ent->classname, "team_CTF_blueflag") == 0)
                flag_blue = ent;

            if (flag_red && flag_blue)
                break;
        }
    }

    if (!flag_red || !flag_blue) {
        return;
    }

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            VectorSubtract(&flag_red->s.pos.trBase, &gWPArray[i]->origin, &a);
            tlen = VectorLength(&a);

            if (tlen < bestdist) {
                trap->Trace(&tr, &flag_red->s.pos.trBase, &mins, &maxs, &gWPArray[i]->origin, flag_red->s.number, MASK_SOLID, qfalse, 0, 0);

                if (tr.fraction == 1 || tr.entityNum == flag_red->s.number) {
                    bestdist = tlen;
                    bestindex = i;
                    found = 1;
                }
            }
        }
    }

    if (found) {
        gWPArray[bestindex]->flags |= WPFLAG_RED_FLAG;
        flagRed = gWPArray[bestindex];
        oFlagRed = flagRed;
        eFlagRed = flag_red;
    }

    bestdist = 999999;
    bestindex = 0;
    found = 0;

    for (int i = 0, size = gWPArray.size(); i < size; i++) {
        if (gWPArray[i]) {
            VectorSubtract(&flag_blue->s.pos.trBase, &gWPArray[i]->origin, &a);
            tlen = VectorLength(&a);

            if (tlen < bestdist) {
                trap->Trace(&tr, &flag_blue->s.pos.trBase, &mins, &maxs, &gWPArray[i]->origin, flag_blue->s.number, MASK_SOLID, qfalse, 0, 0);

                if (tr.fraction == 1 || tr.entityNum == flag_blue->s.number) {
                    bestdist = tlen;
                    bestindex = i;
                    found = 1;
                }
            }
        }
    }

    if (found) {
        gWPArray[bestindex]->flags |= WPFLAG_BLUE_FLAG;
        flagBlue = gWPArray[bestindex];
        oFlagBlue = flagBlue;
        eFlagBlue = flag_blue;
    }
}

int SavePathData(const char *filename) {
    fileHandle_t f;
    char *fileString;
    char *storeString;
    char *routePath;
    vector3 a;
    float flLen;
    int i, n;

    fileString = NULL;
    i = 0;

    if (gWPArray.empty()) {
        return 0;
    }

    routePath = (char *)B_TempAlloc(1024);

    Com_sprintf(routePath, 1024, "botroutes/%s.wnt", filename);

    trap->FS_Open(routePath, &f, FS_WRITE);

    B_TempFree(1024); // routePath

    if (!f) {
        trap->Print(S_COLOR_RED "ERROR: Could not open file to write path data\n");
        return 0;
    }

    if (!RepairPaths(qfalse)) // check if we can see all waypoints from the last. If not, try to branch over.
    {
        trap->FS_Close(f);
        return 0;
    }

    CalculatePaths(); // make everything nice and connected before saving

    FlagObjects(); // currently only used for flagging waypoints nearest CTF flags

    fileString = (char *)B_TempAlloc(524288);
    storeString = (char *)B_TempAlloc(4096);

    Com_sprintf(fileString, 524288, "%i %i %f %s { ", gWPArray[i]->index, gWPArray[i]->flags, (double)gWPArray[i]->weight, vtos(&gWPArray[i]->origin));

    n = 0;

    while (n < gWPArray[i]->neighbornum) {
        if (gWPArray[i]->neighbors[n].forceJumpTo) {
            Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num, gWPArray[i]->neighbors[n].forceJumpTo);
        } else {
            Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
        }
        n++;
    }

    if (gWPArray[i + 1] && gWPArray[i + 1]->index) {
        VectorSubtract(&gWPArray[i]->origin, &gWPArray[i + 1]->origin, &a);
        flLen = VectorLength(&a);
    } else {
        flLen = 0;
    }

    gWPArray[i]->disttonext = flLen;

    Com_sprintf(fileString, 524288, "%s} %f\n", fileString, (double)flLen);

    i++;

    for (int size = gWPArray.size(); i < size; i++) {
        // sprintf(fileString, "%s%i %i %f (%f %f %f) { ", fileString, gWPArray[i]->index, gWPArray[i]->flags, gWPArray[i]->weight, gWPArray[i]->origin.x,
        // gWPArray[i]->origin.y, gWPArray[i]->origin.z);
        Com_sprintf(storeString, 4096, "%i %i %f %s { ", gWPArray[i]->index, gWPArray[i]->flags, (double)gWPArray[i]->weight, vtos(&gWPArray[i]->origin));

        n = 0;

        while (n < gWPArray[i]->neighbornum) {
            if (gWPArray[i]->neighbors[n].forceJumpTo) {
                Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num, gWPArray[i]->neighbors[n].forceJumpTo);
            } else {
                Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
            }
            n++;
        }

        if (gWPArray[i + 1] && gWPArray[i + 1]->index) {
            VectorSubtract(&gWPArray[i]->origin, &gWPArray[i + 1]->origin, &a);
            flLen = VectorLength(&a);
        } else {
            flLen = 0;
        }

        gWPArray[i]->disttonext = flLen;

        Com_sprintf(storeString, 4096, "%s} %f\n", storeString, (double)flLen);

        strcat(fileString, storeString);
    }

    trap->FS_Write(fileString, strlen(fileString), f);

    B_TempFree(524288); // fileString
    B_TempFree(4096);   // storeString

    trap->FS_Close(f);

    trap->Print("Path data has been saved and updated. You may need to restart the level for some things to be properly calculated.\n");

    return 1;
}

#define MAX_SPAWNPOINT_ARRAY 64
int gSpawnPointNum = 0;
gentity_t *gSpawnPoints[MAX_SPAWNPOINT_ARRAY];

int G_NearestNodeToPoint(vector3 *point) { // gets the node on the entire grid which is nearest to the specified coordinates.
    vector3 vSub;
    int bestIndex = -1;
    int i = 0;
    float bestDist = 0;
    float testDist = 0;

    while (i < nodenum) {
        VectorSubtract(&nodetable[i].origin, point, &vSub);
        testDist = VectorLength(&vSub);

        if (bestIndex == -1) {
            bestIndex = i;
            bestDist = testDist;

            i++;
            continue;
        }

        if (testDist < bestDist) {
            bestIndex = i;
            bestDist = testDist;
        }
        i++;
    }

    return bestIndex;
}

void G_NodeClearForNext(void) { // reset nodes for the next trail connection.
    int i = 0;

    while (i < nodenum) {
        nodetable[i].flags = 0;
        nodetable[i].weight = 99999;

        i++;
    }
}

void G_NodeClearFlags(void) { // only clear out flags so nodes can be reused.
    int i = 0;

    while (i < nodenum) {
        nodetable[i].flags = 0;

        i++;
    }
}

int G_NodeMatchingXY(float x, float y) { // just get the first unflagged node with the matching x,y coordinates.
    int i = 0;

    while (i < nodenum) {
        if (nodetable[i].origin.x == x && nodetable[i].origin.y == y && !nodetable[i].flags) {
            return i;
        }

        i++;
    }

    return -1;
}

int G_NodeMatchingXY_BA(int x, int y, int final) { // return the node with the lowest weight that matches the specified x,y coordinates.
    int i = 0;
    int bestindex = -1;
    float bestWeight = 9999;

    while (i < nodenum) {
        if ((int)nodetable[i].origin.x == x && (int)nodetable[i].origin.y == y && !nodetable[i].flags && ((nodetable[i].weight < bestWeight) || (i == final))) {
            if (i == final) {
                return i;
            }
            bestindex = i;
            bestWeight = nodetable[i].weight;
        }

        i++;
    }

    return bestindex;
}

int G_RecursiveConnection(int start, int end, int weight, qboolean traceCheck, float baseHeight) {
    int indexDirections[4]; // 0 == down, 1 == up, 2 == left, 3 == right
    int recursiveIndex = -1;
    int i = 0;
    int passWeight = weight;
    vector2 givenXY;
    trace_t tr;

    passWeight++;
    nodetable[start].weight = passWeight;

    givenXY.x = nodetable[start].origin.x;
    givenXY.y = nodetable[start].origin.y;
    givenXY.x -= DEFAULT_GRID_SPACING;
    indexDirections[0] = G_NodeMatchingXY(givenXY.x, givenXY.y);

    givenXY.x = nodetable[start].origin.x;
    givenXY.y = nodetable[start].origin.y;
    givenXY.x += DEFAULT_GRID_SPACING;
    indexDirections[1] = G_NodeMatchingXY(givenXY.x, givenXY.y);

    givenXY.x = nodetable[start].origin.x;
    givenXY.y = nodetable[start].origin.y;
    givenXY.y -= DEFAULT_GRID_SPACING;
    indexDirections[2] = G_NodeMatchingXY(givenXY.x, givenXY.y);

    givenXY.x = nodetable[start].origin.x;
    givenXY.y = nodetable[start].origin.y;
    givenXY.y += DEFAULT_GRID_SPACING;
    indexDirections[3] = G_NodeMatchingXY(givenXY.x, givenXY.y);

    i = 0;
    while (i < 4) {
        if (indexDirections[i] == end) { // we've connected all the way to the destination.
            return indexDirections[i];
        }

        if (indexDirections[i] != -1 && nodetable[indexDirections[i]].flags) { // this point is already used, so it's not valid.
            indexDirections[i] = -1;
        } else if (indexDirections[i] != -1) { // otherwise mark it as used.
            nodetable[indexDirections[i]].flags = 1;
        }

        if (indexDirections[i] != -1 &&
            traceCheck) { // if we care about trace visibility between nodes, perform the check and mark as not valid if the trace isn't clear.
            trap->Trace(&tr, &nodetable[start].origin, NULL, NULL, &nodetable[indexDirections[i]].origin, ENTITYNUM_NONE, CONTENTS_SOLID, qfalse, 0, 0);

            if (tr.fraction != 1) {
                indexDirections[i] = -1;
            }
        }

        if (indexDirections[i] != -1) { // it's still valid, so keep connecting via this point.
            recursiveIndex = G_RecursiveConnection(indexDirections[i], end, passWeight, traceCheck, baseHeight);
        }

        if (recursiveIndex != -1) { // the result of the recursive check was valid, so return it.
            return recursiveIndex;
        }

        i++;
    }

    return recursiveIndex;
}

#ifdef DEBUG_NODE_FILE
void G_DebugNodeFile(void) {
    fileHandle_t f;
    int i = 0;
    float placeX;
    char fileString[131072];
    gentity_t *terrain = G_Find(NULL, FOFS(classname), "terrain");

    fileString[0] = 0;

    placeX = terrain->r.absmin[0];

    while (i < nodenum) {
        strcat(fileString, va("%i-%f ", i, nodetable[i].weight));
        placeX += DEFAULT_GRID_SPACING;

        if (placeX >= terrain->r.absmax[0]) {
            strcat(fileString, "\n");
            placeX = terrain->r.absmin[0];
        }
        i++;
    }

    trap->FS_Open("ROUTEDEBUG.txt", &f, FS_WRITE);
    trap->FS_Write(fileString, strlen(fileString), f);
    trap->FS_Close(f);
}
#endif

//#define ASCII_ART_DEBUG
//#define ASCII_ART_NODE_DEBUG

#ifdef ASCII_ART_DEBUG

#define ALLOWABLE_DEBUG_FILE_SIZE 1048576

void CreateAsciiTableRepresentation(void) { // Draw a text grid of the entire waypoint array (useful for debugging final waypoint placement)
    fileHandle_t f;
    int i = 0;
    int sP = 0;
    int placeX;
    int placeY;
    int oldX;
    int oldY;
    char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
    char bChr = '+';
    gentity_t *terrain = G_Find(NULL, FOFS(classname), "terrain");

    placeX = terrain->r.absmin[0];
    placeY = terrain->r.absmin[1];

    oldX = placeX - 1;
    oldY = placeY - 1;

    while (placeY < terrain->r.absmax[1]) {
        while (placeX < terrain->r.absmax[0]) {
            qboolean gotit = qfalse;

            i = 0;
            for (int size = gWPArray.size(); i < size; i++) {
                if (((int)gWPArray[i]->origin.x <= placeX && (int)gWPArray[i]->origin.x > oldX) &&
                    ((int)gWPArray[i]->origin.y <= placeY && (int)gWPArray[i]->origin.y > oldY)) {
                    gotit = qtrue;
                    break;
                }
            }

            if (gotit) {
                if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD) {
                    bChr = 'F';
                } else if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK) {
                    bChr = 'B';
                } else {
                    bChr = '+';
                }

                if (gWPArray[i]->index < 10) {
                    fileString[sP] = bChr;
                    fileString[sP + 1] = '0';
                    fileString[sP + 2] = '0';
                    fileString[sP + 3] = va("%i", gWPArray[i]->index)[0];
                } else if (gWPArray[i]->index < 100) {
                    char *vastore = va("%i", gWPArray[i]->index);

                    fileString[sP] = bChr;
                    fileString[sP + 1] = '0';
                    fileString[sP + 2] = vastore[0];
                    fileString[sP + 3] = vastore[1];
                } else if (gWPArray[i]->index < 1000) {
                    char *vastore = va("%i", gWPArray[i]->index);

                    fileString[sP] = bChr;
                    fileString[sP + 1] = vastore[0];
                    fileString[sP + 2] = vastore[1];
                    fileString[sP + 3] = vastore[2];
                } else {
                    fileString[sP] = 'X';
                    fileString[sP + 1] = 'X';
                    fileString[sP + 2] = 'X';
                    fileString[sP + 3] = 'X';
                }
            } else {
                fileString[sP] = '-';
                fileString[sP + 1] = '-';
                fileString[sP + 2] = '-';
                fileString[sP + 3] = '-';
            }

            sP += 4;

            if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16) {
                break;
            }
            oldX = placeX;
            placeX += DEFAULT_GRID_SPACING;
        }

        placeX = terrain->r.absmin[0];
        oldX = placeX - 1;
        fileString[sP] = '\n';
        sP++;

        if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16) {
            break;
        }

        oldY = placeY;
        placeY += DEFAULT_GRID_SPACING;
    }

    fileString[sP] = 0;

    trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
    trap->FS_Write(fileString, strlen(fileString), f);
    trap->FS_Close(f);
}

void CreateAsciiNodeTableRepresentation(int start, int end) { // draw a text grid of a single node path, from point A to Z.
    fileHandle_t f;
    int i = 0;
    int sP = 0;
    int placeX;
    int placeY;
    int oldX;
    int oldY;
    char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
    gentity_t *terrain = G_Find(NULL, FOFS(classname), "terrain");

    placeX = terrain->r.absmin[0];
    placeY = terrain->r.absmin[1];

    oldX = placeX - 1;
    oldY = placeY - 1;

    while (placeY < terrain->r.absmax[1]) {
        while (placeX < terrain->r.absmax[0]) {
            qboolean gotit = qfalse;

            i = 0;
            while (i < nodenum) {
                if (((int)nodetable[i].origin.x <= placeX && (int)nodetable[i].origin.x > oldX) &&
                    ((int)nodetable[i].origin.y <= placeY && (int)nodetable[i].origin.y > oldY)) {
                    gotit = qtrue;
                    break;
                }
                i++;
            }

            if (gotit) {
                if (i == start) { // beginning of the node trail
                    fileString[sP] = 'A';
                    fileString[sP + 1] = 'A';
                    fileString[sP + 2] = 'A';
                    fileString[sP + 3] = 'A';
                } else if (i == end) { // destination of the node trail
                    fileString[sP] = 'Z';
                    fileString[sP + 1] = 'Z';
                    fileString[sP + 2] = 'Z';
                    fileString[sP + 3] = 'Z';
                } else if (nodetable[i].weight < 10) {
                    fileString[sP] = '+';
                    fileString[sP + 1] = '0';
                    fileString[sP + 2] = '0';
                    fileString[sP + 3] = va("%f", nodetable[i].weight)[0];
                } else if (nodetable[i].weight < 100) {
                    char *vastore = va("%f", nodetable[i].weight);

                    fileString[sP] = '+';
                    fileString[sP + 1] = '0';
                    fileString[sP + 2] = vastore[0];
                    fileString[sP + 3] = vastore[1];
                } else if (nodetable[i].weight < 1000) {
                    char *vastore = va("%f", nodetable[i].weight);

                    fileString[sP] = '+';
                    fileString[sP + 1] = vastore[0];
                    fileString[sP + 2] = vastore[1];
                    fileString[sP + 3] = vastore[2];
                } else {
                    fileString[sP] = 'X';
                    fileString[sP + 1] = 'X';
                    fileString[sP + 2] = 'X';
                    fileString[sP + 3] = 'X';
                }
            } else {
                fileString[sP] = '-';
                fileString[sP + 1] = '-';
                fileString[sP + 2] = '-';
                fileString[sP + 3] = '-';
            }

            sP += 4;

            if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16) {
                break;
            }
            oldX = placeX;
            placeX += DEFAULT_GRID_SPACING;
        }

        placeX = terrain->r.absmin[0];
        oldX = placeX - 1;
        fileString[sP] = '\n';
        sP++;

        if (sP >= ALLOWABLE_DEBUG_FILE_SIZE - 16) {
            break;
        }

        oldY = placeY;
        placeY += DEFAULT_GRID_SPACING;
    }

    fileString[sP] = 0;

    trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
    trap->FS_Write(fileString, strlen(fileString), f);
    trap->FS_Close(f);
}
#endif

#ifdef _DEBUG
#define PATH_TIME_DEBUG
#endif

void LoadPath_ThisLevel(void) {
    vmCvar_t mapname;
    int i = 0;
    gentity_t *ent = NULL;

    trap->Cvar_Register(&mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM);

    if (LoadPathData(level.rawmapname) == 2) {
        // enter "edit" mode if cheats enabled?
    }

    trap->Cvar_Update(&bot_wp_edit);

    if (bot_wp_edit.value) {
        gBotEdit = 1;
    } else {
        gBotEdit = 0;
    }

    // set the flag entities
    while (i < level.num_entities) {
        ent = &g_entities[i];

        if (ent && ent->inuse && ent->classname) {
            if (!eFlagRed && strcmp(ent->classname, "team_CTF_redflag") == 0) {
                eFlagRed = ent;
            } else if (!eFlagBlue && strcmp(ent->classname, "team_CTF_blueflag") == 0) {
                eFlagBlue = ent;
            }

            if (eFlagRed && eFlagBlue) {
                break;
            }
        }

        i++;
    }
}

gentity_t *GetClosestSpawn(gentity_t *ent) {
    gentity_t *spawn;
    gentity_t *closestSpawn = NULL;
    float closestDist = -1;
    int i = MAX_CLIENTS;

    spawn = NULL;

    while (i < level.num_entities) {
        spawn = &g_entities[i];

        if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch"))) {
            float checkDist;
            vector3 vSub;

            VectorSubtract(&ent->client->ps.origin, &spawn->r.currentOrigin, &vSub);
            checkDist = VectorLength(&vSub);

            if (closestDist == -1 || checkDist < closestDist) {
                closestSpawn = spawn;
                closestDist = checkDist;
            }
        }

        i++;
    }

    return closestSpawn;
}

gentity_t *GetNextSpawnInIndex(gentity_t *currentSpawn) {
    gentity_t *spawn;
    gentity_t *nextSpawn = NULL;
    int i = currentSpawn->s.number + 1;

    spawn = NULL;

    while (i < level.num_entities) {
        spawn = &g_entities[i];

        if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch"))) {
            nextSpawn = spawn;
            break;
        }

        i++;
    }

    if (!nextSpawn) { // loop back around to 0
        i = MAX_CLIENTS;

        while (i < level.num_entities) {
            spawn = &g_entities[i];

            if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch"))) {
                nextSpawn = spawn;
                break;
            }

            i++;
        }
    }

    return nextSpawn;
}

int AcceptBotCommand(char *cmd, gentity_t *pl) {
    int OptionalArgument, i;
    int FlagsFromArgument;
    char *OptionalSArgument, *RequiredSArgument;
    //	vmCvar_t mapname;

    if (!gBotEdit) {
        return 0;
    }

    OptionalArgument = 0;
    i = 0;
    FlagsFromArgument = 0;
    OptionalSArgument = NULL;
    RequiredSArgument = NULL;

    // if a waypoint editing related command is issued, bots will deactivate.
    // once bot_wp_save is issued and the trail is recalculated, bots will activate again.

    if (!pl || !pl->client) {
        return 0;
    }

    if (Q_stricmp(cmd, "bot_wp_cmdlist") == 0) // lists all the bot waypoint commands.
    {
        trap->Print(S_COLOR_YELLOW "bot_wp_add" S_COLOR_WHITE
                                   " - Add a waypoint (optional int parameter will insert the point after the specified waypoint index in a trail)\n\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_rem" S_COLOR_WHITE " - Remove a waypoint (removes last unless waypoint index is specified as a parameter)\n\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_addflagged" S_COLOR_WHITE " - Same as wp_add, but adds a flagged point (type bot_wp_addflagged for help)\n\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_switchflags" S_COLOR_WHITE " - Switches flags on an existing waypoint (type bot_wp_switchflags for help)\n\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_tele" S_COLOR_WHITE " - Teleport yourself to the specified waypoint's location\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_killoneways" S_COLOR_WHITE " - Removes oneway (backward and forward) flags on all waypoints in the level\n\n");
        trap->Print(S_COLOR_YELLOW "bot_wp_save" S_COLOR_WHITE " - Saves all waypoint data into a file for later use\n");

        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_add") == 0) {
        gDeactivated = 1;
        OptionalSArgument = ConcatArgs(1);

        if (OptionalSArgument) {
            OptionalArgument = atoi(OptionalSArgument);
        }

        if (OptionalSArgument && OptionalSArgument[0]) {
            CreateNewWP_InTrail(&pl->client->ps.origin, 0, OptionalArgument);
        } else {
            CreateNewWP(&pl->client->ps.origin, 0);
        }
        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_rem") == 0) {
        gDeactivated = 1;

        OptionalSArgument = ConcatArgs(1);

        if (OptionalSArgument) {
            OptionalArgument = atoi(OptionalSArgument);
        }

        if (OptionalSArgument && OptionalSArgument[0]) {
            RemoveWP_InTrail(OptionalArgument);
        } else {
            RemoveWP();
        }

        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_tele") == 0) {
        gDeactivated = 1;
        OptionalSArgument = ConcatArgs(1);

        if (OptionalSArgument) {
            OptionalArgument = atoi(OptionalSArgument);
        }

        if (OptionalSArgument && OptionalSArgument[0]) {
            TeleportToWP(pl, OptionalArgument);
        } else {
            trap->Print(S_COLOR_YELLOW "You didn't specify an index. Assuming last.\n");
            TeleportToWP(pl, gWPArray.size() - 1);
        }
        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_spawntele") == 0) {
        gentity_t *closestSpawn = GetClosestSpawn(pl);

        if (!closestSpawn) { // There should always be a spawn point..
            return 1;
        }

        closestSpawn = GetNextSpawnInIndex(closestSpawn);

        if (closestSpawn) {
            VectorCopy(&closestSpawn->r.currentOrigin, &pl->client->ps.origin);
        }
        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_addflagged") == 0) {
        gDeactivated = 1;

        RequiredSArgument = ConcatArgs(1);

        if (!RequiredSArgument || !RequiredSArgument[0]) {
            trap->Print(S_COLOR_YELLOW
                        "Flag string needed for bot_wp_addflagged\nj - Jump point\nd - Duck point\nc - Snipe or camp standing\nf - Wait for func\nm - Do not "
                        "move to when func is under\ns - Snipe or camp\nx - Oneway, forward\ny - Oneway, back\ng - Mission goal\nn - No visibility\nExample "
                        "(for a point the bot would jump at, and reverse on when traveling a trail backwards):\nbot_wp_addflagged jx\n");
            return 1;
        }

        while (RequiredSArgument[i]) {
            if (RequiredSArgument[i] == 'j') {
                FlagsFromArgument |= WPFLAG_JUMP;
            } else if (RequiredSArgument[i] == 'd') {
                FlagsFromArgument |= WPFLAG_DUCK;
            } else if (RequiredSArgument[i] == 'c') {
                FlagsFromArgument |= WPFLAG_SNIPEORCAMPSTAND;
            } else if (RequiredSArgument[i] == 'f') {
                FlagsFromArgument |= WPFLAG_WAITFORFUNC;
            } else if (RequiredSArgument[i] == 's') {
                FlagsFromArgument |= WPFLAG_SNIPEORCAMP;
            } else if (RequiredSArgument[i] == 'x') {
                FlagsFromArgument |= WPFLAG_ONEWAY_FWD;
            } else if (RequiredSArgument[i] == 'y') {
                FlagsFromArgument |= WPFLAG_ONEWAY_BACK;
            } else if (RequiredSArgument[i] == 'g') {
                FlagsFromArgument |= WPFLAG_GOALPOINT;
            } else if (RequiredSArgument[i] == 'n') {
                FlagsFromArgument |= WPFLAG_NOVIS;
            } else if (RequiredSArgument[i] == 'm') {
                FlagsFromArgument |= WPFLAG_NOMOVEFUNC;
            }

            i++;
        }

        OptionalSArgument = ConcatArgs(2);

        if (OptionalSArgument) {
            OptionalArgument = atoi(OptionalSArgument);
        }

        if (OptionalSArgument && OptionalSArgument[0]) {
            CreateNewWP_InTrail(&pl->client->ps.origin, FlagsFromArgument, OptionalArgument);
        } else {
            CreateNewWP(&pl->client->ps.origin, FlagsFromArgument);
        }
        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_switchflags") == 0) {
        gDeactivated = 1;

        RequiredSArgument = ConcatArgs(1);

        if (!RequiredSArgument || !RequiredSArgument[0]) {
            trap->Print(S_COLOR_YELLOW "Flag string needed for bot_wp_switchflags\nType bot_wp_addflagged for a list of flags and their corresponding "
                                       "characters, or use 0 for no flags.\nSyntax: bot_wp_switchflags <flags> <n>\n");
            return 1;
        }

        while (RequiredSArgument[i]) {
            if (RequiredSArgument[i] == 'j') {
                FlagsFromArgument |= WPFLAG_JUMP;
            } else if (RequiredSArgument[i] == 'd') {
                FlagsFromArgument |= WPFLAG_DUCK;
            } else if (RequiredSArgument[i] == 'c') {
                FlagsFromArgument |= WPFLAG_SNIPEORCAMPSTAND;
            } else if (RequiredSArgument[i] == 'f') {
                FlagsFromArgument |= WPFLAG_WAITFORFUNC;
            } else if (RequiredSArgument[i] == 's') {
                FlagsFromArgument |= WPFLAG_SNIPEORCAMP;
            } else if (RequiredSArgument[i] == 'x') {
                FlagsFromArgument |= WPFLAG_ONEWAY_FWD;
            } else if (RequiredSArgument[i] == 'y') {
                FlagsFromArgument |= WPFLAG_ONEWAY_BACK;
            } else if (RequiredSArgument[i] == 'g') {
                FlagsFromArgument |= WPFLAG_GOALPOINT;
            } else if (RequiredSArgument[i] == 'n') {
                FlagsFromArgument |= WPFLAG_NOVIS;
            } else if (RequiredSArgument[i] == 'm') {
                FlagsFromArgument |= WPFLAG_NOMOVEFUNC;
            }

            i++;
        }

        OptionalSArgument = ConcatArgs(2);

        if (OptionalSArgument) {
            OptionalArgument = atoi(OptionalSArgument);
        }

        if (OptionalSArgument && OptionalSArgument[0]) {
            WPFlagsModify(OptionalArgument, FlagsFromArgument);
        } else {
            trap->Print(S_COLOR_YELLOW "Waypoint number (to modify) needed for bot_wp_switchflags\nSyntax: bot_wp_switchflags <flags> <n>\n");
        }
        return 1;
    }

    if (Q_stricmp(cmd, "bot_wp_killoneways") == 0) {
        for (int i = 0, size = gWPArray.size(); i < size; i++) {
            if (gWPArray[i]) {
                if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD) {
                    gWPArray[i]->flags &= ~WPFLAG_ONEWAY_FWD;
                }
                if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK) {
                    gWPArray[i]->flags &= ~WPFLAG_ONEWAY_BACK;
                }
            }
        }

        return 1;
    }

    if (!Q_stricmp(cmd, "bot_wp_save")) {
        gDeactivated = 0;
        SavePathData(level.rawmapname);
        return 1;
    }

    return 0;
}
