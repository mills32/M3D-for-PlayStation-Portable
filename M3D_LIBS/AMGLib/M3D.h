#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <psptypes.h>
#include <pspkernel.h>

#define COLOR_5650		0 // 16 BIT: 5 RED, 6 GREEN, 5 BLUE, 0 ALPHA
#define COLOR_5551		1 // 16 BIT: 5 RED, 5 GREEN, 5 BLUE, 1 ALPHA
#define COLOR_4444		2 // 16 BIT: 4 RED, 4 GREEN, 4 BLUE, 4 ALPHA
#define COLOR_8888		3 // 32 BIT: 8 RED, 8 GREEN, 8 BLUE, 8 ALPHA
#define COLOR_T4		4 //  4 BIT: INDEXED ( 16 COLOR PALETTE)
#define COLOR_T8		5 //  8 BIT: INDEXED (256 COLOR PALETTE)
#define M3D_SOUND_NOSTREAM	0		//FROM OSL
#define M3D_SOUND_STREAM	0x400	//FROM OSL
#define M3D_IN_VRAM		1
#define M3D_IN_RAM		2
#define ABGR(a,b,g,r)	(((a) << 24)|((b) << 16)|((g) << 8)|(r))
#define ARGB(a,r,g,b)	ABGR((a),(b),(g),(r))
#define RGBA(r,g,b,a)	ARGB((a),(r),(g),(b))
#define M3D_EFFECT_H_WAVE 0
#define M3D_EFFECT_V_WAVE 1
#define M3D_LIGHT_DIRECTIONAL          0
#define M3D_LIGHT_POINTLIGHT           1
#define M3D_LIGHT_SPOTLIGHT            2
#define M3D_TEXTURE_COORDS				0
#define M3D_ENVIRONMENT_MAP				2
#define M3D_PARTICLE_DEFAULT			0
#define M3D_PARTICLE_FLOATING		1
#define M3D_PARTICLE_EXPLOSION		2
#define M3D_PARTICLE_SPRINKLE		3
#define M3D_PARTICLE_SMOKE			4
#define M3D_BULLET_SHAPE_NONE		0
#define M3D_BULLET_SHAPE_BOX		1
#define M3D_BULLET_SHAPE_SPHERE		2
#define M3D_BULLET_SHAPE_CONE		3
#define M3D_BULLET_SHAPE_CYLINDER	4
#define M3D_BULLET_SHAPE_CONVEXHULL	5

//SET 3D camera, clipping will delete triangles if their position is bigger than "far"
void M3D_InitMatrixSystem(float fov,float near,float far,int clipping);

