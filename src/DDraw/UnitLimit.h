#pragma once



class UnitLimit
{
private:
	SingleHook * MPUnitLimit;
	SingleHook * UnitLimit0;
	SingleHook * UnitLimit1;
	SingleHook * UnitLimit2;
public:
	UnitLimit ();
	~UnitLimit();
	
	UnitLimit (DWORD NewLimit);
private:
	void NewUnitLimit (DWORD NewLimit);
};