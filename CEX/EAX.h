﻿// The GPL version 3 License (GPLv3)
// 
// Copyright (c) 2017 vtdev.com
// This file is part of the CEX Cryptographic library.
// 
// This program is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//
// Implementation Details:
// An implementation of an Encrypt and Authenticate mode (EAX).
// Written by John Underhill, January 9, 2017
// Contact: develop@vtdev.com

#ifndef _CEX_EAX_H
#define _CEX_EAX_H

#include "IAeadMode.h"
#include "CMAC.h"
#include "CTR.h"

NAMESPACE_MODE

/// <summary>
/// An Encrypt and Authenticate AEAD Block Cipher Mode (EAX) 
/// </summary> 
/// 
/// <example>
/// <description>Encrypting a single block of bytes:</description>
/// <code>
/// EAX cipher(BlockCiphers::Rijndael);
/// // initialize for encryption
/// cipher.Initialize(true, SymmetricKey(Key, Nonce, [Info]));
/// // encrypt one block
/// size_t encLen = cipher.BlockSize();
/// cipher.Transform(Input, 0, Output, 0, encLen);
/// // append the mac code to the output
/// cipher.Finalize(Output, encLen);
/// </code>
/// </example>
///
/// <example>
/// <description>Decrypting a block of bytes:</description>
/// <code>
/// EAX cipher(BlockCiphers::Rijndael);
/// // initialize for decryption
/// cipher.Initialize(false, SymmetricKey(Key, Nonce, [Associated Data]));
/// // calculate offset; mac code should always be last block after ciphertext
/// size_t decLen = Input.size() - cipher.BlockSize();
/// // decrypt a block
/// cipher.Transform(Input, 0, Output, 0, decLen);
/// // generate the internal mac code and compare it
/// if (!cipher.Verify(Input, decLen))
///		throw;
/// </code>
/// </example>
/// 
/// <remarks>
/// <description><B>Overview:</B></description>
/// <para>
/// The EAX Cipher Mode is an Authenticate Encrypt and Additional Data (AEAD) authenticated mode.<BR></BR>
/// EAX is an online mode, meaning it can stream data of any size, without needing to know the data size in advance.<BR></BR>
/// It also has provable security, dependant on the block cipher used by the mode.<BR></BR>
/// EAX first encrypts the plaintext using a counter mode (CTR), then processes that cipher-text using a CBC-based MAC function used for data authentication.<BR></BR>
/// When encryption is completed, the MAC code is generated and appended to the output stream using the Finalize(Output, Offset) call.<BR></BR>
/// Decryption performs these steps in reverse, processing the cipher-text bytes through the MAC function, then decrypting the data to plain-text.<BR></BR>
/// The Verify(Input, Offset) function can be used to compare the MAC code embedded in the cipher-text with the code generated during the decryption process.</para>
///
/// <description><B>Description:</B></description>
/// <para><EM>Legend:</EM><BR></BR>
/// <B>C</B>=ciphertext, <B>P</B>=plaintext, <B>k</B>=key, <B>E</B>=encrypt, <B>D</B>=decrypt, <B>Mk</B>=keyed mac, <B>T</B>=mac code<BR></BR>
/// <EM>Encryption</EM><BR></BR>
/// For i ...n (Ci = Ek(Pi), T = Mk(Ci)). CT = C||T.<BR></BR>
/// <EM>Decryption</EM><BR></BR>
/// For i ...n (T = Mk(Ci), Pi = D(Ci)). PT = P||T.</para>
///
/// <description><B>Multi-Threading:</B></description>
/// <para>The encryption and decryption functions of the EAX mode can be multi-threaded. This is achieved by processing multiple blocks of message input independently across threads.<BR></BR>
/// The EAX parallel mode also leverages SIMD instructions to 'double parallelize' those segments. An input block assigned to a thread
/// uses SIMD instructions to decrypt/encrypt 4 or 8 blocks in parallel per cycle, depending on which framework is runtime available, 128 or 256 SIMD instructions.<BR></BR>
/// Input blocks equal to, or divisble by the ParallelBlockSize() are processed in parallel on supported systems.
/// The cipher transform is parallelizable, however the authentication pass, (CMAC), is processed sequentially.</para>
///
/// <description>Implementation Notes:</description>
/// <list type="bullet">
/// <item><description>EAX is an AEAD authenticated mode, additional data such as packet header information can be added to the authentication process.</description></item>
/// <item><description>Additional data can be added using the SetAssociatedData(Input, Offset, Length) call, and during Initialize, using the Info parameter of the SymmetricKey.</description></item>
/// <item><description>Calling the Finalize(Output, Offset, Length) function writes the MAC code to the output array in either encryption or decryption operation mode.</description></item>
/// <item><description>The Verify(Input, Offset, Length) function can be used to compare the MAC code embedded with the cipher-text to the internal MAC code generated after a Decryption cycle.</description></item>
/// <item><description>Encryption and decryption can both be pipelined (SSE3-128 or AVX-256), and multi-threaded.</description></item>
/// <item><description>If the system supports Parallel processing, IsParallel() is set to true; passing an input block of ParallelBlockSize() to the transform.</description></item>
/// <item><description>ParallelBlockSize() is calculated automatically based on the processor(s) L1 data cache size, this property can be user defined, and must be evenly divisible by ParallelMinimumSize().</description></item>
/// <item><description>The ParallelBlockSize() can be changed through the ParallelProfile() property</description></item>
/// <item><description>Parallel block calculation ex. <c>ParallelBlockSize = N - (N % .ParallelMinimumSize);</c></description></item>
/// </list>
/// 
/// <description>Guiding Publications:</description>
/// <list type="number">
/// <item><description>The <a href="http://web.cs.ucdavis.edu/~rogaway/papers/eax.pdf">EAX Mode</a> of Operation.</description></item>
/// <item><description>RFC 5116: <a href="https://tools.ietf.org/html/rfc5116">An Interface and Algorithms for Authenticated Encryption</a>.</description></item>
/// <item><description>Handbook of Applied Cryptography <a href="http://cacr.uwaterloo.ca/hac/about/chap7.pdf">Chapter 7: Block Ciphers</a>.</description></item>
/// </list>
/// </remarks>
class EAX : public IAeadMode
{
private:

