#ifndef whiteboardH
#define whiteboardH

#include "elementhandler.h"
#include "IRenderer.h"
#include <deque>

#define InputBoxHeight 32
#define InputBoxWidth 350

#define TextMarkerWidth 10
#define TextMarkerHeight 10

#define PacketGraphicLine 1
#define PacketGraphicMarker 2
#define PacketGraphicText 3
#define PacketDeleteMarker 4
#define PacketTextChanged 5
#define PacketTextMoved 6

#define PacketDeleteOn 10
#define PacketDeleteArea 11


class AlliesWhiteboard
{
  private:

	#pragma pack(1)
    /*struct Pts
      {
      char Type;
	  char Unused1;
      unsigned short x;
      unsigned short y;
      };
    struct PtC : Pts
      {
      char Color;
	  char Unused2;
      };
    struct PtL : PtC
      {
      unsigned short x2;
      unsigned short y2;
      };*/
    struct Pts
      {
      char Type;
	  char Unused1;
      unsigned short x;
      unsigned short y;
      };
    struct PtC
      {
      char Type;
	  char Unused1;
      unsigned short x;
      unsigned short y;
	  char Color;
      };
    struct PtL
      {
      char Type;
	  char Unused1;
      unsigned short x;
      unsigned short y;
      char Color;
	  char Unused2;
      unsigned short x2;
      unsigned short y2;
      };

	#pragma pack()

    struct MMHS
      {
      unsigned short XPos;
      unsigned short YPos;
      char State;
      int SubState;
      };

    float cos_look[360];
    float sin_look[360];
    void GenerateLookupTables();

    int *MapX;
    int *MapY;
    bool Disabled;
    int VirtualKeyCode;
    bool WBKeyDown;
    int DBLClickTime;
    char *PlayerColor;

    CElementHandler ElementHandler;
    GraphicElement *CurrentElement;

    LPDIRECTDRAWSURFACE lpInputBox;
    LPDIRECTDRAWSURFACE lpSmallCircle;

    int LastMouseX, LastMouseY;
    int MarkerX, MarkerY;
    bool InputShown;
    int InputX, InputY;
    int LastMarkerX, LastMarkerY;
    char Text[100];

    int SizeX;
	 int SizeY;
	 int MidX;
	 int MidY;

	 int PlayerDotColors[PLAYERNUM];
	 int PerPlayerMarkerWidth;
	 int PerPlayerMarkerHeight;
	 CHAR PlayerMarkerPcx[256];
	 int PlayerMarkerBackground;


    std::deque<GraphicElement*> PacketHandler;
    std::deque<MMHS> MinimapMarkerHandler;

    void DrawTextInput(LPDIRECTDRAWSURFACE DestSurf);
    void TextInputKeyDown(int Key);
    void TextInputChar(char C);
    void AddTextMarker();
    void AddTextMarker(int X, int Y, char *cText, char C);
    void DeleteMarker(int X, int Y);
    void RestoreAll();
    void EchoMarker(char *cText);
    void EreaseArea(int x, int y);
    GraphicElement *GetTextElementAt(int x, int y, int Area);

    void DrawRotateRect(int x, int y, int Rotation, char *VidBuf, int Pitch);

    void MouseMove(int XStart, int XEnd, int YStart, int YEnd);

    void ReceiveMarkers();
    void SendMarkers();

    void DrawMarkers(LPDIRECTDRAWSURFACE DestSurf);
    void DrawTextMarker(LPDIRECTDRAWSURFACE DestSurf, int X, int Y, char *cText, char Color);

  public:
    AlliesWhiteboard(BOOL VidMem);
    ~AlliesWhiteboard();
    void Blit(LPDIRECTDRAWSURFACE DestSurf);
    void LockBlit(char *VidBuf, int Pitch);
    bool Message(HWND WinProcWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void Set(int iKeyCode);
    int GetMapX();
    int GetMapY();
    void InstantScrollTo(int x, int y);

    void ScrollToCenter(int x, int y);

    int min_clip_y;
  	 int min_clip_x;
	 int max_clip_x;
	 int max_clip_y;

    void DrawLine(int x1, int y1, int x2, int y2, char C, char *VidBuf, int Pitch);
    void DrawFreeLine(int x1, int y1, int x2, int y2, char C, char *VidBuf, int Pitch);
    void Line(int x1, int y1, int x2, int y2, byte Colour, char *VidBuf, int Pitch);
    int Clip_Line(int &x1,int &y1,int &x2, int &y2);
    int Draw_Line(int x0, int y0, int x1, int y1, UCHAR color, UCHAR *vb_start, int lpitch);

	void DrawMinimapMarkers(char *VidBuf, int Pitch, bool Receive);

	void GetMarkers(MarkerArray *Markers);
};

#endif
