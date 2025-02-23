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
#include <M3D.h>


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

void M3D_Quit(){
	oslQuit();
}

M3D_CONTROLLER *M3D_KEYS;
