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

#include "document.hpp"

#include "curl_wrapper.hpp"
#include "exceptions.hpp"
#include "version.hpp"

#include <boost/log/trivial.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/regex.hpp>
#include <json/json.h>
#include <mastodonpp/mastodonpp.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace mastorss
{
using boost::regex;
using boost::regex_replace;
using std::any_of;
using std::ifstream;
using std::istringstream;
using std::move;
using std::string;
using std::stringstream;
using std::transform;

bool operator!=(const Item &a, const Item &b)
{
    return a.guid != b.guid;
}

Document::Document(Config &cfg)
    : _cfg{cfg}
    , _profiledata{_cfg.profiledata}
{
    download();
}

void Document::download(const string &uri, const bool temp_redirect)
{
    namespace cw = curl_wrapper;

    BOOST_LOG_TRIVIAL(debug) << "Downloading <" << uri << "> …";
    cw::CURLWrapper curl;
    curl.set_useragent(string("mastorss/") += version);
    curl.set_maxredirs(0);

    const auto answer{curl.make_http_request(cw::http_method::GET, uri)};

    BOOST_LOG_TRIVIAL(debug) << "Got response: " << answer.status;
    BOOST_LOG_TRIVIAL(debug) << "Got Headers:";
    BOOST_LOG_TRIVIAL(debug) << answer.headers;

    switch (answer.status)
    {
    case 200:
    {
        _raw_doc = answer.body;
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
        _profiledata.feedurl = extract_location(answer);
        if (_profiledata.feedurl.empty())
        {
            throw HTTPException{answer.status};
        }

        // clang-format off
        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (permanent): "
                                 << _profiledata.feedurl;
        // clang-format on
        _cfg.write();
        download();
        break;
    }
    case 302:
    case 303:
    case 307:
    {
temporary_redirect:
        const string newuri{extract_location(answer)};
        if (newuri.empty())
        {
            throw HTTPException{answer.status};
        }

        // clang-format off
        BOOST_LOG_TRIVIAL(debug) << "Feed has new location (temporary): "
                                 << newuri;
        // clang-format on
        download(newuri, true);
        break;
    }
    default:
    {
        throw HTTPException{answer.status};
    }
    }
}

void Document::download()
{
    download(_profiledata.feedurl);
}

void Document::parse()
{
    if (_profiledata.add_hashtags)
    {
        parse_watchwords();
    }
    pt::ptree tree;
    istringstream iss{_raw_doc};
    pt::read_xml(iss, tree);

    if (tree.front().first == "rss")
    {
        BOOST_LOG_TRIVIAL(debug) << "RSS detected.";
        parse_rss(tree);
    }
    else
    {
        throw ParseException{"Could not detect type of feed."};
    }
}

void Document::parse_rss(const pt::ptree &tree)
{
    size_t counter{0};
    for (const auto &child : tree.get_child("rss.channel"))
    {
        if (counter == Config::max_guids)
        {
            BOOST_LOG_TRIVIAL(debug)
                << "Maximum number of items reached. Stopped parsing.";
            break;
        }
        ++counter;
        if (child.first == "item")
        {
            const auto &rssitem = child.second;

            string guid{rssitem.get<string>("guid", "")};
            if (guid.empty()) // We hope either <guid> or <link> are present.
            {
                guid = rssitem.get<string>("link");
            }
            if (any_of(_profiledata.guids.begin(), _profiledata.guids.end(),
                       [&](const auto &old_guid) { return guid == old_guid; }))
            {
                // clang-format off
                BOOST_LOG_TRIVIAL(debug) << "Found already posted GUID: "
                                         << guid;
                // clang-format on
                if (_profiledata.keep_looking)
                {
                    continue;
                }

                BOOST_LOG_TRIVIAL(debug) << "Stopped parsing.";
                break;
            }

            string title{rssitem.get<string>("title")};
            if (any_of(_profiledata.skip.begin(), _profiledata.skip.end(),
                       [&title](const string &skip)
                       // clang-format off
                       { return title.substr(0, skip.size()) == skip; }))
            // clang-format on
            {
                BOOST_LOG_TRIVIAL(debug) << "Skipped GUID: " << guid;
                continue;
            }

            Item item;
            item.description = [&]
            // clang-format off
            {
                string desc{rssitem.get<string>("description")};
                for (const auto &fix : _profiledata.fixes)
                {
                    desc = regex_replace(desc, regex{fix}, "");
                }
                desc = remove_html(desc);
                if (_profiledata.add_hashtags)
                {
                    desc = add_hashtags(desc);
                }
                return desc;
            }();
            // clang-format on
            item.guid = move(guid);
            item.link = rssitem.get<string>("link");
            item.title = mastodonpp::unescape_html(title);
            new_items.push_front(item);

            BOOST_LOG_TRIVIAL(debug) << "Found GUID: " << item.guid;

            if (_profiledata.guids.empty() && !_profiledata.keep_looking)
            {
                BOOST_LOG_TRIVIAL(debug) << "This is the first run.";
                break;
            }
        }
    }
}

string Document::remove_html(string html) const
{
    html = mastodonpp::unescape_html(html); // Decode HTML entities.

    html = regex_replace(html, regex{"<p>"}, "\n\n");

    const list re_list{regex{R"(<!\[CDATA\[)"},      // CDATA beginning.
                       regex{R"(\]\]>)"},            // CDATA end.
                       regex{"<[^>]+>"},             // HTML tags.
                       regex{R"(\r)"},               // Carriage return.
                       regex{"\\n[ \\t\u00a0]+\\n"}, // Space between newlines.
                       regex{R"(^\n+)"}};            // Newlines at beginning.
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

string Document::extract_location(const curl_wrapper::answer &answer)
{
    string location{answer.get_header("Location")};

    if (location.empty())
    {
        throw std::runtime_error{"Could not extract new feed location."};
    }

    return location;
}

string Document::add_hashtags(const string &text)
{
    string out{text};
    for (const auto &tag : _watchwords)
    {
        regex re_tag("([[:space:]\u200b]|^)(" + tag
                         + ")([[:space:]\u200b[:punct:]]|$)",
                     regex::icase);
        out = regex_replace(out, re_tag, "$1#$2$3", boost::format_first_only);
    }

    return out;
}

void Document::parse_watchwords()
{
    Json::Value json;
    const auto filepath = _cfg.get_config_dir() /= "watchwords.json";
    ifstream file(filepath.c_str());
    if (file.good())
    {
        stringstream rawjson;
        rawjson << file.rdbuf();
        rawjson >> json;
        BOOST_LOG_TRIVIAL(debug) << "Read " << filepath;
    }
    else
    {
        BOOST_LOG_TRIVIAL(warning)
            << "File Not found: "
            << (_cfg.get_config_dir() /= "watchwords.json").string();
        return;
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
