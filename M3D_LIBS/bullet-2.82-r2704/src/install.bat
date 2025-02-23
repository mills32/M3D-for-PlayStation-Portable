@echo off
set PSPSDKDIR=C:\pspdev\psp\sdk

echo Files will be copied to the following directory:
echo %PSPSDKDIR%\lib and %PSPSDKDIR%\include
echo Please verify that it's correct, or edit this batch file.
pause

echo "Creating release dir"
mkdir release\
mkdir release\include
mkdir release\lib
copy AMG_Config.h release\include\
copy AMG_3D.h release\include\
copy AMG_Model.h release\include\
copy AMG_Physics.h release\include\
copy AMG_Texture.h release\include\
copy AMG_triParticle.h release\include\
copy AMG_Thread.h release\include\
copy AMG_User.h release\include\
copy AMGLib.h release\include\
copy lightmap.h release\include\
copy AMG_Multimedia.h release\include\
copy libAMG.a release\lib
IF ERRORLEVEL 1 GOTO ERROR1
echo Release folder successfully created.
echo Installing to %PSPSDKDIR%\lib and %PSPSDKDIR%\include
pause

rem mover a PSPDEV
mkdir %PSPSDKDIR%\include\AMG
copy libAMG.a %PSPSDKDIR%\lib\
copy release\include\AMG_Config.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_3D.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_Model.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_Physics.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_Texture.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_triParticle.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_Thread.h %PSPSDKDIR%\include\AMG\
copy release\include\lightmap.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_User.h %PSPSDKDIR%\include\AMG\
copy release\include\AMG_Multimedia.h %PSPSDKDIR%\include\AMG\
copy release\include\AMGLib.h %PSPSDKDIR%\include\AMG\

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
