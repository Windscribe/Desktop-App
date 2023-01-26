# -*- coding: utf-8 -*-


import sys

# Help for setting up this file can be found at:
# https://dmgbuild.readthedocs.io/en/latest/settings.html

# Before Python 3.4, use biplist; afterwards, use plistlib
if sys.version_info < (3, 4):
    import biplist
    def read_plist(path):
        return biplist.readPlist(path)
else:
    import plistlib
    def read_plist(path):
        with open(path, 'rb') as f:
            return plistlib.load(f)

import os.path

application = defines.get('app', 'WindscribeInstaller.app')
appname = os.path.basename(application)

def icon_from_app(app_path):
    plist_path = os.path.join(app_path, 'Contents', 'Info.plist')
    plist = read_plist(plist_path)
    icon_name = plist['CFBundleIconFile']
    icon_root,icon_ext = os.path.splitext(icon_name)
    if not icon_ext:
        icon_ext = '.icns'
    icon_name = icon_root + icon_ext
    return os.path.join(app_path, 'Contents', 'Resources', icon_name)

# Uncomment to override the output filename
# filename = 'test.dmg'

# Uncomment to override the output volume name
# volume_name = 'Test'

# Volume format (see hdiutil create -help)
format = defines.get('format', 'UDBZ')

# Compression level (if relevant)
# compression_level = 9

# Volume size
size = defines.get('size', None)

# Files to include
files = [ application ]

# Symlinks to create
#symlinks = { 'Applications': '/Applications' }

# Files to hide
# hide = [ 'Secret.data' ]

# Files to hide the extension of
# hide_extension = [ 'README.rst' ]

# Volume icon
#
# You can either define icon, in which case that icon file will be copied to the
# image, *or* you can define badge_icon, in which case the icon file you specify
# will be used to badge the system's Removable Disk icon. Badge icons require
# pyobjc-framework-Quartz, and will increase dmg size by ~2.5MB
#
icon = icon_from_app(application)
#badge_icon = icon_from_app(application)

# Where to put the icons
icon_locations = {
    appname: (176, 192)
    }

# .. Window configuration ......................................................
background = defines.get('background', 'osx_install_background.tiff')

show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
sidebar_width = 180

# Window position in ((x, y), (w, h)) format
window_rect = ((100, 425), (350, 375))

# Select the default view; must be one of
#
#    'icon-view'
#    'list-view'
#    'column-view'
#    'coverflow'
#
default_view = 'icon-view'

# General view configuration
show_icon_preview = False

# Set these to True to force inclusion of icon/list view settings (otherwise
# we only include settings for the default view)
include_icon_view_settings = 'auto'
include_list_view_settings = 'auto'

# .. Icon view configuration ...................................................

arrange_by = None
grid_offset = (0, 0)
grid_spacing = 100
scroll_position = (0, 0)
label_pos = 'bottom' # or 'right'
text_size = 12
icon_size = 64
