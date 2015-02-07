#ifndef _D_BOMBJACK_H_
#define _D_BOMBJACK_H_

#include "burnint.h"
#include "ay8910.h"
#include "sega_spr.h"
#include "sega_pcm.h"
#include "saturn/ovl.h"

#define nBurnSoundLen 128

int ovlInit(char *szShortName) __attribute__ ((boot,section(".boot")));
static INT32 BjZInit();
static INT32 BjInit();
static INT32 BjExit();
static INT32 BjFrame();

UINT8 DrvJoy1[7] = {0, 0, 0, 0, 0, 0, 0};
UINT8 DrvJoy2[7] = {0, 0, 0, 0, 0, 0, 0};
UINT8 BjDip[2] = {0, 0};
static UINT8 DrvReset = 0;
static INT32 bombjackIRQ = 0;
static INT32 latch;

static INT32 nCyclesDone[2], nCyclesTotal[2];
static INT32 nCyclesSegment;

static UINT8 *Mem = NULL;
static UINT8 *MemEnd = NULL;
static UINT8 *RamStart = NULL;
static UINT8 *RamEnd = NULL;
static UINT8 *BjGfx = NULL;
static UINT8 *BjMap = NULL;
static UINT8 *BjRom = NULL;
static UINT8 *BjRam = NULL;
static UINT8 *BjColRam = NULL;
static UINT8 *BjVidRam = NULL;
static UINT8 *BjSprRam = NULL;

// sound cpu
static UINT8 *SndRom = NULL;
static UINT8 *SndRam = NULL;

// graphics tiles
static UINT8 *text = NULL;
static UINT8 *sprites = NULL;
static UINT8 *tiles = NULL;

// pallete
static UINT8 *BjPalSrc = NULL;
static UINT32 *BjPalReal = NULL;

static INT16* pFMBuffer;
static INT16* pAY8910Buffer[9];

static UINT8 BjIsBombjackt = 0;

// Dip Switch and Input Definitions
static struct BurnInputInfo DrvInputList[] = {
	{"P1 Coin"      , BIT_DIGITAL  , DrvJoy1 + 0,	  "p1 coin"  },
	{"P1 Start"     , BIT_DIGITAL  , DrvJoy1 + 1,	  "p1 start" },

	{"P1 Up"        , BIT_DIGITAL  , DrvJoy1 + 2, 	"p1 up"    },
	{"P1 Down"      , BIT_DIGITAL  , DrvJoy1 + 3, 	"p1 down"  },
	{"P1 Left"      , BIT_DIGITAL  , DrvJoy1 + 4, 	"p1 left"  },
	{"P1 Right"     , BIT_DIGITAL  , DrvJoy1 + 5, 	"p1 right" },
	{"P1 Button 1"  , BIT_DIGITAL  , DrvJoy1 + 6,		"p1 fire 1"},

	{"P2 Coin"      , BIT_DIGITAL  , DrvJoy2 + 0,	  "p2 coin"  },
	{"P2 Start"     , BIT_DIGITAL  , DrvJoy2 + 1,	  "p2 start" },

	{"P2 Up"        , BIT_DIGITAL  , DrvJoy2 + 2, 	"p2 up"    },
	{"P2 Down"      , BIT_DIGITAL  , DrvJoy2 + 3, 	"p2 down"  },
	{"P2 Left"      , BIT_DIGITAL  , DrvJoy2 + 4, 	"p2 left"  },
	{"P2 Right"     , BIT_DIGITAL  , DrvJoy2 + 5, 	"p2 right" },
	{"P2 Button 1"  , BIT_DIGITAL  , DrvJoy2 + 6,		"p2 fire 1"},

	{"Reset"        , BIT_DIGITAL  , &DrvReset  ,		"reset"    },
	{"Dip Sw(1)"    , BIT_DIPSWITCH, BjDip + 0  ,	  "dip"      },
	{"Dip Sw(2)"    , BIT_DIPSWITCH, BjDip + 1  ,	  "dip"      },
};

STDINPUTINFO(Drv)

static struct BurnDIPInfo BjDIPList[]=
{
	// Default Values
	{0x0f, 0xff, 0xff, 0xc0, NULL},
	{0x10, 0xff, 0xff, 0x00, NULL},

