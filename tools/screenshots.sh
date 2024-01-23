#!/bin/bash

WINDSCRIBE_PID=$(ps aux | grep "[W]indscribe.app" | awk '{print $2}')

take_screenshot() {
    WINDSCRIBE_GEOMETRY="`osascript -e 'tell application "System Events" to get { position, size } of first window of (first process whose unix id is '$WINDSCRIBE_PID')' | tr -d ' '`"
    screencapture -R $WINDSCRIBE_GEOMETRY $1
}

osascript -e 'activate application "Windscribe"'
take_screenshot connect.png

osascript -e 'tell application "System Events" to keystroke "i" using {command down, shift down}'
sleep 1
take_screenshot logout.png
osascript -e 'tell application "System Events" to key code 53' #esc

osascript -e 'tell application "System Events" to keystroke "i" using command down'
sleep 1
take_screenshot quit.png
osascript -e 'tell application "System Events" to key code 53' #esc

osascript -e 'tell application "System Events" to keystroke "r" using command down'
sleep 1
take_screenshot protocol_change.png
osascript -e 'tell application "System Events" to keystroke "r" using {command down, shift down}'

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_general_1.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_general_2.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_general_3.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_account.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_connection_1.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_connection_2.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_connection_3.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 4 # wait longer for robert screen to actually load
take_screenshot preference_robert_1.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_robert_2.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_advanced_1.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_advanced_2.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_help_1.png
osascript -e 'tell application "System Events" to key code 121' # pgdn
sleep 1
take_screenshot preference_help_2.png

osascript -e 'tell application "System Events" to keystroke "p" using command down'
sleep 1
take_screenshot preference_about.png
osascript -e 'tell application "System Events" to keystroke "p" using {command down, shift down}'

osascript -e 'tell application "System Events" to keystroke "n" using command down'
sleep 1
take_screenshot news_feed.png
osascript -e 'tell application "System Events" to keystroke "n" using {command down, shift down}'

osascript -e 'tell application "System Events" to keystroke "g" using command down'
sleep 1
take_screenshot upgrade.png
osascript -e 'tell application "System Events" to keystroke "c" using command down'

osascript -e 'tell application "System Events" to keystroke "d" using command down'
sleep 1
take_screenshot update.png
osascript -e 'tell application "System Events" to keystroke "c" using command down'

osascript -e 'tell application "System Events" to keystroke "l" using command down'
sleep 1
take_screenshot login.png
osascript -e 'tell application "System Events" to key code 53' #esc

osascript -e 'tell application "System Events" to keystroke "e" using command down'
sleep 1
take_screenshot emergency.png
osascript -e 'tell application "System Events" to key code 53' #esc

osascript -e 'tell application "System Events" to keystroke "x" using command down'
sleep 1
take_screenshot external_config.png
osascript -e 'tell application "System Events" to key code 53' #esc

osascript -e 'tell application "System Events" to keystroke "l" using {command down, shift down}'
osascript -e 'tell application "System Events" to keystroke "c" using command down'
