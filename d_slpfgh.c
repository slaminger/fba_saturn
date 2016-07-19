// Tiger Heli, Get Star / Guardian, & Slap Fight

#include "d_slpfgh.h"

int ovlInit(char *szShortName)
{
	struct BurnDriver nBurnDrvTigerHB1 = {
		"tigerhb1", "slpfgh", 
		"Tiger Heli (bootleg, set 1)",
		tigerhb1RomInfo, tigerhb1RomName, tigerhInputInfo, tigerhDIPInfo,
		tigerhInit, tigerhExit, tigerhFrame, NULL
	};

	if (strcmp(nBurnDrvTigerHB1.szShortName, szShortName) == 0) 
	memcpy(shared,&nBurnDrvTigerHB1,sizeof(struct BurnDriver));

	ss_reg    = (SclNorscl *)SS_REG;
	ss_regs  = (SclSysreg *)SS_REGS;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static INT32 MemIndex()
{
	UINT8* Next; Next = Mem;
	Rom01				= Next; Next += 0x012000;		// Z80 main program
	Rom02				= Next; Next += 0x002000;		// Z80 sound program
//	Rom03				= Next; Next += 0x000800;		// m68705 mcu program
	TigerHeliTextROM	= (UINT8 *)(SS_CACHE); //Next; Next += 0x010000;
 	UINT8 *ss_vram = (UINT8 *)SS_SPRAM;
	TigerHeliSpriteROM	= &ss_vram[0x2200]; //Next; Next += 3 * 16 * 16 * 256; // 3 banks, 16x16 tiles, 256 tiles (sprites)
//		= 0x00240000;//Next; Next += 0x040000;
	TigerHeliTileROM	= (UINT8 *)(SS_CACHE+0x8000); //0x00280000;//Next; Next += 0x040000;
	RamStart			= Next;
	Ram01				= Next; Next += 0x000800;		// Z80 main work RAM
	RamShared			= Next; Next += 0x000800;		// Shared RAM
	TigerHeliTextRAM	= Next; Next += 0x001000;
	TigerHeliSpriteRAM	= Next; Next += 0x000800;
	TigerHeliSpriteBuf	= Next; Next += 0x000800;
	TigerHeliTileRAM	= Next; Next += 0x001000;
//	Ram03			= Next; Next += 0x000080;
	RamEnd				= Next;
	pFMBuffer			= (INT16*)Next; Next += nBurnSoundLen * 6 * sizeof(INT16);
	TigerHeliPaletteROM	= Next; Next += 0x000300;
	TigerHeliPalette	= (UINT32*)Next; Next += 0x000100 * sizeof(UINT32);
	MemEnd				= Next;

	return 0;
}

// ---------------------------------------------------------------------------
//	Graphics

static INT32 nTigerHeliTileXPosLo, nTigerHeliTileXPosHi, nTigerHeliTileYPosLo;
static INT32 nTigerHeliTileMask, nTigerHeliSpriteMask;

static UINT8 tigerhRecalcPalette = 0;

static void TigerHeliPaletteInit()
{
	int delta=0;
	for (INT32 i = 0; i < 0x0100; i++) {
		INT32 r, g, b;

		r = TigerHeliPaletteROM[i + 0x0000];	  // Red
		r |= r << 4;
		g = TigerHeliPaletteROM[i + 0x0100];	  // Green
		g |= g << 4;
		b = TigerHeliPaletteROM[i + 0x0200];	  // Blue
		b |= b << 4;

		colBgAddr[delta] = colBgAddr2[i] = TigerHeliPalette[i] = BurnHighCol(r, g, b, 0);
		delta++; if ((delta & 3) == 0) delta += 12;
	}

	return;
}

// ---------------------------------------------------------------------------
UINT8 __fastcall tigerhReadCPU0(UINT16 a)
{
	if (a >= 0xc800 && a <= 0xcfff) {
		if (CZetGetPC(-1) == 0x6d34) return 0xff;
		return RamShared[a - 0xc800];
	}
	
	switch (a) {
		case 0xE803: {
			
			UINT8 nProtectSequence[3] = { 0, 1, (0 + 5) ^ 0x56 };
			
			UINT8 val = nProtectSequence[nProtectIndex];
			nProtectIndex = (nProtectIndex + 1) % 3;
			return val;
		}

	}

	return 0;
}

UINT8 __fastcall tigerhReadCPU0_tigerhb1(UINT16 a)
{
	if (a >= 0xc800 && a <= 0xcfff) {
		if (CZetGetPC(-1) == 0x6d34) return 0xff;
		return RamShared[a - 0xc800];
	}
	
	switch (a) {
		case 0xE803: {
			return 0x83;
		}
	}
	
	return 0;
}


void __fastcall tigerhWriteCPU0(UINT16 a, UINT8 d)
{
	if(a>= 0xD000 && a <= 0xDFFF)
	{
		a-= 0xD000;
		if(TigerHeliTileRAM[a]!=d)
		{
			TigerHeliTileRAM[a] = d;
			if(a>=0x800)
			{
				a &=0x7ff;
				INT32 sx = (a & 0x3f);// * 8;
				INT32 sy = (a / 0x40);// * 8;

				INT32 attr  = TigerHeliTileRAM[a] + (d * 0x100);
				INT32 code  = (attr & 0x0fff) & nTigerHeliTileMask;
				INT32 color = (attr & 0xf000) >> 12;

				int x = (sx|(sy<<6))<<1;
				ss_map2[x] = color;
				ss_map2[x+1] = code+0x1000;
			}
		}
		 return;
	}

	if(a>= 0xF000 && a <= 0xFFFF)
	{
		a-= 0xF000;
		if(TigerHeliTextRAM[a]!=d)
		{
			TigerHeliTextRAM[a] = d;
			if(a>=0x800)
			{
				a &=0x7ff;
				INT32 sx = (a & 0x3f); // * 8); // - 8;
				INT32 sy = (a / 0x40); // * 8); // - 15;

				INT32 attr  = TigerHeliTextRAM[a] + (d * 0x100);
				INT32 code  =  attr & 0x03ff;
				INT32 color = (attr & 0xfc00) >> 10;

				int x = (sx|(sy<<6))<<1;
				ss_map[x] = color & 0x3f;
				ss_map[x+1] = code;
			}
		}
		 return;
	}

	switch (a) {
		case 0xE800:
			nTigerHeliTileXPosLo = d;
			break;
		case 0xE801:
			nTigerHeliTileXPosHi = d;
			break;
		case 0xE802:
			nTigerHeliTileYPosLo = d;
			break;

		case 0xe803: 

			break;

	}
}

void __fastcall tigerhWriteCPU0_slapbtuk(UINT16 a, UINT8 d)
{
	switch (a) {
		case 0xE800:
			nTigerHeliTileXPosHi = d;
			break;
		case 0xE802:
			nTigerHeliTileYPosLo = d;
			break;
		case 0xE803:
			nTigerHeliTileXPosLo = d;
			break;
	}
}

UINT8 __fastcall tigerhInCPU0(UINT16 a)
{
	a &= 0xFF;

	switch (a) {
		case 0x00: {

			UINT8 nStatusSequence[3] = { 0xC7, 0x55, 0x00 };

			UINT8 nStatus = nStatusSequence[nStatusIndex];
			nStatusIndex++;
			if (nStatusIndex > 2) nStatusIndex = 0;
			return nStatus;
		}
	}

	return 0;
}

UINT8 __fastcall tigerhInCPU0_gtstarba(UINT16 a)
{
	a &= 0xFF;

	switch (a) {
		case 0x00: {
			if (CZetGetPC(-1) == 0x6d1e)	return 0;
			if (CZetGetPC(-1) == 0x6d24)	return 6;
			if (CZetGetPC(-1) == 0x6d2c)	return 2;
			if (CZetGetPC(-1) == 0x6d34)	return 4;
			return 0;
		}
	}

	return 0;
}

void __fastcall tigerhOutCPU0(UINT16 a, UINT8  d)
{
	a &= 0xFF;

	switch (a) {
		case 0x00:					// Assert reset line on sound CPU

			if (bSoundCPUEnable) {
				CZetClose();
				CZetOpen(1);
				CZetReset();
				CZetClose();
				CZetOpen(0);

				bSoundCPUEnable = false;
			}

			break;
		case 0x01:					// Release reset line on sound CPU
			bSoundCPUEnable = true;
			break;

		case 0x02:
		case 0x03:
			nFlipscreen = ~a & 1;
			break;

		case 0x06:					// Disable interrupts

			bInterruptEnable = false;
			CZetSetIRQLine(0, CZET_IRQSTATUS_NONE);
			break;
		case 0x07:					// Enable interrupts

			bInterruptEnable = true;
			break;

		case 0x08:

			// ROM bank 0 selected
			CZetMapArea(0x8000, 0xBFFF, 0, Rom01 + 0x8000);
			CZetMapArea(0x8000, 0xBFFF, 2, Rom01 + 0x8000);
			break;
		case 0x09:

			// ROM bank 1 selected
			CZetMapArea(0x8000, 0xBFFF, 0, Rom01 + 0xC000);
			CZetMapArea(0x8000, 0xBFFF, 2, Rom01 + 0xC000);
			break;

		case 0x0c:
		case 0x0d:
			nPalettebank = a & 1;
			break;;
	}
}

UINT8 __fastcall tigerhReadCPU1(UINT16 a)
{
	switch (a) {
		case 0xA081:
			return AY8910Read(0);
		case 0xA091:
			return AY8910Read(1);
	}

	return 0;
}

void __fastcall tigerhWriteCPU1(UINT16 a, UINT8 d)
{
	switch (a) {
		case 0xA080:
			AY8910Write(0, 0, d);
			break;
		case 0xA082:
			AY8910Write(0, 1, d);
			break;
		case 0xA090:
			AY8910Write(1, 0, d);
			break;
		case 0xA092:
			AY8910Write(1, 1, d);
			break;
		case 0xA0E0:
			bSoundNMIEnable = true;
			break;
	}
}

UINT8 __fastcall tigerhInCPU1(UINT16 a)
{
	return 0;
}

void __fastcall tigerhOutCPU1(UINT16 a, UINT8 d)
{
//	bprintf(PRINT_NORMAL, _T("Attempt by CPU1 to write port %02X -> %02X.\n"), a, d);
}

static UINT8 tigerhReadPort0(UINT32 data)
{
	return ~tigerhInput[0];
}
static UINT8 tigerhReadPort1(UINT32 data)
{
	return ~tigerhInput[1];
}
static UINT8 tigerhReadPort2(UINT32 data)
{
	return ~tigerhInput[2];
}
static UINT8 tigerhReadPort3(UINT32 data)
{
	return ~tigerhInput[3];
}
// ---------------------------------------------------------------------------

static INT32 tigerhLoadROMs()
{
	INT32 nRomOffset = 0;
	if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb2")) nRomOffset = 1;
	if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb3")) nRomOffset = 2;

	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom ",4,80);
	// Z80 main program
	switch (nWhichGame) {
		case 0:											// Tiger Heli
			if (BurnLoadRom(Rom01 + 0x0000, 0, 1)) {
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 1",4,80);
				return 1;
			}
			if (BurnLoadRom(Rom01 + 0x4000, 1, 1)) {
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 2",4,80);
				return 1;
			}
			if (BurnLoadRom(Rom01 + 0x8000, 2, 1)) {
				//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 3",4,80);
				return 1;
			}
			break;
		case 2:	{										// Slap Fight
			if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb2")) {
				if (BurnLoadRom(Rom01 + 0x0000, 0, 1)) {
					return 1;
				}
				if (BurnLoadRom(Rom01 + 0x4000, 1, 1)) {
					return 1;
				}
				if (BurnLoadRom(Rom01 + 0x8000, 2, 1)) {
					return 1;
				}
				break;
			} else {
				if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb3")) {
					if (BurnLoadRom(Rom01 + 0x0000, 0, 1)) {
						return 1;
					}
					if (BurnLoadRom(Rom01 + 0x4000, 1, 1)) {
						return 1;
					}
					if (BurnLoadRom(Rom01 + 0x10000, 2, 1)) {
						return 1;
					}
					if (BurnLoadRom(Rom01 + 0x8000, 3, 1)) {
						return 1;
					}
					break;
				} else {
					if (BurnLoadRom(Rom01 + 0x0000, 0, 1)) {
						return 1;
					}
					if (BurnLoadRom(Rom01 + 0x8000, 1, 1)) {
						return 1;
					}
					break;
				}
			}
		}
	}

	// Sprites
	{
		INT32 nRet = 0, nBaseROM = 0;
		switch (nWhichGame) {
			case 0:										// Tiger Heli
				nBaseROM = 3;
				break;
			case 2:										// Slap Fight
				nBaseROM = 2 + nRomOffset;
				break;
		}

		INT32 nSize;

		{
			struct BurnRomInfo ri;

			ri.nType = 0;
			ri.nLen = 0;

			BurnDrvGetRomInfo(&ri, nBaseROM);

			nSize = ri.nLen;
		}

//		UINT8* pTemp = (UINT8*)BurnMalloc(nSize * 4);
		UINT8* pTemp = (UINT8*)0x00240000;
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 4",4,80);

		for (INT32 i = 0; i < 4; i++) {
			nRet |= BurnLoadRom(pTemp + nSize * i, nBaseROM + i, 1);
		}
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 5",4,80);

		for (INT32 i = 0; i < nSize; i++) {
			for (INT32 j = 0; j < 8; j++) {
				TigerHeliSpriteROM[(i << 3) + j]  = ((pTemp[i + nSize * 0] >> (7 - j)) & 1) << 3;
				TigerHeliSpriteROM[(i << 3) + j] |= ((pTemp[i + nSize * 1] >> (7 - j)) & 1) << 2;
				TigerHeliSpriteROM[(i << 3) + j] |= ((pTemp[i + nSize * 2] >> (7 - j)) & 1) << 1;
				TigerHeliSpriteROM[(i << 3) + j] |= ((pTemp[i + nSize * 3] >> (7 - j)) & 1) << 0;
			}
		}

		for (INT32 i = 0; i < nSize*8; i+=2) 
		{
			TigerHeliSpriteROM[(i >>1)]  = TigerHeliSpriteROM[i] << 4 | TigerHeliSpriteROM[i+1];
		}

//		BurnFree(pTemp);
		pTemp = NULL;

		nTigerHeliSpriteMask = (nSize >> 5) - 1;

		if (nRet) {
			return 1;
		}
	}

	// Text layer
	{
		INT32 nBaseROM = 0;
		switch (nWhichGame) {
			case 0:										// Tiger Heli
				nBaseROM = 7;
				break;
			case 1:										// Get Star
				nBaseROM = 7;
				break;
			case 2:										// Slap Fight
				nBaseROM = 6 + nRomOffset;
				break;
		}

//		UINT8* pTemp = (UINT8*)BurnMalloc(0x4000);
		UINT8* pTemp = (UINT8*)0x00240000;
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 6",4,80);

		if (BurnLoadRom(pTemp + 0x0000, nBaseROM + 0, 1)) {
			return 1;
		}
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 7",4,80);

		if (BurnLoadRom(pTemp + 0x2000, nBaseROM + 1, 1)) {
			return 1;
		}

		for (INT32 i = 0; i < 0x02000; i++) {
			for (INT32 j = 0; j < 8; j++) {
				TigerHeliTextROM[(i << 3) + j]  = ((pTemp[i + 0x0000] >> (7 - j)) & 1) << 1;
				TigerHeliTextROM[(i << 3) + j] |= ((pTemp[i + 0x2000] >> (7 - j)) & 1) << 0;
			}
		}

		for (INT32 i = 0; i < 0x10000; i+=2) 
		{
			TigerHeliTextROM[(i >>1)]  = TigerHeliTextROM[i] << 4 | TigerHeliTextROM[i+1];
		}

//		BurnFree(pTemp);
		pTemp = NULL;
	}

	// Tile layer
	{
		INT32 nRet = 0, nBaseROM = 0;
		switch (nWhichGame) {
			case 0:										// Tiger Heli
				nBaseROM = 9;
				break;
			case 1:										// Get Star
				nBaseROM = 9;
				break;
			case 2:										// Slap Fight
				nBaseROM = 8 + nRomOffset;
				break;
		}

		INT32 nSize;

		{
			struct BurnRomInfo ri;

			ri.nType = 0;
			ri.nLen = 0;

			BurnDrvGetRomInfo(&ri, nBaseROM);

			nSize = ri.nLen;
		}

//		UINT8* pTemp = (UINT8*)BurnMalloc(nSize * 4);
		UINT8* pTemp = (UINT8*)0x00240000;
	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 8",4,80);

		for (INT32 i = 0; i < 4; i++) {
			nRet |= BurnLoadRom(pTemp + nSize * i, nBaseROM + i, 1);
		}

  	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom 9",4,80);

		for (INT32 i = 0; i < nSize; i++) {
			for (INT32 j = 0; j < 8; j++) {
				TigerHeliTileROM[(i << 3) + j]  = ((pTemp[i + nSize * 0] >> (7 - j)) & 1) << 3;
				TigerHeliTileROM[(i << 3) + j] |= ((pTemp[i + nSize * 1] >> (7 - j)) & 1) << 2;
				TigerHeliTileROM[(i << 3) + j] |= ((pTemp[i + nSize * 2] >> (7 - j)) & 1) << 1;
				TigerHeliTileROM[(i << 3) + j] |= ((pTemp[i + nSize * 3] >> (7 - j)) & 1) << 0;
			}
		}

		for (INT32 i = 0; i < nSize*8; i+=2) 
		{
			TigerHeliTileROM[(i >>1)]  = TigerHeliTileROM[i] << 4 | TigerHeliTileROM[i+1];
		}

//		BurnFree(pTemp);
		pTemp = NULL;

		nTigerHeliTileMask = (nSize >> 3) - 1;

		if (nRet) {
			return 1;
		}
	}

	// Colour PROMs
	{
		INT32 nBaseROM = 0;
		switch (nWhichGame) {
			case 0:										// Tiger Heli
				nBaseROM = 13;
				break;
			case 2:										// Slap Fight
				nBaseROM = 12 + nRomOffset;
				break;
		}
   	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom10",4,80);

		if (BurnLoadRom(TigerHeliPaletteROM + 0x0000, nBaseROM + 0, 1)) {
			return 1;
		}
  	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom11",4,80);

		if (BurnLoadRom(TigerHeliPaletteROM + 0x0100, nBaseROM + 1, 1)) {
			return 1;
		}
		if (BurnLoadRom(TigerHeliPaletteROM + 0x0200, nBaseROM + 2, 1)) {
			return 1;
		}
	}

	// Z80 program
	{
		INT32 nBaseROM = 0;
		switch (nWhichGame) {
			case 0:										// Tiger Heli
				nBaseROM = 16;
				break;
			case 2:										// Slap Fight
				nBaseROM = 15 + nRomOffset;
				break;
		}
  	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom12",4,80);
		if (BurnLoadRom(Rom02, nBaseROM, 1)) {
			return 1;
		}
	}
  	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"loadrom13",4,80);

	return 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------
