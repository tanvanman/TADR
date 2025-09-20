#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "hook/hook.h"

class VeterancyHack
{
public:
	~VeterancyHack();
	static VeterancyHack* GetInstance();

	unsigned captureCostProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned drawKillsProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned devDrawKillsProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned takeDamageProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned dealDamageProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned aimBuffProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned accuracyBuffProc(PInlineX86StackBuffer X86StrackBuffer);
	unsigned reloadTimeBuffProc(PInlineX86StackBuffer X86StrackBuffer);

private:
	VeterancyHack();
	static std::unique_ptr<VeterancyHack> m_instance;
	std::vector<std::unique_ptr<SingleHook> > m_hooks;

	unsigned m_idxVetThresholds;		// cavedog: 5 10 15 20 25
	unsigned m_idxAccuracyBuffRate;		// cavedog: 12

	std::map<std::string, std::vector<unsigned> > m_thresholds;
	std::vector<unsigned>& getThresholds(const std::string& key);
};
