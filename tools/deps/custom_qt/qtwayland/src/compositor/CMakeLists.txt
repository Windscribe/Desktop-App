# Generated from compositor.pro.

#####################################################################
## WaylandCompositor Module:
#####################################################################

qt_internal_add_module(WaylandCompositor
    PLUGIN_TYPES wayland-graphics-integration-server wayland-hardware-layer-integration
    SOURCES
        compat/removed_api.cpp
        ../shared/qwaylandinputmethodeventbuilder.cpp ../shared/qwaylandinputmethodeventbuilder_p.h
        ../shared/qwaylandmimehelper.cpp ../shared/qwaylandmimehelper_p.h
        ../shared/qwaylandsharedmemoryformathelper_p.h
        compositor_api/qwaylandbufferref.cpp compositor_api/qwaylandbufferref.h
        compositor_api/qwaylandclient.cpp compositor_api/qwaylandclient.h
        compositor_api/qwaylandcompositor.cpp compositor_api/qwaylandcompositor.h compositor_api/qwaylandcompositor_p.h
        compositor_api/qwaylanddestroylistener.cpp compositor_api/qwaylanddestroylistener.h compositor_api/qwaylanddestroylistener_p.h
        compositor_api/qwaylandkeyboard.cpp compositor_api/qwaylandkeyboard.h compositor_api/qwaylandkeyboard_p.h
        compositor_api/qwaylandkeymap.cpp compositor_api/qwaylandkeymap.h compositor_api/qwaylandkeymap_p.h
        compositor_api/qwaylandoutput.cpp compositor_api/qwaylandoutput.h compositor_api/qwaylandoutput_p.h
        compositor_api/qwaylandoutputmode.cpp compositor_api/qwaylandoutputmode.h compositor_api/qwaylandoutputmode_p.h
        compositor_api/qwaylandpointer.cpp compositor_api/qwaylandpointer.h compositor_api/qwaylandpointer_p.h
        compositor_api/qwaylandresource.cpp compositor_api/qwaylandresource.h
        compositor_api/qwaylandseat.cpp compositor_api/qwaylandseat.h compositor_api/qwaylandseat_p.h
        compositor_api/qwaylandsurface.cpp compositor_api/qwaylandsurface.h compositor_api/qwaylandsurface_p.h
        compositor_api/qwaylandsurfacegrabber.cpp compositor_api/qwaylandsurfacegrabber.h
        compositor_api/qwaylandtouch.cpp compositor_api/qwaylandtouch.h compositor_api/qwaylandtouch_p.h
        compositor_api/qwaylandview.cpp compositor_api/qwaylandview.h compositor_api/qwaylandview_p.h
        extensions/qwaylandidleinhibitv1.cpp extensions/qwaylandidleinhibitv1.h extensions/qwaylandidleinhibitv1_p.h
        extensions/qwaylandiviapplication.cpp extensions/qwaylandiviapplication.h extensions/qwaylandiviapplication_p.h
        extensions/qwaylandivisurface.cpp extensions/qwaylandivisurface.h extensions/qwaylandivisurface_p.h
        extensions/qwaylandqttextinputmethod.cpp extensions/qwaylandqttextinputmethod.h extensions/qwaylandqttextinputmethod_p.h
        extensions/qwaylandqttextinputmethodmanager.cpp extensions/qwaylandqttextinputmethodmanager.h extensions/qwaylandqttextinputmethodmanager_p.h
        extensions/qwaylandqtwindowmanager.cpp extensions/qwaylandqtwindowmanager.h extensions/qwaylandqtwindowmanager_p.h
        extensions/qwaylandshell.cpp extensions/qwaylandshell.h extensions/qwaylandshell_p.h
        extensions/qwaylandshellsurface.cpp extensions/qwaylandshellsurface.h
        extensions/qwaylandtextinput.cpp extensions/qwaylandtextinput.h extensions/qwaylandtextinput_p.h
        extensions/qwaylandtextinputmanager.cpp extensions/qwaylandtextinputmanager.h extensions/qwaylandtextinputmanager_p.h
        extensions/qwaylandviewporter.cpp extensions/qwaylandviewporter.h extensions/qwaylandviewporter_p.h
        extensions/qwaylandwlshell.cpp extensions/qwaylandwlshell.h extensions/qwaylandwlshell_p.h
        extensions/qwaylandxdgdecorationv1.cpp extensions/qwaylandxdgdecorationv1.h extensions/qwaylandxdgdecorationv1_p.h
        extensions/qwaylandxdgoutputv1.cpp extensions/qwaylandxdgoutputv1.h extensions/qwaylandxdgoutputv1_p.h
        extensions/qwaylandxdgshell.cpp extensions/qwaylandxdgshell.h extensions/qwaylandxdgshell_p.h
        extensions/qwlqtkey.cpp extensions/qwlqtkey_p.h
        extensions/qwlqttouch.cpp extensions/qwlqttouch_p.h
        global/qtwaylandcompositorglobal.h
        global/qtwaylandqmlinclude.h
        global/qwaylandcompositorextension.cpp global/qwaylandcompositorextension.h global/qwaylandcompositorextension_p.h
        global/qwaylandutils_p.h
        hardware_integration/qwlclientbufferintegration.cpp hardware_integration/qwlclientbufferintegration_p.h
        wayland_wrapper/qwlbuffermanager.cpp wayland_wrapper/qwlbuffermanager_p.h
        wayland_wrapper/qwlclientbuffer.cpp wayland_wrapper/qwlclientbuffer_p.h
        wayland_wrapper/qwlregion.cpp wayland_wrapper/qwlregion_p.h
    INCLUDE_DIRECTORIES
        ../shared
        compositor_api
        extensions
        global
        wayland_wrapper
    GENERATE_CPP_EXPORTS
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WaylandGlobalPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Wayland::Server
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::WaylandGlobalPrivate
)

