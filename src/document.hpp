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

#include <string>

namespace mastorss
{
using std::string;

class Document
{
public:
    explicit Document(string uri);
    ~Document();
    Document(const Document &other) = default;
    Document &operator=(const Document &other) = delete;
    Document(Document &&other) = default;
    Document &operator=(Document &&other) = delete;

    void download();

private:
    const string _uri;
    string _raw_doc;
};
} // namespace mastorss

#endif  // MASTORSS_DOCUMENT_HPP
