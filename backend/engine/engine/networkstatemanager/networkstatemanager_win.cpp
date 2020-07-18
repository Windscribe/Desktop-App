#include "networkstatemanager_win.h"
#include <QMutex>
#include <windows.h>
#include <wininet.h>
#include "Utils/logger.h"

//NetworkStateManager_win *g_NetworkStateManager = NULL;

//class CNetworkListManagerEvent : public INetworkListManagerEvents
//{
//public:
//    CNetworkListManagerEvent() : m_ref(0)
//    {
//    }

//    virtual ~CNetworkListManagerEvent()
//    {
//    }

//    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
//    {
//        if (IsEqualIID(riid, IID_IUnknown))
//        {
//            *ppvObject = (IUnknown *)this;
//        }
//        else if (IsEqualIID(riid ,IID_INetworkListManagerEvents))
//        {
//            *ppvObject = (INetworkListManagerEvents *)this;
//        }
//        else
//        {
//            return E_NOINTERFACE;
//        }

//        ((IUnknown*)*ppvObject)->AddRef();

//        return S_OK;
//    }

//    ULONG STDMETHODCALLTYPE AddRef()
//    {
//        return (ULONG)InterlockedIncrement(&m_ref);
//    }

//    ULONG STDMETHODCALLTYPE Release()
//    {
//        LONG Result = InterlockedDecrement(&m_ref);
//        if (Result == 0)
//            delete this;
//        return (ULONG)Result;
//    }

//    virtual HRESULT STDMETHODCALLTYPE ConnectivityChanged(NLM_CONNECTIVITY newConnectivity)
//    {
//        g_NetworkStateManager->onlineStateChanged(newConnectivity != NLM_CONNECTIVITY_DISCONNECTED);
//        return S_OK;
//    }

//private:

//    LONG m_ref;
//};

//CNetworkListManagerEvent *g_netEvent = NULL;

// -----------------------------------------------------------------------------------------------
NetworkStateManager_win::NetworkStateManager_win(QObject *parent, INetworkDetectionManager *networkDetectionManager) : INetworkStateManager(parent)
  , pNetworkDetectionManager_(networkDetectionManager)
  //, pNetworkListManager_(NULL), pConnectPoint_(NULL)
  , bIsOnline_(true), mutex_(QMutex::Recursive)
{
//    CoInitializeEx(NULL, COINIT_MULTITHREADED);
//    Q_ASSERT(g_NetworkStateManager == NULL);
//    g_NetworkStateManager = this;
//    g_netEvent = NULL;
//    HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager,
//                        (LPVOID *)&pNetworkListManager_);
//    if (hr != S_OK)
//    {
//        pNetworkListManager_ = NULL;
//    }
//    else
//    {
//        IConnectionPointContainer *pCPContainer = NULL;
//        hr = pNetworkListManager_->QueryInterface(IID_IConnectionPointContainer, (void **)&pCPContainer);
//        if (SUCCEEDED(hr))
//        {
//            hr = pCPContainer->FindConnectionPoint(IID_INetworkListManagerEvents, &pConnectPoint_);
//            if (SUCCEEDED(hr))
//            {
//                cookie_ = NULL;
//                g_netEvent = new CNetworkListManagerEvent;
//                hr = pConnectPoint_->Advise((IUnknown *)g_netEvent, &cookie_);
//                if (!SUCCEEDED(hr))
//                {
//                    pConnectPoint_->Release();
//                    pConnectPoint_ = NULL;
//                    g_netEvent = NULL;
//                }
//            }
//            pCPContainer->Release();
//        }
//    }

//    bIsOnline_ = isOnline();

    connect(pNetworkDetectionManager_, SIGNAL(networkChanged(ProtoTypes::NetworkInterface)), SLOT(onDetectionManagerNetworkChanged(ProtoTypes::NetworkInterface)));
    bIsOnline_ = pNetworkDetectionManager_->isOnline();
}

NetworkStateManager_win::~NetworkStateManager_win()
{
//    if (pConnectPoint_)
//    {
//        pConnectPoint_->Unadvise(cookie_);
//        pConnectPoint_->Release();
//        g_netEvent = NULL;
//    }

//    if (pNetworkListManager_)
//    {
//        pNetworkListManager_->Release();
//    }
//    g_NetworkStateManager = NULL;
}

bool NetworkStateManager_win::isOnline()
{
    //qDebug() << "Checking isOnline";
    QMutexLocker locker(&mutex_);
    return bIsOnline_;

   // qDebug() << "Past Mutex";
//    if (pNetworkListManager_ != NULL)
//    {
//        NLM_CONNECTIVITY status;
//        if (pNetworkListManager_->GetConnectivity(&status) == S_OK)
//        {
//            //qDebug() << "Connected: " << (status != NLM_CONNECTIVITY_DISCONNECTED);
//            return status != NLM_CONNECTIVITY_DISCONNECTED;
//        }
//        else
//        {
//            //qDebug() << "Fake True";
//            return true;
//        }
//    }
//    else
//    {
//        DWORD dw;
//        if (InternetGetConnectedState(&dw, 0))
//        {
//            return (dw & INTERNET_CONNECTION_LAN) || (dw & INTERNET_CONNECTION_MODEM) ||
//                   (dw & INTERNET_CONNECTION_PROXY);
//        }
//        else
//        {
//            return true;
//        }
//    }
}

void NetworkStateManager_win::onDetectionManagerNetworkChanged(ProtoTypes::NetworkInterface networkInterface)
{
    QMutexLocker locker(&mutex_);

    bIsOnline_ = pNetworkDetectionManager_->isOnline();
    qCDebug(LOG_BASIC) << "Network connectivity changed:" << bIsOnline_;
    emit stateChanged(bIsOnline_, "");
}

//void NetworkStateManager_win::onlineStateChanged(bool connectivity)
//{
//    QMutexLocker locker(&mutex_);
//    if (bIsOnline_ != connectivity)
//    {
//        bIsOnline_ = connectivity;
//        qCDebug(LOG_BASIC) << "Network connectivity changed:" << bIsOnline_;
//        emit stateChanged(bIsOnline_, "");
//    }
//}
