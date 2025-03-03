/*
 * PSP Software Development Kit - https://github.com/pspdev
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * Motion JPEG video playback
 *
 * For 60fps video use CPU speeds > 166MHz 
 *
 * Video1: by Mills (Music: Arpent by Kevin McLeon).
 * Video2:
 */


#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(int argc, char* argv[]){
	M3D_Init(COLOR_8888,0);
	
	M3D_VIDEO_PlayH264("Files/bad_apple60_H264.avi");
	M3D_VIDEO_PlayFullScreenMJPEG("Files/Video_MJPEG1.avi",0);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Screen = M3D_TextureCreate(256,256,COLOR_8888,M3D_IN_VRAM);
	int MyTexture_Video = M3D_VIDEO_LoadMJPEG("Files/Video_MJPEG2.avi");
	M3D_VIDEO_MJPEGToTexture_Start(MyTexture_Video,1,Screen);
	
	int x = 0;
	while (1){
		M3D_updateScreen(0x00000000);
		
		M3D_DrawSprite(Screen,x++,0,0,0,0,0);
		if (x > 480) x = -256; 

		M3D_2DMode(1);
		M3D_Printf(Font0, 8,8,0xffffffff,0,0,0,"DECODE VIDEO TO TEXTURE");
		M3D_2DMode(0);
	}
	M3D_VIDEO_MJPEGToTexture_Stop(Screen);
	return 0;
}

