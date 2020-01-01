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

#ifndef MASTORSS_MASTOAPI_HPP
#define MASTORSS_MASTOAPI_HPP

#include "config.hpp"
#include "document.hpp"

#include <mastodon-cpp/mastodon-cpp.hpp>

namespace mastorss
{
class MastoAPI
{
public:
    explicit MastoAPI(ProfileData &data);

    void post_item(const Item &item);

private:
    ProfileData &_profile;
    Mastodon::API _masto;
    constexpr static size_t _max_guids{100};
};
} // namespace mastorss

#endif  // MASTORSS_MASTOAPI_HPP
