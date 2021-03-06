/*  This file is part of mastorss.
 *  Copyright © 2019, 2020 tastytea <tastytea@tastytea.de>
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
#include <mastodonpp/mastodonpp.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mastorss
{
using std::back_inserter;
using std::cin;
using std::cout;
using std::getenv;
using std::getline;
using std::ifstream;
using std::move;
using std::ofstream;
using std::stoul;
using std::stringstream;
using std::transform;

std::ostream &operator<<(std::ostream &out, const ProfileData &data)
{
    out << "append: \"" << data.append << "\", "
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
        << "guids: [";
    for (const auto &guid : data.guids)
    {
        out << '"' << guid << '"';
        if (guid != *data.guids.rbegin())
        {
            out << ", ";
        }
    }
    out << "instance: \"" << data.instance << "\", "
        << "interval: " << data.interval << ", "
        << "keep_looking: " << data.keep_looking << ", "
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
        << "titles_only: " << data.titles_only << ", ";
    out << "replacements: [";
    for (const auto &replacement : data.replacements)
    {
        out << '"' << replacement.first << "\": \"" << replacement.second
            << '"';
        if (replacement != *data.replacements.rbegin())
        {
            out << ", ";
        }
    }
    out << "], ";
    out << "add_hashtags: " << data.add_hashtags;

    return out;
}

Config::Config(string profile_name)
    : profile{move(profile_name)}
{
    const fs::path filename = get_filename();
    BOOST_LOG_TRIVIAL(debug) << "Config filename is: " << filename;

    ifstream file(filename.c_str());
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

fs::path Config::get_config_dir() const
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

    return dir;
}

fs::path Config::get_filename() const
{
    return get_config_dir() /= "config-" + profile + ".json";
}

void Config::generate()
{
    string line;

    cout << "Instance (domain): ";
    getline(cin, line);
    profiledata.instance = line;

    profiledata.access_token = get_access_token(line);

    cout << "URL of the feed: ";
    std::getline(cin, line);
    profiledata.feedurl = line;

    cout << "Post only titles? [y/n]: ";
    std::getline(cin, line);
    profiledata.titles_only = (line[0] == 'y');

    cout << "Post titles as cw? [y/n]: ";
    std::getline(cin, line);
    profiledata.titles_as_cw = (line[0] == 'y');

    cout << "Append this string to each post: ";
    std::getline(cin, line);
    profiledata.append = line;

    cout << "Interval between posts in seconds [30]: ";
    std::getline(cin, line);
    if (line.empty())
    {
        line = "30";
    }
    profiledata.interval = static_cast<uint32_t>(stoul(line));

    cout << "Maximum size of posts [500]: ";
    std::getline(cin, line);
    if (line.empty())
    {
        line = "500";
    }
    profiledata.max_size = stoul(line);

    cout << "Add hashtags according to watchwords.json? [y/n]: ";
    std::getline(cin, line);
    profiledata.add_hashtags = (line[0] == 'y');

    BOOST_LOG_TRIVIAL(debug) << "Generated configuration.";
    write();
}

string Config::get_access_token(const string &instance) const
{
    string client_id;
    string client_secret;
    string url;
    mastodonpp::Instance masto{instance, ""};
    mastodonpp::Instance::ObtainToken token{masto};

    auto ret{token.step_1("mastorss", "write:statuses",
                          "https://schlomp.space/tastytea/mastorss")};

    if (ret)
    {
        string code;

        cout << "Visit " << ret << " to authorize this application.\n";
        cout << "Insert code: ";
        std::getline(cin, code);
        ret = token.step_2(code);
        if (ret)
        {
            BOOST_LOG_TRIVIAL(debug) << "Got access token: " << ret.body;
            return ret.body;
        }
    }

    throw CURLException{ret.curl_error_code};
}

void Config::parse()
{
    profiledata.access_token = _json[profile]["access_token"].asString();
    profiledata.append = _json[profile]["append"].asString();
    profiledata.feedurl = _json[profile]["feedurl"].asString();
    profiledata.fixes = jsonarray_to_stringlist(_json[profile]["fixes"]);
    profiledata.guids = jsonarray_to_stringlist(_json[profile]["guids"]);
    profiledata.instance = _json[profile]["instance"].asString();
    if (!_json[profile]["interval"].isNull())
    {
        profiledata.interval = static_cast<uint32_t>(
            _json[profile]["interval"].asUInt64());
    }
    profiledata.keep_looking = _json[profile]["keep_looking"].asBool();
    if (!_json[profile]["max_size"].isNull())
    {
        profiledata.max_size = _json[profile]["max_size"].asUInt64();
    }
    profiledata.skip = jsonarray_to_stringlist(_json[profile]["skip"]);
    profiledata.titles_as_cw = _json[profile]["titles_as_cw"].asBool();
    profiledata.titles_only = _json[profile]["titles_only"].asBool();
    for (const auto &search : _json[profile]["replacements"].getMemberNames())
    {
        profiledata.replacements.push_back(
            {search, _json[profile]["replacements"][search].asString()});
    }
    profiledata.add_hashtags = _json[profile]["add_hashtags"].asBool();

    BOOST_LOG_TRIVIAL(debug) << "Read config: " << profiledata;
}

void Config::write()
{
    _json[profile]["access_token"] = profiledata.access_token;
    _json[profile]["append"] = profiledata.append;
    _json[profile]["feedurl"] = profiledata.feedurl;
    _json[profile]["guids"] = stringlist_to_jsonarray(profiledata.guids);
    _json[profile]["fixes"] = stringlist_to_jsonarray(profiledata.fixes);
    _json[profile]["instance"] = profiledata.instance;
    _json[profile]["interval"] = profiledata.interval;
    _json[profile]["keep_looking"] = profiledata.keep_looking;
    _json[profile]["max_size"] = static_cast<Json::Value::UInt64>(
        profiledata.max_size);
    _json[profile]["skip"] = stringlist_to_jsonarray(profiledata.skip);
    _json[profile]["titles_as_cw"] = profiledata.titles_as_cw;
    _json[profile]["titles_only"] = profiledata.titles_only;
    for (const auto &replacement : profiledata.replacements)
    {
        _json[profile]["replacements"][replacement.first] = replacement.second;
    }
    _json[profile]["add_hashtags"] = profiledata.add_hashtags;

    ofstream file(get_filename().c_str());
    if (file.good())
    {
        file << _json.toStyledString();
    }

    BOOST_LOG_TRIVIAL(debug) << "Wrote config file.";
}

list<string> Config::jsonarray_to_stringlist(const Json::Value &jsonarray) const
{
    list<string> stringlist;
    std::transform(jsonarray.begin(), jsonarray.end(),
                   back_inserter(stringlist),
                   [](const Json::Value &value) { return value.asString(); });

    return stringlist;
}

Json::Value
Config::stringlist_to_jsonarray(const list<string> &stringlist) const
{
    Json::Value jsonarray;

    for (const auto &entry : stringlist)
    {
        jsonarray.append(entry);
    }

    return jsonarray;
}
} // namespace mastorss
