#pragma once

void G_StoreTrail( gentity_t *ent );
void G_ResetTrail( gentity_t *ent );
void G_TimeShiftClient( gentity_t *ent, int time );
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftClient( gentity_t *ent );
void G_UnTimeShiftAllClients( gentity_t *skip );
