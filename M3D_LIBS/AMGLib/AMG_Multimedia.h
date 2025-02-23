#ifndef _AMG_MULTIMEDIA_H_
#define _AMG_MULTIMEDIA_H_


#include "AMG_Texture.h"


#ifdef __cplusplus
	extern "C" {
#endif

// Includes
#include <psptypes.h>

//Modify OSLib MikMod reverb
void M3D_MikModReverb(u8 val);

//VIDEO MJPEG+AAC (AVI)

int AMG_Load_AVI(char *path);

void AMG_Play_FullScreen_AVI(char *path, int loop, u32 button);

void AMG_Play_AVI_ToTexture(int video_handle, int loop, AMG_Texture *tex);

void AMG_Stop_AVI_ToTexture(AMG_Texture *tex);

void AMG_Close_AVI(int video_handle);

int Load_Play_AVI_H264(const char *path,u32 button);



#ifdef __cplusplus
	}
#endif

#endif
