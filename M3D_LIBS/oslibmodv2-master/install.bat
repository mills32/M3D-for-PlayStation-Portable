@echo off

rem This defines the installation directory, please change it:
set PSPSDKDIR=C:\pspdev\psp\sdk

echo Files will be copied to the following directory:
echo %PSPSDKDIR%
echo Please verify that it's correct, or edit this batch file.
pause

echo
echo "---------------"
echo "OSLib installer"
echo "---------------"
echo "Creating directories...."
mkdir %PSPSDKDIR%\include\oslib
mkdir %PSPSDKDIR%\include\oslib\intraFont
mkdir %PSPSDKDIR%\include\oslib\libpspmath
mkdir %PSPSDKDIR%\include\oslib\adhoc

echo "Copying lib...."
copy libosl.a %PSPSDKDIR%\lib\
IF ERRORLEVEL 1 GOTO ERROR

echo "Copying header files...."
copy src\intraFont\intraFont.h %PSPSDKDIR%\include\oslib\intraFont\
copy src\intraFont\libccc.h %PSPSDKDIR%\include\oslib\intraFont\
copy src\libpspmath\pspmath.h %PSPSDKDIR%\include\oslib\libpspmath\
copy src\adhoc\pspadhoc.h %PSPSDKDIR%\include\oslib\adhoc\
copy src\oslmath.h %PSPSDKDIR%\include\oslib\
copy src\net.h %PSPSDKDIR%\include\oslib\
copy src\browser.h %PSPSDKDIR%\include\oslib\
copy src\audio.h %PSPSDKDIR%\include\oslib\
copy src\bgm.h %PSPSDKDIR%\include\oslib\
copy src\dialog.h %PSPSDKDIR%\include\oslib\
copy src\drawing.h %PSPSDKDIR%\include\oslib\
copy src\keys.h %PSPSDKDIR%\include\oslib\
copy src\map.h %PSPSDKDIR%\include\oslib\
copy src\messagebox.h %PSPSDKDIR%\include\oslib\
copy src\osk.h %PSPSDKDIR%\include\oslib\
copy src\saveload.h %PSPSDKDIR%\include\oslib\
copy src\oslib.h %PSPSDKDIR%\include\oslib\
copy src\text.h %PSPSDKDIR%\include\oslib\
copy src\usb.h %PSPSDKDIR%\include\oslib\
copy src\vfpu_ops.h %PSPSDKDIR%\include\oslib\
copy src\VirtualFile.h %PSPSDKDIR%\include\oslib\
copy src\vram_mgr.h %PSPSDKDIR%\include\oslib\
copy src\ccc.h %PSPSDKDIR%\include\oslib\
copy src\sfont.h %PSPSDKDIR%\include\oslib\

IF ERRORLEVEL 1 GOTO ERROR
echo Installation completed successfully.
pause
exit

:ERROR
color c
echo Installation failed. Please verify the installation path!
pause