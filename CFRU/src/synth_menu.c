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

#include "../include/new/dns.h" //TransferPlttBuffer decl
#include "../include/new/synth.h"

/*
synth_menu.c
	Pokémon SYNTH — the Synth Menu (MVP: stat editor pane).
	Skeleton modeled on src/dexnav.c's GUI. See specs/synth_menu.md.
*/

extern const u8 gText_SynthTitle[];
extern const u8 gText_SynthBudget[];
extern const u8 gText_SynthHP[];
extern const u8 gText_SynthAtk[];
extern const u8 gText_SynthDef[];
extern const u8 gText_SynthSpAtk[];
extern const u8 gText_SynthSpDef[];
extern const u8 gText_SynthSpeed[];
extern const u8 gText_SynthHelp[];

enum
{
	WIN_SYNTH_TITLE,
	WIN_SYNTH_STATS,
	WIN_SYNTH_HELP,
	SYNTH_WINDOW_COUNT,
};

enum SynthBgs { SBG_TEXTBOX, SBG_TEXT_2, SBG_TEXT, SBG_BACKGROUND }; //DexNav chassis

//MUST be 4-aligned: vanilla InitWindows copies templates with word loads, and
//ARM7TDMI rotates unaligned LDRs — a 2-aligned array scrambles every field
//(root cause of the 2026-07/08 renderer bug; struct's natural align is only 2).
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
	[WIN_SYNTH_STATS] =
	{
		.bg = SBG_TEXT,
		.tilemapLeft = 1,
		.tilemapTop = 3,
		.width = 18,
		.height = 14,
		.paletteNum = 15,
		.baseBlock = 57,
	},
	[WIN_SYNTH_HELP] =
	{
		.bg = SBG_TEXT,
		.tilemapLeft = 0,
		.tilemapTop = 18,
		.width = 30,
		.height = 2,
		.paletteNum = 15,
		.baseBlock = 309,
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

//(bg3 tilemap ptr removed with chassis revert)

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

// Menu state lives at a FIXED EWRAM address (gSynthMenuState in synth.h):
// CFRU's linker places .bss/COMMON in ROM, so mutable statics are silently
// dead. Never add a mutable static to this file.
#define sSynthMenuMon   (gSynthMenuState->mon)
#define sSynthMenuData  (gSynthMenuState->data)
#define sSelectedStat   (gSynthMenuState->selectedStat)

// --- Drawing ----------------------------------------------------------------

static void CommitSynthWindow(u8 windowId)
{
	PutWindowTilemap(windowId);
	CopyWindowToVram(windowId, COPYWIN_BOTH);
}

static void DrawSynthTitle(void)
{
	FillWindowPixelBuffer(WIN_SYNTH_TITLE, PIXEL_FILL(1));
	WindowPrint(WIN_SYNTH_TITLE, 1, 4, 0, &sSynthText, 0, gText_SynthTitle);
	CommitSynthWindow(WIN_SYNTH_TITLE);
}

static void DrawSynthHelp(void)
{
	FillWindowPixelBuffer(WIN_SYNTH_HELP, PIXEL_FILL(1));
	WindowPrint(WIN_SYNTH_HELP, 0, 4, 0, &sSynthText, 0, gText_SynthHelp);
	CommitSynthWindow(WIN_SYNTH_HELP);
}

static void DrawSynthStats(void)
{
	struct SynthData* data = sSynthMenuData;

	FillWindowPixelBuffer(WIN_SYNTH_STATS, PIXEL_FILL(1));

	// Budget line: "Budget  266/300"
	u16 used = GetSynthAllocatedTotal(data);
	u16 total = GetSynthTotalBudget(data);
	WindowPrint(WIN_SYNTH_STATS, 1, 4, 0, &sSynthText, 0, gText_SynthBudget);
	ConvertIntToDecimalStringN(gStringVar1, used, STR_CONV_MODE_RIGHT_ALIGN, 3);
	ConvertIntToDecimalStringN(gStringVar2, total, STR_CONV_MODE_RIGHT_ALIGN, 3);
	StringCopy(gStringVar4, gStringVar1);
	StringAppend(gStringVar4, (const u8[]) {0xBA, 0xFF}); // '/'
	StringAppend(gStringVar4, gStringVar2);
	WindowPrint(WIN_SYNTH_STATS, 1, 92, 0, &sSynthText, 0, gStringVar4);

	for (u32 i = 0; i < 6; ++i)
	{
		const struct TextColor* color = (i == sSelectedStat) ? &sSynthSelected : &sSynthText;
		u8 y = 20 + i * 14;

		WindowPrint(WIN_SYNTH_STATS, 1, 4, y, color, 0, sSynthStatNames[i]);
		ConvertIntToDecimalStringN(gStringVar1, data->baseStats[i], STR_CONV_MODE_RIGHT_ALIGN, 3);
		WindowPrint(WIN_SYNTH_STATS, 1, 100, y, color, 0, gStringVar1);
	}

	CommitSynthWindow(WIN_SYNTH_STATS);
}

// --- Input ------------------------------------------------------------------

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

static void Task_SynthMenuInput(u8 taskId)
{
	if (JOY_NEW(B_BUTTON))
	{
		DestroyTask(taskId);
		if (sWindowsOk)
			FreeAllWindowBuffers();
		sSynthMenuMon = NULL;
		sSynthMenuData = NULL;
		SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
		return;
	}

	if (!sWindowsOk)
		return; //diagnostic mode: only B works, nothing else touched

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
	else if (JOY_REPT(DPAD_RIGHT))
	{
		if (TryAdjustStat(1))
		{
			PlaySE(SE_SELECT);
			DrawSynthStats();
		}
	}
	else if (JOY_REPT(DPAD_LEFT))
	{
		if (TryAdjustStat(-1))
		{
			PlaySE(SE_SELECT);
			DrawSynthStats();
		}
	}
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
			{
				DrawSynthTitle();
				DrawSynthStats();
				DrawSynthHelp();
			}
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

	sSynthMenuMon = mon;
	sSynthMenuData = data;
	sSelectedStat = 0;
	return TRUE;
}

// --- DEBUG: boot directly into the menu with dummy data (hooks file entry
// CB2_SynthMenuBootDebug replaces the copyright screen). REMOVE BEFORE RELEASE.
void CB2_SynthMenuBootDebug(void)
{
	struct SynthData* dbg = ((struct SynthData*) 0x203D8DC) + 5; //slot 5 as debug scratch
	dbg->statBudget = 300;
	dbg->resonancePoints = 0;
	dbg->statCapPerStat = 80;
	dbg->postGameUnlocked = FALSE;
	for (u32 i = 0; i < 6; ++i)
		dbg->baseStats[i] = 50;
	sSynthMenuData = dbg;
	sSelectedStat = 0;
	gMain.state = 0;
	SetMainCallback2(CB2_SynthMenu);
}
