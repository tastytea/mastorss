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

    const size_t len_status{status.size() + item.link.size() + 2};
    const size_t len_append{[&]
    {
        if (_profile.append.empty())
        {
            return size_t{0};
        }
        return _profile.append.size() + 2;
    }()};
    const size_t len_max{[&]
    {
        if (_profile.titles_as_cw)
        {
            // Subjects (CWs) count into the post length.
            return _profile.max_size - item.title.size();
        }
        return _profile.max_size;
    }()};
    BOOST_LOG_TRIVIAL(debug) << "Maximum status length: " << len_max;;

    if ((len_status + len_append) > len_max)
    {
        constexpr string_view omission = " […]";
        status.resize(len_max - len_append - omission.size());

        // Don't cut in the middle of a word.
        const auto pos = status.rfind(' ');
        if (pos != string::npos)
        {
            status.resize(pos);
        }

        status.append(omission);
        BOOST_LOG_TRIVIAL(debug) << "Status resized to: " << status.size();
    }

    status.append("\n\n" + item.link);

    if (!_profile.append.empty())
    {
        status.append("\n\n" + _profile.append);
    }
    BOOST_LOG_TRIVIAL(debug) << "Status length: " << status.size();
    BOOST_LOG_TRIVIAL(debug) << "Status: \"" << status << '"';

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
