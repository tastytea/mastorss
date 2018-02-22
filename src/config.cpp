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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <mastodon-cpp.hpp>
#include "mastorss.hpp"

namespace pt = boost::property_tree;

using std::cout;
using std::cerr;
using std::cin;
using std::string;

std::uint16_t read_config(pt::ptree &config, const string &profile, string &instance, string &access_token, string &feedurl)
{
    bool config_changed = false;

    // Read config file, get access token
    try {
        pt::read_json(filepath + "config-" + profile + ".json", config);
        instance = config.get(profile + ".instance", "");
        access_token = config.get(profile + ".access_token", "");
        feedurl = config.get(profile + ".feedurl", "");
        max_size = config.get(profile + ".max_size", max_size);
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
