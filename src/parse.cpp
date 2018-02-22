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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <mastodon-cpp.hpp>
#include "mastorss.hpp"

namespace pt = boost::property_tree;

using std::cerr;
using std::string;

// Translate &#0123; to chars, translate some named entities to chars
void unescape_html(string &str)
{
    string html = str;
    str = "";
    // Used to convert int to utf-8 char
    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> u8c;
    std::regex reentity("&#(\\d{4});");
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

std::vector<string> parse_website(const string &profile, const string &xml)
{
    pt::ptree json;
    std::vector<string> watchwords;

    try
    {
        pt::read_json(filepath + "watchwords.json", json);
    }
    catch (std::exception &e)
    {
        // most likely file not found
        cerr << "ERROR: " << filepath << "watchwords.json not found or not readable.\n";
        cerr << e.what() << '\n';
        return {};
    }

    try
    {
        for (const pt::ptree::value_type &value : json.get_child(profile + ".tags"))
        {
            watchwords.push_back(value.second.data());
        }
    }
    catch (const std::exception &e)
    {
        // Node not found, no problem
    }
    try
    {
        for (const pt::ptree::value_type &value : json.get_child("global.tags"))
        {
            watchwords.push_back(value.second.data());
        }
    }
    catch (const std::exception &e)
    {
        // Node not found, no problem
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
                string str = title + "\n\n" + desc;

                // ANF News puts this always on top, causing us to think it's new
                if (title.compare("Newsticker zu den Angriffen auf Efrîn ") == 0)
                {
                    continue;
                }

                unescape_html(str);

                std::regex reparagraph("</p><p>");
                std::regex recdata1("<!\\[CDATA\\[");
                std::regex recdata2("\\]\\]>");
                std::regex restrip("<[^>]*>");

                // de.indymedia.org articles sometimes have CSS in the description
                std::regex reindyfuckup("\\/\\* Style Definitions \\*\\/[.[:space:]]*$");
                // Direkte Action closing
                std::regex redaclosing("Der Beitrag .* erschien zuerst auf Direkte Aktion.");
                // GG/BO closing
                std::regex reggboclosing("Die von den einzelnen AutorInnen .*$");

                str = std::regex_replace(str, reparagraph, "\n\n");
                str = std::regex_replace(str, recdata1, "");
                str = std::regex_replace(str, recdata2, "");
                str = std::regex_replace(str, restrip, "");
                str = std::regex_replace(str, reindyfuckup, "");
                str = std::regex_replace(str, redaclosing, "");
                str = std::regex_replace(str, reggboclosing, "");
                str = std::regex_replace(str, std::regex("\\n \\n]"), "\n\n");      // remove space between newlines
                str = std::regex_replace(str, std::regex("[\\n\\r ]{3,}"), "\n");   // remove excess newlines

                for (const string &hashtag : watchwords)
                {
                    std::regex rehashtag("([[:space:][:punct:]]|^)(" + hashtag + ")([[:space:][:punct:]]|$)",
                                         std::regex_constants::icase);
                    str = std::regex_replace(str, rehashtag, "$1#$2$3",
                                             std::regex_constants::format_first_only);
                }
                if ((str.size() + link.size()) > (std::uint16_t)(max_size - 15))
                {
                    str.resize((max_size - link.size() - 15));
                    str.resize(str.rfind(' '));
                    str += " […]";
                }
                str += "\n\n" + link + "\n\n#bot";
                ret.push_back(str);
            }
        }
    }

    return ret;
}
