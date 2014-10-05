#include "oddraw.h"
#include "iddrawsurface.h"
#include "unitrotate.h"

char *NewMem;

CUnitRotate::CUnitRotate()
{
  LocalShare->UnitRotate = this;
  IDDrawSurface::OutptTxt ( "New CUnitRotate");
}

CUnitRotate::~CUnitRotate()
{

}

bool CUnitRotate::Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch(Msg)
    {
    case WM_KEYDOWN:
      if(wParam == 81 && (GetAsyncKeyState(17)&0x8000)>0) // ctrl + q
        {
        Rotate();
        //SetMem();
        return true;
        }
      break;
    }


  return false;
}

void CUnitRotate::SetMem()
{
  NewMem = new char[500000];
  int Add = (int)&NewMem;
  WriteProcessMemory(GetCurrentProcess(), (void*)0x42E46A, &Add, 4, NULL);
  memset(NewMem, 500000, 0);

}

void CUnitRotate::Rotate()
{
  //FixAck();

  int UnitOffset = 0x118;

  int *PTR1 = (int*)0x511de8;
  int *UnitPTR = (int*)((*PTR1)+0x1b8e+0x3c);

  int NumUnits = *((int*)(*PTR1+0x1ca7))& 0x0000ffff;

  int i=0;
  int BeginUnitsArray_p  = 0;

  while(BeginUnitsArray_p <NumUnits)
    {
    //short *BeginUnitsArray_p  = (short*)(*UnitPTR + 2 + i*UnitOffset);
    char *UnitDead = (char*)(*UnitPTR + 247 + i*UnitOffset);
    char *Builder = (char*)(*UnitPTR + 31 + i*UnitOffset);
    short *XPos = (short*)(*UnitPTR + 0x6c + i*UnitOffset);
    short *YPos = (short*)(*UnitPTR + 0x74 + i*UnitOffset);
    int *IsUnit = (int*)(*UnitPTR + 0 + i*UnitOffset);
    char *UnitSelected = (char*)(*UnitPTR + 272 + i*UnitOffset);
    //char *Working = (char*)(*UnitPTR + 186 + i*UnitOffset);
    //int *Moving = (int*)(*UnitPTR + 208 + i*UnitOffset);
    int *UnitOrderPTR = (int*)(*UnitPTR + 92 + i*UnitOffset);

    int *DefiPTR = (int*)(*UnitPTR + 146 + i*UnitOffset);
    int *DefPTR = (int*)*DefiPTR;



    if(*UnitDead!=0 && *UnitDead!=1)
      {

      if(strcmp((char*)DefPTR, "Vehicle Plant") == 0)
        {
        char *NewUnitDef = new char[0x249];
        memcpy(NewUnitDef, (char*)*DefiPTR, 0x249);
        *DefiPTR = (int)NewUnitDef;

        char *NewYardMap = new char[48];
        int *YardMap = (int*)(NewUnitDef + 334);
        memcpy(NewYardMap, (char*)*YardMap, 48);
        NewYardMap[2] = 0x2d;
        NewYardMap[3] = 0x2d;
        NewYardMap[4] = 0x2d;
        NewYardMap[5] = 0x2d;
        //for(int x=0; x<480; x++)
        //  NewYardMap[x] = 0x2D;
        int *tmpPTR = (int*)YardMap;
        *tmpPTR = (int)NewYardMap;

        IDDrawSurface::OutptTxt(NewUnitDef);
        }

      BeginUnitsArray_p ++;
      }
    i++;
    if(i == 5000)
      {
      //LastNum = 0;
      return;
      }
    }

}
