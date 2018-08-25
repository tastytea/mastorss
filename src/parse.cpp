/*  This file is part of mastorss.
 *  Copyright © 2018 tastytea <tastytea@tastytea.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <regex>
#include <sstream>
#include <locale>
#include <codecvt>
#include <fstream>
#include <jsoncpp/json/json.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include <mastodon-cpp/easy/all.hpp>
#include "mastorss.hpp"

using std::cerr;
using std::string;
namespace pt = boost::property_tree;

std::vector<Mastodon::Easy::Status> parse_website(const string &xml)
{
    Json::Value list;
    std::vector<string> watchwords;

    std::ifstream file(filepath + "watchwords.json");
    if (file.is_open())
    {
        std::stringstream json;
        json << file.rdbuf();
        file.close();
        json >> list;
    }
    else
    {
        cerr << "ERROR: " << filepath << "watchwords.json not found or not readable.\n";
        return {};
    }

    // Read profile-specific hashtags or fail silently
    for (const Json::Value &value : list[profile]["tags"])
    {
        watchwords.push_back(value.asString());
    }

    // Read global hashtags or fail silently
    for (const Json::Value &value : list["global"]["tags"])
    {
        watchwords.push_back(value.asString());
    }

    pt::ptree rss;
    std::istringstream iss(xml);
    pt::read_xml(iss, rss);
    std::vector<Mastodon::Easy::Status> ret;

    for (const pt::ptree::value_type &v : rss.get_child("rss.channel"))
    {
        if (v.second.size() > 0)
        {
            if (string(v.first.data()).compare("item") == 0)
            {
                string title = v.second.get_child("title").data();
                string link = v.second.get_child("link").data();
                string desc = v.second.get_child("description").data();

                Mastodon::Easy::Status status;
                string content = "";
                if (config[profile]["titles_as_cw"].asBool())
                {
                    status.spoiler_text(title);
                }
                else
                {
                    content = title;
                }
                if (!config[profile]["titles_only"].asBool())
                {
                    if (!content.empty())
                    {
                        content += "\n\n";
                    }
                    content += desc;

                    // Shrink overly long texts, to speed up replace operations
                    if (content.length() > 2000)
                    {
                        content.resize(2000);
                    }
                }

                bool skipthis = false;
                try
                {
                    // Skip entries beginning with this text
                    for (const Json::Value &v : config[profile]["skip"])
                    {
                        const string skip = v.asString();
                        if (!skip.empty())
                        {
                            if (title.compare(0, skip.length(), skip) == 0)
                            {
                                skipthis = true;
                                break;
                            }
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    // Node not found, no problem
                }
                if (skipthis)
                {
                    continue;
                }

                content = Mastodon::API::unescape_html(content);

                // Try to turn the HTML into human-readable text
                std::regex reparagraph("<p>");
                std::regex recdata1("<!\\[CDATA\\[");
                std::regex recdata2("\\]\\]>");
                std::regex restrip("<[^>]*>");

                individual_fixes(content);

                content = std::regex_replace(content, reparagraph, "\n\n");
                content = std::regex_replace(content, recdata1, "");
                content = std::regex_replace(content, recdata2, "");
                content = std::regex_replace(content, restrip, "");
                // remove \r
                content = std::regex_replace(content, std::regex("\\r"), "");
                // replace NO-BREAK SPACE with space (UTF-8: 0xc2a0)
                content = std::regex_replace(content, std::regex("\u00a0"), " ");
                // remove whitespace between newlines
                content = std::regex_replace(content, std::regex("\\n[ \t]+\\n"), "");
                // remove excess newlines
                content = std::regex_replace(content, std::regex("\\n{3,}"), "\n\n");

                for (const string &hashtag : watchwords)
                {
                    std::regex rehashtag("([[:space:][:punct:]]|^)(" + hashtag +
                                         ")([[:space:][:punct:]]|$)", std::regex_constants::icase);
                    content = std::regex_replace(content, rehashtag, "$1#$2$3",
                                                 std::regex_constants::format_first_only);
                }
                // Why is this necessary? Why does ##hashtag happen?
                content = std::regex_replace(content, std::regex("##"), "#");

                uint16_t appendix_size = config[profile]["append"].asString().length();
                if ((content.size() + link.size() + appendix_size)
                    > static_cast<std::uint16_t>(max_size - 4))
                {
                    content.resize((max_size - link.size() - appendix_size - 4));
                    content.resize(content.rfind(' ')); // Cut at word boundary
                    content += " […]";
                }
                // Remove trailing newlines
                while (content.back() == '\n' ||
                       content.back() == '\r')
                {
                    content.resize(content.length() - 1);
                }

                content += "\n\n" + link;

                if (!config[profile]["append"].empty())
                {
                    content += "\n\n" + config[profile]["append"].asString();
                }
                status.content(content);
                ret.push_back(status);
            }
        }
    }

    return ret;
}

// Read regular expressions from the config file and delete all matches.
void individual_fixes(string &str)
{
    for (const Json::Value &v : config[profile]["fixes"])
    {
        std::regex refix(v.asString());
        str = std::regex_replace(str, refix, "");
    }
}
