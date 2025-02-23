#include <M3D.h>

PSP_MODULE_INFO("forest", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


//source files in "OEM US FORMAT". 
char text_box[] = {
	"…ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂª\n"
	"›@HEALTH@∞∞∞∞∞∞∞∞@@@KEYS@@@ﬁ\n"
	"»‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹º\n"
};

int Sample_AnimIdle[] = {2,2,0,1,0};
int Sample_AnimWalkFast[] = {4,12,5,1,0};
int Sample_AnimWalkSlow[] = {4,12,10,1,0};
int Sample_AnimJump[] = {3,3,0,1,0};
int Sample_AnimDie[] = {13,17,16,0,0};


/* Simple thread */
int main(int argc, char **argv){
	int n_keys = 0;
	M3D_Init(COLOR_5551,1);
	M3D_FrameSkip(0);
	M3D_DITHER(1);
	M3D_SetMipMapping(1,0.3);
	
	M3D_Texture *Font0 = M3D_LoadTexture("Files/font8.png",0,COLOR_4444);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(20,20,20,255),RGBA(80,80,80,255));
	M3D_LightSetPosition(0, 0, 1, 0);
	
	M3D_Model *Sky = M3D_LoadModelPLY("Files/sky.ply",0,COLOR_T4);
	M3D_ModelBIN *Mesh = M3D_LoadModelBIN("Files/map.m3b",COLOR_T8);
	M3D_Model *Keys[4];
	Keys[0] = M3D_LoadModelPLY("Files/key.ply",0,0);//This has a black border by itself added in Blender
	Keys[1] = M3D_ModelClone(Keys[0]);
	Keys[2] = M3D_ModelClone(Keys[0]);
	Keys[3] = M3D_ModelClone(Keys[0]);
	Keys[4] = M3D_ModelClone(Keys[0]);
	
	M3D_SkinnedActor *Fox = M3D_LoadSkinnedActor("Files/fox.m3a",0.03,COLOR_T8);
	M3D_SkinnedActorConfig(Fox,2,6,20,1,0);
	
	M3D_SkinnedActor *Badguy0 = M3D_LoadSkinnedActor("Files/badguy.m3a",0.03,COLOR_T8);
	M3D_SkinnedActorConfig(Badguy0,1,9,4,1,0);
	M3D_SkinnedActor *Badguy1 = M3D_SkinnedActorClone(Badguy0);
	M3D_SkinnedActorConfig(Badguy1,1,9,4,1,0);
	M3D_SkinnedActor *Badguy2 = M3D_SkinnedActorClone(Badguy0);
	M3D_SkinnedActorConfig(Badguy2,1,9,4,1,0);
	M3D_SkinnedActor *Badguy3 = M3D_SkinnedActorClone(Badguy0);
	M3D_SkinnedActorConfig(Badguy3,1,9,4,1,0);
	
	M3D_BulletInitPhysics(1000, 16);
	M3D_BulletSetGravity(0, -16.0f,0);
	
	M3D_ModelBINInitPhysics(Mesh);
	M3D_SkinnedActorSetPosition(Fox,0,4,0);
	M3D_SkinnedActorInitPhysics(Fox,1);
	M3D_SkinnedActorSetPosition(Badguy0,-4,0.5,4);
	M3D_SkinnedActorInitPhysics(Badguy0,1);
	M3D_SkinnedActorSetPosition(Badguy1,1,0,17);
	M3D_SkinnedActorInitPhysics(Badguy1,1);
	M3D_SkinnedActorSetPosition(Badguy2,25,0.5,6);
	M3D_SkinnedActorInitPhysics(Badguy2,1);
	M3D_SkinnedActorSetPosition(Badguy3,20,0,37);
	M3D_SkinnedActorInitPhysics(Badguy3,1);
	
	M3D_ModelConfPhysics(Keys[0],0, 0, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Keys[0],0,3,3,3);
	M3D_ModelInitPhysics(Keys[0]);
	M3D_ModelConfPhysics(Keys[1],0, 0, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Keys[1],0,4,2,33);
	M3D_ModelInitPhysics(Keys[1]);
	M3D_ModelConfPhysics(Keys[2],0, 0, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Keys[2],0,24,3,6);
	M3D_ModelInitPhysics(Keys[2]);
	M3D_ModelConfPhysics(Keys[3],0, 0, M3D_BULLET_SHAPE_SPHERE);
	M3D_ModelSetPosition(Keys[3],0,26,10,24);
	M3D_ModelInitPhysics(Keys[3]);

	M3D_MaterialSetName(Badguy0,0,"BADG");
	M3D_MaterialSetName(Badguy1,0,"BADG");
	M3D_MaterialSetName(Badguy2,0,"BADG");
	M3D_MaterialSetName(Badguy3,0,"BADG");
	M3D_MaterialSetName(Mesh,0,"GND0");
	M3D_MaterialSetName(Fox,0,"FOX_");
	M3D_MaterialSetName(Keys[0],0,"KEYS");
	M3D_MaterialSetName(Keys[1],0,"KEYS");
	M3D_MaterialSetName(Keys[2],0,"KEYS");
	M3D_MaterialSetName(Keys[3],0,"KEYS");
	
	M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0,0,-4);
	M3D_InitMatrixSystem(45.0f,0.5,100,1);

	M3D_ShadowprojectionSet(64,8);

	M3D_Player player;
	player.Health = 8;
	player.AnimIdle = Sample_AnimIdle;
	player.AnimWalkSlow = Sample_AnimWalkSlow;
	player.AnimWalkFast = Sample_AnimWalkFast;
	player.AnimJump = Sample_AnimJump;
	player.AnimDie = Sample_AnimDie;
	
	//Fade in from black
	M3D_SetFade(1,0,8);
	
	while (1){
		
		M3D_updateScreen(0xff440000);
		M3D_BulletUpdatePhysics();
		
		M3D_ReadButtons();
		
		M3D_LightSetPosition(0, 1, 1, 1);
		
		M3D_CameraSet(camera);
		M3D_CameraFollowSkinnedActor(camera,Fox,3,4);
		
		M3D_LightEnable(0);
		M3D_DrawSkyBox(Sky,45);
		
		M3D_ModelBINRender(Mesh,0);
		ScePspFVector3 FOX_Position = M3D_SkinnedActorGetPos(Fox);
		
		M3D_ModelRender(Keys[0],0);
		M3D_ModelRender(Keys[1],0);
		M3D_ModelRender(Keys[2],0);
		M3D_ModelRender(Keys[3],0);
		M3D_SkinnedActorCastShadow(Fox, 140, 1, 0);
		M3D_SkinnedActorRender(Fox);
		M3D_SkinnedActorCastShadow(Badguy0, 140, 1, 0);
		M3D_SkinnedActorRender(Badguy0);
		M3D_SkinnedActorCastShadow(Badguy1, 140, 1, 0);
		M3D_SkinnedActorRender(Badguy1);
		M3D_SkinnedActorCastShadow(Badguy2, 140, 1, 0);
		M3D_SkinnedActorRender(Badguy2);
		M3D_SkinnedActorCastShadow(Badguy3, 140, 1, 0);
		M3D_SkinnedActorRender(Badguy3);
		M3D_LightDisable(0);

		M3D_ModelRotate(Keys[0],0,0,2,0);
		M3D_ModelRotate(Keys[1],0,0,2,0);
		M3D_ModelRotate(Keys[2],0,0,2,0);
		M3D_ModelRotate(Keys[3],0,0,2,0);
		
		M3D_EnemyMove(Badguy0);
		M3D_EnemyMove(Badguy1);
		M3D_EnemyMove(Badguy2);
		M3D_EnemyMove(Badguy3);
		
		M3D_CharacterMove(Fox,&player,"BADG",0,"KEYS",1,8);
		
		for (n_keys = 0; n_keys < 4; n_keys++){
			if (M3D_ModelCheckCollision(Keys[n_keys],0,"FOX_")){
				M3D_ModelDeletePhysics(Keys[n_keys]);
				M3D_ModelSetPosition(Keys[n_keys],0,0,-10,0);
			}
		}
		
		M3D_2DMode(1);
			if (player.Health != 0) M3D_Printf(Font1,0,16,0xffffffff,0,0,0,"    SIMPLE 3D PLATFORM DEMO");
			M3D_Printf(Font0,0,240,0xffffffff,0,0,0,text_box);
			M3D_Printf(Font0,196,248,0xffffffff,0,0,0,"%i",player.Items[0]);
			M3D_DrawHealthBar(8*9,248,player.Health);
			if (player.Health == 0) M3D_Printf(Font1,180,60,0xffffffff,0,0,0,"GAME OVER");
			//SHOW EXIT MESSAGE
			if(FOX_Position.z>42 && FOX_Position.x>19 && FOX_Position.x<23) {
				if (player.Items[0] != 4) M3D_Printf(Font1,50,50,0xffffffff,0,0,0,"NEED 4 KEYS");
				if (player.Items[0] == 4) break;
			}
		M3D_2DMode(0);
		
		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	//Fade in from black
	M3D_SetFade(2,0,8);
	while (1){
		M3D_updateScreen(0xff000000);
		M3D_ReadButtons();
		
		M3D_2DMode(1);
			M3D_Printf(Font1,180,60,0xffffffff,0,0,0,"LEVEL CLEARED");
		M3D_2DMode(0);
	}

	return 0;
}
