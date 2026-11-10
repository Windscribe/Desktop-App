#pragma once

#include <functional>
#include <optional>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

// A hydra profile (kind windscribe.hydra.profile, version 2): private UDP entry points, each in front
// of one Windscribe datacenter; hydra forwards to one of the datacenter's servers. A front applies to the datacenter with its datacenter_id; its WireGuard hostname is
// what hydra resolves, and only logged here. The profile carries addresses only; keys and AmneziaWG settings come from the
// session as usual. The parse rules match the hydra CLI's, so a file it writes always imports and a
// file with any other field (a pasted PrivateKey, or a version 1 server address) never does.
class HydraProfile
{
public:
    struct Front
    {
        QString location;     // the profile's own label; free text, so never what the app names it by
        QString datacenter;   // the datacenter's WireGuard hostname (the server list's wg_endpoint)
        int datacenterId = 0; // the datacenter's id (the server list's group id), which the app matches on
        uint exitPort = 0;
        QString entryIp;
        uint entryPort = 0;

        QString endpoint() const { return entryIp + ":" + QString::number(entryPort); }
        // endpoint() for logs: the entry is often also the user's own egress address.
        QString redactedEndpoint() const { return redactIp(entryIp) + ":" + QString::number(entryPort); }
    };

    // What Preferences shows: the locations to select (at most kShownLocations, each as the server list
    // names it, or the profile's label marked as not in the server list) and how many more there are; or,
    // for a profile that covers every location, only that.
    struct Locations
    {
        bool all = false;
        QStringList shown;
        int more = 0;
    };

    static constexpr int kMaxFileSize = 64 * 1024;
    static constexpr int kShownLocations = 3;

    // On failure returns nullopt and puts a message for the user in error. Only an import validates
    // (strict); a stored profile was validated when it was imported, so reading it back only needs its
    // fields to be readable, and a later, stricter app keeps it.
    static std::optional<HydraProfile> parse(const QByteArray &json, QString &error, bool strict = true);

    // An IPv4 address with its last octet hidden, for logs.
    static QString redactIp(const QString &ip);

    // A DNS name, as hydra checks a front's datacenter: dot-separated labels of ASCII letters, digits
    // and inner hyphens, 1-63 characters each, 253 in all, and at least two labels.
    static bool isHostname(const QString &name);

    const QString &id() const { return id_; }
    // Whether the fronts cover every Windscribe location there was when hydra wrote the profile.
    bool all() const { return all_; }
    const QVector<Front> &fronts() const { return fronts_; }
    const QByteArray &json() const { return json_; }

    // The front for the datacenter with this id (a server list group's id), or nullptr.
    const Front *frontForDatacenter(int datacenterId) const;
    // One line per front, for the CLI and the preferences page. serverListName gives the app's own name
    // for a datacenter, or an empty string when it isn't in the server list; only then does the line use
    // the profile's label, marked as not in the server list.
    QStringList summary(const std::function<QString(int datacenterId)> &serverListName) const;
    Locations locations(const std::function<QString(int datacenterId)> &serverListName) const;

    // The imported profile lives in the app's data folder, next to the advanced parameters file.
    static QString path();
    // nullopt when nothing is imported, or when the stored file can't be read (logged, and the reason put
    // in error when given). Not validated again: see parse().
    static std::optional<HydraProfile> load(QString *error = nullptr);
    // Validates json and stores it, replacing any imported profile.
    static std::optional<HydraProfile> save(const QByteArray &json, QString &error);
    static bool remove();

private:
    QString id_;
    bool all_ = false;
    QVector<Front> fronts_;
    QByteArray json_;
};
