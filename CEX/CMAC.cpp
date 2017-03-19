#include "CMAC.h"
#include "ArrayUtils.h"
#include "CBC.h"
#include "IntUtils.h"
#include "ISO7816.h"
#include "SymmetricKey.h"

NAMESPACE_MAC

using Cipher::Symmetric::Block::Mode::CBC;

//~~~Constructor~~~//

CMAC::CMAC(BlockCiphers CipherType)
	:
	m_cipherMode(new CBC(CipherType)),
	m_cipherKey(0),
	m_cipherType(CipherType),
	m_destroyEngine(true),
	m_isDestroyed(false),
	m_isInitialized(false),
	m_legalKeySizes(0),
	m_macSize(m_cipherMode->BlockSize()),
	m_msgCode(m_macSize),
	m_wrkBuffer(m_macSize),
	m_wrkOffset(0)
{
	Scope();
}

CMAC::CMAC(IBlockCipher* Cipher)
	:
	m_cipherMode(new CBC(Cipher)),
	m_cipherKey(0),
	m_cipherType(Cipher->Enumeral()),
	m_destroyEngine(false),
	m_isDestroyed(false),
	m_isInitialized(false),
	m_legalKeySizes(0),
	m_macSize(m_cipherMode->BlockSize()),
	m_msgCode(m_macSize),
	m_wrkBuffer(m_macSize),
	m_wrkOffset(0)
{
	Scope();
}

CMAC::~CMAC()
{
	Destroy();
}

//~~~Public Functions~~~//

void CMAC::Compute(const std::vector<byte> &Input, std::vector<byte> &Output)
{
	if (!m_isInitialized)
		throw CryptoMacException("CMAC:Compute", "The Mac is not initialized!");

	if (Output.size() != m_macSize)
		Output.resize(m_macSize);

	Update(Input, 0, Input.size());
	Finalize(Output, 0);
}

void CMAC::Destroy()
{
	if (!m_isDestroyed)
	{
		m_isDestroyed = true;
		m_cipherType = BlockCiphers::None;
		m_isInitialized = false;
		m_macSize = 0;
		m_wrkOffset = 0;

		try
		{
			if (m_destroyEngine)
			{
				m_destroyEngine = false;

				if (m_cipherMode != 0)
					delete m_cipherMode;
			}

			Utility::ArrayUtils::ClearVector(m_cipherKey);
			Utility::ArrayUtils::ClearVector(m_K1);
			Utility::ArrayUtils::ClearVector(m_K2);
			Utility::ArrayUtils::ClearVector(m_legalKeySizes);
			Utility::ArrayUtils::ClearVector(m_msgCode);
			Utility::ArrayUtils::ClearVector(m_wrkBuffer);
		}
		catch (std::exception& ex)
		{
			throw CryptoMacException("CMAC:Destroy", "Could not clear all variables!", std::string(ex.what()));
		}
	}
}

size_t CMAC::Finalize(std::vector<byte> &Output, size_t OutOffset)
{
	if (!m_isInitialized)
		throw CryptoMacException("CMAC:Finalize", "The Mac is not initialized!");
	if ((Output.size() - OutOffset) < m_macSize)
		throw CryptoMacException("CMAC:Finalize", "The Output buffer is too short!");

	if (m_wrkOffset != m_cipherMode->BlockSize())
	{
		Cipher::Symmetric::Block::Padding::ISO7816 pad;
		pad.AddPadding(m_wrkBuffer, m_wrkOffset);
		Utility::IntUtils::XORBLK(m_K2, 0, m_wrkBuffer, 0, m_macSize, m_cipherMode->ParallelProfile().SimdProfile());
	}
	else
	{
		Utility::IntUtils::XORBLK(m_K1, 0, m_wrkBuffer, 0, m_macSize, m_cipherMode->ParallelProfile().SimdProfile());
	}

	m_cipherMode->Transform(m_wrkBuffer, 0, m_msgCode, 0);
	memcpy(&Output[OutOffset], &m_msgCode[0], m_macSize);
	Reset();

	return m_macSize;
}

