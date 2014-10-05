#pragma once

class IncreaseCompositeBuf 
{
public:
	DWORD CurtX, CurtY;
private:
	DWORD OrgX, OrgY;
	SingleHook * CompositeBufSizeHook;;

public:
	IncreaseCompositeBuf ();
	IncreaseCompositeBuf (DWORD x, DWORD y);
	~IncreaseCompositeBuf ();
private:
	void WriteNewLimit (DWORD x, DWORD y);
};
