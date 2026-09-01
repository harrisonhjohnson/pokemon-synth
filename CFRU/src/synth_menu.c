#include "defines.h"
#include "../include/bg.h"
#include "../include/gpu_regs.h"
#include "../include/main.h"
#include "../include/new/Vanilla_functions.h"
#include "../include/menu.h"
#include "../include/new_menu_helpers.h"
#include "../include/overworld.h"
#include "../include/palette.h"
#include "../include/party_menu.h"
#include "../include/scanline_effect.h"
#include "../include/sound.h"
#include "../include/string_util.h"
#include "../include/task.h"
#include "../include/text.h"
#include "../include/text_window.h"
#include "../include/window.h"
#include "../include/constants/songs.h"

#include "../include/new/build_pokemon.h" //CalculateMonStatsNew + GetMonAbility decls
#include "../include/new/dns.h" //TransferPlttBuffer decl
#include "../include/new/ability_util.h" //GetAbilityName
#include "../include/new/synth.h"
#include "../include/constants/species.h"
#include "../include/constants/abilities.h"

/*
synth_menu.c
	Pokémon SYNTH — the Synth Menu.
	Sectioned single screen per specs/synth_menu.md: Banner / Type / Stats /
	Ability. Form bar deferred (no preset tables yet); Ability bar is
	display-only until the ability override engine is wired.
	Skeleton modeled on src/dexnav.c's GUI.
*/

extern const u8 gText_SynthTitle[];
extern const u8 gText_SynthBudget[];
extern const u8 gText_SynthHP[];
extern const u8 gText_SynthAtk[];
extern const u8 gText_SynthDef[];
extern const u8 gText_SynthSpAtk[];
extern const u8 gText_SynthSpDef[];
extern const u8 gText_SynthSpeed[];
extern const u8 gText_SynthHelpNav[];
extern const u8 gText_SynthHelpStats[];
extern const u8 gText_SynthHelpType[];
extern const u8 gText_SynthHelpForm[];
extern const u8 gText_SynthFormLabel[];
extern const u8 gText_SynthType1[];
extern const u8 gText_SynthType2[];
extern const u8 gText_SynthNone[];
extern const u8 gText_SynthAbilityLabel[];
extern const u8 gText_SynthAbilityFrom[];
extern const u8 gText_SynthParenClose[];
extern const u8 gText_SynthHelpAbility[];
extern const u8 gText_SynthLv[];

extern const u8 gTypeNames[][7];
extern const u8 gAbilityNames[][13];

//string_util.h's copy of this decl is commented out; live one is in a battle header
u8* __attribute__((long_call)) StringGetEnd10(u8* str);

enum
{
	WIN_SYNTH_TITLE,
	WIN_SYNTH_FORM,
	WIN_SYNTH_TYPES,
	WIN_SYNTH_STATS,
	WIN_SYNTH_ABILITY,
	WIN_SYNTH_HELP,
	SYNTH_WINDOW_COUNT,
};

// Cursor sections, in vertical order. Stats sits to the RIGHT of Type
// (d-pad right from Type, left from Stats), everything else stacks.
enum
{
	SEC_BANNER,
	SEC_FORM,
	SEC_TYPE,
	SEC_STATS,
	SEC_ABILITY,
};

enum SynthBgs { SBG_TEXTBOX, SBG_TEXT_2, SBG_TEXT, SBG_BACKGROUND }; //DexNav chassis