	// Dip Sw(1)
	{0,		0xfe, 0,	4,	  "Coin A"},
	{0x0f, 0x01, 0x03, 0x00, "1 coin 1 credit"},
	{0x0f, 0x01, 0x03, 0x01, "1 coin 2 credits"},
	{0x0f, 0x01, 0x03, 0x02, "1 coin 3 credits"},
	{0x0f, 0x01, 0x03, 0x03, "1 coin 6 credits"},

	{0,		0xfe, 0,	4,	  "Coin B"},
	{0x0f, 0x01, 0x0c, 0x04, "2 coins 1 credit"},
	{0x0f, 0x01, 0x0c, 0x00, "1 coin 1 credit"},
	{0x0f, 0x01, 0x0c, 0x08, "1 coin 2 credits"},
	{0x0f, 0x01, 0x0c, 0x0c, "1 coin 3 credits"},

	{0,		0xfe, 0,	4,	  "Lives"},
	{0x0f, 0x01, 0x30, 0x30, "2"},
	{0x0f, 0x01, 0x30, 0x00, "3"},
	{0x0f, 0x01, 0x30, 0x10, "4"},
	{0x0f, 0x01, 0x30, 0x20, "5"},

	{0,		0xfe, 0,	2,	  "Cabinet"},
	{0x0f, 0x01, 0x40, 0x40, "Upright"},
	{0x0f, 0x01, 0x40, 0x00, "Cocktail"},

	{0,		0xfe, 0,	2,	  "Demo sounds"},
	{0x0f, 0x01, 0x80, 0x00, "Off"},
	{0x0f, 0x01, 0x80, 0x80, "On"},

	// Dip Sw(2)
	{0,		0xfe, 0,	4,	  "Initial high score"},
	{0x10, 0x01, 0x07, 0x00, "10000"},
	{0x10, 0x01, 0x07, 0x01, "100000"},
	{0x10, 0x01, 0x07, 0x02, "30000"},
	{0x10, 0x01, 0x07, 0x03, "50000"},
	{0x10, 0x01, 0x07, 0x04, "100000"},
	{0x10, 0x01, 0x07, 0x05, "50000"},
	{0x10, 0x01, 0x07, 0x06, "100000"},
	{0x10, 0x01, 0x07, 0x07, "50000"},

	{0,		0xfe, 0,	4,	  "Bird speed"},
	{0x10, 0x01, 0x18, 0x00, "Easy"},
	{0x10, 0x01, 0x18, 0x08, "Medium"},
	{0x10, 0x01, 0x18, 0x10, "Hard"},
	{0x10, 0x01, 0x18, 0x18, "Hardest"},

	{0,		0xfe, 0,	4,	  "Enemies number & speed"},
	{0x10, 0x01, 0x60, 0x20, "Easy"},
	{0x10, 0x01, 0x60, 0x00, "Medium"},
	{0x10, 0x01, 0x60, 0x40, "Hard"},
	{0x10, 0x01, 0x60, 0x60, "Hardest"},

	{0,		0xfe, 0,	2,	  "Special coin"},
	{0x10, 0x01, 0x80, 0x00, "Easy"},
	{0x10, 0x01, 0x80, 0x80, "Hard"},
};

STDDIPINFO(Bj)

// Bomb Jack (set 1)
static struct BurnRomInfo BombjackRomDesc[] = {
	{ "09j01b.bin",    0x2000, 0xc668dc30, BRF_ESS | BRF_PRG },		//  0 Z80 code
	{ "10l01b.bin",    0x2000, 0x52a1e5fb, BRF_ESS | BRF_PRG },		//  1
	{ "11m01b.bin",    0x2000, 0xb68a062a, BRF_ESS | BRF_PRG },		//  2
	{ "12n01b.bin",    0x2000, 0x1d3ecee5, BRF_ESS | BRF_PRG },		//  3
	{ "13.1r",          0x2000, 0x70e0244d, BRF_ESS | BRF_PRG },		//  4

	// graphics 3 bit planes:
	{ "03e08t.bin",    0x1000, 0x9f0470d5, BRF_GRA },			 // chars
	{ "04h08t.bin",    0x1000, 0x81ec12e6, BRF_GRA },
	{ "05k08t.bin",    0x1000, 0xe87ec8b1, BRF_GRA },

