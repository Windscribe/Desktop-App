#include <windows.h>
#include <shldisp.h>
#include <shlobj.h>
#include <exdisp.h>
#include <atlbase.h>
#include <stdlib.h>
#include "ShellExecuteAsUser.h"

void ShellExecuteAsUser::shellExecuteFromExplorer(PCWSTR pszFile, PCWSTR pszParameters, PCWSTR pszDirectory,
	PCWSTR pszOperation, int nShowCmd)
{
	CComPtr<IShellFolderViewDual> spFolderView;
	getDesktopAutomationObject(IID_PPV_ARGS(&spFolderView));
	CComPtr<IDispatch> spdispShell;
	spFolderView->get_Application(&spdispShell);

	CComQIPtr<IShellDispatch2>(spdispShell)
		->ShellExecute(CComBSTR(pszFile),
			CComVariant(pszParameters ? pszParameters : L""),
			CComVariant(pszDirectory ? pszDirectory : L""),
			CComVariant(pszOperation ? pszOperation : L""),
			CComVariant(nShowCmd));
}

void ShellExecuteAsUser::getDesktopAutomationObject(REFIID riid, void **ppv)
{
	CComPtr<IShellView> spsv;
	findDesktopFolderView(IID_PPV_ARGS(&spsv));
	CComPtr<IDispatch> spdispView;
	spsv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView));
	spdispView->QueryInterface(riid, ppv);
}

void ShellExecuteAsUser::findDesktopFolderView(REFIID riid, void **ppv)
{
	CComPtr<IShellWindows> spShellWindows;
	spShellWindows.CoCreateInstance(CLSID_ShellWindows);

	CComVariant vtLoc(CSIDL_DESKTOP);
	CComVariant vtEmpty;
	long lhwnd;
	CComPtr<IDispatch> spdisp;
	spShellWindows->FindWindowSW(
		&vtLoc, &vtEmpty,
		SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp);

	CComPtr<IShellBrowser> spBrowser;
	CComQIPtr<IServiceProvider>(spdisp)->
		QueryService(SID_STopLevelBrowser,
			IID_PPV_ARGS(&spBrowser));

	CComPtr<IShellView> spView;
	spBrowser->QueryActiveShellView(&spView);

	spView->QueryInterface(riid, ppv);
}