#ifndef PROTOBUFCOMMAND_H
#define PROTOBUFCOMMAND_H

#include "command.h"

namespace IPC
{

template <class T>
class ProtobufCommand : public Command
{
public:
    ProtobufCommand(char *buf = NULL, int size = 0)
    {
        if (buf != NULL && size != 0)
        {
            protoObj.ParseFromArray(buf, size);
        }
    }

    virtual std::vector<char> getData() const
    {
        size_t size = protoObj.ByteSizeLong();
        std::vector<char> buf(size);
        if (size > 0)
        {
            protoObj.SerializeToArray(&buf[0], size);
        }
        return buf;
    }

    virtual std::string getStringId() const
    {
        return protoObj.descriptor()->full_name();
    }

    virtual std::string getDebugString() const
    {
        return "[" + protoObj.descriptor()->name() + "] " + protoObj.DebugString();
    }

    T &getProtoObj() { return protoObj; }

private:
    T protoObj;
};

} // namespace IPC

#endif // PROTOBUFCOMMAND_H
