#include "oddraw.h"
#include "tamem.h"
#include "taHPI.h"
#include "tafunctions.h"

#include "hook/etc.h"
#include "hook/hook.h"


_TAHPI * TAHPI;

//-----

_fopen_HPI fopen_HPI= (_fopen_HPI)0x4BB2E0;

_read_HPI read_HPI= (_read_HPI)0x04BB7C0;

_fclose_HPI fclose_HPI= (_fclose_HPI)0x04BB5D0;

_filelength_HPI filelength_HPI= (_filelength_HPI)0x04BBC40;

_filelen_HPI filelen_HPI= (_filelen_HPI)0x04BBD00;

_readfile_HPI readfile_HPI= (_readfile_HPI)0x004BBE50;

_InitTAHPIAry InitTAHPIAry= (_InitTAHPIAry)0x41D4C0;


_TAHPI::_TAHPI (BOOL Inited)
{
	TADontInit_ISH= NULL;
	if (!Inited)
	{

		*reinterpret_cast<DWORD *>(0x051F320+ 0x618)= 0;
		*reinterpret_cast<DWORD *>(0x051F320+ 0x61c)= 0;
		*reinterpret_cast<DWORD *>(0x51FBD0)= 0x051F320;
		*reinterpret_cast<DWORD *>(0x51F320)= 0x400000;

		InitTAPath ( );
		Init_srand ( 30);

		*reinterpret_cast<DWORD *>(0x50289C)= 1;

		InitTAHPIAry ( );

		//
		
		TADontInit_ISH= new InlineSingleHook ( TADontInit_Addr , 14, INLINE_5BYTESLAGGERJMP, TADontInit);
	}
}

_TAHPI::~_TAHPI ()
{
	if (NULL!=TADontInit_ISH)
	{
		delete TADontInit_ISH;
		TADontInit_ISH= NULL;
	}
}

POPENTAFILE _TAHPI::fopen(char *FilePath, char *Mode)
{
	return fopen_HPI ( FilePath, Mode);
}
int _TAHPI::read ( POPENTAFILE TAFile , void *DstBuf, int len)
{
	return read_HPI ( TAFile, DstBuf, len);
}
int _TAHPI::fclose (POPENTAFILE TAFile)
{
	return fclose_HPI ( TAFile);
}
unsigned int _TAHPI::filelen (POPENTAFILE TAFILE)
{
	return filelen_HPI ( TAFILE);
}
unsigned int _TAHPI::filelength (char *FilePath)
{
	return filelength_HPI ( FilePath);
}
char * _TAHPI::readfile (char * FilePath, unsigned int *ReadLen_Ptr)
{
	return readfile_HPI ( FilePath, ReadLen_Ptr);
}

void _TAHPI::free_readfile ( char * mem_ptr)
{
	freeTAMem ( mem_ptr);
}

int __stdcall TADontInit(PInlineX86StackBuffer X86StrackBuffer)
{
	X86StrackBuffer->Eax|= 1;
	return X86STRACKBUFFERCHANGE;
}