	static const size_t BLOCK_SIZE = 16;
	const size_t MAX_PRLALLOC = 100000000;
	const size_t TAG_MINLEN = 12;

	CTR m_cipherMode;
	std::vector<byte> m_aadData;
	bool m_aadLoaded;
	bool m_aadPreserve;
	bool m_autoIncrement;
	size_t m_blockSize;
	std::vector<byte> m_cipherKey;
	BlockCiphers m_cipherType;
	bool m_destroyEngine;
	std::vector<byte> m_eaxNonce;
	std::vector<byte> m_eaxVector;
	bool m_isDestroyed;
	bool m_isEncryption;
	bool m_isFinalized;
	bool m_isInitialized;
	bool m_isLoaded;
	std::vector<SymmetricKeySize> m_legalKeySizes;
	Mac::CMAC m_macGenerator;
	size_t m_macSize;
	std::vector<byte> m_msgTag;
	ParallelOptions m_parallelProfile;

public:

	EAX(const EAX&) = delete;
	EAX& operator=(const EAX&) = delete;
	EAX& operator=(EAX&&) = delete;

	//~~~Properties~~~//

	/// <summary>
	/// Get/Set: Enable auto-incrementing of the input nonce, each time the Finalize method is called.
	/// <para>Treats the Nonce value loaded during Initialize as a monotonic counter; 
	/// incrementing the value by 1 and re-calculating the working set each time the cipher is finalized. 
	/// If set to false, requires a re-key after each finalizer cycle.<para>
	/// </summary>
	virtual bool &AutoIncrement() { return m_autoIncrement; }

	/// <summary>
	/// Get: Block size of internal cipher in bytes
	/// </summary>
	virtual const size_t BlockSize() { return m_blockSize; }

	/// <summary>
	/// Get: The block ciphers formal type name
	/// </summary>
	virtual BlockCiphers CipherType() { return m_cipherType; }

	/// <summary>
	/// Get: The underlying Block Cipher instance
	/// </summary>
	virtual IBlockCipher* Engine() { return m_cipherMode.Engine(); }

	/// <summary>
	/// Get: The Cipher Modes enumeration type name
	/// </summary>
	virtual const CipherModes Enumeral() { return CipherModes::EAX; }

	/// <summary>
	/// Get: True if initialized for encryption, False for decryption
	/// </summary>
	virtual const bool IsEncryption() { return m_isEncryption; }

	/// <summary>
	/// Get: The Block Cipher is ready to transform data
	/// </summary>
	virtual const bool IsInitialized() { return m_isInitialized; }

	/// <summary>
	/// Get: Processor parallelization availability.
	/// <para>Indicates whether parallel processing is available with this mode.
	/// If parallel capable, input/output data arrays passed to the transform must be ParallelBlockSize in bytes to trigger parallelization.</para>
	/// </summary>
	virtual const bool IsParallel() { return m_parallelProfile.IsParallel(); }

