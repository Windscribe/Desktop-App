#pragma once
#include <string>
#include <vector>

class SimpleXorCrypt
{
public:
    static std::string encrypt(const std::string &data, const std::string &key);
    static std::string decrypt(const std::string &data, const std::string &key);
};

