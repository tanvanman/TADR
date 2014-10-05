#pragma once

class TAGameAreaReDrawer
{
public:
	LPDIRECTDRAWSURFACE GameAreaSurfaceFront_ptr;

public:
	TAGameAreaReDrawer ();
	~TAGameAreaReDrawer ();

	void BlitTAGameArea (LPDIRECTDRAWSURFACE DestSurf);

	BOOL MixDSufInBlit (LPRECT DescRect, LPDIRECTDRAWSURFACE Src_DDrawSurface, LPRECT SrcScope);
	BOOL MixBitsInBlit (LPRECT DescRect, LPBYTE SrcBits, LPPOINT SrcAspect, LPRECT SrcScope);
	BOOL GrayBlitOfBits (LPRECT DescRect, LPBYTE SrcBits, LPPOINT SrcAspect, LPRECT SrcScope, BOOL NoMapped);

	LPRECT TAWGameAreaRect (LPRECT Out_Rect);

	LPDIRECTDRAWSURFACE InitOwnSurface (LPDIRECTDRAW TADD, BOOL VidMem);

	void ReleaseSurface (void);

	HRESULT Lock (  LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT Unlock(  LPVOID lpSurfaceData);
		
	LPDIRECTDRAWSURFACE Flip (void);
	void Cls (void);
private:
	LPDIRECTDRAWSURFACE GameAreaSurfaceBack_ptr;
	LPBYTE DescBuf;
	RECT DescRect_Buf;
};