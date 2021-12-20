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

QPixmap WidgetUtils_win::extractProgramIcon(QString filePath)
{
    HICON icon;
    HINSTANCE instance = NULL;
    icon = ExtractIconA(instance, (LPSTR)filePath.toLocal8Bit().constData(), 0);

    QPixmap p;
    if (icon != NULL && (int) icon != 1)
    {
        p = QPixmap(qt_pixmapFromWinHICON(icon));
    }
    else
    {
        // qDebug() << "Failed to call qt_pixmapFromWinHICON: " << filePath << "(" << icon << ")";
    }

    DestroyIcon(icon);

    return p;
}

QPixmap WidgetUtils_win::extractWindowsAppProgramIcon(QString filePath)
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
    QString logoScaledFilter = logoFilter + ".scale-";

    QString logoFilePath, logoFilePathScaled;
    const auto assetFileList = assetDir.entryInfoList(QDir::Files);
    for (const QFileInfo &assetFile : assetFileList)
    {
        if (assetFile.fileName().contains(logoScaledFilter) &&
            !assetFile.fileName().contains("contrast-")) {
            logoFilePathScaled = assetFile.absoluteFilePath();
            break;
        } else if (assetFile.fileName().contains(logoFilter)) {
            if (logoFilePath.isEmpty())
                logoFilePath = assetFile.absoluteFilePath();
        }
    }

    QPixmap logoPixmap;
    if (!logoFilePathScaled.isEmpty())
        logoPixmap = QPixmap(logoFilePathScaled);
    else if (!logoFilePath.isEmpty())
        logoPixmap = QPixmap(logoFilePath);

    if (!logoPixmap.isNull()) {
        // Fill transparent background with a WindowsApps background color.
        // This should also be parsed from the manifest, but apparently it is always the same.
        QPixmap filledPixmap(logoPixmap.size());
        filledPixmap.fill(QColor("#0078D4"));
        QPainter painter(&filledPixmap);
        painter.drawPixmap(0, 0, logoPixmap);
        painter.end();
        logoPixmap = filledPixmap;
    }

    return logoPixmap;
}

namespace
{
// Constants are taken from "qwindowssystemtrayicon.cpp".
const LPCWSTR kWindowName = L"QTrayIconMessageWindow";
const UINT q_uNOTIFYICONID = 0;
const UINT MYWM_NOTIFYICON = WM_APP + 101;

HWND WidgetUtils_win_GetSystemTrayIconHandle()
{
    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
    HWND trayHwnd = 0;
    while ((trayHwnd = FindWindowEx(0, trayHwnd, nullptr, kWindowName)) != 0) {
        const auto winInstance =
            reinterpret_cast<const HINSTANCE>(GetWindowLongPtr(trayHwnd, GWLP_HINSTANCE));
        if (winInstance == appInstance)
            break;
    }
    return trayHwnd;
}
}  // namespace

void WidgetUtils_win::updateSystemTrayIcon(const QPixmap &pixmap, QString tooltip)
{
    HWND trayHwnd = WidgetUtils_win_GetSystemTrayIconHandle();
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

void WidgetUtils_win::fixSystemTrayIconDblClick()
{
    // Fix Qt bug skipping a single click after handling a double click.
    // Apparently this is a wontfix on the Qt side: https://bugreports.qt.io/browse/QTBUG-54850
    HWND trayHwnd = WidgetUtils_win_GetSystemTrayIconHandle();
    if (trayHwnd)
        SendMessage(trayHwnd, MYWM_NOTIFYICON, 0, MAKELPARAM(NIN_SELECT, q_uNOTIFYICONID));
}

