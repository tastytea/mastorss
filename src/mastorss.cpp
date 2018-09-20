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
#include <jsoncpp/json/json.h>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include <mastodon-cpp/easy/all.hpp>
#include "version.hpp"
#include "mastorss.hpp"

using Mastodon::API;
using std::cout;
using std::cerr;
using std::cin;
using std::string;
using std::this_thread::sleep_for;
using std::chrono::seconds;

// Initialize global variables
std::uint16_t max_size = 500;
const string filepath = string(getenv("HOME")) + "/.config/mastorss/";
Json::Value config;
std::string profile;

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

    string instance = "";
    string access_token = "";
    string feedurl = "";
    profile = argv[1];
    std::uint16_t ret;
    string answer;
    std::vector<Mastodon::Easy::Status> entries;

    read_config(instance, access_token, feedurl);
    curlpp_init();

    ret = http_get(feedurl, answer, "mastorss/" + (string)global::version);
    if (ret != 0)
    {
        std::cerr << "Error code: " << ret << '\n';
        std::cerr << answer << '\n';
        return ret;
    }
    entries = parse_feed(answer);

    string last_entry = config[profile]["last_entry"].asString();
    if (last_entry.empty())
    {
        // If no last_entry is stored in the config file,
        // make last_entry the second-newest entry.
        last_entry = entries.at(1).content();
    }
    config[profile]["last_entry"] = entries.front().content();

    bool new_content = false;
    for (auto rit = entries.rbegin(); rit != entries.rend(); ++rit)
    {
        if (!new_content && (*rit).content().compare(last_entry) == 0)
        {
            // If the last entry is found in entries,
            // start tooting in the next loop.
            new_content = true;
            continue;
        }
        else if (!new_content)
        {
            continue;
        }

        Mastodon::Easy::Status answer;
        Mastodon::Easy masto(instance, access_token);

        answer = masto.send_post(*rit, ret);
        if (answer.to_object().isNull())
        {
            std::cerr << "Could not send post for unknown reasons.\n";
            std::cerr << "Please file a bug at <https://schlomp.space/tastytea/mastorss/issues>.\n";
            return 1;
        }

        if (ret != 0)
        {
            std::cerr << "Error code: " << ret << '\n';
            std::cerr << answer.to_object().asString() << '\n';
            return ret;
        }

        if (rit != entries.rend())
        {   // Only sleep if this is not the last entry
            sleep_for(seconds(config[profile]["interval"].asUInt64()));
        }
    }

    // Write the new last_entry only if no error happened.
    write_config();

    return 0;
}
