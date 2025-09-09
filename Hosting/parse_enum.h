#pragma once
/*
#include "Hosting/xml_spec_reader.h"
*/

#include <string>
#include <vector>
#include <algorithm>
#include <optional>
#include "GmpiApiCommon.h"
#include "GmpiApiEditor.h"

namespace gmpi
{
namespace hosting
{

struct enum_entry
{
	int id{};
	std::string name;
};

inline std::vector<enum_entry> parse_enum_list(std::string_view enum_list)
{
	std::vector<enum_entry> result;

	auto trim = [](std::string_view s) -> std::string_view
	{
		while (!s.empty() && isspace(s.front()))
			s.remove_prefix(1);
		while (!s.empty() && isspace(s.back()))
			s.remove_suffix(1);
		return s;
		};

	size_t start = 0;
	while (start < enum_list.size())
	{
		auto commaPos = enum_list.find(',', start);
		if (commaPos == std::string_view::npos)
			commaPos = enum_list.size();
		auto token = trim(enum_list.substr(start, commaPos - start));
		if (!token.empty())
		{
			auto equalPos = token.find('=');
			enum_entry entry;
			if (equalPos != std::string_view::npos)
			{
				entry.name = std::string(trim(token.substr(0, equalPos)));
				auto idStr = trim(token.substr(equalPos + 1));
				try
				{
					entry.id = std::stoi(std::string(idStr));
				}
				catch (...)
				{
					entry.id = result.empty() ? 0 : result.back().id + 1; // Default
				}
			}
			else
			{
				entry.name = std::string(token);
				entry.id = result.empty() ? 0 : result.back().id + 1; // Default
			}
			result.push_back(entry);
		}
		start = commaPos + 1;
	}
	return result;
}

} // namespace hosting
} // namespace gmpi