set(compositor_no_pch_sources
    compat/removed_api.cpp
)

foreach(src ${compositor_no_pch_sources})
    qt_update_ignore_pch_source(WaylandCompositor ${src})
endforeach()

# Resources:
set(compositor_resource_files
    "shaders/surface.vert.qsb"
    "shaders/surface_rgba.frag.qsb"
    "shaders/surface_rgbx.frag.qsb"
    "shaders/surface_y_u_v.frag.qsb"
    "shaders/surface_y_uv.frag.qsb"
    "shaders/surface_y_xuxv.frag.qsb"
    "shaders/surface_oes_external.frag"
)

qt_internal_add_resource(WaylandCompositor "compositor"
    PREFIX
        "/qt-project.org/wayland/compositor"
    FILES
        ${compositor_resource_files}
)

qt6_generate_wayland_protocol_server_sources(WaylandCompositor
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/idle-inhibit-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/ivi-application.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/presentation-time.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/scaler.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/text-input-unstable-v2.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/text-input-unstable-v4-wip.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/viewporter.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/wayland.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/xdg-decoration-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/xdg-output-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/protocol/xdg-shell.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/hardware-integration.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/qt-key-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/qt-text-input-method-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/qt-texture-sharing-unstable-v1.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/qt-windowmanager.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/server-buffer-extension.xml
        ${CMAKE_CURRENT_SOURCE_DIR}/../extensions/touch-extension.xml
)

#### Keys ignored in scope 1:.:.:compositor.pro:<TRUE>:
# MODULE = "waylandcompositor"

## Scopes:
#####################################################################

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_wayland_text_input_v4_wip
    SOURCES
        extensions/qwaylandtextinputv4.cpp extensions/qwaylandtextinputv4.h extensions/qwaylandtextinputv4_p.h
        extensions/qwaylandtextinputmanagerv4.cpp extensions/qwaylandtextinputmanagerv4.h extensions/qwaylandtextinputmanagerv4_p.h
    DEFINES
        QT_WAYLAND_TEXT_INPUT_V4_WIP=1
)

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_opengl
    SOURCES
        hardware_integration/qwlclientbufferintegrationfactory.cpp hardware_integration/qwlclientbufferintegrationfactory_p.h
        hardware_integration/qwlclientbufferintegrationplugin.cpp hardware_integration/qwlclientbufferintegrationplugin_p.h
        hardware_integration/qwlhardwarelayerintegration.cpp hardware_integration/qwlhardwarelayerintegration_p.h
        hardware_integration/qwlhardwarelayerintegrationfactory.cpp hardware_integration/qwlhardwarelayerintegrationfactory_p.h
        hardware_integration/qwlhardwarelayerintegrationplugin.cpp hardware_integration/qwlhardwarelayerintegrationplugin_p.h
        hardware_integration/qwlhwintegration.cpp hardware_integration/qwlhwintegration_p.h
        hardware_integration/qwlserverbufferintegration.cpp hardware_integration/qwlserverbufferintegration_p.h
        hardware_integration/qwlserverbufferintegrationfactory.cpp hardware_integration/qwlserverbufferintegrationfactory_p.h
        hardware_integration/qwlserverbufferintegrationplugin.cpp hardware_integration/qwlserverbufferintegrationplugin_p.h
    PUBLIC_LIBRARIES
        Qt::OpenGL
)

