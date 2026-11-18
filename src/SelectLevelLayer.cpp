#include "SelectLevelLayer.h"
#include "GameLayer.h"
#include "Application.h"

#include "Common.h"
#include "base64.h"
#include "PlatformUtils.h"
#include <fstream>
#include <algorithm>
#include <cstring>
#include <raylib.h>

std::shared_ptr<SelectLevelLayer> SelectLevelLayer::create()
{
	std::shared_ptr<SelectLevelLayer> ptr(new SelectLevelLayer);

	if (ptr->init())
		return ptr;

	return nullptr;
}

bool SelectLevelLayer::init()
{
	ImageLoader::ParsePlist("GJ_GameSheet-uhd.plist");
	ImageLoader::ParsePlist("GJ_GameSheet02-uhd.plist");
	ImageLoader::ParsePlist("GJ_GameSheet03-uhd.plist");
	ImageLoader::ParsePlist("GJ_GameSheet04-uhd.plist");

	searchStr[0] = 0;
	searchStrPersistent[0] = 0;

	return true;
}

static std::optional<std::string> getLocalLevelsXml()
{
	auto appdata = PlatformUtils::getGDAppdata();
	if (!appdata)
		return {};

	auto file = appdata.value() / "CCLocalLevels.dat";
	std::ifstream f(file);
	if (!f.good())
		return {};

	std::string xmlstr(std::istreambuf_iterator<char>{f}, {});

	Common::xorString(xmlstr, 11);
	xmlstr = GameLevel::decompressLvlStr(xmlstr);
	return xmlstr;
}
static std::optional<std::vector<std::string>> getLevelNamesFromLocalLevels()
{
	auto xmlopt = getLocalLevelsXml();
	if (!xmlopt)
		return {};

	auto& xmlstr = xmlopt.value();

	auto pos = xmlstr.find("<k>k2</k>");

	if (pos == std::string_view::npos)
		return {};

	std::vector<std::string> names;
	while (pos != std::string_view::npos)
	{
		auto tagStart = xmlstr.find("<s>", pos);
		auto tagEnd = xmlstr.find("</s>", tagStart);
		auto beginning = xmlstr.begin();
		names.emplace_back(beginning + tagStart + 3, beginning + tagEnd);
		pos = xmlstr.find("<k>k2</k>", pos + 9);
	}
	return names;
}

static std::optional<std::string> getLevelStringFromLocalLevels(std::string_view levelName)
{
	auto xmlopt = getLocalLevelsXml();
	if (!xmlopt)
		return {};

	auto& xmlstr = xmlopt.value();

	auto pos = xmlstr.find("<k>k2</k>");
	if (pos == std::string_view::npos)
		return {};

	while (pos != std::string_view::npos)
	{
		auto tagStart = xmlstr.find("<s>", pos);
		auto tagEnd = xmlstr.find("</s>", tagStart);
		auto beginning = xmlstr.begin();
		std::string_view name(beginning + tagStart + 3, beginning + tagEnd);
		if (name == levelName)
		{
			tagStart = xmlstr.find("<k>k4</k>", tagEnd);
			tagStart = xmlstr.find("<s>", tagStart);
			tagEnd = xmlstr.find("</s>", tagStart);
			auto b = xmlstr.begin();
			std::string levelString(b + tagStart + 3, b + tagEnd);
			return GameLevel::decompressLvlStr(levelString);
		}
		pos = xmlstr.find("<k>k2</k>", pos + 9);
	}
	return {};
}

