
#include "burnint.h"
#include "m6502.new/m6502_intf.h"
//#include "m6502.new/m6502.h"
#include "slapstic.h"
#include "snd/sn76496.h"
#include "saturn/ovl.h"

#define SOUND_LEN 256
int ovlInit(char *szShortName) __attribute__ ((boot,section(".boot")));

/*static*/ UINT8 *SaturnMem = NULL;
///*static*/ UINT8 *ss_vram;
/*static*/ Uint16 *cram_lut;
/*static*/ UINT16 *map_offset_lut;
/*static*/ unsigned char 	bg_dirtybuffer[2048];

/*static*/ UINT8 *AllMem = NULL;
/*static*/ UINT8 *MemEnd = NULL;
/*static*/ UINT8 *AllRam = NULL;
/*static*/ UINT8 *RamEnd = NULL;
/*static*/ UINT8 *Drv6502ROM = NULL;
/*static*/ UINT8 *DrvGfxROM = NULL;
/*static*/ UINT8 *DrvNVRAM = NULL;
/*static*/ UINT8 *Drv6502RAM = NULL;
/*static*/ UINT8 *DrvVidRAM = NULL;
/*static*/ UINT8 *DrvPalRAM = NULL;

/*static*/ INT32 nvram_enable;

/*static*/ UINT32 *DrvPalette = NULL;
/*static*/ UINT8   DrvRecalc;

/*static*/ INT32 watchdog;
/*static*/ INT32 master_clock;
/*static*/ INT32 vblank;
/*static*/ INT32 is_Bootleg;

/*static*/ UINT8 DrvJoy1[8];
/*static*/ UINT8 DrvJoy2[8];
/*static*/ UINT8 DrvDips[1];
/*static*/ UINT8 DrvInputs[2];
/*static*/ UINT8 DrvReset;

/*static*/ INT32 DrvInit();
/*static*/ INT32 DrvExit();
/*static*/ INT32 DrvFrame();
/*static*/ INT32 DrvDraw();
/*static*/ void DrawLayer();

