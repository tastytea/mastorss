/*  This file is part of mastorss.
 *  Copyright © 2018 tastytea <tastytea@tastytea.de>
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

#include <string>
#include <cstdint>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include "mastorss.hpp"

using std::string;
using std::cerr;

namespace curlopts = curlpp::options;

const std::uint16_t http_get(const string &feedurl, string &answer, const string &useragent)
{
    try
    {
        std::ostringstream oss;
        curlpp::Easy request;
        request.setOpt<curlopts::Url>(feedurl);
        request.setOpt<curlopts::UserAgent>(useragent);
        request.setOpt<curlopts::HttpHeader>(
        {
            "Connection: close",
        });
        
        
        oss << request;
        answer = oss.str();

        return 0;
    }
    catch (curlpp::RuntimeError &e)
    {
        cerr << "RUNTIME ERROR: " << e.what() << std::endl;
        return 0xffff;
    }
    catch (curlpp::LogicError &e)
    {
        cerr << "LOGIC ERROR: " << e.what() << std::endl;
        return 0xffff;
    }

    return 0;
}