void SelectLevelLayer::GUI()
{
	const Color textColor = {204, 204, 212, 255};
	const Color panelColor = {18, 19, 24, 245};
	const Color buttonColor = {42, 43, 53, 255};
	const Color highlightColor = {78, 100, 89, 255};
	const Vector2 mousePosition = GetMousePosition();
	const bool mousePressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	const int screenWidth = GetScreenWidth();

	DrawRectangle(0, 0, screenWidth, GetScreenHeight(), panelColor);
	DrawText("Select Level", 20, 16, 26, textColor);

	Rectangle searchRect = {20, 54, 360, 38};
	DrawRectangleRec(searchRect, searchFocused ? highlightColor : buttonColor);
	DrawRectangleLinesEx(searchRect, 1, textColor);
	DrawText(searchStr.data(), 30, 64, 18, textColor);

	if (mousePressed)
		searchFocused = CheckCollisionPointRec(mousePosition, searchRect);

	if (searchFocused)
	{
		int character = GetCharPressed();
		size_t length = std::strlen(searchStr.data());
		while (character > 0)
		{
			if (character >= 32 && character < 127 && length + 1 < searchStr.size())
			{
				searchStr[length++] = static_cast<char>(character);
				searchStr[length] = '\0';
			}
			character = GetCharPressed();
		}

		if (IsKeyPressed(KEY_BACKSPACE) && length > 0)
			searchStr[length - 1] = '\0';
	}

	auto button = [&](const char* label, Rectangle bounds) {
		bool hovered = CheckCollisionPointRec(mousePosition, bounds);
		DrawRectangleRec(bounds, hovered ? highlightColor : buttonColor);
		DrawRectangleLinesEx(bounds, 1, textColor);
		DrawText(label, static_cast<int>(bounds.x + 10), static_cast<int>(bounds.y + 10), 16, textColor);
		return mousePressed && hovered;
	};

	if (button("Load Level", {400, 54, 120, 38}))
	{
		page = 0;
		tableScroll = 0;
		levels.clear();
		searchLevels(searchStr.data(), page);
		searchStrPersistent = searchStr;
	}

	if (button("More", {530, 54, 80, 38}))
	{
		page++;
		searchLevels(searchStrPersistent.data(), page);
	}

	if (button("Show editor levels", {620, 54, 180, 38}))
	{
		if (auto localLevels = getLevelNamesFromLocalLevels())
		{
			int i = 0;
			for (const auto& name : localLevels.value())
			{
				GameLevel& level = this->levels.emplace_back();
				level.levelID = i++;
				level.name = name;
				level.localLevel = true;
			}
		}
	}

	const int tableX = 20;
	const int tableY = 108;
	const int rowHeight = 32;
	const int tableHeight = GetScreenHeight() - tableY - 20;
	const std::array<int, 7> columnWidths = {240, 110, 80, 380, 70, 100, 100};
	const std::array<const char*, 7> headers = {"Name", "Downloads", "Likes", "Description", "Stars", "ID", "Open"};
	int x = tableX;
	DrawRectangle(tableX, tableY, std::min(screenWidth - tableX * 2, 1080), rowHeight, buttonColor);
	for (size_t column = 0; column < headers.size(); ++column)
	{
		DrawText(headers[column], x + 8, tableY + 8, 16, textColor);
		x += columnWidths[column];
	}

	Rectangle tableBounds = {static_cast<float>(tableX), static_cast<float>(tableY + rowHeight),
							 static_cast<float>(std::min(screenWidth - tableX * 2, 1080)),
							 static_cast<float>(std::max(tableHeight - rowHeight, 0))};
	if (CheckCollisionPointRec(mousePosition, tableBounds))
		tableScroll = std::max(0.0f, tableScroll - GetMouseWheelMove() * rowHeight);

	float maxScroll = std::max(0.0f, levels.size() * rowHeight - tableBounds.height);
	tableScroll = std::min(tableScroll, maxScroll);

	BeginScissorMode(tableX, tableY + rowHeight, static_cast<int>(tableBounds.width), static_cast<int>(tableBounds.height));
	for (size_t i = 0; i < levels.size(); ++i)
	{
		const GameLevel& level = levels[i];
		float rowY = tableY + rowHeight + i * rowHeight - tableScroll;
		if (rowY + rowHeight < tableY + rowHeight || rowY > tableY + tableHeight)
			continue;

		DrawRectangle(tableX, static_cast<int>(rowY), static_cast<int>(tableBounds.width), rowHeight,
					  i % 2 == 0 ? Color{29, 30, 37, 255} : Color{23, 24, 30, 255});

		std::array<std::string, 7> values = {
			level.name,
			std::to_string(level.downloads),
			std::to_string(level.likes),
			level.description,
			std::to_string(level.stars),
			std::to_string(level.levelID),
			""};
		x = tableX;
		for (size_t column = 0; column < values.size() - 1; ++column)
		{
			DrawText(values[column].c_str(), x + 8, static_cast<int>(rowY + 8), 15, textColor);
			x += columnWidths[column];
		}

		Rectangle openRect = {static_cast<float>(x + 4), rowY + 3, static_cast<float>(columnWidths.back() - 8),
							  static_cast<float>(rowHeight - 6)};
		bool hovered = CheckCollisionPointRec(mousePosition, openRect);
		DrawRectangleRec(openRect, hovered ? highlightColor : buttonColor);
		DrawText("Open", static_cast<int>(openRect.x + 12), static_cast<int>(openRect.y + 5), 15, textColor);

		if (mousePressed && hovered)
		{
			if (level.localLevel)
			{
				if (auto lvlstr = getLevelStringFromLocalLevels(level.name))
				{
					std::shared_ptr<GameLayer> gameLayer = GameLayer::createWithLevelString(lvlstr.value());
					Application::instance->pushLayer(gameLayer);
				}
			}
			else
			{
				std::shared_ptr<GameLayer> gameLayer = GameLayer::create(level.levelID);
				Application::instance->pushLayer(gameLayer);
			}
		}
	}
	EndScissorMode();
}

void SelectLevelLayer::searchLevels(std::string searchQuery, int page)
{
	auto response = PlatformUtils::httpPost(
		"http://www.boomlings.com/database/getGJLevels21.php",
		{{"secret", "Wmfd2893gb7"}, {"str", searchQuery}, {"type", "0"}, {"page", std::to_string(page)}});
	if (!response)
		return;

	auto levelSplit = Common::splitByDelim(Common::splitByDelim(*response, '#')[0], '|');

	for (auto& str : levelSplit)
	{
		GameLevel level = GameLevel::fromString(str);
		levels.push_back(level);
	}
}
