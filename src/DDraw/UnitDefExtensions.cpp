#include "UnitDefExtensions.h"
#include "iddrawsurface.h"
#include "tamem.h"

//static unsigned int loadUnitDefHookAddr = 0x42ac7e;	// "ensure" unit data
static unsigned int loadUnitDefHookAddr = 0x42bf97;		// "load" unit data
static unsigned int loadUnitDefHookProc(PInlineX86StackBuffer X86StrackBuffer)
{
	return UnitDefExtensions::GetInstance()->LoadUnitDefs(X86StrackBuffer);
}

std::unique_ptr<UnitDefExtensions> UnitDefExtensions::m_instance;

UnitDefExtensions::UnitDefExtensions()
{
	m_hook.reset(new InlineSingleHook(loadUnitDefHookAddr, 5, INLINE_5BYTESLAGGERJMP, loadUnitDefHookProc));
}

UnitDefExtensions::~UnitDefExtensions()
{
}

UnitDefExtensions* UnitDefExtensions::GetInstance()
{
	if (!m_instance)
	{
		m_instance.reset(new UnitDefExtensions());
	}
	return m_instance.get();
}

unsigned UnitDefExtensions::registerIntKey(const std::string& key, int defaultValue)
{
	if (m_keyIndices.count(key) > 0u)
	{
		IDDrawSurface::OutptFmtTxt("[UnitDefExtensions::registerIntKey] key %s is already defined with index=0x%x!", key.c_str(), m_keyIndices[key]);
		return m_keyIndices[key];
	}

	m_defaultIntValues.push_back(defaultValue);
	return m_keyIndices[key] = (INT_KEY << 30) | (m_defaultIntValues.size() - 1u);
}

unsigned UnitDefExtensions::registerFloatKey(const std::string& key, double defaultValue)
{
	if (m_keyIndices.count(key) > 0u)
	{
		IDDrawSurface::OutptFmtTxt("[UnitDefExtensions::registerFloatKey] key %s is already defined with index=0x%x!", key.c_str(), m_keyIndices[key]);
		return m_keyIndices[key];
	}

	m_defaultFloatValues.push_back(defaultValue);
	return m_keyIndices[key] = (FLOAT_KEY << 30) | (m_defaultFloatValues.size() - 1u);
}

unsigned UnitDefExtensions::registerStringKey(const std::string& key, std::string defaultValue)
{
	if (m_keyIndices.count(key) > 0u)
	{
		IDDrawSurface::OutptFmtTxt("[UnitDefExtensions::registerStringKey] key %s is already defined with index=0x%x!", key.c_str(), m_keyIndices[key]);
		return m_keyIndices[key];
	}

	m_defaultStringValues.push_back(defaultValue);
	return m_keyIndices[key] = (STRING_KEY << 30) | (m_defaultStringValues.size() - 1u);
}

unsigned UnitDefExtensions::getKeyIndex(const std::string& key)
{
	if (m_keyIndices.count(key) == 0u)
	{
		IDDrawSurface::OutptFmtTxt("[UnitDefExtensions::getKeyIndex] key %s is not known!", key.c_str(), m_keyIndices[key]);
		return -1;
	}
	return m_keyIndices[key];
}

int UnitDefExtensions::getInt(unsigned unitDefId, unsigned index)
{
	if (index >> 30 == INT_KEY)
	{
		index &= ~(3u << 30);
		if (unitDefId < m_intValues.size() && index < m_intValues[unitDefId].size())
		{
			return m_intValues[unitDefId][index];
		}
		if (index < m_defaultIntValues.size())
		{
			return m_defaultIntValues[index];
		}
	}
	return 0;
}

double UnitDefExtensions::getDouble(unsigned unitDefId, unsigned index)
{
	if (index >> 30 == FLOAT_KEY)
	{
		index &= ~(3u << 30);
		if (unitDefId < m_floatValues.size() && index < m_floatValues[unitDefId].size())
		{
			return m_floatValues[unitDefId][index];
		}
		if (index < m_defaultFloatValues.size())
		{
			return m_defaultFloatValues[index];
		}
	}
	return 0.0;
}

