#pragma once

#include "engine/serverapi/serverapi.h"
#include "types/checkupdate.h"

namespace api_resources {

class CheckUpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit CheckUpdateManager(QObject *parent, server_api::ServerAPI *serverAPI);
    virtual ~CheckUpdateManager();

    // makes an immediate call to check update and runs a regular update every 24 hours
    void checkUpdate(UPDATE_CHANNEL channel);

signals:
    void checkUpdateUpdated(const types::CheckUpdate &checkUpdate);

private slots:
    void onCheckUpdateAnswer();
    void onFetchTimer();

private:
    server_api::ServerAPI *serverAPI_;
    UPDATE_CHANNEL updateChannel_;
    server_api::BaseRequest *curRequest_;
    QTimer *fetchTimer_;

    static constexpr int k24Hours = 24 * 60 * 60 * 1000;
    static constexpr int kMinute = 60 * 1000;

    void fetchCheckUpdate();
};

} // namespace api_resources
