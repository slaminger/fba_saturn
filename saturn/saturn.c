#include "saturn.h"

//#include "sc_saturn.h"
#define GAME_BY_PAGE 16
//#define OVLADDR  0x060A5000
#define OVLADDR  0x060A5000
#define LOWADDR 0x00200000
volatile SysPort	*__port;
static trigger_t	pltrigger[2],pltriggerE[2];
//static unsigned char ServiceRequest = 0;
//static unsigned char *ServiceDip = 0;
extern unsigned char play;
unsigned char drvquit;
//struct BurnDriver* oDriver;
/*typedef struct  {
  int arena;    // total space allocated from system 
  int ordblks;  // number of non-inuse chunks 
  int smblks;   //* unused -- always zero 
  int hblks;    //* number of mmapped regions 
  int hblkhd;   //* total space in mmapped regions 
  int usmblks;  //* unused -- always zero 
  int fsmblks;  //* unused -- always zero 
  int uordblks; //* total allocated space 
  int fordblks; //* total non-inuse space 
  int keepcost; // top-most, releasable (via malloc_trim) space 
}mallinfo;	*/
//-------------------------------------------------------------------------------------------------------------------------------------
void	UsrVblankIn( void )
{
#ifdef FONT
	char xx[4];
   PER_GetPort(__port);	
#endif
	PCM_MeVblIn();
	SCL_ScrollShow();
	if(nBurnFunction!=NULL) nBurnFunction();
#ifndef ACTION_REPLAY

	if(play)
	{
#endif
#ifdef FONT
		frame_y++;

		if(frame_y==hz)
		{
				if(frame_displayed!=frame_x)
				{
					sprintf(xx,"%03d",frame_x);
					FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)xx,136,20);
					frame_displayed = frame_x;
				}
				frame_y=frame_x=0;
		}
#endif
#ifndef ACTION_REPLAY
	}
