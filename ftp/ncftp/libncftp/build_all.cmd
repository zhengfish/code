::
::
::

@echo off

@echo build sio.lib
cd /d %~dp0
cd sio
msbuild sio.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build Strn.lib
cd /d %~dp0
cd Strn
msbuild Strn.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp.lib
cd /d %~dp0
cd libncftp
msbuild LibNcFTP.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpget\ncftpget
cd /d %~dp0
cd libncftp\samples\ncftpget
msbuild ncftpget.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\ncftpput
cd /d %~dp0
cd libncftp\samples\ncftpput
msbuild ncftpput.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\minincftp
cd /d %~dp0
cd libncftp\samples\minincftp
msbuild minincftp.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\misc
cd /d %~dp0
cd libncftp\samples\misc
msbuild misc.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\simpleget
cd /d %~dp0
cd libncftp\samples\simpleget
msbuild simpleget.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\ntftpls
cd /d %~dp0
cd libncftp\samples\ntftpls
msbuild ntftpls.vcxproj /t:Build /p:Configuration=release;Platform=x86

@echo build libncftp\samples\ncftpput\ncftpsyncput
cd /d %~dp0
cd libncftp\samples\ncftpsyncput
msbuild ncftpsyncput.vcxproj /t:Build /p:Configuration=release;Platform=x86

cd /d %~dp0

::___END___
