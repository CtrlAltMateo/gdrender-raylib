#include "Common.h"

#include <raylib.h>

#include <charconv>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <sys/types.h>

namespace
{

uint32_t readLittleEndian(const unsigned char* data)
{
	return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
		   (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

uint32_t readBigEndian(const unsigned char* data)
{
	return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
		   (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

uint32_t calculateCrc32(const unsigned char* data, int length)
{
	uint32_t crc = 0xffffffff;
	for (int i = 0; i < length; ++i)
	{
		crc ^= data[i];
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
	}
	return ~crc;
}

uint32_t calculateAdler32(const unsigned char* data, int length)
{
	uint32_t a = 1;
	uint32_t b = 0;
	for (int i = 0; i < length; ++i)
	{
		a = (a + data[i]) % 65521;
		b = (b + a) % 65521;
	}
	return (b << 16) | a;
}

}

int64_t Common::inflateMemory(unsigned char* in, int64_t inLength, unsigned char** out)
{
	*out = nullptr;
	if (!in || inLength < 6 || inLength > std::numeric_limits<int>::max())
		return -1;

	const unsigned char* compressed = nullptr;
	size_t compressedLength = 0;
	const unsigned char* trailer = nullptr;
	bool gzip = false;

	if (inLength >= 18 && in[0] == 0x1f && in[1] == 0x8b)
	{
		gzip = true;
		unsigned char flags = in[3];
		size_t offset = 10;
		size_t trailerOffset = static_cast<size_t>(inLength) - 8;
		if ((flags & 0xe0) != 0 || offset > trailerOffset)
			return -1;

		if ((flags & 0x04) != 0)
		{
			if (offset + 2 > trailerOffset)
				return -1;
			size_t extraLength = in[offset] | (static_cast<size_t>(in[offset + 1]) << 8);
			offset += 2;
			if (extraLength > trailerOffset - offset)
				return -1;
			offset += extraLength;
		}

		for (unsigned char flag : {0x08, 0x10})
		{
			if ((flags & flag) == 0)
				continue;
			while (offset < trailerOffset && in[offset] != 0)
				++offset;
			if (offset == trailerOffset)
				return -1;
			++offset;
		}

		if ((flags & 0x02) != 0)
			offset += 2;
		if (offset > trailerOffset)
			return -1;

		compressed = in + offset;
		compressedLength = trailerOffset - offset;
		trailer = in + trailerOffset;
	}
	else
	{
		unsigned char cmf = in[0];
		unsigned char flg = in[1];
		if ((cmf & 0x0f) != 8 || (cmf >> 4) > 7 || ((cmf << 8) | flg) % 31 != 0 || (flg & 0x20) != 0)
			return -1;

		compressed = in + 2;
		compressedLength = static_cast<size_t>(inLength) - 6;
		trailer = in + inLength - 4;
	}

	if (compressedLength > std::numeric_limits<int>::max())
		return -1;

	int outputLength = 0;
	unsigned char* output = DecompressData(compressed, static_cast<int>(compressedLength), &outputLength);
	if (!output || outputLength < 0)
	{
		if (output)
			MemFree(output);
		TraceLog(LOG_ERROR, "Failed to decompress level data");
		return -1;
	}

	uint32_t expectedChecksum = gzip ? readLittleEndian(trailer) : readBigEndian(trailer);
	uint32_t actualChecksum = gzip ? calculateCrc32(output, outputLength) : calculateAdler32(output, outputLength);
	uint32_t expectedLength = gzip ? readLittleEndian(trailer + 4) : static_cast<uint32_t>(outputLength);
	if (actualChecksum != expectedChecksum || (gzip && static_cast<uint32_t>(outputLength) != expectedLength))
	{
		MemFree(output);
		TraceLog(LOG_ERROR, "Compressed level data failed its checksum");
		return -1;
	}

	*out = output;
	return outputLength;
}

std::vector<std::string> Common::splitByDelim(const std::string& str, char delim)
{
	std::vector<std::string> tokens;
	size_t pos = 0;
	size_t len = str.length();
	tokens.reserve(len / 2);

	while (pos < len)
	{
		size_t end = str.find_first_of(delim, pos);
		if (end == std::string::npos)
		{
			tokens.emplace_back(str.substr(pos));
			break;
		}
		tokens.emplace_back(str.substr(pos, end - pos));
		pos = end + 1;
	}

	return tokens;
}

std::vector<std::string_view> Common::splitByDelimStringView(std::string_view str, char delim)
{
	std::vector<std::string_view> tokens;
	size_t pos = 0;
	size_t len = str.length();

	while (pos < len)
	{
		size_t end = str.find(delim, pos);
		if (end == std::string_view::npos)
		{
			tokens.emplace_back(str.substr(pos));
			break;
		}
		tokens.emplace_back(str.substr(pos, end - pos));
		pos = end + 1;
	}

	return tokens;
}

int Common::stoi(std::string_view s)
{
	int ret = 0.0f;
	std::from_chars(s.data(), s.data() + s.size(), ret);
	return ret;
}

float Common::stof(std::string_view s)
{
	float ret = 0.0f;
	std::from_chars(s.data(), s.data() + s.size(), ret);
	return ret;
}

int Common::sectionForPos(float x)
{
	int section = x / 100;
	if (section < 0)
		section = 0;
	return section;
}

void Common::xorString(std::string& str, int key)
{
	for (char& c : str)
	{
		c = c ^ key;
	}
}

glm::mat4 Common::rotateMatrixAroundPoint(const glm::mat4& originalMatrix, float angle, const glm::vec2& center)
{
	const float rad = glm::radians(angle);

	glm::vec3 center3 = glm::vec3(center, 0);

	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), rad, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 translation1 = glm::translate(glm::mat4(1.0f), -center3);

	glm::mat4 translation2 = glm::translate(glm::mat4(1.0f), center3);

	return translation2 * rotation * translation1;
}

glm::vec2 Common::transformPoint(glm::vec2 point, glm::mat4& mat)
{
	return glm::vec2(mat[0][0] * point.x + mat[1][0] * point.y + mat[3][0], mat[0][1] * point.x + mat[1][1] * point.y + mat[3][1]);
}