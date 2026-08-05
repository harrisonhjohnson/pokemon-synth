#pragma once

#include "../global.h"

/**
 * \file synth.h
 * \brief Pokémon SYNTH — core data structures and API for the customizable
 *        Synth species. See specs/synth_system.md in the design bible.
 *
 * Identity rule (architectural rule #0): SynthData is keyed to the mon's
 * personality value, NEVER to a party or box slot. Slots reorder; saves
 * corrupt. Personality is stable for the life of the mon.
 */

#define SYNTH_SLOT_COUNT 6      // 6 gift Synths total (spec decision #1)
#define SYNTH_TYPE_NONE 0xFF    // single-typed sentinel (spec: TYPE_NONE)
#define SYNTH_STAT_MIN 20       // per-stat minimum, all stages (decision #5)
#define SYNTH_RP_RATIO 4        // 4 EVs -> 1 Resonance Point (decision #6)

// Archetypes — displayed as "Form" in UI (decision #17)
#define ARCHETYPE_PHYSICAL_SWEEPER 0
#define ARCHETYPE_SPECIAL_SWEEPER  1
#define ARCHETYPE_DUAL_ATTACKER    2
#define ARCHETYPE_DEFENSIVE_WALL   3
#define ARCHETYPE_BULKY_SWEEPER    4
#define ARCHETYPE_SUPPORT          5

struct SynthData
{
    /*0x00*/ u32 personality;        // identity key; 0 = slot free
    /*0x04*/ u8 isSynth;             // 1 if slot in use
    /*0x05*/ u8 synthIndex;          // which gift (0-5); 0 = starter
    /*0x06*/ u8 stage;               // 0=Synth1, 1=Synth2, 2=Synth3
    /*0x07*/ u8 primaryType;         // runtime type override
    /*0x08*/ u8 secondaryType;       // SYNTH_TYPE_NONE = single-typed
    /*0x09*/ u8 activeFormId;        // archetype preset index (decision #20)
    /*0x0A*/ u8 isUltimateForm;      // starter only (decision #23)
    /*0x0B*/ u8 postGameUnlocked;    // champion flag lifted caps
    /*0x0C*/ u16 baseStats[6];       // [HP, Atk, Def, SpA, SpD, Spe]
    /*0x18*/ u16 statBudget;         // base BST for current species
    /*0x1A*/ u16 resonancePoints;    // earned bonus BST (4 EV : 1 RP)
    /*0x1C*/ u16 resonanceCap;       // per-stage RP cap (50/75/100)
    /*0x1E*/ u16 selectedAbilityId;  // defaults to ABILITY_TRACE
    /*0x20*/ u16 rememberedAbilityId;// 0 = never assigned (decision #11)
    /*0x22*/ u8 unlockedTypeFlags[3];// 18 bits used; bit = type id
    /*0x25*/ u8 statCapPerStat;      // 80/100/120/150/255 by stage
    /*0x26*/ u8 padding[2];
}; // 0x28 = 40 bytes; 6 slots = 240 bytes in save expansion

// --- Species predicates -----------------------------------------------------

bool8 IsSynthSpecies(u16 species);
bool8 IsSynthMon(struct Pokemon* mon);

// Base BST budget by species — never derive from stage (spec rule #2):
// SYNTH=300, SYNTH2=400 (later: SYNTH3=500, ULTIMATE=680, LEGEND=600)
u16 GetSynthBaseBudget(u16 species);
u8 GetSynthStatCap(u16 species, bool8 postGame);
u16 GetSynthResonanceCap(u16 species, bool8 postGame);

// --- Storage ----------------------------------------------------------------

// Lookup by personality; returns NULL if the mon has no SynthData.
struct SynthData* GetSynthDataForMon(struct Pokemon* mon);

// Allocate (or return existing) SynthData for a Synth mon. Initializes to the
// species' stage defaults: balanced preset stats, Trace, no secondary type.
struct SynthData* AllocSynthDataForMon(struct Pokemon* mon, u8 synthIndex);

// --- Validation & stat plumbing --------------------------------------------

// Enforces: sum(baseStats) <= statBudget + resonancePoints;
// SYNTH_STAT_MIN <= each stat <= statCapPerStat. Clamps in place, returns
// TRUE if input was already valid.
bool8 SynthValidateStats(struct SynthData* data);

// Total spendable budget right now (base + earned RP).
u16 GetSynthTotalBudget(struct SynthData* data);
// Currently allocated total across the six stats.
u16 GetSynthAllocatedTotal(struct SynthData* data);

// Battle/stat override entry points (wired to engine read sites in a later
// session — declared now so call sites can land incrementally):
u16 SynthGetBaseStat(struct Pokemon* mon, u8 statIndex, u16 vanillaValue);
u8 SynthGetType(struct Pokemon* mon, u8 typeSlot, u8 vanillaType);

// --- Menu runtime state (fixed EWRAM — CFRU's linker puts .bss in ROM!) -----
// Lives in the carved Box-22 region after the 6 SynthData slots.
// 0x203D8DC + 240 = 0x203D9CC. See ram_locs.h.
struct SynthMenuState
{
    struct Pokemon* mon;
    struct SynthData* data;
    u8* tilemapPtr; //bg3 tilemap buffer, alive only during CB2_SynthMenu init
    u8 selectedStat;
    u8 windowsOk;
    u8 padding[2];
};
#define gSynthMenuState ((struct SynthMenuState*) 0x203D9CC)

// --- Synth Menu (src/synth_menu.c) ------------------------------------------

// Stage the menu for a mon (allocates SynthData if needed). Returns FALSE if
// the mon can't have a menu. Call before handing CB2_SynthMenu to the party
// menu's exitCallback.
bool8 SynthMenuPrepare(struct Pokemon* mon);
void CB2_SynthMenu(void);
