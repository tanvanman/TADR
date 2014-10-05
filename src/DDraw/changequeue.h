#ifndef changequeueH
#define changequeueH

class CChangeQueue
{
  public:
    CChangeQueue();
    ~CChangeQueue();
    bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void Blit(LPDIRECTDRAWSURFACE DestSurf);

};

#endif
