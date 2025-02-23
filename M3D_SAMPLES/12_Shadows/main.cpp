
#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(){
	float ang = 0;
	float MODE = 0;
	
    M3D_Init(COLOR_5551,0);	
	M3D_FrameSkip(0);
	M3D_DITHER(1);

	M3D_LightSet(0, M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(180,180,180,255),RGBA(90,90,90,255));
	M3D_LightSetPosition(0, 1,1,1);

    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0,4,0.01);
	M3D_CameraSetEye(camera,0,1,0);
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	M3D_SetMipMapping(1,0.6);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_Model *Plane = M3D_LoadModel("Files/BKG.obj",0,COLOR_T4);
	M3D_Model *Suzane = M3D_LoadModel("Files/suzane.obj",0,COLOR_4444);
	M3D_Model *Palm = M3D_LoadModel("Files/Palm.obj",0,COLOR_4444);
	
	M3D_ModelSetPosition(Suzane,0,-5,2,0);
	M3D_ModelSetPosition(Palm,0,5,0,-1);
	//M3D_ModelSetPosition(Tetra,0,-1,2,3);
	M3D_SkinnedActor *A = M3D_LoadSkinnedActor("Files/dragon.m3a",0.01,COLOR_4444);
	M3D_SkinnedActorConfig(A,0,4,50,1,0);
	M3D_SkinnedActorSetPosition(A,0,0,0);

	M3D_ShadowprojectionSet(128,18);
	
	while (1){
		M3D_updateScreen(0xFFFFFFFF);
		M3D_CameraSetPosition(camera,0,8,10);
		M3D_CameraSetEye(camera,0,1,0);
		M3D_CameraSet(camera);

		M3D_LightEnable(0);
			if(MODE == 1){
				M3D_ModelStartShadowReceiver(Plane,0,0,0,0,160,0);
					M3D_SkinnedActorRenderShadow(A);
					M3D_ModelRenderShadow(Suzane,0);
					M3D_ModelRenderShadow(Palm,0);
				M3D_EndShadowReceiver();
			}
			if(MODE == 0){
				M3D_ModelRender(Plane,0);
				M3D_ModelCastShadow(Suzane,0,130,1,0);
				M3D_ModelCastShadow(Palm,0,130,1,0);
				M3D_SkinnedActorCastShadow(A,130,1,0);
			}
			
			M3D_SkinnedActorRender(A);
			M3D_ModelRender(Suzane,0);
			M3D_ModelRender(Palm,0);
		M3D_LightDisable(0);

		M3D_LightSetPosition(0,M3D_Sin(ang),1,M3D_Cos(ang));
		
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"           SHADOWS           ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," X = Simple stencil shadow\n O = Real shadows");
		M3D_2DMode(0);	
		
		ang+=0.02;
		
		M3D_ReadButtons();
		if (M3D_KEYS->pressed.circle) MODE = 1;	 
		if (M3D_KEYS->pressed.cross) MODE = 0;	 
        if (M3D_KEYS->pressed.triangle) M3D_Quit();	    
	}
	return 0;
}
