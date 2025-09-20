#include "VeterancyHack.h"
#include "UnitDefExtensions.h"
#include "iddrawsurface.h"
#include "tamem.h"

#include <sstream>
#include <iostream>

unsigned getVetLevel(const std::vector<unsigned>& thresholds, unsigned kills)
{
	if (thresholds.size() == 0u || kills < thresholds[0]) return 0;
	auto it = std::upper_bound(thresholds.begin(), thresholds.end(), kills);
	return unsigned(std::distance(thresholds.begin(), it));
}

unsigned getUnboundedVetLevel(const std::vector<unsigned>& thresholds, unsigned kills)
{
	unsigned N = thresholds.size();
	if (N == 0u || kills < thresholds[0])
	{
		return 0;
	}
	if (kills <= thresholds[N - 1u])
	{
		return getVetLevel(thresholds, kills);
	}
	if (N >= 2u)
	{
		unsigned rate = thresholds[N - 1u] - thresholds[N - 2u];
		unsigned vetLevel = N + (kills - thresholds[N - 1u]) / rate;
		return vetLevel;
	}
	if (N == 1u)
	{
		unsigned vetLevel = kills / thresholds[0];
		return vetLevel;
	}
	return 0;
}

static unsigned int captureCostAddr = 0x4043d8;
static unsigned int captureCostProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->captureCostProc(X86StrackBuffer);
}
unsigned VeterancyHack::captureCostProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* attackTarget = (UnitStruct*)X86StrackBuffer->Edx;
	const std::string &thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		attackTarget->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getUnboundedVetLevel(thresholds, attackTarget->Kills);
	X86StrackBuffer->Eax = 10u + vetLevel;
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x4043ec;
	return X86STRACKBUFFERCHANGE;
}


static unsigned int devDrawKillsAddr = 0x467ccf;
static unsigned int devDrawKillsProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->devDrawKillsProc(X86StrackBuffer);
}
unsigned VeterancyHack::devDrawKillsProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Edi;
	const std::string &thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getVetLevel(thresholds, unit->Kills);
	X86StrackBuffer->Ecx = unit->Kills;
	if (vetLevel > 0u)
	{
		static char buffer[16];
		std::snprintf(buffer, sizeof(buffer), "Vet%d", vetLevel);
		X86StrackBuffer->Eax = (unsigned)buffer;
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x467cce;
	}
	else
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x467d0e;
	}
	return X86STRACKBUFFERCHANGE;
}


static unsigned int drawKillsAddr = 0x46b306;
static unsigned int drawKillsProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->drawKillsProc(X86StrackBuffer);
}
unsigned VeterancyHack::drawKillsProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Esi;
	const std::string& thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getVetLevel(thresholds, unit->Kills);
	X86StrackBuffer->Ecx = unit->Kills;
	if (vetLevel > 0u)
	{
		static char buffer[16];
		std::snprintf(buffer, sizeof(buffer), "Vet%d", vetLevel);
		X86StrackBuffer->Eax = (unsigned) buffer;
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x46b331;
	}
	else
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x46b358;
	}
	return X86STRACKBUFFERCHANGE;
}


static unsigned int takeDamageAddr = 0x489bfa;
static unsigned int takeDamageProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->takeDamageProc(X86StrackBuffer);
}
unsigned VeterancyHack::takeDamageProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Esi;
	const std::string& thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getVetLevel(thresholds, unit->Kills);
	if (vetLevel > 25)
	{
		vetLevel = 25;
	}
	X86StrackBuffer->Edx = vetLevel;
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x489c16;
	return X86STRACKBUFFERCHANGE;
}

static unsigned int dealDamageAddr = 0x499db5;
static unsigned int dealDamageProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->dealDamageProc(X86StrackBuffer);
}
unsigned VeterancyHack::dealDamageProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Ebx;
	const std::string& thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getVetLevel(thresholds, unit->Kills);
	X86StrackBuffer->Edx = vetLevel;
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x499dd1;
	return X86STRACKBUFFERCHANGE;
}

static unsigned int aimBuffAddr = 0x48a324;
static unsigned int aimBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->aimBuffProc(X86StrackBuffer);
}
unsigned VeterancyHack::aimBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Edi;
	const std::string& thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	if (thresholds.size() > 0u && unit->Kills > thresholds[0])
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x48a332;
		return X86STRACKBUFFERCHANGE;
	}
	else
	{
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x48a42d;
		return X86STRACKBUFFERCHANGE;
	}
}

