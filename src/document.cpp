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
#include <json/json.h>
#include <mastodon-cpp/mastodon-cpp.hpp>
#include <restclient-cpp/connection.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace mastorss
{
using boost::regex;
using boost::regex_replace;
using std::any_of;
using std::transform;
using std::ifstream;
using std::istringstream;
using std::stringstream;
using std::string;
using std::move;

bool operator !=(const Item &a, const Item &b)
{
    return a.guid != b.guid;
}

Document::Document(Config &cfg)
    : _cfg{cfg}
    , _profiledata{_cfg.profiledata}
{
    RestClient::init();

    download();
}

Document::~Document()
{
    RestClient::disable();
}

void Document::download(const string &uri, const bool temp_redirect)
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
        BOOST_LOG_TRIVIAL(debug) << "Downloaded feed: " << _profiledata.feedurl;
        break;
    }
    case 301:
    case 308:
    {
        if (temp_redirect)
        {
            goto temporary_redirect; // NOLINT(cppcoreguidelines-avoid-goto)
        }
        _profiledata.feedurl = extract_location(response.headers);
        if (_profiledata.feedurl.empty())
        {
            throw HTTPException{response.code};
        }

        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (permanent): "
                                 << _profiledata.feedurl;
        _cfg.write();
        download();
        break;
    }
    case 302:
    case 303:
    case 307:
    {
    temporary_redirect:
        const string newuri{extract_location(response.headers)};
        if (newuri.empty())
        {
            throw HTTPException{response.code};
        }

        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (temporary): "
                                 << newuri;
        download(newuri, true);
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
    download(_profiledata.feedurl);
}

void Document::parse()
{
    parse_watchwords();
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

            string guid{rssitem.get<string>("guid", "")};
            if (guid.empty())   // We hope either <guid> or <link> are present.
            {
                guid = rssitem.get<string>("link");
            }
            if (guid == _profiledata.last_guid)
            {
                BOOST_LOG_TRIVIAL(debug)
                    << "Found already posted GUID, stopped parsing.";
                break;
            }

            string title{rssitem.get<string>("title")};
            if (any_of(_profiledata.skip.begin(), _profiledata.skip.end(),
                       [&title](const string &skip)
                       { return title.substr(0, skip.size()) == skip; }))
            {
                BOOST_LOG_TRIVIAL(debug) << "Skipped GUID: " << guid;
                continue;
            }

            Item item;
            item.description = [&]
            {
                string desc{rssitem.get<string>("description")};
                for (const auto &fix : _profiledata.fixes)
                {
                    desc = regex_replace(desc, regex{fix}, "");
                }
                desc = remove_html(desc);
                return add_hashtags(desc);
            }();
            item.guid = move(guid);
            item.link = rssitem.get<string>("link");
            item.title = move(title);
            new_items.push_front(item);

            BOOST_LOG_TRIVIAL(debug) << "Found GUID: " << item.guid;

            if (_profiledata.last_guid.empty())
            {
                BOOST_LOG_TRIVIAL(debug) << "This is the first run.";
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

string Document::add_hashtags(const string &text)
{
    string out{text};
    for (const auto &tag : _watchwords)
    {
        regex re_tag("([[:space:][:punct:]]|^)("
            + tag + ")([[:space:][:punct:]]|$)", regex::icase);
        out = regex_replace(out, re_tag, "$1#$2$3", boost::format_first_only);
    }

    return out;
}

void Document::parse_watchwords()
{
    Json::Value json;
    const auto filepath = _cfg.get_config_dir() /= "watchwords.json";
    ifstream file{filepath};
    if (file.good())
    {
        stringstream rawjson;
        rawjson << file.rdbuf();
        rawjson >> json;
        BOOST_LOG_TRIVIAL(debug) << "Read " << filepath;
    }
    else
    {
        throw FileException{"File Not found:"
                + (_cfg.get_config_dir() /= "watchwords.json").string()};
    }

    const auto &tags_profile = json[_cfg.profile]["tags"];
    const auto &tags_global = json["global"]["tags"];
    transform(tags_profile.begin(), tags_profile.end(),
              std::back_inserter(_watchwords),
              [](const Json::Value &value) { return value.asString(); });
    transform(tags_global.begin(), tags_global.end(),
              std::back_inserter(_watchwords),
              [](const Json::Value &value) { return value.asString(); });
}


} // namespace mastorss
