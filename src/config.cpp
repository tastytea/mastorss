/*  This file is part of mastorss.
 *  Copyright © 2019 tastytea <tastytea@tastytea.de>
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

#include "config.hpp"
#include "exceptions.hpp"

#include <boost/log/trivial.hpp>
#include <mastodon-cpp/mastodon-cpp.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mastorss
{
using std::stoul;
using std::getenv;
using std::ifstream;
using std::ofstream;
using std::cin;
using std::cout;
using std::stringstream;
using std::runtime_error;
using std::getline;
using std::move;

std::ostream &operator <<(std::ostream &out, const ProfileData &data)
{
    out << "access_token: \"" << data.access_token << "\", "
        << "append: \"" << data.append << "\", "
        << "feedurl: \"" << data.feedurl << "\", "
        << "fixes: [";
    for (const auto &fix : data.fixes)
    {
        out << '"' << fix << '"';
        if (fix != *data.fixes.rbegin())
        {
            out << ", ";
        }
    }
    out << "], "
        << "instance: \"" << data.instance << "\", "
        << "interval: " << data.interval << ", "
        << "last_guid: \"" << data.last_guid << "\", "
        << "max_size: " << data.max_size << ", "
        << "skip: [";
    for (const auto &skip : data.skip)
    {
        out << '"' << skip << '"';
        if (skip != *data.skip.rbegin())
        {
            out << ", ";
        }
    }
    out << "], "
        << "titles_as_cw: " << data.titles_as_cw << ", "
        << "titles_only: " << data.titles_only;

    return out;
}

Config::Config(string profile)
    :_profile{move(profile)}
{
    const fs::path filename = get_filename();
    BOOST_LOG_TRIVIAL(debug) << "Config filename is: " << filename;

    ifstream file{filename.c_str()};
    if (file.good())
    {
        stringstream rawjson;
        rawjson << file.rdbuf();
        rawjson >> _json;
        parse();
    }
    else
    {
        generate();
    }
}

fs::path Config::get_filename() const
{
    char *envdir = getenv("XDG_CONFIG_HOME");
    fs::path dir;

    if (envdir != nullptr)
    {
        dir = envdir;
    }
    else
    {
        envdir = getenv("HOME");
        if (envdir != nullptr)
        {
            dir = fs::path{envdir} /= ".config";
        }
        else
        {
            throw FileException{"Couldn't find configuration directory."};
        }
    }

    dir /= "mastorss";
    if (fs::create_directories(dir))
    {
        BOOST_LOG_TRIVIAL(debug) << "Created config dir: " << dir;
    }

    return dir /= "config-" + _profile + ".json";
}

void Config::generate()
{
    string line;

    cout << "Instance (domain): ";
    getline(cin, line);
    data.instance = line;

    data.access_token = get_access_token(line);

    cout << "URL of the feed: ";
    std::getline(cin, line);
    data.feedurl = line;

    cout << "Post only titles? [y/n]: ";
    std::getline(cin, line);
    if (line[0] == 'y')
    {
        data.titles_only = true;
    }
    else
    {
        data.titles_only = false;
    }

    cout << "Post titles as cw? [y/n]: ";
    std::getline(cin, line);
    if (line[0] == 'y')
    {
        data.titles_as_cw = true;
    }
    else
    {
        data.titles_as_cw = false;
    }

    cout << "Append this string to each post: ";
    std::getline(cin, line);
    data.append = line;

    cout << "Interval between posts in seconds [30]: ";
    std::getline(cin, line);
    if (line.empty())
    {
        line = "30";
    }
    data.interval = static_cast<uint32_t>(stoul(line));

    cout << "Maximum size of posts [500]: ";
    std::getline(cin, line);
    if (line.empty())
    {
        line = "500";
    }
    data.max_size = static_cast<uint32_t>(stoul(line));

    BOOST_LOG_TRIVIAL(debug) << "Generated configuration.";
    write();
}

string Config::get_access_token(const string &instance) const
{
    string client_id;
    string client_secret;
    string url;
    Mastodon::API masto(instance, "");

    Mastodon::return_call ret
        {
            masto.register_app1("mastorss", "urn:ietf:wg:oauth:2.0:oob",
                                "write",
                                "https://schlomp.space/tastytea/mastorss",
                                client_id, client_secret, url)
        };

    if (ret)
    {
        string code;
        string access_token;

        cout << "Visit " << url << " to authorize this application.\n";
        cout << "Insert code: ";
        std::getline(cin, code);
        ret = masto.register_app2(client_id, client_secret,
                                  "urn:ietf:wg:oauth:2.0:oob",
                                  code, access_token);
        if (ret)
        {
            BOOST_LOG_TRIVIAL(debug) << "Got access token: " << access_token;
            return access_token;
        }
    }

    throw MastodonException{ret.error_code};
}

void Config::parse()
{
    data.access_token = _json[_profile]["access_token"].asString();
    data.append = _json[_profile]["append"].asString();
    data.feedurl = _json[_profile]["feedurl"].asString();
    for (const auto &fix : _json[_profile]["fixes"])
    {
        data.fixes.push_back(fix.asString());
    }
    data.instance = _json[_profile]["instance"].asString();
    if (!_json[_profile]["interval"].isNull())
    {
        data.interval =
            static_cast<uint32_t>(_json[_profile]["interval"].asUInt64());
    }
    data.last_guid = _json[_profile]["last_guid"].asString();
    if (!_json[_profile]["max_size"].isNull())
    {
        data.max_size =
            static_cast<uint32_t>(_json[_profile]["max_size"].asUInt64());
    }
    for (const auto &skip : _json[_profile]["skip"])
    {
        data.skip.push_back(skip.asString());
    }
    data.titles_as_cw = _json[_profile]["titles_as_cw"].asBool();
    data.titles_only = _json[_profile]["titles_only"].asBool();

    BOOST_LOG_TRIVIAL(debug) << "Read config: " << data;
}

void Config::write()
{
    _json[_profile]["access_token"] = data.access_token;
    _json[_profile]["append"] = data.append;
    _json[_profile]["feedurl"] = data.feedurl;
    // Leave fixes.
    _json[_profile]["instance"] = data.instance;
    _json[_profile]["interval"] = data.interval;
    _json[_profile]["last_guid"] = data.last_guid;
    _json[_profile]["max_size"] = data.max_size;
    // Leave skip.
    _json[_profile]["titles_as_cw"] = data.titles_as_cw;
    _json[_profile]["titles_as_cw"] = data.titles_only;

    ofstream file{get_filename().c_str()};
    if (file.good())
    {
        file << _json.toStyledString();
    }

    BOOST_LOG_TRIVIAL(debug) << "Wrote config file.";
}
} // namespace mastorss
