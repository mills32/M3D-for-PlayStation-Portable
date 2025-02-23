#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>

#include <oslib/oslib.h>

PSP_MODULE_INFO("intraFontTest", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);



int main(){

    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslInitAudio();

    oslIntraFontInit(INTRAFONT_CACHE_ALL | INTRAFONT_STRING_UTF8); // All fonts loaded with oslLoadIntraFontFile will have UTF8 support

    // Load fonts 
    OSL_FONT* ltn = oslLoadFontFile("c64_pro_mono-style.pgf");
    oslIntraFontSetStyle(ltn, 1.0f,0xFFFFFFFF,0x00000000,INTRAFONT_ALIGN_LEFT);
	oslStartDrawing();
	oslSetFont(ltn);
	oslDrawString(16,16, "L");
    oslEndDrawing();
	oslSyncFrame();
    while(1){};
	sceKernelExitGame();
    return 0;
}
