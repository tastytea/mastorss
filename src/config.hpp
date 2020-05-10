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

#ifndef MASTORSS_CONFIG_HPP
#define MASTORSS_CONFIG_HPP

#include <boost/filesystem.hpp>
#include <json/json.h>

#include <cstdint>
#include <list>
#include <string>
#include <string_view>
#include <utility>

namespace mastorss
{
namespace fs = boost::filesystem;
using std::uint32_t;
using std::list;
using std::string;
using std::string_view;
using std::pair;

/*!
 *  @brief  The configuration for a profile as data structure.
 *
 *  @since  0.10.0
 */
struct ProfileData
{
    string access_token;
    string append;
    string feedurl;
    list<string> fixes;
    list<string> guids;
    string instance;
    bool keep_looking{false};
    uint32_t interval{30};
    size_t max_size{500};
    list<string> skip;
    bool titles_as_cw{false};
    bool titles_only{false};
    list<pair<string, string>> replacements;

    friend std::ostream &operator <<(std::ostream &out,
                                     const ProfileData &data);
};

/*!
 *  @brief  A configuration file.
 *
 *  @since  0.10.0
 */
class Config
{
public:
    explicit Config(string profile_name);

    const string profile;
    ProfileData profiledata;

    void write();
    [[nodiscard]]
    fs::path get_config_dir() const;

private:
    Json::Value _json;

    [[nodiscard]]
    fs::path get_filename() const;
    void generate();
    [[nodiscard]]
    string get_access_token(const string &instance) const;
    void parse();
    list<string> jsonarray_to_stringlist(const Json::Value &jsonarray) const;
    Json::Value stringlist_to_jsonarray(const list<string> &stringlist) const;
};
} // namespace mastorss

#endif  // MASTORSS_CONFIG_HPP