qt_internal_extend_target(WaylandCompositor CONDITION TARGET Qt::Quick
    PUBLIC_LIBRARIES
        Qt::Quick
)

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_wayland_compositor_quick
    SOURCES
        compositor_api/qwaylandcompositorquickextensions.cpp compositor_api/qwaylandcompositorquickextensions_p.h
        compositor_api/qwaylandmousetracker.cpp compositor_api/qwaylandmousetracker_p.h
        compositor_api/qwaylandquickchildren.h
        compositor_api/qwaylandquickcompositor.cpp compositor_api/qwaylandquickcompositor.h
        compositor_api/qwaylandquickitem.cpp compositor_api/qwaylandquickitem.h compositor_api/qwaylandquickitem_p.h
        compositor_api/qwaylandquickoutput.cpp compositor_api/qwaylandquickoutput.h
        compositor_api/qwaylandquicksurface.cpp compositor_api/qwaylandquicksurface.h compositor_api/qwaylandquicksurface_p.h
        extensions/qwaylandivisurfaceintegration.cpp extensions/qwaylandivisurfaceintegration_p.h
        extensions/qwaylandquickshellintegration.cpp extensions/qwaylandquickshellintegration.h
        extensions/qwaylandquickshellsurfaceitem.cpp extensions/qwaylandquickshellsurfaceitem.h extensions/qwaylandquickshellsurfaceitem_p.h
        extensions/qwaylandquickxdgoutputv1.cpp extensions/qwaylandquickxdgoutputv1.h
        extensions/qwaylandwlshellintegration.cpp extensions/qwaylandwlshellintegration_p.h
        extensions/qwaylandxdgshellintegration.cpp extensions/qwaylandxdgshellintegration_p.h
        extensions/qwaylandpresentationtime.cpp extensions/qwaylandpresentationtime_p.h extensions/qwaylandpresentationtime_p_p.h
        global/qwaylandquickextension.cpp global/qwaylandquickextension.h
    LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
    PRIVATE_MODULE_INTERFACE
        Qt::QmlPrivate
        Qt::QuickPrivate
)


qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_wayland_datadevice
    SOURCES
        wayland_wrapper/qwldatadevice.cpp wayland_wrapper/qwldatadevice_p.h
        wayland_wrapper/qwldatadevicemanager.cpp wayland_wrapper/qwldatadevicemanager_p.h
        wayland_wrapper/qwldataoffer.cpp wayland_wrapper/qwldataoffer_p.h
        wayland_wrapper/qwldatasource.cpp wayland_wrapper/qwldatasource_p.h
)

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_im
    SOURCES
        compositor_api/qwaylandinputmethodcontrol.cpp compositor_api/qwaylandinputmethodcontrol.h compositor_api/qwaylandinputmethodcontrol_p.h
)

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_draganddrop
    SOURCES
        compositor_api/qwaylanddrag.cpp compositor_api/qwaylanddrag.h
)

qt_internal_extend_target(WaylandCompositor CONDITION QT_FEATURE_opengl AND QT_FEATURE_wayland_compositor_quick
    SOURCES
        compositor_api/qwaylandquickhardwarelayer.cpp compositor_api/qwaylandquickhardwarelayer_p.h
        extensions/qwltexturesharingextension.cpp extensions/qwltexturesharingextension_p.h
)
qt_internal_add_docs(WaylandCompositor
    doc/qtwaylandcompositor.qdocconf
)

if (TARGET Qt::Qml)
    qt_internal_add_qml_module(WaylandCompositor
        URI QtWayland.Compositor
        VERSION ${PROJECT_VERSION}
        CLASS_NAME QWaylandCompositorPlugin
        PLUGIN_TARGET qwaylandcompositorplugin
        PAST_MAJOR_VERSIONS 1
        QML_FILES
            qmlfiles/WaylandCursorItem.qml
            qmlfiles/WaylandOutputWindow.qml
        DEPENDENCIES
            QtQuick
    )
endif()

qt_record_extra_qt_main_tools_package_dependency(WaylandCompositor WaylandScannerTools "${PROJECT_VERSION}")
qt_record_extra_qt_package_dependency(WaylandCompositor WaylandGlobalPrivate "${PROJECT_VERSION}")