//MUST be 4-aligned: vanilla InitWindows copies templates with word loads, and
//ARM7TDMI rotates unaligned LDRs — a 2-aligned array scrambles every field
//(root cause of the 2026-07/08 renderer bug; struct's natural align is only 2).
//baseBlocks: each bg's windows must total < 512 tiles (per-bg charblock;
//overflow reads the next charblock and renders blank). bg2 (SBG_TEXT) carries
//title+types+stats = 356 tiles; form+ability+help live on bg1 (SBG_TEXT_2)'s
//own charblock = 172 tiles. InitWindows allocs bg1's tilemap buffer itself.
static const struct WindowTemplate sSynthWinTemplates[SYNTH_WINDOW_COUNT + 1] __attribute__((aligned(4))) =
{
	[WIN_SYNTH_TITLE] =
	{
		.bg = SBG_TEXT,
		.tilemapLeft = 1,
		.tilemapTop = 0,
		.width = 28,
		.height = 2,
		.paletteNum = 15,
		.baseBlock = 1,
	},
	[WIN_SYNTH_FORM] =
	{
		.bg = SBG_TEXT_2,
		.tilemapLeft = 1,
		.tilemapTop = 2,
		.width = 28,
		.height = 2,
		.paletteNum = 15,
		.baseBlock = 1,
	},
	[WIN_SYNTH_TYPES] =
	{
		.bg = SBG_TEXT,
		.tilemapLeft = 1,
		.tilemapTop = 4,
		.width = 8,
		.height = 9,
		.paletteNum = 15,
		.baseBlock = 57,
	},
	[WIN_SYNTH_STATS] =
	{
		.bg = SBG_TEXT,
		.tilemapLeft = 10,
		.tilemapTop = 4,
		.width = 19,
		.height = 12,
		.paletteNum = 15,
		.baseBlock = 129,
	},
	[WIN_SYNTH_ABILITY] =
	{
		.bg = SBG_TEXT_2,
		.tilemapLeft = 1,
		.tilemapTop = 16,
		.width = 28,
		.height = 2,
		.paletteNum = 15,
		.baseBlock = 57,
	},
	[WIN_SYNTH_HELP] =
	{
		.bg = SBG_TEXT_2,
		.tilemapLeft = 0,
		.tilemapTop = 18,
		.width = 30,
		.height = 2,
		.paletteNum = 15,
		.baseBlock = 113,
	},
	DUMMY_WIN_TEMPLATE,
};

static const struct BgTemplate sSynthBgTemplates[] = //verbatim DexNav layout
{
	[SBG_TEXTBOX] =    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .screenSize = 0, .paletteMode = 0, .priority = 0, .baseTile = 0},
	[SBG_TEXT_2] =     {.bg = 1, .charBaseIndex = 1, .mapBaseIndex = 30, .screenSize = 0, .paletteMode = 0, .priority = 1, .baseTile = 0},
	[SBG_TEXT] =       {.bg = 2, .charBaseIndex = 2, .mapBaseIndex = 29, .screenSize = 0, .paletteMode = 0, .priority = 2, .baseTile = 0},
	[SBG_BACKGROUND] = {.bg = 3, .charBaseIndex = 3, .mapBaseIndex = 28, .screenSize = 0, .paletteMode = 0, .priority = 3, .baseTile = 0},
};

static const struct TextColor sSynthText      = {1, 2, 3}; // dark on white
static const struct TextColor sSynthSelected  = {2, 1, 3}; // inverted row
static const u16 sSynthBackdrop[] = {RGB(3, 11, 11)};      // dark teal

static const u16 sDiagRed[] = {RGB(28, 4, 4)}; //diagnostic backdrop: InitWindows failed
#define sWindowsOk (gSynthMenuState->windowsOk)

static const u8* const sSynthStatNames[6] =
{
	gText_SynthHP, gText_SynthAtk, gText_SynthDef,
	gText_SynthSpAtk, gText_SynthSpDef, gText_SynthSpeed,
};

// Type picker display order (all 18 real types; ids are NON-contiguous —
// MYSTERY 0x09 and the 0x12-0x16 gap sit between DARK and FAIRY).
static const u8 sSynthTypeOrder[18] =
{
	TYPE_NORMAL, TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_ICE,
	TYPE_FIGHTING, TYPE_POISON, TYPE_GROUND, TYPE_FLYING, TYPE_PSYCHIC,
	TYPE_BUG, TYPE_ROCK, TYPE_GHOST, TYPE_DRAGON, TYPE_DARK, TYPE_STEEL,
	TYPE_FAIRY,
};

// Menu state lives at a FIXED EWRAM address (gSynthMenuState in synth.h):
// CFRU's linker places .bss/COMMON in ROM, so mutable statics are silently
// dead. Never add a mutable static to this file.
#define sSynthMenuMon   (gSynthMenuState->mon)
#define sSynthMenuData  (gSynthMenuState->data)
#define sSelectedStat   (gSynthMenuState->selectedStat)
#define sSection        (gSynthMenuState->section)
#define sInSubMenu      (gSynthMenuState->inSubMenu)
#define sTypeSlot       (gSynthMenuState->typeSlot)
//(formCursor field currently unused — form scroll applies directly)

// --- Drawing ----------------------------------------------------------------

static void CommitSynthWindow(u8 windowId)
{
	PutWindowTilemap(windowId);
	CopyWindowToVram(windowId, COPYWIN_BOTH);
}

