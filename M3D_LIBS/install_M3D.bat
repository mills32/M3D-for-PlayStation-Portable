@echo off
set PSPSDKDIR=C:\PSPDEV\psp\sdk

echo Files will be copied to the following directory:
echo %PSPSDKDIR%\lib and %PSPSDKDIR%\include
echo Please verify that it's correct, or edit this batch file.
pause

echo "Creating release dir"
mkdir release\
mkdir release\include
mkdir release\lib

copy AMGLib\M3D.h release\include
copy AMGLib\libAMG.a release\lib
copy bullet-2.82-r2704\src\libbulletpsp.a release\lib
copy oslibmodv2-master\libosl.a release\lib
cd release\lib

echo "Joining libs"
rem crsT
wsl ./join.sh
pause
rem crsT libM3D.a libAMG.a libbulletpsp.a libosl.a

rem del libAMG.a
rem del libbulletpsp.a
rem del libosl.a
cd ..
cd ..

IF ERRORLEVEL 1 GOTO ERROR1

echo Release folder successfully created.
echo Installing to %PSPSDKDIR%\lib and %PSPSDKDIR%\include
copy release\lib\libM3D.a %PSPSDKDIR%\lib\
copy release\include\M3D.h %PSPSDKDIR%\include\

IF ERRORLEVEL 1 GOTO ERROR2
echo Installed successfully created.
pause
exit

:ERROR1
color c
echo Resease folder failed. Please verify the installation path!
pause
exit

:ERROR2
color c
echo Installation failed. Please verify the installation path!
pause
exit
