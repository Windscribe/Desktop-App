#include "persistentsettings.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <chrono>
#include "utils/wsnet_logger.h"


namespace wsnet {

PersistentSettings::PersistentSettings(const std::string &settings)
{
    using namespace rapidjson;

    if (settings.empty()) {
        g_logger->info("Use default ServerAPI settings");
    } else {
        Document doc;
        doc.Parse(settings.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            g_logger->error("ServerAPI settings incorrect format, use default ServerAPI settings");
            return;
        }

        auto jsonObject = doc.GetObj();
        if (!jsonObject.HasMember("version")) {
            g_logger->error("ServerAPI settings incorrect format, use default ServerAPI settings");
            return;
        }

        if (jsonObject.HasMember("flvId"))
            failoverId_ = jsonObject["flvId"].GetString();
        if (jsonObject.HasMember("countryOverride"))
            countryOverride_ = jsonObject["countryOverride"].GetString();

        if (jsonObject.HasMember("authHash"))
            authHash_ = jsonObject["authHash"].GetString();
        if (jsonObject.HasMember("sessionStatus"))
            sessionStatus_ = jsonObject["sessionStatus"].GetString();
        if (jsonObject.HasMember("locations"))
            locations_ = jsonObject["locations"].GetString();
        if (jsonObject.HasMember("serverCredentialsOvpn"))
            serverCredentialsOvpn_ = jsonObject["serverCredentialsOvpn"].GetString();
        if (jsonObject.HasMember("serverCredentialsIkev2"))
            serverCredentialsIkev2_ = jsonObject["serverCredentialsIkev2"].GetString();
        if (jsonObject.HasMember("serverConfigs"))
            serverConfigs_ = jsonObject["serverConfigs"].GetString();
        if (jsonObject.HasMember("portMap"))
            portMap_ = jsonObject["portMap"].GetString();
        if (jsonObject.HasMember("staticIps"))
            staticIps_ = jsonObject["staticIps"].GetString();
        if (jsonObject.HasMember("notifications"))
            notifications_ = jsonObject["notifications"].GetString();
        if (jsonObject.HasMember("sessionToken")) {
            if (jsonObject["sessionToken"].IsObject()) {
                auto now = std::chrono::system_clock::now();
                auto nowTimestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                for (auto& m : jsonObject["sessionToken"].GetObject()) {
                    if (m.value.IsObject()) {
                        auto tokenObj = m.value.GetObject();
                        if (tokenObj.HasMember("token") && tokenObj["token"].IsString() &&
                            tokenObj.HasMember("expiry") && tokenObj["expiry"].IsInt64()) {
                            std::int64_t expiryTimestamp = tokenObj["expiry"].GetInt64();
                            if (expiryTimestamp > nowTimestamp) {
                                // Valid token
                                sessionTokens_[m.name.GetString()] = {tokenObj["token"].GetString(), expiryTimestamp};
                            } else if (expiryTimestamp == 0) {
                                // No expiry time (reboot without disconnecting?) Set expiry time to 24h from now
                                sessionTokens_[m.name.GetString()] = {tokenObj["token"].GetString(), nowTimestamp + 86400};
                            } else {
                                // Expired token
                                sessionTokens_.erase(m.name.GetString());
                            }
                        }
                    }
                }
            }
        }

        g_logger->info("ServerAPI settings settled sucessfully");
    }
}

void PersistentSettings::setFailoverId(const std::string &failoverId)
{
    std::lock_guard locker(mutex_);
    failoverId_ = failoverId;
}

std::string PersistentSettings::failoverId() const
{
    std::lock_guard locker(mutex_);
    return failoverId_;
}

void PersistentSettings::setCountryOverride(const std::string &countryOverride)
{
    std::lock_guard locker(mutex_);
    countryOverride_ = countryOverride;
}

std::string PersistentSettings::countryOverride() const
{
    std::lock_guard locker(mutex_);
    return countryOverride_;
}

void PersistentSettings::setAuthHash(const std::string &authHash)
{
    std::lock_guard locker(mutex_);
    authHash_ = authHash;
}

std::string PersistentSettings::authHash() const
{
    std::lock_guard locker(mutex_);
    return authHash_;
}

void PersistentSettings::setSessionStatus(const std::string &sessionStatus)
{
    std::lock_guard locker(mutex_);
    sessionStatus_ = sessionStatus;
}

std::string PersistentSettings::sessionStatus() const
{
    std::lock_guard locker(mutex_);
    return sessionStatus_;
}

void PersistentSettings::setLocations(const std::string &locations)
{
    std::lock_guard locker(mutex_);
    locations_ = locations;
}

std::string PersistentSettings::locations() const
{
    std::lock_guard locker(mutex_);
    return locations_;
}

void PersistentSettings::setServerCredentialsOvpn(const std::string &serverCredentials)
{
    std::lock_guard locker(mutex_);
    serverCredentialsOvpn_ = serverCredentials;
}

std::string PersistentSettings::serverCredentialsOvpn() const
{
    std::lock_guard locker(mutex_);
    return serverCredentialsOvpn_;
}

void PersistentSettings::setServerCredentialsIkev2(const std::string &serverCredentials)
{
    std::lock_guard locker(mutex_);
    serverCredentialsIkev2_ = serverCredentials;
}

std::string PersistentSettings::serverCredentialsIkev2() const
{
    std::lock_guard locker(mutex_);
    return serverCredentialsIkev2_;
}

void PersistentSettings::setServerConfigs(const std::string &serverConfigs)
{
    std::lock_guard locker(mutex_);
    serverConfigs_ = serverConfigs;
}

std::string PersistentSettings::serverConfigs() const
{
    std::lock_guard locker(mutex_);
    return serverConfigs_;
}

void PersistentSettings::setPortMap(const std::string &portMap)
{
    std::lock_guard locker(mutex_);
    portMap_ = portMap;
}

std::string PersistentSettings::portMap() const
{
    std::lock_guard locker(mutex_);
    return portMap_;
}

void PersistentSettings::setStaticIps(const std::string &staticIps)
{
    std::lock_guard locker(mutex_);
    staticIps_ = staticIps;
}

std::string PersistentSettings::staticIps() const
{
    std::lock_guard locker(mutex_);
    return staticIps_;
}

void PersistentSettings::setNotifications(const std::string &notifications)
{
    std::lock_guard locker(mutex_);
    notifications_ = notifications;
}

std::string PersistentSettings::notifications() const
{
    std::lock_guard locker(mutex_);
    return notifications_;
}

void PersistentSettings::setSessionTokens(const std::map<std::string, std::pair<std::string, std::int64_t>> &sessionTokens)
{
    std::lock_guard locker(mutex_);
    sessionTokens_ = sessionTokens;
}

std::map<std::string, std::pair<std::string, std::int64_t>> PersistentSettings::sessionTokens() const
{
    std::lock_guard locker(mutex_);
    return sessionTokens_;
}

std::string PersistentSettings::getAsString() const
{
    std::lock_guard locker(mutex_);
    // serialize to json string
    using namespace rapidjson;
    Document doc;
    doc.SetObject();
    doc.AddMember("version", kVersion, doc.GetAllocator());
    if (!failoverId_.empty())
        doc.AddMember("flvId", StringRef(failoverId_.c_str()), doc.GetAllocator());
    if (!countryOverride_.empty())
        doc.AddMember("countryOverride", StringRef(countryOverride_.c_str()), doc.GetAllocator());

    if (!authHash_.empty())
        doc.AddMember("authHash", StringRef(authHash_.c_str()), doc.GetAllocator());
    if (!sessionStatus_.empty())
        doc.AddMember("sessionStatus", StringRef(sessionStatus_.c_str()), doc.GetAllocator());
    if (!locations_.empty())
        doc.AddMember("locations", StringRef(locations_.c_str()), doc.GetAllocator());
    if (!serverCredentialsOvpn_.empty())
        doc.AddMember("serverCredentialsOvpn", StringRef(serverCredentialsOvpn_.c_str()), doc.GetAllocator());
    if (!serverCredentialsIkev2_.empty())
        doc.AddMember("serverCredentialsIkev2", StringRef(serverCredentialsIkev2_.c_str()), doc.GetAllocator());
    if (!serverConfigs_.empty())
        doc.AddMember("serverConfigs", StringRef(serverConfigs_.c_str()), doc.GetAllocator());
    if (!portMap_.empty())
        doc.AddMember("portMap", StringRef(portMap_.c_str()), doc.GetAllocator());
    if (!staticIps_.empty())
        doc.AddMember("staticIps", StringRef(staticIps_.c_str()), doc.GetAllocator());
    if (!notifications_.empty())
        doc.AddMember("notifications", StringRef(notifications_.c_str()), doc.GetAllocator());
    if (!sessionTokens_.empty()) {
        Value tokenObj(kObjectType);
        for (const auto& [key, tokenPair] : sessionTokens_) {
            Value hostKey(key.c_str(), doc.GetAllocator());
            Value tokenData(kObjectType);
            tokenData.AddMember("token", Value(tokenPair.first.c_str(), doc.GetAllocator()), doc.GetAllocator());
            tokenData.AddMember("expiry", tokenPair.second, doc.GetAllocator());
            tokenObj.AddMember(hostKey, tokenData, doc.GetAllocator());
        }
        doc.AddMember("sessionToken", tokenObj, doc.GetAllocator());
    }

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

} // namespace wsnet
