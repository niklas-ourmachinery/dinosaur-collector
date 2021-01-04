@echo off
if not defined TM_SDK_DIR (
echo TM_SDK_DIR environment variable is not set. Please point it to your The Machinery directory.
pause
exit
)
%TM_SDK_DIR%/bin/tmbuild.exe
if NOT ["%errorlevel%"]==["0"] pause