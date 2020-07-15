ECHO off
SET QT_PATH=c:\Qt

ECHO ===== Installing Qt into C:\Qt =====

ECHO Qt directory: "%QT_PATH%"

rd /s /q %QT_PATH%
mkdir %QT_PATH%

ECHO Downloading Qt 5.12.6 sources, please wait. This may take a few minutes.
curl.exe https://download.qt.io/archive/qt/5.12/5.12.6/single/qt-everywhere-src-5.12.6.zip -o %QT_PATH%\qt.zip -k -L

7z x %QT_PATH%\qt.zip -o%QT_PATH%
PUSHD %QT_PATH%
rename qt-everywhere-src-5.12.6 5.12.6
del %QT_PATH%\qt.zip             

POPD
REM copy /Y qt_changed_files\qcommonstyle.cpp %QT_PATH%\5.12.1\qtbase\src\widgets\styles\qcommonstyle.cpp
REM copy /Y qt_changed_files\qgesturemanager.cpp %QT_PATH%\5.12.1\qtbase\src\widgets\kernel\qgesturemanager.cpp
REM copy /Y qt_changed_files\qstandardgestures_p.h %QT_PATH%\5.12.1\qtbase\src\widgets\kernel\qstandardgestures_p.h

PUSHD %QT_PATH%

ECHO Building Qt 5.12.6, please wait. This may take a few minutes.

cd %QT_PATH%\5.12.6

set PATH=C:\Python27;%PATH%

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
call configure.bat -opensource -confirm-license -I c:\libs\openssl\include -L c:\libs\openssl\lib -prefix c:\qt -openssl -nomake examples -skip qt3d -skip qtactiveqt -skip qtcanvas3d -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtlocation -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquickcontrols -skip qtquickcontrols2 -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview
set CL=/MP
nmake
nmake install

POPD


REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\Qt.zip c:\Qt
)
)

