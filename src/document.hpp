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

#ifndef MASTORSS_DOCUMENT_HPP
#define MASTORSS_DOCUMENT_HPP

#include "config.hpp"

#include <boost/property_tree/ptree.hpp>
#include <restclient-cpp/restclient.h>

#include <string>
#include <vector>

namespace mastorss
{
namespace pt = boost::property_tree;
using std::string;
using std::vector;

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
    ~Document();
    Document(const Document &other) = default;
    Document &operator=(const Document &other) = delete;
    Document(Document &&other) = default;
    Document &operator=(Document &&other) = delete;

    vector<Item> new_items;

    void download();
    void download(const string &uri);
    void parse();

private:
    Config &_cfg;
    ProfileData &_data;
    string _raw_doc;

    void parse_rss(const pt::ptree &tree);
    [[nodiscard]]
    string remove_html(string html) const;
    [[nodiscard]]
    string extract_location(const RestClient::HeaderFields &headers) const;
};
} // namespace mastorss

#endif  // MASTORSS_DOCUMENT_HPP
