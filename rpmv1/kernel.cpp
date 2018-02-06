//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "kernel.h"
#include <circle/string.h>
#include <circle/debug.h>
#include <circle/memio.h>
#include <circle/bcm2835.h>
#include <circle/gpiopin.h>
#include <circle/util.h>
#include <assert.h>

#define RPMV_D0	(1 << 0)
#define MD00_PIN	0
#define RA08	(1 << 8)
#define RA09	(1 << 9)
#define RA10	(1 << 10)
#define RA11	(1 << 11)
#define RA12	(1 << 12)
#define RA13	(1 << 13)
#define RA14	(1 << 14)
#define RA15	(1 << 15)
#define LE_A	(1 << 16)
#define LE_B	(1 << 17)
#define RCLK	(1 << 25)
#define RWAIT	(1 << 24)
#define RINT	(1 << 20)
#define RBUSDIR (1 << 22)

#define SLTSL	RA11
#define MREQ	RA10
#define IORQ	RA12
#define RD		RA08
#define WR		RA14
#define RESET	RA13

#define DRIVE		"SD:"
#define FILENAME	"/msx.rom" 

extern CKernel Kernel;
static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED)	
{
	m_ActLED.Blink (1);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}
	
	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	} 
	
	if (bOK)
	{
//		bOK = m_EMMC.Initialize ();
	}
	m_Screen.Write("RPMV V1\n", 10);
	return bOK;
}
#define GPIO (read32 (ARM_GPIO_GPLEV0))
#define GPIO_CLR(x) write32 (ARM_GPIO_GPCLR0, x)
#define GPIO_SET(x) write32 (ARM_GPIO_GPSET0, x)

char * CKernel::fnRPI_FILES(char *drive, char *pattern)
{
	// Show contents of root directory
	DIR Directory;
	FILINFO FileInfo;
	CString FileName;
	FRESULT Result = f_findfirst (&Directory, &FileInfo,  DRIVE "/", "*.tap");
	int len = 0, len2= 0, length = 0;
	memset(files, 0, 256*256);
	memset(files2, 0, 256*256);
	for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
	{
		if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
		{
			strcpy(files+len, FileInfo.fname);
			strcpy(files2+len2, FileInfo.fname);
			length = strlen(FileInfo.fname);
			len += length;
			len2 += length > 24 ? 24 : length;
			*(files+(len++))='\\';
			*(files2+(len2++))='\\';
		}

		Result = f_findnext (&Directory, &FileInfo);
	}
	FileName.Format ("%s\n", files);
	m_Screen.Write ((const char *) FileName, FileName.GetLength ());			
	return files;
}
unsigned char ROM[] = {
#include "rom.c"
};	

TShutdownMode CKernel::Run (void)
{

	unsigned short status;
	int rd;
	CSpinLock spinlock;
	CString FileName;
	FRESULT Result;
	int addr;
//	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
//	m_Logger.Write (FromKernel, LogNotice, "SPC-1000 Extension");	
	
	// Mount file system
	if (f_mount (&m_FileSystem, DRIVE, 1) != FR_OK)
	{
//		m_Logger.Write (FromKernel, LogPanic, "Cannot mount drive: %s", DRIVE);
	} 
	
	// Create file and write to it
	
#if 0
	FIL File;
	Result= f_open (&File, DRIVE FILENAME, FA_READ | FA_OPEN_EXISTING);
	if (Result != FR_OK)
	{
		//m_Logger.Write (FromKernel, LogPanic, "Cannot open file: %s", FILENAME);
	}
	else
	{
		FileName.Format ("loading file: %s\n", FILENAME);
		m_Screen.Write ((const char *) FileName, FileName.GetLength ());
	}
	unsigned nBytesRead;
	f_read(&File, ROM, sizeof ROM, &nBytesRead);
	f_close (&File);
#endif	
	m_Screen.Write("completed\n", 10);
	spinlock.Acquire();
	GPIO_SET(LE_B);
	GPIO_CLR(LE_A);
	write32 (ARM_GPIO_GPFSEL0, 0x249249);
	write32 (ARM_GPIO_GPFSEL0+4, 0x900000);
	write32 (ARM_GPIO_GPFSEL0+8, 0);
	GPIO_CLR(0xff);
//	spinlock.Release();
	while(1)
	{
#if 0		
		if (!(GPIO & RESET))
		{
			datain = 0;
			dataout = 0;
			cflag = p = q = 0;
			readsize = 0;
			cmd = 0;
			//Message.Format ("RESET\n");
			//m_Screen.Write ((const char *) Message, Message.GetLength ());
			while(!(GPIO & RPSPC_RST));
		}
#endif	
		GPIO_SET(LE_B);
		GPIO_CLR(LE_A);
		addr = GPIO & 0xffff;
		GPIO_SET(LE_A);
		GPIO_CLR(LE_B);
		if (!(GPIO & (SLTSL|MREQ)))
		{
			status = GPIO & 0xffff;
			m_Screen.printf("addr=%04x\n", addr);
			rd = status & RD;
			if (!rd)
			{
				GPIO_CLR(0xff);
				GPIO_SET(ROM[addr - 0x4000]);
			}
			while(!(GPIO & (SLTSL|MREQ)));
		}
	}
//	spinlock.Release();
	return ShutdownReboot;
}
