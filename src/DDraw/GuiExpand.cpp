#include "oddraw.h"

#include <vector>
using namespace std;
#include "TAConfig.h"

#include "sharedialog.h"
#include "MenuResolution.h"
#include "fullscreenminimap.h"
#include "GUIExpand.h"



GUIExpand * GUIExpander;


	//////////////-------
GUIExpand::GUIExpand ()
{
	myShareDialog= NULL;
	SyncMenuResolution= NULL;
#ifdef USEMEGAMAP
	myMinimap= NULL;
#endif
	if (MyConfig->GetIniBool ( "ShareDialogExpand", TRUE))
	{
		myShareDialog = new ShareDialogExpand(TRUE);
	}
#ifdef USEMEGAMAP
	myMinimap= new FullScreenMinimap ( MyConfig->GetIniBool ( "FullScreenMinimap", FALSE) );
#endif
	int i= MyConfig->GetIniInt ( "MenuWidth", 0);
	if (0!=i)
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
#ifdef USEMEGAMAP
	if (NULL!=myMinimap)
	{
		delete myMinimap;
	}
#endif
}

