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
// An implementation of a Output FeedBack Mode (OFB).
// Written by John Underhill, January 2, 2015
// Updated September 16, 2016
// Contact: develop@vtdev.com

#ifndef _CEX_OFB_H
#define _CEX_OFB_H

#include "ICipherMode.h"

NAMESPACE_MODE

/// <summary>
/// An implementation of a Output FeedBack Mode
/// </summary>
/// 
/// <example>
/// <description>Encrypting a single block of bytes:</description>
/// <code>
/// OFB cipher(BlockCiphers::AHX);
/// // initialize for encryption
/// cipher.Initialize(true, SymmetricKey(Key, Nonce));
/// // encrypt one block
/// cipher.Transform(Input, 0, Output, 0);
/// </code>
/// </example>
///
/// <remarks>
/// <description><B>Overview:</B></description>
/// <para>Output Feedback Mode (OFB) is a similar construction to the CFB mode, and allows encryption of various block sizes.<BR></BR>
/// It differs in that the output of the encryption block function, (rather than the ciphertext), serves as the feedback register.<BR></BR>
/// The cipher is initialized by copying the initialization vector to an internal register, prepended by zeroes.<BR></BR>
/// During a transformation, this register is encrypted by the underlying cipher into a buffer, the buffer is then XOR'd with the input message block to produce the ciphertext.<BR></BR>
/// The vector block is then rotated so that the latter half of the vector is shifted to the start of the array, and the buffer is moved to the end of the array.</para>
/// 
/// <description><B>Description:</B></description>
/// <para><EM>Legend:</EM><BR></BR> 
/// C=ciphertext, P=plaintext, K=key, E=encrypt, ^=XOR<BR></BR>
/// <EM>Encryption</EM><BR></BR>
/// I1 ← IV. For 1 ≤ j ≤ u, given plaintext block Pj:<BR></BR>
/// (a) Oj ← EK(Ij). -Compute the block cipher output.<BR></BR>
/// (b) Tj ← the r leftmost bits of Oj. -Assume the leftmost is identified as bit 1.<BR></BR>
/// (c) Cj ← Pj ^ Tj. -Transmit the r-bit ciphertext block Cj.<BR></BR>
/// (d) Ij+1 ← 2r · Ij + Tj mod 2n. -Update the block cipher input for the next block.<BR></BR>
/// <EM>Decryption</EM><BR></BR>
/// I1 ← IV . For 1 ≤ j ≤ u, upon receiving Cj:<BR></BR>
/// Pj ← Cj ^ Tj, where Tj, Oj, and Ij are computed as an encryption cycle; K(C).</para>
///
/// <description><B>Implementation Notes:</B></description>
/// <list type="bullet">
/// <item><description></description></item>
/// <item><description>A cipher mode constructor can either be initialized with a block cipher instance, or using the block ciphers enumeration name.</description></item>
/// <item><description>A block cipher instance created using the enumeration constructor, is automatically deleted when the class is destroyed.</description></item>
/// <item><description>The Transform functions are virtual, and can be accessed from an ICipherMode instance.</description></item>
/// <item><description>The DecryptBlock and EncryptBlock functions can only be accessed through the class instance.</description></item>
/// <item><description>The transformation methods can not be called until the Initialize(bool, ISymmetricKey) function has been called.</description></item>
/// <item><description>Due to block chain depenencies in OFB mode, neither the encryption or decryption functions can be processed in parallel.</description></item>
/// </list>
/// 
/// <description>Guiding Publications:</description>
/// <list type="number">
/// <item><description>NIST <a href="http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf">SP800-38A</a>.</description></item>
/// <item><description>FIPS <a href="http://csrc.nist.gov/publications/fips/fips81/fips81.htm">PUB81</a>.</description></item>
/// <item><description>Handbook of Applied Cryptography <a href="http://cacr.uwaterloo.ca/hac/about/chap7.pdf">Chapter 7: Block Ciphers</a>.</description></item>
/// </list>
/// </remarks>
class OFB : public ICipherMode
{
private:
	static const size_t BLOCK_SIZE = 16;

	IBlockCipher* m_blockCipher;
	size_t m_blockSize;
	BlockCiphers m_cipherType;
	bool m_destroyEngine;
	bool m_isDestroyed;
	bool m_isEncryption;
	bool m_isInitialized;
	bool m_isParallel;
	std::vector<byte> m_ofbBuffer;
	std::vector<byte> m_ofbVector;
	ParallelOptions m_parallelProfile;

public:

	OFB(const OFB&) = delete;
	OFB& operator=(const OFB&) = delete;
	OFB& operator=(OFB&&) = delete;

