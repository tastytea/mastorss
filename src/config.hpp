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

#ifndef MASTORSS_CONFIG_HPP
#define MASTORSS_CONFIG_HPP

#include <boost/filesystem.hpp>
#include <jsoncpp/json/json.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mastorss
{
namespace fs = boost::filesystem;
using std::uint32_t;
using std::string;
using std::string_view;
using std::vector;

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
    vector<string> fixes;
    string instance;
    uint32_t interval{30};
    string last_guid;
    size_t max_size{500};
    vector<string> skip;
    bool titles_as_cw{false};
    bool titles_only{false};

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
    explicit Config(string profile);

    ProfileData data;

    void write();

private:
    const string _profile;
    Json::Value _json;

    [[nodiscard]]
    fs::path get_filename() const;
    void generate();
    [[nodiscard]]
    string get_access_token(const string &instance) const;
    void parse();
};
} // namespace mastorss

#endif  // MASTORSS_CONFIG_HPP