extern "C" {

float M3D_Sin(float angle);
float M3D_Cos(float angle);
float M3D_Deg2Rad(float n);
float M3D_Rad2Deg(float n);
float M3D_SquareRoot(float val);
float M3D_Randf(float min, float max);
float M3D_fabsf(float x);



/******************************************/
/************** STRUCTS *******************/
/******************************************/

//OSL KEYS
typedef union		{
	struct		{
		int select:1;					//!< Select
		int reserved1:2;				//!< For padding, do not use
		int start:1;					//!< Start
		int up:1;						//!< Up (d-pad)
		int right:1;					//!< Right (d-pad)
		int down:1;						//!< Down (d-pad)
		int left:1;						//!< Left (d-pad)
		int L:1;						//!< L (shoulder)
		int R:1;						//!< R (shoulder)
		int reserved2:2;				//!< For padding, do not use
		int triangle:1;					//!< Triangle
		int circle:1;					//!< Circle
		int cross:1;					//!< Cross
		int square:1;					//!< Square
		int home:1;						//!< Home (seems not to work)
		int hold:1;						//!< Hold (power switch in the opposite direction)
		int reserved3:5;				//!< For padding, do not use
		int note:1;						//!< Note (seems not to work)
	};
	unsigned int value;					//!< 32-bit value containing all keys
} M3D_KEYLIST;

//OSL CONTROL TYPE
typedef struct		{
	M3D_KEYLIST held;						//!< Keys currently down (held)
	M3D_KEYLIST pressed;					//!< Keys pressed (only reported once when the user pressed it)
	M3D_KEYLIST released;					//!< Keys released (only reported once when the user releases it)
	M3D_KEYLIST lastHeld;					//!< Allows you to trick with the held member without messing up the auto-repeat feature
	short autoRepeatInit;					//!< Time for the initialization of the autorepeat feature
	short autoRepeatInterval;				//!< Interval before the autorepeat feature is switched on (the time the user must let the key down)
	int autoRepeatMask;						//!< Keys affected by the autorepeat feature
	short autoRepeatCounter;				//!< Counter (internal)
	signed char analogToDPadSensivity;		//!< Minimum sensivity for the analog to d-pad function in each direct, 0 means disable analog to d-pad. 127 is the maximum: the stick has to be completely pressed to be detected. A typical value is 80.
	signed char analogX;					//!< Horizontal position of the analog stick (-128: left, +127: right)
	signed char analogY;					//!< Vertical position of the analog stick (-128: top, +127: bottom)
    int holdAffectsAnalog;
} M3D_CONTROLLER;

extern M3D_CONTROLLER *M3D_KEYS;

typedef struct {
	u8 Health = 8;
	u8 Items[64] = {0};
	u32 CollidedWith;
	int *AnimIdle;
	int *AnimWalkFast;
	int *AnimWalkSlow;
	int *AnimJump;
	int *AnimDie;
	u8 Ground;
}M3D_Player;


//M3D OBJECT TYPES
//----------------

typedef struct{ void *Texture2D; } M3D_Texture;				//Image for sprites or textures
typedef struct{ void *Map2D; } M3D_MAP;						//2D OSL_MAP
typedef struct{ void *Sound; } M3D_SOUND;					//OSL_SOUND MP3/WAV/Traker file
typedef struct{ void *Camera; } M3D_Camera;					//A PSP 3D Camera
typedef struct{ void *Model; } M3D_Model;					//OBJ/PLY loaded 3D model
typedef struct{ void *Model; } M3D_ModelBIN;				//Binary PSP format model
typedef struct{ void *SkinnedModel; } M3D_SkinnedActor;		//3D model with bone animations
typedef struct{ void *MorphingModel; } M3D_MorphingActor;	//3D model with shape animations	
typedef struct {void *NurbsSurface;} M3D_NurbsSurface;		//3D surface


/******************************************/
/************** FUNCTIONS *****************/
/******************************************/

//SOUND (OSL)
M3D_SOUND *M3D_LoadMP3(const char *path,int type);
M3D_SOUND *M3D_LoadMOD(const char *path,int type);
M3D_SOUND *M3D_LoadWAV(const char *path,int type);
void M3D_SOUND_Loop(M3D_SOUND *sound, int loop);
void M3D_SOUND_Play(M3D_SOUND *sound, int voice);
void M3D_SOUND_Stop(M3D_SOUND *sound);
void M3D_SOUND_Delete(M3D_SOUND *sound);
int M3D_SOUND_Playing(M3D_SOUND *sound);
void M3D_MikModReverb(u8 rev); //DISABLE reverb to make mikmod faster

//M3D(AMG - OSLib)
void M3D_Init(u32 psm, u32 TV);
void M3D_SetFade(u32 mode, u32 color, u32 speed);//Fade screen. Mode 1 in / mode 2 out
//Frameskip: (0 = keep 60 if possible, else drop frames. /  1 = keep 30 (does not work well)
void M3D_FrameSkip(int mode); 
int M3D_TV_State();//TV out state 0 = OFF; 1 = ON
extern int M3D_ScreenX;	//Screen size
extern int M3D_ScreenY;	//Screen size
void M3D_ReadButtons();
void M3D_Quit();
u16 M3D_GetFreeRAM();
u16 M3D_GetTotalVRAM();
u16 M3D_GetUsedVRAM();
void M3D_updateScreen(u32 color);//Use this at the start (or end) of any loop, or after any update you do on screen.
int M3D_GetCpuSpeed();
char *M3D_GetScreenMode();
void M3D_DITHER(int mode);//PSX style dither, it looks great with 5551 and 5650 modes
void M3D_2DMode(int mode);//Use this if you have to draw 2D stuff, or print things.
void M3D_FogEnable(float near, float far, u32 color);
void M3D_FogDisable();

/*PRINT TEXT
------------
	tex: texture (font) to use
 	wave_amp: wave amplitude, if 0, regular text will be printed.
  	wave_speed: wave size or wave length.
   	wave_val: wave position. Change this to animate the wave.
*/
void M3D_Print(M3D_Texture *tex, int x, int y, u32 color, int wave_amp, int wave_speed, float wave_val, char *text);
#define M3D_Printf(tex, x, y, color, wave_amp, wave_speed, wave_val, ...){\
	char __str[1000];\
	sprintf(__str , __VA_ARGS__);\
	M3D_Print(tex, x, y, color, wave_amp, wave_speed, wave_val,  __str);\
}


