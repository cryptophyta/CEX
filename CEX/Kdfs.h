#ifndef CEX_KDFS_H
#define CEX_KDFS_H

#include "CexDomain.h"

NAMESPACE_ENUMERATION

/// <summary>
/// Key Derivation Functions enumeration names
/// </summary>
enum class Kdfs : byte
{
	/// <summary>
	/// No kdf is specified
	/// </summary>
	None = 0,

	/// <summary>
	/// The Hierarchal Key Distribution System: HKDS(SHAKE-128)
	/// </summary>
	HKDS128 = 1,
	/// <summary>
	/// The Hierarchal Key Distribution System: HKDS(SHAKE-128)
	/// </summary>
	HKDS256 = 2,
	/// <summary>
	/// The Hierarchal Key Distribution System: HKDS(SHAKE-128)
	/// </summary>
	HKDS512 = 2,

	/// <summary>
	/// A Hash based Key Derivation Function: HKDF(SHA2-256)
	/// </summary>
	HKDF256 = 1,
	/// <summary>
	/// A Hash based Key Derivation Function: HKDF(SHA2-512)
	/// </summary>
	HKDF512 = 2,
	/// <summary>
	/// An implementation of the Hash based Key Derivation Function: KDF2(SHA2-256)
	/// </summary>
	KDF2256 = 3,
	/// <summary>
	/// An implementation of the Hash based Key Derivation Function: KDF2(SHA2-512)
	/// </summary>
	KDF2512 = 4,
	/// <summary>
	/// An implementation of a Passphrase Based KDF: PBKDF2(SHA2-256)
	/// </summary>
	PBKDF2256 = 5,
	/// <summary>
	/// An implementation of a Passphrase Based KDF: PBKDF2(SHA2-512)
	/// </summary>
	PBKDF2512 = 6,
	/// <summary>
	/// An implementation of the SCRYPT(SHA2-256)
	/// </summary>
	SCRYPT256 = 7,
	/// <summary>
	/// An implementation of the SCRYPT(SHA2-512)
	/// </summary>
	SCRYPT512 = 8,
	/// <summary>
	/// An implementation of the SHAKE-128 XOF function
	/// </summary>
	SHAKE128 = 9,
	/// <summary>
	/// An implementation of the SHAKE-256 XOF function
	/// </summary>
	SHAKE256 = 10,
	/// <summary>
	/// An implementation of the SHAKE-512 XOF function
	/// </summary>
	SHAKE512 = 11,
	/// <summary>
	/// An implementation of the SHAKE-1024 XOF function
	/// </summary>
	SHAKE1024 = 12
};

class KdfConvert
{
public:

	/// <summary>
	/// Derive the Kdfs formal string name from the enumeration name
	/// </summary>
	/// 
	/// <param name="Enumeral">The Kdfs enumeration member</param>
	///
	/// <returns>The matching Kdfs string name</returns>
	static std::string ToName(Kdfs Enumeral);

	/// <summary>
	/// Derive the Kdfs enumeration type-name from the formal string name
	/// </summary>
	/// 
	/// <param name="Name">The Kdfs string name</param>
	///
	/// <returns>The matching Kdfs enumeration type name</returns>
	static Kdfs FromName(std::string &Name);
};
NAMESPACE_ENUMERATIONEND
#endif