/*static*/ struct BurnInputInfo AtetrisInputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 1,	"p1 coin"	},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy2 + 3,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 2,	"p1 right"	},
	{"P1 Button",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 fire 1"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy1 + 0,	"p2 coin"	},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 5,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 7,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 6,	"p2 right"	},
	{"P2 Button",		BIT_DIGITAL,	DrvJoy2 + 4,	"p2 fire 1"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Dips",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
};

STDINPUTINFO(Atetris)

/*static*/ struct BurnDIPInfo AtetrisDIPList[]=
{
	{0x0b, 0xff, 0xff, 0x00, NULL		},

	{0   , 0xfe, 0   ,    2, "Freeze"	},
	{0x0b, 0x01, 0x04, 0x00, "Off"		},
	{0x0b, 0x01, 0x04, 0x04, "On"		},

	{0   , 0xfe, 0   ,    2, "Freeze Step"	},
	{0x0b, 0x01, 0x08, 0x00, "Off"		},
	{0x0b, 0x01, 0x08, 0x08, "On"		},

	{0   , 0xfe, 0   ,    2, "Service Mode"	},
	{0x0b, 0x01, 0x80, 0x00, "Off"		},
	{0x0b, 0x01, 0x80, 0x80, "On"		},
};

STDDIPINFO(Atetris)

/*static*/ struct BurnDIPInfo AtetriscDIPList[]=
{
	{0x0b, 0xff, 0xff, 0x00, NULL		},

	{0   , 0xfe, 0   ,    2, "Freeze"	},
	{0x0b, 0x01, 0x04, 0x00, "Off"		},
	{0x0b, 0x01, 0x04, 0x04, "On"		},

	{0   , 0xfe, 0   ,    2, "Freeze Step"	},
	{0x0b, 0x01, 0x08, 0x00, "Off"		},
	{0x0b, 0x01, 0x08, 0x08, "On"		},

	{0   , 0xfe, 0   ,    0, "Flip Controls"},
	{0x0b, 0x01, 0x20, 0x00, "Off"		},
	{0x0b, 0x01, 0x20, 0x20, "On"		},

	{0   , 0xfe, 0   ,    2, "Service Mode"	},
	{0x0b, 0x01, 0x80, 0x00, "Off"		},
	{0x0b, 0x01, 0x80, 0x80, "On"		},
};

STDDIPINFO(Atetrisc)



// Tetris (cocktail set 1)

/*static*/ struct BurnRomInfo atetriscRomDesc[] = {
	{ "tetcktl1.rom",	0x10000, 0x9afd1f4a, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "136066-1103.35a",	0x10000, 0xec2a7f93, 2 | BRF_GRA },           //  1 Graphics Tiles
};

STD_ROM_PICK(atetrisc)
STD_ROM_FN(atetrisc)

// Tetris (cocktail set 2)

/*static*/ struct BurnRomInfo atetrisc2RomDesc[] = {
	{ "136066-1102.45f",	0x10000, 0x1bd28902, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "136066-1103.35a",	0x10000, 0xec2a7f93, 2 | BRF_GRA },           //  1 Graphics Tiles
};

STD_ROM_PICK(atetrisc2)
STD_ROM_FN(atetrisc2)

/*
struct BurnDriver BurnDrvAtetris = {
	"atetris", NULL, NULL, NULL, "1988",
	"Tetris (set 1)\0", "No sound", "Atari Games", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetrisRomInfo, atetrisRomName, NULL, NULL, AtetrisInputInfo, AtetrisDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	336, 240, 4, 3
};
*/

// Tetris (set 2)

/*static*/ struct BurnRomInfo atetrisaRomDesc[] = {
	{ "d1",			0x10000, 0x2bcab107, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "136066-1101.35a",	0x10000, 0x84a1939f, 2 | BRF_GRA },           //  1 Graphics Tiles
};

STD_ROM_PICK(atetrisa)
STD_ROM_FN(atetrisa)

// Tetris (bootleg set 1)

/*static*/ struct BurnRomInfo atetrisbRomDesc[] = {
	{ "tetris.01",		0x10000, 0x944d15f6, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "tetris.02",		0x10000, 0x5c4e7258, 2 | BRF_GRA },           //  1 Graphics Tiles

	{ "tetris.03",		0x00800, 0x26618c0b, 0 | BRF_OPT },           //  2 Unknown Prom
};

STD_ROM_PICK(atetrisb)
STD_ROM_FN(atetrisb)

// Tetris (bootleg set 2)

/*static*/ struct BurnRomInfo atetrisb2RomDesc[] = {
	{ "k1-01",		0x10000, 0xfa056809, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "136066-1101.35a",	0x10000, 0x84a1939f, 2 | BRF_GRA },           //  1 Graphics Tiles
};

STD_ROM_PICK(atetrisb2)
STD_ROM_FN(atetrisb2)

// Tetris (set 1)

/*static*/ struct BurnRomInfo atetrisRomDesc[] = {
	{ "1100.45f",	0x10000, 0x2acbdb09, 1 | BRF_ESS | BRF_PRG }, //  0 6502 Code

	{ "1101.35a",	0x10000, 0x84a1939f, 2 | BRF_GRA },           //  1 Graphics Tiles
};

STD_ROM_PICK(atetris)
STD_ROM_FN(atetris)

/*
struct BurnDriver BurnDrvAtetrisb2 = {
	"atetrisb2", "atetris", NULL, NULL, "1988",
	"Tetris (bootleg set 2)\0", NULL, "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetrisb2RomInfo, atetrisb2RomName, NULL, NULL, AtetrisInputInfo, AtetrisDIPInfo,
	BootInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	336, 240, 4, 3
};

struct BurnDriver BurnDrvAtetrisc = {
	"atetrisc", "atetris", NULL, NULL, "1989",
	"Tetris (cocktail set 1)\0", "No sound", "Atari Games", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetriscRomInfo, atetriscRomName, NULL, NULL, AtetrisInputInfo, AtetriscDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	240, 336, 3, 4
};

struct BurnDriver BurnDrvAtetrisc2 = {
	"atetrisc2", "atetris", NULL, NULL, "1989",
	"Tetris (cocktail set 2)\0", "No sound", "Atari Games", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetrisc2RomInfo, atetrisc2RomName, NULL, NULL, AtetrisInputInfo, AtetriscDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	240, 336, 3, 4
};

struct BurnDriver BurnDrvAtetrisa = {
	"atetrisa", "atetris", NULL, NULL, "1988",
	"Tetris (set 2)\0", "No sound", "Atari Games", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetrisaRomInfo, atetrisaRomName, NULL, NULL, AtetrisInputInfo, AtetrisDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	336, 240, 4, 3
};

struct BurnDriver BurnDrvAtetrisb = {
	"atetrisb", "atetris", NULL, NULL, "1988",
	"Tetris (bootleg set 1)\0", "No sound", "bootleg", "Miscellaneous",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_BOOTLEG, 2, HARDWARE_MISC_PRE90S, GBF_PUZZLE, 0,
	NULL, atetrisbRomInfo, atetrisbRomName, NULL, NULL, AtetrisInputInfo, AtetrisDIPInfo,
	DrvInit, DrvExit, DrvFrame, DrvDraw, NULL, &DrvRecalc, 0x100,
	336, 240, 4, 3
};
*/
