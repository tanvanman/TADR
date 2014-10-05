#ifndef dddtaH
#define dddtaH

#include "IRenderer.h"
#include "tamem.h"

#include <deque>

class CDDDTA
{
private:
	HANDLE MemMap;
	void *MapView;
	TAdynmemStruct *TAdynmem;
	ParticleBase *Smoke;
	int PartOffset;
	CMinimapHandler MinimapHandler;
	LPDIRECTSOUND3DLISTENER lpDs3dListener;

	bool TA3dEnabled;
	bool SoundEnabled;

	SharedMem *DDDSharedMem;
	void Print(HDC hdc, PrimitiveStruct *Object);
	void WriteSubPart(SubPart *Part, PrimitiveStruct *Primitive);
	void InitDDDTA();
	void inline SetActiveUnit(BeginUnitsArray_p  *DestUnit, UnitStruct *TAUnit);
	void WriteProjectiles();
	void WriteExplosion();
	void WriteSmoke();
	void CreateSharedTAMem(int TAPTR, int Size, char *Name);
	void RestoreAllSharedTAMem();
	void CheckMessages();
	void WriteUnits();
	void (__stdcall *ShowText)(PlayerStruct *Player, char *Text, int Unk1, int Unk2);
	void (__stdcall *InterpretCommand)(char *Command, int Access); //Access 1 = no cheats, 3 = cheats

	HWND TAhWnd;

	struct TAShareMemStruct
	{
		int TAAcessPTR;
		int OldMem;
		int Size;
		HANDLE MemMap;
		void *MapView;
	};
	std::deque<TAShareMemStruct> SharedTAMemList;

public:
	CDDDTA();
	~CDDDTA();

    bool Message(HWND WinProchWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	void FrameUpdate();
	void Blit(LPDIRECTDRAWSURFACE DestSurf);
	void SendTextToSpring(const char *Text);
	void (__cdecl *TADeleteMem)(void *Mem);
	void DeInitDDDTA();
	void Moo();
};

#endif
