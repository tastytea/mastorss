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
#include <cstdlib>
#include <random>
#include <regex>
#include <sstream>
#include <thread>
#include <chrono>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <mastodon-cpp.hpp>
#include "version.hpp"
#include "mastorss.hpp"

namespace pt = boost::property_tree;
using Mastodon::API;
using std::cout;
using std::cerr;
using std::cin;
using std::string;

std::uint16_t max_size = 500;
const string filepath = string(getenv("HOME")) + "/.config/mastorss/";

std::uint16_t read_config(pt::ptree &config, const string &profile, string &instance, string &access_token, string &feedurl)
{
    bool config_changed = false;

    // Read config file, get access token
    try {
        pt::read_json(filepath + "config-" + profile + ".json", config);
        instance = config.get(profile + ".instance", "");
        access_token = config.get(profile + ".access_token", "");
        feedurl = config.get(profile + ".feedurl", "");
    }
    catch (std::exception &e)
    {
        // most likely no config file found
        cout << "Config file not readable. Building new one.\n";
        const boost::filesystem::path path(filepath);
        boost::filesystem::create_directory(filepath);
    }

    if (instance.empty())
    {
        cout << "Instance: ";
        cin >> instance;
        config.put(profile + ".instance", instance);
        config_changed = true;
    }
    if (access_token.empty())
    {
        cout << "No access token found.\n";
        string client_id, client_secret, url;
        Mastodon::API masto(instance, "");
        std::uint16_t ret = masto.register_app1(instance,
                                                "mastorss",
                                                "urn:ietf:wg:oauth:2.0:oob",
                                                "write",
                                                "",
                                                client_id,
                                                client_secret,
                                                url);
        if (ret == 0)
        {
            string code;
            cout << "Visit " << url << " to authorize this application.\n";
            cout << "Insert code: ";
            cin >> code;

            masto.register_app2(instance,
                                client_id,
                                client_secret,
                                "urn:ietf:wg:oauth:2.0:oob",
                                code,
                                access_token);
            if (ret == 0)
            {
                config.put(profile + ".access_token", access_token);
                config_changed = true;
            }
            else
            {
                cerr << "Error code: " << ret << '\n';
                return ret;
            }
        }
        else
        {
            cerr << "Error code: " << ret << '\n';
            return ret;
        }
        
    }
    if (feedurl.empty())
    {
        cout << "feedurl: ";
        cin >> feedurl;
        config.put(profile + ".feedurl", feedurl);
        config_changed = true;
    }
    if (config_changed)
    {
        pt::write_json(filepath + "config-" + profile + ".json", config);
    }

    return 0;
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
        std::cerr << e.what() << '\n';
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

                str = std::regex_replace(str, relt, "<");
                str = std::regex_replace(str, regt, ">");
                str = std::regex_replace(str, reparagraph, "\n\n");
                str = std::regex_replace(str, recdata1, "");
                str = std::regex_replace(str, recdata2, "");
                str = std::regex_replace(str, restrip, "");
                str = std::regex_replace(str, reindyfuckup, "");

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

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "usage: " << argv[0] << " <profile> [max size]\n";
        return 32;
    }

    if (argc == 3)
    {
        max_size = std::stoi(argv[2]);
    }

    pt::ptree config;
    string instance = "";
    string access_token = "";
    string feedurl = "";
    const string profile = argv[1];
    std::uint16_t ret;

    read_config(config, profile, instance, access_token, feedurl);
    curlpp_init();

    string answer;
    string last_entry = config.get(profile + ".last_entry", "");
    std::vector<string> entries;

    ret = http_get(feedurl, answer, "mastorss/" + (string)global::version);
    if (ret != 0)
    {
        return ret;
    }
    entries = parse_website(profile, answer);

    if (last_entry.empty())
    {
        last_entry = entries.at(1);
    }
    config.put(profile + ".last_entry", entries.front());

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
        Mastodon::API masto(instance, access_token);

        API::parametermap parameters =
        {
            { "status", { *rit } },
            { "visibility", { "public" } }
        };
        ret = masto.post(API::v1::statuses, parameters, answer);

        if (ret == 0)
        {
            pt::write_json(filepath + "config-" + profile + ".json", config);
        }
        else
        {
            std::cerr << "Error code: " << ret << '\n';
            std::cerr << answer << '\n';
            return ret;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
