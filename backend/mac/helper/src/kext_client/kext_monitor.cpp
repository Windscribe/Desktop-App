#include <IOKit/kext/KextManager.h>
#include <thread>

#include "kext_monitor.h"
#include "../logger.h"
#include "../utils.h"

static const CFStringRef kDriverKextIdentifier = CFSTR("com.windscribe.kext");

KextMonitor::KextMonitor() : isKextLoaded_(false)
{
}

void KextMonitor::setKextPath(const std::string &kextPath)
{
    // can be called only once
    if (kextPath_.empty())
    {
        kextPath_ = kextPath;
        // Unload kext if it was loaded by a prior run";
        unloadKext();
    }
}

bool KextMonitor::loadKext()
{
    if (kextPath_.empty())
    {
        LOG("Error: kext  path is empty");
        return false;
    }
    
    if (isKextLoaded_)
    {
        LOG("Kext already loaded");
        return true;
    }
    
    // do cmd: sudo chown -R root:wheel "WindscribeKext.kext", if need. Otherwise kext won't load.
    chmodRecursively(kextPath_.c_str(), 0, 0);
    
    std::vector<std::string> pars;
    pars.push_back("-v");
    pars.push_back(kextPath_);
    
    std::string outLog;
    int exitCode = Utils::executeCommand("/usr/bin/kextutil", pars, &outLog);
    if (exitCode == 0)
    {
        LOG("Kext successful loaded");
        isKextLoaded_ = true;
        return true;
    }
    else
    {
        LOG("Failed to load kext: %s", outLog.c_str());
        return false;
    }
}

bool KextMonitor::unloadKext()
{
    if (kextPath_.empty())
    {
        LOG("Error: kext  path is empty");
        return false;
    }

    // Check if the kext is loaded.  The /sbin/kextunload command below blocks for ~1s on
    // macOS >= 11 if the kext is not loaded.  This adds ~8s delay to the startup of the app
    // the first time it is run after install/reboot, as we call kextunload, and this
    // method, multiple times.
    if (!isKextLoaded())
    {
        isKextLoaded_ = false;
        return true;
    }

    LOG("Unloading kext - currently loaded: %d", isKextLoaded_);

    // Unloading our kext may take a few attempts under normal conditions.
    // If we have any sockets filters still attached inside the kext they need to first be unregistered. The first unload attempt does this.
    // Assuming we successfully unregister the socket filters, our second (or shortly there after) attempt should succeed.
    // If we still cannot unload the kext after our final attempt then we have a legitimate error, so return this to the caller.
    std::vector<std::string> pars;
    pars.push_back(kextPath_);
    int retryCount = 5;
    for (int i = 0; i < retryCount; ++i)
    {
        std::string stderr;
        int exitCode = Utils::executeCommand("/sbin/kextunload", pars, &stderr);
        
        if (exitCode == 0)
        {
            LOG("Kext successful unloaded");
            isKextLoaded_ = false;
            return true;
        }

        // If it's not loaded to begin with, treat this as success (don't keep
        // trying).
        // '3' is the return code for most errors, so we have to check the
        // error text.
        if (stderr.find("not found for unload request") != std::string::npos)
        {
            LOG("Kext was not loaded, done trying to unload");
            isKextLoaded_ = false;
            return true;
        }

        if (i+1 < retryCount)
        {
            LOG("Unable to unload Kext, exit code: %d, %s. Retrying %d of %d", exitCode, stderr.c_str(), i+1, retryCount);
            // Wait a little bit before we try to unload again
            // (to give the socket filters time to unregister)
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        else
        {
            LOG("Unable to unload Kext");
            return false;
        }
    }

    return false;
}

void KextMonitor::chmodRecursively(const char *name, uid_t user, gid_t group)
{
    DIR *dir;
        struct dirent *entry;
        char path[PATH_MAX];
    
        if (!(dir = opendir(name)))
            return;
        
        chmodIfNeed(name, user, group);
    
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_DIR)
            {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                
                snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
                chmodRecursively(path, user, group);
            }
            else {
                snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
                chmodIfNeed(path, user, group);
            }
        }
        closedir(dir);
}

void KextMonitor::chmodIfNeed(const char *name, uid_t user, gid_t group)
{
    struct stat statbuf;
    if (!stat(name, &statbuf))
    {
        if (user != statbuf.st_uid || group != statbuf.st_gid)
        {
            chown(name, user, group);
        }
    }
    else
    {
        chown(name, user, group);
    }
}

bool KextMonitor::isKextLoaded() const
{
    bool kextLoaded = false;
    
    CFStringRef kext_ids[1];
    kext_ids[0] = kDriverKextIdentifier;
    CFArrayRef kext_id_query = CFArrayCreate(nullptr, (const void**)kext_ids, 1, &kCFTypeArrayCallBacks);
    CFDictionaryRef kext_infos = KextManagerCopyLoadedKextInfo(kext_id_query, nullptr);
    CFRelease(kext_id_query);

    CFDictionaryRef windscribe_driver_info = nullptr;
    if (CFDictionaryGetValueIfPresent(kext_infos, kDriverKextIdentifier, (const void**)&windscribe_driver_info)) {
        kextLoaded = CFBooleanGetValue((CFBooleanRef)CFDictionaryGetValue(windscribe_driver_info, CFSTR("OSBundleStarted")));
    }
    else {
        LOG("KextMonitor::isKextLoaded(): Windscribe kext not loaded on this host");
    }
    
    CFRelease(kext_infos);
    
    return kextLoaded;
}
