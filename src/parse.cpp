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
#include "mastorss.hpp"

using std::cerr;
using std::string;
namespace pt = boost::property_tree;

// Translate &#0123; to chars, translate some named entities to chars
void unescape_html(string &str)
{
    string html = str;
    str = "";
    // Used to convert int to utf-8 char
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> u8c;
    std::regex reentity("&#(\\d{2,4});");
    std::smatch match;
    
    while (std::regex_search(html, match, reentity))
    {
        str += match.prefix().str() + u8c.to_bytes(std::stoi(match[1].str()));
        html = match.suffix().str();
    }
    str += html;

    std::regex relt("&lt;");
    std::regex regt("&gt;");
    std::regex reamp("&amp;");
    std::regex requot("&quot;");
    std::regex reapos("&apos;");

    str = std::regex_replace(str, relt, "<");
    str = std::regex_replace(str, regt, ">");
    str = std::regex_replace(str, reamp, "&");
    str = std::regex_replace(str, requot, "\"");
    str = std::regex_replace(str, reapos, "\'");
}

std::vector<string> parse_website(const string &xml)
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
    std::vector<string> ret;

    for (const pt::ptree::value_type &v : rss.get_child("rss.channel"))
    {
        if (v.second.size() > 0)
        {
            if (string(v.first.data()).compare("item") == 0)
            {
                string title = v.second.get_child("title").data();
                string link = v.second.get_child("link").data();
                string desc = v.second.get_child("description").data();

                string str = title;
                if (!config[profile]["titles_only"].asBool())
                {
                    str += "\n\n" + desc;
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

                unescape_html(str);

                // Try to turn the HTML into human-readable text
                std::regex reparagraph("<p>");
                std::regex recdata1("<!\\[CDATA\\[");
                std::regex recdata2("\\]\\]>");
                std::regex restrip("<[^>]*>");

                individual_fixes(str);

                str = std::regex_replace(str, reparagraph, "\n\n");
                str = std::regex_replace(str, recdata1, "");
                str = std::regex_replace(str, recdata2, "");
                str = std::regex_replace(str, restrip, "");
                str = std::regex_replace(str, std::regex("\\r"), "");           // remove \r
                str = std::regex_replace(str, std::regex("\uc2a0"), " ");       // replace NO-BREAK SPACE with space
                str = std::regex_replace(str, std::regex("\\n[ \t]+\\n"), "");  // remove whitespace between newlines
                str = std::regex_replace(str, std::regex("\\n{3,}"), "\n\n");   // remove excess newlines

                for (const string &hashtag : watchwords)
                {
                    std::regex rehashtag("([[:space:][:punct:]]|^)(" + hashtag + ")([[:space:][:punct:]]|$)",
                                         std::regex_constants::icase);
                    str = std::regex_replace(str, rehashtag, "$1#$2$3",
                                             std::regex_constants::format_first_only);
                }
                // Why is this necessary? Why does ##hashtag happen?
                str = std::regex_replace(str, std::regex("##"), "#");
                if ((str.size() + link.size()) > static_cast<std::uint16_t>(max_size - 15))
                {
                    str.resize((max_size - link.size() - 15));
                    str.resize(str.rfind(' ')); // Cut at word boundary
                    str += " […]";
                }
                // Remove trailing newlines
                while (str.back() == '\n' ||
                       str.back() == '\r')
                {
                    str.resize(str.length() - 1);
                }
                str += "\n\n" + link + "\n\n#bot";
                ret.push_back(str);
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