static void initLayers()
{
    Uint16	CycleTb[]={
		0xff45, 0x6fff, //A0
		0xffff, 0xffff,	//A1
		0x012f,0xffff,   //B0
		0xffff, 0xffff  //B1
//		0x4eff, 0x1fff, //B1
	};
 	SclConfig	scfg;

// 3 nbg
	scfg.dispenbl			= ON;
	scfg.charsize			= SCL_CHAR_SIZE_1X1;//OK du 1*1 surtout pas toucher
	scfg.pnamesize		= SCL_PN2WORD; //2word
	scfg.platesize		= SCL_PL_SIZE_1X1; // ou 2X2 ?
	scfg.coltype			= SCL_COL_TYPE_16;//SCL_COL_TYPE_256;
	scfg.datatype		= SCL_CELL;
	scfg.flip					= SCL_PN_10BIT; // on force � 0
	scfg.patnamecontrl =  0x0000; // a0 + 0x8000
	scfg.plate_addr[0] = (Uint32)SS_MAP;
	scfg.plate_addr[1] = (Uint32)SS_MAP;
	scfg.plate_addr[2] = (Uint32)SS_MAP;
	scfg.plate_addr[3] = (Uint32)SS_MAP;
	SCL_SetConfig(SCL_NBG0, &scfg);
// 3 nbg

//	scfg.pnamesize		= SCL_PN1WORD; //2word
	scfg.platesize		= SCL_PL_SIZE_1X1; // ou 2X2 ?
	scfg.flip					= SCL_PN_10BIT; // on force � 0
	scfg.patnamecontrl =  0x0000; // a1
	scfg.plate_addr[0] = (Uint32)SS_MAP2;
	scfg.plate_addr[1] = (Uint32)SS_MAP2;
	scfg.plate_addr[2] = (Uint32)SS_MAP2;
	scfg.plate_addr[3] = (Uint32)SS_MAP2;

	SCL_SetConfig(SCL_NBG2, &scfg);

//	scfg.dispenbl			= OFF;
	scfg.bmpsize 	   = SCL_BMP_SIZE_512X256;
	scfg.datatype 	   = SCL_BITMAP;
	scfg.mapover	   = SCL_OVER_0;
	scfg.plate_addr[0] = (Uint32)SS_FONT;
	scfg.plate_addr[1] = NULL;
	scfg.plate_addr[2] = NULL;
	scfg.plate_addr[3] = NULL;
// 3 nbg	
	SCL_SetConfig(SCL_NBG1, &scfg);
	SCL_SetCycleTable(CycleTb);	
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void initColors()
{
	memset(SclColRamAlloc256,0,sizeof(SclColRamAlloc256));
	colBgAddr2 = (Uint16*)SCL_AllocColRam(SCL_SPR,OFF);	
	colBgAddr  = (Uint16*)SCL_AllocColRam(SCL_NBG0,OFF);	  //ON
	SCL_SetColRamOffset(SCL_NBG2, 0,OFF);
	SCL_SetColRam(SCL_NBG1,8,8,palette);
//	colBgAddr2 = (Uint16*)SCL_AllocColRam(SCL_NBG2,OFF);//OFF);
//	(Uint16*)SCL_AllocColRam(SCL_NBG2,OFF);

}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void DrvInitSaturn()
{
	SPR_InitSlaveSH();
	nBurnSprites = 259; //256;
	nBurnLinescrollSize = 0;
	nSoundBufferPos = 0;

	SS_MAP  = ss_map   =(Uint16 *)(SCL_VDP2_VRAM_B0+0x10000);
	SS_MAP2 = ss_map2  = (Uint16 *)(SCL_VDP2_VRAM_B0+0x14000);
	SS_FONT = ss_font  = (Uint16 *)(SCL_VDP2_VRAM_B1);
	SS_CACHE= cache    =(Uint8  *)(SCL_VDP2_VRAM_A0);

	ss_BgPriNum     = (SclBgPriNumRegister *)SS_N0PRI;
	ss_SpPriNum     = (SclSpPriNumRegister *)SS_SPPRI;

	ss_sprite		= (SprSpCmd *)SS_SPRIT;
	ss_scl			= (Fixed32 *)SS_SCL;
//	ss_scl1			= (Fixed32 *)SS_SCL1;

	ss_regs->tvmode = 0x8011;
// 	SCL_SetWindow(SCL_W0,SCL_NBG0,SCL_NBG1,SCL_NBG1,16,0,240,256);
// 	SCL_SetWindow(SCL_W1,SCL_NBG1,SCL_NBG0,SCL_NBG0,16,0,240,256);

	SS_SET_N0PRIN(6);
	SS_SET_N1PRIN(7);
	SS_SET_N2PRIN(4);
	SS_SET_S0PRIN(5);

	initLayers();
	initColors();
	initSprites(240-1,256-1,0,0,0,0);
//	initScrolling(ON,SCL_VDP2_VRAM_B1);
//	memset((Uint8 *)ss_map, 0x11,0x4000);
//	memset((Uint8 *)ss_map2,0x11,0x4000);
//	memset((Uint8 *)ss_font,0x11,0x4000);
//	memset(&ss_scl[0],0,240);

	drawWindow(0,256,0,4,68);
	*(unsigned int*)OPEN_CSH_VAR(nSoundBufferPos) = 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static INT32 tigerhExit()
{
//	GenericTilesExit();

	CZetExit();
	AY8910Exit(0);
	AY8910Exit(1);

	// Deallocate all used memory
	BurnFree(Mem);
	
//	GetStarType = 0;

	return 0;
}

static void tigerhDoReset()
{
	bInterruptEnable = false;
	bSoundNMIEnable = false;
	bSoundCPUEnable = true;
	
	nStatusIndex = 0;
	nProtectIndex = 0;
	nPalettebank = 0;
	nFlipscreen = 0;

	CZetOpen(0);
	CZetReset();
	CZetClose();

	CZetOpen(1);
	CZetReset();
	CZetClose();

	return;
}

static INT32 tigerhInit()
{
	DrvInitSaturn();

	//FNT_Print256_2bpp((volatile unsigned char *)SS_FONT,(unsigned char *)"DrvInitSaturn",4,80);
	UINT8 *tmp0 = (UINT8*)0x00200000;
	sprintf(tmp0,"drvinitsaturn      ");

	INT32 nLen;

	nWhichGame = -1;

	if (strcmp(BurnDrvGetTextA(DRV_NAME), "tigerh") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "tigerhj") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "tigerhb1") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "tigerhb2") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "tigerhb3") == 0) {
		nWhichGame = 0;
	}

	if (strcmp(BurnDrvGetTextA(DRV_NAME), "alcon") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "slapfigh") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb1") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb2") == 0 || strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb3") == 0) {
		nWhichGame = 2;
	}

	// Find out how much memory is needed
	Mem = NULL;
	MemIndex();
	nLen = MemEnd - (UINT8*)0;
		sprintf(tmp0,"before malloc      ");

	if ((Mem = (UINT8*)BurnMalloc(nLen)) == NULL) 
	{
		sprintf(tmp0,"malloc failed      ");
		return 1;
	}
	memset(Mem, 0, nLen);										   	// blank all memory
	MemIndex();													   	// Index the allocated memory
		sprintf(tmp0,"malloc done      ");

	// Load the roms into memory
	if (tigerhLoadROMs()) 
	{
		sprintf(tmp0,"loadrom failed      ");
		return 1;
	}
	sprintf(tmp0,"loadrom done      ");
	
	if (!strcmp(BurnDrvGetTextA(DRV_NAME), "getstarb1")) Rom01[0x6d56] = 0xc3;

	{
		CZetInit(2);

		// Main CPU setup
		CZetOpen(0);

		// Program ROM
		CZetMapArea(0x0000, 0x7FFF, 0, Rom01);
		CZetMapArea(0x0000, 0x7FFF, 2, Rom01);
		// Banked ROM
		CZetMapArea(0x8000, 0xBFFF, 0, Rom01 + 0x8000);
		CZetMapArea(0x8000, 0xBFFF, 2, Rom01 + 0x8000);

		// Work RAM
		CZetMapArea(0xC000, 0xC7FF, 0, Ram01);
		CZetMapArea(0xC000, 0xC7FF, 1, Ram01);
		CZetMapArea(0xC000, 0xC7FF, 2, Ram01);

		// Shared RAM
		if (strcmp(BurnDrvGetTextA(DRV_NAME), "getstarb1")) {
			CZetMapArea(0xC800, 0xCFFF, 0, RamShared);
		}
		CZetMapArea(0xC800, 0xCFFF, 1, RamShared);
		CZetMapArea(0xC800, 0xCFFF, 2, RamShared);
		
		// Tile RAM
		CZetMapArea(0xD000, 0xDFFF, 0, TigerHeliTileRAM);
//		CZetMapArea(0xD000, 0xDFFF, 1, TigerHeliTileRAM);
		CZetMapArea(0xD000, 0xDFFF, 2, TigerHeliTileRAM);
		// Sprite RAM
		CZetMapArea(0xE000, 0xE7FF, 0, TigerHeliSpriteRAM);
		CZetMapArea(0xE000, 0xE7FF, 1, TigerHeliSpriteRAM);
		CZetMapArea(0xE000, 0xE7FF, 2, TigerHeliSpriteRAM);
		
		if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb2") || !strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb3")) {
			CZetMapArea(0xec00, 0xeFFF, 0, Rom01 + 0x10c00);
			CZetMapArea(0xec00, 0xeFFF, 2, Rom01 + 0x10c00);
		}
		
		// Text RAM
		CZetMapArea(0xF000, 0xFFFF, 0, TigerHeliTextRAM);
