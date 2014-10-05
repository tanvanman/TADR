// MinimapHandler.cpp: implementation of the CMinimapHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "oddraw.h"

#include "iddrawsurface.h"

#include "MinimapHandler.h"
#include "whiteboard.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMinimapHandler::CMinimapHandler()
{
   int *PTR = (int*)0x00511de8;
   TAdynmem = (TAdynmemStruct*)(*PTR);

   IDDrawSurface::OutptTxt ( "New CMinimapHandler");
}

CMinimapHandler::~CMinimapHandler()
{

}

void CMinimapHandler::InitMinimap(SharedMem *DDDSharedMem)
{

	this->DDDSharedMem = DDDSharedMem;

	bool bExists;

   	DDDSharedMem->RadarpicX = TAdynmem->RadarFinal->XSize;
	DDDSharedMem->RadarpicY = TAdynmem->RadarFinal->YSize;

   //create the mapping to the file
	MemMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
                               NULL,
                               PAGE_READWRITE,
                               0,
								256*256,
                               "Radarpic");

   //see weather this is the first time this file has been mapped to
	bExists = (GetLastError() == ERROR_ALREADY_EXISTS);

   //Map a view into the Mapped file
	RadarPic = MapViewOfFile(MemMap,
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            0);

  if (!bExists)
     memset(RadarPic, NULL, TAdynmem->RadarFinal->XSize*TAdynmem->RadarFinal->YSize);
}

void CMinimapHandler::DeinitMinimap()
{
	if(RadarPic!=NULL)
		UnmapViewOfFile(RadarPic);
	RadarPic = NULL;

	if(MemMap != NULL)
		CloseHandle(MemMap);
	MemMap = NULL;
}

void CMinimapHandler::FrameUpdate()
{
	int *bufin = (int*)TAdynmem->RadarFinal->PixelPTR;
	int *bufout = (int*)RadarPic;

	for(int i=0; i<3969; i++)
	{
		bufout[i] = bufin[i];
	}

	((AlliesWhiteboard*)LocalShare->Whiteboard)->DrawMinimapMarkers((char*)bufout, TAdynmem->RadarFinal->XSize, true);
}