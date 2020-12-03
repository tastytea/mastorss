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

#include "config.hpp"
#include "curl_wrapper.hpp"
#include "document.hpp"
#include "exceptions.hpp"
#include "mastoapi.hpp"
#include "version.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace mastorss
{

using std::cerr;
using std::cout;
using std::runtime_error;
using std::string_view;
using std::chrono::seconds;
using std::this_thread::sleep_for;

namespace error
{
constexpr int noprofile = 1;
constexpr int network = 2;
constexpr int file = 3;
// constexpr int mastodon = 4;
constexpr int json = 5;
constexpr int parse = 6;
constexpr int unknown = 9;
} // namespace error

void print_version();
void print_help(string_view command);
int run(string_view profile_name, bool dry_run);

void print_version()
{
    cout
        << "mastorss " << version
        << "\nCopyright (C) 2019, 2020 tastytea <tastytea@tastytea.de>\n"
           "License GPLv3: GNU GPL version 3 "
           "<https://www.gnu.org/licenses/gpl-3.0.html>.\n"
           "This program comes with ABSOLUTELY NO WARRANTY. "
           "This is free software,\n"
           "and you are welcome to redistribute it under certain conditions.\n";
}

void print_help(const string_view command)
{
    cerr << "Usage: " << command << " [--version|--help|--dry-run] <profile>\n"
         << "See manpage for details.\n";
}

int run(const string_view profile_name, const bool dry_run)
{
    const string_view profilename{profile_name};
    BOOST_LOG_TRIVIAL(debug) << "Using profile: " << profilename;

    try
    {
        Config cfg{profilename.data()};
        Document doc{cfg};
        doc.parse();

        MastoAPI masto{cfg.profiledata};
        if (!doc.new_items.empty())
        {
            for (const auto &item : doc.new_items)
            {
                masto.post_item(item, dry_run);
                if (item != *doc.new_items.rbegin())
                { // Don't sleep if this is the last item.
                    if (!dry_run)
                    {
                        sleep_for(seconds(cfg.profiledata.interval));
                    }
                    else
                    {
                        sleep_for(seconds(1));
                    }
                }
            }
            if (!dry_run)
            {
                cfg.write();
            }
        }
    }
    catch (const FileException &e)
    {
        cerr << e.what() << '\n';
        return error::file;
    }
    catch (const HTTPException &e)
    {
        cerr << e.what() << '\n';
        return error::network;
    }
    catch (const CURLException &e)
    {
        cerr << e.what() << '\n';
        return error::network;
    }
    catch (const curl_wrapper::CURLException &e)
    {
        cerr << e.what() << '\n';
        return error::network;
    }
    catch (const Json::RuntimeError &e)
    {
        cerr << "JSON error:\n" << e.what() << '\n';
        return error::json;
    }
    catch (const ParseException &e)
    {
        cerr << e.what() << '\n';
        return error::parse;
    }
    catch (const runtime_error &e)
    {
        cerr << e.what() << '\n';
        return error::unknown;
    }

    return 0;
}

} // namespace mastorss

int main(int argc, char *argv[])
{
    using namespace mastorss;
    using std::getenv;
    using std::string_view;
    using std::vector;

    const vector<string_view> args(argv, argv + argc);

    if (getenv("MASTORSS_DEBUG") == nullptr)
    {
        boost::log::core::get()->set_filter(boost::log::trivial::severity
                                            >= boost::log::trivial::info);
    }
    else
    {
        boost::log::core::get()->set_filter(boost::log::trivial::severity
                                            >= boost::log::trivial::debug);
    }

    if (args.size() == 1)
    {
        print_help(args[0]);
        return error::noprofile;
    }

    if (args.size() > 1)
    {
        if (args[1] == "--version")
        {
            print_version();
        }
        else if (args[1] == "--help")
        {
            print_help(args[0]);
        }
        else if (args[1] == "--dry-run")
        {
            return run(args[2], true);
        }
        else
        {
            return run(args[1], false);
        }
    }

    return 0;
}
