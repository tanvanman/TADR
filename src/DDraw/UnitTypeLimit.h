#pragma once

class IncreaseUnitTypeLimit
{
public:
	int CurtUnitTypeNum;

private:
	ModifyHook * Prologue_Weight;
	ModifyHook * Argc_0_Weight;
	ModifyHook * Argc_1_Weight;
	ModifyHook * Epilogue_Weight;
	ModifyHook * Prologue_limit;
	ModifyHook * Argc_0_limit;
	ModifyHook * Argc_1_limit;
	ModifyHook * Argc_2_limit;
	ModifyHook * Epilogue_limit;
	ModifyHook * Push_FindSpot;
	SingleHook * Mov_FindSpot;

	// Sibling sites that iterate the same UnitTypeID-indexed bitmask 16 DWORDs at a time
	// (see SelectionBitmaskFix notes in LimitCrack.cpp).
	SingleHook * Mov_BinarySearchByName_OR;       // 0x488E3D MOV EDX,0x10 — UnitInfo_BinarySearchByName OR loop
	SingleHook * Mov_AI_ParseProcWeight_Clear;    // 0x406DBD MOV ECX,0x10 — AI_ParseProc_Weight  bitmask clear
	SingleHook * Mov_AI_ParseProcLimit_Clear;     // 0x406E51 MOV ECX,0x10 — AI_ParseProc_Limit   bitmask clear

	// Ctrl-Z hotkey — SelectUnitsSameType uses its own stack-allocated bitmask
	ModifyHook * Prologue_SelectSameType;         // 0x48BE08 SUB ESP,0x40
	SingleHook * Mov_SelectSameType_Clear;        // 0x48BE21 MOV ECX,0x10
	ModifyHook * Epilogue_SelectSameType;         // 0x48BF1E ADD ESP,0x40

public:
	IncreaseUnitTypeLimit ();
	IncreaseUnitTypeLimit ( int Number);
	~IncreaseUnitTypeLimit ();
private:
	void WriteNewLimit (DWORD Number);
};
