#include "utils.h"

#include <thread>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace utils {

// generate random integer in [min, max]
#ifdef _WIN32
__declspec(thread) char _generator_backing_double[sizeof(std::mt19937)];
__declspec(thread) std::mt19937* _generator_double;
__declspec(thread) char _generator_backing_int[sizeof(std::mt19937)];
__declspec(thread) std::mt19937* _generator_int;
#endif

int random(const int &min, const int &max)
{
    std::uniform_int_distribution<int> distribution(min, max);

#ifdef _WIN32
    static __declspec(thread) bool inited = false;
    if (!inited)
    {
        _generator_int = new(_generator_backing_int) std::mt19937(clock() + (unsigned int)std::hash<std::thread::id>()(std::this_thread::get_id()));
        inited = true;
    }
    return distribution(*_generator_int);
#else
    static thread_local std::mt19937* generator = nullptr;
    if (!generator) generator = new std::mt19937(clock() + std::hash<std::thread::id>()(std::this_thread::get_id()));
    return distribution(*generator);
#endif
    return 0;
}

// check if ip is valid ip address string
bool isIpAddress(const std::string &ip)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
    return result > 0;
}


}