static unsigned int accuracyBuffAddr = 0x49d6ea;
static unsigned int accuracyBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->accuracyBuffProc(X86StrackBuffer);
}
unsigned VeterancyHack::accuracyBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Edi;
	int rate = UnitDefExtensions::GetInstance()->getInt(
		unit->UnitType->UnitTypeID,
		m_idxAccuracyBuffRate);

	if (rate > 0)
	{
		X86StrackBuffer->Ebx = unit->Kills / rate;
		X86StrackBuffer->Ecx += 0x800;
		X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x49d702;
		return X86STRACKBUFFERCHANGE;
	}
	else
	{
		return 0;
	}
}

static unsigned int reloadTimeBuffAddr = 0x49e468;
static unsigned int reloadTimeBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return VeterancyHack::GetInstance()->reloadTimeBuffProc(X86StrackBuffer);
}
unsigned VeterancyHack::reloadTimeBuffProc(PInlineX86StackBuffer X86StrackBuffer)
{
	UnitStruct* unit = (UnitStruct*)X86StrackBuffer->Edi;
	const std::string& thresholdsStr = UnitDefExtensions::GetInstance()->getString(
		unit->UnitType->UnitTypeID,
		m_idxVetThresholds);

	std::vector<unsigned>& thresholds = getThresholds(thresholdsStr);
	unsigned vetLevel = getVetLevel(thresholds, unit->Kills);
	if (vetLevel > 16u)
	{
		vetLevel = 16u;
	}
	X86StrackBuffer->Ecx = vetLevel;
	X86StrackBuffer->rtnAddr_Pvoid = (LPVOID)0x49e48d;
	return X86STRACKBUFFERCHANGE;
}

std::unique_ptr<VeterancyHack> VeterancyHack::m_instance;

VeterancyHack::VeterancyHack()
{
	m_idxVetThresholds = UnitDefExtensions::GetInstance()->registerStringKey("VeterancyThresholds", "5 10 15 20 25");
	m_idxAccuracyBuffRate = UnitDefExtensions::GetInstance()->registerIntKey("VeterancyAccuracyBuffRate", 12);

	m_hooks.push_back(std::make_unique<InlineSingleHook>(captureCostAddr, 5, INLINE_5BYTESLAGGERJMP, ::captureCostProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(drawKillsAddr, 5, INLINE_5BYTESLAGGERJMP, ::drawKillsProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(devDrawKillsAddr, 5, INLINE_5BYTESLAGGERJMP, ::devDrawKillsProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(takeDamageAddr, 5, INLINE_5BYTESLAGGERJMP, ::takeDamageProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(dealDamageAddr, 5, INLINE_5BYTESLAGGERJMP, ::dealDamageProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(aimBuffAddr, 5, INLINE_5BYTESLAGGERJMP, ::aimBuffProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(accuracyBuffAddr, 5, INLINE_5BYTESLAGGERJMP, ::accuracyBuffProc));
	m_hooks.push_back(std::make_unique<InlineSingleHook>(reloadTimeBuffAddr, 5, INLINE_5BYTESLAGGERJMP, ::reloadTimeBuffProc));
}

VeterancyHack::~VeterancyHack()
{
}

VeterancyHack* VeterancyHack::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new VeterancyHack());
	}
	return m_instance.get();
}

static std::vector<unsigned> splitInts(const std::string& input)
{
	std::vector<unsigned> result;
	std::istringstream iss(input);
	std::string token;

	while (iss >> token)
	{
		try
		{
			size_t pos = 0;
			unsigned value = std::stoul(token, &pos);
			if (pos == token.size())
			{
				result.push_back(value); // only accept if fully parsed
			}
		}
		catch (...)
		{
			// ignore invalid tokens
		}
	}

	return result;
}

std::vector<unsigned>& VeterancyHack::getThresholds(const std::string& key)
{
	auto it = m_thresholds.find(key);
	if (it == m_thresholds.end())
	{
		m_thresholds[key] = splitInts(key);
		if (m_thresholds[key].size() == 0)
		{
			m_thresholds[key].push_back(5);
			m_thresholds[key].push_back(10);
			m_thresholds[key].push_back(15);
			m_thresholds[key].push_back(20);
			m_thresholds[key].push_back(25);
		}
		return m_thresholds[key];
	}
	else
	{
		return it->second;
	}
}
