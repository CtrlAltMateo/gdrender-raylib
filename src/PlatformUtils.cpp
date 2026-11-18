#include "PlatformUtils.h"
#include <optional>
#include "Application.h"
#include <array>
#include <fstream>
#include <cassert>
#include <portable-file-dialogs/portable-file-dialogs.h>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#ifdef _WIN32
#	include <windows.h>
#	include <shlobj.h>
#	include <winhttp.h>

namespace
{

struct HttpHandle
{
	HINTERNET value = nullptr;

	~HttpHandle()
	{
		if (value)
			WinHttpCloseHandle(value);
	}
};

std::string encodeFormValue(const std::string& value)
{
	static constexpr char digits[] = "0123456789ABCDEF";
	std::string encoded;
	for (unsigned char character : value)
	{
		if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') || character == '-' || character == '_' || character == '.' ||
			character == '~')
		{
			encoded.push_back(static_cast<char>(character));
		}
		else
		{
			encoded.push_back('%');
			encoded.push_back(digits[character >> 4]);
			encoded.push_back(digits[character & 15]);
		}
	}
	return encoded;
}

}

std::optional<std::string> PlatformUtils::httpPost(
	const std::string& url, const std::vector<std::pair<std::string, std::string>>& fields)
{
	std::wstring wideUrl(url.begin(), url.end());
	URL_COMPONENTS components = {};
	components.dwStructSize = sizeof(components);
	components.dwHostNameLength = static_cast<DWORD>(-1);
	components.dwUrlPathLength = static_cast<DWORD>(-1);
	components.dwExtraInfoLength = static_cast<DWORD>(-1);
	if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &components))
	{
		std::cerr << "Unable to parse request URL: " << GetLastError() << '\n';
		return {};
	}

	std::wstring host(components.lpszHostName, components.dwHostNameLength);
	std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
	if (components.dwExtraInfoLength > 0)
		path.append(components.lpszExtraInfo, components.dwExtraInfoLength);

	HttpHandle session{WinHttpOpen(L"", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
								   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
	if (!session.value)
	{
		std::cerr << "Unable to open WinHTTP session: " << GetLastError() << '\n';
		return {};
	}

	HttpHandle connection{WinHttpConnect(session.value, host.c_str(), components.nPort, 0)};
	if (!connection.value)
	{
		std::cerr << "Unable to connect to level server: " << GetLastError() << '\n';
		return {};
	}

	DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
	HttpHandle request{WinHttpOpenRequest(connection.value, L"POST", path.c_str(), nullptr,
										  WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags)};
	if (!request.value)
	{
		std::cerr << "Unable to create level request: " << GetLastError() << '\n';
		return {};
	}

	std::string body;
	for (const auto& [key, value] : fields)
	{
		if (!body.empty())
			body.push_back('&');
		body += encodeFormValue(key);
		body.push_back('=');
		body += encodeFormValue(value);
	}

	static constexpr wchar_t headers[] = L"Content-Type: application/x-www-form-urlencoded\r\n";
	if (!WinHttpSendRequest(request.value, headers, static_cast<DWORD>(-1), body.data(),
							static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0) ||
		!WinHttpReceiveResponse(request.value, nullptr))
	{
		std::cerr << "Level request failed: " << GetLastError() << '\n';
		return {};
	}

	DWORD statusCode = 0;
	DWORD statusSize = sizeof(statusCode);
	if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
							 WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) ||
		statusCode < 200 || statusCode >= 300)
	{
		std::cerr << "Level server returned HTTP status " << statusCode << '\n';
		return {};
	}

	std::string response;
	DWORD available = 0;
	do
	{
		if (!WinHttpQueryDataAvailable(request.value, &available))
		{
			std::cerr << "Unable to read level response: " << GetLastError() << '\n';
			return {};
		}

		std::vector<char> buffer(available);
		DWORD received = 0;
		if (available > 0 && !WinHttpReadData(request.value, buffer.data(), available, &received))
		{
			std::cerr << "Unable to read level response: " << GetLastError() << '\n';
			return {};
		}
		if (received > 0)
			response.append(buffer.data(), received);
	} while (available > 0);

	return response;
}