static void DrawSynthBanner(void)
{
	struct Pokemon* mon = sSynthMenuMon;
	const struct TextColor* color = (sSection == SEC_BANNER) ? &sSynthSelected : &sSynthText;

	FillWindowPixelBuffer(WIN_SYNTH_TITLE, PIXEL_FILL(1));

	GetMonData(mon, MON_DATA_NICKNAME, gStringVar1);
	StringGetEnd10(gStringVar1);
	WindowPrint(WIN_SYNTH_TITLE, 1, 4, 0, color, 0, gStringVar1);

	StringCopy(gStringVar4, gText_SynthLv);
	ConvertIntToDecimalStringN(gStringVar2, GetMonData(mon, MON_DATA_LEVEL, NULL), STR_CONV_MODE_LEFT_ALIGN, 3);
	StringAppend(gStringVar4, gStringVar2);
	WindowPrint(WIN_SYNTH_TITLE, 1, 96, 0, &sSynthText, 0, gStringVar4);

	WindowPrint(WIN_SYNTH_TITLE, 1, 152, 0, &sSynthText, 0, gText_SynthTitle);
	CommitSynthWindow(WIN_SYNTH_TITLE);
}

// Form bar: shows the active form's name. Inside the sub-menu the name is
// highlighted; scrolling applies presets directly (no preview state).
static void DrawSynthForm(void)
{
	struct SynthData* data = sSynthMenuData;
	bool8 editing = (sInSubMenu && sSection == SEC_FORM);
	const struct TextColor* color = (sSection == SEC_FORM) ? &sSynthSelected : &sSynthText;

	FillWindowPixelBuffer(WIN_SYNTH_FORM, PIXEL_FILL(1));
	WindowPrint(WIN_SYNTH_FORM, 0, 4, 4, color, 0, gText_SynthFormLabel);
	WindowPrint(WIN_SYNTH_FORM, 1, 64, 2, editing ? &sSynthSelected : &sSynthText, 0,
	            GetSynthFormPreset(data, data->activeFormId)->name);
	CommitSynthWindow(WIN_SYNTH_FORM);
}

static void DrawSynthTypes(void)
{
	struct SynthData* data = sSynthMenuData;

	FillWindowPixelBuffer(WIN_SYNTH_TYPES, PIXEL_FILL(1));

	for (u32 slot = 0; slot < 2; ++slot)
	{
		// Section focus highlights both labels; sub-menu highlights only the
		// slot being edited.
		const struct TextColor* color = &sSynthText;
		if (sSection == SEC_TYPE && (sInSubMenu ? (sTypeSlot == slot) : TRUE))
			color = &sSynthSelected;

		u8 y = 2 + slot * 34;
		WindowPrint(WIN_SYNTH_TYPES, 0, 4, y, color, 0, slot == 0 ? gText_SynthType1 : gText_SynthType2);

		u8 type = (slot == 0) ? data->primaryType : data->secondaryType;
		const u8* name = (type == SYNTH_TYPE_NONE) ? gText_SynthNone : gTypeNames[type];
		WindowPrint(WIN_SYNTH_TYPES, 1, 8, y + 13, &sSynthText, 0, name);
	}

	CommitSynthWindow(WIN_SYNTH_TYPES);
}

static void DrawSynthStats(void)
{
	struct SynthData* data = sSynthMenuData;

	FillWindowPixelBuffer(WIN_SYNTH_STATS, PIXEL_FILL(1));

	// Budget line: "Budget  266/300" — doubles as the section focus marker.
	const struct TextColor* budgetColor =
		(sSection == SEC_STATS && !sInSubMenu) ? &sSynthSelected : &sSynthText;
	u16 used = GetSynthAllocatedTotal(data);
	u16 total = GetSynthTotalBudget(data);
	WindowPrint(WIN_SYNTH_STATS, 1, 4, 0, budgetColor, 0, gText_SynthBudget);
	ConvertIntToDecimalStringN(gStringVar1, used, STR_CONV_MODE_RIGHT_ALIGN, 3);
	ConvertIntToDecimalStringN(gStringVar2, total, STR_CONV_MODE_RIGHT_ALIGN, 3);
	StringCopy(gStringVar4, gStringVar1);
	StringAppend(gStringVar4, (const u8[]) {0xBA, 0xFF}); // '/'
	StringAppend(gStringVar4, gStringVar2);
	WindowPrint(WIN_SYNTH_STATS, 1, 92, 0, &sSynthText, 0, gStringVar4);

	for (u32 i = 0; i < 6; ++i)
	{
		const struct TextColor* color =
			(sInSubMenu && sSection == SEC_STATS && i == sSelectedStat)
				? &sSynthSelected : &sSynthText;
		u8 y = 16 + i * 13;

		WindowPrint(WIN_SYNTH_STATS, 1, 4, y, color, 0, sSynthStatNames[i]);
		ConvertIntToDecimalStringN(gStringVar1, data->baseStats[i], STR_CONV_MODE_RIGHT_ALIGN, 3);
		WindowPrint(WIN_SYNTH_STATS, 1, 100, y, color, 0, gStringVar1);
	}

	CommitSynthWindow(WIN_SYNTH_STATS);
}

