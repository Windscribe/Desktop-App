#include "widgetutils_win.h"

#include <QDir>
#include <QPainter>
#include <QScopeGuard>
#include <QXmlStreamReader>

#include <Windows.h>
#include <Objbase.h>
#include <shellapi.h>
#include <ShObjIdl_core.h>

#include "dpiscalemanager.h"
#include "utils/log/categories.h"

Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);

QPixmap WidgetUtils_win::extractProgramIcon(QString filePath)
{
    HINSTANCE instance = NULL;
    HICON icon = ExtractIcon(instance, filePath.toStdWString().c_str(), 0);

    QPixmap p;
    if (icon != NULL && (__int64)icon != 1) {
        p = QPixmap(qt_pixmapFromWinHICON(icon));
    }
    else {
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
                if (rXml.name() == QStringLiteral("Package"))
                {
                    rXml.readNext();
                }
                else if (rXml.name() == QStringLiteral("Properties"))
                {
                    while (!rXml.atEnd())
                    {
                        if (rXml.name() == QStringLiteral("Logo"))
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

void WidgetUtils_win::setTaskbarIconOverlay(const QWidget &appMainWindow, const QIcon *icon)
{
    ITaskbarList4* taskbarInterface = nullptr;
    HICON iconHandle = nullptr; // A null icon handle will clear the icon overlay

    auto comRelease = qScopeGuard([&] {
        // Free converted icon, since ITaskbarList::SetOverlayIcon creates a copy.
        if (iconHandle) {
            ::DestroyIcon(iconHandle);
        }

        if (taskbarInterface) {
            taskbarInterface->Release();
        }
    });


    HRESULT result = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList4, reinterpret_cast<void **>(&taskbarInterface));
    if (FAILED(result)) {
        qCCritical(LOG_BASIC) << "WidgetUtils_win::setTaskbarIconOverlay() could not create an IID_ITaskbarList4 instance" << HRESULT_CODE(result);
        return;
    }

    result = taskbarInterface->HrInit();
    if (FAILED(result)) {
        qCCritical(LOG_BASIC) << "WidgetUtils_win::setTaskbarIconOverlay() IID_ITaskbarList4::HrInit failed" << HRESULT_CODE(result);
        return;
    }

    if (icon) {
        // Note: we will clear the current overlay icon, if any, if we cannot obtain a native handle to the specified icon overlay.
        auto nativeIconSize = ::GetSystemMetrics(SM_CXSMICON);
        iconHandle = icon->pixmap(nativeIconSize).toImage().toHICON();
    }

    HWND hwnd = reinterpret_cast<HWND>(appMainWindow.winId());
    result = taskbarInterface->SetOverlayIcon(hwnd, iconHandle, L"");
    if (FAILED(result)) {
        qCCritical(LOG_BASIC) << "WidgetUtils_win::setTaskbarIconOverlay() IID_ITaskbarList4::SetOverlayIcon failed" << HRESULT_CODE(result);
    }
}

QRect WidgetUtils_win::availableGeometry(const QWidget &appMainWindow, const QScreen &screen)
{
    // QScreen::availableGeometry() has a bug, as of Qt 6.7.2 and older, where it does not report
    // changes in taskbar size on Windows 10.  The following logic is extracted from the monitorData()
    // method in Qt's qtbase\src\plugins\platforms\windows\qwindowsscreen.cpp source.

    HWND hwnd = reinterpret_cast<HWND>(appMainWindow.winId());
    HMONITOR hMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (hMonitor == NULL) {
        qCCritical(LOG_BASIC) << "WidgetUtils_win::availableGeometry() MonitorFromWindow failed" << ::GetLastError();
        return screen.availableGeometry();
    }

    MONITORINFO minfo;
    memset(&minfo, 0, sizeof(MONITORINFO));
    minfo.cbSize = sizeof(MONITORINFO);
    BOOL result = ::GetMonitorInfo(hMonitor, &minfo);
    if (!result) {
        qCCritical(LOG_BASIC) << "WidgetUtils_win::availableGeometry() GetMonitorInfo failed" << ::GetLastError();
        return screen.availableGeometry();
    }

    return QRect(minfo.rcWork.left,
                 minfo.rcWork.top,
                 (minfo.rcWork.right - minfo.rcWork.left)/DpiScaleManager::instance().curDevicePixelRatio(),
                 (minfo.rcWork.bottom - minfo.rcWork.top)/DpiScaleManager::instance().curDevicePixelRatio());
}