	/// <summary>
	/// Get: Array of allowed cipher input key byte-sizes
	/// </summary>
	virtual std::vector<SymmetricKeySize> LegalKeySizes() const { return m_legalKeySizes; }

	/// <summary>
	/// Get: The maximum legal tag length in bytes
	/// </summary>
	virtual const size_t MaxTagSize() { return m_macSize; }

	/// <summary>
	/// Get: The minimum legal tag length in bytes
	/// </summary>
	virtual const size_t MinTagSize() { return TAG_MINLEN; }

	/// <summary>
	/// Get: The cipher mode name
	/// </summary>
	virtual const std::string Name() { return "EAX"; }

	/// <summary>
	/// Get: Parallel block size; the byte-size of the input/output data arrays passed to a transform that trigger parallel processing.
	/// <para>This value can be changed through the ParallelProfile class.<para>
	/// </summary>
	virtual const size_t ParallelBlockSize() { return m_parallelProfile.ParallelBlockSize(); }

	/// <summary>
	/// Get/Set: Parallel and SIMD capability flags and sizes 
	/// <para>The maximum number of threads allocated when using multi-threaded processing can be set with the ParallelMaxDegree() property.
	/// The ParallelBlockSize() property is auto-calculated, but can be changed; the value must be evenly divisible by ParallelMinimumSize().
	/// Changes to these values must be made before the <see cref="Initialize(SymmetricKey)"/> function is called.</para>
	/// </summary>
	virtual ParallelOptions &ParallelProfile() { return m_cipherMode.ParallelProfile(); }

	/// <summary>
	/// Get/Set: Persist a one-time associated data for the entire session.
	/// <para>Allows the use of a single SetAssociatedData() call to apply the MAC data to all segments.
	/// Finalize and Verify can be called multiple times, applying the initial associated data to each finalize cycle.<para>
	/// </summary>
	virtual bool &PreserveAD() { return m_aadPreserve; }

	/// <summary>
	/// Get: Returns the full finalized MAC code value array
	/// </summary>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if the cipher has not been finalized</exception>
	virtual const std::vector<byte> Tag() 
	{ 
		if (!m_isFinalized)
			throw CryptoCipherModeException("EAX:Tag", "The cipher mode has not been finalized!");

		return m_msgTag; 
	}

	//~~~Constructor~~~//

	/// <summary>
	/// Initialize the Cipher Mode using a block cipher type name.
	/// <para>The cipher instance is created and destroyed automatically.</para>
	/// </summary>
	///
	/// <param name="CipherType">The enumeration name of the block cipher</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if an ivalid block cipher type is used</exception>
	explicit EAX(BlockCiphers CipherType);

	/// <summary>
	/// Initialize the Cipher Mode using a block cipher instance
	/// </summary>
	///
	/// <param name="Cipher">An uninitialized Block Cipher instance; can not be null</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if a null block cipher is used</exception>
	explicit EAX(IBlockCipher* Cipher);

	/// <summary>
	/// Finalize objects
	/// </summary>
	virtual ~EAX();

	//~~~Public Functions~~~//

