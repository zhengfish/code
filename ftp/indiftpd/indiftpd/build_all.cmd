::
::
::

@echo off

@echo build filezillaserver.exe
cd /d %~dp0
msbuild indiftpd.sln /t:Build /p:Configuration=Release;Platform=x86

cd /d %~dp0

::___END___
