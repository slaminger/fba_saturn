// FB Alpha UPL (Ninja Kid 2) driver module
// Based on MAME driver by Roberto Ventura, Leandro Dardini, Yochizo, Nicola Salmoria

//#include "tiles_generic.h"
//#include "z80_intf.h"
#include "d_ninjakd2.h"
#include "mc8123.h"
//#include "burn_ym2203.h"
#define nScreenHeight 192
#define nScreenWidth 256

int ovlInit(char *szShortName)
{
	struct BurnDriver nBurnDrvRobokid = {
		"robokid", "ninkd2",
		"Atomic Robo-kid (World, Type-2)", 
		robokidRomInfo, robokidRomName, DrvInputInfo, RobokidDIPInfo,
		RobokidInit, DrvExit, DrvFrame, NULL
	};

	if (strcmp(nBurnDrvRobokid.szShortName, szShortName) == 0) 
	memcpy(shared,&nBurnDrvRobokid,sizeof(struct BurnDriver));

	ss_reg   = (SclNorscl *)SS_REG;
	ss_regs = (SclSysreg *)SS_REGS;
	ss_regd = (SclDataset *)SS_REGD;
}

/*static*/  void DrvPaletteUpdate(INT32 offset)
{
	offset &= 0x7fe;

	INT32 p = (DrvPalRAM[offset+0] * 256) + DrvPalRAM[offset+1];

	INT32 r = p >> 12;
	INT32 g = (p >> 8) & 0xf;
	INT32 b = (p >> 4) & 0xf;

	r |= r << 4;
	g |= g << 4;
	b |= b << 4;

	DrvPalette[offset/2] = BurnHighCol(r,g,b,0);
}

/*static*/  void ninjakd2_bankswitch(INT32 data)
{
	INT32 nBank = 0x10000 + (data * 0x4000);

	nZ80RomBank = data;

	CZetMapMemory(DrvZ80ROM0 + nBank, 	0x8000, 0xbfff, MAP_ROM);
}

/*static*/  void ninjakd2_bgconfig(INT32 sel, INT32 offset, UINT8 data)
{
	switch (offset & 0x07)
	{
		case 0:
			scrollx[sel] = (scrollx[sel] & 0x700) + data;
		return;

		case 1:
			scrollx[sel] = (scrollx[sel] & 0x0ff) + ((data & 0x07) * 256);
		return;

		case 2:
			scrolly[sel] = (scrolly[sel] & 0x100) + data;
		return;

		case 3:
			scrolly[sel] = (scrolly[sel] & 0x0ff) + ((data & 0x01) * 256);
		return;

		case 4:
			tilemap_enable[sel] = data & 0x01;
		return;
	}
}

/*static*/  UINT8 __fastcall ninjakd2_main_read(UINT16 address)
{
	switch (address)
	{
		case 0xc000:
		case 0xc001:
		case 0xc002:
		case 0xdc00:
		case 0xdc01:
		case 0xdc02:
		case 0xf800:
		case 0xf801:
		case 0xf802:
			return DrvInputs[address & 3];

		case 0xc003:
		case 0xc004:
		case 0xdc03:
		case 0xdc04:
		case 0xf803:
		case 0xf804:
			return DrvDips[(address & 7) - 3];
	}

	return 0;
}

/*static*/  void __fastcall ninjakd2_main_write(UINT16 address, UINT8 data)
{
	if ((address & 0xf800) == 0xc800) {
		DrvPalRAM[address & 0x7ff] = data;
		DrvPaletteUpdate(address);
		return;
	}

	switch (address)
	{
		case 0xc200:
			*soundlatch = data;
		return;

		case 0xc201:
		{
			if (data & 0x10)
			{
				CZetClose();
				CZetOpen(1);
				CZetReset();
				CZetClose();
				CZetOpen(0);
			}

			*flipscreen = data & 0x80;
		}
		return;

		case 0xc202:
			ninjakd2_bankswitch(data & 0x07);
		return;

		case 0xc203:
			overdraw_enable = data & 0x01;
		return;

		case 0xc208:
		case 0xc209:
		case 0xc20a:
		case 0xc20b:
		case 0xc20c:
			ninjakd2_bgconfig(0, address, data);
		return;
	}
}

/*static*/  void __fastcall mnight_main_write(UINT16 address, UINT8 data)
{
	if ((address & 0xf800) == 0xf000) {
		DrvPalRAM[address & 0x7ff] = data;
		DrvPaletteUpdate(address);
		return;
	}

	switch (address)
	{
		case 0xfa00:
			*soundlatch = data;
		return;

		case 0xfa01:
		{
			if (data & 0x10)
			{
				CZetClose();
				CZetOpen(1);
				CZetReset();
				CZetClose();
				CZetOpen(0);
			}

			*flipscreen = data & 0x80;
		}
		return;

		case 0xfa02:
			ninjakd2_bankswitch(data & 0x07);
		return;

		case 0xfa03:
			overdraw_enable = data & 0x01;
		return;

		case 0xfa08:
		case 0xfa09:
		case 0xfa0a:
		case 0xfa0b:
		case 0xfa0c:
			ninjakd2_bgconfig(0, address, data);
		return;
	}
}

/*static*/  void robokid_rambank(INT32 sel, UINT8 data)
{
	UINT8 *ram[3] = { DrvBgRAM0, DrvBgRAM1, DrvBgRAM2 };
	INT32 off[2][3]  = { { 0xd800, 0xd400, 0xd000 }, { 0xc400, 0xc800, 0xcc00 } };

	INT32 nBank = 0x400 * data;

	nZ80RamBank[sel&3] = data;

	CZetMapMemory(ram[sel&3] + nBank, off[sel>>2][sel&3], off[sel>>2][sel&3] | 0x3ff, MAP_RAM);
}

