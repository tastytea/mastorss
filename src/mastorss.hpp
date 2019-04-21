/*  This file is part of mastorss.
 *  Copyright © 2018, 2019 tastytea <tastytea@tastytea.de>
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

#ifndef mastorss_HPP
#define mastorss_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <jsoncpp/json/json.h>
#include <mastodon-cpp/easy/easy.hpp>

using std::string;

extern std::uint16_t max_size;
extern const string filepath;
extern Json::Value config;
extern std::string profile;

std::uint16_t read_config(string &instance, string &access_token, string &feedurl);
bool write_config();

std::vector<Mastodon::Easy::Status> parse_feed(const string &xml);
void individual_fixes(string &str);

std::uint16_t http_get(const string &feedurl,
                             string &answer, const string &useragent = "");
void curlpp_init();

#endif // mastorss_HPP
