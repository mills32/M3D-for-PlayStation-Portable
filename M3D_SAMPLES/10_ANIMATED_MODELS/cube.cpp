///ANIMATION 3D
#include <M3D.h>

PSP_MODULE_INFO("Cube Sample", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU | THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(-1024);


int main(){

    M3D_Init(COLOR_4444,0);
	M3D_DITHER(1);
	M3D_LightSet(0, M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(200,200,200,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 1,1,1);
    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,-3,3,4);
	M3D_CameraSetEye(camera,0,1,0);
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	M3D_SetMipMapping(1,0);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_Model *Mirror = M3D_LoadModelPLY("Files/mirror.ply",0,COLOR_5650);
	
	//Load morphing model
	M3D_MorphingActor *Morph0 = M3D_LoadMorphingActor("Files/morph0.m3m",0,COLOR_4444);
	M3D_MorphingActorConfig(Morph0, 0, 6, 130, 1);
	
	//Load skinned models
	int Model = 0;
	M3D_SkinnedActor *A[6];
	A[0] = M3D_LoadSkinnedActor("Files/dragon.m3a",0.02,COLOR_4444);
	M3D_SkinnedActorConfig(A[0],0,4,40,1,1);
	A[1] = M3D_LoadSkinnedActor("Files/plant.m3a",0.02,COLOR_4444);
	M3D_SkinnedActorConfig(A[1],0,10,60,1,1);
	A[2] = M3D_LoadSkinnedActor("Files/fox.m3a",0.02,COLOR_4444);
	M3D_SkinnedActorConfig(A[2],2,6,30,1,0);
	A[3] = M3D_LoadSkinnedActor("Files/dino.m3a",0.02,COLOR_4444);
	M3D_SkinnedActorConfig(A[3],10,13,10,1,0);
	A[4] = M3D_LoadSkinnedActor("Files/alien.m3a",0.02,COLOR_4444);
	M3D_SkinnedActorConfig(A[4],1,9,20,1,0);
	
	Model = 0;
	float pos_y = 0;
	while(1){
	    M3D_updateScreen(0x66666666);
		M3D_CameraSet(camera);

        M3D_LightEnable(0);
		
		//Draw Skinned models
		if (Model != 5){
			if (Model == 4) M3D_SkinnedActorSetPosition(A[Model],0,1,0);
			else {M3D_SkinnedActorSetPosition(A[Model],0,0,0);
				M3D_ModelSetPosition(Mirror,0,0,0,0);
				M3D_SkinnedActorSetPosition(A[Model],0,M3D_Sin(pos_y+=0.04)/2,0);
				M3D_StartReflection(Mirror,0);
					M3D_SkinnedActorRenderMirror(A[Model],1);
				M3D_FinishReflection();
			}
			M3D_SkinnedActorRender(A[Model]);
		}
		
		//Draw morphing model
		if (Model == 5) {
			M3D_MorphingActorSetPosition(Morph0,0,1,0);
			M3D_MorphingActorRotate(Morph0,0,0.1,0.2);
			M3D_MorphingActorRender(Morph0);
		}
			
		M3D_LightDisable(0);

		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"  MORPHING & SKINNED  MODELS  ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," LEFT/RIGHT to change model\n L TRIGGER / R TRIGGER TO ROTATE");
			if (Model != 5) M3D_Printf(Font0,10,64,0xffffffff,0,0,0,"SKINNED MODEL");
			if (Model == 5) M3D_Printf(Font0,10,64,0xffffffff,0,0,0,"MORPHING MODEL");
		M3D_2DMode(0);		

		M3D_ReadButtons();
		if (M3D_KEYS->pressed.left) {Model -= 1;} 
		if (M3D_KEYS->pressed.right) {Model += 1;}
		if (Model != 5){
			if (M3D_KEYS->held.L) M3D_SkinnedActorRotate(A[Model],0,-1,0);
			if (M3D_KEYS->held.R) M3D_SkinnedActorRotate(A[Model],0,+1,0);
		}
		if (Model == 6) Model = 0;
		if (Model == -1) Model = 5;
        if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	return 0;
}