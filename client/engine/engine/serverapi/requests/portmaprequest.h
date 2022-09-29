#pragma once

#include "baserequest.h"
#include "types/portmap.h"

namespace server_api {

class PortMapRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit PortMapRequest(QObject *parent, const QString &authHash);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    types::PortMap portMap() const { return portMap_; }

private:
    QString authHash_;

    // output values
    types::PortMap portMap_;
};

} // namespace server_api {

