#include "serverapi_settings.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <spdlog/spdlog.h>


namespace wsnet {

ServerAPISettings::ServerAPISettings(const std::string &settings)
{
    using namespace rapidjson;

    if (settings.empty()) {
        spdlog::info("Use default ServerAPI settings");
    } else {
        Document doc;
        doc.Parse(settings.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            spdlog::error("ServerAPI settings incorrect format, use default ServerAPI settings");
            return;
        }

        auto jsonObject = doc.GetObj();
        if (!jsonObject.HasMember("version")) {
            spdlog::error("ServerAPI settings incorrect format, use default ServerAPI settings");
            return;
        }

        if (jsonObject.HasMember("flvId"))
            failoverId_ = jsonObject["flvId"].GetString();
        if (jsonObject.HasMember("countryOverride"))
            countryOverride_ = jsonObject["countryOverride"].GetString();

        spdlog::error("ServerAPI settings settled sucessfully");
    }
}

void ServerAPISettings::setFailovedId(const std::string &failoverId)
{
    std::lock_guard locker(mutex_);
    failoverId_ = failoverId;
}

std::string ServerAPISettings::failoverId() const
{
    std::lock_guard locker(mutex_);
    return failoverId_;
}

void ServerAPISettings::setCountryOverride(const std::string &countryOverride)
{
    std::lock_guard locker(mutex_);
    countryOverride_ = countryOverride;
}

std::string ServerAPISettings::countryOverride() const
{
    std::lock_guard locker(mutex_);
    return countryOverride_;
}

std::string ServerAPISettings::getAsString() const
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

    StringBuffer sb;
    Writer<StringBuffer> writer(sb);
    doc.Accept(writer);
    return sb.GetString();
}

} // namespace wsnet