// Ability pool per spec: Trace always first, then the party's abilities
// (deduped, the edited Synth itself excluded), then the remembered slot if it
// isn't already present. Max 1 + 5 + 1 = 7 entries; rebuilt on demand — no
// cached state (CFRU statics are dead, and the party can change between opens).
struct SynthAbilityEntry
{
	u8 ability;
	u16 source; // SPECIES_NONE for Trace
};

static u8 BuildAbilityPool(struct SynthAbilityEntry* pool)
{
	struct SynthData* data = sSynthMenuData;
	u8 count = 0;

	pool[count].ability = ABILITY_TRACE;
	pool[count].source = SPECIES_NONE;
	++count;

	for (u32 i = 0; i < PARTY_SIZE; ++i)
	{
		struct Pokemon* mon = &gPlayerParty[i];
		u16 species = GetMonData(mon, MON_DATA_SPECIES, NULL);
		if (species == SPECIES_NONE || GetMonData(mon, MON_DATA_IS_EGG, NULL) || mon == sSynthMenuMon)
			continue;

		u8 ability = GetMonAbility(mon);
		if (ability == ABILITY_NONE)
			continue;

		bool8 dup = FALSE;
		for (u32 j = 0; j < count; ++j)
		{
			if (pool[j].ability == ability)
			{
				dup = TRUE;
				break;
			}
		}
		if (!dup)
		{
			pool[count].ability = ability;
			pool[count].source = species;
			++count;
		}
	}

	if (data->rememberedAbilityId != ABILITY_NONE)
	{
		bool8 dup = FALSE;
		for (u32 j = 0; j < count; ++j)
		{
			if (pool[j].ability == data->rememberedAbilityId)
			{
				dup = TRUE;
				break;
			}
		}
		if (!dup)
		{
			pool[count].ability = data->rememberedAbilityId;
			pool[count].source = data->rememberedSourceSpecies;
			++count;
		}
	}

	return count;
}

static void DrawSynthAbility(void)
{
	struct SynthData* data = sSynthMenuData;
	bool8 editing = (sInSubMenu && sSection == SEC_ABILITY);
	const struct TextColor* color = (sSection == SEC_ABILITY) ? &sSynthSelected : &sSynthText;
	u16 species = GetMonData(sSynthMenuMon, MON_DATA_SPECIES, NULL);

	StringCopy(gStringVar4, GetAbilityName(data->selectedAbilityId, species));

	// "(from X)" for a party-sourced selection; Trace shows bare.
	struct SynthAbilityEntry pool[8];
	u8 count = BuildAbilityPool(pool);
	for (u32 i = 0; i < count; ++i)
	{
		if (pool[i].ability == data->selectedAbilityId && pool[i].source != SPECIES_NONE)
		{
			//the string compiler strips leading/trailing spaces, so the spaces
			//around "(from" are composed here (FR charset space = 0x00)
			StringAppend(gStringVar4, (const u8[]) {0x00, 0xFF});
			StringAppend(gStringVar4, gText_SynthAbilityFrom);
			StringAppend(gStringVar4, (const u8[]) {0x00, 0xFF});
			GetSpeciesName(gStringVar1, pool[i].source);
			StringAppend(gStringVar4, gStringVar1);
			StringAppend(gStringVar4, gText_SynthParenClose);
			break;
		}
	}

	FillWindowPixelBuffer(WIN_SYNTH_ABILITY, PIXEL_FILL(1));
	WindowPrint(WIN_SYNTH_ABILITY, 0, 4, 4, color, 0, gText_SynthAbilityLabel);
	WindowPrint(WIN_SYNTH_ABILITY, 1, 64, 2, editing ? &sSynthSelected : &sSynthText, 0, gStringVar4);
	CommitSynthWindow(WIN_SYNTH_ABILITY);
}

