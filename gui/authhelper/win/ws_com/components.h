#pragma once

#include <objbase.h>
#include <iostream>

extern HMODULE g_hModule;
extern long g_cServerLocks;
extern long g_cComponents;

extern void debugOut(const char* format, ...);

interface IAuthHelper : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
};

class CAuthHelper : public IAuthHelper
{
public:
    CAuthHelper();
    virtual ~CAuthHelper();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
private:
    LONG m_ref;
};

class CAuthHelperFactory : public IClassFactory
{
public:
    CAuthHelperFactory();
    virtual ~CAuthHelperFactory();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    virtual ULONG STDMETHODCALLTYPE AddRef() override;
    virtual ULONG STDMETHODCALLTYPE Release() override;
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnknownOuter, const IID &iid, void **ppv) override;
    virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL bLock) override;
    static BOOL StartFactories();
    static void StopFactories();
    static void CloseExe();
private:
    LONG m_ref;
};

class CFactoryData
{
public:
    CFactoryData() : m_pIClassFactory(NULL), m_dwRegister(0), m_pCLSID(NULL), m_RegistryName("") {}

    const CLSID *m_pCLSID;
    const char * m_RegistryName;
    IClassFactory *m_pIClassFactory;
    DWORD m_dwRegister;
    BOOL IsClassID(const CLSID &clsid) const
    {
        return (*m_pCLSID == clsid);
    }
};