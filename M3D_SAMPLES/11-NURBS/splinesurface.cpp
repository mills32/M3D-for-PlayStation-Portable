#include <M3D.h>

PSP_MODULE_INFO("NURBS", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(int argc, char* argv[]){
	
	M3D_Init(COLOR_5650,0);
	M3D_SetMipMapping(1,-1);
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(20,20,20,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 1, 0.6, 0);
	
	M3D_Camera *camera = M3D_CameraInit();
	M3D_InitMatrixSystem(45.0f,0.5,100,1);

	M3D_NurbsSurface *Surface = M3D_CreateNurbsSurface("texture.png",4,7);
	//M3D_NurbsDelete(Surface);

	// run sample
	float angle = 0;
	int mode = 0;
	int tmode = 0;
	while(1){
		M3D_updateScreen(0x00000000);
		M3D_CameraSetPosition(camera,0,3,4);
		M3D_CameraSetEye(camera,0,0,0);
		M3D_CameraSet(camera);

		M3D_LightEnable(0);	

		M3D_NurbsRotate(Surface,0,0.01,0);
		if (mode == 2) M3D_NurbsSurfaceSet(Surface,mode,-2,-2,1,0,0.1,angle);
		else M3D_NurbsSurfaceSet(Surface,mode,0,0,0,0,0.4,angle);
		M3D_NurbsSurfaceRender(Surface);
		
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"   HARDWARE NURBS SURFACES   ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," X to change surface mode; O to change texture mapping");
		M3D_2DMode(0);	
		
		M3D_ReadButtons();
		if (M3D_KEYS->pressed.cross) {mode++;if(mode==3)mode = 0;}
		if (M3D_KEYS->pressed.circle) {tmode++;M3D_NurbsSetMapping(Surface,tmode&1);}
		if (M3D_KEYS->pressed.square) M3D_NurbsSetMapping(Surface,0);
		if (M3D_KEYS->pressed.triangle) M3D_NurbsSetMapping(Surface,1);
		
		angle+=0.1;
	}
	
	return 0;
}
