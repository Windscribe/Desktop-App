#include "executable_signature_win.h"

#include <WinTrust.h>
#include <SoftPub.h>
#include <strsafe.h>

#include <codecvt>

#include "executable_signature.h"
#include "executable_signature_defs.h"
#include "utils/wsscopeguard.h"

#pragma comment(lib, "wintrust")
#pragma comment(lib, "crypt32.lib")

ExecutableSignaturePrivate::ExecutableSignaturePrivate(ExecutableSignature* const q) : ExecutableSignaturePrivateBase(q)
{
}

ExecutableSignaturePrivate::~ExecutableSignaturePrivate()
{
}

bool ExecutableSignaturePrivate::verify(const std::string &exePath)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring converted = converter.from_bytes(exePath);
    return verify(converted);
}

bool ExecutableSignaturePrivate::verify(const std::wstring &exePath)
{
    if (!verifyEmbeddedSignature(exePath)) {
        return false;
    }

    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    PCCERT_CONTEXT certContext = NULL;

    auto exitGuard = wsl::wsScopeGuard([&] {
        if (certContext != NULL) CertFreeCertificateContext(certContext);
        if (hStore != NULL) CertCloseStore(hStore, 0);
        if (hMsg != NULL) CryptMsgClose(hMsg);
    });

    // Get message handle and store handle from the signed file.
    DWORD encoding, contentType, formatType;
    BOOL result = CryptQueryObject(CERT_QUERY_OBJECT_FILE, exePath.c_str(), CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY,
                                   0, &encoding, &contentType, &formatType, &hStore, &hMsg, NULL);
    if (!result) {
        lastError_ << "CryptoQueryObject failed: " << ::GetLastError();
        return false;
    }

    // Get signer information size and ensure it is within reasonable bounds.
    DWORD signerInfoSize = 0;
    result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &signerInfoSize);
    if (!result) {
        lastError_ << "CryptMsgGetParam(size check) failed: " << ::GetLastError();
        return false;
    }

    if (signerInfoSize == 0 || signerInfoSize >= STRSAFE_MAX_CCH) {
        lastError_ << "CryptMsgGetParam(size check) returned an out-of-bounds size: " << signerInfoSize;
        return false;
    }

    // Get Signer Information.
    std::unique_ptr<unsigned char> signerInfoBuffer(new unsigned char[signerInfoSize]);
    result = CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, signerInfoBuffer.get(), &signerInfoSize);
    if (!result) {
        lastError_ << "CryptMsgGetParam failed: " << ::GetLastError();
        return false;
    }

    // Search for the signer certificate in the temporary certificate store.
    PCMSG_SIGNER_INFO signerInfo = reinterpret_cast<PCMSG_SIGNER_INFO>(signerInfoBuffer.get());
    CERT_INFO certInfo;
    certInfo.Issuer = signerInfo->Issuer;
    certInfo.SerialNumber = signerInfo->SerialNumber;

    certContext = CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&certInfo, NULL);
    if (!certContext) {
        lastError_ << "CertFindCertificateInStore failed: " << ::GetLastError();
        return false;
    }

    return checkCertificate(certContext);
}

bool ExecutableSignaturePrivate::verifyEmbeddedSignature(const std::wstring &exePath)
{
    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(FileData);
    FileData.pcwszFilePath = exePath.c_str();
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    WINTRUST_DATA WinTrustData;
    memset(&WinTrustData, 0, sizeof(WinTrustData));
    WinTrustData.cbStruct = sizeof(WinTrustData);
    WinTrustData.pPolicyCallbackData = NULL;
    WinTrustData.pSIPClientData = NULL;
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    // Prevent revocation checks over the network as they can take quite some time (~15-20s observed by some customers).
    WinTrustData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    WinTrustData.hWVTStateData = NULL;
    WinTrustData.pwszURLReference = NULL;
    WinTrustData.dwUIContext = 0;
    WinTrustData.pFile = &FileData;

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG lStatus = WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

    if (lStatus != ERROR_SUCCESS) {
        lastError_ << "WinVerifyTrust returned: " << lStatus;
        return false;
    }

    return true;
}

bool ExecutableSignaturePrivate::checkCertificate(PCCERT_CONTEXT certContext)
{
    // Get Subject name size.  If the specified name type is not found, returns a null-terminated empty string with a returned character count of 1.
    DWORD nameLen = CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
    if (nameLen <= 1) {
        lastError_ << "CertGetNameString(size check) failed";
        return false;
    }

    std::unique_ptr<wchar_t> nameBuf(new wchar_t[nameLen]);
    DWORD charCount = CertGetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, nameBuf.get(), nameLen);
    if (charCount <= 1) {
        lastError_ << "CertGetNameString failed";
        return false;
    }

    int result = wcsncmp(nameBuf.get(), WINDOWS_CERT_SUBJECT_NAME, charCount);
    return result == 0;
}
