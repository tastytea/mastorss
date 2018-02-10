**mastorss** dumps RSS feeds into a mastodon account.
It is hacked together and generally only extended/fixed when it fails.
Use at your own risk.

# Install

## Dependencies

 * Tested OS: Linux
 * C++ compiler (tested: gcc 6.4, clang 5.0)
 * [cmake](https://cmake.org/) (tested: 3.9.6)
 * [boost](http://www.boost.org/) (tested: 1.63.0)
 * [libcurl](https://curl.haxx.se/) (tested: 7.58.0)
 * [curlpp](http://www.curlpp.org/) (tested: 0.8.1)
 * [mastodon-cpp](https://github.com/tastytea/mastodon-cpp) (at least: 0.2.0)

## Get sourcecode

### Development version

    git clone …

## Compile

    mkdir build
    cd build/
    cmake ..
    make

cmake options:

 * `-DCMAKE_BUILD_TYPE=Debug` for a debug build

Install with `make install`.

# Usage

Put watchwords.json into `~/.config/mastorss/`. Launch with profile name.
In the first run only the newest entry is tooted.

The profile can't be named "global".

## Error codes

|      Code | Explanation                   |
| --------: |:------------------------------|
|         0 | No error                      |
|         1 | Invalid call                  |
|         2 | Not implemented               |
|        16 | Connection failed             |
|        17 | TLS error                     |
|        18 | Invalid response from server  |
|        32 | Wrong number of arguments     |
|        33 | File not found                |
| 100 - 999 | HTTP status codes             |
|     65535 | Unknown exception             |

If you use a debug build, you get more verbose error messages.

# Copyright

    Copyright © 2018 tastytea <tastytea@tastytea.de>.
    License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
    This program comes with ABSOLUTELY NO WARRANTY. This is free software,
    and you are welcome to redistribute it under certain conditions.
