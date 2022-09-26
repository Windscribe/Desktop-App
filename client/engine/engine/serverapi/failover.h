#pragma once

#include <QObject>

namespace server_api {

//
class Failover : public QObject
{
    Q_OBJECT
public:
    explicit Failover(QObject *parent);
    virtual ~Failover();


    void reset();

private:
};

} // namespace server_api
