#include "executable_signature_defs.h"
#include "executable_signature_win.h"
#include <tlhelp32.h>
#include <psapi.h>

#ifdef QT_CORE_LIB
#include <QCoreApplication>
#include <QDir>
#endif

#pragma comment (lib, "wintrust")
#pragma comment(lib, "crypt32.lib")

bool ExecutableSignature_win::verify(const wchar_t *szExePath)
{
	if (!verifyEmbeddedSignature(szExePath))
	{
		return false;
	}

	// Get message handle and store handle from the signed file.
	bool isValid = false;
	HCERTSTORE hStore = NULL;
	HCRYPTMSG hMsg = NULL;
	PCCERT_CONTEXT pCertContext = NULL;
	DWORD dwSignerInfo;
	PCMSG_SIGNER_INFO pSignerInfo = NULL;
	DWORD dwEncoding, dwContentType, dwFormatType;
	CERT_INFO CertInfo;

	BOOL fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE, szExePath, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY,
									0, &dwEncoding, &dwContentType, &dwFormatType, &hStore, &hMsg, NULL);
	if (!fResult)
	{
		return false;
	}

	// Get signer information size.
	fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
	if (!fResult)
	{
		goto finish;
	}

    // cppcheck-suppress LocalAllocCalled
	pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);


	// Get Signer Information.
	fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0,	(PVOID)pSignerInfo,	&dwSignerInfo);
	if (!fResult)
	{
		goto finish;
	}

	// Search for the signer certificate in the temporary  certificate store.
	CertInfo.Issuer = pSignerInfo->Issuer;
	CertInfo.SerialNumber = pSignerInfo->SerialNumber;

	pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,	0, CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo,NULL);
	if (!pCertContext)
	{
		goto finish;
	}

	isValid = checkWindscribeCertificate(pCertContext);

finish:
	if (pSignerInfo != NULL) LocalFree(pSignerInfo);
	if (pCertContext != NULL) CertFreeCertificateContext(pCertContext);
	if (hStore != NULL) CertCloseStore(hStore, 0);
	if (hMsg != NULL) CryptMsgClose(hMsg);

	return isValid;
}

bool ExecutableSignature_win::verifyEmbeddedSignature(const wchar_t *pwszSourceFile)
{
	bool isValid = false;

	WINTRUST_FILE_INFO FileData;
	memset(&FileData, 0, sizeof(FileData));
	FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	FileData.pcwszFilePath = pwszSourceFile;
	FileData.hFile = NULL;
	FileData.pgKnownSubject = NULL;

	GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_DATA WinTrustData;

	memset(&WinTrustData, 0, sizeof(WinTrustData));
	WinTrustData.cbStruct = sizeof(WinTrustData);
	WinTrustData.pPolicyCallbackData = NULL;
	WinTrustData.pSIPClientData = NULL;
	WinTrustData.dwUIChoice = WTD_UI_NONE;
	WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
	WinTrustData.hWVTStateData = NULL;
	WinTrustData.pwszURLReference = NULL;
	WinTrustData.dwUIContext = 0;
	WinTrustData.pFile = &FileData;

	LONG lStatus = WinVerifyTrust(NULL, &WVTPolicyGUID,	&WinTrustData);

	isValid = (lStatus == ERROR_SUCCESS);

	// Any hWVTStateData must be released by a call with close.
	WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
	lStatus = WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

	return isValid;
}

bool ExecutableSignature_win::checkWindscribeCertificate(PCCERT_CONTEXT pCertContext)
{
	bool fReturn = false;
	LPTSTR szName = NULL;

	// Get Subject name size.
    DWORD dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
	if (!dwData)
	{
		return false;
	}
	
    // cppcheck-suppress LocalAllocCalled
	szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
	if (!(CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szName, dwData)))
	{
		LocalFree(szName);
		return false;
	}

    fReturn = (wcscmp(szName, WINDOWS_CERT_SUBJECT_NAME) == 0);

	LocalFree(szName);

	return fReturn;
}

#ifdef QT_CORE_LIB

bool ExecutableSignature_win::isParentProcessGui()
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe32;
    DWORD ppid = 0, pid = GetCurrentProcessId();

    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    ZeroMemory( &pe32, sizeof( pe32 ) );
    pe32.dwSize = sizeof( pe32 );
    if( !Process32First( hSnapshot, &pe32 ) )
    {
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        if( pe32.th32ProcessID == pid )
        {
            ppid = pe32.th32ParentProcessID;
            break;
        }
    } while( Process32Next( hSnapshot, &pe32 ) );

    CloseHandle( hSnapshot );

    if (ppid == 0)
    {
        return false;
    }

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ppid);
    if (processHandle == NULL)
    {
        return false;
    }

    wchar_t filename[MAX_PATH];
    if (GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH) == 0)
    {
        CloseHandle(processHandle);
        return false;
    }
    CloseHandle(processHandle);

    QString parentPath = QString::fromStdWString(filename);
    QString guiPath = QCoreApplication::applicationDirPath() + "/Windscribe.exe";
    guiPath = QDir::toNativeSeparators(QDir::cleanPath(guiPath));

    return (parentPath.compare(guiPath, Qt::CaseInsensitive) == 0) && verify(parentPath);
}

bool ExecutableSignature_win::verify(const QString &executablePath)
{
    return verify(executablePath.toStdWString().c_str());
}

#endif
