#include "StreamReader.h"
#include "CryptoProcessingException.h"

NAMESPACE_IO

byte StreamReader::ReadByte()
{
	if (m_streamData.Position() < m_streamData.Length())
	{
		std::vector<byte> data(1);
		m_streamData.Read(data, 0, 1);
		return data[0];
	}

	throw CEX::Exception::CryptoProcessingException("StreamReader:ReadByte", "The array does not contain enough data!");
}

std::vector<byte> StreamReader::ReadBytes(size_t Length)
{
	if (m_streamData.Position() + Length <= m_streamData.Length())
	{
		std::vector<byte> data(Length);
		m_streamData.Read(data, 0, Length);
		return data;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadBytes", "The array does not contain enough data!");
	}
}

short StreamReader::ReadInt16()
{
	uint sze = sizeof(short);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		short num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadInt16", "The array does not contain enough data!");
	}
}

ushort StreamReader::ReadUInt16()
{
	uint sze = sizeof(ushort);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		ushort num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadUInt16", "The array does not contain enough data!");
	}
}

int StreamReader::ReadInt32()
{
	uint sze = sizeof(int);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		int num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadInt32", "The array does not contain enough data!");
	}
}

uint StreamReader::ReadUInt32()
{
	uint sze = sizeof(uint);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		uint num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadUInt32", "The array does not contain enough data!");
	}
}

long StreamReader::ReadInt64()
{
	uint sze = sizeof(long);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		long num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadInt64", "The array does not contain enough data!");
	}
}

ulong StreamReader::ReadUInt64()
{
	uint sze = sizeof(ulong);

	if (m_streamData.Position() + sze < m_streamData.Length())
	{
		std::vector<byte> data(sze);
		m_streamData.Read(data, 0, sze);
		ulong num(0);
		memcpy(&num, &data[0], sze);
		return num;
	}
	else
	{
		throw CEX::Exception::CryptoProcessingException("StreamReader:ReadUInt64", "The array does not contain enough data!");
	}
}

NAMESPACE_IOEND