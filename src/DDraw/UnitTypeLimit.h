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
public:
	IncreaseUnitTypeLimit ();
	IncreaseUnitTypeLimit ( int Number);
	~IncreaseUnitTypeLimit ();
private:
	void WriteNewLimit (DWORD Number);
};
