#include "burnint.h"
#include "m6809_intf.h"

//#define MAX_CPU		1

//int nM6809Count = 0;
//static int nActiveCPU = 0;
unsigned char * pMemMap[0x100 * 3];
//unsigned char ** pMemMap100;
unsigned char ** pMemMap100;
unsigned char ** pMemMap200;
pReadByteHandler ReadByte;
pWriteByteHandler WriteByte;
pReadOpHandler ReadOp;
pReadOpArgHandler ReadOpArg;
m6809_Regs reg;

static unsigned char M6809ReadByteDummyHandler(unsigned short d)
{
	return 0;
}

static void M6809WriteByteDummyHandler(unsigned short d, unsigned char value)
{
}

static unsigned char M6809ReadOpDummyHandler(unsigned short d)
{
	return 0;
}

static unsigned char M6809ReadOpArgDummyHandler(unsigned short d)
{
	return 0;
}

void M6809Reset()
{
	m6809_reset();
}

int M6809Init(int num)
{
	int j;

//	memset(m6809CPUContext, 0, num * sizeof(M6809Ext));
	ReadByte = M6809ReadByteDummyHandler;
	WriteByte = M6809WriteByteDummyHandler;	
	ReadOp = M6809ReadOpDummyHandler;
	ReadOpArg = M6809ReadOpArgDummyHandler;
	
		for (j = 0; j < (0x0100 * 3); j++) {
//			m6809CPUContext[i].pMemMap[j] = NULL;
			pMemMap[j] = NULL;
		}
//	}
	pMemMap100 = (UINT8 *)&pMemMap[0x100];
	pMemMap200 = (UINT8 *)&pMemMap[0x200];
	
	m6809_init(NULL);
	return 0;
}

void M6809Exit()
{
//	nM6809Count = 0;

//	free(m6809CPUContext);
//	m6809CPUContext = NULL;
}

void M6809Open(int num)
{
//	nActiveCPU = num;
	
//	m6809_set_context(&m6809CPUContext[nActiveCPU].reg);
	m6809_set_context(&reg);
	
//	nM6809CyclesTotal = nM6809CyclesDone[nActiveCPU];
}

void M6809Close()
{
//	m6809_get_context(&m6809CPUContext[nActiveCPU].reg);
m6809_get_context(&reg);
	
//	nM6809CyclesDone[nActiveCPU] = nM6809CyclesTotal;
//	nActiveCPU = -1;
}
   /*
int M6809GetActive()
{
	return nActiveCPU;
}	*/

void M6809SetIRQ(int vector, int status)
{
	if (status == M6809_IRQSTATUS_NONE) {
		m6809_set_irq_line(vector, 0);
	}
	
	if (status == M6809_IRQSTATUS_ACK) {
		m6809_set_irq_line(vector, 1);
	}
	
	if (status == M6809_IRQSTATUS_AUTO) {
		m6809_set_irq_line(vector, 1);
		m6809_execute(0);
		m6809_set_irq_line(vector, 0);
		m6809_execute(0);
	}
}

void M6809RunEnd()
{

}

int M6809MapMemory(unsigned char* pMemory, unsigned short nStart, unsigned short nEnd, int nType)
{
	unsigned short i;
	unsigned char cStart = (nStart >> 8);
//	unsigned char **pMemMap = m6809CPUContext[nActiveCPU].pMemMap;

	for (i = cStart; i <= (nEnd >> 8); ++i) {
		if (nType & M6809_READ)	{
			pMemMap[0     + i] = pMemory + ((i - cStart) << 8);
		}
		if (nType & M6809_WRITE) {
			pMemMap100[0 + i] = pMemory + ((i - cStart) << 8);
		}
		if (nType & M6809_FETCH) {
			pMemMap200[0 + i] = pMemory + ((i - cStart) << 8);
		}
	}
	return 0;

}

void M6809MapMemory2(unsigned char* pMemory, unsigned short nStart, unsigned short nEnd) //, int nType)
{
	unsigned short i;
	nStart >>= 8;
	nEnd >>= 8;

	for (i = nStart; i <= nEnd; ++i) 
	{
		pMemMap200[i] = pMemMap[i] = pMemory + ((i - nStart) << 8);
	}
}

void M6809SetReadByteHandler(unsigned char (*pHandler)(unsigned short))
{
//	m6809CPUContext[nActiveCPU].ReadByte = pHandler;
	ReadByte = 	pHandler;
}

void M6809SetReadOpHandler(unsigned char (*pHandler)(unsigned short))
{
//	m6809CPUContext[nActiveCPU].ReadOp = pHandler;
	ReadOp = pHandler;
}

void M6809SetReadOpArgHandler(unsigned char (*pHandler)(unsigned short))
{
//	m6809CPUContext[nActiveCPU].ReadOpArg = pHandler;
	ReadOpArg = pHandler;
}

unsigned char M6809ReadByte(unsigned short Address)
{
	// check mem map
//	unsigned char * pr = m6809CPUContext[nActiveCPU].pMemMap[0x000 | (Address >> 8)];
	unsigned char * pr = pMemMap[0x000 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	return DrvGngM6809ReadByte(Address);
}

void M6809WriteByte(unsigned short Address, unsigned char Data)
{
	// check mem map
//	unsigned char * pr = pMemMap[0x100 | (Address >> 8)];
	unsigned char * pr = pMemMap100[(Address >> 8)];
	if (pr != NULL) {
		pr[Address & 0xff] = Data;
		return;
	}
	DrvGngM6809WriteByte(Address, Data);
}

unsigned char M6809ReadOp(unsigned short Address)
{
	// check mem map
//	unsigned char * pr = m6809CPUContext[nActiveCPU].pMemMap[0x200 | (Address >> 8)];
//	unsigned char * pr = pMemMap[0x200 | (Address >> 8)];
	unsigned char * pr = pMemMap200 [(Address >> 8)];

	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	else
	// check handler
	//if (m6809CPUContext[nActiveCPU].ReadOp != NULL) {
//		return m6809CPUContext[nActiveCPU].ReadOp(Address);
		return ReadOp(Address);
	//}
	
	return 0;
}

unsigned char M6809ReadOpArg(unsigned short Address)
{
	// check mem map
//	unsigned char * pr = m6809CPUContext[nActiveCPU].pMemMap[0x000 | (Address >> 8)];
	unsigned char * pr = pMemMap[0x000 | (Address >> 8)];
	if (pr != NULL) {
		return pr[Address & 0xff];
	}
	else
	// check handler
	//if (m6809CPUContext[nActiveCPU].ReadOpArg != NULL) {
//		return m6809CPUContext[nActiveCPU].ReadOpArg(Address);
		return ReadOpArg(Address);
	//}
	
	return 0;
}

