#ifndef pcxreadH
#define pcxreadH

#include <windows.h>
#include "oddraw.h"
//creates a direct draw surface from a PCX resource
LPDIRECTDRAWSURFACE CreateSurfPCXResource(WORD PCXNum, bool VidMem);

typedef struct RGB_color_typ
        {

        unsigned char red;    // red   component of color 0-63
        unsigned char green;  // green component of color 0-63
        unsigned char blue;   // blue  component of color 0-63

        } RGB_color, *RGB_color_ptr;

typedef struct pcx_header_typ
        {
        char manufacturer;
        char version;
        char encoding;
        char bits_per_pixel;
        short x,y;
        short width,height;
        short horz_res;
        short vert_res;
        char ega_palette[48];
        char reserved;
        char num_color_planes;
        short bytes_per_line;
        short palette_type;
        char padding[58];

        } pcx_header, *pcx_header_ptr;


typedef struct pcx_picture_typ
        {
        pcx_header header;
        RGB_color palette[256];
        unsigned char *buffer;

        } pcx_picture, *pcx_picture_ptr;


void PCX2BitMap(unsigned char *inbuff,pcx_picture_typ *imgout, long length);
void RestoreFromPCX(WORD PCXNum, LPDIRECTDRAWSURFACE lpSurf);

void RestoreFromPCXFile(LPSTR FileName, LPDIRECTDRAWSURFACE lpSurf);

#endif
