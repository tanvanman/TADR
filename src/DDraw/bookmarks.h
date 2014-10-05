#include "oddraw.h"

class CBookmarks
{
  private:
  public:
    CBookmarks();
    ~CBookmarks();
    bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void Blit(LPDIRECTDRAWSURFACE DestSurf);
};