#include <M3D.h>

PSP_MODULE_INFO("Hello World", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(12*1024);

//implement automatic mipmapping or custom mipmapping (use texname_m1.png and texname_m2.png)
//Textures must be at least 64 bytes width to support mipmaps. Smaller textures can be used
//but the code to load them correctly is not worth it.

char text[] = {"\
Some programs like pngquant, can create palettes using  \n\
RGBA colors, so indexed textures could have RGBA colors.\n\
But most programs (GIMP, Photoshop) do not support this,\n\
So PURE BLUE color will be transparent in 16 and 256 color\n\
images.\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\
To use custom mipmaps:\n\
    -main texture: name.png\n\
    -mipmap1:      mame_mip1.png\n\
    -mipmap2:      name_mip2.png"
};	
	
	
int main(){
	//float ram1 = oslGetRamStatus().maxAvailable /1024/1024;
	M3D_Init(COLOR_8888,0);
	
	// Crea una camara
    M3D_Camera *camera = M3D_CameraInit();
	M3D_CameraSetPosition(camera,0,0,4);
	M3D_CameraSetEye(camera,0,0,0);
	M3D_InitMatrixSystem(45.0f,0.5,100,1);
	M3D_SetMipMapping(1,0.6);
	
	M3D_Texture *Font0 = M3D_GetFont(0);
	M3D_Texture *Font1 = M3D_GetFont(1);
	M3D_Model *Ground_32 = M3D_LoadModelPLY("Files/ground.ply",0, COLOR_8888);
	M3D_Model *Plane_32 = M3D_LoadModelPLY("Files/tex_32.ply",0, COLOR_8888);
	M3D_Model *Plane_256 = M3D_LoadModelPLY("Files/tex_256.ply",0, COLOR_T8);
	M3D_Model *Plane_16 = M3D_LoadModelPLY("Files/tex_16.ply",0, COLOR_T4);
	M3D_ModelSetLighting(Plane_32,0,0);
	M3D_ModelSetLighting(Plane_256,0,0);
	M3D_ModelSetLighting(Plane_16,0,0);

	M3D_ModelSetPosition(Plane_256,0,1.2,0,0);
	M3D_ModelSetPosition(Plane_16,0,-1.6,0,0);

	while(1){
		M3D_updateScreen(0x00000000);
		M3D_CameraSet(camera);
		
		M3D_ModelRender(Ground_32,0);
		M3D_ModelRender(Plane_256,0);
		M3D_ModelRender(Plane_32,0);
		M3D_ModelRender(Plane_16,0);
		
		//DRAW 2D STUFF
		M3D_2DMode(1);
			M3D_Printf(Font1,0, 8,0xffddffdd,0,0,0,"         MIP  MAPPING         ");
			M3D_Printf(Font0,0,32,0xffffffff,0,0,0," UP/DOWN to move camera");
			M3D_Printf(Font0,8,200,0xffffffff,0,0,0,text);
			M3D_DrawLine(32,24,0xff00FF00,480-32,24,0xffff0000);
		M3D_2DMode(0);
		
		M3D_ReadButtons();
		if (M3D_KEYS->held.up) M3D_CameraMove(camera,0,0,-0.1);
		if (M3D_KEYS->held.down) M3D_CameraMove(camera,0,0,+0.1); 
		if (M3D_KEYS->pressed.triangle) M3D_Quit();
	}
	
	return 0;
}