//		CZetMapArea(0xF000, 0xFFFF, 1, TigerHeliTextRAM);
		CZetMapArea(0xF000, 0xFFFF, 2, TigerHeliTextRAM);


		if (!strcmp(BurnDrvGetTextA(DRV_NAME), "tigerhb1")) {
			CZetSetReadHandler(tigerhReadCPU0_tigerhb1);
		} else {
			CZetSetReadHandler(tigerhReadCPU0);
		}
		
		if (!strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb2") || !strcmp(BurnDrvGetTextA(DRV_NAME), "slapfighb3")) {
			CZetSetWriteHandler(tigerhWriteCPU0_slapbtuk);
		} else {
			CZetSetWriteHandler(tigerhWriteCPU0);
		}
		
		if (!strcmp(BurnDrvGetTextA(DRV_NAME), "getstarb1")) {
			CZetSetInHandler(tigerhInCPU0_gtstarba);
		} else {
			CZetSetInHandler(tigerhInCPU0);
		}
		
		CZetSetOutHandler(tigerhOutCPU0);

		CZetClose();

//		CZetInit(1);

		// Sound CPU setup
		CZetOpen(1);

		// Program ROM
		CZetMapArea(0x0000, 0x1FFF, 0, Rom02);
		CZetMapArea(0x0000, 0x1FFF, 2, Rom02);

		// Work RAM
		CZetMapArea(0xC800, 0xCFFF, 0, RamShared);
		CZetMapArea(0xC800, 0xCFFF, 1, RamShared);
		CZetMapArea(0xC800, 0xCFFF, 2, RamShared);


		CZetSetReadHandler(tigerhReadCPU1);
		CZetSetWriteHandler(tigerhWriteCPU1);
		CZetSetInHandler(tigerhInCPU1);
		CZetSetOutHandler(tigerhOutCPU1);

		CZetClose();
	}

	pAY8910Buffer[0] = pFMBuffer + nBurnSoundLen * 0;
	pAY8910Buffer[1] = pFMBuffer + nBurnSoundLen * 1;
	pAY8910Buffer[2] = pFMBuffer + nBurnSoundLen * 2;
	pAY8910Buffer[3] = pFMBuffer + nBurnSoundLen * 3;
	pAY8910Buffer[4] = pFMBuffer + nBurnSoundLen * 4;
	pAY8910Buffer[5] = pFMBuffer + nBurnSoundLen * 5;

	AY8910Init(0, 1500000, nBurnSoundRate, &tigerhReadPort0, &tigerhReadPort1, NULL, NULL);
	AY8910Init(1, 1500000, nBurnSoundRate, &tigerhReadPort2, &tigerhReadPort3, NULL, NULL);