std::optional<fs::path> PlatformUtils::getGDAppdata()
{
	PWSTR path_tmp;

	auto get_folder_path_ret = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path_tmp);

	/* Error check */
	if (get_folder_path_ret != S_OK)
	{
		CoTaskMemFree(path_tmp);
		return {};
	}

	fs::path path{path_tmp};
	CoTaskMemFree(path_tmp);

	return path / "GeometryDash";
}

#else

std::optional<std::string> PlatformUtils::httpPost(
	const std::string&, const std::vector<std::pair<std::string, std::string>>&)
{
	std::cerr << "Level server requests are only supported on Windows\n";
	return {};
}

std::optional<fs::path> PlatformUtils::getGDAppdata()
{
	return {};
}

#endif

fs::path PlatformUtils::getApplicationPath()
{
	auto path = fs::temp_directory_path() / "GDRender";
	if (!fs::exists(path))
	{
		assert(fs::create_directory(path));
	}
	return path;
}

static auto find_super_directory(std::string_view suffix, const fs::path& start_path)
{
	for (auto path = fs::absolute(start_path); !path.empty() && path != path.parent_path(); path = path.parent_path())
	{
		auto ret = path / suffix;
		if (fs::is_directory(ret))
		{
			return ret;
		}
	}
	return fs::path();
}

bool PlatformUtils::updateResourcesPath()
{
	if (!fs::exists(PlatformUtils::customResourcesPath))
	{
		fs::path custom = find_super_directory("CustomResources", fs::current_path());
		if (!fs::exists(custom))
		{
			pfd::message("Error",
						 "Could not find the CustomResources folder. The CustomResources folder needs to be in the "
						 "executable location or one of its parent folders",
						 pfd::choice::ok, pfd::icon::error)
				.ready();
			return false;
		}
		PlatformUtils::customResourcesPath = custom;
	}

	PlatformUtils::restoreResourcesPath();

	if (fs::exists(PlatformUtils::resourcesPath) && PlatformUtils::isValidResourcesPath(PlatformUtils::resourcesPath))
		return true;

	auto m = pfd::message("Could not find Geometry Dash Resources", "Select a Geometry Dash Resources folder",
						  pfd::choice::yes_no, pfd::icon::info);

	if (m.result() != pfd::button::yes)
		return false;

	std::string dir = pfd::select_folder("Select any directory", pfd::path::home()).result();
	if (!PlatformUtils::isValidResourcesPath(dir))
	{
		pfd::message("Error", "Invalid Resources folder", pfd::choice::ok, pfd::icon::error).result();
		return false;
	}
	PlatformUtils::resourcesPath = dir;
	auto res = pfd::message("Remember", "Remember folder?", pfd::choice::yes_no, pfd::icon::question).result();
	if (res == pfd::button::yes)
	{
		PlatformUtils::saveResourcesPath(dir);
	}
	return true;
}

bool PlatformUtils::isValidResourcesPath(const std::filesystem::path& p)
{
	if (!fs::exists(p))
		return false;

	constexpr std::array testFiles{"game_bg_01_001-uhd.png",	   "GJ_GameSheet-uhd.plist",   "GJ_GameSheet-uhd.png",
								   "GJ_GameSheet02-uhd.plist", "GJ_GameSheet02-uhd.png",   "GJ_GameSheet03-uhd.plist",
								   "GJ_GameSheet03-uhd.png",   "GJ_GameSheet04-uhd.plist", "GJ_GameSheet04-uhd.png"};
	for (const auto& t : testFiles)
	{
		if (!fs::exists(p / t))
			return false;
	}
	return true;
}

bool PlatformUtils::saveResourcesPath(const std::filesystem::path& p)
{
	if (!fs::exists(p))
		return false;

	fs::path path = PlatformUtils::getApplicationPath() / "resources_path.txt";
	if (auto file = std::ofstream(path))
	{
		file << p.string();
		file.close();
		return true;
	}
	return false;
}
bool PlatformUtils::restoreResourcesPath()
{
	std::ifstream t(PlatformUtils::getApplicationPath() / "resources_path.txt");
	if (!t.good())
		return false;

	fs::path newpath((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	if (!fs::exists(newpath))
	{
		return false;
	}

	PlatformUtils::resourcesPath = newpath;
	return true;
}
