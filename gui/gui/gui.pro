#-------------------------------------------------
#
# Project created by QtCreator 2019-01-26T04:48:17
#
#-------------------------------------------------

QT       += core gui network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Windscribe
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += WINDSCRIBE_GUI

COMMON_PATH = $$PWD/../../common

INCLUDEPATH += $$COMMON_PATH

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG(release, debug|release) {
    DEFINES += WINDSCRIBE_EMBEDDED_ENGINE
}

win32 {
    CONFIG(release, debug|release){
        INCLUDEPATH += c:/libs/protobuf_release/include
        LIBS += -Lc:/libs/protobuf_release/lib -llibprotobuf
    }
    CONFIG(debug, debug|release){
        INCLUDEPATH += c:/libs/protobuf_debug/include
        LIBS += -Lc:/libs/protobuf_debug/lib -llibprotobufd
    }

    LIBS += -luser32
    LIBS += -lAdvapi32
    LIBS += -lIphlpapi

    RC_FILE = gui.rc

    # Supress protobuf linker warnings
    QMAKE_LFLAGS += /IGNORE:4099

    # Generate debug information (symbol files) for Windows
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_CXXFLAGS += /Zi
    QMAKE_LFLAGS += /DEBUG

SOURCES += multipleaccountdetection/multipleaccountdetection_win.cpp \
           multipleaccountdetection/secretvalue_win.cpp \
           $$COMMON_PATH/utils/crashdump.cpp \
           $$COMMON_PATH/utils/crashhandler.cpp \
           $$COMMON_PATH/utils/winutils.cpp \
           $$COMMON_PATH/utils/widgetutils_win.cpp \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.cpp \
           utils/scaleutils_win.cpp \
           launchonstartup/launchonstartup_win.cpp \
           application/preventmultipleinstances_win.cpp



HEADERS += multipleaccountdetection/multipleaccountdetection_win.h \
           multipleaccountdetection/secretvalue_win.h \
           $$COMMON_PATH/utils/crashdump.h \
           $$COMMON_PATH/utils/crashhandler.h \
           $$COMMON_PATH/utils/winutils.h \
           $$COMMON_PATH/utils/widgetutils_win.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_win.h \
           utils/scaleutils_win.h \
           launchonstartup/launchonstartup_win.h \
           application/preventmultipleinstances_win.h

}


macx {

LIBS += -framework Foundation
LIBS += -framework AppKit
LIBS += -framework CoreFoundation
LIBS += -framework CoreServices
LIBS += -framework Security
LIBS += -framework SystemConfiguration
LIBS += -framework ServiceManagement

INCLUDEPATH += $$(HOME)/LibsWindscribe/protobuf/include
LIBS += -L$$(HOME)/LibsWindscribe/protobuf/lib -lprotobuf

SOURCES += multipleaccountdetection/multipleaccountdetection_mac.cpp

#remove unused parameter warnings
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

OBJECTIVE_SOURCES += \
                $$COMMON_PATH//utils/macutils.mm \
                application/checkrunningapp/checkrunningapp_mac.mm \
                application/openlocationshandler_mac.mm \
                launchonstartup/launchonstartup_mac.mm \
                utils/interfaceutils_mac.mm \
                $$COMMON_PATH/exithandler_mac.mm \
                $$COMMON_PATH/utils/widgetutils_mac.mm \
                $$COMMON_PATH/utils/executable_signature/executable_signature_mac.mm

HEADERS += \
           $$COMMON_PATH//utils/macutils.h \
           multipleaccountdetection/multipleaccountdetection_mac.h \
           application/checkrunningapp/checkrunningapp_mac.h \
           application/openlocationshandler_mac.h \
           launchonstartup/launchonstartup_mac.h \
           utils/interfaceutils_mac.h \
           $$COMMON_PATH/exithandler_mac.h \
           $$COMMON_PATH/utils/widgetutils_mac.h \
           $$COMMON_PATH/utils/executable_signature/executable_signature_mac.h

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.11
ICON = windscribe.icns
QMAKE_INFO_PLIST = info.plist

#copy WindscribeLauncher.app to Windscribe.app/Contents/Library/LoginItems folder
makedir.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems
copydata.commands = $(COPY_DIR) $$PWD/../../../client-desktop-installer/mac/binaries/launcher/WindscribeLauncher.app $$OUT_PWD/Windscribe.app/Contents/Library/LoginItems

first.depends = $(first) makedir copydata
export(first.depends)
export(makedir.commands)
export(copydata.commands)
#export(makedir4.commands)
QMAKE_EXTRA_TARGETS += first makedir copydata #makedir4 copydata4

# only for release build
# comment CONFIG... line if need embedded engine and cli for debug purposes
CONFIG(release, debug|release) {

    #copy WindscribeEngine.app to Windscribe.app/Contents/Library folder
    copydata3.commands = $(COPY_DIR) $$PWD/../../../client-desktop-installer/mac/binaries/engine/WindscribeEngine.app $$OUT_PWD/Windscribe.app/Contents/Library
    first.depends += copydata3
    export(copydata3.commands)
    QMAKE_EXTRA_TARGETS += copydata3

    # package cli inside Windscribe.app/Contents/MacOS
    makedir4.commands = $(MKDIR) $$OUT_PWD/Windscribe.app/Contents/MacOS
    first.depends += makedir4
    export(makedir4.commands)
    QMAKE_EXTRA_TARGETS += makedir4
    copydata4.commands = $(COPY_FILE) $$PWD/../../../client-desktop-installer/mac/binaries/cli/windscribe-cli $$OUT_PWD/Windscribe.app/Contents/MacOS/windscribe-cli
    first.depends += copydata4
    export(copydata4.commands)
    QMAKE_EXTRA_TARGETS += copydata4
}

RESOURCES += \
    windscribe_mac.qrc

} # macx


