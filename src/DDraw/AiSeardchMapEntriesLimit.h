#pragma once

class IncreaseAISearchMapEntriesLimit
{
private:
	//DWORD OrginalLimit;
	SingleHook * ModifyTheLimit;
public:
	IncreaseAISearchMapEntriesLimit ();
	IncreaseAISearchMapEntriesLimit (DWORD NewLimit);
	~IncreaseAISearchMapEntriesLimit ();
};
