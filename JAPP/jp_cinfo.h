#pragma once

#define CINFO_FLIPKICK (0x00000001u)      // allow player flip-kicking
#define CINFO_JK2ROLL1 (0x00000002u)      // Grip whilst rolling
#define CINFO_JK2ROLL2 (0x00000004u)      // Grip whilst rolling + chainable rolls
#define CINFO_JK2ROLL3 (0x00000008u)      // Long roll + breakable
#define CINFO_YELLOWDFA (0x00000010u)     // 'improved' yellow DFA
#define CINFO_HEADSLIDE (0x00000020u)     // jp_slideOnPlayer/japp_slideOnPlayer set
#define CINFO_NOSPCARTWHEEL (0x00000040u) // don't allow sp cartwheel
#define CINFO_NEWDFAPRIM (0x00000080u)    // TODO: new primary DFA
#define CINFO_SPINKICKS (0x00000100u)     // allow diagonal spin kicks
#define CINFO_UNKNOWN03 (0x00000200u)
#define CINFO_MACROSCAN1 (0x00000400u) // IGNORE: macro-scan 1
#define CINFO_MACROSCAN2 (0x00000800u) // IGNORE: macro-scan 2
#define CINFO_JK2DFA (0x00001000u)     // TODO: JK2 DFA ???
#define CINFO_NOKATA (0x00002000u)     // no kata
#define CINFO_UNKNOWN02 (0x00004000u)
#define CINFO_NEWDFAALT (0x00008000u)        // new alternate DFA
#define CINFO_LEDGEGRAB (0x00010000u)        // TODO: allow ledge-grab
#define CINFO_ALTDIM (0x00020000u)           // TODO: any jp_altDim is set
#define CINFO_ALWAYSPICKUPWEAP (0x00040000u) // always allow picking up weapons, will just add ammo
#define CINFO_CPMPHYSICS (0x00080000u)       // CPM style player physics
#define CINFO_WEAPONROLL (0x00100000u)       // allow rolling with weapons
#define CINFO_PRIVDUELWEAP (0x00200000u)     // allow other weapons in duels
#define CINFO_NOBUSYATK (0x00400000u)        // /--don't allow saber attacks when "busy"
#define CINFO_NOBUTTERFLY (0x00800000u)      // |--don't allow butterflies
#define CINFO_NOSTAB (0x01000000u)           // |--don't allow backstab/rollstab
#define CINFO_NODFA (0x02000000u)            // |--don't allow DFAs
#define CINFO_TOGGLESPECIALATK (0x04000000u) // ^ allow toggling of these
#define CINFO_VQ3PHYS (0x08000000u)
#define CINFO_NOSTRAFEJUMP (0x10000000u)
#define CINFO_UNUSED03 (0x20000000u)
#define CINFO_UNUSED02 (0x40000000u)
#define CINFO_UNUSED01 (0x80000000u)
