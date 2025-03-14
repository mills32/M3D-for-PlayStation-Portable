/*
3D STUFF
*/

//#include <oslib/oslib.h>
//  #include <AMG/AMGLib.h>
#include <M3D.h>

PSP_MODULE_INFO("Hello 3D", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);



int main(){
	int USER_RAM = M3D_GetFreeRAM();
	int USER_RAM1 = 0;
	int USER_VRAM = M3D_GetTotalVRAM();
	int USER_VRAM1 = 0;
	M3D_Init(COLOR_5650,0);
	M3D_DITHER(1);
	
    M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(200,200,200,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 0,1,1);
	
	M3D_Camera *camera = M3D_CameraInit();
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	
	M3D_SetMipMapping(1,0.6);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_Model *RedGear = M3D_LoadModelPLY("Files/redgear.ply",0,0);
	M3D_Model *GreenGear = M3D_LoadModelPLY("Files/greengear.ply",0,0);
	M3D_Model *BlueGear = M3D_LoadModelPLY("Files/bluegear.ply",0,0);
	M3D_Model *Donut = M3D_LoadModelPLY("Files/donut.ply",0,0);
	M3D_Model *Teapot = M3D_LoadModel("Files/teapot.obj",0,COLOR_4444);
	M3D_Model *PlasmaCube = M3D_LoadModelPLY("Files/plasmacube.ply",0,1);
	M3D_Texture *Plasma = M3D_PlasmaTextureCreate("files/plasma_palette.png");
	M3D_ModelSetTexture(PlasmaCube,0,0,Plasma);
	
	M3D_ModelSetPosition(RedGear,0,-3,-2.0, 0.0);
	M3D_ModelSetPosition(GreenGear,0,3.1,-2.0, 0.0);
	M3D_ModelSetRotation(GreenGear,0,0,0,-9);
	M3D_ModelSetPosition(BlueGear,0,-3.1,4.2, 0.0);
	M3D_ModelSetRotation(BlueGear,0,0,0,-25);
	M3D_ModelSetPosition(Donut,0,4,4,8);
	M3D_ModelSetPosition(Teapot,0,4,1,15);
	M3D_ModelSetPosition(PlasmaCube,0,-4,3,14);
	
	int model = 0;
	float angle = 0;
	float angle2 = 0;
	int FREE_RAM = M3D_GetFreeRAM();

	while (1){
		M3D_updateScreen(RGBA(0, 0, 0,0));
		
		M3D_CameraSet(camera);
		M3D_CameraSetPosition(camera,-10,10,20);
		M3D_CameraSetEye(camera,10,-4,0);

		M3D_LightEnable(0);

		M3D_ModelRotate(RedGear,0,0,0,1);
		M3D_ModelRender(RedGear,0);
		M3D_ModelRotate(GreenGear,0,0,0,-2);
		M3D_ModelRender(GreenGear,0);
		M3D_ModelRotate(BlueGear,0,0,0,-2);
		M3D_ModelRender(BlueGear,0);
		M3D_ModelRotate(Donut,0,0,-0.2,0.3);
		M3D_ModelRender(Donut,0);
		M3D_ModelRotate(Teapot,0,0,+0.2,0.3);
		M3D_ModelRender(Teapot,0);
		M3D_PlasmaTextureUpdate(Plasma,3,1,0.5,2);
		M3D_ModelRotate(PlasmaCube,0,0,0.1,-0.1);
		M3D_ModelSetOrigin(PlasmaCube,0,-4,2,14);//overrides origin set by Translate/Rotate functions
		M3D_ModelRender(PlasmaCube,0);

		M3D_LightDisable(0);
		
		//DRAW 2D STUFF
		M3D_2DMode(1);
			M3D_Printf(Font1,128, 8,0xffddffdd,0,0,0,"BASIC 3D %f",M3D_Sin(M3D_Deg2Rad(angle2)));
			M3D_Printf(Font0,28,228,0xffffffff,0,0,0,"RENDER MODE %s\nVIDEO RAM TOTAL: %i KB\nVIDEO RAM  USED: %i KB",M3D_GetScreenMode(),USER_VRAM,M3D_GetUsedVRAM());
			M3D_Printf(Font0,28,252,0xffffffff,0,0,0,"MAIN RAM  USER: %i KB\nMAIN RAM  FREE: %i KB",USER_RAM,FREE_RAM);
			M3D_DrawLine(8,24,0xff00FF00,480-8,24,0xffff0000);
		M3D_2DMode(0);
		
		//READ CONTROLS
		M3D_ReadButtons();

		//UPDATE VARIABLES
		angle+=0.01;
		angle2+=0.5;
		if (model ==  3) model = 0;
		if (model == -1) model = 2;
        if (M3D_KEYS->pressed.triangle) M3D_Quit();	
	}
	return 0;
}
