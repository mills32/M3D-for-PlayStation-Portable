#include <M3D.h>

PSP_MODULE_INFO("2D Stuff (OSLib)", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);


//Supported languages: any which supports ansi (1 byte per character) encoding.
//In your text editor, select "ANSI" encoding. To use English, you are done. 
//To use Spanish, French or any language containing non-English characters, select your character set.
//For example, Spanish and French work with "Western Europe->OEM US".
 
char test_box_8x8[] = {
	"浜様様様様様様様様様様様様�\n"
	"�  TEXT BOX 8x8  FORMAT:  �\n"
	"� OEM US / CP 437 / ANSI  �\n"
	"藩様様様様様様様様様様様様�\n"
};

char test_box_16x16[] = {
	"浜様様様様様様様融\n"
	"� TEXT BOX 16x16 �\n"
	"藩様様様様様様様夕\n"
};
	// 
//For this character set to work well, LOAD and SAVE
//source files in "OEM US FORMAT". 
char test_chars[] = {
	"\n"
	" !\"#$%%&'()*+,-./\n"
	"0123456789:;<=>?\n"
	"@ABCDEFGHIJKLMNO\n"
	"PQRSTUVWXYZ[\\]^_\n"
	"`abcdefghijklmno\n"
	"pqrstuvwxyz{|}~0\n"
	"����������������\n"
	"����������������\n"
	"�、�ぅΗ┤������\n"
	"葦音屡浜 � � 毳�\n"
	"珀矣粤肄蓍裨跋鈿\n"
	"韵鴦����������� \n"	
};


//Declaring these outside main causes choppy animations (unknown bug)
u8 Waterfall_Cycle[] = {
	2,//Number of palette cycles in the image
	0,0,18,8,//First cycle: rate (blend speed); invert; First colour; Number of colours;
	0,0,48,8 //Next  cycle: rate (blend speed); invert; First colour; Number of colours;
};
u8 Floppy_Cycle[] = {1,0,4,9,6};
u8 anim0[7] = {6, 0,1,2,2,1,0};//Sprite Animation => Number of frames; Frames;
u8 anim1[3] = {2, 0,1};
	
int main(){
	int USER_RAM = M3D_GetFreeRAM();
	int USER_VRAM = M3D_GetTotalVRAM();
	float ScrollX = 0, ScrollY = 0, Angle = 0;
	u32 Ship_X = 80,Ship_Y = 80;
	
	//Initialization
	M3D_Init(COLOR_5650,1);

	M3D_Texture *Sprite_RocketA = M3D_LoadTexture("Files/Rocketa.png",M3D_IN_VRAM,COLOR_T4);
	M3D_Texture *Sprite_RocketB = M3D_LoadTexture("Files/Rocketb.png",M3D_IN_RAM,COLOR_T4);
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	
	//Load Maps
	M3D_MAP *Layer3 = M3D_LoadMapTMX("Files/Level1_clouds.tmx",1,M3D_IN_VRAM,COLOR_T4);
	M3D_MAP *Layer2 = M3D_LoadMapTMX("Files/level1_bkg.tmx",1,M3D_IN_VRAM,COLOR_T4);		
	M3D_MAP *Layer1 = M3D_LoadMapTMX("Files/Level1.tmx",0,M3D_IN_VRAM,COLOR_T8);
	
	M3D_Texture *Floppy = M3D_LoadTexture("Files/floppy.png",0,COLOR_T4);
	//Load the sounds
	M3D_SOUND *music = M3D_LoadMP3("Files/vlogshowreel.mp3",M3D_SOUND_STREAM);
	M3D_SOUND_Loop(music, 1);
	M3D_SOUND_Play(music, 0);	
	
	//Do not call this in a loop
	int FREE_RAM = M3D_GetFreeRAM();
	int USED_VRAM = M3D_GetUsedVRAM();
	
	/*
	for (int i = 128; i< 256;i++) test_chars[i] = i;
	test_chars[256] = '\0';*/
	
	//M3D_Quit();
	while (1){
		M3D_updateScreen(0x00000000);
		
		M3D_2DMode(1);

		//The gradient blue background
		M3D_DrawGradientRect(0,0,M3D_ScreenX,M3D_ScreenY,RGBA(0,0,128,255),RGBA(0,0,128,255),RGBA(0,255,255,255),RGBA(0,255,255,255));
		
		//The maps
		Angle+=0.01;
		ScrollX+=0.5; ScrollY = 70+(M3D_Sin(Angle)*46);
		M3D_MapSetScroll(Layer3,(int)ScrollX,(int)ScrollY);
		M3D_MapSetScroll(Layer2,(int)(ScrollX*2),(int)(ScrollY*2));
		M3D_MapSetScroll(Layer1,(int)(ScrollX*4),(int)(ScrollY*4));
		
		//Animate tiles (purple bush)
		M3D_MapAnimateTiles(Layer1,2,47,61, 3,0.08);
		
		//Animate palette (water)
		M3D_PalettesCycle(2,Layer1,Waterfall_Cycle,0);
		
		//Draw maps
		M3D_DrawMap(Layer3);
		M3D_DrawMap(Layer2);
		M3D_DrawMap(Layer1);

		M3D_PalettesCycle(0,Floppy,Floppy_Cycle,1);
		M3D_DrawSprite(Floppy,0,100);
		
		//You control this rocket
		M3D_DrawSprite(Sprite_RocketB,Ship_X,Ship_Y,170,128,anim0,0.2);
		//VFPU sprite rotation
		M3D_DrawSpriteScaleRot(Sprite_RocketA,330+(M3D_Cos(Angle)*10),480+(M3D_Sin(Angle)*128)-(ScrollY*4),M3D_Cos(Angle)*40,1+(M3D_Sin(Angle)/2),64,64,anim1,0.4);
		
		//TEXT BOXES
		M3D_Printf(Font0,  8,232,0xffffffff,0,0,0,test_box_8x8);
		M3D_Printf(Font1,  8, 48,0xffffffff,0,0,0,test_box_16x16);
		M3D_Printf(Font0,340, 48,0xff888888,0,0,0,test_chars);

		//TEXT INFO
		M3D_Printf(Font1,  8,  4,0xff0000ff,0,0,0,"    OSLIB 2D STUFF SAMPLE");
		M3D_Printf(Font0,280,228,0xffffffff,0,0,0,"RENDER MODE %s\nVIDEO RAM TOTAL: %i KB\nVIDEO RAM  USED: %i KB",M3D_GetScreenMode(),USER_VRAM,USED_VRAM);
		M3D_Printf(Font0,280,252,0xffffffff,0,0,0,"MAIN RAM  USER: %i KB\nMAIN RAM  FREE: %i KB",USER_RAM,FREE_RAM);
		M3D_Printf(Font0,  8,216,0xffffCCCC,0,0,0,"L R U D: MOVE SHIP; HOME: EXIT");
		
		M3D_DrawLine(8,22,0xff00FF00,480-8,22,0xffff0000);
		
		M3D_2DMode(0);

		M3D_ReadButtons();
		if (M3D_KEYS->held.right) Ship_X++;
		if (M3D_KEYS->held.left) Ship_X--;
		if (M3D_KEYS->held.up) Ship_Y--;
		if (M3D_KEYS->held.down) Ship_Y++;
		
		//Buttons 
		if (M3D_KEYS->pressed.triangle) M3D_Quit();			//Exit the sample	
	}

	M3D_MapUnload(Layer1);
	M3D_MapUnload(Layer2);
	M3D_MapUnload(Layer3);
	
	return 0;
}
