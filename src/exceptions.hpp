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

#ifndef MASTORSS_EXCEPTIONS_HPP
#define MASTORSS_EXCEPTIONS_HPP

#include <cstdint>
#include <exception>
#include <string>

namespace mastorss
{
using std::uint16_t;
using std::exception;
using std::string;

class HTTPException : public exception
{
public:
    const uint16_t error_code;

    explicit HTTPException(int error);

    [[nodiscard]]
    const char *what() const noexcept override;
};

class CURLException : public exception
{
public:
    const uint16_t error_code;

    explicit CURLException(int error);

    [[nodiscard]]
    const char *what() const noexcept override;
};

class MastodonException : public exception
{
public:
    const uint16_t error_code;

    explicit MastodonException(int error);

    [[nodiscard]]
    const char *what() const noexcept override;
};

class FileException : public exception
{
public:
    explicit FileException(string message);

    [[nodiscard]]
    const char *what() const noexcept override;

private:
    const string _message;
};
} // namespace mastorss

#endif  // MASTORSS_EXCEPTIONS_HPP
