/*  This file is part of mastorss.
 *  Copyright © 2019-2021 tastytea <tastytea@tastytea.de>
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

#ifndef MASTORSS_DOCUMENT_HPP
#define MASTORSS_DOCUMENT_HPP

#include "config.hpp"
#include "curl_wrapper.hpp"

#include <boost/property_tree/ptree.hpp>

#include <list>
#include <string>

namespace mastorss
{
namespace pt = boost::property_tree;
using std::list;
using std::string;

/*!
 *  @brief  An Item of a feed.
 *
 *  @since  0.10.0
 */
struct Item
{
    string description;
    string guid;
    string link;
    string title;

    friend bool operator!=(const Item &a, const Item &b);
};

/*!
 *  @brief  A feed.
 *
 *  @since  0.10.0
 */
class Document
{
public:
    explicit Document(Config &cfg);
    Document(const Document &other) = default;
    Document &operator=(const Document &other) = delete;
    Document(Document &&other) = default;
    Document &operator=(Document &&other) = delete;

    list<Item> new_items;

    void parse();

private:
    Config &_cfg;
    ProfileData &_profiledata;
    string _raw_doc;
    list<string> _watchwords;

    void download();
    /*!
     *  @brief  Download document.
     *
     *  @param  uri           The URI to download.
     *  @param  temp_redirect `true` if this is an temporary redirect.
     *
     *  The argument `temp_redirect` is there for cases where the original URI
     *  is redirected temporarily, and the new URI is redirected permanently.
     *
     *  @since  0.10.0
     */
    void download(const string &uri, bool temp_redirect = false);
    void parse_rss(const pt::ptree &tree);
    [[nodiscard]] static string remove_html(string html);
    [[nodiscard]] static string
    extract_location(const curl_wrapper::answer &answer);
    string add_hashtags(const string &text);
    void parse_watchwords();
};
} // namespace mastorss

#endif // MASTORSS_DOCUMENT_HPP
