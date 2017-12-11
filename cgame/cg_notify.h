#pragma once

#include "qcommon/q_shared.h"

/*
Requires:
	libgtk2.0-dev
	libnotify-dev

Icons:
	emblem-important
	process-working
	call-start
	appointment-new
*/

void CG_NotifyInit( void );
bool CG_NotifySend( const char *title, const char *msg, uint32_t timeout = 5000, const char *iconName = "emblem-important" );
void CG_NotifyShutdown( void );