static void DrawSynthHelp(void)
{
	const u8* text = gText_SynthHelpNav;
	if (sInSubMenu)
	{
		if (sSection == SEC_TYPE)
			text = gText_SynthHelpType;
		else if (sSection == SEC_FORM)
			text = gText_SynthHelpForm;
		else if (sSection == SEC_ABILITY)
			text = gText_SynthHelpAbility;
		else
			text = gText_SynthHelpStats;
	}

	FillWindowPixelBuffer(WIN_SYNTH_HELP, PIXEL_FILL(1));
	WindowPrint(WIN_SYNTH_HELP, 0, 4, 0, &sSynthText, 0, text);
	CommitSynthWindow(WIN_SYNTH_HELP);
}

static void DrawSynthAll(void)
{
	DrawSynthBanner();
	DrawSynthForm();
	DrawSynthTypes();
	DrawSynthStats();
	DrawSynthAbility();
	DrawSynthHelp();
}

// --- Stat editing -----------------------------------------------------------

static bool8 TryAdjustStat(s8 delta)
{
	struct SynthData* data = sSynthMenuData;
	u16 value = data->baseStats[sSelectedStat];

	if (delta > 0)
	{
		if (value >= data->statCapPerStat)
			return FALSE;
		if (!data->postGameUnlocked
		&& GetSynthAllocatedTotal(data) >= GetSynthTotalBudget(data))
			return FALSE;
		data->baseStats[sSelectedStat] = value + 1;
	}
	else
	{
		if (value <= SYNTH_STAT_MIN)
			return FALSE;
		data->baseStats[sSelectedStat] = value - 1;
	}

	SynthValidateStats(data);
	return TRUE;
}

// --- Type editing -----------------------------------------------------------

// Cycle the focused slot's type by +/-1 through the display order, skipping
// locked types. Secondary slot has a virtual "None" entry before index 0 and
// skips the primary type (duplicate typing is meaningless). Changes apply
// immediately (same rule as stats); battle re-stamps types at battle start.
static bool8 TryCycleType(s8 delta)
{
	struct SynthData* data = sSynthMenuData;
	bool8 secondary = (sTypeSlot == 1);
	u8 current = secondary ? data->secondaryType : data->primaryType;

	// Position in the virtual list: secondary list is [None, order...],
	// primary list is [order...].
	s32 count = secondary ? 19 : 18;
	s32 pos = 0;
	if (current != SYNTH_TYPE_NONE)
	{
		for (s32 i = 0; i < 18; ++i)
		{
			if (sSynthTypeOrder[i] == current)
			{
				pos = secondary ? i + 1 : i;
				break;
			}
		}
	}

	for (s32 step = 1; step <= count; ++step)
	{
		s32 candidate = (pos + delta * step % count + count) % count;
		u8 type;

		if (secondary && candidate == 0)
			type = SYNTH_TYPE_NONE;
		else
			type = sSynthTypeOrder[secondary ? candidate - 1 : candidate];

		if (type != SYNTH_TYPE_NONE)
		{
			if (!SynthIsTypeUnlocked(data, type))
				continue;
			if (secondary && type == data->primaryType)
				continue;
		}

		if (type == current)
			return FALSE; // wrapped all the way around — nothing else legal

		if (secondary)
			data->secondaryType = type;
		else
		{
			data->primaryType = type;
			// Primary landing on the secondary's type would duplicate — clear it.
			if (data->secondaryType == type)
				data->secondaryType = SYNTH_TYPE_NONE;
		}
		return TRUE;
	}

	return FALSE;
}

// --- Input ------------------------------------------------------------------

static void SynthMenuExit(u8 taskId)
{
	DestroyTask(taskId);
	if (sWindowsOk)
		FreeAllWindowBuffers();

	//SYNTH: stats are only recomputed when CalculateMonStatsNew runs (level-up,
	//evolution, a few build paths). Without this call the edits sit in SynthData
	//and the summary screen keeps showing the pre-edit numbers until the mon
	//happens to level — which reads exactly like "the override hook is broken".
	if (sSynthMenuMon != NULL)
		CalculateMonStatsNew(sSynthMenuMon);

	sSynthMenuMon = NULL;
	sSynthMenuData = NULL;
	SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
}

