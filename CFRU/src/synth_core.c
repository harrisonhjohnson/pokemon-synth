#include "defines.h"
#include "../include/new/synth.h"
#include "../include/new/util.h" //GetMonType
#include "../include/constants/species.h"
#include "../include/constants/abilities.h"

/*
synth_core.c
	Pokémon SYNTH — SynthData storage, lookup, and validation.
	Identity: keyed by personality value (architectural rule #0).
*/

// --- Storage backend --------------------------------------------------------
// Persistent: lives in the expanded save block region carved from former PC
// Box 22 (see ram_locs.h). Auto-persisted via flash sectors 30/31, auto-wiped
// by NewGameWipeNewSaveData's 0x2EA4 memset. 240 of 0x6CC carved bytes used.
#define SYNTH_STORAGE() ((struct SynthData*) 0x203D8DC) //keep in sync with gSynthSaveData in ram_locs.h

// --- Species predicates -----------------------------------------------------

bool8 IsSynthSpecies(u16 species)
{
	return species == SPECIES_SYNTH
		|| species == SPECIES_SYNTH2;
	// Later stages append here: SYNTH3, SYNTH3_ULTIMATE, SYNTH_LEGEND
}

bool8 IsSynthMon(struct Pokemon* mon)
{
	return IsSynthSpecies(GetMonData(mon, MON_DATA_SPECIES, NULL));
}

u16 GetSynthBaseBudget(u16 species)
{
	switch (species)
	{
		case SPECIES_SYNTH:   return 300;
		case SPECIES_SYNTH2:  return 400;
		default:              return 0;
	}
}

u8 GetSynthStatCap(u16 species, bool8 postGame)
{
	if (postGame)
		return 255;

	switch (species)
	{
		case SPECIES_SYNTH:   return 80;
		case SPECIES_SYNTH2:  return 100;
		default:              return 255;
	}
}

u16 GetSynthResonanceCap(u16 species, bool8 postGame)
{
	if (postGame)
		return 0xFFFF;

	switch (species)
	{
		case SPECIES_SYNTH:   return 50;
		case SPECIES_SYNTH2:  return 75;
		default:              return 0;
	}
}

// --- Storage ----------------------------------------------------------------

struct SynthData* GetSynthDataForMon(struct Pokemon* mon)
{
	u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
	struct SynthData* store = SYNTH_STORAGE();

	if (personality == 0 || !IsSynthMon(mon))
		return NULL;

	for (u32 i = 0; i < SYNTH_SLOT_COUNT; ++i)
	{
		if (store[i].isSynth && store[i].personality == personality)
			return &store[i];
	}

	return NULL;
}

static void InitSynthDataDefaults(struct SynthData* data, u16 species)
{
	u16 budget = GetSynthBaseBudget(species);
	u16 perStat = budget / 6;               // balanced preset (300 -> 50s)

	data->stage = (species == SPECIES_SYNTH2) ? 1 : 0;
	data->primaryType = TYPE_NORMAL;        // real type chosen via picker/menu
	data->secondaryType = SYNTH_TYPE_NONE;
	data->activeFormId = 0;                 // decision #20
	data->isUltimateForm = FALSE;
	data->postGameUnlocked = FALSE;
	data->statBudget = budget;
	data->resonancePoints = 0;
	data->resonanceCap = GetSynthResonanceCap(species, FALSE);
	data->selectedAbilityId = ABILITY_TRACE; // decision #10
	data->rememberedAbilityId = 0;           // decision #11
	data->statCapPerStat = GetSynthStatCap(species, FALSE);

	for (u32 i = 0; i < 6; ++i)
		data->baseStats[i] = perStat;
	// distribute remainder so the sum equals the budget exactly
	data->baseStats[0] += budget - (u16)(perStat * 6);

	for (u32 i = 0; i < 3; ++i)
		data->unlockedTypeFlags[i] = 0;
}

struct SynthData* AllocSynthDataForMon(struct Pokemon* mon, u8 synthIndex)
{
	struct SynthData* existing = GetSynthDataForMon(mon);
	struct SynthData* store = SYNTH_STORAGE();

	if (existing != NULL)
		return existing;

	if (!IsSynthMon(mon) || synthIndex >= SYNTH_SLOT_COUNT)
		return NULL;

	struct SynthData* data = &store[synthIndex];
	if (data->isSynth) // slot taken by a different mon — should not happen
		return NULL;

	data->personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
	data->isSynth = TRUE;
	data->synthIndex = synthIndex;
	InitSynthDataDefaults(data, GetMonData(mon, MON_DATA_SPECIES, NULL));
	return data;
}

