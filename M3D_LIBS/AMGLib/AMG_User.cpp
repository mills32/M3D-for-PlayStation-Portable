// Includes
#include "AMG_User.h"
#include <time.h>
#include <psprtc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include "AMG_3D.h"
#include <oslib/oslib.h>
#include <pspmoduleinfo.h>
#include <psppower.h>
#include "AMG_Texture.h"
#include "AMG_Model.h"

// Cambia la velocidad de CPU y el BUS (bus = cpu/2)
void M3D_SetCpuSpeed(int mhz){
	int pllfreq = 0; //valid from 19-333
	int cpufreq = 0; //valid from 1-333
	int busfreq = 0; //valid from 1-167 CPU/2
	
	if ((mhz <1)||(mhz>333)) mhz = 222;
	cpufreq = mhz;
	pllfreq = cpufreq;
	if (pllfreq < 19) pllfreq = 19;
	busfreq = cpufreq/2;
	if (!busfreq) busfreq = 1;
	scePowerSetClockFrequency(pllfreq,cpufreq,busfreq);
}

//Obtiene la velocidad de la CPU
int M3D_GetCpuSpeed(){
	return scePowerGetCpuClockFrequency();
}


int AMG_PSP_MessageTVOUT(){
	const char message[] = {"    TV OUT  720x480 \n  (CONNECT CABLE)"};
	static unsigned int __attribute__((aligned(16))) list[262144];
	
	sceGuInit();
    sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(480,272,(void*)0x88000,512);
    sceGuDepthBuffer((void*)0x110000,512);
    sceGuOffset(2048 - (480/2),2048 - (272/2));
    sceGuViewport(2048,2048,480,272);
    sceGuDepthRange(0xc350,0x2710);
    sceGuScissor(0,0,480,272);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_CULL_FACE);
    sceGuEnable(GU_CLIP_PLANES);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
	
	pspUtilityMsgDialogParams dialog;
	memset(&dialog, 0, sizeof(dialog));
    dialog.base.size = sizeof(dialog);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&dialog.base.language); // Prompt language
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&dialog.base.buttonSwap); // X/O button swap

    dialog.base.graphicsThread = 0x11;
    dialog.base.accessThread = 0x13;
    dialog.base.fontThread = 0x12;
    dialog.base.soundThread = 0x10;
    dialog.mode = PSP_UTILITY_MSGDIALOG_MODE_TEXT;
	dialog.options = PSP_UTILITY_MSGDIALOG_OPTION_TEXT;
	dialog.options |= PSP_UTILITY_MSGDIALOG_OPTION_YESNO_BUTTONS|PSP_UTILITY_MSGDIALOG_OPTION_DEFAULT_NO;		
	
    strcpy(dialog.message, message);

    sceUtilityMsgDialogInitStart(&dialog);
	int running = 1;
    while(running){
		sceGuStart(GU_DIRECT,list);
		// clear screen
		sceGuClearColor(0xff000000);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
		sceGuFinish();
		sceGuSync(0,0);
	
		switch(sceUtilityMsgDialogGetStatus()) {
			case 2: sceUtilityMsgDialogUpdate(1);break;
			case 3: sceUtilityMsgDialogShutdownStart();break;
			case 0: running = 0;	
		}
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
    }
	
	if(dialog.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_YES) return 1;
	else if(dialog.buttonPressed == PSP_UTILITY_MSGDIALOG_RESULT_NO) return 0;
	else return 0;
}

void M3D_ReadButtons(){
	oslReadKeys();
}

void M3D_Wait(u32 seconds){
	u32 counter = 0;
	while(counter < (u32)(seconds*60)){
		counter++;
		sceDisplayWaitVblankStart();
	}
}


//Loading animation
SceUID AMG_Loading_Thread;
void *AMG_Loading_A;
AMG_Texture *AMG_Loading_Sprite;
int AMG_LOADING = 0;
int AMG_LOADING_X = 0;
int AMG_LOADING_Y = 0;
u16 AMG_LOADING_TX = 0;
u16 AMG_LOADING_TY = 0;
float AMG_LOADING_SPEED = 0;

SceKernelThreadEntry AMG_Loading_Thread_loop(){
	u8 anim[256] = {0};
	u16 tiles = (AMG_Loading_Sprite->Width/AMG_LOADING_TX)*(AMG_Loading_Sprite->Height/AMG_LOADING_TY);
	if (tiles > 256) AMG_Error("TOO MANY TILES IN LOADING ANIMATION\nDEMASIADOS TLES EN ANIMACIÓN DE CARGA",0);
	anim[0] = tiles;
	for (int i = 0; i < tiles;i++) anim[i+1] = i;
	int x,y;
	if (AMG_LOADING_X == 0 && AMG_LOADING_Y == 0){
		x = (AMG.ScreenWidth/2) - (AMG_LOADING_TX/2);
		y = (AMG.ScreenHeight/2) - (AMG_LOADING_TY/2);
	} else {
		x = AMG_LOADING_X;
		y = AMG_LOADING_Y;
	}
	while(AMG_LOADING){
		M3D_2DMode(1);
		M3D_DrawSprite((M3D_Texture*)AMG_Loading_Sprite,x,y,AMG_LOADING_TX,AMG_LOADING_TY,anim,AMG_LOADING_SPEED);
		M3D_2DMode(0);
		M3D_updateScreen(0);
	}
	sceKernelExitThread(1);
	return 0;
}

