# M3DLIB for PSP
# --------------

This is a "GAME ENGINE" for the PlayStation Portable, it is a wrapper around several libraries,
which are combined in one, the relevant functions are exposed as "M3D" by including M3D.h.
Thanks to the creators of AMGLib (Andresmargar), OSLib, and bullet.

I just wanted to create an engine easy to use, and powerful enough to create things
you can call "games" for PSP.

OSLIB: is unchanged as it was in late 2024, I include a copy here, in M3D_LIBS folder.
If you want to access all OSL functions, just "#include <oslib/oslib.h>"
This M3D warpper does not implemet any wifi/net/load/save functions, you can use OSL ones.

BULLET: version 2.82-r2704, I also include it in M3D_LIBS folder.

AMGLib: This is a heavily modified version of the original, I added a lot of functions here.
Thanks again to Andresmargar for creating this.

I added a lot of stuff to AMGLib and fixed a lot of bugs (I think)
OSLib is untouched.

## FUNCTIONS
## ---------



## INSTALL & TEST
## --------------

Copy libM3D.a to pspsdl libs folder, and M3D.h to include folder.
In windows you can use "install.bat" inside M3D_LIBS folder (edit the file and configure
your PSPSDKDIR first).

To compile the samples or create yoour programs, run make inside any sample folder, 
or run compile_WSL.bat in windows. You need the last pspsdk (2025) which was only releassed 
for linux. If you use windows, most computers can run WSL1 from windows  (WSL 1 is fast). 
If your PC is very old and can't run WSL just install linux, or use a  virtual machine with 
any linux distrubution. If you use mac, I can't help you.

## CREATE ASSETS
## --------------

**IMAGES:** just PNG files, 32 bit RGBA, 24 bit RGB, indexed 256/16 colors.
**MAPS:** create them using a PNG image for the tiles and TILED to create the map http://www.mapeditor.org/
maps must be in CSV format
**MUSIC & SOUND:** OSL handles this, it can load wav, MP3, BGM, and tracker formats (MOD,XM,IT). 
Be careful when loading tracker modules, they can use a lot of CPU/RAM.

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
