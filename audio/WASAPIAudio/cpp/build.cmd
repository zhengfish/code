::
::
::

@echo off

if "%VisualStudioVersion%"=="14.0" (
msbuild WASAPIAudio.sln /m /t:Build /p:Configuration=Debug /p:Platform=x86
) else if "%VisualStudioVersion%"=="15.0" (
msbuild WASAPIAudio.sln /m /t:ReBuild /p:Configuration=Debug /p:Platform=x86
) else (
echo "No VisualStudioVersion"
)


::___END___
