#include "g_local.h"

qboolean Client_Supports( gentity_t *ent, unsigned int supportFlag )
{
	return !!(ent->client->pers.CSF & supportFlag );
}

const char *supportFlagNames[CSF_NUM] = {
	"CSF_GRAPPLE_SWING",		// Can correctly predict movement when using the grapple hook
	"CSF_SCOREBOARD_LARGE",		// Can correctly parse scoreboard messages with information for 32 clients
	"CSF_SCOREBOARD_KD",		// Can correctly parse scoreboard messages with extra K/D information
	"CSF_CHAT_FILTERS",			// Can correctly parse chat messages with proper delimiters
	"CSF_FIXED_WEAPON_ANIMS",	// Fixes the missing concussion rifle animations
};