void CMAC::Initialize(ISymmetricKey &KeyParams)
{
	if (!SymmetricKeySize::Contains(m_cipherMode->LegalKeySizes(), KeyParams.Key().size(), 0, 0))
		throw CryptoMacException("CMAC:Initialize", "Key size is too small; must be minimum key size!");

	if (m_isInitialized)
		Reset();

	m_cipherKey = KeyParams.Key();
	std::vector<byte> tmpIv(m_cipherMode->BlockSize());
	Key::Symmetric::SymmetricKey kp(m_cipherKey, tmpIv);
	m_cipherMode->Initialize(true, kp);

	if (KeyParams.Info().size() != 0 &&
		m_cipherType != BlockCiphers::Rijndael &&
		m_cipherType != BlockCiphers::Serpent &&
		m_cipherType != BlockCiphers::Twofish)
	{
		if (KeyParams.Info().size() <= m_cipherMode->Engine()->DistributionCodeMax())
		{
			m_cipherMode->Engine()->DistributionCode() = KeyParams.Info();
		}
		else
		{
			// info is too large; size to optimal max, ignore remainder
			std::vector<byte> tmpInfo(m_cipherMode->Engine()->DistributionCodeMax());
			memcpy(&tmpInfo[0], &KeyParams.Info()[0], tmpInfo.size());
			m_cipherMode->Engine()->DistributionCode() = tmpInfo;
		}
	}

	std::vector<byte> lu(m_cipherMode->BlockSize());
	std::vector<byte> tmpz(m_cipherMode->BlockSize());
	m_cipherMode->Transform(tmpz, 0, lu, 0);
	m_K1 = GenerateSubkey(lu);
	m_K2 = GenerateSubkey(m_K1);
	m_cipherMode->Initialize(true, kp);

	m_isInitialized = true;
}

void CMAC::Reset()
{
	// reinitialize the cbc iv
	memset(&static_cast<CBC*>(m_cipherMode)->Nonce()[0], (byte)0, static_cast<CBC*>(m_cipherMode)->Nonce().size());
	memset(&m_msgCode[0], 0, m_msgCode.size());
	memset(&m_wrkBuffer[0], 0, m_wrkBuffer.size());
	m_wrkOffset = 0;
}

void CMAC::Update(byte Input)
{
	if (m_wrkOffset == m_wrkBuffer.size())
	{
		m_cipherMode->Transform(m_wrkBuffer, 0, m_msgCode, 0);
		m_wrkOffset = 0;
	}

	m_wrkBuffer[m_wrkOffset++] = Input;
}

void CMAC::Update(const std::vector<byte> &Input, size_t InOffset, size_t Length)
{
	if (Length == 0)
		return;
	if (!m_isInitialized)
		throw CryptoMacException("CMAC:Update", "The Mac is not initialized!");
	if ((InOffset + Length) > Input.size())
		throw CryptoMacException("CMAC:Update", "The Input buffer is too short!");

	if (m_wrkOffset == m_cipherMode->BlockSize())
	{
		m_cipherMode->Transform(m_wrkBuffer, 0, m_msgCode, 0);
		m_wrkOffset = 0;
	}

	size_t diff = m_cipherMode->BlockSize() - m_wrkOffset;
	if (Length > diff)
	{
		memcpy(&m_wrkBuffer[m_wrkOffset], &Input[InOffset], diff);
		m_cipherMode->Transform(m_wrkBuffer, 0, m_msgCode, 0);
		m_wrkOffset = 0;
		Length -= diff;
		InOffset += diff;

		while (Length > m_cipherMode->BlockSize())
		{
			m_cipherMode->Transform(Input, InOffset, m_msgCode, 0);
			Length -= m_cipherMode->BlockSize();
			InOffset += m_cipherMode->BlockSize();
		}
	}

	if (Length > 0)
	{
		memcpy(&m_wrkBuffer[m_wrkOffset], &Input[InOffset], Length);
		m_wrkOffset += Length;
	}
}

//~~~Private Functions~~~//

std::vector<byte> CMAC::GenerateSubkey(std::vector<byte> &Input)
{
	int fbit = (Input[0] & 0xFF) >> 7;
	std::vector<byte> tmpKey(Input.size());

	for (size_t i = 0; i < Input.size() - 1; i++)
		tmpKey[i] = (byte)((Input[i] << 1) + ((Input[i + 1] & 0xFF) >> 7));

	tmpKey[Input.size() - 1] = (byte)(Input[Input.size() - 1] << 1);

	if (fbit == 1)
		tmpKey[Input.size() - 1] ^= (Input.size() == m_cipherMode->BlockSize()) ? CT87 : CT1B;

	return tmpKey;
}

void CMAC::Scope()
{
	m_legalKeySizes.resize(m_cipherMode->LegalKeySizes().size());
	// cbc iv is always zero-size with cmac
	for (size_t i = 0; i < m_legalKeySizes.size(); ++i)
		m_legalKeySizes[i] = SymmetricKeySize(m_cipherMode->LegalKeySizes()[i].KeySize(), 0, m_cipherMode->LegalKeySizes()[i].InfoSize());
}

NAMESPACE_MACEND