	//~~~Properties~~~//

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
	virtual IBlockCipher* Engine() { return m_blockCipher; }

	/// <summary>
	/// Get: The cipher modes type name
	/// </summary>
	virtual const CipherModes Enumeral() { return CipherModes::OFB; }

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
	virtual std::vector<SymmetricKeySize> LegalKeySizes() const { return m_blockCipher->LegalKeySizes(); }

	/// <summary>
	/// Get: The cipher modes class name
	/// </summary>
	virtual const std::string Name() { return "OFB"; }

	/// <summary>
	/// Get: Parallel block size; the byte-size of the input/output data arrays passed to a transform that trigger parallel processing.
	/// <para>This value can be changed through the ParallelProfile class.<para>
	/// </summary>
	virtual const size_t ParallelBlockSize() { return m_parallelProfile.ParallelBlockSize(); }

	/// <summary>
	/// Get/Set: Parallel and SIMD capability flags and sizes (Not supported in this mode)
	/// </summary>
	virtual ParallelOptions &ParallelProfile() { return m_parallelProfile; }

	//~~~Constructor~~~//

	/// <summary>
	/// Initialize the Cipher Mode using a block cipher type name
	/// </summary>
	///
	/// <param name="CipherType">The formal enumeration name of a block cipher</param>
	/// <param name="RegisterSize">Register size in bytes; minimum is 1 byte, maximum is the Block Ciphers internal block size</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if an undefined block cipher type name is used, or the specified register size is invalid</exception>
	explicit OFB(BlockCiphers CipherType, size_t RegisterSize = 16);

	/// <summary>
	/// Initialize the Cipher Mode using a block cipher instance
	/// </summary>
	///
	/// <param name="Cipher">Uninitialized block cipher instance; can not be null</param>
	/// <param name="RegisterSize">Register size in bytes; minimum is 1 byte, maximum is the Block Ciphers internal block size; default value is 16 bytes.</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if a null block cipher is used, or the specified register size is invalid</exception>
	explicit OFB(IBlockCipher* Cipher, size_t RegisterSize = 16);

	/// <summary>
	/// Finalize objects
	/// </summary>
	virtual ~OFB();

	//~~~Public Functions~~~//

	/// <summary>
	/// Release all resources associated with the object
	/// </summary>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if state could not be destroyed</exception>
	virtual void Destroy();

	/// <summary>
	/// Encrypt a single block of bytes. 
	/// <para>Initialize(bool, ISymmetricKey) must be called before this method can be used.
	/// Encrypts one block of bytes beginning at a zero index.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of plain text bytes</param>
	/// <param name="Output">The output array of encrypted bytes</param>
	void EncryptBlock(const std::vector<byte> &Input, std::vector<byte> &Output);

	/// <summary>
	/// Encrypt a block of bytes using offset parameters. 
	/// <para>Initialize(bool, ISymmetricKey) must be called before this method can be used.
	/// Encrypts one block of bytes at the designated offsets.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of plain text bytes</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of encrypted bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	void EncryptBlock(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset);

	/// <summary>
	/// Initialize the Block Cipher instance
	/// </summary>
	/// 
	/// <param name="Encryption">True if cipher is used for encryption, False to decrypt</param>
	/// <param name="KeyParams">SymmetricKey containing the encryption Key and Initialization Vector</param>
	/// 
	/// <exception cref="CryptoCipherModeException">Thrown if a null Key or Nonce is used</exception>
	virtual void Initialize(bool Encryption, ISymmetricKey &KeyParams);

	/// <summary>
	/// Transform an entire block of bytes. 
	/// <para>Encrypts one block of bytes beginning at a zero index.
	/// This method is used in a buffering strategy, where buffers of size BlockSize() are processed and copied to a larger array.
	/// If the Input and Output array sizes differ, the smallest array size will be processed.
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
	/// One block is processed of BlockSize() for OFB sequential mode.
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
	/// Initialize(bool, ISymmetricKey) must be called before this method can be used.</para>
	/// </summary>
	/// 
	/// <param name="Input">The input array of bytes to transform</param>
	/// <param name="InOffset">Starting offset within the input array</param>
	/// <param name="Output">The output array of transformed bytes</param>
	/// <param name="OutOffset">Starting offset within the output array</param>
	/// <param name="Length">The number of bytes to transform</param>
	virtual void Transform(const std::vector<byte> &Input, const size_t InOffset, std::vector<byte> &Output, const size_t OutOffset, const size_t Length);
};

NAMESPACE_MODEEND
#endif