SOURCES += \
    ../backend/backend.cpp \
    ../backend/connectstatehelper.cpp \
    ../backend/firewallstatehelper.cpp \
    ../backend/notificationscontroller.cpp \
    $$COMMON_PATH/ipc/generated_proto/types.pb.cc \
    ../backend/locationsmodel/alllocationsmodel.cpp \
    ../backend/locationsmodel/basiccitiesmodel.cpp \
    ../backend/locationsmodel/basiclocationsmodel.cpp \
    ../backend/locationsmodel/configuredcitiesmodel.cpp \
    ../backend/locationsmodel/favoritecitiesmodel.cpp \
    ../backend/locationsmodel/favoritelocationsstorage.cpp \
    ../backend/locationsmodel/locationsmodel.cpp \
    ../backend/locationsmodel/sortlocationsalgorithms.cpp \
    ../backend/locationsmodel/staticipscitiesmodel.cpp \
    ../backend/preferences/accountinfo.cpp \
    ../backend/preferences/detectlanrange.cpp \
    ../backend/preferences/guisettingsfromver1.cpp \
    ../backend/preferences/preferences.cpp \
    ../backend/preferences/preferenceshelper.cpp \
    $$COMMON_PATH/types/locationid.cpp \
    ../backend/types/pingtime.cpp \
    ../backend/types/types.cpp \
    ../backend/types/upgrademodetype.cpp \
    $$COMMON_PATH/utils/extraconfig.cpp \
    $$COMMON_PATH/utils/languagesutil.cpp \
    $$COMMON_PATH/utils/logger.cpp \
    $$COMMON_PATH/utils/mergelog.cpp \
    $$COMMON_PATH/utils/utils.cpp \
    $$COMMON_PATH/utils/widgetutils.cpp \
    $$COMMON_PATH/utils/executable_signature/executable_signature.cpp \
    $$COMMON_PATH/version/appversion.cpp \
    ../backend/persistentstate.cpp \
    application/windowsnativeeventfilter.cpp \
    application/windscribeapplication.cpp \
    commongraphics/imageitem.cpp \
    commongraphics/scalablegraphicsobject.cpp \
    commonwidgets/combomenuwidget.cpp \
    commonwidgets/combomenuwidgetbutton.cpp \
    commonwidgets/iconwidget.cpp \
    commonwidgets/lightwidget.cpp \
    connectwindow/connectionbadgedots.cpp \
    connectwindow/connectstateprotocolport.cpp \
    connectwindow/ipaddressitem/ipaddressitem.cpp \
    connectwindow/ipaddressitem/numberitem.cpp \
    connectwindow/ipaddressitem/numberspixmap.cpp \
    connectwindow/ipaddressitem/octetitem.cpp \
    connectwindow/middleitem.cpp \
    connectwindow/serverratingindicator.cpp \
    dialogs/advancedparametersdialog.cpp \
    dialogs/dialoggetusernamepassword.cpp \
    dialogs/dialogmessagecpuusage.cpp \
    graphicresources/fontdescr.cpp \
    graphicresources/iconmanager.cpp \
    graphicresources/independentpixmap.cpp \
    launchonstartup/launchonstartup.cpp \
    locationswindow/widgetlocations/itemwidgetcity.cpp \
    locationswindow/widgetlocations/itemwidgetheader.cpp \
    locationswindow/widgetlocations/itemwidgetregion.cpp \
    locationswindow/widgetlocations/scrollbar.cpp \
    locationswindow/widgetlocations/widgetcitieslist.cpp \
    locationswindow/widgetlocations/widgetlocations.cpp \
    locationswindow/widgetlocations/widgetlocationslist.cpp \
    preferenceswindow/connectionwindow/packetsizeeditboxitem.cpp \
    overlaysconnectwindow//upgradewindowitem.cpp \
    overlaysconnectwindow/updatewindowitem.cpp \
    overlaysconnectwindow/generalmessagetwobuttonwindowitem.cpp \
    overlaysconnectwindow/generalmessagewindowitem.cpp \
    overlaysconnectwindow/updateappitem.cpp \
    preferenceswindow/debugwindow/apiresolutionitem.cpp \
    preferenceswindow/generalwindow/versioninfoitem.cpp \
    preferenceswindow/networkwhitelistwindow/currentnetworkitem.cpp \
    preferenceswindow/networkwhitelistwindow/networklistitem.cpp \
    preferenceswindow/networkwhitelistwindow/networkwhitelistshared.cpp \
    preferenceswindow/splittunnelingwindow/appincludeditem.cpp \
    preferenceswindow/splittunnelingwindow/appsearchitem.cpp \
    preferenceswindow/splittunnelingwindow/iporhostnameitem.cpp \
    preferenceswindow/splittunnelingwindow/newiporhostnameitem.cpp \
    preferenceswindow/splittunnelingwindow/searchlineedititem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingappsitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingappssearchitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingappssearchwindowitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingappswindowitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingiphostnamewindowitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingipsandhostnamesitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingswitchitem.cpp \
    tooltips/itooltip.cpp \
    tooltips/serverratingstooltip.cpp \
    tooltips/tooltipbasic.cpp \
    tooltips/tooltipcontroller.cpp \
    tooltips/tooltipdescriptive.cpp \
    tooltips/tooltiputil.cpp \
    utils/ipvalidation.cpp \
    utils/protoenumtostring.cpp \
    commonwidgets/custommenuwidget.cpp \
    commonwidgets/customtexteditwidget.cpp \
    commonwidgets/scrollareawidget.cpp \
    commonwidgets/verticalscrollbarwidget.cpp \
    loginwindow/initwindowitem.cpp \
    log/logviewerwindow.cpp \
    preferenceswindow/debugwindow/advancedparameterswindowitem.cpp \
    utils/hardcodedsettings.cpp \
    utils/simplecrypt.cpp \
    utils/writeaccessrightschecker.cpp \
    blockconnect.cpp \
    dpiscaleawarewidget.cpp \
    dpiscalemanager.cpp \
    freetrafficnotificationcontroller.cpp \
    guitest.cpp \
    systemtray/locationstraymenubutton.cpp \
    systemtray/locationstraymenuitemdelegate.cpp \
    systemtray/locationstraymenuwidget.cpp \
    systemtray/locationstraymenunative.cpp \
    loginattemptscontroller.cpp \
        main.cpp \
        mainwindow.cpp \
    loginwindow/loginwindowitem.cpp \
    loginwindow/welcomewindowitem.cpp \
    loginwindow/credentialswindowitem.cpp \
    loginwindow/logginginwindowitem.cpp \
    loginwindow/loginyesnobutton.cpp \
    loginwindow/usernamepasswordentry.cpp \
    loginwindow/loginbutton.cpp \
    loginwindow/iconhoverengagebutton.cpp \
    loginwindow/firewallturnoffbutton.cpp \
    graphicresources/fontmanager.cpp \
    graphicresources/imageresourcessvg.cpp \
    graphicresources/imageresourcesjpg.cpp \
    emergencyconnectwindow/emergencyconnectwindowitem.cpp \
    emergencyconnectwindow/textlinkbutton.cpp \
    commongraphics/commongraphics.cpp \
    connectwindow/connectwindowitem.cpp \
    locationswindow/locationswindow.cpp \
    locationswindow/widgetlocations/backgroundpixmapanimation.cpp \
    locationswindow/widgetlocations/cursorupdatehelper.cpp \
    locationswindow/widgetlocations/itemtimems.cpp \
    locationswindow/widgetlocations/locationstab.cpp \
    locationswindow/widgetlocations/widgetcities.cpp \
    locationswindow/widgetlocations/widgetlocationssizes.cpp \
    $$COMMON_PATH/ipc/commandfactory.cpp \
    $$COMMON_PATH/ipc/connection.cpp \
    $$COMMON_PATH/ipc/server.cpp \
    $$COMMON_PATH/ipc/tcpconnection.cpp \
    $$COMMON_PATH/ipc/tcpserver.cpp \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.cc \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.cc \
    connectwindow/background.cpp \
    connectwindow/connectbutton.cpp \
    connectwindow/locationsbutton.cpp \
    connectwindow/wifiname.cpp \
    connectwindow/firewallbutton.cpp \
    utils/makecustomshadow.cpp \
    utils/shadowmanager.cpp \
    preferenceswindow/preferenceswindowitem.cpp \
    preferenceswindow/bottomresizeitem.cpp \
    preferenceswindow/generalwindow/generalwindowitem.cpp \
    preferenceswindow/scrollareaitem.cpp \
    preferenceswindow/accountwindow/accountwindowitem.cpp \
    preferenceswindow/connectionwindow/connectionwindowitem.cpp \
    preferenceswindow/debugwindow//debugwindowitem.cpp \
    preferenceswindow/sharewindow/sharewindowitem.cpp \
    preferenceswindow/preferencestab/preferencestabcontrolitem.cpp \
    commongraphics/checkboxbutton.cpp \
    preferenceswindow/checkboxitem.cpp \
    preferenceswindow/dividerline.cpp \
    preferenceswindow/escapebutton.cpp \
    preferenceswindow/comboboxitem.cpp \
    newsfeedwindow/newsfeedwindowitem.cpp \
    preferenceswindow/accountwindow/usernameitem.cpp \
    preferenceswindow/accountwindow/emailitem.cpp \
    preferenceswindow/accountwindow/planitem.cpp \
    preferenceswindow/accountwindow/expiredateitem.cpp \
    preferenceswindow/accountwindow/editaccountitem.cpp \
    preferenceswindow/connectionwindow/subpageitem.cpp \
    preferenceswindow/basepage.cpp \
    preferenceswindow/baseitem.cpp \
    preferenceswindow/sharewindow/securehotspotitem.cpp \
    preferenceswindow/sharewindow/proxygatewayitem.cpp \
    preferenceswindow/editboxitem.cpp \
    preferenceswindow/sharewindow/proxyipaddressitem.cpp \
    preferenceswindow/debugwindow/viewlogitem.cpp \
    preferenceswindow/debugwindow/advancedparametersitem.cpp \
    preferenceswindow/connectionwindow/firewallmodeitem.cpp \
    preferenceswindow/connectionwindow/automanualswitchitem.cpp \
    preferenceswindow/connectionwindow/connectionmodeitem.cpp \
    preferenceswindow/connectionwindow/packetsizeitem.cpp \
    preferenceswindow/connectionwindow/macspoofingitem.cpp \
    preferenceswindow/connectionwindow/macaddressitem.cpp \
    preferenceswindow/connectionwindow/autorotatemacitem.cpp \
    commongraphics/verticalscrollbar.cpp \
    preferenceswindow/networkwhitelistwindow/networkwhitelistwindowitem.cpp \
    preferenceswindow/splittunnelingwindow/splittunnelingwindowitem.cpp \
    preferenceswindow/proxysettingswindow/proxysettingswindowitem.cpp \
    commongraphics/clickablegraphicsobject.cpp \
    commongraphics/textbutton.cpp \
    commongraphics/bubblebuttonbright.cpp \
    externalconfig/externalconfigwindowitem.cpp \
    twofactorauth/twofactorauthwindowitem.cpp \
    twofactorauth/twofactorauthokbutton.cpp \
    bottominfowidget/sharingfeatures/sharingfeatureswindowitem.cpp \
    bottominfowidget/sharingfeatures/sharingfeature.cpp \
    bottominfowidget/upgradewidget/upgradewidgetitem.cpp \
    bottominfowidget/bottominfoitem.cpp \
    commongraphics/bubblebuttondark.cpp \
    loginwindow/blockableqlineedit.cpp \
    commongraphics/texticonbutton.cpp \
    commongraphics/verticaldividerline.cpp \
    commonwidgets/custommenulineedit.cpp \
    commonwidgets/iconbuttonwidget.cpp \
    commonwidgets/textbuttonwidget.cpp \
    preferenceswindow/proxysettingswindow/proxysettingsitem.cpp \
    newsfeedwindow/scrollablemessage.cpp \
    commongraphics/iconbutton.cpp \
    preferenceswindow/textitem.cpp \
    connectwindow/logonotificationsbutton.cpp \
    types/dnswhileconnectedtype.cpp \
    preferenceswindow/connectionwindow/dnswhileconnecteditem.cpp \
    languagecontroller.cpp \
    locationswindow/widgetlocations/staticipdeviceinfo.cpp \
    locationswindow/widgetlocations/configfooterinfo.cpp \
    mainwindowcontroller.cpp \
    multipleaccountdetection/multipleaccountdetectionfactory.cpp


