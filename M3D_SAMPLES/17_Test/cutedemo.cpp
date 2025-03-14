///ANIMATION 3D
#include <M3D.h>

PSP_MODULE_INFO("Sample", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU | THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(-1024);


int main(){
    M3D_Init(COLOR_5650,0);
	M3D_DITHER(0);
	M3D_FrameSkip(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	//light
	M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(255,255,255,255),RGBA(60,60,60,255));
	M3D_LightSetPosition(0,-0.4,0.6,0.2);
  
	// Crea una camara
    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0.2,0.6,1.2);
	M3D_CameraSetEye(camera,0,0.2,0);

	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	M3D_SetMipMapping(1,0.3);
	
	// load
	M3D_NurbsSurface *Water = M3D_CreateNurbsSurface("Files/sea.png",COLOR_T4,12,16);
	M3D_Texture *Brillo = M3D_LoadTexture("Files/shine.png",0,COLOR_4444);
	M3D_TextureSetMapping(Brillo,M3D_ENVIRONMENT_MAP, 1, 2);

	M3D_Model *Ship = M3D_LoadModel("Files/ship.obj",0.01,COLOR_4444);
	M3D_NurbsSetMapping(Water,1);
	
	M3D_Model *Sky = M3D_LoadModel("Files/sky.obj",0,COLOR_5650);
	M3D_ModelSetLighting(Sky,0,0);
	M3D_ModelSetMultiTexture(Ship,0,0,Brillo);
	
	M3D_ShadowprojectionSet(128,2);

	float Angle = 0;
	float Angle2 = 0;
	float water_motion = 0;
	u8 posx = 0; u8 posy = 0;
	while(1){
	    M3D_updateScreen(0xffffffff);
		
		M3D_CameraSet(camera);

        M3D_LightEnable(0);	
		
		M3D_FogEnable(3, 5,0xffe2a95c);
		M3D_NurbsSurfaceSet(Water,2,-2,-2,1,0,0.24,water_motion);
		M3D_NurbsStartShadowReceiver(Water,0,0,0,150,0);
			M3D_ModelRenderShadow(Ship,0);
		M3D_EndShadowReceiver();
		
		M3D_FogDisable();
		M3D_ModelRender(Sky,0);
		M3D_ModelRender(Ship,0);
		
		//M3D_NurbsSurfaceRender(Water);
		
		M3D_LightDisable(0);

		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"DEMO");
		M3D_2DMode(0);		

		//Water1->Object[0].Rot.y += 0.02;
		M3D_CameraSetEye(camera,0,(M3D_Sin(Angle)*0.1)+0.2f,0);
		M3D_CameraSetUp(camera,M3D_Cos(Angle)*0.1,1,0);
		//camera->Up.y = AMG_Sin(Angle)*0.2;
		M3D_ModelSetRotation(Ship,0, 0, 0.12+M3D_Sin(Angle)*5,M3D_Sin(Angle2)*10);
		
		Angle+= 0.02;
		Angle2+= 0.04;
		water_motion += 0.04;
		posx++; posy++;
		//exit
		M3D_ReadButtons();
        if (M3D_KEYS->pressed.triangle)M3D_Quit();
	}
	return 0;
}