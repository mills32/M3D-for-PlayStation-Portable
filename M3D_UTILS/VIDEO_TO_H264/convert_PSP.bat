@echo off
SET VIDEO_NAME=%1%
SET CODEC=libx264 -profile:v main -x264-params
rem x264 parameters PSP MENU & HOMEBREW PLAYER, don't forget to use -x264-params quotes
SET _OPT=ref=2:b_pyramid=0:8x8dct=0:bframes=0
rem other parameters
SET GOPS=:keyint=65536:keyint_min=65536
SET BITRATE=:bitrate=500:vbv-maxrate=600:vbv-bufsize=1000


rem Try to fix timestap errors because videos will look like crap if ffmpeg shows "Non-monotonous DTS"
SET MP4VERSION=-use_wallclock_as_timestamps 1

rem variables
SET CONTAINER=mp4
SET VIDEO_CONTAINER=0
SET AUDIO=0
SET AUDIO_FORMAT=-c:a aac -ar 44100 -ab 128k
SET SIZE=0
SET SIZE_FILTERS=
SET RATE=500

rem convert
echo. 
echo CONVERT VIDEO FOR PLAYSTATION PORTABLE
echo --------------------------------------
echo.                                          
echo INPUT VIDEO = %VIDEO_NAME%


echo.   
echo SELECT VIDEO CONTAINER (default AVI):  
echo - 0 AVI (Homebrew player);
echo - 1 MP4 (XMB menu player);
set /p VIDEO_CONTAINER=CONTAINER: 
if %VIDEO_CONTAINER% equ 0 (set CONTAINER=avi)
if %VIDEO_CONTAINER% equ 1 (set CONTAINER=mp4)
if %VIDEO_CONTAINER% equ 0 (SET GOPS=:keyint=65536:keyint_min=65536)
if %VIDEO_CONTAINER% equ 1 (SET GOPS=:keyint=240:keyint_min=24)
if %VIDEO_CONTAINER% equ 0 (set MP4VERSION= )
if %VIDEO_CONTAINER% gtr 1 (set CONTAINER=avi)
if %VIDEO_CONTAINER% gtr 1 (SET GOPS=:keyint=65536:keyint_min=65536)

echo.   
echo SELECT AUDIO FORMAT (default AAC):  
echo - 0 AAC;
echo - 1 UNCHANGED: when input is AAC and rate less than 256k;  
set /p AUDIO=FORMAT: 
if %AUDIO% equ 0 (set AUDIO_FORMAT=-c:a aac -ar 44100 -ab 128k -ac 2)
if %AUDIO% equ 1 (set AUDIO_FORMAT=-c:a copy )
if %AUDIO% gtr 1 (set AUDIO_FORMAT=-c:a aac -ar 44100 -ab 128k -ac 2)

echo.   
echo SELECT VIDEO SIZE:  
echo - 0 480x272;
echo - 1 720x480;       
set /p SIZE=VIDEO SIZE:
if %SIZE% equ 0 (set SIZE_FILTERS=-vf "scale=-2:272,pad=720:272:(ow-iw)/2:(oh-ih)/2,crop=480:272")
if %SIZE% equ 1 (set SIZE_FILTERS=-vf "scale=-2:480,pad=1280:480:(ow-iw)/2:(oh-ih)/2,crop=720:480")
rem 816x516

echo.   
echo SELECT VIDEO QUALITY IN KBPS (default 500; MAX 2500):  
set /p RATE=VIDEO QUALITY: 
set /a BUF=%RATE%/2
set BITRATE=:bitrate=%RATE%:vbv-maxrate=%RATE%:vbv-bufsize=%BUF%
rem
ffmpeg -i %VIDEO_NAME% -c:v %CODEC% "%_OPT%%GOPS%%BITRATE%" %SIZE_FILTERS% %AUDIO_FORMAT% psp_video.%CONTAINER%
pause