/*static*/  void __fastcall robokid_main_write(UINT16 address, UINT8 data)
{
	if ((address & 0xf800) == 0xc000) {
		DrvPalRAM[address & 0x7ff] = data;
		DrvPaletteUpdate(address);
		return;
	}

	switch (address)
	{
		case 0xdc00:
			*soundlatch = data;
		return;

		case 0xdc01:
		{
			if (data & 0x10) {
				CZetClose();
				CZetOpen(1);
				CZetReset();
				CZetClose();
				CZetOpen(0);
			}

			*flipscreen = data & 0x80;
		}
		return;

		case 0xdc02:
			ninjakd2_bankswitch(data & 0x0f);
		return;

		case 0xdc03:
			overdraw_enable = data & 0x01;
		return;

		case 0xdd00:
		case 0xdd01:
		case 0xdd02:
		case 0xdd03:
		case 0xdd04:
			ninjakd2_bgconfig(0, address, data);
		return;

		case 0xdd05:
			robokid_rambank(0, data & 1);
		return;

		case 0xde00:
		case 0xde01:
		case 0xde02:
		case 0xde03:
		case 0xde04:
			ninjakd2_bgconfig(1, address, data);
		return;

		case 0xde05:
			robokid_rambank(1, data & 1);
		return;

		case 0xdf00:
		case 0xdf01:
		case 0xdf02:
		case 0xdf03:
		case 0xdf04:
			ninjakd2_bgconfig(2, address, data);
		return;

		case 0xdf05:
			robokid_rambank(2, data & 1);
		return;
	}
}

// Copied directly from MAME
/*static*/  UINT8 omegaf_protection_read(INT32 offset)
{
	UINT8 result = 0xff;

	switch (m_omegaf_io_protection[1] & 3)
	{
		case 0:
			switch (offset)
			{
				case 1:
					switch (m_omegaf_io_protection[0] & 0xe0)
					{
						case 0x00:
							if (++m_omegaf_io_protection_tic & 1)
							{
								result = 0x00;
							}
							else
							{
								switch (m_omegaf_io_protection_input)
								{
									// first interrogation
									// this happens just after setting mode 0.
									// input is not explicitly loaded so could be anything
									case 0x00:
										result = 0x80 | 0x02;
										break;

									// second interrogation
									case 0x8c:
										result = 0x80 | 0x1f;
										break;

									// third interrogation
									case 0x89:
										result = 0x80 | 0x0b;
										break;
								}
							}
							break;

						case 0x20:
							result = 0xc7;
							break;

						case 0x60:
							result = 0x00;
							break;

						case 0x80:
							result = 0x20 | (m_omegaf_io_protection_input & 0x1f);
							break;

						case 0xc0:
							result = 0x60 | (m_omegaf_io_protection_input & 0x1f);
							break;
					}
					break;
			}
			break;

		case 1: // dip switches
			switch (offset)
			{
				case 0: result = DrvDips[0]; break;
				case 1: result = DrvDips[1]; break;
				case 2: result = 0x02; break;
			}
			break;

		case 2: // player inputs
			switch (offset)
			{
				case 0: result = DrvInputs[1]; break;
				case 1: result = DrvInputs[2]; break;
				case 2: result = 0x01; break;
			}
			break;
	}

	return result;
}

/*static*/  UINT8 __fastcall omegaf_main_read(UINT16 address)
{
	switch (address)
	{
		case 0xc000:
			return DrvInputs[0];

		case 0xc001:
		case 0xc002:
		case 0xc003:
			return omegaf_protection_read(address - 0xc001);
	}

	return 0;
}

/*static*/  void __fastcall omegaf_main_write(UINT16 address, UINT8 data)
{
	if ((address & 0xf800) == 0xd800) {
		DrvPalRAM[address & 0x7ff] = data;
		DrvPaletteUpdate(address);
		return;
	}

	switch (address)
	{
		case 0xc000:
			*soundlatch = data;
		return;

		case 0xc001:
		{
			if (data & 0x10) {
				CZetClose();
				CZetOpen(1);
				CZetReset();
				CZetClose();
				CZetOpen(0);
			}

			*flipscreen = data & 0x80;
		}
		return;

		case 0xc002:
			ninjakd2_bankswitch(data & 0x0f);
		return;

		case 0xc003:
			overdraw_enable = data & 0x01;
		return;

		case 0xc004:
		case 0xc005:
		case 0xc006:
		{
			if (address == 0xc006 && (data & 1) && !(m_omegaf_io_protection[2] & 1))
			{
				m_omegaf_io_protection_input = m_omegaf_io_protection[0];
			}
		
			m_omegaf_io_protection[address - 0xc004] = data;
		}
		return;

		case 0xc100:
		case 0xc101:
		case 0xc102:
		case 0xc103:
		case 0xc104:
			ninjakd2_bgconfig(0, address, data);
		return;

		case 0xc105:
			robokid_rambank(4|0, data & 7);
		return;

		case 0xc200:
		case 0xc201:
		case 0xc202:
		case 0xc203:
		case 0xc204:
			ninjakd2_bgconfig(1, address, data);
		return;

		case 0xc205:
			robokid_rambank(4|1, data & 7);
		return;

		case 0xc300:
		case 0xc301:
		case 0xc302:
		case 0xc303:
		case 0xc304:
			ninjakd2_bgconfig(2, address, data);
		return;

		case 0xc305:
			robokid_rambank(4|2, data & 7);
		return;
	}
}

/*static*/  void ninjakd2_sample_player(INT16 *dest, INT32 len)
{
	if (ninjakd2_sample_offset == -1) return;

	for (INT32 i = 0; i < len; i++)
	{
		UINT16 ofst =  ninjakd2_sample_offset + ((i * 271) / len);

		if (DrvSndROM[ofst] == 0) {
			ninjakd2_sample_offset = -1;
			break;
		}

		INT32 sample = BURN_SND_CLIP(((DrvSndROM[ofst]<<7) * 45) / 100);

		dest[i*2+0] = BURN_SND_CLIP(dest[i*2+0]+sample);
		dest[i*2+1] = BURN_SND_CLIP(dest[i*2+1]+sample);
	}

	if (ninjakd2_sample_offset != -1)
		ninjakd2_sample_offset += 271;
}

/*static*/  void __fastcall ninjakd2_sound_write(UINT16 address, UINT8 data)
{
	data = data;

	switch (address)
	{
		case 0xf000:
			ninjakd2_sample_offset = data << 8;
		return;
	}
}

