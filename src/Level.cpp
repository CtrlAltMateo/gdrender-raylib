#include "Level.h"

#include <string>
#include <algorithm>

#include "Common.h"
#include "base64.h"

#include <raylib.h>

std::string GameLevel::decompressLvlStr(std::string compressedLvlStr)
{
	if (compressedLvlStr.empty()) return "";

	std::replace(compressedLvlStr.begin(), compressedLvlStr.end(), '_', '/');
	std::replace(compressedLvlStr.begin(), compressedLvlStr.end(), '-', '+');

	std::string decoded = base64_decode(compressedLvlStr);

	unsigned char* data = (unsigned char*)decoded.data();

	unsigned char* a = nullptr;
	int64_t deflatedLen = Common::inflateMemory(data, decoded.length(), &a);
	if (deflatedLen < 0 || !a)
		return "";

	std::string levelString = std::string((const char*)a, deflatedLen);

	MemFree(a);

	return levelString;
}