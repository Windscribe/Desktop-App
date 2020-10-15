#include "backendcommander.h"

#include "../backend/persistentstate.h"
#include "utils/utils.h"
#include "utils/logger.h"

BackendCommander::BackendCommander(CliCommand cmd, const QString &location) : QObject()
    , backend_(nullptr)
    , command_(cmd)
    , locationStr_(location)
    , receivedStateInit_(false)
    , receivedLocationsInit_(false)
    , sent_(false)
{
    backend_ = new Backend(ProtoTypes::CLIENT_ID_CLI, 54321, "cli app", this);
    connect(dynamic_cast<QObject*>(backend_), SIGNAL(initFinished(ProtoTypes::InitState)), SLOT(onBackendInitFinished(ProtoTypes::InitState)));
    connect(dynamic_cast<QObject*>(backend_), SIGNAL(firewallStateChanged(bool)), SLOT(onBackendFirewallStateChanged(bool)));
    connect(dynamic_cast<QObject*>(backend_), SIGNAL(connectStateChanged(ProtoTypes::ConnectState)), SLOT(onBackendConnectStateChanged(ProtoTypes::ConnectState)));
    connect(dynamic_cast<QObject*>(backend_), SIGNAL(locationsUpdated()), SLOT(onBackendLocationsUpdated()));

}

BackendCommander::~BackendCommander()
{
}

void BackendCommander::initAndSend()
{
    backend_->basicInit();
}

void BackendCommander::closeBackendConnection()
{
    emit report("Closing backend connection");
    backend_->basicClose();
}

void BackendCommander::onBackendInitFinished(ProtoTypes::InitState state)
{
    if (state == ProtoTypes::INIT_SUCCESS)
    {
        if (backend_->isCanLoginWithAuthHash()) // Logged in
        {
            emit report("Forcing CLI state update");
            backend_->forceCliStateUpdate();
        }
        else
        {
            emit finished(tr("Error: Please login in the GUI before issuing commands"));
        }
    }
    else
    {
        emit finished(tr("Error: Failed to initialize with Engine"));
    }
}

void BackendCommander::onBackendFirewallStateChanged(bool isEnabled)
{
    if (sent_ && (command_ == CLI_COMMAND_FIREWALL_ON || command_ == CLI_COMMAND_FIREWALL_OFF))
    {
        if (isEnabled)
        {
            emit finished(tr("Firewall is ON"));
        }
        else
        {
            emit finished(tr("Firewall is OFF"));
        }
    }
}

void BackendCommander::onBackendConnectStateChanged(ProtoTypes::ConnectState state)
{
    if (sent_ && command_ >= CLI_COMMAND_CONNECT && command_ <= CLI_COMMAND_DISCONNECT)
    {
        if (state.connect_state_type() == ProtoTypes::CONNECTED)
        {
            if (state.has_location())
            {
                LocationID currentLocation = LocationID::createFromProtoBuf(state.location());
                PersistentState::instance().setLastLocation(currentLocation);

                QString locationConnectedTo;
                if (currentLocation.isBestLocation())
                {
                    locationConnectedTo = tr("Best Location");
                }
                else
                {
                    locationConnectedTo = QString::fromStdString(state.location().city());
                }
                emit finished(tr("Connected to ") + locationConnectedTo);
            }
        }
        else if (state.connect_state_type() == ProtoTypes::DISCONNECTED)
        {
            emit finished(tr("Disconnected"));
        }
    }

    receivedStateInit_ = true;
    if (receivedStateInit_ && receivedLocationsInit_)
    {
        sendOneCommand();
    }
}

void BackendCommander::onBackendLocationsUpdated()
{
    qCDebug(LOG_IPC) << "Locations Updated";
    receivedLocationsInit_ = true;
    if (receivedStateInit_ && receivedLocationsInit_)
    {
        sendOneCommand();
    }
}

void BackendCommander::sendOneCommand()
{
    if (!sent_)
    {
        sent_ = true;
        sendCommand();
    }
}

void BackendCommander::sendCommand()
{
    if (command_ == CLI_COMMAND_CONNECT)
    {
        qCDebug(LOG_BASIC) << "Connecting to last";
        const LocationID &id = PersistentState::instance().lastLocation();
        backend_->sendConnect(id);
    }
    else if (command_ == CLI_COMMAND_CONNECT_BEST)
    {
        qCDebug(LOG_BASIC) << "Connecting to best";
        backend_->sendConnect(backend_->getLocationsModel()->getBestLocationId());
    }
    else if (command_ == CLI_COMMAND_CONNECT_LOCATION)
    {
        if (locationStr_ != "")
        {
            LocationID lid = backend_->getLocationsModel()->findLocationByFilter(locationStr_);
            if (lid.isValid())
            {
                qCDebug(LOG_BASIC) << "Connecting to" << lid.getHashString();
                backend_->sendConnect(lid);
            }
            else
            {
                emit finished(tr("Error: Could not find server matching: \"") + locationStr_ + "\"");
            }
        }
        else
        {
            emit finished(tr("Error: Please specify a server region, city or nickname"));
        }
    }
    else if (command_ == CLI_COMMAND_DISCONNECT)
    {
        if (backend_->isDisconnected())
        {
            emit finished(tr("Already Disconnected"));
        }
        else
        {
            backend_->sendDisconnect();
        }
    }
    else if (command_ == CLI_COMMAND_FIREWALL_ON)
    {
        if (backend_->isFirewallAlwaysOn())
        {
            emit finished(tr("Firewall is in Always On mode and cannot be changed"));
        }
        else
        {
            if (backend_->isFirewallEnabled()) // already on
            {
                emit finished(tr("Firewall is already ON"));
            }
            else
            {
                qCDebug(LOG_BASIC) << "Sending firewall on";
                backend_->firewallOn(false);
            }
        }
    }
    else if (command_ == CLI_COMMAND_FIREWALL_OFF)
    {
        if (backend_->isFirewallAlwaysOn())
        {
            emit finished(tr("Firewall is in Always On mode and cannot be changed"));
        }
        else
        {
            if (!backend_->isFirewallEnabled()) // already off
            {
                emit finished(tr("Firewall is already OFF"));
            }
            else
            {
                qCDebug(LOG_BASIC) << "Sending firewall off";
                backend_->firewallOff(false);
            }
        }
    }
    else if (command_ == CLI_COMMAND_LOCATIONS)
    {
        Utils::giveFocusToGui();
        Utils::openGuiLocations();
        emit finished(tr("Viewing Locations..."));
    }
}