// Banner left/right: switch to the previous/next Synth in the party.
static bool8 TrySwitchSynthMon(s8 delta)
{
	struct SynthMenuState* state = gSynthMenuState;

	if (state->partySynthCount < 2)
		return FALSE;

	// The outgoing mon keeps its edits — recompute before switching away.
	CalculateMonStatsNew(sSynthMenuMon);

	state->partyPos = (state->partyPos + state->partySynthCount + delta) % state->partySynthCount;
	struct Pokemon* mon = &gPlayerParty[state->partySlots[state->partyPos]];

	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL)
	{
		for (u32 i = 0; i < SYNTH_SLOT_COUNT; ++i)
		{
			data = AllocSynthDataForMon(mon, i);
			if (data != NULL)
				break;
		}
	}
	if (data == NULL)
		return FALSE; // no free SynthData slot; stay on current mon

	sSynthMenuMon = mon;
	sSynthMenuData = data;
	sSelectedStat = 0;
	sTypeSlot = 0;
	return TRUE;
}

static void SynthHandleSubMenuInput(void)
{
	if (JOY_NEW(B_BUTTON))
	{
		PlaySE(SE_SELECT);
		sInSubMenu = FALSE;
		// Apply stat edits to the live mon on sub-menu exit, not only on
		// full menu exit (see the recompute note in SynthMenuExit).
		if (sSection == SEC_STATS)
			CalculateMonStatsNew(sSynthMenuMon);
		DrawSynthAll();
		return;
	}

	if (sSection == SEC_STATS)
	{
		if (JOY_REPT(DPAD_DOWN))
		{
			PlaySE(SE_SELECT);
			sSelectedStat = (sSelectedStat + 1) % 6;
			DrawSynthStats();
		}
		else if (JOY_REPT(DPAD_UP))
		{
			PlaySE(SE_SELECT);
			sSelectedStat = (sSelectedStat + 5) % 6;
			DrawSynthStats();
		}
		else if (JOY_REPT(DPAD_RIGHT) || JOY_REPT(DPAD_LEFT))
		{
			if (TryAdjustStat(JOY_REPT(DPAD_RIGHT) ? 1 : -1))
			{
				PlaySE(SE_SELECT);
				// Spec "Form Auto-Assignment": every stat edit re-resolves
				// the closest preset (sticky on ties) and updates the bar.
				u8 closest = SynthGetClosestForm(sSynthMenuData);
				if (closest != sSynthMenuData->activeFormId)
				{
					sSynthMenuData->activeFormId = closest;
					DrawSynthForm();
				}
				DrawSynthStats();
			}
			else
				PlaySE(SE_ERROR); // at cap, budget limit, or stat minimum
		}
	}
	else if (sSection == SEC_FORM)
	{
		if (JOY_REPT(DPAD_DOWN) || JOY_REPT(DPAD_UP))
		{
			// Harrison 2026-08-31: presets apply ON SCROLL — no A-to-confirm
			// (deviates from the spec's dropdown+select; matches the
			// immediate-apply rule stats and types already follow).
			u8 count = GetSynthFormCount(sSynthMenuData);
			u8 next = (sSynthMenuData->activeFormId + (JOY_REPT(DPAD_DOWN) ? 1 : count - 1)) % count;
			PlaySE(SE_SELECT);
			SynthApplyFormPreset(sSynthMenuData, next);
			CalculateMonStatsNew(sSynthMenuMon);
			DrawSynthForm();
			DrawSynthStats();
		}
	}
	else if (sSection == SEC_ABILITY)
	{
		if (JOY_REPT(DPAD_DOWN) || JOY_REPT(DPAD_UP) || JOY_REPT(DPAD_LEFT) || JOY_REPT(DPAD_RIGHT))
		{
			struct SynthAbilityEntry pool[8];
			u8 count = BuildAbilityPool(pool);
			bool8 fwd = JOY_REPT(DPAD_DOWN) || JOY_REPT(DPAD_RIGHT);
			u8 cur = gSynthMenuState->abilityCursor;

			if (cur >= count)
				cur = 0;
			cur = (cur + (fwd ? 1 : count - 1)) % count;
			gSynthMenuState->abilityCursor = cur;

			// Scroll-applies (house rule). Landing on a party-sourced ability
			// also refreshes the remembered slot — the spec's "populate on
			// confirm" collapses to this once there is no confirm step.
			sSynthMenuData->selectedAbilityId = pool[cur].ability;
			if (pool[cur].source != SPECIES_NONE)
			{
				sSynthMenuData->rememberedAbilityId = pool[cur].ability;
				sSynthMenuData->rememberedSourceSpecies = pool[cur].source;
			}

			PlaySE(SE_SELECT);
			DrawSynthAbility();
		}
	}
	else if (sSection == SEC_TYPE)
	{
		if (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT))
		{
			PlaySE(SE_SELECT);
			sTypeSlot ^= 1;
			DrawSynthTypes();
		}
		else if (JOY_REPT(DPAD_DOWN))
		{
			if (TryCycleType(1))
				PlaySE(SE_SELECT);
			else
				PlaySE(SE_ERROR);
			DrawSynthTypes();
		}
		else if (JOY_REPT(DPAD_UP))
		{
			if (TryCycleType(-1))
				PlaySE(SE_SELECT);
			else
				PlaySE(SE_ERROR);
			DrawSynthTypes();
		}
	}
}

