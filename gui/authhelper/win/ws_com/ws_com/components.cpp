#include "components.h"
#include "guids.h"

HMODULE g_hModule = NULL;
long g_cServerLocks = 0;
long g_cComponents = 0;

CFactoryData g_FactoryData;

// Component
CAuthHelper::CAuthHelper() : m_ref(1)
{ 
	InterlockedIncrement(&g_cComponents); 
}

CAuthHelper::~CAuthHelper()
{ 
	InterlockedDecrement(&g_cComponents); 
	CAuthHelperFactory::CloseExe();
}

HRESULT STDMETHODCALLTYPE CAuthHelper::QueryInterface(REFIID riid, void** ppv)
{
	// std::cout << "Querying interface (component)" << std::endl;
	if (IsEqualIID(riid, IID_IUnknown))
	{
		*ppv = static_cast<IAuthHelper*>(this); // note casting to IUnknown would be unsafe here
	}
	else if (IsEqualIID(riid, IID_AUTH_HELPER))
	{
		*ppv = static_cast<IAuthHelper*>(this);
	}
	else
	{
		// std::cout << "No interface (component)" << std::endl;
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CAuthHelper::AddRef()
{
	// std::cout << "Adding component ref" << std::endl;
	return (ULONG)InterlockedIncrement(&m_ref);
}

ULONG STDMETHODCALLTYPE CAuthHelper::Release()
{
	// std::cout << "Releasing component ref" << std::endl;
	LONG Result = InterlockedDecrement(&m_ref);
	if (Result == 0)
	{
		// std::cout << "Deleting component" << std::endl;
		delete this;
		return 0;
	}
	return m_ref;
}


//
//
// Factory
CAuthHelperFactory::CAuthHelperFactory() : m_ref(1)
{
}

CAuthHelperFactory::~CAuthHelperFactory()
{
}


HRESULT STDMETHODCALLTYPE CAuthHelperFactory::QueryInterface(REFIID riid, void** ppv)
{
	// std::cout << "Querying interface (factory)" << std::endl;
	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
	{
		*ppv = static_cast<IClassFactory*>(this); // note casting to IUnknown would be unsafe here
	}
	else
	{
		// std::cout << "No interface (factory)" << std::endl;
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CAuthHelperFactory::AddRef()
{
	// std::cout << "Adding Factory ref" << std::endl;
	return (ULONG)InterlockedIncrement(&m_ref);
}

ULONG STDMETHODCALLTYPE CAuthHelperFactory::Release()
{
	// std::cout << "Releasing factory ref" << std::endl;
	LONG Result = InterlockedDecrement(&m_ref);
	if (Result == 0)
	{
		// std::cout << "Deleting factory" << std::endl;
		delete this;
		return 0;
	}
	return m_ref;
}

HRESULT STDMETHODCALLTYPE CAuthHelperFactory::CreateInstance(IUnknown *pUnknownOuter, const IID &iid, void **ppv)
{
	// std::cout << "Factory creating instance" << std::endl;
	if (pUnknownOuter != NULL)
	{
		return CLASS_E_NOAGGREGATION;
	}

	CAuthHelper *pMyThing = new CAuthHelper;
	if (pMyThing == NULL)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pMyThing->QueryInterface(iid, ppv);
	pMyThing->Release();
	return hr;
}

HRESULT STDMETHODCALLTYPE CAuthHelperFactory::LockServer(BOOL bLock)
{
	if (bLock)
	{
		InterlockedIncrement(&g_cServerLocks);
	}
	else
	{
		InterlockedDecrement(&g_cServerLocks);
	}
	CloseExe();
	return S_OK;
}

BOOL CAuthHelperFactory::StartFactories()
{
	// std::cout << "Starting factories..." << std::endl;

	g_FactoryData.m_pCLSID = &CLSID_AUTH_HELPER;
	g_FactoryData.m_RegistryName = "My Thing Factory";

	IClassFactory* pIFactory = new CAuthHelperFactory();

	DWORD dwRegister;
	HRESULT hr = ::CoRegisterClassObject(
		*g_FactoryData.m_pCLSID,
		pIFactory,
		CLSCTX_LOCAL_SERVER,
		REGCLS_SINGLEUSE,
		&dwRegister);

	if (FAILED(hr))
	{
		pIFactory->Release();
		return FALSE;
	}
	g_FactoryData.m_pIClassFactory = pIFactory;
	g_FactoryData.m_dwRegister = dwRegister;
	return TRUE;
}

void CAuthHelperFactory::StopFactories()
{
	// std::cout << "Stopping factories..." << std::endl;
	if (g_FactoryData.m_dwRegister != 0)
	{
		::CoRevokeClassObject(g_FactoryData.m_dwRegister);
	}

	IClassFactory *pIFactory = g_FactoryData.m_pIClassFactory;
	if (pIFactory != NULL)
	{
		pIFactory->Release();
	}
}

void CAuthHelperFactory::CloseExe()
{
    //debugOut("CAuthHelperFactory::CloseExe()");
	if (DllCanUnloadNow() == S_OK)
	{
        //debugOut("CAuthHelperFactory::CloseExe() posting quit message");
        if (::PostMessage(NULL, WM_QUIT, 0, 0) == FALSE) {
            //debugOut("CAuthHelperFactory::CloseExe() PostMessage failed (%lu)", ::GetLastError());
        }
	}
}

void debugOut(const char* format, ...)
{
    va_list arg_list;
    va_start(arg_list, format);

    char szMsg[1024];
    _vsnprintf_s(szMsg, 1024, _TRUNCATE, format, arg_list);
    va_end(arg_list);

    ::OutputDebugStringA(szMsg);
}
