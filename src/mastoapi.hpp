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

#ifndef MASTORSS_MASTOAPI_HPP
#define MASTORSS_MASTOAPI_HPP

#include "config.hpp"
#include "document.hpp"

#include <mastodonpp/mastodonpp.hpp>

#include <string>

namespace mastorss
{
using std::string;

class MastoAPI
{
public:
    explicit MastoAPI(ProfileData &data);

    void post_item(const Item &item);

private:
    ProfileData &_profile;
    mastodonpp::Instance _instance;
    constexpr static size_t _max_guids{100};

    string replacements_apply(const string &text);
};
} // namespace mastorss

#endif  // MASTORSS_MASTOAPI_HPP
