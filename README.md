# M3DLIB for PSP  
  
This is a "GAME ENGINE" for the PlayStation Portable, it is a wrapper around several libraries,
which are combined in one, the relevant functions are exposed as "M3D" by including M3D.h.  
Thanks to the creators of AMGLib (Andresmargar), OSLib, and bullet.  
  
I just wanted to create an engine easy to use, and powerful enough to create things  
you can call "games" for PSP.  
  
**<ins>OSLIB:</ins>** is unchanged as it was in late 2024, I include a copy here, in M3D_LIBS folder.  
If you want to access all OSL functions, just "#include <oslib/oslib.h>".  
This M3D warpper does not implemet any wifi/net/load/save functions, you can use OSL ones.  
  
**<ins>BULLET:</ins>** version 2.82-r2704, I also include it in M3D_LIBS folder.  
  
**<ins>AMGLib:</ins>** This is a heavily modified version of the original, I added a lot of functions here.  
Thanks again to Andresmargar for creating this.  
  
I added a lot of stuff to AMGLib and fixed a lot of bugs (I think)
OSLib is untouched.  

## FUNCTIONS  
  
  

## INSTALL & TEST
  
Copy libM3D.a to pspsdk libs folder, and M3D.h to include folder.
In windows you can use "install.bat" inside M3D_LIBS folder (edit the file and configure
your PSPSDKDIR first).  
   
To compile the samples or create yoour programs, run make inside any sample folder, 
or run compile_WSL.bat in windows. You need the last pspsdk (2025) which was only releassed 
for linux. If you use windows, most computers can run WSL1 from windows  (WSL 1 is fast). 
If your PC is very old and can't run WSL just install linux, or use a  virtual machine with 
any linux distrubution. If you use mac, I can't help you.  
   
   
## CREATE ASSETS

**<ins>IMAGES:</ins>** just PNG files, 32 bit RGBA, 24 bit RGB, indexed 256/16 colors.  
**<ins>MAPS:</ins>** create them using a PNG image for the tiles and TILED to create the map http://www.mapeditor.org/
maps must be in CSV format.  
**<ins>MUSIC & SOUND:</ins>** OSL handles this, it can load wav, MP3, BGM, and tracker formats (MOD,XM,IT).  
Be careful when loading tracker modules, they can use a lot of CPU/RAM.  
**<ins>STATIC MODELS:</ins>** I use Blender 2.79 for 3D model creation, because 2.8+ works very slow on my PC.  
Blender 2.7 will run very well on any PC.  
Supported models are:
  - ply: The fist version, as it is exported by Blender 2.7.
  - obj/mtl: Should have no problems with these.
  - m3b: Binary PSP models, in PSP's internal format. (Blender 2.7 export plugin included).  

**<ins>SKINNED MODELS:</ins>** These models use BONES to show animations, skinned models are incredibly complex, so I created a very simple format
(Blender 2.7 export plugin included) which is processed by the PSP when loading.  
It is probably very easy to port the plugins to newer versions of Blender, but I just didn't have the motivation to port them.  
Supported models are:
  - m3a: custom format (Blender 2.7 export plugin included).
  

## OPTIMIZE
  
1- Do not use lighting unless you really need it.  
2- Use small (< 128x128) and indexed textures (16/256 colours).  
3- Use models as low poly as you can.  
4- Avoid using nested "for" loops bigger than 128x128.  
5- Use color mode 5650 (or 5551 if you want the shadows to work) with dither enabled, it is much faster that 8888 mode and it will look good.  