//	AY8910SetAllRoutes(0, 0.25, BURN_SND_ROUTE_BOTH);
//	AY8910SetAllRoutes(1, 0.25, BURN_SND_ROUTE_BOTH);
	sprintf(tmp0,"drvinit done      ");

	TigerHeliPaletteInit();
	sprintf(tmp0,"palette done      ");

//	GenericTilesInit();

	tigerhDoReset();

	return 0;
}

static void TigerHeliBufferSprites()
{
	memcpy(TigerHeliSpriteBuf, TigerHeliSpriteRAM, 0x0800);
}

static void draw_bg_layer()
{
	INT32 scrollx = (((nTigerHeliTileXPosHi * 256) + nTigerHeliTileXPosLo) + 8) & 0x1ff;
	INT32 scrolly = (nTigerHeliTileYPosLo + 15) & 0xff;

	for (INT32 offs = 0; offs < 64 * 32; offs++)
	{
		INT32 sx = (offs & 0x3f);// * 8;
		INT32 sy = (offs / 0x40);// * 8;

//		sx -= scrollx;
//		if (sx < -7) sx += 512;
//		sy -= scrolly;
//		if (sy < -7) sy += 256;

//		if (sy >= nScreenHeight || sx >= nScreenWidth) continue;

		INT32 attr  = TigerHeliTileRAM[offs] + (TigerHeliTileRAM[0x800 + offs] * 0x100);
		INT32 code  = (attr & 0x0fff) & nTigerHeliTileMask;
		INT32 color = (attr & 0xf000) >> 12;

		int x = (sx|(sy<<6))<<1;
		ss_map2[x] = color;
		ss_map2[x+1] = code+0x1000;
//		Render8x8Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, TigerHeliTileROM);
	}
}
/*
static void draw_txt_layer()
{
	for (INT32 offs = 0; offs < 64 * 32; offs++)
	{
		INT32 sx = (offs & 0x3f); // * 8); // - 8;
		INT32 sy = (offs / 0x40); // * 8); // - 15;

//		if (sy < -7 || sx < -7 || sy >= nScreenHeight || sx >= nScreenWidth) continue;

		INT32 attr  = TigerHeliTextRAM[offs] + (TigerHeliTextRAM[0x800 + offs] * 0x100);
		INT32 code  =  attr & 0x03ff;
		INT32 color = (attr & 0xfc00) >> 10;

//		Render8x8Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 2, 0, 0, TigerHeliTextROM);
		int x = (sx|(sy<<6))<<1;
		ss_map[x] = color & 0x3f;
		ss_map[x+1] = code;
	}
}
*/
static void draw_sprites()
{
	UINT8 *ram = TigerHeliSpriteBuf;
	UINT8 delta=3;
	for (INT32 i = 3; i < 131; i++)
	{
			ss_sprite[i].charAddr = 0;
			ss_sprite[i].charSize  = 0;
			ss_sprite[i].ax    = 0;
			ss_sprite[i].ay    =  0;
	}
//	for (INT32 offs = 0; offs < 0x800; offs += 4)
	for (INT32 offs = 0; offs < 0x800; offs += 4)
	{
/*			ss_sprite[delta].control   = ( JUMP_NEXT | FUNC_NORMALSP);
			ss_sprite[delta].charAddr = 0x440+((offs+64*4)<<2);
			ss_sprite[delta].charSize  = 0x210;
			ss_sprite[delta].ax    = ((offs/4) & 0x0f)*16;
			ss_sprite[delta].ay    =  ((offs/4) / 0x10)*16;
			delta++;  
*/
		if( (ram[offs + 3] - 15) > 0)
		{
			INT32 attr  =  ram[offs + 2];
			ss_sprite[delta].control   = ( JUMP_NEXT | FUNC_NORMALSP);
			UINT16 code  = (ram[offs + 0] | ((attr & 0xc0) << 2)) & 0x1ff; // nTigerHeliSpriteMask;
			ss_sprite[delta].charAddr = 0x440+(code<<4);
			ss_sprite[delta].charSize  = 0x210;
//			ss_sprite[delta].ax    = /*((offs/4) & 0x0f)*16; */(ram[offs + 1] | (attr << 8 & 0x100)) - (13 + 8);
			ss_sprite[delta].ax    = /*((offs/4) & 0x0f)*16; */(ram[offs + 1] | (attr << 8 & 0x100)) - (13);
			ss_sprite[delta].ay    =  /*((offs/4) / 0x10)*16; */ram[offs + 3] - 15;
			ss_sprite[delta].color=  (attr >> 1 & 0xf)<<4;
			delta++;
		}	   

//		Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0, 0, TigerHeliSpriteROM);
	}
}

