::
::
::

@echo off

@echo build zlib.lib
cd /d %~dp0
cd zlib
nmake -f win32/Makefile.msc

@echo build libfilezilla.lib
cd /d %~dp0
msbuild libfilezilla/lib/libfilezilla.vcxproj /t:Build /p:Configuration=dll_release;Platform=x86

@echo build filezillaserver.exe
cd /d %~dp0


::___END___
