#include "cgraphic.h"
#include <math.h>
#pragma warning(disable:4244)

CGraphic::CGraphic()
{
  GenerateLookupTables();
}

CGraphic::~CGraphic()
{

}

void CGraphic::GenerateLookupTables()
{
  for (int ang = 0; ang < 360; ang++)
    {
    float theta = (float)ang*PI/(float)180;
    cos_look[ang] = cos(theta);
    sin_look[ang] = sin(theta);
    }
}

void CGraphic::DrawRotateRect(LPDIRECTDRAWSURFACE DestSurf, int x, int y, int Rotation)
{
  int Length = 1+(Rotation/3);
  int x1,x2,x3,x4;
  int y1,y2,y3,y4;
  int V;

  V = Rotation-45;
  x1 = x-(cos_look[abs(V)%360]*Length);
  y1 = y+(sin_look[abs(V)%360]*Length);
  V = Rotation+45;
  x2 = x-(cos_look[abs(V)%360]*Length);
  y2 = y+(sin_look[abs(V)%360]*Length);
  V = Rotation+135;
  x3 = x-(cos_look[abs(V)%360]*Length);
  y3 = y+(sin_look[abs(V)%360]*Length);
  V = Rotation+225;
  x4 = x-(cos_look[abs(V)%360]*Length);
  y4 = y+(sin_look[abs(V)%360]*Length);


}
