#pragma once

#include <iomanip>
#include <stdexcept>
#include <windows.h>
#include <wincrypt.h>

#define SHA256_DIGEST_LENGTH 32

class HmacSha256Calculator {
private:
	// hProv:           Handle to a cryptographic service provider (CSP). 
	//                  This example retrieves the default provider for  
	//                  the PROV_RSA_FULL provider type.  
	// hHash:           Handle to the hash object needed to create a hash.
	// hKey:            Handle to a symmetric key. This example creates a 
	//                  key for the RC4 algorithm.
	// hHmacHash:       Handle to an HMAC hash.
	// HmacInfo:        Instance of an HMAC_INFO structure that contains 
	//                  information about the HMAC hash.
	HCRYPTPROV  hProv;
	HCRYPTHASH  hHash;
	HCRYPTKEY   hKey;
	HCRYPTHASH  hHmacHash;
	HMAC_INFO   HmacInfo;

public:
	HmacSha256Calculator(const unsigned char* key, DWORD key_len);
	~HmacSha256Calculator();

	bool processChunk(const unsigned char* data, unsigned dataLen);
	bool getCurrentHash(unsigned char* buffer, DWORD len);
	bool finalize(unsigned char* buffer, DWORD len);
};

std::ostream& operator<<(std::ostream& os, HmacSha256Calculator& hmac);
