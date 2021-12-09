#include "executable_signature_win.h"

#include <WinTrust.h>
#include <SoftPub.h>

#include <codecvt>

#include "executable_signature_defs.h"
#include "executable_signature.h"

#pragma comment(lib, "wintrust")
#pragma comment(lib, "crypt32.lib")

ExecutableSignaturePrivate::ExecutableSignaturePrivate(ExecutableSignature* const q) : q_ptr(q)
{
}

ExecutableSignaturePrivate::~ExecutableSignaturePrivate()
{
}

bool ExecutableSignaturePrivate::verify(const std::wstring& exePath)
{
    if (!verifyEmbeddedSignature(exePath))
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

    BOOL fResult = CryptQueryObject(CERT_QUERY_OBJECT_FILE, exePath.c_str(), CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY,
									0, &dwEncoding, &dwContentType, &dwFormatType, &hStore, &hMsg, NULL);
	if (!fResult)
	{
        q_ptr->setLastError(L"CryptoQueryObject failed: " + std::to_wstring(::GetLastError()));
		return false;
	}

	// Get signer information size.
	fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
	if (!fResult)
	{
        q_ptr->setLastError(L"CryptMsgGetParam(size check) failed: " + std::to_wstring(::GetLastError()));
        goto finish;
	}

    // cppcheck-suppress LocalAllocCalled
	pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);


	// Get Signer Information.
	fResult = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0,	(PVOID)pSignerInfo,	&dwSignerInfo);
	if (!fResult)
	{
        q_ptr->setLastError(L"CryptMsgGetParam failed: " + std::to_wstring(::GetLastError()));
        goto finish;
	}

	// Search for the signer certificate in the temporary  certificate store.
	CertInfo.Issuer = pSignerInfo->Issuer;
	CertInfo.SerialNumber = pSignerInfo->SerialNumber;

	pCertContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,	0, CERT_FIND_SUBJECT_CERT, (PVOID)&CertInfo,NULL);
	if (!pCertContext)
	{
        q_ptr->setLastError(L"CertFindCertificateInStore failed: " + std::to_wstring(::GetLastError()));
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

bool ExecutableSignaturePrivate::verifyEmbeddedSignature(const std::wstring &exePath)
{
	bool isValid = false;

	WINTRUST_FILE_INFO FileData;
	memset(&FileData, 0, sizeof(FileData));
	FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = exePath.c_str();
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
    WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

    if (!isValid) {
        q_ptr->setLastError(L"WinVerifyTrust returned: " + std::to_wstring(lStatus));
    }

	return isValid;
}

bool ExecutableSignaturePrivate::checkWindscribeCertificate(PCCERT_CONTEXT pCertContext)
{
	bool fReturn = false;
	LPTSTR szName = NULL;

    // Get Subject name size.
    DWORD dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
	if (!dwData)
	{
        q_ptr->setLastError(L"CertGetNameString(size check) failed");
        return false;
	}
	
    // cppcheck-suppress LocalAllocCalled
	szName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
	if (!(CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szName, dwData)))
	{
		LocalFree(szName);
        q_ptr->setLastError(L"CertGetNameString failed");
        return false;
	}

    fReturn = (wcscmp(szName, WINDOWS_CERT_SUBJECT_NAME) == 0);

	LocalFree(szName);

	return fReturn;
}
