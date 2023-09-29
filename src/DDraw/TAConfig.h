#pragma once

#include <vector>
#include <string>

extern LPCSTR TAIniStr;

class RegDword
{
	LPSTR myName;
	DWORD myValue;
public:
	RegDword (LPSTR Name, int NameLen, DWORD Value);
	~RegDword ();

	LPCSTR Name (void);
	DWORD Value (void);
};

class RegString
{
	LPSTR myName;
	LPSTR myStr;
public:
	RegString (LPSTR Name, int NameLen, LPSTR Str, int mystrLen);
	~RegString ();

	LPCSTR Name (void);
	LPCSTR Str (void);
};

typedef struct _EnumRegInfo
{
	std::vector<RegDword *>::iterator Dword_iter;
	std::vector<RegString *>::iterator String_iter;
	int Count;
}EnumRegInfo, * PEnumRegInfo;

class TADRConfig 
{
private:
	bool IsDdrawIni;

	char IniFilePath_cstr[MAX_PATH];

	std::vector<RegString *> RegStrings_vec;
	std::vector<RegDword *> RegDwords_vec;

public:
	TADRConfig ();
	~TADRConfig ();

	BOOL GetIniBool (LPCSTR ConfigName, BOOL Default);
	int GetIniInt (LPCSTR ConfigName, int DefaultValue);
	int GetIniStr (LPCSTR ConfigName, LPSTR lpReturnedString, DWORD nSize, LPSTR DefaultStr);

	BOOL SetIniBool (LPCSTR ConfigName, BOOL Value);
	int EnumIniRegInfo_End (PEnumRegInfo * iterator_arg);
	int EnumIniRegInfo_Next (PEnumRegInfo * iterator_arg, LPCSTR * Name_pp, LPCVOID *  Data_p);
	int EnumIniRegInfo_Begin (PEnumRegInfo * iterator_arg, LPCSTR * Name_pp, LPCVOID *  Data_p);

	LONG WriteTAReg_Str (LPTSTR lpValueName, LPCSTR Data, DWORD Strlen);
	LONG WriteTAReg_Dword (LPTSTR lpValueName, DWORD Value);
	LONG ReadTAReg_Dword (LPTSTR lpValueName, DWORD * Value_p);
	LONG ReadTAReg_Str (LPTSTR lpValueName, LPCSTR Data, DWORD Strlen);

	unsigned int GetIniCrc (void);
	
	LPCSTR FindRegStr (LPSTR Name_cstrp, LPCSTR Default);
	DWORD FindRegDword (LPSTR Name_cstrp, DWORD Default);
private:
	void LoadIniRegSetting ( LPBYTE IniFileData);
	HKEY TARegPath_HKEY (void);
};

extern TADRConfig * MyConfig;;