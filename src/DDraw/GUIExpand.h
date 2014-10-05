#pragma once

class MenuResolution;
class ShareDialogExpand;
class FullScreenMinimap;
class GUIExpand
{
public:
	MenuResolution * SyncMenuResolution;
	ShareDialogExpand * myShareDialog;

	FullScreenMinimap * myMinimap;

public:
	GUIExpand ();
	~GUIExpand();

private:

	;
};

extern GUIExpand * GUIExpander;

