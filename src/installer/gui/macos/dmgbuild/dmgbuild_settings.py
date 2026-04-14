# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os.path
import plistlib
import sys

application = defines.get('app', 'WindscribeInstaller.app')
appname = os.path.basename(application)

def icon_from_app(app_path):
    plist_path = os.path.join(app_path, 'Contents', 'Info.plist')
    with open(plist_path, 'rb') as f:
        plist = plistlib.load(f)
    icon_name = plist['CFBundleIconFile']
    icon_root, icon_ext = os.path.splitext(icon_name)
    if not icon_ext:
        icon_ext = '.icns'
    icon_name = icon_root + icon_ext
    return os.path.join(app_path, 'Contents', 'Resources', icon_name)

format = defines.get('format', 'ULMO')
size = defines.get('size', None)
files = [ application ]
icon = icon_from_app(application)

icon_locations = {
    appname: (175, 192)
}

background = defines.get('background', 'installer_background.png')

show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
sidebar_width = 180

window_rect = ((100, 425), (350, 350))

default_view = 'icon-view'
show_icon_preview = False

include_icon_view_settings = 'auto'
include_list_view_settings = 'auto'

arrange_by = None
grid_offset = (0, 0)
grid_spacing = 100
scroll_position = (0, 0)
label_pos = 'bottom'
text_size = 12
icon_size = 64
