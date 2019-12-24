#include "config.hpp"
#include "document.hpp"
#include "exceptions.hpp"
#include "version.hpp"

#include <iostream>
#include <string_view>
#include <vector>

using namespace mastorss;
using std::cout;
using std::cerr;
using std::string_view;
using std::vector;

namespace mastorss
{
namespace error
{
constexpr int noprofile = 1;
constexpr int network = 2;
constexpr int file = 3;
constexpr int mastodon = 4;
constexpr int unknown = 9;
} // namespace error

void print_version();
void print_help(const string_view &command);

void print_version()
{
    cout << "mastorss " << version << "\n"
        "Copyright (C) 2019 tastytea <tastytea@tastytea.de>\n"
        "License GPLv3: GNU GPL version 3 "
        "<https://www.gnu.org/licenses/gpl-3.0.html>.\n"
        "This program comes with ABSOLUTELY NO WARRANTY. "
        "This is free software,\n"
        "and you are welcome to redistribute it under certain conditions.\n";
}

void print_help(const string_view &command)
{
    cerr << "Usage: " << command << " [--version] <profile>\n";
}
} // namespace mastorss

int main(int argc, char *argv[])
{
    const vector<string_view> args(argv, argv + argc);

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
        else
        {
            const string_view profile = args[1];
            BOOST_LOG_TRIVIAL(debug) << "Using profile: " << profile;

            try
            {
                Config cfg(profile.data());
                Document doc(cfg.data.feedurl);
            }
            catch (const FileException &e)
            {
                cerr << e.what() << '\n';
                return error::file;
            }
            catch (const MastodonException &e)
            {
                cerr << e.what() << '\n';
                return error::mastodon;
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
            catch (const runtime_error &e)
            {
                cerr << e.what() << '\n';
                return error::unknown;
            }
        }
    }

    return 0;
}
