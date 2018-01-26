#ifndef AFRINTICKER_HPP
#define AFRINTICKER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;
using std::string;

void read_config(pt::ptree &config, const string &profile, string &instance, string &access_token);
std::vector<string> parse_website(const string &profile, const string &xml);

// http.cpp
const std::uint16_t http_get(const string &host, const string &path,
                             string &answer, const string &useragent = "");

#endif // AFRINTICKER_HPP
