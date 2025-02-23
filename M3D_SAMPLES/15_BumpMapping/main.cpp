//PSP does not support regular pixel shaders.
//BUT, it has a VFPU, a chip made to compute vector operations,
//so this is a custom "HARDWARE PIXEL SHADER" which only works on simple images/textures

#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(){
	M3D_Init(COLOR_5650,0);
	M3D_FrameSkip(0); 
	M3D_SetMipMapping(0,0);
	
    M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(60,60,60,255),RGBA(60,60,60,255));
	M3D_LightSetPosition(0, 1, 0.6, 0);
	
	M3D_Camera *camera = M3D_CameraInit();
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	M3D_CameraSetPosition(camera,0,0,4);
	M3D_CameraSetEye(camera,0,0,0);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_Model *Light = M3D_LoadModelPLY("files/light.ply",0,0);
	M3D_Model *Plane[4];
	Plane[0] = M3D_LoadModelPLY("files/p1.ply",0,0);
	M3D_ModelLoadNormalTexture(Plane[0],0,0,"files/p1_h.png",COLOR_T8);
	Plane[1] = M3D_LoadModelPLY("files/p2.ply",0,0);
	M3D_ModelLoadNormalTexture(Plane[1],0,0,"files/p2_h.png",COLOR_T8);
	Plane[2] = M3D_LoadModelPLY("files/p3.ply",0,COLOR_T4);
	M3D_ModelLoadNormalTexture(Plane[2],0,0,"files/p3_h.png",COLOR_T4);
	
	int model = 0;
	float val = 0;
	while (1){
		M3D_updateScreen(0xff222222);
		M3D_CameraSet(camera);
		M3D_LightSetPosition(0,2*M3D_Cos(val),0.6, 2*M3D_Sin(val));

		val+=0.01;
		ScePspFVector3 Light_Pos = M3D_LightGetPosition(0);
		M3D_ModelSetPosition(Light,0,Light_Pos.x,Light_Pos.y,Light_Pos.z);

		M3D_LightEnable(0);
			M3D_ModelSetRotation(Plane[model],0,45,0,0);
			M3D_ModelRotate(Plane[model],0,0,0.4,0);
			M3D_ModelSetNormalTexture(Plane[model],0,0,camera,0);
			M3D_ModelRender(Plane[model],0);
			M3D_ModelRender(Light,0);
		M3D_LightDisable(0);
		
		//DRAW 2D STUFF
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"   FAKE BUMP/NORMAL MAPPING   ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," L/R = change model.\n Only for plane/flat models");
		M3D_2DMode(0);
		
		M3D_ReadButtons();
		if (M3D_KEYS->pressed.right) model++;
		if (M3D_KEYS->pressed.left)	model--;
		if (model ==  3) model = 0;
		if (model == -1) model = 2;
		
        if (M3D_KEYS->pressed.triangle) M3D_Quit();	
	}
	return 0;
}