/*TEXTURES
----------
	psm: color mode for textures (COLOR_4444/5551/5650/8888)
	load: were to store the texture (M3D_IN_RAM/VRAM). It will always try to load to VRAM
	mapping: normal (M3D_TEXTURE_COORDS) reflection (M3D_ENVIRONMENT_MAP)	
	filter: 0 pixelated, 1 smooth
*/
M3D_Texture *M3D_GetFont(int n);
void M3D_SetMipMapping(int set, float bias);
M3D_Texture *M3D_LoadTexture(const char *path, u8 load, u32 psm);
M3D_Texture *M3D_LoadHugeImage(const char *filename);//Load 640x480 and 720x480 images
M3D_Texture *M3D_LoadRawImage(const char *path);//Load image to be blitted as background very fast
M3D_Texture *M3D_TextureCreate(u16 width, u16 height, u32 psm, u8 load);
M3D_Texture *M3D_RenderTextureCreate(u16 width, u16 height);
void M3D_TextureSetFilter(M3D_Texture *t, int filter);
void M3D_TextureSetMapping(M3D_Texture *t, u32 mapping, u8 l0, u8 l1);
void M3D_DrawHugeImage(M3D_Texture *image, s16 x, s16 y);//Draw 640x480 or 720x480 image
void M3D_DrawSprite(M3D_Texture *tex, int x = 0, int y = 0, int tile_size_x = 0, int tile_size_y = 0, u8 *anim = NULL, float speed = 0);
void M3D_DrawSpriteScaleRot(M3D_Texture *tex, int x = 0, int y = 0, float rot = 0, float scale = 1, int tile_size_x = 0, int tile_size_y = 0, u8 *anim = NULL, float speed = 0);
void M3D_DrawImage(M3D_Texture *t, int x, int y);//Draw image very fast
void M3D_TextureUnload(M3D_Texture *tex);
void M3D_Texture3D_Animate(M3D_Texture *tex, u8 xframes, u8 yframes, u8 *anim, float speed);
void M3D_RenderToTextureEnable(M3D_Texture *t);//Render to a custom texture, you can then use it on models.
void M3D_RenderToTextureDisable(void);

/*MAPS
------
	mode: 0 = unswizzled tiles, slower but allows tile animations; 1 = swizzled tiles, faster but does not allow tile aniations.
	psm: color mode for tiles (COLOR_4444/5551/5650/8888)
	load: were to store the tiles (M3D_IN_RAM/VRAM). It will always try to load to VRAM
	slot: tile animation slot (there are 16 slots)
	tile: destination tile number in tilemap
	start_tile: source tile number in tilemap
	anim_size: number of tiles to use in aminmation
*/
M3D_MAP *M3D_LoadMapTMX(const char *path, u8 mode, u8 load, u32 psm);
void M3D_DrawMap(M3D_MAP *map);
void M3D_MapSetScroll(M3D_MAP *map, int x, int y);
void M3D_MapAnimateTiles(M3D_MAP *map, int slot, int tile, int start_tile, u8 anim_size, float speed);
void M3D_MapUnload(M3D_MAP *map);


