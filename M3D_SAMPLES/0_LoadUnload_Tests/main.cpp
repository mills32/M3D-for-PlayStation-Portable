/*
RAM TESTS
*/
#include <M3D.h>

PSP_MODULE_INFO("RAM TESTS", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


int main(){
	M3D_Init(COLOR_5650,0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	int FREE_RAM0 = 0;
	int FREE_RAM = 0;
	int USED_VRAM = 0;
	int USED_VRAM0 = 0;
	int n = 32; //number of tests

	//TEXTURE
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	FREE_RAM0 = FREE_RAM;
	USED_VRAM0 = USED_VRAM;
	for(int i = 0;i<n;i++){
		M3D_Texture *R =  M3D_LoadTexture("Files/test.png",0,COLOR_8888);
		M3D_TextureUnload(R);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD TEXTURE\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",M3D_GetFreeRAM()-FREE_RAM,M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	
	//WAV
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_SOUND *S = M3D_LoadWAV("files/sound.wav",M3D_SOUND_NOSTREAM);
		M3D_SOUND_Delete(S);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD WAV\n\n\nFREE RAM=%i KB\n\nTEST=%i",M3D_GetFreeRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\n\n\nWAIT 6 SECONDS FOR NEXT TEST",M3D_GetFreeRAM()-FREE_RAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	
	//MP3
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_SOUND *S = M3D_LoadWAV("files/sound.mp3",M3D_SOUND_STREAM);
		M3D_SOUND_Delete(S);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MP3\n\n\nFREE RAM=%i KB\n\nTEST=%i",M3D_GetFreeRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\n\n\nWAIT 6 SECONDS FOR NEXT TEST",M3D_GetFreeRAM()-FREE_RAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//MAP
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_MAP *M = M3D_LoadMapTMX("Files/map.tmx",1,M3D_IN_VRAM,COLOR_T8 );
		M3D_MapUnload(M);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MAP / TILES\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//OBJ
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_Model *R = M3D_LoadModel("Files/test.obj",0,COLOR_8888);
		M3D_ModelUnload(R);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL OBJ\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	
	//PLY
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_Model *R = M3D_LoadModelPLY("Files/test.ply",0,COLOR_T4);
		M3D_ModelUnload(R);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL PLY\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//M3B
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_ModelBIN *M = M3D_LoadModelBIN("Files/map.m3b",COLOR_T8);
		M3D_ModelBINUnload(M);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL BIN\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//M3A
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_SkinnedActor *M = M3D_LoadSkinnedActor("Files/test.m3a",0.1,COLOR_5650);
		M3D_SkinnedActorUnload(M);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL ANIMATED\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//M3M
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_MorphingActor *M = M3D_LoadMorphingActor("Files/test.m3m",0.1,COLOR_5650);
		M3D_MorphingActorUnload(M);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL MORPHING\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//PARTICLES
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_ParticleSystemInit(4);
		M3D_LoadParticle(0,"Files/test.png",0,0,0);M3D_ParticleStart(0,16);
		M3D_LoadParticle(1,"Files/test.png",0,0,0);M3D_ParticleStart(1,16);
		M3D_LoadParticle(2,"Files/test.png",0,0,0);M3D_ParticleStart(2,16);
		M3D_LoadParticle(3,"Files/test.png",0,0,0);M3D_ParticleStart(3,16);
		M3D_ParticleSystemUnload();
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD PARTICLES\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}

	//NURBS SURFACE
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_NurbsSurface *S = M3D_CreateNurbsSurface("Files/test.png",COLOR_T4,16,16);
		M3D_NurbsDelete(S);
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD NURBS\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	
	
	//PHYSICS
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_BulletInitPhysics(1000,32);
		M3D_BulletFinishPhysics();
		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD PHYSICS WORLD\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	

	//MODEL PHYSICS
	M3D_BulletInitPhysics(1000, 16);
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	for(int i = 0;i<n;i++){
		M3D_Model *M = M3D_LoadModelPLY("Files/test.ply",0,COLOR_8888);
		M3D_ModelConfPhysics(M,0,1,M3D_BULLET_SHAPE_CONVEXHULL);
		M3D_ModelInitPhysics(M);
		M3D_ConstraintHinge(M,0, 0 ,1,0,0, 0);
		M3D_ModelDeletePhysics(M);//M3D_ConstraintsRemove(M,0,0);
		M3D_ModelUnload(M);
		
		M3D_ModelBIN *A = M3D_LoadModelBIN("Files/map.m3b",COLOR_8888);
		M3D_ModelBINInitPhysics(A);
		M3D_ModelBINDeletePhysics(A);
		M3D_ModelBINUnload(A);

		M3D_SkinnedActor *B = M3D_LoadSkinnedActor("Files/test.m3a",0,COLOR_8888);
		M3D_SkinnedActorInitPhysics(B,1);
		M3D_SkinnedActorDeletePhysics(B);
		M3D_SkinnedActorUnload(B);
		
		M3D_MorphingActor *D = M3D_LoadMorphingActor("Files/test.m3m",0.1,COLOR_5650);
		M3D_MorphingActorInitPhysics(D,1);
		M3D_MorphingActorDeletePhysics(D);
		M3D_MorphingActorUnload(D);

		M3D_Model *C = M3D_LoadModelPLY("Files/test.ply",0,COLOR_8888);
		M3D_VehicleInitPhysics(C,1,1,1,1,1,1);
		M3D_ModelDeletePhysics(C);
		M3D_ModelUnload(C);

		M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD MODEL PHYSICS\n\n\nFREE RAM=%i KB\nUSED VRAM=%i KB\nTEST=%i",M3D_GetFreeRAM(),M3D_GetUsedVRAM(),i);
		if (i == n-1) M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 6 SECONDS FOR NEXT TEST",FREE_RAM-M3D_GetFreeRAM(),M3D_GetUsedVRAM()-USED_VRAM);
		M3D_updateScreen(0x00000000);
		if (i == n-1) M3D_Wait(6);
	}
	M3D_BulletFinishPhysics();

	//END
	FREE_RAM = M3D_GetFreeRAM();
	USED_VRAM = M3D_GetUsedVRAM();
	
	M3D_Printf(Font1,0,0,0xffff00ff,0,0,0,"LOAD/UNLOAD TEST FINISHED");
	M3D_Printf(Font1,0,120,0xffecff00,0,0,0,"WASTED RAM=%i KB (SHOULD BE 0)\nWASTED VRAM=%i KB (SHOULD BE 0)\n\nWAIT 12 SECONDS TO EXIT",FREE_RAM0-FREE_RAM,USED_VRAM-USED_VRAM0);
	M3D_updateScreen(0x00000000);
	M3D_Wait(12);

	M3D_Quit();
}