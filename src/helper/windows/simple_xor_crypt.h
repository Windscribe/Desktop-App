#pragma once

class SimpleXorCrypt
{
public:
    static std::string encrypt(std::string &data, const std::string &key);
    static std::string decrypt(std::string &data, const std::string &key);
};

