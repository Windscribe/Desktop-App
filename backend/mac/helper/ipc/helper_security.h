#ifndef HELPER_SECURITY_H
#define HELPER_SECURITY_H

#include <map>
#include <unistd.h>

class HelperSecurity
{
public:
    static HelperSecurity &instance()
    {
        static HelperSecurity single_instance;
        return single_instance;
    }

    // Reset cached validity.
    void reset();

    // Check if process id is a trusted Windscribe engine app.
    bool verifyProcessId(pid_t pid);

private:
    bool verifyProcessIdImpl(pid_t pid);
    std::map<pid_t,bool> pid_validity_cache_;
};

#endif  // HELPER_SECURITY_H
