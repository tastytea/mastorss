#ifndef mastorss_HPP
#define mastorss_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;
using std::string;

extern std::uint16_t max_size;
extern const string filepath;

std::uint16_t read_config(pt::ptree &config, const string &profile, string &instance, string &access_token, string &feedurl);

std::vector<string> parse_website(const string &profile, const string &xml);
void unescape_html(const string &str);

const std::uint16_t http_get(const string &feedurl,
                             string &answer, const string &useragent = "");
void curlpp_init();

#endif // mastorss_HPP