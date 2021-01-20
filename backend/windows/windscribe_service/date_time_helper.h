#pragma once

class DateTimeHelper
{
public:
    DateTimeHelper();

    template<typename T>
    std::basic_string<T> getCurrentTimeString() const;

private:
    UINT64 init_milliseconds_;
};
