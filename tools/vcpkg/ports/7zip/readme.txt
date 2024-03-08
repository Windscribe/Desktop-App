A custom 7zip port for Windscribe that adds the following:
1) Forced static linking which is needed for Windows in particular.
2) Some c files are not included in the build, but they are used by us.

All changes relative to the original vcpkg are marked with comments "#added by Windscribe".