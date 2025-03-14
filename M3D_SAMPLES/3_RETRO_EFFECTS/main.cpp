
#include <M3D.h>

PSP_MODULE_INFO("Retro Effects", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


char demo_text[] = {
	" WRITE SOME WEIRD TEXT HERE, (THE LONGER, THE BETTER).      "
	"FOR AN AUTHENTIC DEMOSCENE EXPERIENCE, BE SURE THIS TEXT MAKES NO SENSE AT ALL!!!.  "
};

char demo_text1[] = {
	0x20,0x21,0x23,0x14,0x16,
};

//MAIN FUNCTION: PROGRAM STARTS HERE
int main(){
	
	//Some variables
	float wavet = 0.0f;
	u8 floppy_cycle[] = {1,0,4,9,6};
	int demo_text_x = 512;
	int effect_alpha = 0;
	int bkg_demo_effect = 0; //0 copper bars; 1 rotozoom; 2 full screen plasma; 3 twister
	int demo_time = 0; 
	
	M3D_Init(COLOR_8888,1);
	M3D_FrameSkip(0);

	//Load images
	M3D_Texture *Font1 = M3D_LoadTexture("Files/demofont.png",0,COLOR_T4);
	M3D_Texture *Logo = M3D_LoadTexture("files/logo.png",0,COLOR_T4);
	M3D_Texture *Floppy = M3D_LoadTexture("files/floppy.png",0,COLOR_T4);
	M3D_Texture *RotoBKG = M3D_LoadTexture("files/rotozoom.png",0,COLOR_T8);
	M3D_Plasma2DSet("files/plasma_palette.png");
	
	//Load a sample mod file
	M3D_MikModReverb(0);
	M3D_SOUND *Tune = M3D_LoadMOD("files/enigma.mod",M3D_SOUND_NOSTREAM);
	M3D_SOUND_Loop(Tune, 1);
	M3D_SOUND_Play(Tune, 1);	
	
	//Fade in from black
	M3D_SetFade(1,0,1);

	//MAIN LOOP
	while (1){
		M3D_updateScreen(0x00000000);
		M3D_2DMode(1);
		
		//COPPER BARS
		if (bkg_demo_effect == 0)M3D_DrawCopperBars(8,16,120,80,effect_alpha,0.02);
		//ROTOZOOM
		if (bkg_demo_effect == 1){
			M3D_DrawRotoZoom(RotoBKG,effect_alpha,M3D_Sin(wavet/8)*256,0,wavet*4,0.5+(M3D_Sin(wavet/16)/4));
		}
		//PLASMA
		if (bkg_demo_effect == 2) M3D_Draw2DPlasma(effect_alpha,3,1,1,1);

		M3D_DrawImageWave(Logo,80,30,wavet,M3D_EFFECT_H_WAVE,4, 64);
		
		M3D_PalettesCycle(0,Floppy,floppy_cycle,1);
		M3D_DrawSpriteScaleRot(Floppy,48,48,wavet*4,1,0,0,0,0);
		
		M3D_Printf(Font1,demo_text_x--, 230,0xff00FF00, 16,2,wavet,demo_text);
		
		M3D_2DMode(0);
		
		//Ipdate variables
		if (demo_text_x < -2200) demo_text_x = 512;
		if (demo_time < 63 ) effect_alpha+=4;//Fade in effect
		if (demo_time > 1024-63 ) effect_alpha-=4;//Fade out effect
		if (demo_time == 1024) {bkg_demo_effect++;demo_time = 0;effect_alpha = 0;}
		if (bkg_demo_effect == 3) bkg_demo_effect = 0;
		
		wavet-=0.1;
		demo_time++;
		
		M3D_ReadButtons();
		//You can also exit using HOME button.
        if (M3D_KEYS->pressed.triangle) M3D_Quit();   
	}
	
	return 0;
}
	
