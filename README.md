**mastorss** dumps RSS feeds into a mastodon account.
Supports RSS 2.0 but not RSS 0.92. Does not support Atom at the moment.

\<item\>s in feeds must have \<link\>, \<title\> and \<description\>.

The documentation is far from complete, sorry.

# Install

## Dependencies

 * Tested OS: Linux
 * C++ compiler (tested: gcc 6.3, 7.3)
 * [cmake](https://cmake.org/) (tested: 3.9 / 3.11)
 * [boost](http://www.boost.org/) (tested: 1.65 / 1.62)
 * [curlpp](http://www.curlpp.org/) (tested: 0.8 / 0.7)
 * [mastodon-cpp](https://schlomp.space/tastytea/mastodon-cpp) (at least: 0.18.4)
 * [jsoncpp](https://github.com/open-source-parsers/jsoncpp) (tested: 1.8 / 1.7)

## Get sourcecode

### Development version

    git clone https://schlomp.space/tastytea/mastorss.git

## Compile

    mkdir build
    cd build/
    cmake ..
    make

## Install

Install with `make install`.

# Usage

Put `watchwords.json` into `~/.config/mastorss/`. Launch with profile name.
The first occurence of every watchword in an RSS item will be turned into a hashtag.
For profile-specific watchwords see the example in `watchwords.json`.
In the first run only the newest entry is tooted.

The profile can't be named "global".

## Example config file

${HOME}/.config/mastorss/config-example.json

    {
        "example": {
            "instance": "botsin.space",
            "feedurl": "https:\/\/example.com\/feed.rss",
            "access_token": "123abc",
            "max_size": 400,
            "titles_only": false,
            "skip":
            [
                "If the entry starts with this, skip it",
                "Skip me too!"
            ],
            "fixes":
            [
                "delete this",
                "<p>[Rr]ead more(\.{3}|…)</p>"
            ],
            "append": "#bot"
        }
    }


## Error codes

Same as [mastodon-cpp](https://schlomp.space/tastytea/mastodon-cpp/src/branch/master/README.md#error-codes)

# Copyright

    Copyright © 2018 tastytea <tastytea@tastytea.de>.
    License GPLv3: GNU GPL version 3 <https://www.gnu.org/licenses/gpl-3.0.html>.
    This program comes with ABSOLUTELY NO WARRANTY. This is free software,
    and you are welcome to redistribute it under certain conditions.
