#include <M3D.h>

PSP_MODULE_INFO("triParticleTest", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);



int main(int argc, char **argv){
	M3D_Init(COLOR_5650,0);//Mode 5650 uses less video ram, and looks OK (green color has 6 bits) 
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	M3D_DITHER(1);
	M3D_SetMipMapping(1,0.6);
	M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,6,6,6);
	M3D_CameraSetEye(camera,0,0,0);
	M3D_InitMatrixSystem(45.0f,0.1,100,1);

	M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(10,10,10,255),RGBA(180,180,180,255));
	M3D_LightSetPosition(0, 1, 0, 1);

	M3D_Texture *BKG = M3D_LoadRawImage("Files/sky.png");
	
	M3D_Model *Mesh = M3D_LoadModelPLY("Files/bkg.ply",0,COLOR_4444);
	M3D_ModelSetLighting(Mesh,0,0);
	
	M3D_ParticleSystemInit(5);
	M3D_LoadParticle(0,"Files/spark.png",M3D_PARTICLE_SPRINKLE,1,1);
	M3D_ParticleSetPosition(0, 0,1.2,0);
	M3D_ParticleSetVelocity(0,-0.2,1,0.2);
	M3D_ParticleSetAnimation(0, 1,4,8);
	M3D_ParticleSetGravity(0, 0,-0.8,0);
	M3D_ParticleStart(0,0.3);

	M3D_LoadParticle(1,"Files/star.png",0,0,0);
	M3D_ParticleSetPosition(1, 2,1,0);
	M3D_ParticleSetVelocity(1,1,0,0);
	M3D_ParticleStart(1,0.3);

	M3D_LoadParticle(2,"Files/smoke.png",M3D_PARTICLE_SMOKE,0,1);
	M3D_ParticleSetPosition(2,-2.3,0.5,0.77);
	M3D_ParticleSetVelocity(2, 0,0.1,0);
	M3D_ParticleStart(2,0.6);

	M3D_LoadParticle(3,"Files/fire.png",M3D_PARTICLE_EXPLOSION,0,1);
	M3D_ParticleSetPosition(3,-1,1.4,-2);
	
	M3D_LoadParticle(4,"Files/snow.png",M3D_PARTICLE_FLOATING,0,0);	
	M3D_ParticleSetFilter(4,0);
	M3D_ParticleSetPosition(4,0,8,0);
	M3D_ParticleSetVelocity(4,0,-0.8,0);
	M3D_ParticleStart(4,0.5);

	M3D_SOUND *Boom = M3D_LoadWAV("files/explosion.wav",M3D_SOUND_NOSTREAM);
	//M3D_SOUND *M3D_LoadWAV(const char *path,int type);
	M3D_SOUND_Loop(Boom, 0);	

	float cam_rot = 0;
	while (1){
		
		M3D_updateScreen(0x00000000);
		M3D_CameraSetPosition(camera,M3D_Sin(cam_rot)*6,3,M3D_Cos(cam_rot)*6);
		M3D_CameraSetEye(camera,0,0.5,0);
		M3D_CameraSet(camera);
		
		M3D_2DMode(1);
			M3D_DrawImage(BKG,0,0);
		M3D_2DMode(0);
		
		M3D_LightEnable(0);
			M3D_ModelRender(Mesh,0);
			M3D_ParticlesRender(1);
		M3D_LightDisable(0);
		
		//DRAW 2D STUFF
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"     PARTICLES  (OPENTRI)     ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," X/O = Enable/disable wand sparkles\n Square = Explode bomb");
			M3D_DrawLine(32,24,0xff00FF00,480-32,24,0xffff0000);
		M3D_2DMode(0);
		
		
		cam_rot += 0.01;
		
		M3D_ReadButtons();
		if (M3D_KEYS->pressed.circle) M3D_ParticleStop(0);
		if (M3D_KEYS->pressed.cross) M3D_ParticleStart(0,0.3);
		if (M3D_KEYS->released.square) {M3D_SOUND_Play(Boom,0);M3D_ParticleStart(3,0.4);}
		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}

	return 0;
}
