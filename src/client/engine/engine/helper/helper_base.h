#pragma once

#include "ihelperbackend.h"
#include <sstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "../../../../helper/common/helper_commands.h"
#include <spdlog/spdlog.h>

class HelperBase
{
public:
    // Take ownership of the backend
    explicit HelperBase(std::unique_ptr<IHelperBackend> backend, spdlog::logger *logger) : backend_(std::move(backend)), logger_(logger) {}
    virtual ~HelperBase() {}

    IHelperBackend *backend() { return backend_.get(); }

protected:

    // Serialize all args with boost and send a command
    template<typename... Args>
    std::string sendCommand(HelperCommand cmdId, const Args&... args)
    {
        std::ostringstream oss(std::ios::binary);
        boost::archive::binary_oarchive archive(oss, boost::archive::no_header);
        if constexpr (sizeof...(Args) > 0) {
            (archive << ... << args);
        }
        return backend_->sendCmd((int)cmdId, oss.str());
    }

    // Deserialize returned parameters
    template<typename... Args>
    void deserializeAnswer(const std::string &data, Args&... args)
    {
        if (data.empty()) {
            logger_->error("HelperBase::deserializeAnswer error: data is empty");
            return;
        }
        try {
            std::istringstream iss(data, std::ios::binary);
            boost::archive::binary_iarchive archive(iss, boost::archive::no_header);
            if constexpr (sizeof...(Args) > 0) {
                (archive >> ... >> args);
            }
        } catch(const std::exception & ex) {

            logger_->error("HelperBase::deserializeAnswer exception: {}", ex.what());
        }
    }

private:
    std::unique_ptr<IHelperBackend> backend_;
    spdlog::logger *logger_;
};