/*static*/  UINT8 __fastcall ninjakd2_sound_read(UINT16 address)
{
	switch (address)
	{
		case 0xe000:
			return *soundlatch;
	}

	return 0;
}

/*static*/  void __fastcall ninjakd2_sound_write_port(UINT16 port, UINT8 data)
{
	switch (port & 0xff)
	{
		case 0x00:
		case 0x01:
		case 0x80:
		case 0x81:
//			BurnYM2203Write((port >> 7) & 1, port & 1, data);
		return;
	}
}
/*
inline  void DrvYM2203IRQHandler(INT32, INT32 nStatus)
{
	ZetSetIRQLine(0, (nStatus) ? CPU_IRQSTATUS_ACK : CPU_IRQSTATUS_NONE);
}
*/
/*static*/  void ninjakd2_sound_init()
{
	CZetInit(1);
	CZetOpen(1);

//	CZetMapMemory(DrvZ80ROM1, 0x0000, 0xbfff, MAP_ROM);

	CZetMapArea(0x0000, 0xbfff, 0, DrvZ80ROM1);
	CZetMapArea2(0x0000, 0xbfff, 2, DrvZ80ROM1 + 0x10000, DrvZ80ROM1);

	CZetMapMemory(DrvZ80RAM1,		0xc000, 0xc7ff, MAP_RAM);
	CZetSetOutHandler(ninjakd2_sound_write_port);
	CZetSetWriteHandler(ninjakd2_sound_write);
	CZetSetReadHandler(ninjakd2_sound_read);
	CZetClose();

/*
	BurnYM2203Init(2,  1500000, &DrvYM2203IRQHandler, 0);
	BurnTimerAttachZet(5000000);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_YM2203_ROUTE,   0.50, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_1, 0.10, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_2, 0.10, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(0, BURN_SND_YM2203_AY8910_ROUTE_3, 0.10, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_YM2203_ROUTE,   0.50, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_1, 0.10, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_2, 0.10, BURN_SND_ROUTE_BOTH);
	BurnYM2203SetRoute(1, BURN_SND_YM2203_AY8910_ROUTE_3, 0.10, BURN_SND_ROUTE_BOTH);
*/
}

/*static*/  INT32 DrvDoReset()
{
	memset(AllRam, 0, RamEnd - AllRam);

	CZetOpen(0);
	CZetReset();
	CZetClose();

	CZetOpen(1);
	CZetReset();
//	BurnYM2203Reset();
	CZetClose();

	memset (scrollx, 0, 3 * sizeof(UINT16));
	memset (scrolly, 0, 3 * sizeof(UINT16));

	nZ80RomBank = 0;
	memset (nZ80RamBank, 0, 3);

	overdraw_enable = 0;
	memset (tilemap_enable, 0, 3);

	memset (m_omegaf_io_protection, 0, 3);
	m_omegaf_io_protection_input = 0;
	m_omegaf_io_protection_tic = 0;

	ninjakd2_sample_offset = -1;

	previous_coin[0] = previous_coin[1] = 0;

//	HiscoreReset();

	return 0;
}

/*static*/  INT32 MemIndex()
{
	UINT8 *Next; Next = AllMem;

	DrvZ80ROM0	= Next; Next += 0x050000;
	DrvZ80ROM1	= Next; Next += 0x020000;

	DrvGfxROM0	= Next; Next += 0x010000;
/*	DrvGfxROM1	= Next; Next += 0x080000;
	DrvGfxROM2	= Next; Next += 0x100000;
	DrvGfxROM3	= Next; Next += 0x100000;
	DrvGfxROM4	= Next; Next += 0x100000;
*/
	DrvZ80Key	= Next; Next += 0x002000;

	DrvSndROM	= Next; Next += 0x010000;

	DrvPalette	= (UINT32*)Next; Next += 0x0400 * sizeof(UINT32);

	AllRam		= Next;

	DrvZ80RAM0	= Next; Next += 0x001a00;
	DrvZ80RAM1	= Next; Next += 0x000800;
	DrvSprRAM	= Next; Next += 0x000600;
	DrvPalRAM	= Next; Next += 0x000800;
	DrvFgRAM	= Next; Next += 0x000800;
	DrvBgRAM0	= Next;
	DrvBgRAM	= Next; Next += 0x002000;
	DrvBgRAM1	= Next; Next += 0x002000;
	DrvBgRAM2	= Next; Next += 0x002000;

	soundlatch	= Next; Next += 0x000001;
	flipscreen	= Next; Next += 0x000001;

	pSpriteDraw	= (UINT16*)Next; Next += 256 * 256 * sizeof(UINT16);

	RamEnd		= Next;

	MemEnd		= Next;

	return 0;
}

/*static*/  INT32 DrvGfxDecode(UINT8 *rom, INT32 len, INT32 type)
{
	INT32 Plane[4]   = { STEP4(0,1) };
	INT32 XOffs0[16] = { STEP8(0,4), STEP8(32*8,4) };
	INT32 XOffs1[16] = { STEP8(0,4), STEP8(64*8,4) };
	INT32 YOffs0[16] = { STEP8(0,32), STEP8(64*8,32) };
	INT32 YOffs1[16] = { STEP16(0,32) };

	UINT8 *tmp = (UINT8*)BurnMalloc(len);
	if (tmp == NULL) {
		return 1;
	}

	memcpy (tmp, rom, len);

	switch (type)
	{
		case 0:
			GfxDecode4Bpp((len * 2) / ( 8 *  8), 4,  8,  8, Plane, XOffs0, YOffs0, 0x100, tmp, rom);
		break;

		case 1:
			GfxDecode4Bpp((len * 2) / (16 * 16), 4, 16, 16, Plane, XOffs0, YOffs0, 0x400, tmp, rom);
		break;

		case 2:
			GfxDecode4Bpp((len * 2) / (16 * 16), 4, 16, 16, Plane, XOffs1, YOffs1, 0x400, tmp, rom);
		break;
	}

	BurnFree (tmp);

	return 0;
}

