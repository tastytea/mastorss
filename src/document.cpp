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
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>

#include <string>
#include <utility>

namespace mastorss
{
using std::string;
using std::move;

Document::Document(string uri)
    : _uri{move(uri)}
{
    RestClient::init();

    download();
}

Document::~Document()
{
    RestClient::disable();
}

void Document::download()
{
    RestClient::Connection connection(_uri);
    connection.SetUserAgent(string("mastorss/").append(version));
    connection.FollowRedirects(true, 10);

    RestClient::Response response{connection.get("")};

    switch (response.code)
    {
    case 200:
    {
        _raw_doc = response.body;
        BOOST_LOG_TRIVIAL(debug) << "Downloaded feed: " << _uri;
        break;
    }
    case 301:
    case 308:
    {
        // TODO(tastytea): Handle permanent redirections.
        throw std::runtime_error{"Permanent redirect, "
                "no solution implemented yet."};
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
} // namespace mastorss
