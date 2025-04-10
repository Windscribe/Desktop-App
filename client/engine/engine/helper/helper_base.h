#pragma once

#include "ihelperbackend.h"
#include <sstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include "../../../../backend/common/helper_commands.h"

class HelperBase
{
public:
    // Take ownership of the backend
    explicit HelperBase(std::unique_ptr<IHelperBackend> backend) : backend_(std::move(backend)) {}
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
        std::istringstream iss(data, std::ios::binary);
        boost::archive::binary_iarchive archive(iss, boost::archive::no_header);
        if constexpr (sizeof...(Args) > 0) {
            (archive >> ... >> args);
        }
    }

private:
    std::unique_ptr<IHelperBackend> backend_;

};
