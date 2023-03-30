#pragma once

#include "baserequest.h"
#include "types/robertfilter.h"

namespace server_api {

class SetRobertFiltersRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit SetRobertFiltersRequest(QObject *parent, const QString &authHash, const types::RobertFilter &filter);

    QString contentTypeHeader() const override;
    QByteArray postData() const override;
    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

private:
    QString authHash_;
    types::RobertFilter filter_;
};

} // namespace server_api {