	/// <summary>
	/// Decrypt a single block of bytes.
	/// <para>Decrypts one block of bytes beginning at a zero index.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of encrypted bytes</param>
	/// <param name="Output">The output array of decrypted bytes</param>
	void DecryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output);

	/// <summary>
	/// Decrypt a block of bytes with offset parameters.
	/// <para>Decrypts one block of bytes using the designated offsets.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of encrypted bytes</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of decrypted bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	void DecryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset);

	/// <summary>
	/// Release all resources associated with the object
	/// </summary>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if state could not be destroyed</exception>
	virtual void Destroy();

	/// <summary>
	/// Encrypt a single block of bytes. 
	/// <para>Encrypts one block of bytes beginning at a zero index.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of plain text bytes</param>
	/// <param name="Output">The output array of encrypted bytes</param>
	void EncryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output);

	/// <summary>
	/// Encrypt a block of bytes using offset parameters. 
	/// <para>Encrypts one block of bytes using the designated offsets.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of plain text bytes</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of encrypted bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	void EncryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset);

	/// <summary>
	/// Calculate the MAC code (Tag) and copy it to the Output array.   
	/// <para>The output array must be of sufficient length to receive the MAC code.
	/// This function finalizes the Encryption/Decryption cycle, all data must be processed before this function is called.
	/// Initialize(bool, ISymmetricKey) must be called before the cipher can be re-used.</para>
	/// </summary>
	/// 
	/// <param name="Output">The output array that receives the authentication code</param>
	/// <param name="Offset">Starting offset within the output array</param>
	/// <param name="Length">The number of MAC code bytes to write to the output array.
	/// <para>Must be no greater then the MAC functions output size, and no less than the minimum Tag size of 12 bytes.</para></param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if the cipher is not initialized, or output array is too small</exception>
	virtual void Finalize(std::vector<byte> &Output, const size_t Offset, const size_t Length);

	/// <summary>
	/// Initialize the Cipher instance.
	/// <para>The legal symmetric key and nonce sizes are contained in the LegalKeySizes() property.
	/// The Info parameter of the SymmetricKey can be used as the initial associated data.</para>
	/// </summary>
	/// 
	/// <param name="Encryption">True if cipher is used for encryption, false to decrypt</param>
	/// <param name="KeyParams">SymmetricKey containing the encryption Key and Nonce</param>
	/// 
	/// <exception cref="CryptoCipherModeException">Thrown if a null or invalid Key/Nonce is used</exception>
	virtual void Initialize(bool Encryption, ISymmetricKey &KeyParams);

	/// <summary>
	/// Add additional data to the authentication generator.  
	/// <para>Must be called after Initialize(bool, ISymmetricKey), and before any processing of plaintext or ciphertext input. 
	/// This function can only be called once per each initialization/finalization cycle.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of bytes to process</param>
	/// <param name="Offset">Starting offset within the input array</param>
	/// <param name="Length">The number of bytes to process</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if the cipher is not initialized</exception>
	virtual void SetAssociatedData(const std::vector<byte> &Input, const size_t Offset, const size_t Length);

	/// <summary>
	/// Transform an entire block of bytes. 
	/// <para>Encrypts one block of bytes beginning at a zero index.
	/// This method is used in a buffering strategy, where buffers of size BlockSize() for sequential processing, 
	/// or ParallelBlockSize() for parallel processing, are processed and copied to a larger array by the caller.
	/// Buffers should be of the same size, either BlockSize() or ParallelBlockSize().
	/// If the Input and Output array sizes differ, the smallest array size will be processed.
	/// To disable parallel processing, set the ParallelOptions().IsParallel() property to false.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of bytes to transform</param>
	/// <param name="Output">The output array of transformed bytes</param>
	/// 
	/// <returns>The number of bytes processed</returns>
	virtual size_t Transform(const std::vector<byte> &Input, std::vector<byte> &Output);

	/// <summary>
	/// Transform a block of bytes with offset parameters.
	/// <para>Transforms one block of bytes at the designated offsets.
	/// This method is used when looping through two large arrays utilizing offsets incremented by the caller.
	/// One block is processed of either ParallelBlockSize() for parallel processing, or BlockSize() for sequential mode.
	/// To disable parallel processing, set the ParallelOptions().IsParallel() property to false.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of bytes to transform</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of transformed bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	/// 
	/// <returns>The number of bytes processed</returns>
	virtual size_t Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset);

	/// <summary>
	/// Transform a length of bytes with offset parameters. 
	/// <para>This method processes a specified length of bytes, utilizing offsets incremented by the caller.
	/// If IsParallel() is set to true, and the length is at least ParallelBlockSize(), the transform is run in parallel processing mode.
	/// To disable parallel processing, set the ParallelOptions().IsParallel() property to false.
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of bytes to transform</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of transformed bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	/// <param name="Length">The number of bytes to transform</param>
	virtual void Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length);

	/// <summary>
	/// Generate the internal MAC code and compare it with the tag contained in the Input array.   
	/// <para>This function finalizes the Decryption cycle and generates the MAC tag.
	/// The cipher must be set for Decryption and the cipher-text bytes fully processed before calling this function.
	/// Verify can be called in place of a Finalize(Output, Offset, Length) call, or after finalization.
	/// Initialize(bool, ISymmetricKey) must be called before the cipher can be re-used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array containing the expected authentication code</param>
	/// <param name="Offset">Starting offset within the input array</param>
	/// <param name="Length">The number of bytes to compare.
	/// <para>Must be no greater then the MAC functions output size, and no less than the MinTagSize() size.</para></param>
	/// 
	/// <returns>Returns false if the MAC code does not match</returns>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if the cipher is not initialized for decryption</exception>
	virtual bool Verify(const std::vector<byte> &Input, const size_t Offset, const size_t Length);

private:

	void CalculateMac();
	void Reset();
	void Scope();
	void UpdateTag(byte Tag, const std::vector<byte> &Nonce);
};

NAMESPACE_MODEEND
#endif