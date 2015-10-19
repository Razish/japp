//
// NPC_misc.cpp
//
#include "b_local.h"
#include "qcommon/q_shared.h"

void Debug_Printf( const xcvar *cv, int debugLevel, const char *fmt, ... ) {
	const char *color;
	va_list argptr;
	char msg[1024];

	if ( cv->getFloat() < debugLevel )
		return;

	if ( debugLevel == DEBUG_LEVEL_DETAIL )
		color = S_COLOR_WHITE;
	else if ( debugLevel == DEBUG_LEVEL_INFO )
		color = S_COLOR_GREEN;
	else if ( debugLevel == DEBUG_LEVEL_WARNING )
		color = S_COLOR_YELLOW;
	else //if (debugLevel == DEBUG_LEVEL_ERROR)
		color = S_COLOR_RED;

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );

	Com_Printf( "%s%5i:%s", color, level.time, msg );
}

void Debug_NPCPrintf( gentity_t *printNPC, const xcvar *cv, int debugLevel, const char *fmt, ... ) {
	const char *color;
	va_list argptr;
	char msg[1024];

	if ( cv->getFloat() < debugLevel ) {
		return;
	}

	if ( debugLevel == DEBUG_LEVEL_DETAIL )
		color = S_COLOR_WHITE;
	else if ( debugLevel == DEBUG_LEVEL_INFO )
		color = S_COLOR_GREEN;
	else if ( debugLevel == DEBUG_LEVEL_WARNING )
		color = S_COLOR_YELLOW;
	else //if (debugLevel == DEBUG_LEVEL_ERROR)
		color = S_COLOR_RED;

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof(msg), fmt, argptr );
	va_end( argptr );

	Com_Printf( "%s%5i (%s) %s", color, level.time, printNPC->targetname, msg );
}
