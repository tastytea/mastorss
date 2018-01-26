/*  This file is part of rss2mastodon.
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
#include <cstdlib>
#include <random>
#include <regex>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <mastodon-cpp.hpp>
#include "rss2mastodon.hpp"

namespace pt = boost::property_tree;
using Mastodon::API;
using std::cout;
using std::cerr;
using std::string;

const string filepath = string(getenv("HOME")) + "/.config/rss2mastodon/";

void read_config(pt::ptree &config, const string &profile, string &instance, string &access_token, string &feedurl)
{
    bool config_changed = false;

    // Read config file, get access token
    try {
        pt::read_json(filepath + "config.json", config);
        instance = config.get(profile + ".instance", "");
        access_token = config.get(profile + ".access_token", "");
        feedurl = config.get(profile + ".feedurl", "");
    }
    catch (std::exception &e)
    {
        // most likely no config file found
        cout << "No config file found. Building new one.\n";
        const boost::filesystem::path path(filepath);
        boost::filesystem::create_directory(filepath);
    }

    if (instance.empty())
    {
        cout << "Instance: ";
        std::cin >> instance;
        config.put(profile + ".instance", instance);
        config_changed = true;
    }
    if (access_token.empty())
    {
        cout << "access_token: ";
        std::cin >> access_token;
        config.put(profile + ".access_token", access_token);
        config_changed = true;
    }
    if (feedurl.empty())
    {
        cout << "feedurl: ";
        std::cin >> feedurl;
        config.put(profile + ".feedurl", feedurl);
        config_changed = true;
    }
    if (config_changed)
    {
        pt::write_json(filepath + "config.json", config);
    }
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
        std::cerr << "ERROR: " << filepath << "watchwords.json not found or not readable.\n";
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

                for (const string &hashtag : watchwords)
                {
                    std::regex rehashtag("\\b(" + hashtag + ")\\b", std::regex_constants::icase);
                    str = std::regex_replace(str, rehashtag, "#$1");
                }
                str += "\n\n" + link + "\n\n#bot";
                ret.push_back(str);
            }
        }
    }

    return ret;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "usage: " << argv[0] << " <profile>\n";
        return 32;
    }

    pt::ptree config;
    string instance = "";
    string access_token = "";
    string feedurl = "";
    const string profile = argv[1];

    read_config(config, profile, instance, access_token, feedurl);
    std::size_t pos = 0;
    pos = feedurl.find("//") + 2;
    const string hostname = feedurl.substr(pos, feedurl.find('/', pos) - pos);
    const string path = feedurl.substr(pos + hostname.size());

    string answer;
    string last_entry = config.get(profile + ".last_entry", "");
    std::vector<string> entries;
    http_get(hostname, path, answer);
    entries = parse_website(profile, answer);

    config.put(profile + ".last_entry", entries.front());
    pt::write_json(filepath + "config.json", config);

    bool new_content = false;
    for (auto rit = entries.rbegin(); rit != entries.rend(); ++rit)
    {
        if (!new_content && (*rit).compare(last_entry) == 0)
        {
            new_content = true;
            continue;
        }
        else if (!new_content)
        {
            continue;
        }

        string answer;
        std::uint16_t ret;
        Mastodon::API masto(instance, access_token);

        API::parametermap parameters =
        {
            { "status", { *rit } },
            { "visibility", { "public" } }
        };
        ret = masto.post(API::v1::statuses, parameters, answer);

        if (ret == 0)
        {
            //cout << answer << '\n';
        }
        else
        {
            std::cerr << "Error code: " << ret << '\n';
            return ret;
        }
    }

    return 0;
}
