///TV
#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

int main(int argc, char **argv){

	// Inicializa la PSP
	M3D_Init(COLOR_5650,1);
	M3D_DITHER(1);
	//Light
    M3D_LightSet(0, M3D_LIGHT_DIRECTIONAL,0x0fffffff,0x0f444444,0x0f444444);
	M3D_LightSetPosition(0,-1,1,1);
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	
    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0,0,25);
	M3D_CameraSetEye(camera,0,0,0);
	
	M3D_SetMipMapping(1,0.4);
	
	M3D_Model *TVS = M3D_LoadModelPLY("Files/tvs.ply",0,COLOR_T4);
	M3D_Model *_3D = M3D_LoadModelPLY("Files/_3d.ply",0,COLOR_T4);
	M3D_Model *Screens = M3D_LoadModelPLY("Files/screens.ply",0,0);
	M3D_Model *Crate = M3D_LoadModelPLY("Files/crate.ply",0,COLOR_T4);
	M3D_Texture *Screen_Image = M3D_RenderTextureCreate(64,64);
	M3D_Texture *VideoTexture = M3D_TextureCreate(64,64,COLOR_8888,M3D_IN_RAM);
	int MyTexture_Video = M3D_VIDEO_LoadMJPEG("Files/PSP64x64.avi");
	
	M3D_Texture *Font8 = M3D_GetFont(0);
	M3D_Texture *Font = M3D_GetFont(1);
	
	M3D_Texture *reflect_texture = M3D_LoadTexture("Files/city_ref.png",0,COLOR_4444);
	M3D_TextureSetMapping(reflect_texture,M3D_ENVIRONMENT_MAP, 1, 2);
	M3D_ModelSetMultiTexture(TVS,0,0,reflect_texture);
	M3D_ModelSetMultiTexture(Screens,0,0,reflect_texture);
	//For env map textures on top of coloured vertices (ply and binary) makes them less dark.
	//TVS->Object[0].Group[0].MultiTexture->TFX = GU_TFX_REPLACE;

	M3D_ModelSetTexture(Screens,0,0,Screen_Image);
	M3D_ModelSetLighting(Screens,0,0);

	//Start video playback to texture
	M3D_VIDEO_MJPEGToTexture_Start(MyTexture_Video,1,VideoTexture);
	
	float angle2 = 0.0f;
	float wavetext = 0.0f;
	while(1){
		M3D_updateScreen(0xffb290de);

		M3D_LightEnable(0);
		M3D_CameraSet(camera);
		
		//Render things to "Screen_Image" texture (Simulated CRT diplay)
		M3D_RenderToTextureEnable(Screen_Image);
			M3D_2DMode(1);
			//Draw the video which is being decoded in another thread
			M3D_DrawSprite(VideoTexture,0,0,0,0,0,0);
			M3D_2DMode(0);
			//Draw a 3D object on top of the video
			M3D_ModelRotate(Crate,0,0,1,2);
			M3D_ModelSetPosition(Crate,0,0,M3D_Sin(angle2+=0.03)*16,0);
			M3D_ModelRender(Crate,0);
			//Draw some old style CRT OSD text
			M3D_2DMode(1);
			M3D_Print(Font8,2,2,0xff00ff00,0,0,0,(char*)"CH1");
			M3D_2DMode(0);
		//Finish renderind to CRT screen, go back to PSP screen
		M3D_RenderToTextureDisable();
		
		//Draw a simple background
		M3D_2DMode(1);
		M3D_DrawGradientRect(0,0,M3D_ScreenX,M3D_ScreenY,RGBA(149,72,172,255),RGBA(219,119,206,255),RGBA(162,118,208,255),RGBA(58,57,146,255));
		M3D_2DMode(0);
		
		//Draw a bunch of CRTS, logos and CRT Screens, which will display the rendered stuff (Screen_Image)
		M3D_ModelRotate(TVS,0,0,0.3,0);
		M3D_ModelRotate(_3D,0,0,0.3,0);
		M3D_ModelRotate(Screens,0,0,0.3,0);
		M3D_ModelRender(TVS,0);
		M3D_ModelRender(Screens,0);
		M3D_ModelRender(_3D,0);
		
		M3D_LightDisable(0);

		//Draw some info text
		M3D_2DMode(1);
			M3D_DrawSprite(Screen_Image,0,0,0,0,0,0);
			M3D_Printf(Font,16*8,16,0xffffffff,8,1,wavetext,"RENDER TO TEXTURE");
			M3D_Printf(Font,16*4,240,0xffffffff,20,2,wavetext,"RENDER TO 64x64 TEXTURE");
		M3D_2DMode(0);
		
		wavetext += 0.1;
		
		M3D_ReadButtons();	
		if (M3D_KEYS->held.up) M3D_CameraMove(camera,0,0,-0.1);
		if (M3D_KEYS->held.down) M3D_CameraMove(camera,0,0,+0.1);
        if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	return 0;
}
