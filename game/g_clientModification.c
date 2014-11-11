#include "g_local.h"
#include "JAPP/jp_csflags.h"

qboolean Client_Supports( const gentity_t *ent, uint32_t supportFlag ) {
	return !!(ent->client->pers.CSF & supportFlag);
}

const char *supportFlagNames[CSF_NUM] = {
	"CSF_GRAPPLE_SWING",		// Can correctly predict movement when using the grapple hook
	"CSF_SCOREBOARD_LARGE",		// Can correctly parse scoreboard messages with information for 32 clients
	"CSF_SCOREBOARD_KD",		// Can correctly parse scoreboard messages with extra K/D information
	"CSF_CHAT_FILTERS",			// Can correctly parse chat messages with proper delimiters
	"CSF_FIXED_WEAPON_ANIMS",	// Fixes the missing concussion rifle animations
};

const char *clientPluginDisableNames[CPD_NUM] = {
	"CPD_NEWDRAINEFX",
	"CPD_DUELSEEOTHERS",
	"CPD_ENDDUELROTATION",
	"CPD_BLACKSABERSDISABLE",
	"CPD_AUTOREPLYDISABLE",
	"CPD_NEWFORCEEFFECT",
	"CPD_NEWDEATHMSG_DISABLE",
	"CPD_NEWSIGHTEFFECT",
	"CPD_NOALTDIMEFFECT",
	"CPD_HOLSTEREDSABER",
	"CPD_LEDGEGRAB",
	"CPD_NEWDFAPRIM",
	"CPD_NEWDFAALT",
	"CPD_NOSPCARTWHEEL",		// don't allow SP cartwheel
	"CPD_ALLOWLIBCURL",
	"CPD_NOKATA",				// don't allow katas
	"CPD_NOBUTTERFLY",			// don't allow butterflies
	"CPD_NOSTAB",				// don't allow backstab/rollstab
	"CPD_NODFA",				// don't allow DFAs
	"CPD_OLDGRAPPLE",			// automatically release grapple
};
