#ifndef _CEX_GCM_H
#define _CEX_GCM_H

#include "IAeadMode.h"
#include "CTR.h"
#include "GHASH.h"

NAMESPACE_MODE

/// <summary>
/// A Galois/Counter Authenticated Block Cipher Mode
/// </summary> 
/// 
/// <example>
/// <description>Encrypting a single block of bytes:</description>
/// <code>
/// GCM cipher(BlockCiphers::Rijndael);
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
/// GCM cipher(BlockCiphers::Rijndael);
/// // initialize for decryption
/// cipher.Initialize(false, SymmetricKey(Key, Nonce, [Info]));
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
/// The GCM Cipher Mode is an Authenticate Encrypt and Additional Data (AEAD) authenticated mode.<BR></BR>
/// GCM is an online mode, meaning it can stream data of any size, without needing to know the data size in advance.<BR></BR>
/// GCM uses a Galois Multiply function then combines the ciphertext with an authentication code to produce an authentication tag.<BR></BR>
/// A nonce is generated and XOR'd with the encrypted plain-text to create the cipher-text.<BR></BR>
/// Decryption performs these steps in reverse, creating a nonce and the cipher-text bytes through the decryption function, then adding the plain-text to a checksum.<BR></BR>
/// The Verify(Input, Offset) function can be used to compare the MAC code embedded in the cipher-text with the code generated during the decryption process.<BR></BR>
/// The Finalize(Output, Offset, Length) function writes the MAC code to an output stream in either encryption or decryption operation modes.
/// </para>
///
/// <description><B>Description:</B></description>
/// <para><EM>Mac Legend:</EM><BR></BR> 
/// <B>H</B>=hash-key, <B>A</B>=plain-text, <B>C</B>=cipher-text, <B>m</B>=message-length, <B>n</B>=ciphertext-length, <B>||</B>=OR, <B>^</B>=XOR</para>
/// <para><EM>MAC Function</EM><BR></BR>
/// 1) for i = 1...m-1, (Xi-1 ^ Ai) * H.<BR></BR>
/// 2) for i = m (Xi-1 ^ (Am || 0<sup>128-v</sup>)) * H.<BR></BR>
/// 3) for i = m+1...m-1, (Xi-1 ^ Ci-m) * H.<BR></BR>
/// 4) for i = m + n (Xm+n-1 ^ (Cn || 0<sup>128-u</sup>)) * H.<BR></BR>
/// 5) for i = m + n + 1 (Xm+n ^ (len(A)||len(C))) * H.<BR></BR></para>
///
/// <para><EM>Cipher Legend:</EM><BR></BR>
/// <B>C</B>=ciphertext, <B>P</B>=plaintext, <B>k</B>=key, <B>E</B>=encrypt, <B>D</B>=decrypt, <B>Mk</B>=keyed mac, <B>T</B>=mac code<BR></BR>
/// <EM>Encryption</EM><BR></BR>
/// for i ...n (Ci = Ek(Pi), T = Mk(Ci)). CT = C||T.<BR></BR>
/// <EM>Decryption</EM><BR></BR>
/// for i ...n (T = Mk(Ci), Pi = D(Ci)). PT = P||T.</para>
///
/// <description><B>Multi-Threading:</B></description>
/// <para>The encryption and decryption functions of GCM mode can be multi-threaded. This is achieved by processing multiple blocks of message input independently across threads.<BR></BR>
/// The GCM parallel mode also leverages SIMD instructions to 'double parallelize' those segments. An input block assigned to a thread
/// uses SIMD instructions to decrypt/encrypt 4 or 8 blocks in parallel per cycle, depending on which framework is runtime available, 128 or 256 SIMD instructions.<BR></BR>
/// Input blocks equal to, or divisble by the ParallelBlockSize() are processed in parallel on supported systems.
/// Sequential processing is used when the system dows not support SIMD or has only one core, or a standard an input blockis less than the parallel block size.</para>
///
/// <description>Implementation Notes:</description>
/// <list type="bullet">
/// <item><description>GCM is an AEAD authenticated mode, additional data such as packet header information can be added to the authentication process.</description></item>
/// <item><description>Additional data can be added using the SetAssociatedData(Input, Offset, Length) call.</description></item>
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
/// <item><description>The <a href="http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-spec.pdf">Galois/Counter Mode</a> of Operation (GCM).</description></item>
/// <item><description>RFC 5116: <a href="https://tools.ietf.org/html/rfc5288">AES Galois Counter Mode</a> (GCM) Cipher Suites for TLS.</description></item>
/// <item><description>RFC 5116: <a href="https://tools.ietf.org/html/rfc5116">An Interface and Algorithms for Authenticated Encryption</a>.</description></item>
/// </list>
/// </remarks>
class GCM : public IAeadMode
{
private:

	static const size_t BLOCK_SIZE = 16;
	const size_t MAX_PRLALLOC = 100000000;
	const size_t TAG_MINLEN = 12;

	std::vector<byte> m_aadData;
	bool m_aadLoaded;
	bool m_aadPreserve;
	size_t m_aadSize;
	bool m_autoIncrement;
	std::vector<byte> m_checkSum;
	CTR m_cipherMode;
	BlockCiphers m_cipherType;
	bool m_destroyEngine;
	Mac::GHASH* m_gcmHash;
	std::vector<byte> m_gcmKey;
	std::vector<byte> m_gcmNonce;
	std::vector<byte> m_gcmVector;
	bool m_isDestroyed;
	bool m_isEncryption;
	bool m_isFinalized;
	bool m_isInitialized;
	std::vector<SymmetricKeySize> m_legalKeySizes;
	size_t m_msgSize;
	std::vector<byte> m_msgTag;
	ParallelOptions m_parallelProfile;

public:

	GCM(const GCM&) = delete;
	GCM& operator=(const GCM&) = delete;
	GCM& operator=(GCM&&) = delete;

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
	virtual const size_t BlockSize() { return BLOCK_SIZE; }

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
	virtual const CipherModes Enumeral() { return CipherModes::GCM; }

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
	virtual const size_t MaxTagSize() { return BLOCK_SIZE; }

	/// <summary>
	/// Get: The minimum legal tag length in bytes
	/// </summary>
	virtual const size_t MinTagSize() { return TAG_MINLEN; }

	/// <summary>
	/// Get: The cipher mode name
	/// </summary>
	virtual const std::string Name() { return "GCM"; }

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
	virtual ParallelOptions &ParallelProfile() { return m_parallelProfile; }

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
			throw CryptoCipherModeException("GCM:Tag", "The cipher mode has not been finalized!");

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
	explicit GCM(BlockCiphers CipherType);

	/// <summary>
	/// Initialize the Cipher Mode using a block cipher instance
	/// </summary>
	///
	/// <param name="Cipher">An uninitialized Block Cipher instance; can not be null</param>
	///
	/// <exception cref="Exception::CryptoCipherModeException">Thrown if a null block cipher is used</exception>
	explicit GCM(IBlockCipher* Cipher);

	/// <summary>
	/// Finalize objects
	/// </summary>
	virtual ~GCM();

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
};

NAMESPACE_MODEEND
#endif