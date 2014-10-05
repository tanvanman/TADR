/// hook in .text:00493A92 054 8B 0D E8 1D 51 00                                               mov     ecx, TAMainStructPtr
//text:004934F5 00C 68 CC 93 50 00                                                  push    offset aMapinfo ; "MAPINFO"
#include "oddraw.h"
#include "iddrawsurface.h"
#include "tamem.h"
#include "tafunctions.h"
#include "hook/etc.h"
#include "hook/hook.h"
#include "sharedialog.h"

using namespace softwaredebugmode;



void __stdcall MetalPosProc(GUIInfo * GUIINFO_P, int)
{
	GUI0IDControl * UpperControl= GUIINFO_P->TheActive_GUIMEM->ControlsAry;

	GUI3_4IDControl * setsharemetal;
	char Num[100];

	if (0xffffffff!=SubGUIIndex ( UpperControl, "SM#", 0xe))
	{
		setsharemetal= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHRMETAL");

		float scale= static_cast<float>(setsharemetal->knobpos)/ static_cast<float>(setsharemetal->range- 1);
		if (1<scale)
		{
			scale= 1;
		}
		if (scale<0)
		{
			scale= 0;
		}
		int Metal= static_cast<int>(scale* setsharemetal->thick);
		if (Metal!=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareMetal))
		{
			wsprintfA ( Num, "%d", Metal);
			SetValue_GUI5ID ( GUIINFO_P, "SM#", Num, 0);
		}
	}
}

void __stdcall EnergyPosProc(GUIInfo * GUIINFO_P, int)
{
	GUI0IDControl * UpperControl= GUIINFO_P->TheActive_GUIMEM->ControlsAry;

	GUI3_4IDControl * setshareenergy;
	char Num[100];

	if (0xffffffff!=SubGUIIndex ( UpperControl, "SE#", 0xe))
	{
		setshareenergy= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHAREGRY");

		float scale= static_cast<float>(setshareenergy->knobpos)/ static_cast<float>(setshareenergy->range- 1);
		if (1<scale)
		{
			scale= 1;
		}
		if (scale<0)
		{
			scale= 0;
		}

		int Energy= static_cast<int>(scale* setshareenergy->thick);
		if (Energy!=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareEnergy))
		{
			wsprintfA ( Num, "%d", Energy);

			SetValue_GUI5ID ( GUIINFO_P, "SE#", Num, 0);
		}
	}
}

void __stdcall GameSpeedPosProc(GUIInfo * GUIINFO_P, int)
{


	GUI0IDControl * UpperControl= GUIINFO_P->TheActive_GUIMEM->ControlsAry;
	GUI3_4IDControl * gamespeed;
	gamespeed= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_GAMESPEED");


	if (gamespeed->knobpos!=(*TAmainStruct_PtrPtr)->GameSpeed)
	{
		ChangeGameSpeed ( gamespeed->knobpos, TRUE);
	}
}


int __stdcall ShareDialogInit (PInlineX86StackBuffer X86StrackBuffer)
{
	GUI0IDControl * UpperControl= (*TAmainStruct_PtrPtr)->desktopGUI.TheActive_GUIMEM->ControlsAry;

	GUIInfo * UpperGUIInfo= reinterpret_cast<GUIInfo *>(X86StrackBuffer->Esi);
	char Num[100];
	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHAREMETAL", 0xe))
	{//
		GUI1IDControl * sharemetal;
		sharemetal= (GUI1IDControl *)SubControl_str2ptr ( UpperControl, "EN_SHAREMETAL");

		if (0!=(SharedMetal&((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].PlayerInfo->SharedBits)))
		{
			sharemetal->status_curnt= 1;
		}
		else
		{
			sharemetal->status_curnt= 0;
		}
	}


	if (0xffffffff!=SubGUIIndex ( UpperControl, "SRL_SETSHRMETAL", 0xe))
	{//
		GUI3_4IDControl * setsharemetal;
		
		setsharemetal= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHRMETAL");
		setsharemetal->thick=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].PlayerRes.fMaxMetalStorage);
		setsharemetal->knobsize=setsharemetal->height;
		setsharemetal->range= setsharemetal->width- setsharemetal->knobsize;
		
		setsharemetal->knobpos= static_cast<short int>((((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareMetal)/setsharemetal->thick)* setsharemetal->range);
		setsharemetal->pos_proc= MetalPosProc;

		if (0xffffffff!=SubGUIIndex ( UpperControl, "SM#", 0xe))
		{
			wsprintfA ( Num, "%d", static_cast<int>(((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareMetal)));

			SetValue_GUI5ID ( UpperGUIInfo, "SM#", Num, 0);
		}
	}
	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHAREENERGY", 0xe))
	{//
		GUI1IDControl * shareenergy;
		shareenergy= (GUI1IDControl *)SubControl_str2ptr ( UpperControl, "EN_SHAREENERGY");


		if (0!=(SharedEnergy&((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].PlayerInfo->SharedBits)))
		{
			shareenergy->status_curnt= 1;
		}
		else
		{
			shareenergy->status_curnt= 0;
		}
	}

	if (0xffffffff!=SubGUIIndex ( UpperControl, "SRL_SETSHAREGRY", 0xe))
	{//
		GUI3_4IDControl * setshareenergy;
		setshareenergy= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHAREGRY");

		setshareenergy->thick=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].PlayerRes.fMaxEnergyStorage);
		setshareenergy->knobsize= setshareenergy->height;
		setshareenergy->range= setshareenergy->width- setshareenergy->knobsize;

		setshareenergy->knobpos= static_cast<short int>((((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareEnergy)/setshareenergy->thick)* setshareenergy->range);
		setshareenergy->pos_proc= EnergyPosProc;
		
		if (0xffffffff!=SubGUIIndex ( UpperControl, "SE#", 0xe))
		{
			wsprintfA ( Num, "%d",  static_cast<int>(((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareEnergy)));
			SetValue_GUI5ID ( UpperGUIInfo, "SE#", Num, 0);
		}
	}
	
	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHOOTALL", 0xe))
	{//
		GUI1IDControl * shootall;
		shootall= (GUI1IDControl *)SubControl_str2ptr ( UpperControl, "EN_SHOOTALL");
		if (0!=(Shootall& ((*(TAmainStruct_PtrPtr))->SoftwareDebugMode)))
		{
			shootall->status_curnt= 1;
		}
		else
		{
			shootall->status_curnt= 0;
		}
	}

	if (0xffffffff!=SubGUIIndex ( UpperControl, "SRL_GAMESPEED", 0xe))
	{//
		GUI3_4IDControl * gamespeed;
		gamespeed= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_GAMESPEED");

		gamespeed->thick= 20;
		gamespeed->knobsize= 1;
		gamespeed->range= 20;
		if (20<((*(TAmainStruct_PtrPtr))->GameSpeed))
		{
			gamespeed->knobpos= 20;
		}
		else if (((*(TAmainStruct_PtrPtr))->GameSpeed)<0)
		{
			gamespeed->knobpos= 0;
		}
		else
		{
			gamespeed->knobpos= ((*(TAmainStruct_PtrPtr))->GameSpeed);
		}

		gamespeed->pos_proc= GameSpeedPosProc;
	}
	return 0;
}

