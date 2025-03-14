#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);


//Drive parameters
float EngineForce = 0;
float BrakeForce = 0;
float Steering = 0;

int Coconut_Timer = 120;
int Coconut_Number = 0;
int Boxes_Timer = 120;
int Boxes_Number = 0;
M3D_Model *Coconut[5];
M3D_Model *Boxes[5];

void Spawn_Things(){
	float mass = 0.04;
	float friction = 2;
	float rollfriction = 1;
	float restitution = 0;
	if (Coconut_Timer == 120){
		M3D_ModelDeletePhysics(Coconut[Coconut_Number]);
		M3D_ModelSetProperties(Coconut[Coconut_Number],0,friction,rollfriction,restitution);
		M3D_ModelConfPhysics(Coconut[Coconut_Number],0, mass, M3D_BULLET_SHAPE_CONE);
		M3D_ModelSetPosition(Coconut[Coconut_Number],0,0,1,-16);
		M3D_ModelInitPhysics(Coconut[Coconut_Number]);
		Coconut_Timer = 0;
		Coconut_Number++;
		if (Coconut_Number == 3) Coconut_Number = 0;
	}
	Coconut_Timer++;
}
void Spawn_Things2(){
	float mass = 0.1;
	float friction = 2;
	float rollfriction = 1;
	float restitution = 0;
	if (Coconut_Timer == 120){
		M3D_ModelDeletePhysics(Boxes[Boxes_Number]);
		M3D_ModelSetProperties(Boxes[Boxes_Number],0,friction,rollfriction,restitution);
		M3D_ModelConfPhysics(Boxes[Boxes_Number],0, mass, M3D_BULLET_SHAPE_BOX);
		M3D_ModelSetPosition(Boxes[Boxes_Number],0,1,1,-5);
		M3D_ModelInitPhysics(Boxes[Boxes_Number]);
		Boxes_Timer = 0;
		Boxes_Number++;
		if (Boxes_Number == 3) Boxes_Number = 0;
	}
	Boxes_Timer++;
}