void M3D_Loading_Start(const char *path, int x, int y, u16 tx, u16 ty, float speed){
	AMG_LOADING = 1;
	AMG_LOADING_X = x;
	AMG_LOADING_Y = y;
	AMG_LOADING_TX = tx;
	AMG_LOADING_TY = ty;
	AMG_LOADING_SPEED = speed;
	AMG_UnloadTexture(AMG_Loading_Sprite);
	AMG_Loading_Sprite = (AMG_Texture*) M3D_LoadTexture(path,M3D_IN_RAM,COLOR_8888);
	AMG_Loading_Thread = sceKernelCreateThread("SHOW ANIMATION",(SceKernelThreadEntry)AMG_Loading_Thread_loop, 0x8, 0x10000, 0, 0);
	sceKernelStartThread(AMG_Loading_Thread, 4, &AMG_Loading_A);
}

void M3D_Loading_Stop(){
	AMG_LOADING = 0;
	sceKernelWaitThreadEnd(AMG_Loading_Thread, 0);
	AMG_UnloadTexture(AMG_Loading_Sprite);
}

void M3D_Quit(){
	oslQuit();
}

M3D_CONTROLLER *M3D_KEYS;

/*

SceKernelThreadEntry AMG_Loading_Thread_loop(){
	u8 anim[] = {4, 0,1,2,3};
	while(AMG_LOADING){
		if (AMG_Loading_Type == 0) {
			int x = (AMG.ScreenWidth/2) - (AMG_Loading_Sprite->Width/4);
			int y = (AMG.ScreenHeight/2) - (AMG_Loading_Sprite->Height/4);
			M3D_2DMode(1);
			M3D_DrawSprite((M3D_Texture*)AMG_Loading_Sprite,x,y,AMG_Loading_Sprite->Width/2,AMG_Loading_Sprite->Height/2,anim,0.04);
			M3D_2DMode(0);
		}
		if (AMG_Loading_Type == 1) {
			M3D_CameraSet(AMG_Loading_Camera);
			M3D_CameraSetPosition(AMG_Loading_Camera,0,0,3);
			M3D_CameraSetEye(AMG_Loading_Camera,0,0,0);
			AMG_RenderSkinnedActor(AMG_Loading_SkinnedActor);
		}
		M3D_updateScreen(0);
	}
	sceKernelExitThread(1);
	return 0;
}

void M3D_Loading_Start(int type, const char *path, float speed){
	if (type < 0 || type > 2) AMG_Error("ANIMATION TYPE MUST BE 0, 1 or 2\nTIPO DE ANIMACIÓN DEBE SER 0, 1 o 2",0);
	AMG_LOADING = 1;
	AMG_Loading_Type = type;
	if (AMG_Loading_Type == 0) {
		AMG_UnloadTexture(AMG_Loading_Sprite);
		AMG_Loading_Sprite = (AMG_Texture*) M3D_LoadTexture(path,M3D_IN_RAM,COLOR_8888);
	}
	if (AMG_Loading_Type == 1) {
		M3D_SkinnedActorUnload((M3D_SkinnedActor*)AMG_Loading_SkinnedActor);
		AMG_Loading_SkinnedActor = AMG_LoadSkinnedActor(path,0,COLOR_4444);
		AMG_ConfigSkinnedActor(AMG_Loading_SkinnedActor,0,3,speed,1,0);
		if (AMG_Loading_Camera) {free(AMG_Loading_Camera); AMG_Loading_Camera = NULL;}
		AMG_Loading_Camera = M3D_CameraInit();
		AMG_Push_Perspective_Matrix((float)AMG.ScreenWidth,(float)AMG.ScreenHeight,45);
		AMG_InitMatrixSystem(55.0f,0.5,100,1);
		M3D_2DMode(0);
	}
	
	AMG_Loading_Thread = sceKernelCreateThread("Loading animation",(SceKernelThreadEntry)AMG_Loading_Thread_loop, 0x8, 0x10000, 0, 0);
	sceKernelStartThread(AMG_Loading_Thread, 4, &AMG_Loading_A);
}

void M3D_Loading_Stop(){
	AMG_LOADING = 0;
	//sceKernelTerminateDeleteThread(AMG_Loading_Thread);
	sceKernelWaitThreadEnd(AMG_Loading_Thread, 0);
	if (AMG_Loading_Type == 0) AMG_UnloadTexture(AMG_Loading_Sprite);
	if (AMG_Loading_Type == 1) M3D_SkinnedActorUnload((M3D_SkinnedActor*)AMG_Loading_SkinnedActor);
	AMG_Pop_Perspective_Matrix();
}*/