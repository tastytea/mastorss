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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <mastodon-cpp.hpp>
#include "mastorss.hpp"

namespace pt = boost::property_tree;

using std::cerr;
using std::string;

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

                // Some feeds contain encoded xhtml-tags >:|
                std::regex relt("&lt;");
                std::regex regt("&gt;");
                std::regex reparagraph("</p><p>");
                std::regex recdata1("<!\\[CDATA\\[");
                std::regex recdata2("\\]\\]>");
                std::regex restrip("<[^>]*>");
                std::regex reindyfuckup("\\/\\* Style Definitions \\*\\/[.[:space:]]*$");
                // Direkte Action closing
                std::regex redaclosing("Der Beitrag .* erschien zuerst auf Direkte Aktion.");
                // GG/BO closing
                std::regex reggboclosing("Die von den einzelnen AutorInnen .*$");

                str = std::regex_replace(str, relt, "<");
                str = std::regex_replace(str, regt, ">");
                str = std::regex_replace(str, reparagraph, "\n\n");
                str = std::regex_replace(str, recdata1, "");
                str = std::regex_replace(str, recdata2, "");
                str = std::regex_replace(str, restrip, "");
                str = std::regex_replace(str, reindyfuckup, "");
                str = std::regex_replace(str, redaclosing, "");
                str = std::regex_replace(str, reggboclosing, "");
                str = std::regex_replace(str, std::regex("[\\n\\r]{3,}"), "\n");    // remove excess newlines

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
                    str += " […]";
                }
                str += "\n\n" + link + "\n\n#bot";
                ret.push_back(str);
            }
        }
    }

    return ret;
}
