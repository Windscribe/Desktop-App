# vc.mak by Michal Trojnara 1998-2017
# with help of David Gillingham <dgillingham@gmail.com>
# with help of Pierre Delaage <delaage.pierre@free.fr>

# the compilation requires:
# - Visual C++ 2005 Express Edition with Platform SDK
#   http://social.msdn.microsoft.com/forums/en-US/Vsexpressvc/thread/c5c3afad-f4c6-4d27-b471-0291e099a742/
# - Visual C++ 2005 Professional Edition
# - Visual C++ 2008 Express Edition

!IF [ml64.exe /help >NUL 2>&1]
TARGET=win32
SSLLIBS=libeay32.lib ssleay32.lib
!ELSE
TARGET=win64
SSLLIBS=libcrypto.lib libssl.lib
!ENDIF
!MESSAGE Detected target: $(TARGET)
!MESSAGE

# modify this to point to your OpenSSL directory
# either install a precompiled version (*not* the "Light" one) from
# http://www.slproweb.com/products/Win32OpenSSL.html
#SSLDIR=C:\OpenSSL-Win32
# or compile one yourself
SSLDIR=c:\libs\openssl
# or simply install with "nmake -f ms\ntdll.mak install"
#SSLDIR=\usr\local\ssl

INCDIR=$(SSLDIR)\include
LIBDIR=$(SSLDIR)\lib

SRC=..\src
OBJROOT=..\obj
OBJ=$(OBJROOT)\$(TARGET)
BINROOT=..\bin
BIN=$(BINROOT)\$(TARGET)

SHAREDOBJS=$(OBJ)\stunnel.obj $(OBJ)\ssl.obj $(OBJ)\ctx.obj \
	$(OBJ)\verify.obj $(OBJ)\file.obj $(OBJ)\client.obj \
	$(OBJ)\protocol.obj $(OBJ)\sthreads.obj $(OBJ)\log.obj \
	$(OBJ)\options.obj $(OBJ)\network.obj $(OBJ)\resolver.obj \
	$(OBJ)\str.obj $(OBJ)\tls.obj $(OBJ)\fd.obj $(OBJ)\dhparam.obj \
	$(OBJ)\cron.obj
GUIOBJS=$(OBJ)\ui_win_gui.obj $(OBJ)\resources.res
CLIOBJS=$(OBJ)\ui_win_cli.obj

CC=cl
LINK=link

UNICODEFLAGS=/DUNICODE /D_UNICODE
CFLAGS=/MD /W3 /O2 /nologo /I"$(INCDIR)" $(UNICODEFLAGS)
LDFLAGS=/NOLOGO /DEBUG

SHAREDLIBS=ws2_32.lib user32.lib shell32.lib kernel32.lib
GUILIBS=advapi32.lib comdlg32.lib crypt32.lib gdi32.lib psapi.lib
CLILIBS=
# static linking:
#	/LIBPATH:"$(LIBDIR)\VC\static" libeay32MD.lib ssleay32MD.lib

{$(SRC)\}.c{$(OBJ)\}.obj:
	$(CC) $(CFLAGS) -Fo$@ -c $<

{$(SRC)\}.rc{$(OBJ)\}.res:
	$(RC) -fo$@ -r $<

all: build

build: makedirs $(BIN)\stunnel.exe $(BIN)\tstunnel.exe

clean:
	-@ del $(SHAREDOBJS) >NUL 2>&1
	-@ del $(GUIOBJS) >NUL 2>&1
	-@ del $(CLIOBJS) >NUL 2>&1
#	-@ del *.manifest >NUL 2>&1
	-@ del $(BIN)\stunnel.exe >NUL 2>&1
	-@ del $(BIN)\stunnel.exe.manifest >NUL 2>&1
	-@ del $(BIN)\tstunnel.exe >NUL 2>&1
	-@ del $(BIN)\tstunnel.exe.manifest >NUL 2>&1
	-@ rmdir $(OBJ) >NUL 2>&1
	-@ rmdir $(BIN) >NUL 2>&1

makedirs:
	-@ IF NOT EXIST $(OBJROOT) mkdir $(OBJROOT) >NUL 2>&1
	-@ IF NOT EXIST $(OBJ) mkdir $(OBJ) >NUL 2>&1
	-@ IF NOT EXIST $(BINROOT) mkdir $(BINROOT) >NUL 2>&1
	-@ IF NOT EXIST $(BIN) mkdir $(BIN) >NUL 2>&1

$(SHAREDOBJS): *.h vc.mak
$(GUIOBJS): *.h vc.mak
$(CLIOBJS): *.h vc.mak

$(BIN)\stunnel.exe: $(SHAREDOBJS) $(GUIOBJS)
	$(LINK) $(LDFLAGS) $(SHAREDLIBS) $(GUILIBS) /LIBPATH:"$(LIBDIR)" $(SSLLIBS) /OUT:$@ $**
	IF EXIST $@.manifest \
		mt -nologo -manifest $@.manifest -outputresource:$@;1

$(BIN)\tstunnel.exe: $(SHAREDOBJS) $(CLIOBJS)
	$(LINK) $(LDFLAGS) $(SHAREDLIBS) $(CLILIBS) /LIBPATH:"$(LIBDIR)" $(SSLLIBS) /OUT:$@ $**
	IF EXIST $@.manifest \
		mt -nologo -manifest $@.manifest -outputresource:$@;1

# end of vc.mak
