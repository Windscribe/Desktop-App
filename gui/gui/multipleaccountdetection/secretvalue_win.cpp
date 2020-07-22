#include "secretvalue_win.h"

#include <QDataStream>
#include <QSettings>
#include "Utils/logger.h"

wchar_t g_szKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\NetControlPort";

SecretValue_win::SecretValue_win() : crypt_(0xFA7234AAF37A31BE)
{
    // upgrade from prev version
    QSettings settings("Windscribe", "Windscribe");
    if (settings.contains("locationData2"))
    {
        TOldEntry oe;
        readOldEntry(oe);
        if (!oe.username_.isEmpty())
        {
            setValue(oe.username_);
        }
        settings.remove("locationData2");
    }
}

void SecretValue_win::setValue(const QString &value)
{
    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, KEY_ALL_ACCESS, &hKey);

    if (nError == ERROR_FILE_NOT_FOUND)
    {
        nError = RegCreateKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,NULL, &hKey, NULL);
    }

    if (nError != ERROR_SUCCESS)
    {
        qCDebug(LOG_BASIC) << "SecretValue::setValue fatal error:" << nError;
    }
    else
    {
        QByteArray arr = crypt_.encryptToByteArray(value);
        nError = RegSetValueEx(hKey, L"State", 0, REG_BINARY, (const BYTE *)arr.data(), arr.size());
        if (nError != ERROR_SUCCESS)
        {
            qCDebug(LOG_BASIC) << "SecretValue::setValue RegSetValueEx fatal error:" << nError;
        }
        RegCloseKey(hKey);
    }
}

bool SecretValue_win::isExistValue(QString &value)
{
    HKEY hKey;
    LONG nError = RegOpenKeyEx(HKEY_CURRENT_USER, g_szKeyPath, NULL, KEY_QUERY_VALUE, &hKey);
    if (nError != ERROR_SUCCESS)
    {
        return false;
    }
    else
    {
        unsigned char dwReturn[256];
        DWORD dwBufSize = sizeof(dwReturn);

        nError = RegQueryValueEx(hKey, L"State", 0, NULL, (LPBYTE)dwReturn, &dwBufSize);
        if(nError == ERROR_SUCCESS)
        {
            QByteArray arr((const char *)dwReturn, dwBufSize);
            value = crypt_.decryptToByteArray(arr);
            RegCloseKey(hKey);
            return true;
        }
        else
        {
            RegCloseKey(hKey);
            return false;
        }
    }
}

void SecretValue_win::removeValue()
{
    RegDeleteKeyEx(HKEY_CURRENT_USER, g_szKeyPath, KEY_WOW64_32KEY, 0);
}

void SecretValue_win::readOldEntry(TOldEntry &entry)
{
    QSettings settings("Windscribe", "Windscribe");
    QByteArray buf = crypt_.decryptToByteArray(settings.value("locationData2").toByteArray());
    if (!buf.isEmpty())
    {
        QDataStream stream(&buf, QIODevice::ReadOnly);
        stream >> entry.username_;
        stream >> entry.date_;
    }
}