/*static*/  void lineswap_gfx_roms(UINT8 *rom, INT32 len, const INT32 bit)
{
	UINT8 *tmp = (UINT8*)BurnMalloc(len);

	const INT32 mask = (1 << (bit + 1)) - 1;

	for (INT32 sa = 0; sa < len; sa++)
	{
		const INT32 da = (sa & ~mask) | ((sa << 1) & mask) | ((sa >> bit) & 1);
		tmp[da] = rom[sa];
	}

	memcpy (rom, tmp, len);

	BurnFree (tmp);
}

/*static*/  void gfx_unscramble(INT32 gfxlen)
{
	lineswap_gfx_roms(DrvGfxROM0, 0x08000, 13);
	lineswap_gfx_roms(DrvGfxROM1, gfxlen, 14);
	lineswap_gfx_roms(DrvGfxROM2, gfxlen, 14);
}

/*static*/  INT32 Ninjakd2CommonInit()
{
	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(DrvZ80ROM0 + 0x00000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x10000,  1, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x18000,  2, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x20000,  3, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x28000,  4, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM1 + 0x00000,  5, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  6, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  7, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x10000,  8, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x00000,  9, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x10000, 10, 1)) return 1;

		if (BurnLoadRom(DrvSndROM  + 0x00000, 11, 1)) return 1;

		gfx_unscramble(0x20000);
		DrvGfxDecode(DrvGfxROM0, 0x08000, 0);
		DrvGfxDecode(DrvGfxROM1, 0x20000, 1);
		DrvGfxDecode(DrvGfxROM2, 0x20000, 1);
	}

	CZetInit(1);
	CZetOpen(0);
	CZetMapMemory(DrvZ80ROM0,		0x0000, 0x7fff, MAP_ROM);
	CZetMapMemory(DrvZ80ROM0 + 0x10000, 	0x8000, 0xbfff, MAP_ROM);
	CZetMapMemory(DrvPalRAM,			0xc800, 0xcdff, MAP_ROM);
	CZetMapMemory(DrvFgRAM,			0xd000, 0xd7ff, MAP_RAM);
	CZetMapMemory(DrvBgRAM,			0xd800, 0xdfff, MAP_RAM);
	CZetMapMemory(DrvZ80RAM0,		0xe000, 0xf9ff, MAP_RAM);
	CZetMapMemory(DrvSprRAM,			0xfa00, 0xffff, MAP_RAM);
	CZetSetWriteHandler(ninjakd2_main_write);
	CZetSetReadHandler(ninjakd2_main_read);
	CZetClose();

//	ninjakd2_sound_init();

//	GenericTilesInit();

	DrvDoReset();

	return 0;
}

/*static*/  INT32 Ninjakd2Init()
{
	DrvInitSaturn();

	INT32 nRet = Ninjakd2CommonInit();

	if (nRet == 0)
	{
		if (BurnLoadRom(DrvZ80Key  + 0x00000, 12, 1)) return 1;

		mc8123_decrypt_rom(0, 0, DrvZ80ROM1, DrvZ80ROM1 + 0x10000, DrvZ80Key);
	}

	return nRet;
}

/*static*/  INT32 Ninjakd2DecryptedInit()
{
	DrvInitSaturn();

	INT32 nRet = Ninjakd2CommonInit();

	if (nRet == 0)
	{
		memcpy (DrvZ80ROM1 + 0x10000, DrvZ80ROM1, 0x10000);
		memcpy (DrvZ80ROM1, DrvZ80ROM1 + 0x08000, 0x08000);
	}

	return nRet;
}

/*static*/  INT32 MnightInit()
{
	DrvInitSaturn();

	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(DrvZ80ROM0 + 0x00000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x10000,  1, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x18000,  2, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x20000,  3, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x28000,  4, 1)) return 1;

		if (BurnLoadRom(DrvZ80ROM1 + 0x00000,  5, 1)) return 1;
		memcpy (DrvZ80ROM1 + 0x10000, DrvZ80ROM1, 0x10000);

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  6, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  7, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x10000,  8, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x20000,  9, 1)) return 1;
		memcpy (DrvGfxROM1 + 0x30000, DrvGfxROM1 + 0x20000, 0x10000);

		if (BurnLoadRom(DrvGfxROM2 + 0x00000, 10, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x10000, 11, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x20000, 12, 1)) return 1;
		memcpy (DrvGfxROM1 + 0x30000, DrvGfxROM1 + 0x20000, 0x10000);

		gfx_unscramble(0x40000);
		DrvGfxDecode(DrvGfxROM0, 0x08000, 0);
		DrvGfxDecode(DrvGfxROM1, 0x40000, 1);
		DrvGfxDecode(DrvGfxROM2, 0x40000, 1);
	}

	CZetInit(1);
	CZetOpen(0);
	CZetMapMemory(DrvZ80ROM0,		0x0000, 0x7fff, MAP_ROM);
	CZetMapMemory(DrvZ80ROM0 + 0x10000, 	0x8000, 0xbfff, MAP_ROM);
	CZetMapMemory(DrvZ80RAM0,		0xc000, 0xd9ff, MAP_RAM);
	CZetMapMemory(DrvSprRAM,			0xda00, 0xdfff, MAP_RAM);
	CZetMapMemory(DrvBgRAM,			0xe000, 0xe7ff, MAP_RAM);
	CZetMapMemory(DrvFgRAM,			0xe800, 0xefff, MAP_RAM);
	CZetMapMemory(DrvPalRAM,			0xf000, 0xf5ff, MAP_ROM);
	CZetSetWriteHandler(mnight_main_write);
	CZetSetReadHandler(ninjakd2_main_read);
	CZetClose();

//	ninjakd2_sound_init();
//	BurnYM2203SetPSGVolume(0, 0.05);
//	BurnYM2203SetPSGVolume(1, 0.05);

//	GenericTilesInit();

	DrvDoReset();

	return 0;
}