HEADERS += \
    ../../common/names.h \
    ../backend/backend.h \
    ../backend/connectstatehelper.h \
    ../backend/firewallstatehelper.h \
    ../backend/ibackend.h \
    ../backend/notificationscontroller.h \
    $$COMMON_PATH/ipc/generated_proto/types.pb.h \
    ../backend/locationsmodel/alllocationsmodel.h \
    ../backend/locationsmodel/basiccitiesmodel.h \
    ../backend/locationsmodel/basiclocationsmodel.h \
    ../backend/locationsmodel/configuredcitiesmodel.h \
    ../backend/locationsmodel/favoritecitiesmodel.h \
    ../backend/locationsmodel/favoritelocationsstorage.h \
    ../backend/locationsmodel/locationmodelitem.h \
    ../backend/locationsmodel/locationsmodel.h \
    ../backend/locationsmodel/sortlocationsalgorithms.h \
    ../backend/locationsmodel/staticipscitiesmodel.h \
    ../backend/preferences/accountinfo.h \
    ../backend/preferences/detectlanrange.h \
    ../backend/preferences/guisettingsfromver1.h \
    ../backend/preferences/preferences.h \
    ../backend/preferences/preferenceshelper.h \
    $$COMMON_PATH/types/locationid.h \
    ../backend/types/pingtime.h \
    ../backend/types/types.h \
    ../backend/types/upgrademodetype.h \
    $$COMMON_PATH/utils/extraconfig.h \
    $$COMMON_PATH/utils/languagesutil.h \
    $$COMMON_PATH/utils/logger.h \
    $$COMMON_PATH/utils/mergelog.h \
    $$COMMON_PATH/utils/multiline_message_logger.h \
    $$COMMON_PATH/utils/utils.h \
    $$COMMON_PATH/utils/protobuf_includes.h \
    $$COMMON_PATH/utils/widgetutils.h \
    $$COMMON_PATH/utils/executable_signature/executable_signature.h \
    $$COMMON_PATH/version/appversion.h \
    $$COMMON_PATH/version/windscribe_version.h \
    ../backend/persistentstate.h \
    application/windowsnativeeventfilter.h \
    application/windscribeapplication.h \
    commongraphics/imageitem.h \
    commongraphics/scalablegraphicsobject.h \
    commonwidgets/combomenuwidget.h \
    commonwidgets/combomenuwidgetbutton.h \
    commonwidgets/iconwidget.h \
    commonwidgets/lightwidget.h \
    connectwindow/connectionbadgedots.h \
    connectwindow/connectstateprotocolport.h \
    connectwindow/ipaddressitem/ipaddressitem.h \
    connectwindow/ipaddressitem/numberitem.h \
    connectwindow/ipaddressitem/numberspixmap.h \
    connectwindow/ipaddressitem/octetitem.h \
    connectwindow/middleitem.h \
    connectwindow/serverratingindicator.h \
    dialogs/advancedparametersdialog.h \
    dialogs/dialoggetusernamepassword.h \
    dialogs/dialogmessagecpuusage.h \
    locationswindow/widgetlocations/iitemwidget.h \
    locationswindow/widgetlocations/itemwidgetcity.h \
    locationswindow/widgetlocations/itemwidgetheader.h \
    locationswindow/widgetlocations/itemwidgetregion.h \
    locationswindow/widgetlocations/scrollbar.h \
    locationswindow/widgetlocations/widgetcitieslist.h \
    locationswindow/widgetlocations/widgetlocations.h \
    locationswindow/widgetlocations/widgetlocationslist.h \
    overlaysconnectwindow/generalmessagetwobuttonwindowitem.h \
    overlaysconnectwindow/igeneralmessagetwobuttonwindow.h \
    overlaysconnectwindow/generalmessagewindowitem.h \
    overlaysconnectwindow/igeneralmessagewindow.h \
    overlaysconnectwindow/iupgradewindow.h \
    overlaysconnectwindow/upgradewindowitem.h \
    overlaysconnectwindow/iupdatewindow.h \
    overlaysconnectwindow/updatewindowitem.h \
    overlaysconnectwindow/iupdateappitem.h \
    overlaysconnectwindow/updateappitem.h \
    graphicresources/fontdescr.h \
    graphicresources/iconmanager.h \
    graphicresources/independentpixmap.h \
    launchonstartup/launchonstartup.h \
    loginwindow/iinitwindow.h \
    loginwindow/initwindowitem.h \
    preferenceswindow/connectionwindow/packetsizeeditboxitem.h \
    preferenceswindow/debugwindow/apiresolutionitem.h \
    preferenceswindow/generalwindow/versioninfoitem.h \
    preferenceswindow/networkwhitelistwindow/currentnetworkitem.h \
    preferenceswindow/networkwhitelistwindow/networklistitem.h \
    preferenceswindow/networkwhitelistwindow/networkwhitelistshared.h \
    preferenceswindow/splittunnelingwindow/appincludeditem.h \
    preferenceswindow/splittunnelingwindow/appsearchitem.h \
    preferenceswindow/splittunnelingwindow/iporhostnameitem.h \
    preferenceswindow/splittunnelingwindow/newiporhostnameitem.h \
    preferenceswindow/splittunnelingwindow/searchlineedititem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingappsitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingappssearchitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingappssearchwindowitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingappswindowitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingiphostnamewindowitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingipsandhostnamesitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingswitchitem.h \
    tooltips/itooltip.h \
    tooltips/serverratingstooltip.h \
    tooltips/tooltipbasic.h \
    tooltips/tooltipcontroller.h \
    tooltips/tooltipdescriptive.h \
    tooltips/tooltiptypes.h \
    tooltips/tooltiputil.h \
    utils/ipvalidation.h \
    utils/protoenumtostring.h \
    commonwidgets/custommenuwidget.h \
    commonwidgets/customtexteditwidget.h \
    commonwidgets/scrollareawidget.h \
    commonwidgets/verticalscrollbarwidget.h \
    log/logviewerwindow.h \
    preferenceswindow/debugwindow/advancedparameterswindowitem.h \
    utils/hardcodedsettings.h \
    utils/simplecrypt.h \
    utils/writeaccessrightschecker.h \
    blockconnect.h \
    dpiscaleawarewidget.h \
    dpiscalemanager.h \
    freetrafficnotificationcontroller.h \
    guitest.h \
    systemtray/locationstraymenubutton.h \
    systemtray/locationstraymenuitemdelegate.h \
    systemtray/locationstraymenuwidget.h \
    systemtray/locationstraymenunative.h \
    loginattemptscontroller.h \
    mainwindow.h \
    graphicresources/fontmanager.h \
    graphicresources/imageresourcessvg.h \
    graphicresources/imageresourcesjpg.h \
    loginwindow/logginginwindowitem.h \
    connectwindow/connectwindowitem.h \
    locationswindow/locationswindow.h \
    locationswindow/widgetlocations/backgroundpixmapanimation.h \
    locationswindow/widgetlocations/cursorupdatehelper.h \
    locationswindow/widgetlocations/itemtimems.h \
    locationswindow/widgetlocations/iwidgetlocationsinfo.h \
    locationswindow/widgetlocations/locationstab.h \
    locationswindow/widgetlocations/widgetcities.h \
    locationswindow/widgetlocations/widgetlocationssizes.h \
    $$COMMON_PATH/ipc/command.h \
    $$COMMON_PATH/ipc/commandfactory.h \
    $$COMMON_PATH/ipc/connection.h \
    $$COMMON_PATH/ipc/iconnection.h \
    $$COMMON_PATH/ipc/iserver.h \
    $$COMMON_PATH/ipc/protobufcommand.h \
    $$COMMON_PATH/ipc/server.h \
    $$COMMON_PATH/ipc/tcpconnection.h \
    $$COMMON_PATH/ipc/tcpserver.h \
    $$COMMON_PATH/ipc/generated_proto/clientcommands.pb.h \
    $$COMMON_PATH/ipc/generated_proto/servercommands.pb.h \
    connectwindow/connectbutton.h \
    connectwindow/background.h \
    connectwindow/locationsbutton.h \
    connectwindow/wifiname.h \
    connectwindow/firewallbutton.h \
    loginwindow/loginwindowitem.h \
    loginwindow/welcomewindowitem.h \
    loginwindow/credentialswindowitem.h \
    loginwindow/ilogginginwindow.h \
    loginwindow/loginyesnobutton.h \
    loginwindow/usernamepasswordentry.h \
    loginwindow/loginbutton.h \
    loginwindow/iloginwindow.h \
    loginwindow/ilogginginwindow.h \
    loginwindow/iconhoverengagebutton.h \
    loginwindow/firewallturnoffbutton.h \
    emergencyconnectwindow/iemergencyconnectwindow.h \
    commongraphics/commongraphics.h \
    emergencyconnectwindow/emergencyconnectwindowitem.h \
    emergencyconnectwindow/textlinkbutton.h \
    utils/makecustomshadow.h \
    connectwindow/iconnectwindow.h \
    utils/shadowmanager.h \
    connectwindow/iconnectwindow.h \
    preferenceswindow/ipreferenceswindow.h \
    preferenceswindow/preferenceswindowitem.h \
    preferenceswindow/bottomresizeitem.h \
    preferenceswindow/generalwindow/generalwindowitem.h \
    preferenceswindow/scrollareaitem.h \
    preferenceswindow/accountwindow/accountwindowitem.h \
    preferenceswindow/connectionwindow/connectionwindowitem.h \
    preferenceswindow/debugwindow/debugwindowitem.h \
    preferenceswindow/sharewindow/sharewindowitem.h \
    preferenceswindow/preferencestab/ipreferencestabcontrol.h \
    preferenceswindow/preferencestab/preferencestabcontrolitem.h \
    commongraphics/checkboxbutton.h \
    preferenceswindow/checkboxitem.h \
    preferenceswindow/dividerline.h \
    preferenceswindow/escapebutton.h \
    preferenceswindow/comboboxitem.h \
    newsfeedwindow/inewsfeedwindow.h \
    newsfeedwindow/newsfeedwindowitem.h \
    preferenceswindow/accountwindow/usernameitem.h \
    preferenceswindow/accountwindow/emailitem.h \
    preferenceswindow/accountwindow/planitem.h \
    preferenceswindow/accountwindow/expiredateitem.h \
    preferenceswindow/accountwindow/editaccountitem.h \
    preferenceswindow/connectionwindow/subpageitem.h \
    preferenceswindow/basepage.h \
    preferenceswindow/baseitem.h \
    preferenceswindow/sharewindow/securehotspotitem.h \
    preferenceswindow/sharewindow/proxygatewayitem.h \
    preferenceswindow/editboxitem.h \
    preferenceswindow/sharewindow/proxyipaddressitem.h \
    preferenceswindow/debugwindow/viewlogitem.h \
    preferenceswindow/debugwindow/advancedparametersitem.h \
    preferenceswindow/connectionwindow/firewallmodeitem.h \
    preferenceswindow/connectionwindow/automanualswitchitem.h \
    preferenceswindow/connectionwindow/connectionmodeitem.h \
    preferenceswindow/connectionwindow/packetsizeitem.h \
    preferenceswindow/connectionwindow/macspoofingitem.h \
    preferenceswindow/connectionwindow/macaddressitem.h \
    preferenceswindow/connectionwindow/autorotatemacitem.h \
    commongraphics/verticalscrollbar.h \
    preferenceswindow/networkwhitelistwindow/networkwhitelistwindowitem.h \
    preferenceswindow/splittunnelingwindow/splittunnelingwindowitem.h \
    preferenceswindow/proxysettingswindow/proxysettingswindowitem.h \
    commongraphics/clickablegraphicsobject.h \
    commongraphics/textbutton.h \
    commongraphics/bubblebuttonbright.h \
    externalconfig/iexternalconfigwindow.h \
    externalconfig/externalconfigwindowitem.h \
    twofactorauth/itwofactorauthwindow.h \
    twofactorauth/twofactorauthwindowitem.h \
    twofactorauth/twofactorauthokbutton.h \
    connectwindow/iconnectwindow.h \
    bottominfowidget/sharingfeatures/sharingfeatureswindowitem.h \
    bottominfowidget/sharingfeatures/sharingfeature.h \
    bottominfowidget/upgradewidget/upgradewidgetitem.h \
    bottominfowidget/ibottominfoitem.h \
    bottominfowidget/bottominfoitem.h \
    commongraphics/bubblebuttondark.h \
    loginwindow/blockableqlineedit.h \
    commongraphics/texticonbutton.h \
    commongraphics/verticaldividerline.h \
    commonwidgets/custommenulineedit.h \
    commonwidgets/iconbuttonwidget.h \
    commonwidgets/textbuttonwidget.h \
    preferenceswindow/proxysettingswindow/proxysettingsitem.h \
    newsfeedwindow/scrollablemessage.h \
    commongraphics/iconbutton.h \
    preferenceswindow/textitem.h \
    connectwindow/logonotificationsbutton.h \
    types/dnswhileconnectedtype.h \
    preferenceswindow/connectionwindow/dnswhileconnecteditem.h \
    languagecontroller.h \
    locationswindow/widgetlocations/staticipdeviceinfo.h \
    locationswindow/widgetlocations/configfooterinfo.h \
    mainwindowcontroller.h \
    multipleaccountdetection/imultipleaccountdetection.h \
    multipleaccountdetection/multipleaccountdetectionfactory.h

