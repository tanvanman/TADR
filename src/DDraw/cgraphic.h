#ifndef graphicH
#define graphicH

#include "oddraw.h"

const double PI = 3.1415926535;
class CGraphic
{
  private:
    float cos_look[360];
    float sin_look[360];
    void GenerateLookupTables();

  public:
    CGraphic();
    ~CGraphic();
    void DrawRotateRect(LPDIRECTDRAWSURFACE DestSurf, int x, int y, int Rotation);
};

#endif
