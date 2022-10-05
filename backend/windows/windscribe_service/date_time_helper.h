#pragma once

#include <string>

class DateTimeHelper
{
public:
    DateTimeHelper();

    template<typename T>
    std::basic_string<T> getCurrentTimeString() const;

private:
    unsigned long long init_milliseconds_;
};
