#ifndef mastorss_HPP
#define mastorss_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <jsoncpp/json/json.h>

using std::string;

extern std::uint16_t max_size;
extern const string filepath;
extern Json::Value config;
extern std::string profile;

std::uint16_t read_config(string &instance, string &access_token, string &feedurl);
const bool write_config();

std::vector<string> parse_website(const string &xml);
void unescape_html(const string &str);
void individual_fixes(string &str);

const std::uint16_t http_get(const string &feedurl,
                             string &answer, const string &useragent = "");
void curlpp_init();

#endif // mastorss_HPP
