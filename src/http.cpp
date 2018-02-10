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
#include <curlpp/Infos.hpp>
#include "mastorss.hpp"

using std::string;
using std::cerr;

namespace curlopts = curlpp::options;

void curlpp_init()
{
    curlpp::initialize();
}

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
        request.setOpt<curlopts::FollowLocation>(true);
        request.setOpt<curlopts::WriteStream>(&oss);
        
        request.perform();
        std::uint16_t ret = curlpp::infos::ResponseCode::get(request);
        if (ret == 200 || ret == 302 || ret == 307)
        {   // OK or Found or Temporary Redirect
            answer = oss.str();
        }
        else if (ret == 301 || ret == 308)
        {   // Moved Permanently or Permanent Redirect
            // FIXME: The new URL should be passed back somehow
            answer = oss.str();
        }
        else
        {
            return ret;
        }

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
