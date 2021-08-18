
#ifndef KextMonitor_h
#define KextMonitor_h

#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

class KextMonitor
{
public:
    KextMonitor();
    
    void setKextPath(const std::string &kextPath);
    bool loadKext();
    bool unloadKext();
    
private:
    std::string kextPath_;
    bool isKextLoaded_;
        
    void chmodRecursively(const char *name, uid_t user, gid_t group);
    void chmodIfNeed(const char *name, uid_t user, gid_t group);

};

#endif /* KextMonitor_h */
