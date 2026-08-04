#include "defines.h"
#include "../include/new/synth.h"
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

// --- Engine override entry points (call sites wired in a later session) -----

u16 SynthGetBaseStat(struct Pokemon* mon, u8 statIndex, u16 vanillaValue)
{
	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL || statIndex >= 6)
		return vanillaValue;
	return data->baseStats[statIndex];
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