/*MISC
------
	tyoe: 0 = cycle M3D_Texture palette; 1 = cycle M3D_MAP palette
	image: The M3D_Texture or M3D_MAP
	setpalettes: an array which defines palete animation.
		
		u8 palette_animation[] = {
			X,//Number of palette cycles
			0,0,18,8,//First cycle: rate (blend speed), invert, first color in image palette, number of colors to cycle,
			0,0,48,8 //Next  cycle: rate (blend speed), invert ...
			...
			...
		};

	blend: 0 = disabled; 1 = enabled, use blend function to smooth color changes
*/
void M3D_PalettesCycle(int type, void *image, u8 *setpalettes, u8 blend);
//From OSL, Draws a rectangle using vertex colors, use it to draw color gradients.
void M3D_DrawGradientRect(int x, int y, int w, int h, u32 col1, u32 col2, u32 col3, u32 col4);
void M3D_DrawLine(int x0, int y0, u32 color0, int x1, int y1, u32 color1);
//Draw a health bar using an M3D_Texture (font)
void M3D_DrawHealthBar(int x, int y, int health, u32 color = 0xFF00FF00, int sizex = 8, int sizey = 8, M3D_Texture *tex = NULL);


/*RETRO EFFECTS
---------------
	mode: 0 or 1. When drawing a wavy image, it sets vertical or horizontal waves.
	amp: image wave amplitude.
	len: image wave length
	tx,ty: rotozoom rotation center.
	px,py,scale: plasma parameters
	
	M3D_PlasmaTextureCreate: creates a texture which can be used on sprites or 3D models.
	to update this, you run M3D_PlasmaTextureUpdate
	
	M3D_Plasma2DSet: set a full screen plasma, to update this, you just run M3D_Draw2DPlasma.

*/
void M3D_DrawImageWave(M3D_Texture *tex, int x, int y, float wave_val, u32 mode, s16 amp, s16 len);
void M3D_DrawRotoZoom(M3D_Texture *tex, u32 alpha, int tx, int ty, float rot, float scale);
M3D_Texture *M3D_PlasmaTextureCreate(const char *path);
void M3D_PlasmaTextureUpdate(M3D_Texture *tex,int px,int py,float scale,int speed);
void M3D_Plasma2DSet(const char *path);
void M3D_Draw2DPlasma(u8 alpha, int px,int py,float scale,float speed);
void M3D_DrawCopperBars(u8 number,u8 size,u16 center,u16 amplitude,u8 alpha,float speed);


/*VIDEO
-------
	MJPEG videos can be used on 3D models or sprites.
	H264 videos are always displayed on full screen.

*/
int M3D_VIDEO_LoadMJPEG(const char *path);
void M3D_VIDEO_PlayFullScreenMJPEG(const char *path,int loop);
void M3D_VIDEO_MJPEGToTexture_Start(int video_handle, int loop, M3D_Texture *tex);
void M3D_VIDEO_MJPEGToTexture_Stop(M3D_Texture *tex);
int M3D_VIDEO_PlayH264(const char *path);


/*3D CAMERA
----------- 
*/
M3D_Camera *M3D_CameraInit();
void M3D_CameraSet(M3D_Camera *camera);
void M3D_CameraSetPosition(M3D_Camera *camera,float x, float y, float z);
void M3D_CameraSetEye(M3D_Camera *camera,float x, float y, float z);//Where the camera looks at
void M3D_CameraMove(M3D_Camera *camera, float dx, float dy, float dz);
void M3D_CameraSetUp(M3D_Camera *camera, float ux, float uy, float uz);//Camera rotation

/*3D LIGHT
----------
	n: light number, I think PSP has 4 lights
	type: M3D_LIGHT_DIRECTIONAL / M3D_LIGHT_POINTLIGHT / M3D_LIGHT_SPOTLIGHT.
		spotlight is very difficult to set up, I did not provide setup functions for it.
	diffise,specular,ambient: light colors
*/
void M3D_LightSet(int n, u32 type, u32 diffuse, u32 specular, u32 ambient);
void M3D_LightSetPosition(int n, float px, float py, float pz);
ScePspFVector3 M3D_LightGetPosition(int n);
void M3D_LightEnable(u8 n);
void M3D_LightDisable(u8 n);


