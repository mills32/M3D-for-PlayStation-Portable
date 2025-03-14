#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

int main(){
	int ram1 = M3D_GetFreeRAM();
	int i;
	M3D_Init(COLOR_8888,0);
	M3D_FrameSkip(0); 
	M3D_SetMipMapping(1,0.6);
	M3D_InitMatrixSystem(45,0.1,30,0);
    M3D_LightSet(0, M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(20,20,20,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0,-1,1,-1);
    M3D_Camera *camera = M3D_CameraInit();
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	
	M3D_Model *Ball = M3D_LoadModelPLY("Files/ball.ply",0,COLOR_T4);
	M3D_Model *Ground = M3D_LoadModelPLY("Files/maze.ply",0,COLOR_T4);

	M3D_Texture *Shine = M3D_LoadTexture("files/shine.png",0,COLOR_4444);
	M3D_TextureSetMapping(Shine,M3D_ENVIRONMENT_MAP, 1, 2);
	M3D_ModelSetMultiTexture(Ground,0,0,Shine);

	M3D_BulletInitPhysics(256, 4);
	M3D_BulletSetGravity(0, -20.0f,0);
	
	M3D_ModelConfPhysics(Ground,0, 0, M3D_BULLET_SHAPE_CONVEXHULL);
	M3D_ModelInitPhysics(Ground);
	
	//SET BALL 
	M3D_ModelConfPhysics(Ball,0, 0.1, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Ball,0,5.5,1,7.5);
	M3D_ModelInitPhysics(Ball);

	while(1){
		M3D_updateScreen(0x00000000);
		M3D_ReadButtons();
		M3D_BulletUpdatePhysics();

		ScePspFVector3 Ball_Pos = M3D_ModelGetPosition(Ball,0);
		M3D_CameraSetPosition(camera,Ball_Pos.x,Ball_Pos.y+3,Ball_Pos.z+2);
		M3D_CameraSetEye(camera,Ball_Pos.x,Ball_Pos.y,Ball_Pos.z);
		M3D_CameraSet(camera);
		
		M3D_ModelSetOrigin(Ground,0,Ball_Pos.x,Ball_Pos.y,Ball_Pos.z);//overrides origin set by M3D_ModelTranslate and M3D_ModelSetPosition
		M3D_ModelSetRotation(Ground,0,M3D_KEYS->analogY/12,0,M3D_KEYS->analogX/-12);
	
		M3D_LightEnable(0);
			M3D_ModelRender(Ground,0);
			M3D_ModelCastShadow(Ball,0,100,1,0);
			M3D_ModelRender(Ball,0);
		M3D_LightDisable(0);

		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"       PHYSICS (BULLET)      ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0,"DIR PAD = move ball");
		M3D_2DMode(0);	

		if (Ball_Pos.y < -20){
			M3D_ModelDeletePhysics(Ball);
			M3D_ModelConfPhysics(Ball,0, 0.1, M3D_BULLET_SHAPE_SPHERE);
			M3D_ModelSetPosition(Ball,0,5.5,1,7.5);
			M3D_ModelInitPhysics(Ball);
		}

		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	
	return 0;
}
