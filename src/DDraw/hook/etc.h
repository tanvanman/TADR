#ifndef ETC_H56NFRDA
extern "C"
{
#define ETC_H56NFRDA


	void *
		memfind(const void  *in_block,     
		size_t       block_size,  
		const void  *in_pattern,  
		size_t       pattern_size,
		size_t      *shift= NULL,        
		BOOL        *repeat_find= FALSE); 
	char * trim_crlf_ (char * Str);
	LONG RegWriteStr (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, LPCSTR Data, DWORD Strlen);
	LONG RegWriteDword (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Data);
	LONG RegWriteData ( HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Type, LPVOID Data, DWORD TypeLen);

	LONG RegReadData (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, DWORD Type, LPVOID Data, DWORD TypeLen);
	LONG RegReadDword  (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName,  DWORD * Data);
	LONG RegReadStr  (HKEY RootKey, LPTSTR lpSubKey, LPTSTR lpValueName, LPCSTR Data, DWORD Strlen);

	unsigned int __stdcall CalcCRC(char* data, int len);

	int GetOpCodeSize (PBYTE Start);
	int X86ShallToRedirect (PBYTE pbCode, LPVOID OrgOpcodeVaddr, PDWORD AddrRtn_Pdw, LPBYTE OrgOpcodeStart, DWORD LenOfOrgOpcode_Dw);
	DWORD X86RedirectOpcodeToNewBase (LPVOID NewBase, LPBYTE OrgOpcode, DWORD  * LenOfOpcode_Dw, LPBYTE * Rtn_PPb);
	BOOL MemWriteWithBackup (LPVOID Addr_Pvoid, DWORD len_Dw, LPBYTE Org_Pvoid, LPBYTE New_Pvoid);
	int GetMinValidLenWithMatchOpcode ( LPBYTE OrgOpcode_Pb, int AtLeastLen_I);
	char * duplicate_str (char * Org);
	int getDBCSlen (char * DBCSstr);

	LPSTR vkToStr(int vk, LPSTR Rtn, int Len);
	char * clean_remark ( char * src, char remarkTag);
};//extern "C"

#endif //ETC_H56NFRDA