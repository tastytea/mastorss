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

#include "document.hpp"
#include "exceptions.hpp"
#include "version.hpp"

#include <boost/log/trivial.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/regex.hpp>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include <restclient-cpp/connection.h>

#include <list>
#include <sstream>
#include <string>
#include <utility>

namespace mastorss
{
using boost::regex;
using boost::regex_replace;
using std::list;
using std::istringstream;
using std::string;
using std::move;

Document::Document(Config &cfg)
    : _cfg{cfg}
    , _data{cfg.data}
{
    RestClient::init();

    download();
}

Document::~Document()
{
    RestClient::disable();
}

void Document::download(const string &uri)
{
    RestClient::Connection connection{uri};
    connection.SetUserAgent(string("mastorss/").append(version));
    connection.FollowRedirects(false);

    RestClient::Response response{connection.get("")};

    switch (response.code)
    {
    case 200:
    {
        _raw_doc = response.body;
        BOOST_LOG_TRIVIAL(debug) << "Downloaded feed: " << _data.feedurl;
        break;
    }
    case 301:
    case 308:
    {
        _data.feedurl = extract_location(response.headers);
        if (_data.feedurl.empty())
        {
            throw HTTPException{response.code};
        }

        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (permanent): "
                                 << _data.feedurl;
        _cfg.write();
        download();
        break;
    }
    case 302:
    case 303:
    case 307:
    {
        const string newuri{extract_location(response.headers)};
        if (newuri.empty())
        {
            throw HTTPException{response.code};
        }

        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (temporary): "
                                 << _data.feedurl;
        download(newuri);
        break;
    }
    case -1:
    {
        throw CURLException{errno};
    }
    default:
    {
        throw HTTPException{response.code};
    }
    }
}

void Document::download()
{
    download(_data.feedurl);
}

void Document::parse()
{
    pt::ptree tree;
    istringstream iss{_raw_doc};
    pt::read_xml(iss, tree);

    if (tree.front().first == "rss")
    {
        BOOST_LOG_TRIVIAL(debug) << "RSS detected.";
        parse_rss(tree);
    }
}

void Document::parse_rss(const pt::ptree &tree)
{
    for (const auto &child : tree.get_child("rss.channel"))
    {
        if (child.first == "item")
        {
            const auto &rssitem = child.second;

            string guid{rssitem.get<string>("guid")};
            if (guid.empty())   // We hope either <guid> or <link> are present.
            {
                guid = rssitem.get<string>("link");
            }
            if (guid == _data.last_guid)
            {
                break;
            }

            bool skipthis{false};
            string title{rssitem.get<string>("title")};
            for (const auto &skip : _data.skip)
            {
                if (title.substr(0, skip.length()) == skip)
                {
                    skipthis = true;
                    break;
                }
            }
            if (skipthis)
            {
                BOOST_LOG_TRIVIAL(debug) << "Skipped GUID: " << guid;
                continue;
            }

            Item item;
            item.description = [&]
            {
                string desc
                    {remove_html(rssitem.get<string>("description"))};
                for (const auto &fix : _data.fixes)
                {
                    desc = regex_replace(desc, regex{fix}, "");
                }
                return desc;
            }();
            item.guid = move(guid);
            item.link = rssitem.get<string>("link");
            item.title = move(title);
            new_items.push_front(item);

            BOOST_LOG_TRIVIAL(debug) << "Found GUID: " << item.guid;

            if (_data.last_guid.empty())
            {
                break;
            }
        }
    }
}

string Document::remove_html(string html) const
{
    html = Mastodon::unescape_html(html); // Decode HTML entities.

    html = regex_replace(html, regex{"<p>"}, "\n\n");

    const list re_list
        {
            regex{R"(<!\[CDATA\[)"},      // CDATA beginning.
            regex{R"(\]\]>)"},            // CDATA end.
            regex{"<[^>]+>"},             // HTML tags.
            regex{R"(\r)"},               // Carriage return.
            regex{"\\n[ \\t\u00a0]+\\n"}, // Whitespace between newlines.
            regex{R"(^\n+)"}              // Newlines at the beginning.
        };
    for (const regex &re : re_list)
    {
        html = regex_replace(html, re, "");
    }

    // Remove excess newlines.
    html = regex_replace(html, regex{R"(\n{3,})"}, "\n\n");
    // Replace single newlines with spaces (?<= is lookbehind, ?= is lookahead).
    html = regex_replace(html, regex{R"((?<=[^\n])\n(?=[^\n]))"}, " ");

    BOOST_LOG_TRIVIAL(debug) << "Converted HTML to text.";

    return html;
}

string Document::extract_location(const RestClient::HeaderFields &headers) const
{
    string location{headers.at("Location")};
    if (location.empty())
    {
        location = headers.at("location");
    }
    return location;
}
} // namespace mastorss
