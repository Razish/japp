#pragma once

#define CSF_GRAPPLE_SWING		(0x00000001u) // Can correctly predict movement when using the grapple hook
#define CSF_SCOREBOARD_LARGE	(0x00000002u) // Can correctly parse scoreboard messages with information for 32 clients
#define CSF_SCOREBOARD_KD		(0x00000004u) // Can correctly parse scoreboard messages with extra K/D information
#define CSF_CHAT_FILTERS		(0x00000008u) // Can correctly parse chat messages with proper delimiters
#define CSF_FIXED_WEAPON_ANIMS	(0x00000010u) // Fixes the missing concussion rifle animations
#define CSF_WEAPONDUEL			(0x00000020u)
#define CSF_NUM					(6)
#define CSF_UNUSED7				(0x00000040u)
#define CSF_UNUSED8				(0x00000080u)
#define CSF_UNUSED9				(0x00000100u)
#define CSF_UNUSED10			(0x00000200u)
#define CSF_UNUSED11			(0x00000400u)
#define CSF_UNUSED12			(0x00000800u)
#define CSF_UNUSED13			(0x00001000u)
#define CSF_UNUSED14			(0x00002000u)
#define CSF_UNUSED15			(0x00004000u)
#define CSF_UNUSED16			(0x00008000u)
#define CSF_UNUSED17			(0x00010000u)
#define CSF_UNUSED18			(0x00020000u)
#define CSF_UNUSED19			(0x00040000u)
#define CSF_UNUSED20			(0x00080000u)
#define CSF_UNUSED21			(0x00100000u)
#define CSF_UNUSED22			(0x00200000u)
#define CSF_UNUSED23			(0x00400000u)
#define CSF_UNUSED24			(0x00800000u)
#define CSF_UNUSED25			(0x01000000u)
#define CSF_UNUSED26			(0x02000000u)
#define CSF_UNUSED27			(0x04000000u)
#define CSF_UNUSED28			(0x08000000u)
#define CSF_UNUSED29			(0x10000000u)
#define CSF_UNUSED30			(0x20000000u)
#define CSF_UNUSED31			(0x40000000u)
#define CSF_UNUSED32			(0x80000000u)

#define JAPP_CLIENT_FLAGS		(CSF_GRAPPLE_SWING|CSF_SCOREBOARD_LARGE|CSF_SCOREBOARD_KD|CSF_CHAT_FILTERS|CSF_FIXED_WEAPON_ANIMS)
#define JAPLUS_CLIENT_FLAGS 	(CSF_GRAPPLE_SWING|CSF_SCOREBOARD_KD)

#define CPD_NEWDRAINEFX			(0x00000001u) //RAZTODO
#define CPD_DUELSEEOTHERS		(0x00000002u)
#define CPD_ENDDUELROTATION		(0x00000004u) //RAZTODO
#define CPD_BLACKSABERSDISABLE	(0x00000008u)
#define CPD_AUTOREPLYDISABLE	(0x00000010u) //RAZTODO? fuck dat lawl
#define CPD_NEWFORCEEFFECT		(0x00000020u) //RAZTODO
#define CPD_NEWDEATHMSG_DISABLE	(0x00000040u) //RAZTODO
#define CPD_NEWSIGHTEFFECT		(0x00000080u) //RAZTODO
#define CPD_NOALTDIMEFFECT		(0x00000100u) //RAZTODO
#define CPD_HOLSTEREDSABER		(0x00000200u) //RAZTODO
#define CPD_LEDGEGRAB			(0x00000400u)
#define CPD_NEWDFAPRIM			(0x00000800u) //RAZTODO
#define CPD_NEWDFAALT			(0x00001000u) //RAZTODO
#define CPD_NOSPCARTWHEEL		(0x00002000u) // don't allow SP cartwheel
#define CPD_ALLOWLIBCURL		(0x00004000u) //RAZTODO
#define CPD_NOKATA				(0x00008000u) // don't allow katas
#define CPD_NOBUTTERFLY			(0x00010000u) // don't allow butterflies
#define CPD_NOSTAB				(0x00020000u) // don't allow backstab/rollstab
#define CPD_NODFA				(0x00040000u) // don't allow DFAs
#define CPD_OLDGRAPPLE			(0X00080000u) // automatically release grapple
#define CPD_ANNOYINGEMOTES		(0x00100000u) // disable annoying emotes
#define CPD_NOSPINKICKS			(0x00200000u) // don't allow diagonal spin kicks
#define CPD_NUM					(22)
