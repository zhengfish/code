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
cd libfilezilla
msbuild lib/libfilezilla.vcxproj /t:Build /p:Configuration=dll_release;Platform=x86
msbuild lib/libfilezilla.vcxproj /t:Build /p:Configuration=static_release;Platform=x86

@echo build filezillaserver.exe
cd /d %~dp0
cd filezillaserver
msbuild filezillaserver.vcxproj /t:Build /p:Configuration=release;Platform=Win32

cd /d %~dp0

::___END___
