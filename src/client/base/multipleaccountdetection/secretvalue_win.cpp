#include "secretvalue_win.h"

#include <QDataStream>
#include <QIODevice>
#include "utils/log/categories.h"

wchar_t g_szKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\NetControlPort";

SecretValue_win::SecretValue_win() : crypt_(0xFA7234AAF37A31BE)
{
}

void SecretValue_win::setValue(const TEntry &entry)
{
    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND) {
        nError = RegCreateKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS) {
        qCCritical(LOG_BASIC) << "SecretValue::setValue fatal error:" << nError;
        return;
    }

    nError = RegSetValueEx(hKey, L"Version", 0, REG_DWORD, (const BYTE *)&versionForSerialization_, sizeof(versionForSerialization_));
    if (nError != ERROR_SUCCESS) {
        qCCritical(LOG_BASIC) << "SecretValue::setValue RegSetValueEx Version fatal error:" << nError;
        RegCloseKey(hKey);
        return;
    }

    QByteArray buf;
    QDataStream stream(&buf, QIODevice::WriteOnly);
    stream << entry.username_;
    stream << entry.date_;
    stream << entry.userId_;

    QByteArray arr = crypt_.encryptToByteArray(buf);
    nError = RegSetValueEx(hKey, L"State", 0, REG_BINARY, (const BYTE *)arr.data(), arr.size());
    if (nError != ERROR_SUCCESS) {
        qCCritical(LOG_BASIC) << "SecretValue::setValue RegSetValueEx fatal error:" << nError;
    }
    RegCloseKey(hKey);
}

bool SecretValue_win::isExistValue(TEntry &entry)
{
    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, KEY_QUERY_VALUE, &hKey);
    if (nError != ERROR_SUCCESS) {
        return false;
    }

    qint32 version = 0;
    DWORD versionSize = sizeof(version);
    nError = RegQueryValueEx(hKey, L"Version", 0, NULL, (LPBYTE)&version, &versionSize);
    bool hasVersion = (nError == ERROR_SUCCESS);

    unsigned char dwReturn[256];
    DWORD dwBufSize = sizeof(dwReturn);

    nError = RegQueryValueEx(hKey, L"State", 0, NULL, (LPBYTE)dwReturn, &dwBufSize);
    RegCloseKey(hKey);
    if (nError != ERROR_SUCCESS) {
        return false;
    }

    QByteArray arr((const char *)dwReturn, dwBufSize);
    QByteArray buf = crypt_.decryptToByteArray(arr);
    if (buf.isEmpty()) {
        return false;
    }

    if (hasVersion && version == versionForSerialization_) {
        // New format
        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> entry.username_;
        stream >> entry.date_;
        stream >> entry.userId_;
    } else {
        // Legacy format
        entry.username_ = QString::fromUtf8(buf);
        entry.userId_ = QString();
        entry.date_ = QDate();
    }

    return true;
}

void SecretValue_win::removeValue()
{
    RegDeleteKeyEx(HKEY_CURRENT_USER, g_szKeyPath, 0, 0);
}