/*static*/  INT32 RobokidInit()
{
	DrvInitSaturn();
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)" DrvInitSaturn     ",20,60);

	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8 *)0;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"bef BurnMalloc    ",20,60);

	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"aft BurnMalloc    ",20,60);

	{
		if (BurnLoadRom(DrvZ80ROM0 + 0x10000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x20000,  1, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x30000,  2, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x40000,  3, 1)) return 1;
		memcpy (DrvZ80ROM0, DrvZ80ROM0 + 0x10000, 0x10000);

		if (BurnLoadRom(DrvZ80ROM1 + 0x10000,  4, 1)) return 1;
		memcpy (DrvZ80ROM1, DrvZ80ROM1 + 0x10000, 0x10000);

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  5, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  6, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x10000,  7, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x20000,  8, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM1 + 0x30000,  9, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x00000, 10, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x10000, 11, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x20000, 12, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x30000, 13, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x40000, 14, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x50000, 15, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM2 + 0x60000, 16, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM3 + 0x00000, 17, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x10000, 18, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x20000, 19, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x30000, 20, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x40000, 21, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x50000, 22, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x60000, 23, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM3 + 0x70000, 24, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM4 + 0x00000, 25, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM4 + 0x10000, 26, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM4 + 0x20000, 27, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM4 + 0x30000, 28, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM4 + 0x40000, 29, 1)) return 1;
		if (BurnLoadRom(DrvGfxROM4 + 0x50000, 30, 1)) return 1;

		DrvGfxDecode(DrvGfxROM0, 0x08000, 0);
		DrvGfxDecode(DrvGfxROM1, 0x40000, 2);
		DrvGfxDecode(DrvGfxROM2, 0x80000, 2);
		DrvGfxDecode(DrvGfxROM3, 0x80000, 2);
		DrvGfxDecode(DrvGfxROM4, 0x80000, 2);
	}

	CZetInit(1);
	CZetOpen(0);
	CZetMapMemory(DrvZ80ROM0,		0x0000, 0x7fff, MAP_ROM);
	CZetMapMemory(DrvZ80ROM0 + 0x10000, 	0x8000, 0xbfff, MAP_ROM);
	CZetMapMemory(DrvPalRAM,			0xc000, 0xc7ff, MAP_ROM);
	CZetMapMemory(DrvFgRAM,			0xc800, 0xcfff, MAP_RAM);
	CZetMapMemory(DrvBgRAM2,			0xd000, 0xd3ff, MAP_RAM);
	CZetMapMemory(DrvBgRAM1,			0xd400, 0xd7ff, MAP_RAM);
	CZetMapMemory(DrvBgRAM0,			0xd800, 0xdbff, MAP_RAM);
	CZetMapMemory(DrvZ80RAM0,		0xe000, 0xf9ff, MAP_RAM);
	CZetMapMemory(DrvSprRAM,			0xfa00, 0xffff, MAP_RAM);
	CZetSetWriteHandler(robokid_main_write);
	CZetSetReadHandler(ninjakd2_main_read);
	CZetClose();

//	ninjakd2_sound_init();
//	BurnYM2203SetPSGVolume(0, 0.03);
//	BurnYM2203SetPSGVolume(1, 0.03);

//	GenericTilesInit();

	DrvDoReset();

	return 0;
}