// --- Validation & budgets ---------------------------------------------------

u16 GetSynthTotalBudget(struct SynthData* data)
{
	if (data->postGameUnlocked)
		return 0xFFFF; // uncapped (decision #9)
	return data->statBudget + data->resonancePoints;
}

u16 GetSynthAllocatedTotal(struct SynthData* data)
{
	u16 total = 0;
	for (u32 i = 0; i < 6; ++i)
		total += data->baseStats[i];
	return total;
}

bool8 SynthValidateStats(struct SynthData* data)
{
	bool8 wasValid = TRUE;
	u16 budget = GetSynthTotalBudget(data);

	for (u32 i = 0; i < 6; ++i)
	{
		if (data->baseStats[i] < SYNTH_STAT_MIN)
		{
			data->baseStats[i] = SYNTH_STAT_MIN;
			wasValid = FALSE;
		}
		else if (data->baseStats[i] > data->statCapPerStat)
		{
			data->baseStats[i] = data->statCapPerStat;
			wasValid = FALSE;
		}
	}

	if (!data->postGameUnlocked)
	{
		// claw back overspend from the largest stats until within budget
		while (GetSynthAllocatedTotal(data) > budget)
		{
			u32 maxIdx = 0;
			for (u32 i = 1; i < 6; ++i)
				if (data->baseStats[i] > data->baseStats[maxIdx])
					maxIdx = i;
			if (data->baseStats[maxIdx] <= SYNTH_STAT_MIN)
				break; // cannot reduce further; budget floor reached
			--data->baseStats[maxIdx];
			wasValid = FALSE;
		}
	}

	return wasValid;
}

// --- Engine override entry points -------------------------------------------
// Wired into CalculateMonStatsNew (build_pokemon.c) 2026-08-12.

// SynthData->baseStats is authored in MENU DISPLAY order [HP,Atk,Def,SpA,SpD,Spe]
// (see struct comment in synth.h and sStatNames in synth_menu.c). The engine
// indexes stats in CANONICAL order [HP,Atk,Def,Spe,SpA,SpD] — matching both the
// STAT_* enum and struct BaseStats' field layout. They agree on 0-2 and disagree
// on 3-5, so callers pass a STAT_* index and this table maps it to the slot.
// DO NOT reorder SynthData->baseStats to "fix" this — the array is persisted in
// the save block, so reordering silently corrupts every existing Synth mon.
static const u8 sStatIndexToSynthSlot[6] = {0, 1, 2, 5, 3, 4};

u16 SynthGetBaseStat(struct Pokemon* mon, u8 statIndex, u16 vanillaValue)
{
	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL || statIndex >= 6)
		return vanillaValue;
	return data->baseStats[sStatIndexToSynthSlot[statIndex]];
}

// --- Form (archetype) presets -----------------------------------------------
// Tables verbatim from specs/synth_system.md. Menu stat order [HP,Atk,Def,SpA,SpD,Spe].

extern const u8 gText_SynthFormPhysAttacker[];
extern const u8 gText_SynthFormSpecAttacker[];
extern const u8 gText_SynthFormBalanced[];
extern const u8 gText_SynthFormPhysSweeper[];
extern const u8 gText_SynthFormSpecSweeper[];
extern const u8 gText_SynthFormDualAttacker[];
extern const u8 gText_SynthFormDefensiveWall[];
extern const u8 gText_SynthFormBulkySweeper[];
extern const u8 gText_SynthFormSpeedySupport[];

static const struct SynthFormPreset sStage1Forms[3] =
{
	{gText_SynthFormPhysAttacker, {50, 80, 40, 20, 40, 70}},
	{gText_SynthFormSpecAttacker, {50, 20, 40, 80, 40, 70}},
	{gText_SynthFormBalanced,     {50, 50, 50, 50, 50, 50}},
};

static const struct SynthFormPreset sStage2Forms[6] =
{
	{gText_SynthFormPhysSweeper,   {60, 100,  55,  30,  55, 100}},
	{gText_SynthFormSpecSweeper,   {60,  30,  55, 100,  55, 100}},
	{gText_SynthFormDualAttacker,  {65,  80,  60,  80,  60,  55}},
	{gText_SynthFormDefensiveWall, {95,  45, 100,  45, 100,  15}},
	{gText_SynthFormBulkySweeper,  {85, 100,  55,  45,  55,  60}},
	{gText_SynthFormSpeedySupport, {80,  35,  80,  35,  80,  90}},
};

