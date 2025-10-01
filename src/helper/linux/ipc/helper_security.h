#pragma once

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

    // Check if process has the correct signature.
    bool verifySignature();
};
