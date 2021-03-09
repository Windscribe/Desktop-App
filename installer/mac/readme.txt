Scripts for building separate projects and creating a complete installer. 
All binaries are placed in the Binaries subfolder after the build. The binaries will already be signed. 
The build order matters, so consider dependencies. Follow the numbering of the steps.

1) Helper is used as Mac daemon. No dependence on other projects. Helper binary copies to WindscribeEngine.app/Contents/Library/LaunchServices folder during WindscribeEngine project build.
	For build run build_helper.sh script. Location: binaries/helper.

2) WindscribeEngine. This project depends on Helper(the Helper project must be built before building the Engine project). 
	For build run build_engine.sh script. Location: binaries/engine.

3) WindscribeLauncher - project for auto launch app on system boot. No dependence on other projects. WindscribeLauncher.app copies to Windscribe.app/Contents/Library/LoginItems folder during Windscribe GUI project build.
	For build - run build_launcher.sh script.  Location: binaries/launcher.

4) wsappcontrol - local http server for communicate with browser. No dependence on other projects. wsappcontrol copies to Windscribe.app/Contents/Library
	For build - run build_wsappcontrol.sh script.  Location: binaries/wsappcontrol.

5) windscribe-cli - Command line executable that communicates with GUI (native messaging) and Engine (Protobufs) for very basic control of Windscribe Application
	For build - run build_cli.sh script. Location: binaries/cli

6) WindscribeGUI. This project depends on WindscribeLauncher, wsappcontrol, windscribe-cli
	For build - run build_gui.sh script.  Location: binaries/gui.

7) Installer. This project depends on WindscribeGUI project. For build - run build_installer.sh script.  Location: binaries/installer.

8) For build all projects (including installer), run build_all.sh script.