const std::string & UnitDefExtensions::getString(unsigned unitDefId, unsigned index)
{
	if (index >> 30 == STRING_KEY)
	{
		index &= ~(3u << 30);
		if (unitDefId < m_stringValues.size() && index < m_stringValues[unitDefId].size())
		{
			return m_stringValues[unitDefId][index];
		}
		if (index < m_defaultStringValues.size())
		{
			return m_defaultStringValues[index];
		}
	}
	static std::string emptyString;
	return emptyString;
}

void UnitDefExtensions::setInt(unsigned unitDefId, unsigned index, int value)
{
	if (unitDefId < 65536 && index < 65536)
	{
		if (m_intValues.size() <= unitDefId)
		{
			m_intValues.reserve(2u * unitDefId);
			m_intValues.resize(1u + unitDefId);
		}
		if (m_intValues[unitDefId].size() <= index)
		{
			m_intValues[unitDefId].resize(1u + index);
		}
		m_intValues[unitDefId][index] = value;
	}
}

void UnitDefExtensions::setFloat(unsigned unitDefId, unsigned index, double value)
{
	if (unitDefId < 65536 && index < 65536)
	{
		if (m_floatValues.size() <= unitDefId)
		{
			m_floatValues.reserve(2u * unitDefId);
			m_floatValues.resize(1u + unitDefId);
		}
		if (m_floatValues[unitDefId].size() <= index)
		{
			m_floatValues[unitDefId].resize(1u + index);
		}
		m_floatValues[unitDefId][index] = value;
	}
}

void UnitDefExtensions::setString(unsigned unitDefId, unsigned index, const std::string& value)
{
	if (unitDefId < 65536 && index < 65536)
	{
		if (m_stringValues.size() <= unitDefId)
		{
			m_stringValues.reserve(2u * unitDefId);
			m_stringValues.resize(1u + unitDefId);
		}
		if (m_stringValues[unitDefId].size() <= index)
		{
			m_stringValues[unitDefId].resize(1u + index);
		}
		m_stringValues[unitDefId][index] = value;
	}
}

struct TaTdfFile;

typedef int(__thiscall* TdfFile_GetInt_t)(TaTdfFile* thisptr, const char* key, int defaultValue);
typedef double(__thiscall* TdfFile_GetFloat_t)(TaTdfFile* thisptr, const char* key, double defaultValue);
typedef void(__thiscall* TdfFile_GetString_t)(TaTdfFile* thisptr, char* ReceiveBuf, const char* key, size_t Buflen, const char* defaultValue);

static TdfFile_GetInt_t TdfFile_GetInt = (TdfFile_GetInt_t)0x4C46C0;
static TdfFile_GetFloat_t TdfFile_GetFloat = (TdfFile_GetFloat_t)0x4c4760;
static TdfFile_GetString_t TdfFile_GetString = (TdfFile_GetString_t)0x4C48c0;

unsigned UnitDefExtensions::LoadUnitDefs(PInlineX86StackBuffer X86StrackBuffer)
{
	TaTdfFile* tdf = (TaTdfFile*)X86StrackBuffer->Ecx;
	UnitDefStruct* unitDef = (UnitDefStruct*)X86StrackBuffer->Ebp;
	for (auto it = m_keyIndices.begin(); it != m_keyIndices.end(); ++it)
	{
		unsigned _index = it->second;
		unsigned index = it->second & ~(3u << 30);
		switch (_index >> 30)
		{
		case INT_KEY:
		{
			int defaultValue = index < m_defaultIntValues.size() ? m_defaultIntValues[index] : 0;
			int value = TdfFile_GetInt(tdf, it->first.c_str(), defaultValue);
			setInt(unitDef->UnitTypeID, index, value);
			break;
		}
		case FLOAT_KEY:
		{
			double defaultValue = index < m_defaultFloatValues.size() ? m_defaultFloatValues[index] : 0.0;
			double value = TdfFile_GetFloat(tdf, it->first.c_str(), defaultValue);
			setFloat(unitDef->UnitTypeID, index, value);
			break;
		}
		case STRING_KEY:
		{
			std::string defaultValue = index < m_defaultStringValues.size() ? m_defaultStringValues[index] : "";
			char buffer[1024] = { 0 };
			TdfFile_GetString(tdf, buffer, it->first.c_str(), sizeof(buffer) - 1u, defaultValue.c_str());
			setString(unitDef->UnitTypeID, index, buffer);
			break;
		}
		}
	}

	return 0;
}