u8 GetSynthFormCount(struct SynthData* data)
{
	return (data->stage == 0) ? 3 : 6;
}

const struct SynthFormPreset* GetSynthFormPreset(struct SynthData* data, u8 formId)
{
	if (data->stage == 0)
		return &sStage1Forms[formId < 3 ? formId : 0];
	return &sStage2Forms[formId < 6 ? formId : 0];
}

static u16 SynthFormDistance(const struct SynthFormPreset* form, struct SynthData* data)
{
	u16 total = 0;
	for (u32 i = 0; i < 6; ++i)
	{
		s16 diff = (s16) data->baseStats[i] - (s16) form->stats[i];
		total += (diff < 0) ? -diff : diff;
	}
	return total;
}

u8 SynthGetClosestForm(struct SynthData* data)
{
	u8 count = GetSynthFormCount(data);
	u16 dist[6];
	u16 best = 0xFFFF;

	for (u32 i = 0; i < count; ++i)
	{
		dist[i] = SynthFormDistance(GetSynthFormPreset(data, i), data);
		if (dist[i] < best)
			best = dist[i];
	}

	// Tiebreak 2: current form sticky
	if (data->activeFormId < count && dist[data->activeFormId] == best)
		return data->activeFormId;

	// Tiebreak 3: closest Spe (menu slot 5), then 4: lower index (loop order)
	u8 winner = 0xFF;
	u16 bestSpeDiff = 0xFFFF;
	for (u32 i = 0; i < count; ++i)
	{
		if (dist[i] != best)
			continue;
		const struct SynthFormPreset* form = GetSynthFormPreset(data, i);
		s16 speDiff = (s16) data->baseStats[5] - (s16) form->stats[5];
		u16 absSpe = (speDiff < 0) ? -speDiff : speDiff;
		if (absSpe < bestSpeDiff)
		{
			bestSpeDiff = absSpe;
			winner = i;
		}
	}
	return winner;
}

void SynthApplyFormPreset(struct SynthData* data, u8 formId)
{
	const struct SynthFormPreset* form = GetSynthFormPreset(data, formId);

	for (u32 i = 0; i < 6; ++i)
		data->baseStats[i] = form->stats[i];

	data->activeFormId = (formId < GetSynthFormCount(data)) ? formId : 0;
	// Presets are authored within stage budget/caps, but RP-grown budgets and
	// postgame caps make this cheap insurance rather than dead code.
	SynthValidateStats(data);
}

// Vanilla summary screen: BufferMonInfo copies gSpeciesInfo types straight
// into sMonSummaryScreen->monTypes, so a Synth's chosen types never show.
// Called from SynthSummaryTypeIconsHook (entry of PokeSum_PrintMonTypeIcons)
// to re-stamp them through GetMonType. Offsets are vanilla BPRE layout
// (pokefirered src/pokemon_summary_screen.c): ptr @ 0x203B140,
// monTypes @ +0x3220, currentMon @ +0x3290.
void SynthFixSummaryScreenTypes(void)
{
	u8* screen = *(u8**) 0x203B140;
	if (screen == NULL)
		return;

	struct Pokemon* mon = (struct Pokemon*) (screen + 0x3290);
	if (!IsSynthMon(mon))
		return;

	screen[0x3220] = GetMonType(mon, 0);
	screen[0x3221] = GetMonType(mon, 1);
}

u8 SynthGetAbility(struct Pokemon* mon)
{
	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL || data->selectedAbilityId == ABILITY_NONE)
		return ABILITY_NONE;
	return (u8) data->selectedAbilityId;
}

bool8 SynthIsTypeUnlocked(struct SynthData* data, u8 type)
{
	if (type >= 24)
		return FALSE;

	// Interim: no gym unlock script exists yet, so an all-zero flag set means
	// "everything unlocked" rather than "nothing unlocked". Remove this
	// fallback when type_unlock_*.pks scripts start setting bits.
	if (data->unlockedTypeFlags[0] == 0
	 && data->unlockedTypeFlags[1] == 0
	 && data->unlockedTypeFlags[2] == 0)
		return TRUE;

	return (data->unlockedTypeFlags[type / 8] >> (type % 8)) & 1;
}

u8 SynthGetType(struct Pokemon* mon, u8 typeSlot, u8 vanillaType)
{
	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL)
		return vanillaType;
	if (typeSlot == 0)
		return data->primaryType;
	return (data->secondaryType == SYNTH_TYPE_NONE) ? data->primaryType
	                                                : data->secondaryType;
}
