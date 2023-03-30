#pragma once

#include "baserequest.h"
#include "types/robertfilter.h"

namespace server_api {

class GetRobertFiltersRequest : public BaseRequest
{
    Q_OBJECT
public:
    explicit GetRobertFiltersRequest(QObject *parent, const QString &authHash);

    QUrl url(const QString &domain) const override;
    QString name() const override;
    void handle(const QByteArray &arr) override;

    // output values
    QVector<types::RobertFilter> filters() const { return filters_; }

private:
    QString authHash_;
    // output values
    QVector<types::RobertFilter> filters_;
};

} // namespace server_api {

