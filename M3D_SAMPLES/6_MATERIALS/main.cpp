#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


char info[][128] = {
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n        Only  vertices        \n    (stanford ply  format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n    Only vertices + colour    \n    (wavefront obj format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n      Vertices + normals      \n    (stanford ply  format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n  Vertices + colour + normals \n    (wavefront obj format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n     Vertices + 1 texture     \n         (any format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\nVertices + 1 texture + normals\n         (any format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\nVertices + 1 texture + 1 color\n    (wavefront obj format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\nVert + 1 tex + 1 col + normals\n    (wavefront obj format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n   Vertices + vertex colours  \n    (stanford ply  format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n   Vert + vert col + normals  \n    (stanford ply  format)",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\n  Vert + vert col + 1 texture \n (.ply and binary PSP format) ",
"RENDER TYPES (SINGLE MATERIAL)\n\n\n\n\n\n\n\n\n\n\n\n\nVert + vert col + 1 tex + norm\n    (stanford ply  format)",

"    MULTI MATERIAL OBJECTS    \n (only wavefront  OBJ format) \n\n\n\n\n\n\n\n\n\n\n\n         Flat colours",
"    MULTI MATERIAL OBJECTS    \n (only wavefront  OBJ format) \n\n\n\n\n\n\n\n\n\n\n\n    Flat colours + normals",
"    MULTI MATERIAL OBJECTS    \n (only wavefront  OBJ format) \n\n\n\n\n\n\n\n\n\n\n\n    Flat colours + textures   ",
"    MULTI MATERIAL OBJECTS    \n (only wavefront  OBJ format) \n\n\n\n\n\n\n\n\n\n\n\nFlat col + textures + normals ",

"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n      Transparent object      \n       (alpha  texture)",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n   Transparent fading object  \n(blue vertices =  transparent)",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n         Glossy opaque        \n    (environment mapping)",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n      Glossy transparent",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n         Multitexture:        \n    Base texture + glossy     ",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n   Water (Scrolling texture)  ",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n             Fur",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n             Grass",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n     Animated texture (3D)",
"       SAMPLE MATERIALS       \n\n\n\n\n\n\n\n\n\n\n\n\n      Simple fake mirror",
};

u8 custom_texture_animation[] = {12, 0,1,2,3,4,5,6,7,8,9,10,11};

int main(){
	M3D_Init(COLOR_4444,0);
	M3D_DITHER(1);
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	M3D_LightSet(0,M3D_LIGHT_DIRECTIONAL,RGBA(255,255,255,255),RGBA(50,50,50,255),RGBA(90,90,90,255));
	M3D_LightSetPosition(0, 1, 1, 1);
	
	// Crea una camara
    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0,0,6);
	M3D_CameraSetEye(camera,0,0,0);
	M3D_InitMatrixSystem(35.0f,0.5,100,1);
	
	M3D_SetMipMapping(1,0.6);
	
	M3D_Loading_Start("files/Loading.png",0,0,128,128,0.04);

	M3D_Texture *BKG = M3D_LoadRawImage("Files/sky.png");
	
	M3D_Texture *Shine = M3D_LoadTexture("files/20_shine.png",0,COLOR_4444);
	M3D_TextureSetMapping(Shine,M3D_ENVIRONMENT_MAP, 1, 2);
	
	M3D_Model *Reflected = M3D_LoadModelPLY("Files/cube.ply",0.01,COLOR_T4);
	M3D_Model *Mirror = M3D_LoadModelPLY("Files/mirror.ply",0,COLOR_T4);
	
	M3D_Model *Models[28];//Memory leak bug solved! This model array can now be as big as you want.
	
	Models[0] = M3D_LoadModelPLY("Files/0_star.ply",0,0);
	Models[1] = M3D_LoadModel("Files/1_star.obj",0,0);
	Models[2] = M3D_LoadModelPLY("Files/2_star.ply",0,0);
	Models[3] = M3D_LoadModel("Files/3_star.obj",0,0);
	Models[4] = M3D_LoadModelPLY("Files/4_star.ply",0,0);
	Models[5] = M3D_LoadModelPLY("Files/5_star.ply",0,0);
	Models[6] = M3D_LoadModel("Files/6_star.obj",0,0);
	
	Models[7] = M3D_LoadModel("Files/7_star.obj",0,0);
	Models[8] = M3D_LoadModelPLY("Files/8_star.ply",0,0);
	Models[9] = M3D_LoadModelPLY("Files/9_star.ply",0,0);
	Models[10] = M3D_LoadModelPLY("Files/10_star.ply",0,0);
	Models[11] = M3D_LoadModelPLY("Files/11_star.ply",0,0);
	Models[12] = M3D_LoadModel("Files/12_star.obj",0,0);
	
	Models[13] = M3D_LoadModel("Files/13_star.obj",0,0);
	Models[14] = M3D_LoadModel("Files/14_star.obj",0,0);
	Models[15] = M3D_LoadModel("Files/15_star.obj",0,0);
	Models[16] = M3D_LoadModelPLY("Files/16_transparent.ply",0,COLOR_4444);
	Models[17] = M3D_LoadModelPLY("Files/17_alphavertex.ply",0,0);
	
	Models[18] = M3D_LoadModelPLY("Files/18_glossy.ply",0,0);
	M3D_ModelSetTextureMapping(Models[18],0,0,0,M3D_ENVIRONMENT_MAP);
	
	Models[19] = M3D_LoadModelPLY("Files/19_glossy.ply",0,COLOR_4444);
	M3D_ModelSetTextureMapping(Models[19],0,0,0,M3D_ENVIRONMENT_MAP);
	
	Models[20] = M3D_LoadModelPLY("Files/20_multibase.ply",0,COLOR_4444);
	M3D_ModelSetMultiTexture(Models[20],0,0,Shine);
	
	Models[21] = M3D_LoadModelPLY("Files/21_water.ply",0,COLOR_4444);
	
	Models[22] = M3D_LoadModelPLY("Files/furball0.ply",0,COLOR_4444);
	M3D_ModelSetTextureFilter(Models[22],0,0,0,0);
	
	Models[23] = M3D_LoadModelPLY("Files/furball1.ply",0,COLOR_4444);
	M3D_ModelSetTextureFilter(Models[23],0,0,0,0);

	Models[24] = M3D_LoadModelPLY("Files/animated.ply",0,COLOR_4444);
	M3D_ModelTexture3D_Animate(Models[24],0,0,4,4,custom_texture_animation,0.2);
	M3D_ModelSetTextureFilter(Models[24],0,0,0,0);
	
	M3D_Loading_Stop();

	int model_number = 0;
	float bounce = 0;
	while(1){
		M3D_updateScreen(0xFFaaaaaa);
		
		M3D_2DMode(1);
			M3D_DrawImage(BKG,0,0);
		M3D_2DMode(0);
		
		M3D_CameraSet(camera);
		M3D_LightEnable(0);
			if (model_number == 25){
				M3D_CameraSetPosition(camera,0,2,6);
				M3D_ModelSetOrigin(Reflected,0,0,0,0);
				M3D_ModelRotate(Reflected,0,0.2,-0.3,0.6);
				M3D_ModelSetPosition(Mirror,0,0,-0.4,0);
				M3D_StartReflection(Mirror,0);
					M3D_ModelRenderMirror(Reflected,0,0,1);
				M3D_FinishReflection();
				M3D_ModelRender(Reflected,0);
			} else if (model_number == 26){
				M3D_CameraSetPosition(camera,0,0,6);
			} else {
				M3D_CameraSetPosition(camera,0,0,6);
				if (model_number != 21) M3D_ModelRotate(Models[model_number],0,0.2,0.6,0);
				else M3D_ModelScrollTexture(Models[model_number],0,0,0,-0.005);
				M3D_ModelRender(Models[model_number],0);
			}
		M3D_LightDisable(0);

		
		M3D_2DMode(1);
			M3D_Printf(Font1,0,8,0xffffffff,0,0,0,info[model_number]);
		M3D_2DMode(0);
		
		M3D_ReadButtons();
		if (M3D_KEYS->pressed.right) model_number++;
		if (M3D_KEYS->pressed.left) model_number--;
		if (model_number < 0)model_number = 25;
		if (model_number > 25) model_number = 0;
		
		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	
	return 0;
}
