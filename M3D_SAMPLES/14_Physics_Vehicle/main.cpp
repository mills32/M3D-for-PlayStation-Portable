#include <M3D.h>

PSP_MODULE_INFO("Hello 3D", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(){
	M3D_Init(COLOR_5551,1);
	M3D_FrameSkip(0); 
	M3D_SetMipMapping(1,-1);
	
    M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(200,200,200,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 0,0.6,1);
	M3D_Camera *camera = M3D_CameraInit();
	M3D_InitMatrixSystem(45.0f,0.5,1800,1);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_Model *Sky = M3D_LoadModelPLY("Files/sky.ply",0,COLOR_5551);
	M3D_Model *Track = M3D_LoadModelPLY("Files/track.ply",0,COLOR_T8);
	M3D_ModelSetTextureFilter(Track,0,0,0,0);
	M3D_Model *Car = M3D_LoadModelPLY("Files/vehicle.ply",0,0);
	M3D_Model *M_wheel = M3D_LoadModelPLY("Files/wheel.ply",0,0);

	M3D_ShadowprojectionSet(64,6);

	M3D_BulletInitPhysics(256, 32);
	M3D_BulletSetGravity(0, -9.8f,0);

	M3D_ModelConfPhysics(Track,0, 0, M3D_BULLET_SHAPE_CONVEXHULL);
	M3D_ModelInitPhysics(Track);

	M3D_ModelSetPosition(Car,0,0,3,0);
	//mass, wradius, wwidth, wfriction, xd, yd
	M3D_VehicleInitPhysics(Car, 5, 0.25, 0.3, 10, 0.6, 0.68);
	M3D_ModelSetMaxVelocity(Car,0,16);

	while (1){
		//UPDATE SCREEN (render, physics etc)
		M3D_updateScreen(RGBA(0, 0, 0,0));
		M3D_BulletUpdatePhysics();
		
		M3D_VehicleSetCam(Car,1.5,7,0.1);
		
		M3D_LightEnable(0);

		M3D_DrawSkyBox(Sky,45);
		
		//M3D_ModelRender(Track,0);
		//M3D_ModelCastShadow(Car,0,120,0);
		ScePspFVector3 Car_Position = M3D_ModelGetPosition(Car,0);
		M3D_ModelStartShadowReceiver(Track,0,Car_Position.x,Car_Position.y,Car_Position.z,160,0);
			M3D_VehicleRenderShadow(Car,0);
		M3D_EndShadowReceiver();
		M3D_VehicleRender(Car,M_wheel);
		
		M3D_LightDisable(0);
		
		//DRAW 2D STUFF
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"         VEHICLE DEMO         ");
			M3D_Printf(Font0,0,240,0xffffffff,0,0,0," X = Velocitator\n O = Deceleratrix\n PAD L R = STEERING");
		M3D_2DMode(0);
		
		//READ CONTROLS
		M3D_ReadButtons();

		//UPDATE VARIABLES
		M3D_VehicleMove(Car,32,2,0.02);

		if (M3D_KEYS->pressed.triangle) M3D_Quit();	
	}
	return 0;
}