/*3D MODEL
----------
	css: outline size, for cartoon style models with a black outline.
	psm: Color mode for loaded textures
	transparent: enable soecial render mode to reduce artifacts in transparent models
	offset: not yet implemented 
*/
M3D_Model *M3D_LoadModel(const char *path, float css, u32 psm);
M3D_Model *M3D_LoadModelPLY(const char *path, float css, u32 psm);
//M3D_Model *M3D_ModelArray(int number);
M3D_ModelBIN *M3D_LoadModelBIN(const char *path, u32 psm);
void M3D_ModelRender(M3D_Model *model, int transparent);
void M3D_ModelBINRender(M3D_ModelBIN *mesh, u32 offset);
void M3D_ModelSetTexture(M3D_Model *m, int obj_number, int group_number, M3D_Texture *t);
void M3D_ModelSetPosition(M3D_Model* m,int obj,float x, float y, float z);
ScePspFVector3 M3D_ModelGetPosition(M3D_Model* m,int obj);
void M3D_ModelCopyPosition(M3D_Model* m0, int obj0, M3D_Model* m1,int obj1);
void M3D_ModelSetRotation(M3D_Model* m,int obj,float x, float y, float z);
void M3D_ModelTranslate(M3D_Model* m,int obj,float x, float y, float z);
void M3D_ModelRotate(M3D_Model* m,int obj,float x, float y, float z);
void M3D_ModelSetScale(M3D_Model* m,int obj,float x, float y, float z);
void M3D_ModelResetPosition(M3D_Model* m,int obj);
void M3D_ModelResetRotation(M3D_Model* m,int obj);
void M3D_ModelSetOrigin(M3D_Model* m,int obj_number,float x, float y, float z);
void M3D_ModelScrollTexture(M3D_Model* m,int obj_number,int group_number,float x, float y);
M3D_Model *M3D_ModelClone(M3D_Model *m);

void M3D_ModelSetTextureFilter(M3D_Model *model, int obj_number, int group_number, int ntex, int filter);
void M3D_ModelSetTextureMapping(M3D_Model *model, int obj_number, int group_number, int ntex, u32 mapping);
void M3D_ModelSetMultiTexture(M3D_Model *model, int obj_number, int group_number, M3D_Texture *tex);
void M3D_ModelTexture3D_Animate(M3D_Model *model, int obj_number, int group_number, u8 xframes, u8 yframes, u8 *anim, float speed);
void M3D_ModelSetLighting(M3D_Model* m,int obj_number, u8 light);
void M3D_ModelSetOcclusion(M3D_Model* m,int obj,int value);

void M3D_StartReflection(M3D_Model *model, u8 number);
void M3D_ModelRenderMirror(M3D_Model *model, u8 number, u8 light, u8 axis);
void M3D_FinishReflection();

void M3D_DrawSkyBox(M3D_Model *model,float fov);

void M3D_ModelBINSetPosition(M3D_ModelBIN* m,float x, float y, float z);
void M3D_ModelBINSetRotation(M3D_ModelBIN* m,float rx, float ry, float rz);
void M3D_ModelBINTranslate(M3D_ModelBIN* m,float dx, float dy, float dz);
void M3D_ModelBINRotate(M3D_ModelBIN* m,float drx, float dry, float drz);
void M3D_ModelBINSetOrigin(M3D_ModelBIN* m,float ox, float oy, float oz);
void M3D_ModelBINScrollTexture(M3D_ModelBIN* m,float du, float dv);


