#include "config.h"
#include "oddraw.h"

#include <vector>
using namespace std;
#include "TAConfig.h"

#include "sharedialog.h"
#include "MenuResolution.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"
#include "iddrawsurface.h"

GUIExpand * GUIExpander = nullptr;

	//////////////-------
GUIExpand::GUIExpand ()
{
	myShareDialog= NULL;
	SyncMenuResolution= NULL;
#if USEMEGAMAP
	myMinimap= NULL;
#endif
	if (MyConfig->GetIniBool ( "ShareDialogExpand", TRUE))
	{
		myShareDialog = new ShareDialogExpand(TRUE);
	}
#if USEMEGAMAP
	myMinimap= new FullScreenMinimap ( MyConfig->GetIniBool ( "FullScreenMinimap", FALSE), MyConfig->GetIniInt("MegamapFpsLimit", 60));
#endif

	/* 
		Detect cnc-ddraw: 
		GameHandlesClose is a unique export added for c&c games, it can't be found in any other wrapper 
	*/

	if (GetProcAddress(SDDraw, "GameHandlesClose"))
	{
		SyncMenuResolution = new MenuResolution(0, 0);
	}
	else if (MyConfig->GetIniInt("MenuWidth", 0))
	{
		SyncMenuResolution= new MenuResolution ( MyConfig->GetIniInt ( "MenuWidth", 0), MyConfig->GetIniInt ( "MenuHeight", 0));
	}
	else
	{
		SyncMenuResolution= new MenuResolution ( MyConfig->GetIniBool ( "MenuResolution", FALSE));
	}
};

GUIExpand::~GUIExpand ()
{
	if (NULL!=myShareDialog)
	{
		delete myShareDialog;
	}
	if (NULL!=SyncMenuResolution)
	{
		delete SyncMenuResolution;
	}
#if USEMEGAMAP
	if (NULL!=myMinimap)
	{
		delete myMinimap;
	}
#endif
}

