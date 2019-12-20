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

#include <string>

using namespace mastorss;
using std::string;
using std::to_string;

HTTPException::HTTPException(const int error)
    : error_code{static_cast<uint16_t>(error)}
{}

const char *HTTPException::what() const noexcept
{
    static const string error_string{"HTTP error: " + to_string(error_code)};
    return error_string.c_str();
}

CURLException::CURLException(const int error)
    : error_code{static_cast<uint16_t>(error)}
{}

const char *CURLException::what() const noexcept
{
    static const string error_string{"libCURL error: " + to_string(error_code)};
    return error_string.c_str();
}
