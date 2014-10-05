#pragma once

class InlineSingleHook;

class ShareDialogExpand
{
public:
	ShareDialogExpand (BOOL Expand_b);
	~ShareDialogExpand ();
protected:
private:
	InlineSingleHook * ShareDialogInitHok;
	InlineSingleHook * ShareDialogProcHok;
};