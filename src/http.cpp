/*  This file is part of rss2mastodon.
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
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "rss2mastodon.hpp"

using std::string;
using std::cerr;

using boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> ssl_socket;

const std::uint16_t http_get(const string &host, const string &path, string &answer, const string &useragent)
{
    ssl::context ctx(ssl::context::tlsv12);
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    ssl::stream<boost::asio::ip::tcp::socket> socket(io_service, ctx);

    ctx.set_options(ssl::context::tlsv12 | ssl::context::tlsv11 |
                    ssl::context::no_sslv3 | ssl::context::no_sslv2 |
                    ssl::context::no_tlsv1);
    ctx.set_default_verify_paths();

    try
    {
        tcp::resolver::query query(host, "https");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        boost::asio::connect(socket.lowest_layer(), endpoint_iterator);
        socket.lowest_layer().set_option(tcp::no_delay(true));
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR: " << e.what() << "\n";
        return 16;
    }

    try
    {
        // Server Name Indication (SNI)
        SSL_set_tlsext_host_name(socket.native_handle(), host.c_str());

        socket.set_verify_mode(ssl::verify_peer);
        socket.set_verify_callback(ssl::rfc2818_verification(host));

        socket.handshake(ssl_socket::client);
    }
    catch (const std::exception &e)
    {
        cerr << "ERROR: " << e.what() << "\n";
        return 17;
    }

    try
    {
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << "GET " << path;
        request_stream << " HTTP/1.0\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n";
        if (useragent.empty())
        {
            request_stream << "\r\n";
        }
        else
        {
            request_stream << "User-Agent: " << useragent << "\r\n\r\n";
        }

        boost::asio::write(socket, request);

        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        std::uint16_t status_code;
        std::string status_message;
        response_stream >> http_version;
        response_stream >> status_code;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            cerr << "ERROR: Invalid response from server\n";
            cerr << "Response was: " << http_version << " " << status_code
                    << " " << status_message << '\n';
            return 18;
        }
        if (status_code != 200)
        {
            cerr << "ERROR: Response returned with status code "
                    << status_code << ": " << status_message << "\n";
            return status_code;
        }

        // Read headers
        boost::asio::read_until(socket, response, "\r\n\r\n");
        std::string header;
        // cerr << "Header: \n";
        while (std::getline(response_stream, header) && header != "\r")
        {
            // cerr << header << '\n';
        }

        // Read body
        boost::system::error_code error;
        answer = "";
        std::ostringstream oss;
        while (boost::asio::read(socket, response,
                                 boost::asio::transfer_at_least(1), error))
        {
            oss << &response;
        }
        if (error != boost::asio::error::eof)
        {
            // TODO: Find out why the "short read" error occurs
            //throw boost::system::system_error(error);
            //cerr << "ERROR: " << error.message() << '\n';
        }
        answer = oss.str();
        //cerr << "Answer from server: " << oss.str() << '\n';
    }
    catch (const std::exception &e)
    {
        cerr << "Exception: " << e.what() << "\n";
        return 0xffff;
    }

    return 0;
}
