#include "widgetutils_win.h"

#include <Windows.h>
#include "winutils.h"
#include "dpiscalemanager.h"
#include "GraphicResources/independentpixmap.h"
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);

QPixmap *WidgetUtils_win::extractProgramIcon(QString filePath)
{
    HICON icon;
    HINSTANCE instance = NULL;
    icon = ExtractIconA(instance, (LPSTR)filePath.toLocal8Bit().constData(), 0);

    QPixmap *p = NULL;
    if (icon != NULL && (int) icon != 1)
    {
        p = new QPixmap(qt_pixmapFromWinHICON(icon));
    }
    else
    {
        qDebug() << "Failed to call qt_pixmapFromWinHICON: " << filePath << "(" << icon << ")";
    }

    DestroyIcon(icon);

    return p;
}

QPixmap *WidgetUtils_win::extractWindowsAppProgramIcon(QString filePath)
{
    // Get Manifest XML filename -- it contains location of logo
    QDir d = QFileInfo(filePath).absoluteDir();
    QString folderName = d.absolutePath();
    QString manifestName = folderName + "/AppxManifest.xml";

    // read manifest
    QString logoLocation;
    QFile manifestFile(manifestName);
    if (manifestFile.open(QFile::ReadOnly | QFile::Text))
    {
        QXmlStreamReader rXml;

        rXml.setDevice(&manifestFile);
        rXml.readNext();

        while (!rXml.atEnd() && logoLocation == "")
        {
            if (rXml.isStartElement())
            {
                if (rXml.name() == "Package")
                {
                    rXml.readNext();
                }
                else if (rXml.name() == "Properties")
                {
                    while (!rXml.atEnd())
                    {
                        if (rXml.name() == "Logo")
                        {
                            logoLocation = rXml.readElementText();
                            logoLocation = logoLocation.replace("\\", "/");
                            break;
                        }

                        rXml.readNext();
                    }
                }
                else
                {
                    rXml.readNext();
                }
            }
            else
            {
                rXml.readNext();
            }
        }
    }

    // manifest logo filename contains string to find actual filename
    QString manifestLogoFilePath = folderName + "/" + logoLocation;
    QFileInfo manifestLogoFileInfo = QFileInfo(manifestLogoFilePath);
    QDir assetDir(manifestLogoFileInfo.absoluteDir());
    QString logoFilter = manifestLogoFileInfo.completeBaseName(); // filename without extension

    QPixmap *logoPixmap;
    foreach (QFileInfo assetFile, assetDir.entryInfoList(QDir::Files))
    {
        if (assetFile.fileName().contains(logoFilter))
        {
            logoPixmap = new QPixmap(assetFile.absoluteFilePath());
            break;
        }
    }

    return logoPixmap;
}

void WidgetUtils_win::updateSystemTrayIcon(const QPixmap &pixmap, QString tooltip)
{
    // Constants are taken from "qwindowssystemtrayicon.cpp".
    const LPCWSTR kWindowName = L"QTrayIconMessageWindow";
    const UINT q_uNOTIFYICONID = 0;

    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
    HWND trayHwnd = 0;
    while ((trayHwnd = FindWindowEx(0, trayHwnd, nullptr, kWindowName)) != 0) {
        const auto winInstance =
            reinterpret_cast<const HINSTANCE>(GetWindowLongPtr(trayHwnd, GWLP_HINSTANCE));
        if (winInstance == appInstance)
            break;
    }
    if (!trayHwnd)
        return;

    NOTIFYICONDATA tnd;
    memset(&tnd, 0, sizeof(tnd));
    tnd.cbSize = sizeof(tnd);
    tnd.uVersion = NOTIFYICON_VERSION_4;
    tnd.uID = q_uNOTIFYICONID;
    tnd.hWnd = trayHwnd;
    tnd.uFlags = NIF_SHOWTIP;
    if (!pixmap.isNull()) {
        tnd.uFlags |= NIF_ICON;
        tnd.hIcon = qt_pixmapToWinHICON(pixmap);
    }
    if (!tooltip.isEmpty()) {
        tnd.uFlags |= NIF_TIP;
        const int maxsize = sizeof(tnd.szTip) / sizeof(wchar_t);
        const int length = qMin(maxsize - 1, tooltip.size());
        if (length < tooltip.size())
            tooltip.truncate(length);
        tooltip.toWCharArray(tnd.szTip);
        tnd.szTip[length] = wchar_t(0);
    }
    Shell_NotifyIcon(NIM_MODIFY, &tnd);
}