	{ "14j07b.bin",    0x2000, 0x101c858d, BRF_GRA },			 // sprites
	{ "15l07b.bin",    0x2000, 0x013f58f2, BRF_GRA },
	{ "16m07b.bin",    0x2000, 0x94694097, BRF_GRA },

	{ "06l08t.bin",    0x2000, 0x51eebd89, BRF_GRA },			 // background tiles
	{ "07n08t.bin",    0x2000, 0x9dd98e9d, BRF_GRA },
	{ "08r08t.bin",    0x2000, 0x3155ee7d, BRF_GRA },

	{ "02p04t.bin",    0x1000, 0x398d4a02, BRF_GRA },			 // background tilemaps

	{ "01h03t.bin",    0x2000, 0x8407917d, BRF_ESS | BRF_SND },		// sound CPU
};

STD_ROM_PICK(Bombjack)
STD_ROM_FN(Bombjack)


// Bomb Jack (set 2)
static struct BurnRomInfo Bombjac2RomDesc[] = {
	{ "09_j01b.bin",    0x2000, 0xc668dc30, BRF_ESS | BRF_PRG },		//  0 Z80 code
	{ "10_l01b.bin",    0x2000, 0x52a1e5fb, BRF_ESS | BRF_PRG },		//  1
	{ "11_m01b.bin",    0x2000, 0xb68a062a, BRF_ESS | BRF_PRG },		//  2
	{ "12_n01b.bin",    0x2000, 0x1d3ecee5, BRF_ESS | BRF_PRG },		//  3
	{ "13_r01b.bin",    0x2000, 0xbcafdd29, BRF_ESS | BRF_PRG },		//  4

	// graphics 3 bit planes:
	{ "03_e08t.bin",    0x1000, 0x9f0470d5, BRF_GRA },			 // chars
	{ "04_h08t.bin",    0x1000, 0x81ec12e6, BRF_GRA },
	{ "05_k08t.bin",    0x1000, 0xe87ec8b1, BRF_GRA },

	{ "14_j07b.bin",    0x2000, 0x101c858d, BRF_GRA },			 // sprites
	{ "15_l07b.bin",    0x2000, 0x013f58f2, BRF_GRA },
	{ "16_m07b.bin",    0x2000, 0x94694097, BRF_GRA },

	{ "06_l08t.bin",    0x2000, 0x51eebd89, BRF_GRA },			 // background tiles
	{ "07_n08t.bin",    0x2000, 0x9dd98e9d, BRF_GRA },
	{ "08_r08t.bin",    0x2000, 0x3155ee7d, BRF_GRA },

	{ "02_p04t.bin",    0x1000, 0x398d4a02, BRF_GRA },			 // background tilemaps

	{ "01_h03t.bin",    0x2000, 0x8407917d, BRF_ESS | BRF_SND },		// sound CPU
};

STD_ROM_PICK(Bombjac2)
STD_ROM_FN(Bombjac2)
/*
struct BurnDriver BurnDrvBombjack = {
	"bombjack", NULL, NULL, NULL, "1984",
	"Bomb Jack (set 1)\0", NULL, "Tehkan", "Bomb Jack",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING,2,HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, BombjackRomInfo,BombjackRomName, NULL, NULL,DrvInputInfo,BjDIPInfo,
	BjInit,BjExit,BjFrame,NULL,BjScan,
	NULL,0x80,224,256,3,4
};

struct BurnDriver BurnDrvBombjac2 = {
	"bombjack2", "bombjack", NULL, NULL, "1984",
	"Bomb Jack (set 2)\0", NULL, "Tehkan", "Bomb Jack",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE,2,HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, Bombjac2RomInfo,Bombjac2RomName, NULL, NULL,DrvInputInfo,BjDIPInfo,
	BjInit,BjExit,BjFrame,NULL,BjScan,
	NULL,0x80,224,256,3,4
};

struct BurnDriver BurnDrvBombjackt = {
	"bombjackt", "bombjack", NULL, NULL, "1984",
	"Bomb Jack (Tecfri, Spain)\0", NULL, "Tehkan (Tecfri License)", "Bomb Jack",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE,2,HARDWARE_MISC_PRE90S, GBF_PLATFORM, 0,
	NULL, BombjacktRomInfo,BombjacktRomName, NULL, NULL,DrvInputInfo,BjDIPInfo,
	BjtInit,BjExit,BjFrame,NULL,BjScan,
	NULL,0x80,224,256,3,4
};
*/
#endif