static void SynthHandleNavInput(u8 taskId)
{
	if (JOY_NEW(B_BUTTON))
	{
		// Two-step B (spec): section -> banner, banner -> exit menu.
		if (sSection != SEC_BANNER)
		{
			PlaySE(SE_SELECT);
			sSection = SEC_BANNER;
			DrawSynthAll();
		}
		else
			SynthMenuExit(taskId);
		return;
	}

	if (JOY_NEW(A_BUTTON))
	{
		if (sSection != SEC_BANNER)
		{
			PlaySE(SE_SELECT);
			sInSubMenu = TRUE;
			if (sSection == SEC_STATS)
				sSelectedStat = 0;
			else if (sSection == SEC_TYPE)
				sTypeSlot = 0;
			else if (sSection == SEC_ABILITY)
			{
				// Seed the cursor at the current selection's pool position.
				struct SynthAbilityEntry pool[8];
				u8 count = BuildAbilityPool(pool);
				gSynthMenuState->abilityCursor = 0;
				for (u32 i = 0; i < count; ++i)
				{
					if (pool[i].ability == sSynthMenuData->selectedAbilityId)
					{
						gSynthMenuState->abilityCursor = i;
						break;
					}
				}
			}
			DrawSynthAll();
		}
		return;
	}

	if (JOY_NEW(DPAD_DOWN))
	{
		PlaySE(SE_SELECT);
		if (sSection == SEC_BANNER)
			sSection = SEC_FORM;
		else if (sSection == SEC_FORM)
			sSection = SEC_TYPE;
		else if (sSection == SEC_TYPE || sSection == SEC_STATS)
			sSection = SEC_ABILITY;
		DrawSynthAll();
	}
	else if (JOY_NEW(DPAD_UP))
	{
		PlaySE(SE_SELECT);
		if (sSection == SEC_ABILITY)
			sSection = SEC_STATS;
		else if (sSection == SEC_TYPE || sSection == SEC_STATS)
			sSection = SEC_FORM;
		else if (sSection == SEC_FORM)
			sSection = SEC_BANNER;
		DrawSynthAll();
	}
	else if (JOY_NEW(DPAD_RIGHT))
	{
		if (sSection == SEC_TYPE)
		{
			PlaySE(SE_SELECT);
			sSection = SEC_STATS;
			DrawSynthAll();
		}
		else if (sSection == SEC_BANNER && TrySwitchSynthMon(1))
		{
			PlaySE(SE_SELECT);
			DrawSynthAll();
		}
	}
	else if (JOY_NEW(DPAD_LEFT))
	{
		if (sSection == SEC_STATS)
		{
			PlaySE(SE_SELECT);
			sSection = SEC_TYPE;
			DrawSynthAll();
		}
		else if (sSection == SEC_BANNER && TrySwitchSynthMon(-1))
		{
			PlaySE(SE_SELECT);
			DrawSynthAll();
		}
	}
}

static void Task_SynthMenuInput(u8 taskId)
{
	if (!sWindowsOk)
	{
		//diagnostic mode: only B works, nothing else touched
		if (JOY_NEW(B_BUTTON))
			SynthMenuExit(taskId);
		return;
	}

	if (sInSubMenu)
		SynthHandleSubMenuInput();
	else
		SynthHandleNavInput(taskId);
}

// --- Screen setup (modeled on CB2_DexNav) ----------------------------------

static void MainCB2_SynthMenu(void)
{
	RunTasks();
	AnimateSprites();
	BuildOamBuffer();
	UpdatePaletteFade();
}

static void VBlankCB_SynthMenu(void)
{
	LoadOam();
	ProcessSpriteCopyRequests();
	TransferPlttBuffer();
}

static void SynthClearVramOamPlttRegs(void) //verbatim dexnav.c ClearVramOamPlttRegs
{
	DmaFill16(3, 0, VRAM, VRAM_SIZE);
	DmaFill32(3, 0, OAM, OAM_SIZE);
	DmaFill16(3, 0, PLTT, PLTT_SIZE);
	SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
	SetGpuReg(REG_OFFSET_BG3CNT, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG2CNT, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG1CNT, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG0CNT, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG3HOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG3VOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG2HOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG2VOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG1HOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG1VOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG0HOFS, DISPCNT_MODE_0);
	SetGpuReg(REG_OFFSET_BG0VOFS, DISPCNT_MODE_0);
}

