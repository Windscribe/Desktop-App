#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <numeric>
#include <random>

namespace utils {

/* measuring time in ms helper
    usage example:
        auto start = std::chrono::steady_clock::now();
        std::cout << "Elapsed(ms)=" << since(start).count() << std::endl;
*/
template <
    class result_t   = std::chrono::milliseconds,
    class clock_t    = std::chrono::steady_clock,
    class duration_t = std::chrono::milliseconds
    >
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

// Implode a vector of strings into a comma-separated string
inline std::string join(const std::vector<std::string> &strings, const std::string &delim)
{
    if (strings.empty()) {
        return std::string();
    }
    return std::accumulate(strings.begin() + 1, strings.end(), strings[0],
                           [&delim](std::string x, std::string y) {
                               return x + delim + y;
                           }
                           );
}

// Splitting a string by a character
inline std::vector<std::string> split(const std::string &str, char character)
{
    std::stringstream stream(str);
    std::string segment;
    std::vector<std::string> res;
    while(std::getline(stream, segment, character))
        res.push_back(segment);

    return res;

}

// check if ip is valid ip address string
bool isIpAddress(const std::string &ip);

// return first n characters of string
inline std::string leftSubStr(const std::string &s, int n)
{
    if (s.length() >= n) return s.substr(0, n);
    else return s;
}

// find the top domain from the domain name
// for example: api.windscribe.com -> windscribe.com
inline std::string topDomain(const std::string &domain)
{
    if (domain.empty())
        return std::string();

    bool bFirstDotFound = false;
    for (size_t i = domain.size()-1; i > 0; --i) {
        if (domain[i] == '.') {
            if (!bFirstDotFound) {
                bFirstDotFound = true;
            } else {
                return domain.substr(i + 1, domain.size() - i - 1);
            }
        }
    }
    return domain;
}

// generate random integer in [min, max]
int random(const int &min, const int &max);

// Split a string using string delimiter
inline std::vector<std::string> split(const std::string &s, const std::string &delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

template<typename T>
T randomizeList(const T &list)
{
    T result = list;
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle( result.begin(), result.end(), std::default_random_engine((unsigned int)seed));
    return result;
}

}
