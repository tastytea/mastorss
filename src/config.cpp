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
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <experimental/filesystem>
#include <jsoncpp/json/json.h>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include "mastorss.hpp"

using std::cout;
using std::cerr;
using std::cin;
using std::string;
namespace fs = std::experimental::filesystem;

std::uint16_t read_config(string &instance, string &access_token, string &feedurl)
{
    bool config_changed = false;

    // Read config file, get access token
    std::ifstream file(filepath + "config-" + profile + ".json");
    if (file.is_open())
    {
        std::stringstream json;
        json << file.rdbuf();
        file.close();
        json >> config;

        instance = config[profile]["instance"].asString();
        access_token = config[profile]["access_token"].asString();
        feedurl = config[profile]["feedurl"].asString();
        if (!config[profile]["max_size"].isNull())
        {
            max_size = config[profile]["max_size"].asUInt();
        }
    }
    else
    {
        cout << "Config file not found. Building new one.\n";
        fs::create_directory(filepath);
    }

    if (instance.empty())
    {
        cout << "Instance: ";
        cin >> instance;
        config[profile]["instance"] = instance;
        config_changed = true;
    }
    if (access_token.empty())
    {
        cout << "No access token found.\n";
        string client_id, client_secret, url;
        Mastodon::API masto(instance, "");
        std::uint16_t ret = masto.register_app1("mastorss",
                                                "urn:ietf:wg:oauth:2.0:oob",
                                                "write",
                                                "https://github.com/tastytea/mastorss",
                                                client_id,
                                                client_secret,
                                                url);
        if (ret == 0)
        {
            string code;
            cout << "Visit " << url << " to authorize this application.\n";
            cout << "Insert code: ";
            cin >> code;

            ret = masto.register_app2(client_id,
                                      client_secret,
                                      "urn:ietf:wg:oauth:2.0:oob",
                                      code,
                                      access_token);
            if (ret == 0)
            {
                config[profile]["access_token"] = access_token;
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
        config[profile]["feedurl"] = feedurl;
        config_changed = true;
    }
    if (config[profile]["titles_only"].isNull())
    {
        string titles_only;
        cout << "post only titles? [y/n]: ";
        cin >> titles_only;
        if (titles_only[0] == 'y')
        {
            config[profile]["titles_only"] = true;
        }
        else
        {
            config[profile]["titles_only"] = false;
        }
        config_changed = true;
    }
    if (config_changed)
    {
        write_config();
    }

    return 0;
}


const bool write_config()
{
    std::ofstream outfile(filepath + "config-" + profile + ".json");
    if (outfile.is_open())
    {
        outfile.write(config.toStyledString().c_str(),
                      config.toStyledString().length());
        outfile.close();

        return true;
    }

    return false;
}
