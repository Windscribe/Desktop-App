#ifndef Logger_h
#define Logger_h

#define LOG(str, ...) logOut("[%s:%d] " str, __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__)


void logOut(const char *str, ...);

#endif // Logger_h
