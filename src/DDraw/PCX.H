// This is the more Windows(tm) adapted version of my PCX class
// works with Watcom for Win95 at least...last changes made in 1999
// supports reading of 8 and 24-bit images and writing of 8-bit
// images. I also have a version reading 16-bit, but 16-bit PCX-
// images are just 24 bit images (since PCX doesn't support 16-bit)
// Still for use with DirectDraw every card seems to have it's own
// 16-bit format ;-) so you're probably better of writing it yourself ;-)
// if not email me and I can give you the source for the 16-bit too...
// (email is at the end of this message) 
// I'm sorry for the lack of comments and lack of consistency in
// the variable names...sigh!!
// I still think that this class is fairly small and simple to
// use (hey, I'm not saying that there's no improvements to be made,
// quite the opposite, there's A LOT of them, but it works great for
// me at least (ok, ok, it's easy enough for me to say since I've
// created it, but))
//
// Some more info (for sending thanks and bug-reports and other nonsense ;-),
// hey, hey, you can ask me questions on the sourcecode too):
//
// My name: Andreas Magnusson
// My email: md7amag@mdstud.chalmers.se
//
// I would be very pleased if you greeted me in your program if you
// decide to use this sourcecode, if you really don't want to do
// that, I think you should at least send me an email saying that
// this helped you out in some way with the program xyz or whatever
// your naming convention may be...
//
//
// Yep and here comes the disclaimer: You use this source code at
// your own risk and I can't be held responsible for any damage
// physical (on yourself, computer or other equipment/stuff)
// or psychological (mostly inside yourself I guess) that
// was caused while using this sourcecode in any way.
//


#ifndef _PCX_H_
#define _PCX_H_

#include <windows.h>

// This is the header of the PCX-file
struct pcx_header
{
	char manufacturer;
	char version;
	char encoding;
	char bits_per_pixel;
	short x, y;
	short width, height;
	short horz_res;
	short vert_res;
	char ega_palette[48];
	char reserved;
	char num_color_planes;
	short bytes_per_line;
	short palette_type;
	short hori_res1;
	short vert_res1;
	char padding[54];
};


class PCX
{
private:
	long nWidth;
	long nHeight;
	pcx_header header;
	PALETTEENTRY *palette;
	long lImageSize;
	long lSize;
	BYTE *lpBuffer;
	BYTE *WriteLine(BYTE *pbuf, DWORD line);
	BYTE *ReadLine24(BYTE *pbuf, DWORD line, short sv);
public:
	PCX();
	~PCX();
	BOOL Load(const char *fname, BOOL fOnlyHeader = FALSE);
	BOOL Load24(const char *fname);
	BOOL Save(const char *fname);
	long Width();
	long Height();
	long Size() {return lImageSize;}
	long BufSize() {return lSize;}
	pcx_header& Info() {return header;}
	void Erase();
	operator BYTE *() {return lpBuffer;}
	operator const BYTE *() {return lpBuffer;}
	PALETTEENTRY *Palette() {return palette;}
	BYTE *PCXData() {return lpBuffer;}
	BYTE *SetBuffer(BYTE *b);
	BYTE *NewBuffer(short w, short h);
	void SetPalette(PALETTEENTRY *p);
	void CopyPalette(PALETTEENTRY *p);
	long Pitch () {return header.bytes_per_line;}
};
extern PALETTEENTRY TAPalette[];

#endif