/*static*/  INT32 OmegafInit()
{
	DrvInitSaturn();

	AllMem = NULL;
	MemIndex();
	INT32 nLen = MemEnd - (UINT8 *)0;
	if ((AllMem = (UINT8 *)BurnMalloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	{
		if (BurnLoadRom(DrvZ80ROM0 + 0x10000,  0, 1)) return 1;
		if (BurnLoadRom(DrvZ80ROM0 + 0x30000,  1, 1)) return 1;
		memcpy (DrvZ80ROM0, DrvZ80ROM0 + 0x10000, 0x10000);

		if (BurnLoadRom(DrvZ80ROM1 + 0x10000,  2, 1)) return 1;
		memcpy (DrvZ80ROM1, DrvZ80ROM1 + 0x10000, 0x10000);

		if (BurnLoadRom(DrvGfxROM0 + 0x00000,  3, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM1 + 0x00000,  4, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM2 + 0x00000,  5, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM3 + 0x00000,  6, 1)) return 1;

		if (BurnLoadRom(DrvGfxROM4 + 0x00000,  7, 1)) return 1;

		DrvGfxDecode(DrvGfxROM0, 0x08000, 0);
		DrvGfxDecode(DrvGfxROM1, 0x20000, 2);
		DrvGfxDecode(DrvGfxROM2, 0x80000, 2);
		DrvGfxDecode(DrvGfxROM3, 0x80000, 2);
		DrvGfxDecode(DrvGfxROM4, 0x80000, 2);
	}

	CZetInit(1);
	CZetOpen(0);
	CZetMapMemory(DrvZ80ROM0,		0x0000, 0x7fff, MAP_ROM);
	CZetMapMemory(DrvZ80ROM0 + 0x10000, 	0x8000, 0xbfff, MAP_ROM);
	CZetMapMemory(DrvBgRAM0,			0xc400, 0xc7ff, MAP_RAM);
	CZetMapMemory(DrvBgRAM1,			0xc800, 0xcbff, MAP_RAM);
	CZetMapMemory(DrvBgRAM2,			0xcc00, 0xcfff, MAP_RAM);
	CZetMapMemory(DrvFgRAM,			0xd000, 0xd7ff, MAP_RAM);
	CZetMapMemory(DrvPalRAM,			0xd800, 0xdfff, MAP_ROM);
	CZetMapMemory(DrvZ80RAM0,		0xe000, 0xf9ff, MAP_RAM);
	CZetMapMemory(DrvSprRAM,			0xfa00, 0xffff, MAP_RAM);
	CZetSetWriteHandler(omegaf_main_write);
	CZetSetReadHandler(omegaf_main_read);
	CZetClose();

//	ninjakd2_sound_init();
//	BurnYM2203SetRoute(0, BURN_SND_YM2203_YM2203_ROUTE,   0.80, BURN_SND_ROUTE_BOTH);
//	BurnYM2203SetRoute(1, BURN_SND_YM2203_YM2203_ROUTE,   0.80, BURN_SND_ROUTE_BOTH);

//	GenericTilesInit();

	DrvDoReset();

	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void initLayers()
{
    Uint16	CycleTb[]={
		0x4eff, 0x1fff, //A0
		0xffff, 0xffff,	//A1
		0x2f6f,0xf5ff,   //B0
		0xffff, 0xffff  //B1
	};
 	SclConfig	scfg;

	scfg.dispenbl      = ON;
	scfg.charsize      = SCL_CHAR_SIZE_1X1;//OK du 1*1 surtout pas toucher
	scfg.pnamesize   = SCL_PN1WORD;
	scfg.flip              = SCL_PN_12BIT; 
	scfg.platesize     = SCL_PL_SIZE_2X2; // ou 2X2 ?
	scfg.coltype       = SCL_COL_TYPE_16;//SCL_COL_TYPE_256;
	scfg.datatype      = SCL_CELL;
	scfg.patnamecontrl =  0x000c;// VRAM B1 のオフセット 
	scfg.plate_addr[0] = SCL_VDP2_VRAM_A0;
	SCL_SetConfig(SCL_NBG1, &scfg);

	scfg.plate_addr[0] = SCL_VDP2_VRAM_B0;
	SCL_SetConfig(SCL_NBG2, &scfg);

	scfg.dispenbl 	   = ON;
	scfg.bmpsize 	   = SCL_BMP_SIZE_512X256;
//	scfg.coltype 	   = SCL_COL_TYPE_16;//SCL_COL_TYPE_16;//SCL_COL_TYPE_256;
	scfg.datatype 	   = SCL_BITMAP;
	scfg.mapover	       = SCL_OVER_0;
	scfg.plate_addr[0] = SCL_VDP2_VRAM_A1;
	SCL_SetConfig(SCL_NBG0, &scfg);

	SCL_SetCycleTable(CycleTb);

	memset(SCL_VDP2_VRAM_B0,0x00,0x2000);
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void initPosition()
{
	SCL_Open();
	ss_reg->n1_move_y =  (32<<16) ;
	ss_reg->n1_move_x =  (-8<<16) ;
	ss_reg->n2_move_x =   (-8);
	ss_reg->n2_move_y =  32 ;
	SCL_Close();
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void initColors()
{
	colBgAddr = (Uint16*)SCL_AllocColRam(SCL_SPR,OFF);
	colAddr = (Uint16*)SCL_AllocColRam(SCL_NBG1,OFF);
	SCL_AllocColRam(SCL_NBG2,OFF);
	SCL_SetColRam(SCL_NBG0,8,8,palette);
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void DrvInitSaturn(INT32 i)
{
	cleanSprites();
	SPR_InitSlaveSH();
	nBurnSprites  = 67;

//	ss_map  = (Uint16 *)SS_MAP;
//	ss_map2= (Uint16 *)SS_MAP2;
//	ss_font  = (Uint16 *)SS_FONT;

	ss_BgPriNum     = (SclBgPriNumRegister *)SS_N0PRI;
	ss_SpPriNum     = (SclSpPriNumRegister *)SS_SPPRI;

	ss_sprite           = (SprSpCmd *)SS_SPRIT;

	SS_CACHE      = cache = (Uint8 *)SCL_VDP2_VRAM_B1;

	SS_SET_S0PRIN(5);
	SS_SET_N0PRIN(7);
	SS_SET_N2PRIN(4);
	SS_SET_N1PRIN(6);
	initPosition();

		initColors();
		SCL_Open();
		ss_reg->n1_move_x =  (0<<16) ;

	initLayers();

	initSprites(264-1,216-1,0,0,8,-32);
	drawWindow(0,192,192,2,62);
	*(unsigned int*)OPEN_CSH_VAR(nSoundBufferPos) = 0;
	//*(unsigned int*)OPEN_CSH_VAR(SOUND_LEN) = 128;
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void cleanSprites()
{
	unsigned int delta;	
	for (delta=3; delta<nBurnSprites; delta++)
	{
		ss_sprite[delta].charSize   = 0;
		ss_sprite[delta].charAddr   = 0;
		ss_sprite[delta].ax   = 0;
		ss_sprite[delta].ay   = 0;
	} 
}
//-------------------------------------------------------------------------------------------------------------------------------------

/*static*/  INT32 DrvExit()
{
//	BurnYM2203Exit();

//	GenericTilesExit();

	CZetExit();

	BurnFree (AllMem);

	return 0;
}

/*static*/  void DrvCalculatePalette()
{
	for (INT32 i = 0; i < 0x800; i+=2)
	{
		DrvPaletteUpdate(i);
	}
}

/*static*/  void draw_bg_layer()
{
	INT32 xscroll = (scrollx[0] + 0) & 0x1ff;
	INT32 yscroll = (scrolly[0] + 32) & 0x1ff;

	for (INT32 offs = 0; offs < 32 * 32; offs++)
	{
		INT32 sx = (offs & 0x1f) * 16;
		INT32 sy = (offs / 0x20) * 16;

		sx -= xscroll;
		if (sx < -15) sx += 512;
		sy -= yscroll;
		if (sy < -15) sy += 512;

		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		INT32 attr  = DrvBgRAM[offs*2+1];
		INT32 code  = DrvBgRAM[offs*2+0] + ((attr & 0xc0) << 2);
		INT32 flipx = attr & 0x10;
		INT32 flipy = attr & 0x20;
		INT32 color = attr & 0x0f;

		if (flipy) {
			if (flipx) {
//				Render16x16Tile_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			} else {
//				Render16x16Tile_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			}
		} else {
			if (flipx) {
//				Render16x16Tile_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			} else {
//				Render16x16Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			}
		}
	}
}

/*static*/  void draw_mnight_bg_layer()
{
	INT32 xscroll = (scrollx[0] + 0) & 0x1ff;
	INT32 yscroll = (scrolly[0] + 32) & 0x1ff;

	for (INT32 offs = 0; offs < 32 * 32; offs++)
	{
		INT32 sx = (offs & 0x1f) * 16;
		INT32 sy = (offs / 0x20) * 16;

		sx -= xscroll;
		if (sx < -15) sx += 512;
		sy -= yscroll;
		if (sy < -15) sy += 512;

		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		INT32 attr  = DrvBgRAM[offs*2+1];
		INT32 code  = DrvBgRAM[offs*2+0] + ((attr & 0xc0) << 2) + ((attr & 0x10) << 6);
		INT32 flipx = 0;
		INT32 flipy = attr & 0x20;
		INT32 color = attr & 0x0f;

		if (flipy) {
			if (flipx) {
//				Render16x16Tile_FlipXY_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			} else {
//				Render16x16Tile_FlipY_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			}
		} else {
			if (flipx) {
//				Render16x16Tile_FlipX_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			} else {
//				Render16x16Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, DrvGfxROM2);
			}
		}
	}
}

/*static*/  void draw_robokid_bg_layer(INT32 sel, UINT8 *ram, UINT8 *rom, INT32 width, INT32 transp)
{
	if (tilemap_enable[sel] == 0) return;

	INT32 wide = (width) ? 128 : 32;
	INT32 xscroll = scrollx[sel] & ((wide * 16) - 1);
	INT32 yscroll = (scrolly[sel] + 32) & 0x1ff;

	for (INT32 offs = 0; offs < wide * 32; offs++)
	{
		INT32 sx = (offs % wide);
		INT32 sy = (offs / wide);

		INT32 ofst = (sx & 0x0f) + (sy * 16) + ((sx & 0x70) * 0x20);

		sx = (sx * 16) - xscroll;
		if (sx < -15) sx += wide * 16;
		sy = (sy * 16) - yscroll;
		if (sy < -15) sy += 32 * 16;

		if (sx >= nScreenWidth || sy >= nScreenHeight) continue;

		INT32 attr  = ram[ofst * 2 + 1];
		INT32 code  = ram[ofst * 2 + 0] + ((attr & 0x10) << 7) + ((attr & 0x20) << 5) + ((attr & 0xc0) << 2);
		INT32 color = attr & 0x0f;

		if (transp) {
//			Render16x16Tile_Mask_Clip(pTransDraw, code, sx, sy, color, 4, 0xf, 0, rom);
		} else {
//			Render16x16Tile_Clip(pTransDraw, code, sx, sy, color, 4, 0, rom);
		}
	}
}

/*static*/  void draw_fg_layer(INT32 color_offset)
{
	for (INT32 offs = (32 * 4); offs < (32 * 32) - (32 * 4); offs++)
	{
		INT32 sx = (offs & 0x1f) * 8;
		INT32 sy = (offs / 0x20) * 8;

		INT32 attr  = DrvFgRAM[offs*2+1];
		INT32 code  = DrvFgRAM[offs*2+0] + ((attr & 0xc0) << 2);
		INT32 flipx = attr & 0x10;
		INT32 flipy = attr & 0x20;
		INT32 color = attr & 0x0f;

		if (flipy) {
			if (flipx) {
//				Render8x8Tile_Mask_FlipXY_Clip(pTransDraw, code, sx, sy - 32, color, 4, 0xf, color_offset, DrvGfxROM0);
			} else {
//				Render8x8Tile_Mask_FlipY_Clip(pTransDraw, code, sx, sy - 32, color, 4, 0xf, color_offset, DrvGfxROM0);
			}
		} else {
			if (flipx) {
//				Render8x8Tile_Mask_FlipX_Clip(pTransDraw, code, sx, sy - 32, color, 4, 0xf, color_offset, DrvGfxROM0);
			} else {
//				Render8x8Tile_Mask_Clip(pTransDraw, code, sx, sy - 32, color, 4, 0xf, color_offset, DrvGfxROM0);
			}
		}
	}
}

/*static*/  void draw_sprites(INT32 color_offset, INT32 robokid)
{
	int const big_xshift = robokid ? 1 : 0;
	int const big_yshift = robokid ? 0 : 1;

	UINT8* sprptr = DrvSprRAM + 11;
	int sprites_drawn = 0;

	while (1)
	{
		if (sprptr[2] & 0x02)
		{
			int sx = sprptr[1] - ((sprptr[2] & 0x01) << 8);
			int sy = sprptr[0];

			int code = sprptr[3] + ((sprptr[2] & 0xc0) << 2) + ((sprptr[2] & 0x08) << 7);
			int flipx = (sprptr[2] & 0x10) >> 4;
			int flipy = (sprptr[2] & 0x20) >> 5;
			int const color = sprptr[4] & 0x0f;

			int const big = (sprptr[2] & 0x04) >> 2;

			if (*flipscreen)
			{
				sx = 240 - 16*big - sx;
				sy = 240 - 16*big - sy;
				flipx ^= 1;
				flipy ^= 1;
			}

			if (big)
			{
				code &= ~3;
				code ^= flipx << big_xshift;
				code ^= flipy << big_yshift;
			}

			for (int y = 0; y <= big; ++y)
			{
				for (int x = 0; x <= big; ++x)
				{
					int const tile = code ^ (x << big_xshift) ^ (y << big_yshift);

					if (flipy) {
						if (flipx) {
//							Render16x16Tile_Mask_FlipXY_Clip(pSpriteDraw, tile, sx + 16*x, (sy + 16*y) - 32, color, 4, 0xf, color_offset, DrvGfxROM1);
						} else {
//							Render16x16Tile_Mask_FlipY_Clip(pSpriteDraw, tile, sx + 16*x, (sy + 16*y) - 32, color, 4, 0xf, color_offset, DrvGfxROM1);
						}
					} else {
						if (flipx) {
//							Render16x16Tile_Mask_FlipX_Clip(pSpriteDraw, tile, sx + 16*x, (sy + 16*y) - 32, color, 4, 0xf, color_offset, DrvGfxROM1);
						} else {
//							Render16x16Tile_Mask_Clip(pSpriteDraw, tile, sx + 16*x, (sy + 16*y) - 32, color, 4, 0xf, color_offset, DrvGfxROM1);
						}
					}

					++sprites_drawn;

					if (sprites_drawn >= 96)
						break;
				}
			}
		}
		else
		{
			++sprites_drawn;

			if (sprites_drawn >= 96)
				break;
		}

		sprptr += 16;
	}
}

/*static*/  void draw_copy_sprites()
{
	for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
//		if (pSpriteDraw[i] != 0x000f) pTransDraw[i] = pSpriteDraw[i];
	}
}

/*static*/  INT32 Ninjakd2Draw()
{
	if (DrvRecalc) {
		DrvCalculatePalette();
		DrvRecalc = 0;
	}

	if (overdraw_enable) {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			if ((pSpriteDraw[i] & 0x00f0) == 0x00f0) pSpriteDraw[i] = 0x000f;
		}
	} else {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pSpriteDraw[i] = 0x000f;
		}
	}

	draw_sprites(0x100, 0);

//	if (tilemap_enable[0] == 0)
//		BurnTransferClear();

	if (tilemap_enable[0])
		draw_bg_layer();

	draw_copy_sprites();

	draw_fg_layer(0x200);

//	BurnTransferCopy(DrvPalette);

	return 0;
}

/*static*/  INT32 MnightDraw()
{
	if (DrvRecalc) {
		DrvCalculatePalette();
		DrvRecalc = 0;
	}

	if (overdraw_enable) {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			if ((pSpriteDraw[i] & 0x00f0) == 0x00f0) pSpriteDraw[i] = 0x000f;
		}
	} else {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pSpriteDraw[i] = 0x000f;
		}
	}

	draw_sprites(0x100, 0);

//	if (tilemap_enable[0] == 0)
//		BurnTransferClear();

	if (tilemap_enable[0])
		draw_mnight_bg_layer();

	draw_copy_sprites();

	draw_fg_layer(0x200);

//	BurnTransferCopy(DrvPalette);

	return 0;
}

/*static*/  INT32 RobokidDraw()
{
	if (DrvRecalc) {
		DrvCalculatePalette();
		DrvRecalc = 0;
	}

	if (overdraw_enable) {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			if ((pSpriteDraw[i] & 0x00f0) < 0x00e0) pSpriteDraw[i] = 0x000f;
		}
	} else {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pSpriteDraw[i] = 0x000f;
		}
	}

	draw_sprites(0x200, 1);

//	if (tilemap_enable[0] == 0)
//		BurnTransferClear();

	draw_robokid_bg_layer(0, DrvBgRAM0, DrvGfxROM2, 0, 0);

	draw_robokid_bg_layer(1, DrvBgRAM1, DrvGfxROM3, 0, 1);

	draw_copy_sprites();

	draw_robokid_bg_layer(2, DrvBgRAM2, DrvGfxROM4, 0, 1);

	draw_fg_layer(0x300);

//	BurnTransferCopy(DrvPalette);

	return 0;
}

/*static*/  INT32 OmegafDraw()
{
	if (DrvRecalc) {
		DrvCalculatePalette();
		DrvRecalc = 0;
	}

	if (overdraw_enable) {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pSpriteDraw[i] = 0x000f; // no enable??
		}
	} else {
		for (INT32 i = 0; i < nScreenWidth * nScreenHeight; i++) {
			pSpriteDraw[i] = 0x000f;
		}
	}

	draw_sprites(0x200, 1);

//	BurnTransferClear();

	draw_robokid_bg_layer(0, DrvBgRAM0, DrvGfxROM2, 1, 1);
	draw_robokid_bg_layer(1, DrvBgRAM1, DrvGfxROM3, 1, 1);
	draw_robokid_bg_layer(2, DrvBgRAM2, DrvGfxROM4, 1, 1);

	draw_copy_sprites();

	draw_fg_layer(0x300);

//	BurnTransferCopy(DrvPalette);

	return 0;
}

/*static*/  inline void DrvClearOpposites(UINT8* nJoystickInputs)
{ // for active LOW
	if ((*nJoystickInputs & 0x03) == 0x00) {
		*nJoystickInputs |= 0x03;
	}
	if ((*nJoystickInputs & 0x0c) == 0x00) {
		*nJoystickInputs |= 0x0c;
	}
}

/*static*/  INT32 DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		memset (DrvInputs, 0xff, 3);

		for (INT32 i = 0; i < 8; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
			DrvInputs[2] ^= (DrvJoy3[i] & 1) << i;
		}

		DrvClearOpposites(&DrvInputs[1]);
		DrvClearOpposites(&DrvInputs[2]);

		previous_coin[0] = (DrvInputs[0] & 0x40) ? 0 : (previous_coin[0] + 1);
		previous_coin[1] = (DrvInputs[0] & 0x80) ? 0 : (previous_coin[1] + 1);
		if (previous_coin[0] >= 4) DrvInputs[0] |= 0x40;
		if (previous_coin[1] >= 4) DrvInputs[0] |= 0x80;
	}

	CZetNewFrame();

	INT32 nCycleSegment;
	INT32 nInterleave = 10;
	INT32 nCyclesTotal[2] = { 6000000 / 60, 5000000 / 60 };
	INT32 nCyclesDone[2] = { 0, 0 };

	for (INT32 i = 0; i < nInterleave; i++)
	{
		nCycleSegment = nCyclesTotal[0] / nInterleave;

		CZetOpen(0);
		nCyclesDone[0] += CZetRun(nCycleSegment);

		if (i == (nInterleave-1))
		{
//			CZetSetVector(0xd7);
			CZetRaiseIrq(0xd7);
			CZetSetIRQLine(0, CZET_IRQSTATUS_AUTO);
		}

		CZetClose();

		nCycleSegment = nCyclesTotal[1] / nInterleave;

		CZetOpen(1);
	//	nCyclesDone[1] += ZetRun(nCycleSegment);
//		BurnTimerUpdate((i + 1) * nCycleSegment);
		CZetClose();
	}

	CZetOpen(1);

//	BurnTimerEndFrame(nCyclesTotal[1]);

//	if (pBurnSoundOut) {
//		BurnYM2203Update(pBurnSoundOut, nBurnSoundLen);
//		ninjakd2_sample_player(pBurnSoundOut, nBurnSoundLen);
//	}

	CZetClose();

//	if (pBurnDraw) {
//		BurnDrvRedraw();
//	}

	return 0;
}