//SKINNED MODELS
M3D_SkinnedActor *M3D_LoadSkinnedActor(const char *path, float outline, u32 psm);
void M3D_SkinnedActorRender(M3D_SkinnedActor *actor);
void M3D_SkinnedActorConfig(M3D_SkinnedActor *act, int begin, int end, float speed, int loop, int smooth);
void M3D_SkinnedActorSetPosition(M3D_SkinnedActor *act, float x, float y, float z);
void M3D_SkinnedActorCopyPosition(M3D_SkinnedActor *m0,M3D_SkinnedActor *m1);
void M3D_SkinnedActorSetRotation(M3D_SkinnedActor *act, float rx, float ry, float rz);
void M3D_SkinnedActorSetScale(M3D_SkinnedActor *act, float sx, float sy, float sz);
void M3D_SkinnedActorTranslate(M3D_SkinnedActor *act, float dx, float dy, float dz);
void M3D_SkinnedActorRotate(M3D_SkinnedActor *act, float rdx, float rdy, float rdz);
M3D_SkinnedActor *M3D_SkinnedActorClone(M3D_SkinnedActor *actorc);
void M3D_CameraFollowSkinnedActor(M3D_Camera *cam, M3D_SkinnedActor *act, float y, float max);
ScePspFVector3 M3D_SkinnedActorGetPos(M3D_SkinnedActor *actor);
//void M3D_SkinnedActorSetOcclusion(M3D_SkinnedActor *act,int value);
void M3D_SkinnedActorRenderMirror(M3D_SkinnedActor* act, u8 axis);

//MORPHING MODELS
M3D_MorphingActor *M3D_LoadMorphingActor(const char *path, float outline, u32 psm);
void M3D_MorphingActorRender(M3D_MorphingActor *actor);
void M3D_MorphingActorConfig(M3D_MorphingActor *act, int begin, int end, float speed, int smooth);
void M3D_MorphingActorSetPosition(M3D_MorphingActor *act, float x, float y, float z);
void M3D_MorphingActorCopyPosition(M3D_MorphingActor *m0,M3D_MorphingActor *m1);
void M3D_MorphingActorSetRotation(M3D_MorphingActor *act, float rx, float ry, float rz);
void M3D_MorphingActorSetScale(M3D_MorphingActor *act, float sx, float sy, float sz);
void M3D_MorphingActorTranslate(M3D_MorphingActor *act, float dx, float dy, float dz);
void M3D_MorphingActorRotate(M3D_MorphingActor *act, float rdx, float rdy, float rdz);
ScePspFVector3 M3D_MorphingActorGetPosition(M3D_MorphingActor *act);
//void M3D_MorphingActorSetOcclusion(M3D_MorphingActor *act,int value);

//NURBS SURFACE
M3D_NurbsSurface *M3D_CreateNurbsSurface(const char *texpath,float size,int steps);
void M3D_NurbsSurfaceSet(M3D_NurbsSurface *surface, int mode, float px0, float py0, float px1, float py1, float strengh, float angle);
void M3D_NurbsSurfaceRender(M3D_NurbsSurface *surface);
void M3D_NurbsSetMapping(M3D_NurbsSurface *surface, int mode);
void M3D_NurbsDelete(M3D_NurbsSurface *surface);
void M3D_NurbsSetPosition(M3D_NurbsSurface *surface,float x, float y, float z);
void M3D_NurbsSetRotation(M3D_NurbsSurface *surface,float x, float y, float z);
void M3D_NurbsSetScale(M3D_NurbsSurface *surface,float x, float y, float z);
void M3D_NurbsTranslate(M3D_NurbsSurface *surface,float x, float y, float z);
void M3D_NurbsRotate(M3D_NurbsSurface *surface,float x, float y, float z);


//PARTICLES (from OPEN TRI)
void M3D_ParticleSystemInit(int number);
void M3D_LoadParticle(int number,const char *path, signed long emitter_type, int fast, int blend);
void M3D_ParticleSetPosition(int emitter,float x, float y, float z);
void M3D_ParticleSetVelocity(int emitter,float vx, float vy, float vz);
void M3D_ParticleSetGravity(int emitter,float gx, float gy, float gz);
void M3D_ParticleSetAnimation(int emitter,int hframes, int vframes, int loops);
void M3D_ParticleSetFilter(int emitter,int filter);
void M3D_ParticleStart(int emitter,float size);
void M3D_ParticlesRender(float speed );
void M3D_ParticleStop(int emitter);

