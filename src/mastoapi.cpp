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

#include "exceptions.hpp"
#include "mastoapi.hpp"

#include <boost/log/trivial.hpp>

#include <string>
#include <string_view>

namespace mastorss
{
using std::string;
using std::string_view;

MastoAPI::MastoAPI(const ProfileData &data)
    : _profile{data}
    , _masto{_profile.instance, _profile.access_token}
{
}

void MastoAPI::post_item(const Item &item)
{
    string status{[&]
    {
        if (_profile.titles_as_cw)
        {
            if (_profile.titles_only)
            {
                return string{};
            }
            return item.description;
        }

        string s{item.title};
        if (!_profile.titles_only)
        {
            s.append("\n\n" + item.description);
        }
        return s;
    }()};
    status.append("\n\n" + item.link);

    const size_t len{status.size()};
    const size_t len_append{[&]
    {
        if (_profile.append.empty())
        {
            return size_t{};
        }
        return _profile.append.size() + 2;
    }()};
    const size_t len_max{_profile.max_size};
    constexpr string_view omission = " […]";

    if ((len + len_append) > len_max)
    {
        status.resize(len_max - len_append - omission.size());

        // Don't cut in the middle of a word.
        const auto pos = status.rfind(' ');
        if (pos != string::npos)
        {
            status.resize(status.size() - pos);
        }

        status.append(omission);
    }

    if (!_profile.append.empty())
    {
        status.append("\n\n" + _profile.append);
    }

    Mastodon::parameters params{{"status", {status}}};
    if (_profile.titles_as_cw)
    {
        params.push_back({"spoiler_text", {item.title}});
    }

    const auto ret = _masto.post(Mastodon::API::v1::statuses, params);
    if (!ret)
    {
        if (ret.http_error_code != 200)
        {
            throw HTTPException{ret.http_error_code};
        }
        throw MastodonException{ret.error_code};
    }
    BOOST_LOG_TRIVIAL(debug) << "Posted status with GUID: " << item.guid;
}
} // namespace mastorss