int main(){
	int ram1 = M3D_GetFreeRAM();
	int i;
	M3D_Init(COLOR_5551,0);
	M3D_FrameSkip(0); 
	M3D_SetMipMapping(1,0.6);
	M3D_InitMatrixSystem(45,0.1,30,0);
    M3D_LightSet(0, M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(200,200,200,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 0.3,1,0.3);
    M3D_Camera *camera = M3D_CameraInit();
	
	M3D_Model *Mirror = M3D_LoadModelPLY("Files/mirror.ply",0,COLOR_4444);
	M3D_ModelSetPosition(Mirror,0,0,-2.98109,0);//
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	for (i = 0; i < 5;i++) {
		if (i==0) Coconut[i] = M3D_LoadModelPLY("Files/coconut.ply",0,0);
		else Coconut[i] = M3D_ModelClone(Coconut[0]);
		M3D_ModelSetPosition(Coconut[i],0,3,3,0);
	}
	for (i = 0; i < 5;i++) {
		if (i==0) Boxes[i] = M3D_LoadModelPLY("Files/box.ply",0,COLOR_T8);
		else Boxes[i] = M3D_ModelClone(Boxes[0]);
		M3D_ModelSetPosition(Boxes[i],0,0,20,3);
		M3D_MaterialSetName(Boxes[i],0,"BOX_");
	}
	
	M3D_Model *Wood[8];
	for (i = 0; i < 8;i++){
		if (i==0) Wood[i] = M3D_LoadModelPLY("Files/wood.ply",0,COLOR_T4);
		else Wood[i] = M3D_ModelClone(Wood[0]);
		M3D_ModelConfPhysics(Wood[i],0, 0.1, M3D_BULLET_SHAPE_BOX);
		M3D_ModelSetPosition(Wood[i],0,(float)(-3.5+i),0,-6);
		M3D_MaterialSetName(Wood[i],0,"WOOD");
	}
	
	M3D_Model *Ball = M3D_LoadModelPLY("Files/ball.ply",0,COLOR_T4);
	//M3D_ModelSetOcclusion(Ball,0,1);
	M3D_Model *Ground = M3D_LoadModelPLY("Files/Track.ply",0,COLOR_T4);//
	M3D_Texture *BKG = M3D_LoadRawImage("Files/sky.png");

	M3D_BulletInitPhysics(256, 32);
	M3D_BulletSetGravity(0, -9.8f,0);

	M3D_ModelConfPhysics(Ground,0, 0, M3D_BULLET_SHAPE_CONVEXHULL);
	M3D_ModelInitPhysics(Ground);
	for (i = 0; i < 8;i++) M3D_ModelInitPhysics(Wood[i]);
	
	//BUILD AN OLD STYLE SUSPENSION BRIDGE
	M3D_ConstraintHinge(Wood[0],0, 0, -0.5,0,0, 2);
	for (i = 0; i < 7;i++) M3D_ConstraintHinge2(Wood[i],0,Wood[i+1],0, 1, 0.5,0,0, -0.5,0,0, 2);
	M3D_ConstraintHinge(Wood[7],0, 0, 0.5,0,0, 2);

	//SET BALL 
	M3D_ModelConfPhysics(Ball,0, 0.1, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Ball,0,-6, 0,-6);
	M3D_ModelInitPhysics(Ball);


	M3D_MaterialSetName(Ground,0,"GND_");
	//float ram2 = oslGetRamStatus().maxAvailable/1024/1024;

	while(1){
		M3D_updateScreen(0x00000000);
		
		M3D_BulletUpdatePhysics();

		ScePspFVector3 Ball_Pos = M3D_ModelGetPosition(Ball,0);
		M3D_CameraSetPosition(camera,Ball_Pos.x-1,Ball_Pos.y+4,Ball_Pos.z+10);
		M3D_CameraSetEye(camera,Ball_Pos.x,Ball_Pos.y,Ball_Pos.z);
		M3D_CameraSet(camera);
		
		ScePspFVector3 Ray_Down = (ScePspFVector3) {0,-2,0};
		char *Ball_Ground_Material = M3D_BulletRayTracingTest(&Ball_Pos,&Ray_Down);

		M3D_2DMode(1);
			M3D_DrawImage(BKG,0,0);
		M3D_2DMode(0);
		
		M3D_LightEnable(0);
		
			M3D_StartReflection(Mirror,0);
				M3D_ModelRenderMirror(Ball,0,0,1);
			M3D_FinishReflection();
		
			M3D_ModelRender(Ground,0);
			for (i = 0; i < 3;i++) M3D_ModelRender(Coconut[i],0);
			for (i = 0; i < 3;i++) M3D_ModelRender(Boxes[i],0);
			for (i = 0; i < 8;i++) M3D_ModelRender(Wood[i],0);
			M3D_ModelCastShadow(Ball,0,100,1,0);
			M3D_ModelRender(Ball,0);
		M3D_LightDisable(0);
	
		Spawn_Things();
		Spawn_Things2();

		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"       PHYSICS (BULLET)      ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0,"DIR PAD = move ball; BREAK BRIDGE = L; RESET BRIDGE = R");
			M3D_Printf(Font0,0,64,0xffffffff,0,0,0,"Ball on top of: %s",Ball_Ground_Material);
		M3D_2DMode(0);	

		if (Ball_Pos.y < -20){
			M3D_ModelDeletePhysics(Ball);
			M3D_ModelConfPhysics(Ball,0, 0.1, M3D_BULLET_SHAPE_SPHERE);
			M3D_ModelSetPosition(Ball,0,-6,0,-6);
			M3D_ModelInitPhysics(Ball);
		}

		M3D_ReadButtons();
		if (M3D_KEYS->pressed.L) M3D_ConstraintsRemove(Wood[0],0,1);
		if (M3D_KEYS->pressed.R){
			M3D_ConstraintHinge2(Wood[0],0,Wood[1],0, 1, 0.5,0,0, -0.5,0,0, 2);
		}
		if (M3D_KEYS->held.left) M3D_ModelSetForce(Ball,0,-0.005,0,0);
		if (M3D_KEYS->held.right) M3D_ModelSetForce(Ball,0,0.005,0,0);
		if (M3D_KEYS->held.up) M3D_ModelSetForce(Ball,0,0,0,-0.005);
		if (M3D_KEYS->held.down) M3D_ModelSetForce(Ball,0,0,0,0.005);
		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	
	return 0;
}
