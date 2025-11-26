#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "hook/hook.h"

class UnitDefExtensions
{
public:

	~UnitDefExtensions();
	static UnitDefExtensions* GetInstance();

	unsigned registerIntKey(const std::string& key, int defaultValue);
	unsigned registerFloatKey(const std::string& key, double defaultValue);
	unsigned registerStringKey(const std::string& key, std::string defaultValue);
	unsigned getKeyIndex(const std::string& key);

	int getInt(unsigned unitDefId, unsigned index);
	double getDouble(unsigned unitDefId, unsigned index);
	const std::string& getString(unsigned unitDefId, unsigned index);

	void setInt(unsigned unitDefId, unsigned index, int value);
	void setFloat(unsigned unitDefId, unsigned index, double value);
	void setString(unsigned unitDefId, unsigned index, const std::string& value);

	unsigned LoadUnitDefs(PInlineX86StackBuffer X86StrackBuffer);

private:
	UnitDefExtensions();

	static const unsigned INT_KEY = 1u;
	static const unsigned FLOAT_KEY = 2u;
	static const unsigned STRING_KEY = 3u;

	static std::unique_ptr<UnitDefExtensions> m_instance;
	std::unique_ptr<SingleHook> m_hook;

	std::map<std::string, unsigned> m_keyIndices;

	std::vector<int> m_defaultIntValues;
	std::vector<double> m_defaultFloatValues;
	std::vector<std::string> m_defaultStringValues;

	std::vector< std::vector<int> > m_intValues;
	std::vector< std::vector<double> > m_floatValues;
	std::vector< std::vector<std::string> > m_stringValues;
};