#endif
//	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
void   UsrVblankOut( void )
{
	do_keypad();
	InpMake(FBA_KEYPAD);
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void	SetVblank( void )
{
	INT_ChgMsk(INT_MSK_NULL,INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT);
	INT_SetScuFunc(INT_SCU_VBLK_IN,UsrVblankIn);
	INT_SetScuFunc(INT_SCU_VBLK_OUT,UsrVblankOut);
	INT_ChgMsk(INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT,INT_MSK_NULL);
	__port = PER_OpenPort();
}
//-------------------------------------------------------------------------------------------------------------------------------------
//void memsetl_fast(UINT32 *ptr, UINT32 value, UINT32 len);


int main(void)
{
	Uint8	*dst;
    Uint16  loop;	
   
    // 1.Zero Set .bss Section
    for (dst = (Uint8 *)&_bstart; dst < (Uint8 *)&_bend; dst++)
		*dst = 0;
	
    for (dst = (Uint8 *)SystemWork, loop = 0; loop < SystemSize; loop++)
		*dst = 0;


	memset(&play,0x00,1024);

	SYS_CHGSYSCK(1);             //28mhz
	set_imask(0); 
	
	VDP2_InitVRAM();   
	
	ss_main();
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void	_spr2_initialize( void )
{
	set_imask(0);
    SCL_SET_SPTYPE(SCL_TYPE0);
    SCL_SET_SPCLMD(SCL_PALETTE);
    SCL_SET_SPWINEN(SCL_SP_WINDOW);
	SPR_Initial(&aVRAM);
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ __inline__ void	_spr2_transfercommand()
{
	  memcpyl(aVRAM,smsSprite,(nBurnSprites<<5) ) ;
}
//-------------------------------------------------------------------------------------------------------------------------------------
void initScrolling(Uint8 enabled)
{
    SCL_InitLineParamTb(&lp);
	lp.delta_enbl=OFF;
	lp.cell_enbl=OFF;
    lp.v_enbl=OFF;
    lp.h_enbl=enabled;
	if(enabled==ON)
	{
//		lp.line_addr=SCL_VDP2_VRAM_B1-0x600;
		lp.line_addr=SCL_VDP2_VRAM_B0+0x4000;
		SclAddrLsTbl[0] = lp.line_addr;//+0x20;
		SclAddrLsTbl[1] = (Uint32 )&ls_tbl[0];
	}
	else
	{
		lp.line_addr=0x00;
		SclAddrLsTbl[0] = 0x00;
		SclAddrLsTbl[1] = NULL;
		nBurnLinescrollSize = 1;
	}
  

	Scl_n_reg.linecontrl = (lp.h_enbl << 1) & 0x0002;
    lp.interval=0;

	(*(Uint16 *)0x25F8009A) = 0x0003; 
	(*(Uint16 *)0x25F80020) = 0x0303;
	SclProcess = 2;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void initSound()
{
	sndInit();
	PCM_MeInit();
	PcmCreatePara	para;
	PcmInfo 		info;
	PcmStatus	*st;
	static PcmWork g_movie_work;

	PCM_PARA_WORK(&para) = (struct PcmWork *)&g_movie_work;
	PCM_PARA_RING_ADDR(&para) = (Sint8 *)SOUND_BUFFER;
	PCM_PARA_RING_SIZE(&para) = RING_BUF_SIZE;
	PCM_PARA_PCM_ADDR(&para) = PCM_ADDR;
	PCM_PARA_PCM_SIZE(&para) = PCM_SIZE;

	memset((Sint8 *)SOUND_BUFFER,0,SOUNDRATE*16);
	st = &g_movie_work.status;
	st->need_ci = PCM_ON;

	PCM_INFO_FILE_TYPE(&info) = PCM_FILE_TYPE_NO_HEADER;			
	PCM_INFO_DATA_TYPE(&info)=PCM_DATA_TYPE_RLRLRL;//PCM_DATA_TYPE_LRLRLR;
	PCM_INFO_FILE_SIZE(&info) = RING_BUF_SIZE;//SOUNDRATE*2;//0x4000;//214896;
	PCM_INFO_CHANNEL(&info) = 0x01;
	PCM_INFO_SAMPLING_BIT(&info) = 16;

	PCM_INFO_SAMPLING_RATE(&info)	= SOUNDRATE;//30720L;//44100L;
	PCM_INFO_SAMPLE_FILE(&info) = RING_BUF_SIZE;//SOUNDRATE*2;//30720L;//214896;
//	pcm = PCM_CreateStmHandle(&para, stm);
	pcm = createHandle(&para);

	PCM_SetPcmStreamNo(pcm, 0);


	PCM_SetInfo(pcm, &info);
	PCM_ChangePcmPara(pcm);

	PCM_MeSetLoop(pcm, 0x1FF);//SOUNDRATE*60);
	if (pcm == NULL) {
		return;
	}
	PCM_Start(pcm);
}
//-------------------------------------------------------------------------------------------------------------------------------------
void resetLayers()
{
	Uint16	CycleTb[]={
		0x55ee, 0xeeee, //A1
		0xffff, 0xffff,	//A0
		0xff44, 0xeeee,   //B1
		0xffff, 0xffff,  //B0
	};
 	SclConfig	scfg;

	SS_FONT = (Uint16 *)SCL_VDP2_VRAM_A1;
	SS_MAP  = (Uint16 *)SCL_VDP2_VRAM_B0;
	SS_MAP2 = (Uint16 *)SCL_VDP2_VRAM_A0;
//	memset(SCL_VDP2_VRAM,0,0x80000);
//	memset(ss_map,0,0x20000);
//	memset(ss_map2,0,0x20000);
//	memset4_fast(SS_CACHE,0,0x30000);
//	cache    =(Uint16 *)NULL;

	SCL_ParametersInit();

	scfg.dispenbl 	   = ON;
	scfg.bmpsize 	   = SCL_BMP_SIZE_512X256;
	scfg.datatype 	   = SCL_BITMAP;
	scfg.mapover       = SCL_OVER_0;

	scfg.coltype 	   = SCL_COL_TYPE_256;//SCL_COL_TYPE_16;//SCL_COL_TYPE_256;
	scfg.plate_addr[0] = (Uint32)SS_MAP;
	scfg.plate_addr[1] = 0x00;
	SCL_SetConfig(SCL_NBG0, &scfg);

	scfg.coltype 	       = SCL_COL_TYPE_16;//SCL_COL_TYPE_16;//SCL_COL_TYPE_256;
	scfg.plate_addr[0]= (Uint32)SS_FONT;
	scfg.plate_addr[1] = 0x00;
	SCL_SetConfig(SCL_NBG1, &scfg);

	scfg.dispenbl 	   = OFF;
	scfg.plate_addr[0]= (Uint32)NULL;
	SCL_SetConfig(SCL_NBG2, &scfg);

	SCL_SetCycleTable(CycleTb);

    SCL_SET_S0PRIN(0);
    SCL_SET_N0PRIN(2);
    SCL_SET_N1PRIN(3);
    SCL_SET_N2PRIN(1);
}
//--------------------------------------------------------------------------------------------------------------
/*static*/ void resetColors()
{
	Uint16 i;
	Uint16 *VRAM;

	VRAM = (Uint16 *)SCL_COLRAM_ADDR;
	for( i = 0; i < 4096; i++ )
		*(VRAM++) = 0x8000;

	memset(SclColRamAlloc256,0,sizeof(SclColRamAlloc256));
	SCL_SET_SPCAOS(0);
	SCL_SET_N0CAOS(0);
	SCL_SET_N1CAOS(0);
	SCL_SET_N2CAOS(0);
	SCL_SET_N3CAOS(0);

	SCL_AllocColRam(SCL_NBG0,OFF);
	SCL_AllocColRam(SCL_NBG1,OFF);

	SCL_SetColRam(SCL_NBG1,8,8,palette);
}
//-------------------------------------------------------------------------------------------------------------------------------------
/*static*/ void initSaturn()
{

	Uint8	*dst;
//	malloc_trim();
   Uint32 __malloc_sbrk_base;
/*   
	for (dst = (Uint8 *)&_bstart; dst < (Uint8 *)_bend; dst++)
		*dst = 0;
    for (dst = (Uint8 *)&__malloc_sbrk_base; dst < (Uint8 *)OVLADDR; dst++)
//		if(	!(dst>=0x6003500 && dst <0x6004000))
		*dst = 0;
  */
//	nBurnLinescrollSize = 0x400;//0x400
//	nBurnSprites = 131;
//	INT_ChgMsk(INT_MSK_NULL,INT_ST_ALL);
	nBurnFunction = NULL;
	nSoundBufferPos = 0;

	play=1;
//	cleanSprites();
//	_spr2_transfercommand();
	memset(SOUND_BUFFER,0x00,RING_BUF_SIZE*8);
//	memset(ls_tbl,0,sizeof(ls_tbl));
	initScrolling(OFF);

	InitCD();
	VDP2_InitVRAM();
	resetLayers();
//	PCM_MeReset(pcm);
//	SetVblank();
//wait_vblank();
	memset4_fast(SCL_VDP2_VRAM_A0,0,0x20000);
	memset4_fast(SCL_VDP2_VRAM_A1,0,0x20000);

	memset4_fast(SCL_VDP2_VRAM_B0,0,0x20000);
	memset4_fast(SCL_VDP2_VRAM_B1,0,0x20000);

	memset(pltrigger[0],0x00,sizeof(trigger_t));
	memset(pltriggerE[0],0x00,sizeof(trigger_t));

	play = 0;
	resetColors();
	initSprites(352-1,240-1,0,0,0,0);
//	_spr2_transfercommand();
	//SclProcess = 2;
	SetVblank();
	SCL_SetLineParam2(&lp);
wait_vblank();
	play=1;
//	SCL_ParametersInit();
//	SCL_SetLineParam2(&lp);
	PCM_Task(pcm);
	PCM_MeSetVolume(pcm,0);
	PCM_DrvChangePcmPara(pcm,-1,-1);
	play = 0;
	wait_vblank();

	SS_REG   = &Scl_n_reg;
	SS_REGS =	&Scl_s_reg;
	SS_SPRAM = &aVRAM[0];
 	SS_N0PRI = &SclBgPriNum;
	SS_SPPRI = &SclSpPriNum;
	SS_OTHR  = &SclOtherPri;
	SS_SPRIT = &smsSprite[0];
	SS_SCL	 = &ls_tbl[0];

	col[0]=0;
	col[1]=9;
	col[2]=10;
	col[3]=11;

//Scl_n_reg.n0_move_x = 0;
//		SetVblank();
//	SclProcess = 1;
//
//	SetVblank();
//		initColors();
/*
#ifndef ACTION_REPLAY
	if(FntAsciiFontData2bpp==NULL)
		FntAsciiFontData2bpp = (Uint8*)malloc(1600);
	GFS_Load(GFS_NameToId("FONT.BIN"),0,(void *)FntAsciiFontData2bpp,1600);
#endif
	*/
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void ss_main(void)
{
#ifndef ACTION_REPLAY
//	CdUnlock();
	InitCD();
#endif
	hz = get_hz();
	initSound();
	CSH_Init(CSH_4WAY);
//	SPR_InitSlaveSH();
//	slob_init();
	initSaturn();
	BurnLibInit();
	BurnDrvAssignList();
//	testTga();

#ifndef ACTION_REPLAY
	if(FntAsciiFontData2bpp==NULL)
		FntAsciiFontData2bpp = (Uint8*)malloc(1600);
	GFS_Load(GFS_NameToId("FONT.BIN"),0,(void *)FntAsciiFontData2bpp,1600);
#endif

	while(1)
	{
		__port = PER_OpenPort();
		display_menu();
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void VDP2_InitVRAM(void)
{
	/* Variables. */

	Uint32	loop;
	loop = 0;
	while (loop < 0x40000)
	{
		*((Uint32 *) (SCL_VDP2_VRAM_A0 + loop)) = 0;
		*((Uint32 *) (SCL_VDP2_VRAM_B0 + loop)) = 0;
		loop += 4;
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------
static void load_img(int id)
{
	void (*fp)(unsigned int);
	fp = (void *)LOWADDR;
	(*fp)(id);

	if(id!=0)
	{
	 	wait_key(PER_DGT_B);
		load_img(0);
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------
void wait_key(Uint8 key)
{
	SysDevice	*device;
	do
	{
		device = PER_GetDeviceR( &__port[0], 0 );
		pltrigger[0]  = PER_GetTrigger( device );
	}while((pltrigger[0] & PER_DGT_B)==0) ;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static unsigned char update_input(unsigned int *current_page,unsigned char *loaded)
{
	unsigned int i=0;
	SysDevice	*device;

	if(play==0 && ( device = PER_GetDeviceR( &__port[0], 0 )) != NULL )
	{
		pltriggerE[0] = pltrigger[0];
		pltrigger[0]  = PER_GetTrigger( device );
		pltriggerE[0] = (pltrigger[0]) ^ (pltriggerE[0]);
		pltriggerE[0] = (pltrigger[0]) & (pltriggerE[0]);

		for(i=0;i<8;i++)
		{
			if((pltriggerE[0] & pad_asign[i])!=0)
			{
				switch(pltriggerE[0] & pad_asign[i] )
				{
					case PER_DGT_A: 
					load_img(1);
					break;

					case PER_DGT_C: 
					load_img(2);
					break;

					case PER_DGT_D: 
					if(nBurnDrvSelect < nBurnDrvCount-1 && nBurnDrvSelect < ((*current_page) * GAME_BY_PAGE)-1) nBurnDrvSelect++;
//					else								 nBurnDrvSelect=0;
					break;

					case PER_DGT_U:
					if(nBurnDrvSelect >  ((*current_page)-1) * GAME_BY_PAGE)	nBurnDrvSelect--;
//					else						nBurnDrvSelect=nBurnDrvCount-1;
					break;

					case PER_DGT_L: 
					if(*current_page > 1) nBurnDrvSelect = (--(*current_page)-1) * GAME_BY_PAGE;
					break;

					case PER_DGT_R: 
					if(*current_page * GAME_BY_PAGE  < nBurnDrvCount) nBurnDrvSelect = (++(*current_page)-1) * GAME_BY_PAGE;
					break;

					case PER_DGT_S:
					run_fba_emulator();
					loaded[0] = 0;
/*
                0x0000000006019acc                ___malloc_current_mallinfo
                0x0000000006019af4                ___malloc_max_total_mem
                0x0000000006019af8                ___malloc_max_sbrked_mem
                0x0000000006019afc                ___malloc_top_pad
*/
/*
	char toto[50];
	extern int __malloc_max_total_mem;
	extern int __malloc_max_sbrked_mem;
	extern Uint32 __malloc_sbrk_base;
	extern mallinfo  __malloc_current_mallinfo;

	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"A:Help",12,201);
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"C:Credits",127,201);

	sprintf (toto,"arena %08x",__malloc_current_mallinfo.arena) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,171);

	sprintf (toto,"ordblks %08x",__malloc_current_mallinfo.ordblks) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,181);

	sprintf (toto,"hblks %08x",__malloc_current_mallinfo.hblks) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,191);

	sprintf (toto,"hblkhd %08x",__malloc_current_mallinfo.hblkhd) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,201);

	sprintf (toto,"keepcost %08x",__malloc_current_mallinfo.keepcost) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,211);
  */
					break;

					default:
					break;
				}
			}
		}
	}
	return 0;
}
//--------------------------------------------------------------------------------------------------------------
//int the_loop=1;
static void display_menu(void)
{
//	_spr2_initialize();	
//	SetVblank();
//	set_imask(0);
	
	unsigned int l;
	unsigned char loaded=0;
//	sc_init();
//	the_loop = 1;
	/*
	char toto[50];
	extern int __malloc_max_total_mem;
	extern int __malloc_max_sbrked_mem;
	extern Uint32 __malloc_sbrk_base;
	extern int __malloc_trim_threshold;
	extern mallinfo  __malloc_current_mallinfo;

	__malloc_trim_threshold = 1024;

	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"A:Help",12,201);
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"C:Credits",127,201);

	sprintf (toto,"arena %08x",__malloc_current_mallinfo.arena) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,181);

	sprintf (toto,"malloc_trim_threshold %08x",__malloc_trim_threshold) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,191);

	sprintf (toto,"uordblks %08x",__malloc_current_mallinfo.uordblks) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,201);

	sprintf (toto,"fordblks %08x",__malloc_current_mallinfo.fordblks) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,211);

	sprintf (toto,"keepcost %08x",__malloc_current_mallinfo.keepcost) ;
	FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)toto,12,221);
	  */
	unsigned int current_page = 1,m;
	do
	{
		if(!loaded)
		{
			GFS_Load(GFS_NameToId("IMG.BIN"),  0,(void *)LOWADDR, GFS_BUFSIZ_INF);
			load_img(0);	
			loaded=1;
		}
		m=0;
//		char page_header[50];
		char game_name[50];
//		sprintf(page_header,"Game list:                       %02d/%02d",current_page, (nBurnDrvCount+GAME_BY_PAGE-1)/GAME_BY_PAGE);
//		FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)page_header,12,12);

		for (l=(current_page-1)*GAME_BY_PAGE;l<(current_page)*GAME_BY_PAGE && l<nBurnDrvCount;l++ )
		{
			sprintf(game_name,"%-38s",pDriver[l]->szFullNameA);
			if(l==nBurnDrvSelect)	 FNT_Print256_2bppSel((volatile Uint8 *)SS_FONT,(Uint8 *)game_name,20,40+m);	  //12+m
			else					 FNT_Print256_2bpp   ((volatile Uint8 *)SS_FONT,(Uint8 *)game_name,20,40+m);
			m+=10;
		}

		for (;l<(current_page)*GAME_BY_PAGE;l++ )
		{
			sprintf(game_name,"%-38s"," ");
			FNT_Print256_2bpp   ((volatile Uint8 *)SS_FONT,(Uint8 *)game_name,20,40+m);
			m+=10;
		}
		update_input(&current_page,&loaded);
		//sc_check();
//		scd_logout("display_menu",0);

	}while(1);
	

}

#if 0
inline void restart()
{
/*Uint8	*dst;
	for (dst = (Uint8 *)&_bstart; dst < (Uint8 *)&_bend; dst++)
		{
		SetVblank();
		*dst = 0;
		}			*/
// n馗essaire en sortant de green beret 
//		SetVblank();
//		wait_vblank();
		set_imask(1);
		BurnDrvExit();
		PCM_MeStop(pcm);

		_smpc_SSHOFF();
		_smpc_SNDOFF();

	 Uint8	*dst;
    Uint16  loop;	
	for (dst = (Uint8 *)&_bstart; dst < (Uint8 *)&_bend; dst++)
		*dst = 0;

	
	for (dst = (Uint8 *)SystemWork, loop = 0; loop < SystemSize; loop++)
		*dst = 0;
 	for (dst = (Uint8 *)OVLADDR; dst < (Uint8 *)SystemWork; dst++)
		{
		*dst = 0;
		}
//#define SystemWork2  0x060ffc00              /* System Variable Address */
//#define SystemSize2  (0x06100000-0x060ffc00) /* System Variable Size */
//xxxx
		/*
	ss_reg = ss_regs = ss_SpPriNum = ss_BgPriNum = ss_OtherPri = colAddr = NULL;
	aVRAM = cache = colBgAddr = colBgAddr2 = ss_font = ss_map = ss_map2 = ss_scl = NULL;
		*/
		INT_ChgMsk(INT_MSK_NULL,INT_ST_ALL);
		void (*fp)(void);
		fp = (void *)0x6004000;
		(*fp)();
//		main();
}
#endif
//-------------------------------------------------------------------------------------------------------------------------------------
static void SCL_ParametersInit(void)
{
//	Uint16	i;
/*
 *	System Registers Area Initialization
 */
	Scl_s_reg.tvmode = 0x8001; /* HREZ = B'001(352 pixels) the others = 0 */
	Scl_s_reg.ramcontrl = 0x1000; /* color 16bit mode; VRAM A, B banked */
/*
 *	Normal Scroll Registers Area Initialization
 */
	Scl_n_reg.n0_move_x = 0;
	Scl_n_reg.n0_move_y = 0;
	Scl_n_reg.n0_delta_x = FIXED(1);
	Scl_n_reg.n0_delta_y = FIXED(1);
	Scl_n_reg.n1_move_x = 0;
	Scl_n_reg.n1_move_y = 0;
	Scl_n_reg.n1_delta_x = FIXED(1);
	Scl_n_reg.n1_delta_y = FIXED(1);
	Scl_n_reg.n2_move_x = 0;
	Scl_n_reg.n2_move_y = 0;
	Scl_n_reg.n3_move_x = 0;
	Scl_n_reg.n3_move_y = 0;
	Scl_n_reg.zoomenbl = 0;
	Scl_n_reg.linecontrl = 0;
	Scl_n_reg.celladdr = 0;
	Scl_n_reg.lineaddr[0] = 0;
	Scl_n_reg.lineaddr[1] = 0;
	Scl_n_reg.linecolmode = 0;
	Scl_n_reg.backcolmode = 0;

	if(SclProcess == 0)	SclProcess = 1;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void SCL_CopyReg()
{
	Uint16 *regaddr = (Uint16 *)REGADDR;
	memcpyl(&regaddr[0x38], &Scl_n_reg, sizeof(SclNorscl));

	if(!play)
	{
		regaddr[0] = Scl_s_reg.tvmode;		/* add				by C.Y	*/
		regaddr[1] = Scl_s_reg.extenbl;		/* add				by C.Y	*/
		regaddr[3] = Scl_s_reg.vramsize;		/* add				by C.Y	*/
		memcpyw(&regaddr[7] , &Scl_s_reg.ramcontrl , 26 );
		memcpyl(&regaddr[0x14], &Scl_d_reg, sizeof(SclDataset));

		memcpyl((SclOtherPriRegister *)0x25F800E0, &SclOtherPri, sizeof(SclOtherPri));
		memcpyl((SclSpPriNumRegister *)0x25F800F0, &SclSpPriNum, sizeof(SclSpPriNum));
		memcpyl((SclBgPriNumRegister *)0x25F800F8, &SclBgPriNum, sizeof(SclBgPriNum));
	}
    //SCL_Memcpyw(&regaddr[0x38], &Scl_n_reg, sizeof(SclNorscl));
//	memcpyl(&regaddr[0x38], &Scl_n_reg, sizeof(SclNorscl));
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void SCL_InitLineParamTb(SclLineparam *lp)
{
	Uint16	i;

	lp->h_enbl = OFF;
	lp->v_enbl = OFF;
	lp->delta_enbl = OFF;
	lp->cell_enbl = OFF;
	lp->line_addr = 0x00;
	lp->cell_addr = 0x00;
	lp->interval  = 0;

	for(i = 0; i< SCL_MAXLINE; i++) 
	{
//	for(i = 0; i< 192; i++) {
		if(i < SCL_MAXCELL) 
		{
			lp->cell_tbl[i] = 0;
		}
		lp->line_tbl[i].h  = FIXED(0);
		lp->line_tbl[i].v  = FIXED(i);
		lp->line_tbl[i].dh = FIXED(1);
//		lp->line_tbl[i].h  = (0<<16);
//		lp->line_tbl[i].v  = (i<<16);
//		lp->line_tbl[i].dh = (1<<16);
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void	SCL_ScrollShow(void)
{
    switch(SclProcess){
	    case 1:
		SCL_CopyReg();
		SclProcess = 0;
		break;
	    case 2:			/* line scroll setting */
		//SCL_Memcpyw((void *)SclAddrLsTbl[0],(void *)SclAddrLsTbl[1], 0x300);
		memcpyl((void *)SclAddrLsTbl[0],(void *)SclAddrLsTbl[1], nBurnLinescrollSize);
		SCL_CopyReg();
		SclProcess = 0;
		break;
		default:
		break;
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------
void SCL_Open(void)
{
	if(SclProcess == 1)	SclProcess = 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
void SCL_Close(void)
{
	if(SclProcess == 0)	SclProcess = 1;
}
//-------------------------------------------------------------------------------------------------------------------------------------
void	SCL_SetCycleTable(Uint16 *tp)
{
	Uint16 i;
	for(i = 0; i<8; i++){
		Scl_s_reg.vramcyc[i] = tp[i];
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
void SCL_SetConfig(Uint16 sclnum, SclConfig *scfg)
{
/*	Uint16	temp, *mapoffset, boundary, *map, *map2, shift,shift2;	*/
	Uint16	temp;
	Uint16	*mapoffset = 0;
	Uint16	*map = 0;
	Uint16	boundary = 0;
	Uint16	shift = 0;
										/* add by C.yoshida */

	Uint16	max = 0;
	Uint16	i;
	Uint16	flip;
	Uint16	mapoffsetW;
										/* add by C.yoshida */

	flip = scfg->flip;				/* get flip bit*/
	flip = (flip<<14) & 0x4000;
	switch(sclnum) {
		case SCL_NBG0:
			temp = scfg->dispenbl;		/* display enable */
			temp &= 0x0001;
			Scl_s_reg.dispenbl &= 0xfffe;
			Scl_s_reg.dispenbl |= temp;

			temp = scfg->charsize;		/* char size */
			temp &= 0x0001;
			Scl_d_reg.charcontrl0 &= 0xfffe;
			Scl_d_reg.charcontrl0 |= temp;

			Scl_d_reg.patnamecontrl[0] = scfg->patnamecontrl;

			temp = scfg->pnamesize;		/* pattern name size */
			temp = (temp <<15) & 0x8000;
			Scl_d_reg.patnamecontrl[0] &= 0x7fff;
			Scl_d_reg.patnamecontrl[0] |= temp;

			Scl_d_reg.patnamecontrl[0] &= 0xbfff;	/* flip bit set */
			Scl_d_reg.patnamecontrl[0] |= flip;	/* flip bit set */

			temp = scfg->platesize;		/* plate size */
			temp &=  0x0003;
			Scl_d_reg.platesize &= 0xfffc;
			Scl_d_reg.platesize |= temp;

			temp = scfg->bmpsize;		/* bitmap size */
			temp = (temp << 2) & 0x000c;
			Scl_d_reg.charcontrl0 &= 0xfff3;
			Scl_d_reg.charcontrl0 |= temp;

			temp = scfg->coltype;		/* color type */
			temp = (temp <<4) & 0x0070;
			Scl_d_reg.charcontrl0 &= 0xff8f;
			Scl_d_reg.charcontrl0 |= temp;

			temp = scfg->datatype;		/* data type */
			temp = (temp <<1) & 0x0002;
			Scl_d_reg.charcontrl0 &= 0xfffd;
			Scl_d_reg.charcontrl0 |= temp;

			Scl_d_reg.mapoffset0 &= 0xfff0;	/* Init map offset */ 
			mapoffset = &Scl_d_reg.mapoffset0; /* map offset */ 
			shift = 0;
			map = &Scl_d_reg.normap[0];
			max = 2;
			break;
		case SCL_NBG1:
			temp = scfg->dispenbl;		/* display enable */
			temp = (temp << 1) & 0x0002;
			Scl_s_reg.dispenbl &= 0xfffD;
			Scl_s_reg.dispenbl |= temp;

			temp = scfg->charsize;		/* char size */
			temp = (temp<<8) & 0x0100;
			Scl_d_reg.charcontrl0 &= 0xfeff;
			Scl_d_reg.charcontrl0 |= temp;

			Scl_d_reg.patnamecontrl[1] = scfg->patnamecontrl;

			temp = scfg->pnamesize;		/* pattern name size */
			temp = (temp <<15) & 0x8000;
			Scl_d_reg.patnamecontrl[1] &= 0x7fff;
			Scl_d_reg.patnamecontrl[1] |= temp;

			Scl_d_reg.patnamecontrl[1] &= 0xbfff;
			Scl_d_reg.patnamecontrl[1] |= flip; /* flip bit set*/

			temp = scfg->platesize;		/* plate size */
			temp = (temp << 2) &  0x000c;
			Scl_d_reg.platesize &= 0xfff3;
			Scl_d_reg.platesize |= temp;

			temp = scfg->bmpsize;		/* bitmap size */
			temp = (temp << 10) & 0x0c00;
			Scl_d_reg.charcontrl0 &= 0xf3ff;
			Scl_d_reg.charcontrl0 |= temp;

			temp = scfg->coltype;		/* color type */
			temp = (temp <<12) & 0x3000;
			Scl_d_reg.charcontrl0 &= 0xcfff;
			Scl_d_reg.charcontrl0 |= temp;

			temp = scfg->datatype;		/* data type */
			temp = (temp <<9) & 0x0200;
			Scl_d_reg.charcontrl0 &= 0xfdff;
			Scl_d_reg.charcontrl0 |= temp;

			Scl_d_reg.mapoffset0 &= 0xff0f;	/* Init map offset */ 
			mapoffset = &Scl_d_reg.mapoffset0; /* map offset */ 
			shift = 4;
			map = &Scl_d_reg.normap[2];
			max = 2;
			break;

		case SCL_NBG2:
			temp = scfg->dispenbl;		/* display enable */
			temp = (temp << 2) & 0x0004;
			Scl_s_reg.dispenbl &= 0xfffb;
			Scl_s_reg.dispenbl |= temp;

			temp = scfg->charsize;		/* char size */
			temp = temp & 0x0001;
			Scl_d_reg.charcontrl1 &= 0xfffe;
			Scl_d_reg.charcontrl1 |= temp;

			Scl_d_reg.patnamecontrl[2] = scfg->patnamecontrl;

			temp = scfg->pnamesize;		/* pattern name size */
			temp = (temp <<15) & 0x8000;
			Scl_d_reg.patnamecontrl[2] &= 0x7fff;
			Scl_d_reg.patnamecontrl[2] |= temp;

			Scl_d_reg.patnamecontrl[2] &= 0xbfff;
			Scl_d_reg.patnamecontrl[2] |= flip; /* flip bit set*/

			temp = scfg->platesize;		/* plate size */
			temp = (temp << 4) &  0x0030;
			Scl_d_reg.platesize &= 0xffcf;
			Scl_d_reg.platesize |= temp;

			temp = scfg->coltype;		/* color type */
			temp = (temp <<1) & 0x0002;
			Scl_d_reg.charcontrl1 &= 0xfffd;
			Scl_d_reg.charcontrl1 |= temp;

			Scl_d_reg.mapoffset0 &= 0xf0ff;	/* Init map offset */ 
			mapoffset = &Scl_d_reg.mapoffset0; /* map offset */ 
			shift = 8;
			map = &Scl_d_reg.normap[4];
			max = 2;
			break;
	}
/*
 *	Set Map Address
 */

	if(scfg->datatype == SCL_BITMAP) {
		mapoffsetW = ((scfg->plate_addr[0] - SCL_VDP2_VRAM)/0x20000) & 0x0007;
		*mapoffset |= mapoffsetW << shift;
	} else {
		if( scfg->pnamesize == 1){
			if(scfg->charsize == 0) {
				boundary = 0x2000;
			} else {
				boundary = 0x800;
			}
		} else {
			if(scfg->charsize == 0) {
				boundary = 0x4000;
			} else {
				boundary = 0x1000;
			}
		}
		mapoffsetW = (0x01c0 & ((scfg->plate_addr[0] - SCL_VDP2_VRAM) / boundary))
			>> 6;
		*mapoffset |= mapoffsetW << shift;
	}

	for(i = 0; i < max; i++) {
		map[i] = (0x003f & ((scfg->plate_addr[i * 2] - SCL_VDP2_VRAM)
				/ boundary));
		temp = (0x003f & ((scfg->plate_addr[i * 2 + 1] - SCL_VDP2_VRAM)
				/ boundary)) << 8;

		map[i] |= (temp & 0x3f00);
	}
//	if(SclProcess == 0)	SclProcess = 4;
//modif VBT
	if(SclProcess == 0)	SclProcess = 1;
}
//-------------------------------------------------------------------------------------------------------------------------------------
void  SCL_SetColRam(Uint32 Object, Uint32 Index,Uint32 num,void *Color)
{
	Uint16	*ram16;
	ram16   = (Uint16 *)((Uint32)SCL_COLRAM_ADDR + ((SCL_GetColRamOffset(Object) * 0x200))) + Index;
	memcpyl(ram16,(Uint16 *)Color,num*2);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void  SCL_SetColRamOffset(Uint32 Object, Uint32 Offset,Uint8 transparent)
{
    if(Object & SCL_SPR)	SCL_SET_SPCAOS(Offset);

    if( (Object & SCL_NBG0)) //|| (Object & SCL_RBG1))
    {
	SCL_SET_N0CAOS(Offset);
	if(transparent)	Scl_s_reg.dispenbl |= 0x0100;
	else		Scl_s_reg.dispenbl &= 0xfeff;
    }

    if( (Object & SCL_NBG1)) // || (Object & SCL_EXBG))
    {
	SCL_SET_N1CAOS(Offset);
	if(transparent)	Scl_s_reg.dispenbl |= 0x0200;
	else		Scl_s_reg.dispenbl &= 0xfdff;
    }

    if(Object & SCL_NBG2)
    {
	SCL_SET_N2CAOS(Offset);
	if(transparent)	Scl_s_reg.dispenbl |= 0x0400;
	else		Scl_s_reg.dispenbl &= 0xfbff;
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------
Uint32  SCL_AllocColRam(Uint32 Surface, Uint8 transparent)
{
	Uint32	i;
	for(i=0;i<8;i++)
	{
		if(SclColRamAlloc256[i]==0)
		{
			SclColRamAlloc256[i]=Surface;
			SCL_SetColRamOffset(Surface,i,transparent);
			return(SCL_COLRAM_ADDR+(512*i));
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
// VBT passage en static
static Uint32  SCL_GetColRamOffset(Uint32 Object)
{
    switch(Object){
      case SCL_SPR:
	return(SCL_GET_SPCAOS());
	break;
      case SCL_NBG0:
//      case SCL_RBG1:
	return(SCL_GET_N0CAOS());
	break;
      case SCL_NBG1:
//      case SCL_EXBG:
	return(SCL_GET_N1CAOS());
	break;
      default:
	break;
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void SCL_SetLineParam2(SclLineparam *lp)
{
	Uint32	*addr;
	addr = &Scl_n_reg.lineaddr[0];
	//*addr = (lp->line_addr / 2) & 0x0007ffff;
	*addr = (lp->line_addr >>1) & 0x0007ffff;
	SclProcess = 2; //obligatoire
}
//-------------------------------------------------------------------------------------------------------------------------------------
void SPR_Initial(Uint8 **VRAM)
{
    int     iMask;
    iMask = get_imask();
    set_imask(15);                           // interrupt disable
/*	play = 1;
	SclProcess = 1;
    SPR_WRITE_REG(SPR_W_FBCR, SPR_FBCR_MANUAL);
    SCL_ScrollShow();
	set_imask(iMask);                    // interrupt enable
    set_imask(15);                           //* interrupt disable
	SclProcess = 1;
	SPR_WRITE_REG(SPR_W_FBCR, SPR_FBCR_ERASE|0);
    SCL_ScrollShow();
	set_imask(iMask);                    // interrupt enable
    set_imask(15);                           // interrupt disable
	SclProcess = 1;	 
		SPR_WRITE_REG(SPR_W_FBCR, SPR_FBCR_ERASE|1);
    SCL_ScrollShow();
	set_imask(iMask);                    // interrupt enable
    set_imask(15);                           // interrupt disable
	SclProcess = 1;	 
	*/
	/* change frame buffer to auto change mode  */
    SPR_WRITE_REG(SPR_W_FBCR, SPR_FBCR_AUTO);

    /* trig set to auto start drawing */
    SPR_WRITE_REG(SPR_W_PTMR, 0x0002);
    *VRAM     = (Uint8*)VRAM_ADDR;
    *(*VRAM)     = 0x80;                 /* set END command to VRAM top area */
    *((*VRAM)+1) = 0x00;
    set_imask(iMask);                    /* interrupt enable             */
play = 0;

}
//-------------------------------------------------------------------------------------------------------------------------------------
void SPR_SetEraseData(Uint16 eraseData, Uint16 leftX, Uint16 topY,Uint16 rightX, Uint16 botY)
{
	SPR_WRITE_REG(SPR_W_EWDR, eraseData);      /* set erase write data */

	leftX >>= 3;
	SPR_WRITE_REG(SPR_W_EWLR, (leftX << 9) + topY); /* set erase screen left top */

	rightX++;
	rightX >>= 3;
	SPR_WRITE_REG(SPR_W_EWRR, (rightX << 9) + botY);   /* set erase screen right bottom */
}
//-------------------------------------------------------------------------------------------------------------------------------------
void SND_Init(SndIniDt *sys_ini)
{
	Uint8 *adr_sys_info_tbl;
	
	/** BEGIN ****************************************************************/

    PER_SMPC_SND_OFF();                         /* サウンドOFF               */
    POKE_W(ADR_SCSP_REG, SCSP_REG_SET); 
                                                /* SCSP共通ﾚｼﾞｽﾀ設定         */

    DmaClrZero(ADR_SND_MEM, MEM_CLR_SIZE);      // DMAメモリゼロクリア       

		CopyMem(ADR_SND_VECTOR,
                   (void *)(SND_INI_PRG_ADR(*sys_ini)),
                   SND_INI_PRG_SZ(*sys_ini));   // 68Kﾌﾟﾛｸﾞﾗﾑ転送            

    adr_sys_info_tbl = (Uint8 *)(ADR_SND_MEM + PEEK_L(ADR_SYS_TBL +
                                 ADR_SYS_INFO));
                                                /* ｼｽﾃﾑ情報ﾃｰﾌﾞﾙｱﾄﾞﾚｽ取得    */
    adr_host_int_work = (Uint8 *)(ADR_SND_MEM + PEEK_L(ADR_SYS_TBL +
                                  ADR_HOST_INT));
                                                
    adr_com_block = adr_host_int_work;  /* 現在書き込みｺﾏﾝﾄﾞﾌﾞﾛｯｸｱﾄﾞﾚｽ初期化 */

		CopyMem((void *)
                    (PEEK_L(adr_sys_info_tbl + ADR_ARA_ADR) + ADR_SND_MEM),
                   (void *)(SND_INI_ARA_ADR(*sys_ini)),
                   CHG_LONG(SND_INI_ARA_SZ(*sys_ini))); /* ｻｳﾝﾄﾞｴﾘｱﾏｯﾌﾟ転送  */
/* 1994/02/24 Start*/
    intrflag = 0;         /* 割り込みフラグの初期化 */
/* 1994/02/24 End */

    PER_SMPC_SND_ON();                          /* サウンドON                */

}
//-------------------------------------------------------------------------------------------------------------------------------------
Uint8 SND_ChgMap(Uint8 area_no)
 {
/* 1994/02/24 Start */
    if(intrflag) return(SND_RET_NSET);
    intrflag = 1;
/* 1994/02/24 End */
    if(GetComBlockAdr() == OFF) HOST_SET_RETURN(SND_RET_NSET);
    SET_PRM(0, area_no);                        /* パラメータセット          */
    SET_COMMAND(COM_CHG_MAP);                   /* コマンドセット            */

    while(PEEK_W(adr_com_block + ADR_COM_DATA)) _WAIT_();
    if(GetComBlockAdr() == OFF) HOST_SET_RETURN(SND_RET_NSET);
    SET_PRM(0, area_no);                        /* パラメータセット          */
    SET_COMMAND(COM_CHG_MAP);                   /* コマンドセット            */
    while(PEEK_W(adr_com_block + ADR_COM_DATA)) _WAIT_();
    HOST_SET_RETURN(SND_RET_SET);
}

#define DMA_SCU_END     0
//-------------------------------------------------------------------------------------------------------------------------------------
static void CopyMem(void *dst, void *src, Uint32 cnt)
{
	memcpyw(dst, src, cnt);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void DmaClrZero(void *dst, Uint32 cnt)
{
	memset(dst, 0x00, cnt);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void sndInit(void)
{
	SndIniDt 	snd_init;
	unsigned char sound_map[]={0xFF,0xFF};

#ifndef ACTION_REPLAY
	GFS_Load(GFS_NameToId(SDDRV_NAME),0,(void *)SDDRV_ADDR,SDDRV_SIZE);
	SND_INI_PRG_ADR(snd_init) = (Uint16 *)SDDRV_ADDR;
	SND_INI_PRG_SZ(snd_init)  = (Uint16)  SDDRV_SIZE;
#else
	
	SND_INI_PRG_ADR(snd_init) = (Uint16 *)snddrv;
	SND_INI_PRG_SZ(snd_init)  = (Uint16)  sizeof(snddrv);
#endif

	SND_INI_ARA_ADR(snd_init) 	= (Uint16 *)sound_map;
	SND_INI_ARA_SZ(snd_init) 	= (Uint16)sizeof(sound_map);

	SND_Init(&snd_init);
	SND_ChgMap(0);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static PcmHn createHandle(PcmCreatePara *para)
{
	PcmHn pcm;

	pcm = PCM_CreateMemHandle(para);
	if (pcm == NULL) {
		return NULL;
	}
#ifdef _30_HZ
	PCM_NotifyWriteSize(pcm, SOUNDRATE*2*QUAL);
#else
	PCM_NotifyWriteSize(pcm, RING_BUF_SIZE);//SOUNDRATE*2);
#endif
	return pcm;
}
//-------------------------------------------------------------------------------------------------------------------------------------
SndRet SND_ChgPcm(SndPcmChgPrm *cprm)
{
/* 1994/02/24 Start */
    if(intrflag) return(SND_RET_NSET);
    intrflag = 1;
/* 1994/02/24 End */
    if(GetComBlockAdr() == OFF) HOST_SET_RETURN(SND_RET_NSET);
    SET_PRM(0, SND_PRM_NUM(*cprm));
    SET_PRM(1, (SND_PRM_LEV(*cprm) << 5) | ChgPan(SND_PRM_PAN(*cprm)));
    SET_PRM(2, SND_PRM_PICH(*cprm) >> 8);
    SET_PRM(3, SND_PRM_PICH(*cprm));
    SET_PRM(4, (SND_R_EFCT_IN(*cprm) << 3) | SND_R_EFCT_LEV(*cprm));
    SET_PRM(5, (SND_L_EFCT_IN(*cprm) << 3) | SND_L_EFCT_LEV(*cprm));
    SET_COMMAND(COM_CHG_PCM_PRM);               /* コマンドセット            */
    HOST_SET_RETURN(SND_RET_SET);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static Uint8 GetComBlockAdr(void)
{
    if(*NOW_ADR_COM_DATA){              /* 以前のﾌﾞﾛｯｸが引き取り済みでないか?*/
        /* 次コマンドブロックアドレス設定処理 ********************************/
        if(NOW_ADR_COM_DATA >= (MAX_ADR_COM_DATA - SIZE_COM_BLOCK)){
                                                    /* 最大値か?            */
            return OFF;                             /* ﾌﾞﾛｯｸ空き無し      */
        }else{
            adr_com_block += SIZE_COM_BLOCK;        /* 現在ｺﾏﾝﾄﾞﾌﾞﾛｯｸｶｳﾝﾄｱｯﾌﾟ*/
            while(NOW_ADR_COM_DATA < (MAX_ADR_COM_DATA - SIZE_COM_BLOCK)){
                if(*NOW_ADR_COM_DATA){
                    adr_com_block += SIZE_COM_BLOCK;
                }else{
                    return ON;                      /* ﾌﾞﾛｯｸ空き有り         */
                }
            }
            return OFF;                             /* ﾌﾞﾛｯｸ空き無し         */
        }
    }else{
        adr_com_block = adr_host_int_work;  /* ﾌﾞﾛｯｸの先頭へ              */
        while(NOW_ADR_COM_DATA < (MAX_ADR_COM_DATA - SIZE_COM_BLOCK)){
            if(*NOW_ADR_COM_DATA){
                adr_com_block += SIZE_COM_BLOCK;
            }else{
                return ON;                          /* ﾌﾞﾛｯｸ空き有り         */
            }
        }
        return OFF;                                 /* ﾌﾞﾛｯｸ空き無し         */
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------
static Uint16 ChgPan(SndPan pan)
{
    return(((pan) < 0) ? (~(pan) + 0x10 + 1) : (pan));
}
//-------------------------------------------------------------------------------------------------------------------------------------
SndRet SND_StartPcm(SndPcmStartPrm *sprm, SndPcmChgPrm *cprm)
{
    if(intrflag) return(SND_RET_NSET);
    intrflag = 1;
/* 1994/02/24 End */
    if(GetComBlockAdr() == OFF) HOST_SET_RETURN(SND_RET_NSET);
    SET_PRM(0, SND_PRM_MODE(*sprm) | SND_PRM_NUM(*cprm));
    SET_PRM(1, (SND_PRM_LEV(*cprm) << 5) | ChgPan(SND_PRM_PAN(*cprm)));
    SET_PRM(2, SND_PRM_SADR(*sprm) >> 8);
    SET_PRM(3, SND_PRM_SADR(*sprm));
    SET_PRM(4, SND_PRM_SIZE(*sprm) >> 8);
    SET_PRM(5, SND_PRM_SIZE(*sprm));
    SET_PRM(6, SND_PRM_PICH(*cprm) >> 8);
    SET_PRM(7, SND_PRM_PICH(*cprm));
    SET_PRM(8, (SND_R_EFCT_IN(*cprm) << 3) | SND_R_EFCT_LEV(*cprm));
    SET_PRM(9, (SND_L_EFCT_IN(*cprm) << 3) | SND_L_EFCT_LEV(*cprm));
    SET_PRM(11, 0);
    SET_COMMAND(COM_START_PCM);                 /* コマンドセット            */
    HOST_SET_RETURN(SND_RET_SET);
}
//-------------------------------------------------------------------------------------------------------------------------------------
SndRet SND_StopPcm(SndPcmNum pcm_num)
{
/* 1994/02/24 Start */
    if(intrflag) return(SND_RET_NSET);
    intrflag = 1;
/* 1994/02/24 End */
    if(GetComBlockAdr() == OFF) HOST_SET_RETURN(SND_RET_NSET);
    SET_PRM(0, pcm_num);                        /* パラメータセット          */
    SET_COMMAND(COM_STOP_PCM);                  /* コマンドセット            */
    HOST_SET_RETURN(SND_RET_SET);
}
//-------------------------------------------------------------------------------------------------------------------------------------
#define CSH_CCR			(*(volatile Uint8 * )0xfffffe92)	/*	ｷｬｯｼｭｺﾝﾄﾛｰﾙﾚｼﾞｽﾀｱﾄﾞﾚｽ	*/

#define CSH_4WAY			((Uint16)0x0000)	/*	4ｳｪｲｾｯﾄｱｿｼｴｲﾃｨﾌﾞﾓｰﾄﾞ	*/
#define CSH_2WAY			((Uint16)0x0008)	/*	2ｳｪｲｾｯﾄｱｿｼｴｲﾃｨﾌﾞﾓｰﾄﾞ	*/
#define CSH_DISABLE			((Uint16)0x0000)	/*	ｷｬｯｼｭﾃﾞｨｾｰﾌﾞﾙﾓｰﾄﾞ		*/
#define CSH_ENABLE			((Uint16)0x0001)	/*	ｷｬｯｼｭｲﾈｰﾌﾞﾙﾓｰﾄﾞ			*/
#define CSH_CODE_ENABLE		((Uint16)0x0000)	/*	ｺｰﾄﾞｲﾈｰﾌﾞﾙﾓｰﾄﾞ			*/
#define CSH_CODE_DISABLE	((Uint16)0x0002)	/*	ｺｰﾄﾞﾃﾞｨｾｰﾌﾞﾙﾓｰﾄﾞ		*/
#define CSH_DATA_ENABLE		((Uint16)0x0000)	/*	ﾃﾞｰﾀｲﾈｰﾌﾞﾙﾓｰﾄﾞ			*/
#define CSH_DATA_DISABLE	((Uint16)0x0004)	/*	ﾃﾞｰﾀﾃﾞｨｾｰﾌﾞﾙﾓｰﾄﾞ		*/

#define CSH_SET_ENABLE(sw)		((Uint16)(CSH_CCR = (Uint8)((CSH_CCR & 0xfe)|(sw))))
#define CSH_SET_CODE_FILL(sw)	((Uint16)(CSH_CCR = (Uint8)((CSH_CCR & 0xfd)|(sw))))
#define CSH_SET_DATA_FILL(sw)	((Uint16)(CSH_CCR = (Uint8)((CSH_CCR & 0xfb)|(sw))))
#define CSH_SET_WAY_MODE(sw)	((Uint16)(CSH_CCR = (Uint8)((CSH_CCR & 0xf7)|(sw))))
#define CSH_SET_ACS_WAY(way)	((Uint16)(CSH_CCR = (Uint8)((CSH_CCR & 0x3f)|((way) << 6))))

void CSH_Init(Uint16 sw)
{
	Uint32 way;									/*	アクセスウェイ制御変数	*/
	Uint32 i;									/*	アクセスライン制御変数	*/
	Uint32 *adrs;								/*	ｱﾄﾞﾚｽｱﾚｲ領域先頭ｱﾄﾞﾚｽ	*/

	CSH_SET_ENABLE(CSH_DISABLE);				/*	キャッシュディセーブル	*/
	for (way = 0; way < 4; way++) {				/*	４ウェイループ			*/
		CSH_SET_ACS_WAY(way);					/*	アクセスウェイ指定		*/
		adrs = (Uint32 *)0x60000000;			/*	ｱﾄﾞﾚｽｱﾚｲ領域先頭ｱﾄﾞﾚｽ	*/
		for (i = 0; i < 64; i++) {				/*	64ラインループ			*/
			*adrs = 0;							/*	１ラインの情報クリア	*/
			adrs += 4;							/*	次のラインに進む		*/
		}										/*	end for i				*/
	}											/*	end for way				*/
	CSH_SET_WAY_MODE(sw);						/*	ウェイモード設定		*/
	CSH_SET_CODE_FILL(CSH_CODE_ENABLE);			/*	コードフィルイネーブル	*/
	CSH_SET_DATA_FILL(CSH_DATA_ENABLE);			/*	データフィルイネーブル	*/
	CSH_SET_ENABLE(CSH_ENABLE);					/*	キャッシュイネーブル	*/
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void wait_vblank(void)
{
     while((TVSTAT & 8) == 0);
     while((TVSTAT & 8) == 8);
}
//-------------------------------------------------------------------------------------------------------------------------------------
static unsigned int get_hz(void)
{
	if((TVSTAT & 1) == 0)
		return 60;
	else
		return 50;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static int __cdecl SaturnLoadRom(unsigned char* Dest, int* pnWrote, int i, int nGap,int bXor)
{
	int nRet = 0;
	char* pszFilename;
	struct BurnRomInfo ri;
	BurnDrvGetRomName(&pszFilename, i, 0);
		BurnDrvGetRomInfo(&ri, i);
/*		char toto[100];
		sprintf (toto,"ld%s T%d L%d",pszFilename,ri.nType&7,ri.nLen);
		FNT_Print256_2bpp((volatile UINT8 *)SS_FONT,(UINT8 *)toto,10,(10*i)+20);
*/

	int fid			= GFS_NameToId((Sint8 *)strupr(pszFilename));
	long fileSize	= GetFileSize(fid);
	GFS_Load(fid, 0, Dest, ri.nLen);
	pnWrote[0] = ri.nLen;
	wait_vblank();

//		char text[100];
//		sprintf(text,"%d %d %s %d ", fid,fileSize,pszFilename,ri.nLen);
//		FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)text,10,(20*i)+60);

	return nRet;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static int DoInputBlank(int bDipSwitch)
{
  int iJoyNum = 0;
  unsigned int i=0; 
  // Reset all inputs to undefined (even dip switches, if bDipSwitch==1)
  char controlName[12];
  
  DIPInfo.nDIP = 0;
  // Get the targets in the library for the Input Values
  for (i=0; i<nGameInpCount; i++)
  {
    struct BurnInputInfo bii;
    memset(&bii,0,sizeof(bii));
    BurnDrvGetInputInfo(&bii,i);
    
    //if (bDipSwitch==0 && bii.nType==2) continue; // Don't blank the dip switches
	
	if (bii.nType==BIT_DIPSWITCH)
	{
		if (DIPInfo.nDIP == 0)
		{
			DIPInfo.nFirstDIP = i;
			DIPInfo.nDIP = nGameInpCount - i;
			DIPInfo.DIPData = (struct GameInp *)malloc(DIPInfo.nDIP * sizeof(struct GameInp));
			memset(DIPInfo.DIPData,0,DIPInfo.nDIP * sizeof(struct GameInp));
		}
		DIPInfo.DIPData[i-DIPInfo.nFirstDIP].pVal = bii.pVal;
		DIPInfo.DIPData[i-DIPInfo.nFirstDIP].nType = bii.nType;
		DIPInfo.DIPData[i-DIPInfo.nFirstDIP].nConst = 0;
		DIPInfo.DIPData[i-DIPInfo.nFirstDIP].nBit = 0;
	}

	if (bii.szInfo[0]=='p')
		iJoyNum = bii.szInfo[1] - '1';	
	else
	{
	/*	if (strcmp(bii.szInfo, "diag") == 0 || strcmp(bii.szInfo, "test") == 0)
		{
			ServiceDip = bii.pVal;
		}	*/
		continue;
	}
	sprintf(controlName,"p%i coin",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][0].nBit = 4;
		GameInp[iJoyNum][0].pVal = bii.pVal;
		GameInp[iJoyNum][0].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i start",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][1].nBit = 5;
		GameInp[iJoyNum][1].pVal = bii.pVal;
		GameInp[iJoyNum][1].nType = bii.nType;
		switch (iJoyNum)
		{
			case 0:
				P1Start = bii.pVal;
			break;
			case 1:
				P2Start = bii.pVal;
			break;
		}	
	}
    else {
	sprintf(controlName,"p%i up",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][2].nBit = 0;
		GameInp[iJoyNum][2].pVal = bii.pVal;
		GameInp[iJoyNum][2].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i down",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][3].nBit = 1;
		GameInp[iJoyNum][3].pVal = bii.pVal;
		GameInp[iJoyNum][3].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i left",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][4].nBit = 2;
		GameInp[iJoyNum][4].pVal = bii.pVal;
		GameInp[iJoyNum][4].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i right",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][5].nBit = 3;
		GameInp[iJoyNum][5].pVal = bii.pVal;
		GameInp[iJoyNum][5].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 1",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][6].nBit = 6;
		GameInp[iJoyNum][6].pVal = bii.pVal;
		GameInp[iJoyNum][6].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 2",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][7].nBit = 7;
		GameInp[iJoyNum][7].pVal = bii.pVal;
		GameInp[iJoyNum][7].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 3",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][8].nBit = 8;
		GameInp[iJoyNum][8].pVal = bii.pVal;
		GameInp[iJoyNum][8].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 4",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][9].nBit = 9;
		GameInp[iJoyNum][9].pVal = bii.pVal;
		GameInp[iJoyNum][9].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 5",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][10].nBit = 10;
		GameInp[iJoyNum][10].pVal = bii.pVal;
		GameInp[iJoyNum][10].nType = bii.nType;
    }
    else {
	sprintf(controlName,"p%i fire 6",iJoyNum+1);
    if (strcmp(bii.szInfo, controlName) == 0)
    {
    	GameInp[iJoyNum][11].nBit = 11;
		GameInp[iJoyNum][11].pVal = bii.pVal;
		GameInp[iJoyNum][11].nType = bii.nType;
    }}}}}}}}}}}}
  }
  return 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------
static int InpMake(unsigned int key[])
{
	unsigned int i=0; 
	unsigned int down = 0;
/*
	if (ServiceDip)
	{
		*(ServiceDip)=ServiceRequest;
	}*/
	short joyNum=0;
//	for (joyNum=0;joyNum<numJoy;joyNum++)
	{
		for (i=0; i<12; i++)
		{
			if (GameInp[joyNum][i].pVal == NULL) continue;
			
			if ( GameInp[joyNum][i].nBit >= 0 )
			{
				down = key[joyNum] & (1U << GameInp[joyNum][i].nBit);
				
				if (GameInp[joyNum][i].nType!=1) {
					// Set analog controls to full
					if (down) *(GameInp[joyNum][i].pVal)=0xff; else *(GameInp[joyNum][i].pVal)=0x01;
				}
				else
				{
					// Binary controls
					if (down) *(GameInp[joyNum][i].pVal)=1;    else *(GameInp[joyNum][i].pVal)=0;
				}
			}
		}
	}
	for (i=0; i<(int)DIPInfo.nDIP; i++) {
		if (DIPInfo.DIPData[i].pVal == NULL)
			continue;
		*(DIPInfo.DIPData[i].pVal) = DIPInfo.DIPData[i].nConst;
	}
	if (P1P2Start)
	{
		*(P1Start) = *(P2Start) = 1;
	}
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
int InpInit()
{
  unsigned int i=0; 
  int nRet=0;
//  bInputOk = 0;
  // Count the number of inputs
 nGameInpCount=0;
 for (i=0;i<0x1000;i++) 
	  {
    nRet = BurnDrvGetInputInfo(NULL,i);
    if (nRet!=0) {   // end of input list
    	nGameInpCount=i; 
    	break; 
    }
  }

  memset(GameInp,0,12*4*sizeof(struct GameInp));
  DoInputBlank(1);

//  bInputOk = 1;
  return 0;
}

//-------------------------------------------------------------------------------------------------------------------------------------
void InpDIP()
{
	struct BurnDIPInfo bdi;
	struct GameInp* pgi;
	int i, j;
	int nDIPOffset = 0;

	// get dip switch offset 
	for (i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++)
		if (bdi.nFlags == 0xF0) {
			nDIPOffset = bdi.nInput;
			break;
		}




	// set DIP to default
	i = 0;
	j = 40;
	char bDifficultyFound = 0;
	while (BurnDrvGetDIPInfo(&bdi, i) == 0) 
	{
//		char toto[100];
//		sprintf(toto,"%2d. %02x '%s'\n", bdi.nInput, bdi.nFlags, bdi.szText);
//		FNT_Print256_2bpp((volatile Uint8 *)ss_font,(Uint8 *)toto,10,60);
		
		if (bdi.nFlags == 0xFF) 
		{
			pgi = DIPInfo.DIPData + (bdi.nInput + nDIPOffset - DIPInfo.nFirstDIP);
			pgi->nConst = (pgi->nConst & ~bdi.nMask) | (bdi.nSetting & bdi.nMask);
		}
		else
		{
			 if (bdi.nFlags == 0xFE) 
			{
				if ( bdi.szText )
				 {
					col[1]=9;
					col[2]=11;
					col[3]=11;
					FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)bdi.szText,136,j); 
					j+=10;
				 }
			}
			 if (bdi.nFlags == 0x01 && bdi.nSetting == 0x00) 
			{
				if ( bdi.szText )
				 {
					col[1]=9;
					col[2]=10;
					col[3]=10;
					FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)bdi.szText,136,j); 
					j+=10;
				 }
			}
		}
		/*		else 
		if (bdi.nFlags == 0xFE) {
			if ( bdi.szText )
				if ( ( strcmp(bdi.szText, "Difficulty") == 0  ) ||
				     ( strcmp(bdi.szText, "Game Level") == 0  )
				   ) 
			{
				FNT_Print256_2bpp((volatile Uint8 *)ss_font,(Uint8 *)"difficulty found",10,70);				
				bDifficultyFound = 1;
			}
		} else {
			if (bDifficultyFound) {
				if ( bdi.nFlags == 0x01 ) {
					// use GameScreenMode store 
					pgi = DIPInfo.DIPData + (bdi.nInput + nDIPOffset - DIPInfo.nFirstDIP);
					for (j=0; j<8; j++)
						if ((1U << j) & bdi.nMask) 
							break;
					pgi->nConst = (pgi->nConst & ~bdi.nMask) | ((0 << j) & bdi.nMask);
				}
				bDifficultyFound = 0;
			}
		}*/
		i++;
	}
	for (i=0,pgi=DIPInfo.DIPData; i<(int)DIPInfo.nDIP; i++,pgi++) {
		if (pgi->pVal == NULL)
			continue;
		*(pgi->pVal) = pgi->nConst;
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
//void InpDIPSWResetDIPs()
/*
void InpDIPx()
{
	int i = 0;
	struct BurnDIPInfo bdi;
	struct GameInp* pgi;
   int nDIPOffset = 0;

	for (i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++)
		if (bdi.nFlags == 0xF0) {
			nDIPOffset = bdi.nInput;
			break;
		}

	while (BurnDrvGetDIPInfo(&bdi, i) == 0) {
		if (bdi.nFlags == 0xFF) {
//			pgi = GameInp + bdi.nInput + nDIPOffset;
//			pgi->nConst = (pgi->nConst & ~bdi.nMask) | (bdi.nSetting & bdi.nMask);
			pgi = DIPInfo.DIPData + (bdi.nInput + nDIPOffset - DIPInfo.nFirstDIP);
			pgi->nConst = (pgi->nConst & ~bdi.nMask) | (bdi.nSetting & bdi.nMask);

		}
		i++;
	}
}
*/
//-------------------------------------------------------------------------------------------------------------------------------------
static int DrvExit()
{
	if (nBurnDrvSelect < nBurnDrvCount) 
	{
		BurnDrvExit();				// Exit the driver
	}
	BurnExtLoadRom = NULL;

//	nBurnDrvSelect = 0;			// no driver selected
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static int DoLibInit() // Do Init of Burn library driver
{
  	int nRet;
		
	nRet = BurnDrvInit();
	return (nRet) ? 3 : 0 ;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static int nDrvInit(int nDrvNum)
{
	int nRet=0;
	void (*fp)(char *);
	char drv_file[14];
//	DrvExit(); // Make sure exited
	nBurnDrvSelect = nDrvNum; // set the driver number
	BurnExtLoadRom = SaturnLoadRom;

	shared   = pDriver[nBurnDrvSelect];

	if(BurnDrvGetTextA(DRV_PARENT)==NULL)
		sprintf(drv_file,"d_%s.bin",BurnDrvGetTextA(DRV_NAME));
	else
		sprintf(drv_file,"d_%s.bin",BurnDrvGetTextA(DRV_PARENT));


    GFS_Load(GFS_NameToId(strupr(drv_file)), 0, (void *)OVLADDR, GFS_BUFSIZ_INF);
	ChangeDir(BurnDrvGetTextA(DRV_NAME));

	fp = (void *)OVLADDR;
	(*fp)(pDriver[nBurnDrvSelect]->szShortName);
	nRet=DoLibInit(); // Init the Burn library's driver

	if (nRet!=0) 
	{
		BurnDrvExit(); // Exit the driver
		return 1;
	}
	BurnExtLoadRom = SaturnLoadRom;
 /*
	ss_map = (Uint16 *)SS_MAP;
	ss_map2= (Uint16 *)SS_MAP2;
	ss_font= (Uint16 *)SS_FONT;
	cache  = (Uint8  *)SS_CACHE;
   */
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void check_exit(Uint16 data)
{
	 if(            ((data & PER_DGT_S) != 0) 
			   && ((data & PER_DGT_X) != 0) 
			   && ((data & PER_DGT_Y) != 0) 
			   && ((data & PER_DGT_Z) != 0)
	   )
		_smpc_SYSRES();

	 if(            ((data & PER_DGT_S) != 0) 
			   && ((data & PER_DGT_A) != 0) 
			   && ((data & PER_DGT_B) != 0) 
			   && ((data & PER_DGT_C) != 0)
	   )
	{
		play = 0;
		drvquit = 1;
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void do_keypad()
{
	int i;
	
	FBA_KEYPAD[0] = 0;
	FBA_KEYPAD[1] = 0;
	FBA_KEYPAD[2] = 0;
	FBA_KEYPAD[3] = 0;
//	ServiceRequest =0;
	P1P2Start = 0;

	SysDevice	*device;

	if(( device = PER_GetDeviceR( &__port[0], 0 )) != NULL )
//	device = PER_GetDeviceR( &__port[0], 0 ); 
	{
		pltriggerE[0] = pltrigger[0];
		pltrigger[0]  = PER_GetTrigger( device );
		pltriggerE[0] = (pltrigger[0]) ^ (pltriggerE[0]);
		pltriggerE[0] = (pltrigger[0]) & (pltriggerE[0]);

		check_exit(pltrigger[0]);

		for(i=0;i<14;i++)
		{
			if((pltrigger[0] & pad_asign[i])!=0)
			{
				switch(pltrigger[0] & pad_asign[i] )
				{
					case PER_DGT_U: FBA_KEYPAD[0] |= 0x0001; break;
					case PER_DGT_D: FBA_KEYPAD[0] |= 0x0002; break;
					case PER_DGT_L: FBA_KEYPAD[0] |= 0x0004; break;
					case PER_DGT_R: FBA_KEYPAD[0] |= 0x0008; break;
					case PER_DGT_B: FBA_KEYPAD[0] |= 0x0040; break;
					case PER_DGT_A: FBA_KEYPAD[0] |= 0x0080; break;
					case PER_DGT_C: FBA_KEYPAD[0] |= 0x0100; break;
					case PER_DGT_X: FBA_KEYPAD[0] |= 0x0200; break;
					case PER_DGT_Y: FBA_KEYPAD[0] |= 0x0400; break;
					case PER_DGT_Z: FBA_KEYPAD[0] |= 0x0800; break;
					case PER_DGT_TL: FBA_KEYPAD[0] |= 0x0010; break;
					case PER_DGT_TR: FBA_KEYPAD[0] |= 0x0020; break;
					case PER_DGT_S: FBA_KEYPAD[0] |= P1P2Start = 1; break;
				    default:
					break;
				}
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------
static void run_fba_emulator()
{
	nBurnSoundRate = SOUNDRATE;

	ChangeDir("GAMES");

	if (nDrvInit(nBurnDrvSelect) != 0) 
	{
		FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"Driver initialisation failed! Likely causes are:",1,180);
		FNT_Print256_2bpp((volatile Uint8 *)SS_FONT,(Uint8 *)"- Corrupt/Missing ROM(s)\n- I/O Error\n- Memory error\n\n",1,190);
//		while(1);

	}
	InpInit();
	InpDIP();
	play = 1;
	drvquit = 0;
//	PCM_Start(pcm);
	PCM_MeSetVolume(pcm,255);
	PCM_DrvChangePcmPara(pcm,-1,-1);
//	PER_SMPC_SND_ON();
	SetVblank(); // a garder

	while (play)
	{
		BurnDrvFrame();
		SCL_SetLineParam2(&lp);
		_spr2_transfercommand();
		frame_x++;

//		 if(frame_x>=frame_y)
//			wait_vblank();
	}

	if(drvquit==1)
	{
		DrvExit();
//		_smpc_SSHOFF();
		initSaturn();
//		BurnLibInit();
		BurnDrvAssignList();
//		nBurnDrvSelect=0;
	}
	asm("nop\n");
}
//-------------------------------------------------------------------------------------------------------------------------------------
void initSprites(int sx,int sy,int sx2, int sy2,int lx,int ly)
{
	//SPR_InitSlaveSH();
	_spr2_initialize();

	SPR_WRITE_REG(SPR_W_TVMR, 0x0007 & SPR_TV_NORMAL);//SPR_TV_ROT8);//SPR_TV_NORMAL);
	SPR_SetEraseData( 0x0000, 0, 0, sx, sy );
//	SPR_SetEraseData( RGB(31,0,0), 0, 0, sx, sy );
	
// 	CSH_Init(CSH_4WAY);
//	SPR_InitSlaveSH();
	memset(smsSprite,0,sizeof(SprSpCmd)*131);
    smsSprite[0].control    = (JUMP_NEXT | FUNC_SCLIP);

    smsSprite[0].cx         = sx;
    smsSprite[0].cy         = sy;
    smsSprite[0].dx         = sx2;
    smsSprite[0].dy         = sy2;

    smsSprite[1].control    =  (JUMP_NEXT | FUNC_LCOORD);

    smsSprite[1].ax         = lx;
    smsSprite[1].ay         = ly;

    smsSprite[2].control    = (JUMP_NEXT | FUNC_UCLIP);

	smsSprite[2].cx         = sx;
	smsSprite[2].cy         = sy;

	smsSprite[3].control    = CTRL_END;

	_spr2_transfercommand();
}
//-------------------------------------------------------------------------------------------------------------------------------------
void drawWindow(unsigned  int l1,unsigned  int l2,unsigned  int l3,unsigned  int vertleft,unsigned  int vertright)
{
	Uint16 *VRAM;    
	int x,j;  
	col[0]=10;
    VRAM = (Uint16 *)SS_FONT;
	
// barre horizontale haut
	for( x = 0; x < l1; x++ ) // 2 lignes
    {
		for( j = 0; j < 64; j++ ) *VRAM++ = 0xaaaa;
	}

	for( x = 0; x < l2; x++ ) 
    {
		for( j = 0; j < vertleft			  ; j++ ) *VRAM++ = 0xaaaa; // barre verticale gauche
		for( j = 0; j < 128-vertleft-vertright; j++ ) *VRAM++ = 0x0000; // noir
		for( j = 0; j < vertright			  ; j++ ) *VRAM++ = 0xaaaa; // barre verticale droite
	}
// barre horizontale bas
	for( x = 0; x < l3; x++ ) 
    {
		for( j = 0; j < 64; j++ ) *VRAM++ = 0xaaaa;
	}
	
		play=0;
		SclProcess = 1;
		SetVblank();
		wait_vblank();
		play=1;
}
//-------------------------------------------------------------------------------------------------------------------------------------

