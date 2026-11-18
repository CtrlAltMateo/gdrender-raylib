#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace PlatformUtils
{

//this is read only outside the namespace. dont change
inline std::filesystem::path resourcesPath = "Resources";
inline std::filesystem::path customResourcesPath = "CustomResources";

std::optional<std::filesystem::path> getGDAppdata();
std::optional<std::string> httpPost(const std::string& url,
									const std::vector<std::pair<std::string, std::string>>& fields);

std::filesystem::path getApplicationPath();
bool updateResourcesPath();
bool isValidResourcesPath(const std::filesystem::path&);
bool saveResourcesPath(const std::filesystem::path& s);
bool restoreResourcesPath();

std::filesystem::path getMainResource(const auto& whatever)
{
	return resourcesPath / whatever;
}

std::filesystem::path getCustomResource(const auto& whatever)
{
	return customResourcesPath / whatever;
}

} //namespace PlatformUtils
