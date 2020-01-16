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

MastoAPI::MastoAPI(ProfileData &data)
    : _profile{data}
    , _instance{_profile.instance, _profile.access_token}
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

    const size_t len_append{[&]
    {
        if (_profile.append.empty())
        {
            return size_t{0};
        }
        return _profile.append.size() + 2;
    }()};
    const size_t len_status{status.size()};
    const size_t len_max{[&]
    {
        if (_profile.titles_as_cw)
        {
            // Subjects (CWs) count into the post length.
            return _profile.max_size - item.title.size();
        }
        return _profile.max_size;
    }() - item.link.size() - 2 - len_append};
    BOOST_LOG_TRIVIAL(debug)
        << "Maximum text (without link and appendix) length: " << len_max;

    if (len_status > len_max)
    {
        constexpr string_view omission = " […]";
        status.resize(len_max - omission.size());

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

    mastodonpp::parametermap params{{"status", status}};
    if (_profile.titles_as_cw)
    {
        params.insert({"spoiler_text", item.title});
    }

    mastodonpp::Connection connection{_instance};
    const auto ret = connection.post(mastodonpp::API::v1::statuses, params);
    if (!ret)
    {
        if (ret.http_status != 200)
        {
            throw HTTPException{ret.http_status};
        }
        throw CURLException{ret.curl_error_code};
    }
    BOOST_LOG_TRIVIAL(debug) << "Posted status with GUID: " << item.guid;

    _profile.guids.push_back(item.guid);
    if (_profile.guids.size() > _max_guids)
    {
        _profile.guids.pop_front();
    }
}
} // namespace mastorss
