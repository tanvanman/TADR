#ifndef maprectH
#define maprectH

#define RectColor 1

#define RectHeight 536
#define RectWidth 672

class CMapRect
{
  private:
    LPDIRECTDRAWSURFACE lpMapRect;
    int GetMapMaxX();
    int GetMapMaxY();
    void PaintRect();
    int Width;
    int Height;
    int *MapX;
    int *MapY;
    int *MapSizeX;
    int *MapSizeY;
    int XOffset;
    int YOffset;
    int MiniMapWidth;
    int MiniMapHeight;
    int WidthAdd;
    int HeightAdd;
  public:
    CMapRect(BOOL VidMem);
    ~CMapRect();
    void Blit(LPDIRECTDRAWSURFACE DestSurf);
    void LockBlit(char *VidBuf, int Pitch);
    int WorldToMiniX(int x);
    int WorldToMiniY(int y);

};

#endif