static void SynthClearTasksAndGraphicalStructs(void)
{
	ScanlineEffect_Stop();
	ResetTasks();
	ResetSpriteData();
	ResetTempTileDataBuffers();
	ResetPaletteFade();
	FreeAllSpritePalettes();
}

static void Task_SynthMenuFadeIn(u8 taskId)
{
	if (!gPaletteFade->active)
		gTasks[taskId].func = Task_SynthMenuInput;
}

//State machine is a verbatim clone of CB2_DexNav (the known-good custom screen
//in this build) — keep the state order in lockstep with dexnav.c when editing.
void CB2_SynthMenu(void)
{
	switch (gMain.state) {
		case 0:
		default:
			SetVBlankCallback(NULL);
			SynthClearVramOamPlttRegs();
			gMain.state++;
			break;
		case 1:
			SynthClearTasksAndGraphicalStructs();
			gMain.state++;
			break;
		case 2:
			gSynthMenuState->tilemapPtr = Calloc(0x1000);
			ResetBgsAndClearDma3BusyFlags(0);
			InitBgsFromTemplates(0, sSynthBgTemplates, NELEMS(sSynthBgTemplates));
			SetBgTilemapBuffer(SBG_BACKGROUND, gSynthMenuState->tilemapPtr);
			gMain.state++;
			break;
		case 3:
			//No bg art yet (DexNav decompresses tiles+map here): zeroed VRAM +
			//zeroed tilemap = tile 0 transparent, so the backdrop color shows.
			LoadPalette(sSynthBackdrop, 0, 2);
			Menu_LoadStdPalAt(15 * 0x10);
			gMain.state++;
			break;
		case 4:
			if (!free_temp_tile_data_buffers_if_possible())
			{
				ShowBg(SBG_TEXT);
				ShowBg(SBG_TEXT_2);
				ShowBg(SBG_BACKGROUND);
				CopyBgTilemapBufferToVram(SBG_BACKGROUND);
				gMain.state++;
			}
			break;
		case 5:
			Free(gSynthMenuState->tilemapPtr);
			sWindowsOk = InitWindows(sSynthWinTemplates);
			DeactivateAllTextPrinters();
			if (!sWindowsOk)
				LoadPalette(sDiagRed, 0, 2);
			gMain.state++;
			break;
		case 6:
			BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
			gMain.state++;
			break;
		case 7:
			SetVBlankCallback(VBlankCB_SynthMenu);
			if (sWindowsOk)
				DrawSynthAll();
			CreateTask(Task_SynthMenuFadeIn, 0);
			SetMainCallback2(MainCB2_SynthMenu);
			gMain.state = 0;
			break;
	}
}

// --- Party menu entry point -------------------------------------------------
// Called by CursorCb_Synth in party_menu.c BEFORE Task_ClosePartyMenu, so the
// party menu tears down (freeing its heap) before CB2_SynthMenu loads.

bool8 SynthMenuPrepare(struct Pokemon* mon)
{
	if (!IsSynthMon(mon))
		return FALSE;

	// Lazy allocation: find existing by personality, else claim first free slot
	struct SynthData* data = GetSynthDataForMon(mon);
	if (data == NULL)
	{
		for (u32 i = 0; i < SYNTH_SLOT_COUNT; ++i)
		{
			data = AllocSynthDataForMon(mon, i);
			if (data != NULL)
				break;
		}
	}

	if (data == NULL)
		return FALSE; // all 6 slots claimed by other mons; should not happen

	// Roster of party Synths for banner left/right switching.
	struct SynthMenuState* state = gSynthMenuState;
	state->partySynthCount = 0;
	state->partyPos = 0;
	for (u32 i = 0; i < PARTY_SIZE; ++i)
	{
		if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) != SPECIES_NONE
		&& IsSynthMon(&gPlayerParty[i]))
		{
			if (&gPlayerParty[i] == mon)
				state->partyPos = state->partySynthCount;
			state->partySlots[state->partySynthCount++] = i;
		}
	}

	sSynthMenuMon = mon;
	sSynthMenuData = data;
	sSelectedStat = 0;
	sSection = SEC_BANNER; // spec: cursor always opens on the Banner
	sInSubMenu = FALSE;
	sTypeSlot = 0;
	return TRUE;
}