int __stdcall ShareDialogProc (PInlineX86StackBuffer X86StrackBuffer)
{
	GUIInfo * TAUI_p= reinterpret_cast<GUIInfo *> (X86StrackBuffer->Edi);
	GUI0IDControl * UpperControl= TAUI_p->TheActive_GUIMEM->ControlsAry;
	
	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHAREMETAL", 0xe))
	{//
		if (IsPressCommand ( TAUI_p, "EN_SHAREMETAL"))
		{
			ChatText ( "+sharemetal");
		}
	}

	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHAREENERGY", 0xe))
	{//
		if (IsPressCommand ( TAUI_p, "EN_SHAREENERGY"))
		{
			ChatText ( "+shareenergy");
		}
	}

	if (0xffffffff!=SubGUIIndex ( UpperControl, "EN_SHOOTALL", 0xe))
	{//
		if (IsPressCommand ( TAUI_p, "EN_SHOOTALL"))
		{
			ChatText ( "+shootall");
		}
	}

	if (IsPressCommand ( TAUI_p, "OK"))
	{
		char Buf[100];

		if (0xffffffff!=SubGUIIndex ( UpperControl, "SRL_SETSHRMETAL", 0xe))
		{//
			GUI3_4IDControl * setsharemetal;
			setsharemetal= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHRMETAL");

			float scale= static_cast<float>(setsharemetal->knobpos)/ static_cast<float>(setsharemetal->range- 1);
			if (1<scale)
			{
				scale= 1;
			}
			if (scale<0)
			{
				scale= 0;
			}
			int Metal= static_cast<int>(scale* setsharemetal->thick);
			if (Metal!=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareMetal))
			{
				wsprintfA ( Buf, "+setsharemetal %d", Metal);
				ChatText ( Buf);
			}
		}

		if (0xffffffff!=SubGUIIndex ( UpperControl, "SRL_SETSHAREGRY", 0xe))
		{//
			GUI3_4IDControl * setshareenergy;
			setshareenergy= (GUI3_4IDControl *)SubControl_str2ptr ( UpperControl, "SRL_SETSHAREGRY");


			float scale= static_cast<float>(setshareenergy->knobpos)/ static_cast<float>(setshareenergy->range- 1);
			if (1<scale)
			{
				scale= 1;
			}
			if (scale<0)
			{
				scale= 0;
			}

			int Energy= static_cast<int>(scale* setshareenergy->thick);
			if (Energy!=static_cast<int>((*(TAmainStruct_PtrPtr))->Players[(*(TAmainStruct_PtrPtr))->LocalHumanPlayer_PlayerID].ShareEnergy))
			{
				wsprintfA ( Buf, "+setshareenergy %d", Energy);
				ChatText ( Buf);
			}

		}


	}
	return 0;
}


ShareDialogExpand::ShareDialogExpand(BOOL Expand_b)
{
	if (Expand_b)
	{
		ShareDialogInitHok= new InlineSingleHook ( 0x0493A92, 5, INLINE_5BYTESLAGGERJMP, ShareDialogInit);
		ShareDialogProcHok= new InlineSingleHook ( 0x004934F5, 5, INLINE_5BYTESLAGGERJMP, ShareDialogProc);
	}
	else
	{
		ShareDialogInitHok= NULL;
		ShareDialogProcHok= NULL;
	}
}
ShareDialogExpand::~ShareDialogExpand ()
{
	if (NULL!=ShareDialogInitHok)
	{
		delete ShareDialogInitHok;
	}

	if (NULL!=ShareDialogProcHok)
	{
		delete ShareDialogProcHok;
	}
}