#pragma once
#pragma pack(1)
struct _GAFFrame;
struct _GAFENTRY;
typedef struct _GafHeader 
{
	unsigned int Signature;//0x10100
	unsigned int Entries;//number
	unsigned int Always0;
}GafHeader, * PGafHeader;


typedef struct _GAFENTRY
{
	_GAFFrame * PtrFrame;// this one will reloc to own PGAFFrame on running time as well.
	int Animated;   ////2 = animated,5= animated,  10 = fixed.
} GAFENTRY;

typedef struct _GAFSequence
{
	unsigned short Frames;
	unsigned int Signature;//00000001
	unsigned short Padding;
	char Name[32];
	GAFENTRY PtrFrameAry[1];
}GAFSequence, * PGAFSequence;

typedef struct _GAFFrame
{
	unsigned short Width;
	unsigned short Height;
	unsigned short xPosition;
	unsigned short yPosition;
	unsigned char Background;//0x9
	unsigned char Compressed; //
	unsigned short FramePointers;//0x0
	unsigned int Unknown0;//0x0 ??
	unsigned char * PtrFrameBits;
	unsigned int FPS;
} GAFFrame, * PGAFFrame;




#pragma pack ()
LPDIRECTDRAWSURFACE CreateSurfByGafFrame (LPDIRECTDRAW LpDD, PGAFFrame Cursor_P, bool VidMem);
LPDIRECTDRAWSURFACE CreateSurfByGafSequence (LPDIRECTDRAW LpDD, PGAFSequence Cursor_P, bool VidMem);
LPBYTE CopyGafSequenceToBits (LPBYTE GafBits, POINT * GafSize,PGAFSequence Cursor_P, int FrameIndex);

LPBYTE InstanceGAFFrame (PGAFFrame GafFrame, LPBYTE * FrameBits, POINT * Aspect);

void CopyGafToBits (LPBYTE PixelBits, POINT * Aspect, unsigned int X, unsigned int Y, PGAFFrame GafFrame, LPRECT Desc_p= NULL);