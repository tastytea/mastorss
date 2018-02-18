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
#include <cstdlib>  // getenv()
#include <cstdint>
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

// Initialize global variables
std::uint16_t max_size = 500;
const string filepath = string(getenv("HOME")) + "/.config/mastorss/";

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << "usage: " << argv[0] << " <profile> [max size]\n";
        return 10;
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

        if (ret != 0)
        {
            std::cerr << "Error code: " << ret << '\n';
            std::cerr << answer << '\n';
            return ret;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    pt::write_json(filepath + "config-" + profile + ".json", config);

    return 0;
}