RESOURCES += \
    svg.qrc \
    jpg.qrc \
    windscribe.qrc

TRANSLATIONS += \
    languages/ru.ts \
    languages/es.ts \
    languages/fr.ts \
    languages/ja.ts \
    languages/ar.ts \
    languages/hu.ts \
    languages/it.ts \
    languages/ko.ts \
    languages/nl.ts \
    languages/zh.ts \
    languages/de.ts \
    languages/pl.ts \
    languages/tr.ts \
    languages/cs.ts \
    languages/da.ts \
    languages/el.ts \
    languages/pt.ts \
    languages/sk.ts \
    languages/th.ts \
    languages/vi.ts \
    languages/en.ts \
    languages/en_nsfw.ts \
    languages/sv.ts \
    languages/id.ts \
    languages/hi.ts \
    languages/hr.ts

DISTFILES += \
    languages/ar.ts \
    languages/cs.ts \
    languages/da.ts \
    languages/de.ts \
    languages/el.ts \
    languages/en.ts \
    languages/en_nsfw.ts \
    languages/es.ts \
    languages/fr.ts \
    languages/hi.ts \
    languages/hr.ts \
    languages/hu.ts \
    languages/id.ts \
    languages/it.ts \
    languages/ja.ts \
    languages/ko.ts \
    languages/nl.ts \
    languages/pl.ts \
    languages/pt.ts \
    languages/ru.ts \
    languages/sk.ts \
    languages/sv.ts \
    languages/th.ts \
    languages/tr.ts \
    languages/vi.ts \
    languages/zh.ts 

FORMS += \
    dialogs/dialoggetusernamepassword.ui \
    dialogs/dialogmessagecpuusage.ui

