
#ifndef KeychainUtils_hpp
#define KeychainUtils_hpp

namespace KeyChainUtils
{
    // set password to system keychain for ikev2 connection
    bool setUsernameAndPassword(const char *label, const char *serviceName, const char *description, const char *username, const char *password);
};

#endif