//SHADOWS
void M3D_ModelCastShadow(M3D_Model *c, int obj = 0, int alpha = 100, int type = 1, int light = 0);
void M3D_SkinnedActorCastShadow(M3D_SkinnedActor *act, int alpha = 100, int type = 1, int light = 0);

void M3D_ShadowprojectionSet(int psize,float size);

void M3D_ModelStartShadowReceiver(M3D_Model *receiver, int objnumber, float x, float y, float z, u8 alpha, u8 light);
void M3D_NurbsStartShadowReceiver(M3D_NurbsSurface *receiver, float x, float y, float z, u8 alpha, u8 light);
void M3D_ModelBINStartShadowReceiver(M3D_ModelBIN *receiver, float x, float y, float z, u8 alpha, u8 light);

void M3D_ModelRenderShadow(M3D_Model *caster, int obj_number);
void M3D_VehicleRenderShadow(M3D_Model *caster, int obj_number);
void M3D_SkinnedActorRenderShadow(M3D_SkinnedActor *act);
void M3D_EndShadowReceiver();


//TEXTURE PIXEL SHADER
void M3D_ModelLoadNormalTexture(M3D_Model *m,int obj_number, int group,const char *path, u32 psm);
void M3D_ModelSetNormalTexture(M3D_Model *m,int obj_number, int group, M3D_Camera *cam,int l);



//BULLET PHYSICS GENERAL
void M3D_BulletInitPhysics(int world_size, u32 max_objects);
void M3D_BulletUpdatePhysics(void);
void M3D_BulletFinishPhysics(void);
void M3D_BulletSetGravity(float x, float y, float z);
void M3D_ModelConfPhysics(M3D_Model *m, int obj_number, float mass, u32 shapetype);
void M3D_ModelInitPhysics(M3D_Model *model);
void M3D_ModelBINInitPhysics(M3D_ModelBIN *model);
void M3D_SkinnedActorInitPhysics(M3D_SkinnedActor *actor, float mass);

void M3D_ModelSetProperties(M3D_Model *m, int obj_number, float friction, float rollfriction, float restitution);

void M3D_ModelDeletePhysics(M3D_Model *model);
//void AMG_SkinnedActorDeletePhysics(AMG_Skinned_Actor *actor)
//void AMG_ModelBINDeletePhysics(AMG_Skinned_Actor *actor)
u32 M3D_ModelCheckCollision(M3D_Model *m,int obj_number,const void *name);

void M3D_ConstraintBall(M3D_Model *model, int obj_number, float pivotx, float pivoty, float pivotz);
void M3D_ConstraintHinge(M3D_Model *model,int obj_number, float pivotx, float pivoty, float pivotz, int axis);
void M3D_ConstraintHinge2(M3D_Model *model1,int obj_number1, M3D_Model *model2, int obj_number2,float pivotx, float pivoty, float pivotz, float pivot1x, float pivot1y, float pivot1z, int axis);
//void M3D_ConstraintDelete(M3D_Model *model);

void M3D_ModelSetForce(M3D_Model *model, int obj_number, float x, float y, float z);
char *M3D_BulletRayTracingTest(ScePspFVector3 *pos, ScePspFVector3 *vec);

void M3D_CharacterMove(M3D_SkinnedActor *act, M3D_Player *player,const void *bad_col,int item_slot,const void *item_col,float speed, float jump);
void M3D_EnemyMove(M3D_SkinnedActor *act);
void M3D_MaterialSetName(void *model, int object, const void *name);


//BULLET PHYSICS VEHICLE
void M3D_VehicleInitPhysics(M3D_Model *model, float mass, float wradius, float wwidth, float wfriction, float xd, float yd);
void M3D_VehicleRender(M3D_Model *model,M3D_Model *wheel);
void M3D_VehicleSetCam(M3D_Model *model, float y, float z, float rot);
void M3D_VehicleMove(M3D_Model *model, float velocitator, float deceleratrix, float steering);
void M3D_ModelSetMaxVelocity(M3D_Model *model, int obj_number, float vel);
//void AMG_VehicleDeletePhysics(AMG_Skinned_Actor *actor)
ScePspFVector3 M3D_VehicleGetPos(M3D_Model *model, int obj_number);




}