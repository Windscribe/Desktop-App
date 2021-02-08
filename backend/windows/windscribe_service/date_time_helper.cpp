#include "all_headers.h"
#include "date_time_helper.h"
#include <iomanip>

namespace
{
UINT64 GetMillisecondsFromSystemTime(const SYSTEMTIME &st)
{
    return 3600000 * st.wHour + 60000 * st.wMinute + 1000 * st.wSecond + st.wMilliseconds;
}
}

template std::basic_string<char> DateTimeHelper::getCurrentTimeString() const;
template std::basic_string<wchar_t> DateTimeHelper::getCurrentTimeString() const;

DateTimeHelper::DateTimeHelper()
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(st));
    GetSystemTime(&st);
    init_milliseconds_ = GetMillisecondsFromSystemTime(st);
}

template<typename T>
std::basic_string<T> DateTimeHelper::getCurrentTimeString() const
{
    SYSTEMTIME st;
    memset(&st, 0, sizeof(st));
    GetSystemTime(&st);
    const auto current_milliseconds = GetMillisecondsFromSystemTime(st);

    std::basic_ostringstream<T> output_stream;
    output_stream.fill(T('0'));
    output_stream.setf(std::ios::fixed);

    output_stream << std::setw(2) << st.wDay << std::setw(2) << st.wMonth
                  << std::setw(2) << (st.wYear - 2000) << T(' ')
                  << std::setw(2) << st.wHour << T(':') << std::setw(2) << st.wMinute << T(':')
                  << std::setw(2) << st.wSecond << T(':')
                  << std::setw(3) << st.wMilliseconds << T(' ')
                  << std::setw(10) << std::setprecision(3) << std::setfill(T(' '))
                  << (current_milliseconds - init_milliseconds_) / 1000.0;

    return output_stream.str();
}
