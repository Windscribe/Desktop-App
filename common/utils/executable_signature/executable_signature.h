#ifndef EXECUTABLE_SIGNATURE_H
#define EXECUTABLE_SIGNATURE_H

#include <string>

class ExecutableSignaturePrivate;

/*

- Define USE_SIGNATURE_CHECK in your project to have this class perform
  signature checking.  Without it, verify will skip the signature check
  and return true.

- If signature checking is enabled (USE_SIGNATURE_CHECK defined):

  - MacOS:
    - Ensure the Developer ID Application certificate from your team's Apple Developer
      account is installed in your keychain.  Also ensure the MACOS_CERT_DEVELOPER_ID
      in executable_signature_defs.h matches this certificate.

  - Windows:
    - Ensure your code signing certificate is in the installer/windows/signing folder
      and is named 'code_signing.pfx'.  Also ensure the WINDOWS_CERT_SUBJECT_NAME
      in executable_signature_defs.h matches this certificate.

  - Linux:
    - You can use tools/create_new_linux_keypair.sh to create a key pair used to sign
      the binaries and verify their signature.  The key pair will be created in the
      common/keys/linux folder.

- ExecutableSignaturePrivate implemented for each platform in:
  - ExecutableSignature_win
  - ExecutableSignature_mac
  - ExecutableSignature_linux

*/

class ExecutableSignature
{
public:
    explicit ExecutableSignature();
    ~ExecutableSignature();

    bool verify(const std::wstring &exePath);

    // TODO: See ticket #614
    bool verifyWithSignCheck(const std::wstring &exePath);

    const std::wstring& lastError() const;
    void setLastError(const std::wstring& error);

private:
    ExecutableSignaturePrivate* d_ptr;
    std::wstring lastError_;
};

#endif // EXECUTABLE_SIGNATURE_H
