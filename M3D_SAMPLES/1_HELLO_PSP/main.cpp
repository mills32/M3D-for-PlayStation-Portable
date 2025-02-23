
/*
M3DLIB for PSP
--------------

This uses AMGLib by ... OSLib by ... and Bullet ...
I added a lot of stuff to AMGLib and fixed a lot of bugs (I think)
OSLib is untouched.


I just wanted to create an engine easy to use, and powerful enough to create things
you can call "games" for PSP. Many things were already there in the first AMGLib
but I added so many features I called this "M3D" and created another layer to hide
most AMG and OSLib functions and structures. 

You still can access all AMG and OSLib functions by including these:
#include <oslib/oslib.h>
#include <AMG/AMGLib.h>


To use this you need the last pspsdk (2025) which was only releassed for linux.
If you use windows, most computers can run WSL from windows (WSL 1 is OK).
If your PC is very old and can't run WSL just install linux, or use a virtual 
machine with any linux distrubution.
If you use mac, I can't help you.

You need Blender 2.79 for 3D model creation, and animations. Why that version? 
- Blender 2.8+ works very slow on my PC, (and it is not old or slow by any means)
- Blender 2.7 only requires 32-bit CPU, SSE2 support, OpenGL 2.1 and 512 MB of RAM,
So it works on 100% of computers (2025), from toasters to modern ones, and I hope
it will probably work on computers up to 2040 if nothing weird happens to windows
or linux OS.

To create animated models I included an export plugins for Blender 2.79 and sample files.


OPTIMIZATIONS

1- Do not use lighting unless you really need it
2- Use small (< 128x128) and indexed textures (16/256 colours)
3- Use models as low poly as you can.
4- Avoid using nested "for" loops bigger than 128x128, (PSP CPU just dies).

*/

#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

char infotext[] = {
	" WELLCOME TO THE AWESOME M3D LIB (AMGLIB + OSLIB + BULLET). \n"
	" I GATHERED ALL INFO I COULD FIND, ADDING/IMPROOVING 2D&3D  \n"
	" FUNCTIONS, LIKE: INDEXED TEXTURE PALETTES, PHYSICS,        \n"
	" ANIMATED 3D MODELS (HARDWARE MORPHING/SKINNING), RENDER TO \n"
	" TEXTURE, SHADOW PROJECTION, MIPMAPPING...\n\n"
	"              HOPE SOMEBODY FINDS THIS USEFUL              "
};

//MAIN FUNCTION: PROGRAM STARTS HERE
int main(){
	int USER_RAM = M3D_GetFreeRAM();
	int USER_VRAM = M3D_GetTotalVRAM();
	//Initializes OSLib (2D stuff, some loading functions and audio) and AMGLib (3D stuff and rendering)
	M3D_Init(COLOR_8888,0);//Modes 5650, 5551, 4444, 8888
	M3D_FrameSkip(0); //tries to keep 60 fps, drops to 30 when too much
	//Load
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);

	//Do not call this in a loop
	int FREE_RAM = M3D_GetFreeRAM();
	
	//MAIN LOOP
	while (1){
		M3D_updateScreen(0x00000000);
		M3D_2DMode(1);

		M3D_Printf(Font1,  0, 80,0xff44ff44,0,0,0,"        --HELLO  PSP--        ");
		M3D_Printf(Font0,  0,120,0xffffffff,0,0,0,infotext );
		M3D_Printf(Font0,280,220,0xffffffff,0,0,0,"CPU: %i Mhz",M3D_GetCpuSpeed());
		M3D_Printf(Font0,280,228,0xffffffff,0,0,0,"RENDER MODE %s\nVIDEO RAM TOTAL: %i KB\nVIDEO RAM  USED: %i KB",M3D_GetScreenMode(),USER_VRAM,M3D_GetUsedVRAM());
		M3D_Printf(Font0,280,252,0xffffffff,0,0,0,"MAIN RAM  USER: %i KB\nMAIN RAM  FREE: %i KB",USER_RAM,FREE_RAM);
		
		M3D_ReadButtons();
		//You can also exit using HOME button.
		if (M3D_KEYS->pressed.triangle) M3D_Quit();	    
	}
	
	return 0;
}
	
