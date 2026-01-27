#ifndef whiteboardH
#define whiteboardH

#include "elementhandler.h"
#include "IRenderer.h"
#include <chrono>
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
		MMHS(unsigned short _XPos, unsigned short _YPos) :
			XPos(_XPos), YPos(_YPos)
		{
			auto now = std::chrono::system_clock::now();
			timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
		}

		unsigned getFrameNumberMillisecs()
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			return t - timestamp;
		}

		unsigned short XPos;
		unsigned short YPos;
		long long timestamp;
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

	// Markers/lines received from remote peers have no player ID, only a color. We'll assume its their initial color at launch,
	// which is true if peer uses this version of dll, or previous versions but haven't issued a +logo command.
	// Then we can use that initial colour as a lookup for the current colour
	int PlayerNumbersByInitialColor[10];
	char LocalPlayerColorAtLaunch;	// 0..10

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

	// supress TextInputChar when a whiteboard marker has just been placed
	// and enable again when a keybaord key is actually pressed
	bool enableTextInputChar;

    int SizeX;
	int SizeY;
	int MidX;
	int MidY;

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
    void DeleteMarker(int X, int Y);
    void RestoreAll();
    void EchoMarker(char *cText, char color);
    void EreaseArea(int x, int y);
	GraphicElement *GetTextElementAt(int x, int y, int Area);

    void DrawRotateRect(int x, int y, int Rotation, char *VidBuf, int Pitch);

    void MouseMove(int XStart, int XEnd, int YStart, int YEnd);

    void ReceiveMarkers();
    void SendMarkers();

    void DrawMarkers(LPDIRECTDRAWSURFACE DestSurf);
    void DrawTextMarker(LPDIRECTDRAWSURFACE DestSurf, int X, int Y, char *cText, char Color);

	PlayerStruct* GetPlayer(int n);
	char GetLocalPlayerColor();	// 0..10
	char LookupPlayerCurrentColor(char colorAtLaunch /* 0..10 */);	// 0..10
	void DebugPlayerNumberFromInitialColor();
	void InitialisePlayerNumberFromInitialColor();

	static bool IsInGameArea(LPARAM lParam);
	static bool IsInGameArea(LONG x, LONG y);

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