static inline INT32 CheckSleep(INT32 duration)
{
	return 0;
}

static INT32 tigerhFrame()
{
	INT32 nCyclesTotal[3], nCyclesDone[3];
/*
	if (tigerhReset) {													// Reset machine
		tigerhDoReset();
	}
*/
	UINT8 *tmp0 = (UINT8*)0x00200000;

//	sprintf(tmp0,"frame start      ");

	CZetNewFrame();

	// Compile digital inputs
	tigerhInput[0] = 0x00;
	tigerhInput[1] = 0x00;
	for (INT32 i = 0; i < 8; i++) {
		tigerhInput[0] |= (tigerhInpJoy1[i] & 1) << i;
		if (nWhichGame == 0 && i < 4) {
			tigerhInput[1] |= (tigerhInpMisc[i] & 1) << (i ^ 1);
		} else {
			tigerhInput[1] |= (tigerhInpMisc[i] & 1) << i;
		}
	}

	if ((tigerhInput[0] & 0x03) == 0x03) {
		tigerhInput[0] &= ~0x03;
	}
	if ((tigerhInput[0] & 0x0C) == 0x0C) {
		tigerhInput[0] &= ~0x0C;
	}
	if ((tigerhInput[0] & 0x30) == 0x30) {
		tigerhInput[0] &= ~0x30;
	}
	if ((tigerhInput[0] & 0xC0) == 0xC0) {
		tigerhInput[0] &= ~0xC0;
	}

	if (nWhichGame == 1) {
		tigerhInput[0] = (tigerhInput[0] & 0x99) | ((tigerhInput[0] << 1) & 0x44) | ((tigerhInput[0] >> 1) & 0x22);
	}

	nCyclesTotal[0] = 6000000 / 60;
	nCyclesTotal[1] = 3000000 / 60;

	nCyclesDone[0] = nCyclesDone[1] = nCyclesDone[2] = 0;
	nCyclesTotal[2] = 3000000 / 60;

	INT32 nVBlankCycles = 248 * 6000000 / 60 / 262;
	const INT32 nInterleave = 12;
/*
	if (nWhichGame == 9)
	{
		nCyclesTotal[0] = 4000000 / 60;
		nCyclesTotal[1] = 2000000 / 60;
		nVBlankCycles = 248 * 4000000 / 60 / 262;
	}
*/
	INT32 nSoundBufferPos = 0;
	INT32 nSoundNMIMask = 0;
	switch (nWhichGame) {
		case 0:
			nSoundNMIMask = 1;
			break;
		case 1:
			nSoundNMIMask = 3;
			break;
		case 2:
			nSoundNMIMask = 3;
			break;
/*
		case 9:
			nSoundNMIMask = 3; //??
			break;			*/
	}

	bVBlank = false;

	for (INT32 i = 0; i < nInterleave; i++) {
    	INT32 nCurrentCPU;
		INT32 nNext, nCyclesSegment;

		nCurrentCPU = 0;
		CZetOpen(nCurrentCPU);

		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
// 	sprintf(tmp0,"cpu0run      ");

		if (nNext > nVBlankCycles && !bVBlank) {
			nCyclesDone[nCurrentCPU] += CZetRun(nNext - nVBlankCycles);

//			if (pBurnDraw != NULL) {
//				BurnDrvRedraw();											// Draw screen if needed
//			}
// 	sprintf(tmp0,"sprite run      ");
//				draw_bg_layer();
//				draw_txt_layer();

			TigerHeliBufferSprites();
			bVBlank = true;

			if (bInterruptEnable) {
				CZetSetIRQLine(0xff, CZET_IRQSTATUS_AUTO);
			}
		}

		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];
		if (bVBlank || (!CheckSleep(nCurrentCPU))) {					// See if this CPU is busywaiting
			nCyclesDone[nCurrentCPU] += CZetRun(nCyclesSegment);
		} else {
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
		}

		CZetClose();

		nCurrentCPU = 1;
		nNext = (i + 1) * nCyclesTotal[nCurrentCPU] / nInterleave;
		nCyclesSegment = nNext - nCyclesDone[nCurrentCPU];

		if (bSoundCPUEnable) {
			CZetOpen(nCurrentCPU);

			if ((i & nSoundNMIMask) == 0) {
				if (bSoundNMIEnable) {
					CZetNmi();
				}
			}
// 	sprintf(tmp0,"cpu1run      ");

			nCyclesDone[nCurrentCPU] += CZetRun(nCyclesSegment);
			CZetClose();
		} else {
			nCyclesDone[nCurrentCPU] += nCyclesSegment;
		}

		{
			// Render sound segment
//			if (pBurnSoundOut) 
			{
				INT32 nSegmentLength = nBurnSoundLen / nInterleave;
//				INT16* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
//				AY8910Render(&pAY8910Buffer[0], pSoundBuf, nSegmentLength, 0);
				nSoundBufferPos += nSegmentLength;
			}
		}

	}

	{
		// Make sure the buffer is entirely filled.
//		if (pBurnSoundOut) 
		{
			INT32 nSegmentLength = nBurnSoundLen - nSoundBufferPos;
//			INT16* pSoundBuf = pBurnSoundOut + (nSoundBufferPos << 1);
			if (nSegmentLength) 
			{
//				AY8910Render(&pAY8910Buffer[0], pSoundBuf, nSegmentLength, 0);
			}
		}
	}
//	sprintf(tmp0,"frame done      ");
	draw_sprites();
//	INT32 scrollx = (((nTigerHeliTileXPosHi * 256) + nTigerHeliTileXPosLo) + 8 -16) & 0x1ff;
	INT32 scrollx = (((nTigerHeliTileXPosHi * 256) + nTigerHeliTileXPosLo)) & 0x1ff;
	INT32 scrolly = (nTigerHeliTileYPosLo + 15) & 0xff;
	ss_reg->n2_move_x = scrollx;
//	
	return 0;
}
