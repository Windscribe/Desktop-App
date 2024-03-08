#pragma once

#include <QTimer>
#include <wsnet/WSNet.h>
#include "api_responses/checkupdate.h"


namespace api_resources {

class CheckUpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit CheckUpdateManager(QObject *parent);
    virtual ~CheckUpdateManager();

    // makes an immediate call to check update and runs a regular update every 24 hours
    void checkUpdate(UPDATE_CHANNEL channel);

signals:
    void checkUpdateUpdated(const api_responses::CheckUpdate &checkUpdate);

private slots:
    void onFetchTimer();

private:
    UPDATE_CHANNEL updateChannel_;
    std::shared_ptr<wsnet::WSNetCancelableCallback> curRequest_;
    QTimer *fetchTimer_;

    static constexpr int k24Hours = 24 * 60 * 60 * 1000;
    static constexpr int kMinute = 60 * 1000;

    void fetchCheckUpdate();
    void onCheckUpdateAnswer(wsnet::ServerApiRetCode serverApiRetCode, const std::string &jsonData);
};

} // namespace api_resources
