#include "HmacSha256.h"

HmacSha256Calculator::HmacSha256Calculator(const unsigned char* key, DWORD key_len) :
	hProv(NULL),
	hHash(NULL),
	hKey(NULL),
	hHmacHash(NULL)
{
	//--------------------------------------------------------------------
	// Zero the HMAC_INFO structure and use the SHA1 algorithm for
	// hashing.
	ZeroMemory(&HmacInfo, sizeof(HmacInfo));
	HmacInfo.HashAlgid = CALG_SHA_256;

	//--------------------------------------------------------------------
	// Acquire a handle to the default RSA cryptographic service provider.
	if (!CryptAcquireContext(
		&hProv,                   // handle of the CSP
		NULL,                     // key container name
		NULL,                     // CSP name
		PROV_RSA_AES,            // provider type
		CRYPT_VERIFYCONTEXT))     // no key access is requested
	{
		throw std::runtime_error("Error in AcquireContext");
	}

	//--------------------------------------------------------------------
	// Derive a symmetric key from a hash object by performing the
	// following steps:
	//    1. Call CryptCreateHash to retrieve a handle to a hash object.
	//    2. Call CryptHashData to add a text string (password) to the 
	//       hash object.
	//    3. Call CryptDeriveKey to create the symmetric key from the
	//       hashed password derived in step 2.
	// You will use the key later to create an HMAC hash object. 
	if (!CryptCreateHash(
		hProv,                    // handle of the CSP
		CALG_SHA_256,                // hash algorithm to use
		0,                        // hash key
		0,                        // reserved
		&hHash))                  // address of hash object handle
	{
		throw std::runtime_error("Error in CryptCreateHash");
	}

	if (!CryptHashData(
		hHash,                    // handle of the hash object
		key,                    // password to hash
		key_len,            // number of bytes of data to add
		0))                       // flags
	{
		throw std::runtime_error("Error in CryptHashData");
	}

	if (!CryptDeriveKey(
		hProv,                    // handle of the CSP
		CALG_RC4,                 // algorithm ID
		hHash,                    // handle to the hash object
		0,                        // flags
		&hKey))                   // address of the key handle
	{
		throw std::runtime_error("Error in CryptDeriveKey");
	}

	//--------------------------------------------------------------------
	// Create an HMAC by performing the following steps:
	//    1. Call CryptCreateHash to create a hash object and retrieve 
	//       a handle to it.
	//    2. Call CryptSetHashParam to set the instance of the HMAC_INFO 
	//       structure into the hash object.
	//    3. Call CryptHashData to compute a hash of the message.
	//    4. Call CryptGetHashParam to retrieve the size, in bytes, of
	//       the hash.
	//    5. Call malloc to allocate memory for the hash.
	//    6. Call CryptGetHashParam again to retrieve the HMAC hash.

	if (!CryptCreateHash(
		hProv,                    // handle of the CSP.
		CALG_HMAC,                // HMAC hash algorithm ID
		hKey,                     // key for the hash (see above)
		0,                        // reserved
		&hHmacHash))              // address of the hash handle
	{
		throw std::runtime_error("Error in CryptCreateHash");
	}

	if (!CryptSetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HMAC_INFO,             // setting an HMAC_INFO object
		(BYTE*)&HmacInfo,         // the HMAC_INFO object
		0))                       // reserved
	{
		throw std::runtime_error("Error in CryptSetHashParam");
	}
}

HmacSha256Calculator::~HmacSha256Calculator() {
	if (hHmacHash)
		CryptDestroyHash(hHmacHash);
	if (hKey)
		CryptDestroyKey(hKey);
	if (hHash)
		CryptDestroyHash(hHash);
	if (hProv)
		CryptReleaseContext(hProv, 0);
}

bool HmacSha256Calculator::processChunk(const unsigned char* data, unsigned dataLen) {
	if (!CryptHashData(
		hHmacHash,                // handle of the HMAC hash object
		data,                    // message to hash
		dataLen,					// number of bytes of data to add
		0))                       // flags
	{
		throw std::runtime_error("Error in CryptHashData");
	}
	return true;
}

bool HmacSha256Calculator::getCurrentHash(unsigned char* buffer, DWORD len) {
	// Create a duplicate of the current hash state
	HCRYPTHASH hHashDup = NULL;
	if (!CryptDuplicateHash(
		hHmacHash,
		NULL,
		0,
		&hHashDup))
	{
		throw std::runtime_error("Error in CryptDuplicateHash");
	}

	// Get the hash value from the duplicate
	DWORD dwDataLen = 0;
	if (!CryptGetHashParam(
		hHashDup,
		HP_HASHVAL,
		NULL,
		&dwDataLen,
		0))
	{
		CryptDestroyHash(hHashDup);
		throw std::runtime_error("Error in CryptGetHashParam (size)");
	}

	if (dwDataLen != len) {
		CryptDestroyHash(hHashDup);
		throw std::runtime_error("Unexpected Hash result size");
	}

	if (!CryptGetHashParam(
		hHashDup,
		HP_HASHVAL,
		buffer,
		&dwDataLen,
		0))
	{
		CryptDestroyHash(hHashDup);
		throw std::runtime_error("Error in CryptGetHashParam (value)");
	}

	// Clean up the duplicate
	CryptDestroyHash(hHashDup);
	return true;
}

bool HmacSha256Calculator::finalize(unsigned char* buffer, DWORD len) {
	// pbHash:          Pointer to the hash.
	// dwDataLen:       Length, in bytes, of the hash.
	PBYTE       pbHash = NULL;
	DWORD       dwDataLen = 0;

	//--------------------------------------------------------------------
	// Call CryptGetHashParam twice. Call it the first time to retrieve
	// the size, in bytes, of the hash. Allocate memory. Then call 
	// CryptGetHashParam again to retrieve the hash value.

	if (!CryptGetHashParam(
		hHmacHash,                // handle of the HMAC hash object
		HP_HASHVAL,               // query on the hash value
		NULL,                     // filled on second call
		&dwDataLen,               // length, in bytes, of the hash
		0))
	{
		throw std::runtime_error("Error in CryptGetHashParam");
	}

	if (dwDataLen != len) {
		throw std::runtime_error("Unexpected Hash result size");
	}

	if (!CryptGetHashParam(
		hHmacHash,                 // handle of the HMAC hash object
		HP_HASHVAL,                // query on the hash value
		buffer,                    // pointer to the HMAC hash value
		&dwDataLen,                // length, in bytes, of the hash
		0))
	{
		throw std::runtime_error("Error in CryptGetHashParam");
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, HmacSha256Calculator& hmac)
{
	unsigned char buffer[SHA256_DIGEST_LENGTH];
	hmac.getCurrentHash(buffer, sizeof(buffer));
	for (int n = 0; n < SHA256_DIGEST_LENGTH; ++n)
	{
		os << std::hex << std::setw(2) << std::setfill('0') << unsigned(buffer[n]);
	}
	return os;
}