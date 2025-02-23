// Includes
#include "AMG_Multimedia.h"
#include "AMG_3D.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <psputility.h>
#include <pspatrac3.h>
#include <pspaudiocodec.h>
#include <psputility_avmodules.h>
#include <pspmpeg.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspjpeg.h>
#include <pspaudio.h>
#include <pspctrl.h>
#include <psprtc.h>


// A dirty way of modifying OSLib mikmod Reverb.
//Any value > 0 kills the cpu. But you can use it if the CPU is doing nothing at all
void M3D_MikModReverb(u8 val){
	extern u8 md_reverb;  
	if (val < 0) val = 0;
	if (val > 6) val = 6;//too much
	md_reverb = val;
}
	
/////VIDEO 
//This plays a crappy video format: MJPEG+AAC (MP3 sound would be even crappier).
//I wanted to create an mp4/AVC/h264/PMF player but it was just impossible, on latest FW + SDK.
//
// "AMG_Play_FullScreen_AVI" works like a regular video player with sound, but it has no sync functions.
// So audio will be in sync for a limited time, according to my tests, 10 minute videos are ok.
// Anyway, this function is only ment for small FMV (1 minute or 2 at most, because mjpeg videos are BIG).
extern "C" {

u32 AVI_Tex_Swizzle = 0;
u32 MJPEG_unknown = 0;
int AVI_Tex_STOP = 0;
int AVI_AAC_STOP = 0;
u32 AVI_TotalFrames = 0;
u32 AVI_TotalAACFrames = 0;
u64 AVI_Last_Tick = 0;
u64 AVI_Current_Tick = 0;
u32 AVI_Frame = 0;
u32 AVI_AAC_Frame = 0;
u64 AVI_MicroSecPerFrame = 0;
u32 AVI_Loop = 0;
u32 AVI_Streams = 0;
u32 AVI_Width = 0;
u32 AVI_Height = 0;
u32 AVI_WidthP2 = 0;
u32 AVI_HeightP2 = 0;
SceUID AVI_decode_thread;
int AVI_Tex_Handle = 0; 
int AVI_Tex_Loop = 0;
void *AVI_Tex_Target;
u32 *AVI_MJPEG_Offsets;//Positions of 00dc
u32 *AVI_AAC_Offsets;//Positions of 01wb
SceUID AVI_AAC_thread;
int AVI_AAC_File = 0;
int AVI_AAC_Handle = 0;
void *AVI_p;

unsigned long aac_codec_buffer_VIDEO[2048] __attribute__((aligned(64)));
u8 aac_data_buffer[1024] __attribute__((aligned(64)));
short pcm_output_buffer0[2][8*1024] __attribute__((aligned(64)));
u8 MJPEG_frame[256*1024] __attribute__((aligned(64)));
int pcm_output_index = 0;
SceUID aac_handle;
u32 aac_sample_per_frame = 2048;
u8 aac_getEDRAM = 0;
u32 aac_channels;
u32 aac_samplerate;
int AMG_AVIMJPEG_eof = 0;

//Only once, if you do this twice it will crash
// "sceAudiocodecReleaseEDRAM(aac_codec_buffer_VIDEO)" Also crashed  
void AMG_Set_aac_VIDEO_EDRAM(){
	memset(aac_codec_buffer_VIDEO, 0, sizeof(aac_codec_buffer_VIDEO));
	if (sceAudiocodecCheckNeedMem(aac_codec_buffer_VIDEO, 0x1003) < 0) return;
	if (sceAudiocodecGetEDRAM(aac_codec_buffer_VIDEO, 0x1003) < 0 ) return;
	aac_codec_buffer_VIDEO[6] = (unsigned long)aac_data_buffer;
	aac_getEDRAM = 1;
}

int decode_avi_AAC(SceSize args, void *argp){
	if (!aac_getEDRAM) AMG_Set_aac_VIDEO_EDRAM();
	aac_codec_buffer_VIDEO[10] = 44100;
	if (sceAudiocodecInit(aac_codec_buffer_VIDEO, 0x1003) < 0) return 0;
	
	while( !AMG_AVIMJPEG_eof ) {
		sceIoLseek32(aac_handle, AVI_AAC_Offsets[AVI_AAC_Frame], PSP_SEEK_SET); 
		sceIoRead(aac_handle,aac_data_buffer,AVI_AAC_Offsets[AVI_AAC_Frame+1]);
		aac_codec_buffer_VIDEO[8] = (unsigned long)pcm_output_buffer0[pcm_output_index&1];
		aac_codec_buffer_VIDEO[7] = AVI_AAC_Offsets[AVI_AAC_Frame+1];
		aac_codec_buffer_VIDEO[9] = 1024;
		sceAudiocodecDecode(aac_codec_buffer_VIDEO, 0x1003);
		sceAudioSRCChReserve( 1024,44100,2 );
		sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX,pcm_output_buffer0[pcm_output_index&1]);
		pcm_output_index++;
		
		AVI_AAC_Frame+=2;
		if (AVI_AAC_STOP) break;
		if (AVI_AAC_Frame > AVI_TotalAACFrames){
			if (AVI_Loop) AVI_AAC_Frame = 0;
			else break;
		}
		sceKernelDelayThread(11000);
	}

	if (aac_handle) sceIoClose(aac_handle);
	int release = -1;
	while (release < 0) {
		release = sceAudioSRCChRelease();
		sceKernelDelayThread(1000);
	}
	sceKernelExitThread(1);
	return 0;
}

void AMG_Draw_2D_AVI(void *rendertarget,u16 x, u16 y){
	//Define a quad (u v x y z, u v x y z)
	u16 data[] = {0,0,x,y,0,(u16)AVI_Width,(u16)AVI_Height,(u16)(AVI_Width+x),(u16)(AVI_Height+y),0}; 
	u16 *QUAD = (u16*) sceGuGetMemory(10*sizeof(u16));
	memcpy(QUAD,data,10*sizeof(u16));
	// setup texture
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexFunc(GU_TFX_REPLACE,GU_TFX_REPLACE);
	sceGuTexScale(1.0f, 1.0f);
	sceGuTexFilter(GU_NEAREST,GU_NEAREST);
	sceGuTexOffset(0,0);
	sceGuTexMapMode(GU_TEXTURE_COORDS,0,1);
	sceGuTexMode(GU_PSM_8888,0, 0,0);
	sceGuTexImage(0,AVI_WidthP2,AVI_HeightP2,AVI_WidthP2,rendertarget);
	
	//Draw
	sceGuDrawArray(GU_SPRITES, (GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D),2, 0,QUAD);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuEnable(GU_BLEND);
}

//next power of two (from pmp player)
u32 next_pow2(u32 v){
	v-=1;v|=(v>>1);v|=(v>>2);v|=(v>>4);v|=(v>>8);v|=(v>>16);return(v+1);
}

int AMG_Load_AVI(char *path){
	u8 movi = 0;
	u8 idx1 = 0;
	u32 jpegframe = 0;
	u32 aacframe = 0;
	u32 DC = 0x63643030;//mjpeg frame 00dc
	u32 WB = 0x62773130;//aac chunk 01wb
	u32 MOVI = 0x69766f6D;//movi chunk
	u32 INFO = 0x4F464E49;//info chunk
	u32 JUNK = 0x4B4E554A;//junk block
	u32 IDX = 0x31786469;//index block   

	typedef struct{
		u32 name;
		u32 flags;
		u32 offset;
		u32 size;
		u32 name1;
		u32 flags1;
		u32 offset1;
		u32 size1;
	}chunk;
	chunk c;
	
	u32 movi_offset = 0;
	u32 movi_size = 0;
	//u32 idx1_offset = 0;
	u32 idx1_size = 0;
	u32 relative_offset = 0;
	u32 av1 = 0;u32 size = 0; u32 av3 = 0;
	u32 hdrl_size = 0;
	u32 avih_size = 0;
	
	int f = sceIoOpen(path, PSP_O_RDONLY, 0777 );
	if (f<0) AMG_Error((char*)"File not found / Archivo no encontrado",path);
	AVI_Frame = 0;
	AVI_AAC_Frame = 0;
	sceIoRead(f,&av1,4);
	sceIoRead(f,&size,4);
	sceIoRead(f,&av3,4);
	if (av1 != 0x46464952) AMG_Error((char*)"Not an AVI file / no es un archivo AVI",path);//RIFF
	if (av3 != 0x20495641) AMG_Error((char*)"Not an AVI file / no es un archivo AVI",path);//AVI_
	sceIoLseek32(f,0x04,SEEK_CUR);	
	sceIoRead(f,&hdrl_size,4);
	movi_offset = hdrl_size+16+8+4;
	sceIoLseek32(f,0x08,SEEK_CUR);
	sceIoRead(f,&avih_size,4);
	if (avih_size != 0x38) return 0;//RIFF
	sceIoRead(f,&AVI_MicroSecPerFrame,4);
	sceIoLseek32(f,12,SEEK_CUR);
	sceIoRead(f,&AVI_TotalFrames,4);
	sceIoLseek32(f,4,SEEK_CUR);
	sceIoRead(f,&AVI_Streams,4);
	sceIoLseek32(f,4,SEEK_CUR);
	sceIoRead(f,&AVI_Width,4);
	sceIoRead(f,&AVI_Height,4);
	AVI_MJPEG_Offsets = (u32*)calloc(AVI_TotalFrames*2,4);
	AVI_AAC_Offsets = (u32*)calloc(AVI_TotalFrames*16,4);
	//look for movi
	u32 offset2 = sceIoLseek32(f,movi_offset,SEEK_SET);
	while (!movi){
		int b = sceIoRead(f,&c.name,4);
		if (b == 0) {
			free(AVI_MJPEG_Offsets);
			AVI_MJPEG_Offsets=NULL;
			return 0;
		}
		if (c.name == MOVI) {
			movi = 1;
			movi_offset = sceIoLseek(f,0, SEEK_CUR);
			break;
		}
		if (c.name == INFO) {
			sceIoLseek32(f,offset2-4,SEEK_SET);
			sceIoRead(f,&size,4);
			sceIoLseek32(f,size,SEEK_CUR);
			offset2 = sceIoLseek(f,0, SEEK_CUR);
		}
		if (c.name == JUNK) {
			sceIoRead(f,&size,4);
			sceIoLseek32(f,size+8,SEEK_CUR);
		}
	}
	
	//look for idx1
	sceIoLseek32(f,movi_offset-8,SEEK_SET);
	sceIoRead(f,&movi_size,4);
	sceIoLseek32(f,movi_offset+movi_size-4,SEEK_SET);
	sceIoRead(f,&c.name,4);
	if (c.name == IDX) idx1 = 1;
	if (idx1) {
		u32 size = 0;
		//go to idx1
		sceIoLseek32(f,movi_offset+movi_size,SEEK_SET);
		sceIoRead(f,&idx1_size,4);
		
		//check if first offset is absolute or relative (to movi)
		sceIoLseek32(f,8,SEEK_CUR);
		sceIoRead(f,&c.offset,4);//read first offset
		if (c.offset == 4) relative_offset = movi_offset+4;
		//Go bak to read all chunks
		sceIoLseek32(f,movi_offset+movi_size+4,SEEK_SET);
		//read chunks
		while(size != idx1_size){
			sceIoRead(f,&c,16);
			if (c.name == DC) {
				AVI_MJPEG_Offsets[jpegframe++] = c.offset+relative_offset;
				AVI_MJPEG_Offsets[jpegframe++] = c.size;
				//pspDebugScreenPrintf(" %04X %04X\n",c.offset+relative_offset,c.size);
				//while(1);
			} else if (c.name == WB) {
				AVI_AAC_Offsets[aacframe++] = c.offset+relative_offset;
				AVI_AAC_Offsets[aacframe++] = c.size;
				//pspDebugScreenPrintf(" %04X %04X\n",c.offset+relative_offset,c.size);
			}
			size+=16;
		}
		AVI_TotalAACFrames = aacframe;
	} else {
		sceIoLseek32(f,movi_offset+4,SEEK_SET);
		//AVI should have an idx1 chunk
		free(AVI_MJPEG_Offsets);
		AVI_MJPEG_Offsets=NULL;
		return 0;
	}
	
	if((AVI_Width<769)&&(AVI_Height<513)){
		AVI_WidthP2 = next_pow2(AVI_Width);
		AVI_HeightP2 = next_pow2(AVI_Height);
		if (AVI_Width > 512) {
			AVI_WidthP2 = 768;
			AVI_HeightP2 = 512;
		}
	} else {free(AVI_MJPEG_Offsets);AVI_MJPEG_Offsets=NULL;return 0;}

	sceJpegInitMJpeg();
	if(sceJpegCreateMJpeg(AVI_WidthP2,AVI_HeightP2)) {
		free(AVI_MJPEG_Offsets);
		AVI_MJPEG_Offsets=NULL;
		sceJpegFinishMJpeg();
		return -1;
	}

	return f;
}

int M3D_VIDEO_LoadMJPEG(const char *path){
	int a = AMG_Load_AVI((char*)path);
	return a;
}

void Seek_AVI(int frame){
	AVI_Frame = frame;
}

void AMG_Close_AVI(int video_handle){
	sceJpegDeleteMJpeg();
	sceIoClose(video_handle);
	AVI_AAC_STOP = 1;
	sceKernelWaitThreadEnd(AVI_AAC_thread, 0);
	sceKernelDeleteThread(AVI_AAC_thread);
	free(AVI_MJPEG_Offsets);
	AVI_MJPEG_Offsets = NULL;
	free(AVI_AAC_Offsets);
	AVI_AAC_Offsets = NULL;
	AVI_TotalFrames = 0;
	AVI_Frame = 0;
	AVI_MicroSecPerFrame = 0;
	AVI_Streams = 0;
	AVI_Width = 0;
	AVI_Height = 0;
}

void AMG_Play_FullScreen_AVI(char *path, int loop, u32 button){
	//int mode = 0;
	void *framebuffer;
	u8 *JPEG_pixels;
	SceCtrlData pad;
	int video_handle = AMG_Load_AVI(path);
	aac_handle = video_handle;
	if (!aac_handle) return;
	AVI_Loop = loop;
	AVI_AAC_File = video_handle;
	AVI_AAC_thread = sceKernelCreateThread("decode",decode_avi_AAC, 0x8, 0x10000, 0, 0);
	if (AVI_AAC_thread < 0) return;
	if (!video_handle) return;
	//if(AVI_MicroSecPerFrame == 16666)mode = 3;//60 fps
	//rewind video
	sceIoLseek32(video_handle,0,SEEK_SET);
	int read = 1;
	int First_frame = 1;
	AVI_AAC_STOP = 0;
	AVI_Last_Tick = 0;
	sceKernelStartThread(AVI_AAC_thread, 4, &AVI_p);
	JPEG_pixels = (u8*) calloc(4,AVI_Width*AVI_Height);
	sceGuDepthMask(1);//Disable Z buffer writes
	sceGuDisplay(0);
	if(!AMG_TV_State()){
		sceGuClearColor(0x00000000);
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
		sceGuDrawBuffer(GU_PSM_8888,(void*)0x88000,512);
		sceGuDispBuffer(480,272,(void*)0,512);
		framebuffer = (void*)0x88000;
	} else {
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
		sceGuDrawBuffer(GU_PSM_8888,(void*)0x168000,768);
		sceGuDispBuffer(720,480,(void*)0,768);
		framebuffer = (void*)0x168000;
	}
	sceGuDisplay(1);
	while(AVI_Frame < AVI_TotalFrames*2){
		sceRtcGetCurrentTick(&AVI_Current_Tick);
		if((AVI_Current_Tick-AVI_Last_Tick > AVI_MicroSecPerFrame) || First_frame){
			AVI_Last_Tick = AVI_Current_Tick;
			First_frame = 0;
			//M3D_updateScreen(0xffffffff);
			//sceGuFinish();
			//sceGuSync(0,0);
			
			//decode
			sceJpegDecodeMJpeg(MJPEG_frame,AVI_Width*AVI_Height,(void*)((u32)0x04000000+(u32)framebuffer),MJPEG_unknown);
			sceDisplayWaitVblankStart();
			framebuffer = sceGuSwapBuffers();
			//sceGuStart(GU_DIRECT,AVI_list);
			
			AVI_Frame+=2;
			
			read = 1;//allow reading the next frame
			//loop
			if (AVI_Loop & (AVI_Frame == AVI_TotalFrames*2)) AVI_Frame = 0;
			sceCtrlReadBufferPositive(&pad, 1);

			if (pad.Buttons & PSP_CTRL_START) {
				AMG_AVIMJPEG_eof = 1;
				sceKernelWaitThreadEnd(AVI_AAC_thread,0);
				//sceKernelTerminateDeleteThread(AVI_AAC_thread);
				AMG_Close_AVI(video_handle);
				M3D_updateScreen(0x00000000);
				free(JPEG_pixels); JPEG_pixels = NULL;
				sceGuDepthMask(0);
				//Reset buffers and font
				sceGuDrawBuffer(AMG.PSM,(void*)AMG.FB1,AMG.ScreenStride);
				sceGuDispBuffer(AMG.ScreenWidth,AMG.ScreenHeight,(void*)AMG.FB0,AMG.ScreenStride);
				//Avoid crashing by doing the above stuff again after loop exiting
				AVI_Frame = AVI_TotalFrames*2;
				AVI_Current_Tick = 0;
				AVI_Last_Tick = 0;
				read = -1;
				sceJpegFinishMJpeg();
			}
			sceKernelDelayThread(2000);
		}
		if (read == 1){
			//seek frame
			sceIoLseek32(video_handle,AVI_MJPEG_Offsets[AVI_Frame],SEEK_SET);
			sceIoRead(video_handle,MJPEG_frame,AVI_MJPEG_Offsets[AVI_Frame+1]);
			read = 2;//stop reading
		}
		//sceKernelDelayThread(1);
	}
	if(read != -1){ //You did not end the video by pressing a button
		AMG_AVIMJPEG_eof = 1;
		sceKernelWaitThreadEnd(AVI_AAC_thread,0);
		//sceKernelTerminateDeleteThread(AVI_AAC_thread);
		AMG_Close_AVI(video_handle);
		M3D_updateScreen(0x00000000);
		free(JPEG_pixels);JPEG_pixels = NULL;
		sceGuDepthMask(0);
		//Reset buffers and font
		sceGuDisplay(0);
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
		sceGuDrawBuffer(AMG.PSM,(void*)AMG.FB1,AMG.ScreenStride);
		sceGuDispBuffer(AMG.ScreenWidth,AMG.ScreenHeight,(void*)AMG.FB0,AMG.ScreenStride);
		sceJpegFinishMJpeg();
		sceGuDisplay(1);
	}

	AMG_InitFont();
}

void M3D_VIDEO_PlayFullScreenMJPEG(const char *path,int loop){
	AMG_Play_FullScreen_AVI((char*)path,loop,0);
}

int render_avi_totexture(SceSize args, void *argp){
	if (!AVI_Tex_Handle) return -1;
	//rewind video
	sceIoLseek32(AVI_Tex_Handle,0,SEEK_SET);
	//draw frames
	sceRtcGetCurrentTick(&AVI_Last_Tick);
	while(AVI_Frame < AVI_TotalFrames*2){
		sceRtcGetCurrentTick(&AVI_Current_Tick);
		if(AVI_Current_Tick-AVI_Last_Tick > AVI_MicroSecPerFrame){
			AVI_Last_Tick = AVI_Current_Tick;
			//seek frame
			sceIoLseek32(AVI_Tex_Handle,AVI_MJPEG_Offsets[AVI_Frame],SEEK_SET);
			sceIoRead(AVI_Tex_Handle,MJPEG_frame,AVI_MJPEG_Offsets[AVI_Frame+1]);
			//decode
			sceJpegDecodeMJpeg(MJPEG_frame,AVI_Width*AVI_Height,AVI_Tex_Target,MJPEG_unknown);
			AVI_Frame+=2;
			//loop
			if (AVI_Tex_Loop & (AVI_Frame == AVI_TotalFrames*2)) AVI_Frame = 0;
			if (AVI_Tex_STOP) return 0;
		}
		sceKernelDelayThread(1);
	}
	return 0;
}

void AMG_Play_AVI_ToTexture(int video_handle, int loop, AMG_Texture *tex){
	AVI_Tex_Handle = video_handle;
	AVI_Tex_Loop = loop;
	AVI_Tex_Target = tex->Data;
	AVI_decode_thread = sceKernelCreateThread("decode",render_avi_totexture, 0x8, 0x10000, 0, 0);
	if (AVI_decode_thread < 0) return;
	tex->isVideo = 1;
	AVI_Tex_Swizzle = tex->Swizzle;
	if (AVI_Tex_Swizzle) tex->Swizzle = 0;
	sceKernelStartThread(AVI_decode_thread, 4, &AVI_p);
}

void M3D_VIDEO_MJPEGToTexture_Start(int video_handle, int loop, M3D_Texture *tex){
	AMG_Texture *t = (AMG_Texture*)tex;
	AMG_Play_AVI_ToTexture(video_handle,loop,t);
}

void AMG_Stop_AVI_ToTexture(AMG_Texture *tex){
	if (!AVI_Tex_STOP){
		AVI_Tex_STOP = 1;
		sceKernelWaitThreadEnd(AVI_decode_thread, 0);
		sceKernelDeleteThread(AVI_decode_thread);
		tex->isVideo = 0;
		if (AVI_Tex_Swizzle) tex->Swizzle = 1;
		sceJpegFinishMJpeg();
	}
}

void M3D_VIDEO_MJPEGToTexture_Stop(M3D_Texture *tex){
	AMG_Texture *t = (AMG_Texture*)tex;
	AMG_Stop_AVI_ToTexture(t);
}


/////////////
///H264//////
/////////////


u32 AVI_Streams2 = 0;
u32 AVI_TotalFrames2 = 0;
u64 AVI_Last_Tick2 = 0;
u64 AVI_Current_Tick2 = 0;
u32 AVI_Frame2 = 0;
u32 AVI_AAC_Frame2 = 0;
u64 AVI_MicroSecPerFrame2 = 0;
u32 AVI_Loop2 = 0;
u32 AVI_Width2 = 0;
u32 AVI_Height2 = 0;
SceUID AVI_H264_thread;
int AVI_Audio_SemaID;
int AVI_Video_SemaID;
int AVI_DecodeVideo_SemaID;
int AVI_PlayVideo_SemaID;
int _AVI_FILE = 0;
int _AVI_FILE_VIDEO = 0;
u32 AVI_idx1_size2 = 0;
u32 AVI_start2 = 0;
u32 AVI_relative_offset2 = 0;

void *AVI_a;
void *AVI_b;

typedef struct{
	u32 name;
	u32 flags;
	u32 offset;
	u32 size;
	u32 name1;
	u32 flags1;
	u32 offset1;
	u32 size1;
} AVI_chunk;

AVI_chunk c;
AVI_chunk cv;

short pcm_output_buffer[32*2048] __attribute__((aligned(64)));
int audio_frames = 0;
int audio_frame = 0;
int play_frame = 0;
int audio_channel;
int audio_nframes = 1;
int audio_delay = 512;
//H264
//////
int AMG_Video_Interrupt_Number = 8;
struct SceMpegLLI{
	ScePVoid pSrc;
	ScePVoid pDst;
	ScePVoid Next;
	SceInt32 iSize;
};

struct avc_struct{
	int mpeg_init;
	ScePVoid mpeg_data;
	int      mpeg_ringbuffer_construct;
	int      mpeg_create;
	int      mpeg_format;
	int      mpeg_width;

	SceMpegRingbuffer  mpeg_ringbuffer;
	SceMpeg            mpeg;
	ScePVoid           mpeg_es;
	struct SceMpegLLI *mpeg_lli;
	SceMpegAu          mpeg_au;
};

struct avc_struct v_decode;
u32 a_offset;
u32 b_offset;
u32 c_offset;
SceInt64 result;

int AMG_video_H264Thread = 0;
SceFloat32 vsync_frame = 0.0f;
int current_video_frame = 0;
int current_play_frame = 0;
SceInt32 unused;
SceInt32 sceMpegbase_BEA18F91(struct SceMpegLLI *p);

u32 H264_Buffer_Frames		=	16;
u32 H264_max_frame_size		=	0xFFFF;	//For I frames, x4 for 720x480
u32 H264_buffer_size		=	0xFFFF	* 16;

void *H264_Decoded_Frame;
//We just need a buffer of raw h264 frames (and their sizes), each aligned to 64 bytes,
//then "sceMpegAvcDecode" will do everything for us
unsigned char *H264_RingBuffer;//= (void*)(0x4198000);
//I think this replaces "sceMpegGetAvcAu" from sony PMF (video) player samples
//It sets the frame offset inside the buffer to be decoded by "sceMpegAvcDecode"
static void CopyAu2Me(struct avc_struct *p, u8 *source_buffer, u32 size){
	u32 MEAVCBUF = 0x4a000;//MEDIA_ENGINE_AVC_BUFFER
	u32 DMABLOCK = 4095;
	u8 *destination_buffer = (u8 *) MEAVCBUF;
	unsigned int i = 0;
	while (1){
		p->mpeg_lli[i].pSrc = source_buffer;
		p->mpeg_lli[i].pDst = destination_buffer;
		if (size > DMABLOCK){
			p->mpeg_lli[i].iSize = DMABLOCK;
			p->mpeg_lli[i].Next  = &p->mpeg_lli[i + 1];
			source_buffer      += DMABLOCK;
			destination_buffer += DMABLOCK;
			size               -= DMABLOCK;
			i                  ++;
		} else {
			p->mpeg_lli[i].iSize = size;
			p->mpeg_lli[i].Next  = 0;
			break;
		}
	}
	sceKernelDcacheWritebackInvalidateAll();
	sceMpegbase_BEA18F91(p->mpeg_lli);
}

void h264_close(struct avc_struct *p){
	if (p->mpeg_lli != 0) free(p->mpeg_lli);
	if (p->mpeg_es != 0) sceMpegFreeAvcEsBuf(&p->mpeg, p->mpeg_es);
	if (p->mpeg_ringbuffer_construct == 0) sceMpegRingbufferDestruct(&p->mpeg_ringbuffer);
	if (p->mpeg_data != 0) free(p->mpeg_data);
	if (p->mpeg_create == 0) sceMpegDelete(&p->mpeg);
	if (p->mpeg_init == 0) sceMpegFinish();
}

int Set_H264_Decoder(struct avc_struct *p, int bufwidth, int format){
	int mod_64 = 0;
	u32 DMABLOCK = 4095;
	p->mpeg_init = -1;
	p->mpeg_data = 0;
	p->mpeg_ringbuffer_construct = -1;
	p->mpeg_create = -1;
	p->mpeg_es = 0;
	p->mpeg_lli = 0;
	p->mpeg_format = -1;
	p->mpeg_format = format;
	p->mpeg_width = bufwidth;
	p->mpeg_init = sceMpegInit();
	if (p->mpeg_init) return 0;
	int size = sceMpegQueryMemSize(0);
	
	//Malloc 64 aligned
	mod_64 = size & 0x3f;
	if (mod_64 != 0) size += 64 - mod_64;
	p->mpeg_data = memalign(64, size);
	
	p->mpeg_ringbuffer_construct = sceMpegRingbufferConstruct(&p->mpeg_ringbuffer, 0, 0, 0, 0, 0);
	p->mpeg_create = sceMpegCreate(&p->mpeg, p->mpeg_data, size, &p->mpeg_ringbuffer, p->mpeg_width, 0, 0);
	
	//Malloc 64 aligned
	H264_RingBuffer = (u8*)memalign(64,H264_buffer_size);
	
	H264_Decoded_Frame = (void*)memalign(64,512*272*4*8);
	
	SceMpegAvcMode avc_mode;
	avc_mode.iUnk0 = -1;
	avc_mode.iPixelFormat = p->mpeg_format;
	p->mpeg_format = avc_mode.iPixelFormat;
	p->mpeg_es = sceMpegMallocAvcEsBuf(&p->mpeg);
	unsigned int maximum_number_of_blocks = (4096 + DMABLOCK - 1) / DMABLOCK;
	
	//Malloc 64 aligned
	size = sizeof(struct SceMpegLLI) * maximum_number_of_blocks;
	mod_64 = size & 0x3f;
	if (mod_64 != 0) size += 64 - mod_64;
	p->mpeg_lli = (SceMpegLLI*) memalign(64, size);
	
	memset(&p->mpeg_au, -1, sizeof(SceMpegAu));
	p->mpeg_au.iEsBuffer = 1;
	return 1;
}

void update_video(){
	//u32 microsec_passed = 0;
	int delay = 0;
	u32 fps = (1000000.0f/(float)AVI_MicroSecPerFrame2);
	float rate = (float)fps/(float)60.0f;
	
	AVI_Current_Tick2 = vsync_frame;
	if((AVI_Current_Tick2 != AVI_Last_Tick2) || !current_play_frame){
		AVI_Last_Tick2 = AVI_Current_Tick2;
		u32 s0 = current_play_frame*AVI_MicroSecPerFrame2;
		u32 s1 = play_frame*1024*23;
		delay = (s1-s0)/4096000;
		audio_delay = delay;
		sceDisplaySetFrameBuf((void*)((u32)H264_Decoded_Frame+((current_play_frame&7)*(512*272*4))), 512,GU_PSM_8888,PSP_DISPLAY_SETBUF_IMMEDIATE);
		current_play_frame++;
	}
	vsync_frame +=rate;
	//pspDebugScreenSetXY(0,0);
	//pspDebugScreenPrintf("TEST %f",0.998f);
	sceKernelDelayThread(1000);
}

SceKernelThreadEntry decode_video(){
	//int loop_buffer1 = 0;
	while(AMG_video_H264Thread){
		sceKernelWaitSema(AVI_Video_SemaID, 1,0);
		sceKernelSignalSema(AVI_Video_SemaID, 0);
		void *destination_buffer = (void*)((u32)H264_Decoded_Frame+((current_video_frame&7)*(512*272*4)));//(void*)(0x04000000+framebuffer);
		CopyAu2Me(&v_decode,&H264_RingBuffer[a_offset],H264_max_frame_size);
		v_decode.mpeg_au.iAuSize = H264_max_frame_size;
		int result = sceMpegAvcDecode(&v_decode.mpeg,&v_decode.mpeg_au,v_decode.mpeg_width,&destination_buffer,&unused);
		if (result == 0){//It decoded a frame, show it
			sceKernelDcacheWritebackInvalidateAll();
			sceKernelSignalSema(AVI_DecodeVideo_SemaID, 1);		
			//framebuffer = sceGuSwapBuffers();
		} else {
			//pspDebugScreenPrintf("DECODE ERROR\n");
			//sceKernelDelayThread(10000*6);
			//sceKernelExitGame();
			sceKernelSignalSema(AVI_DecodeVideo_SemaID, 1);
		}
	}
	sceKernelExitThread(1);
	return 0;
}

int Load_Play_AVI_H264(const char *path,u32 button){
	int release = -1;
	int loop_buffer = 0;
	int loop_frames = H264_Buffer_Frames/2;
	u32 buffer_start_offset = H264_buffer_size/2;
	u32 offset2;
	a_offset = buffer_start_offset;
	b_offset = 0;
	
	u8 movi = 0;
	u8 idx1 = 0;
	u32 WB = 0x62773130;//aac chunk 01wb
	u32 DC = 0x63643030;//mjpeg frame 00dc
	u32 MOVI = 0x69766f6D;//movi chunk
	u32 INFO = 0x4F464E49;//info chunk
	u32 JUNK = 0x4B4E554A;//junk block
	u32 IDX = 0x31786469;//index block   

	u32 movi_offset = 0;
	u32 movi_size = 0;
	AVI_relative_offset2 = 0;
	u32 av1 = 0;u32 size = 0; u32 av3 = 0;
	u32 hdrl_size = 0;
	u32 avih_size = 0;
	
	_AVI_FILE = sceIoOpen(path, PSP_O_RDONLY, 0777 );
	_AVI_FILE_VIDEO = sceIoOpen(path, PSP_O_RDONLY, 0777 );

	if (_AVI_FILE<0) return 0;
	AVI_Frame2 = 0;
	AVI_AAC_Frame2 = 0;
	sceIoRead(_AVI_FILE,&av1,4);
	sceIoRead(_AVI_FILE,&size,4);
	sceIoRead(_AVI_FILE,&av3,4);
	if (av1 != 0x46464952) return 0;//RIFF
	if (av3 != 0x20495641) return 0;//AVI_
	sceIoLseek32(_AVI_FILE,0x04,SEEK_CUR);	
	sceIoRead(_AVI_FILE,&hdrl_size,4);
	movi_offset = hdrl_size+16+8+4;
	sceIoLseek32(_AVI_FILE,0x08,SEEK_CUR);
	sceIoRead(_AVI_FILE,&avih_size,4);
	if (avih_size != 0x38) return 0;//RIFF
	sceIoRead(_AVI_FILE,&AVI_MicroSecPerFrame2,4);
	sceIoLseek32(_AVI_FILE,12,SEEK_CUR);
	sceIoRead(_AVI_FILE,&AVI_TotalFrames2,4);
	sceIoLseek32(_AVI_FILE,4,SEEK_CUR);
	sceIoRead(_AVI_FILE,&AVI_Streams2,4);
	sceIoLseek32(_AVI_FILE,4,SEEK_CUR);
	sceIoRead(_AVI_FILE,&AVI_Width2,4);
	sceIoRead(_AVI_FILE,&AVI_Height2,4);
	
	//check size
	u32 buff_width = 512;
	if ((AVI_Width2!=480)&&( AVI_Width2!=720)) return 0;
	if ((AVI_Width2==480)&&(AVI_Height2!=272)) return 0;
	//if((AVI_Width2==720)&&(AVI_Height2!=480)) return 0;
	if (AVI_Width2==480) buff_width = 512;
	if (AVI_Width2==720) buff_width = 768;

	H264_max_frame_size	= 0xFFFF;
	H264_buffer_size = H264_max_frame_size * H264_Buffer_Frames;
//goto end_H264;
	sceGuDisplay(0);
	sceGuClearColor(0x00000000);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
	sceGuDrawBuffer(GU_PSM_8888,(void*) 0,512);
	sceGuDisplay(1);
	
	//look for movi
	offset2 = sceIoLseek32(_AVI_FILE,movi_offset,SEEK_SET);
	while (!movi){
		int b = sceIoRead(_AVI_FILE,&c.name,4);
		if (b == 0) {
			return 0;
		}
		if (c.name == MOVI) {
			movi = 1;
			movi_offset = sceIoLseek(_AVI_FILE,0, SEEK_CUR);
			break;
		}
		if (c.name == INFO) {
			sceIoLseek32(_AVI_FILE,offset2-4,SEEK_SET);
			sceIoRead(_AVI_FILE,&size,4);
			sceIoLseek32(_AVI_FILE,size,SEEK_CUR);
			offset2 = sceIoLseek(_AVI_FILE,0, SEEK_CUR);
		}
		if (c.name == JUNK) {
			sceIoRead(_AVI_FILE,&size,4);
			sceIoLseek32(_AVI_FILE,size+8,SEEK_CUR);
		}
	}
	
	//look for idx1
	sceIoLseek32(_AVI_FILE,movi_offset-8,SEEK_SET);
	sceIoRead(_AVI_FILE,&movi_size,4);
	sceIoLseek32(_AVI_FILE,movi_offset+movi_size-4,SEEK_SET);
	sceIoRead(_AVI_FILE,&c.name,4);
	if (c.name == IDX) idx1 = 1;
	if (idx1) {
		//go to idx1
		sceIoLseek32(_AVI_FILE,movi_offset+movi_size,SEEK_SET);
		sceIoRead(_AVI_FILE,&AVI_idx1_size2,4);
		//check if first offset is absolute or relative (to movi)
		sceIoLseek32(_AVI_FILE,8,SEEK_CUR);
		sceIoRead(_AVI_FILE,&c.offset,4);//read first offset
		if (c.offset == 4) AVI_relative_offset2 = movi_offset+4;
		//Go bak to read all chunks
		AVI_start2 = sceIoLseek32(_AVI_FILE,movi_offset+movi_size+4,SEEK_SET);
		//Set video decoder
		Set_H264_Decoder(&v_decode,buff_width, GU_PSM_8888);	
		//Set audio decoder
		if (!aac_getEDRAM) AMG_Set_aac_VIDEO_EDRAM();
		aac_codec_buffer_VIDEO[10] = 44100;
		if (sceAudiocodecInit(aac_codec_buffer_VIDEO, 0x1003) < 0) return 0;
		
		//Start video thread
		AVI_DecodeVideo_SemaID = sceKernelCreateSema("finished decoding video frame", 0, 0, 1, 0);
		if(!AVI_DecodeVideo_SemaID) {sceMpegFinish(); return 0;}
		AVI_Video_SemaID = sceKernelCreateSema("start decoding a video frame", 0, 0, 1, 0);
		if(!AVI_Video_SemaID) {sceMpegFinish(); return 0;}
		AVI_PlayVideo_SemaID = sceKernelCreateSema("start playing video frames", 0, 0, 1, 0);
		if(!AVI_PlayVideo_SemaID) {sceMpegFinish(); return 0;}
		
		AMG_video_H264Thread = 1;
		AVI_H264_thread = sceKernelCreateThread("decode video",(SceKernelThreadEntry)decode_video, 0x8, 0x10000, 0, 0);
		if (AVI_H264_thread < 0) {sceMpegFinish(); return 0;}
		sceKernelStartThread(AVI_H264_thread, 4, &AVI_a);
		sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT,AMG_Video_Interrupt_Number, (void*)(update_video),0);

		//Start main video loop
		u32 size = 0;
		u32 size2 = 0;
		sceDisplayWaitVblankStart();
		sceKernelSignalSema(AVI_PlayVideo_SemaID, 1);
		while(size != AVI_idx1_size2){
			int i=0;
			while (i!=audio_nframes){
				sceIoLseek32(_AVI_FILE,AVI_start2+size,SEEK_SET);
				sceIoRead(_AVI_FILE,&c,16);
				if (c.name == WB) {
					sceIoLseek32(_AVI_FILE,c.offset+AVI_relative_offset2, PSP_SEEK_SET);
					sceIoRead(_AVI_FILE,aac_data_buffer,c.size);
					//decode audio frame
					aac_codec_buffer_VIDEO[6] = (unsigned long)aac_data_buffer;
					aac_codec_buffer_VIDEO[7] = 1024;
					aac_codec_buffer_VIDEO[8] = (unsigned long)&pcm_output_buffer[(audio_frame&31)*aac_sample_per_frame];
					aac_codec_buffer_VIDEO[9] = 1024;
					sceAudiocodecDecode(aac_codec_buffer_VIDEO, 0x1003);
					audio_frame++;
					i++;
				}
				size+=16;
			}
			while(current_video_frame-current_play_frame < 2){
				sceIoLseek32(_AVI_FILE_VIDEO,AVI_start2+size2,SEEK_SET);
				sceIoRead(_AVI_FILE_VIDEO,&cv,16);
				if (cv.name == DC){
					if(!current_video_frame) {
						//Read the first frame
						sceIoLseek32(_AVI_FILE_VIDEO,cv.offset+AVI_relative_offset2,SEEK_SET);
						sceIoRead(_AVI_FILE_VIDEO,&H264_RingBuffer[a_offset],cv.size);
						memcpy(&H264_RingBuffer[b_offset],&H264_RingBuffer[a_offset],cv.size);
					} else {
						//Update buffer pointers for next frame
						a_offset += H264_max_frame_size;
						b_offset += H264_max_frame_size;
						loop_buffer++;
						if (loop_buffer == loop_frames) {
							loop_buffer = 0;
							a_offset = buffer_start_offset;
							b_offset = 0;
						}
						//Read next frame
						sceIoLseek32(_AVI_FILE_VIDEO,cv.offset+AVI_relative_offset2,SEEK_SET);
						sceIoRead(_AVI_FILE_VIDEO,&H264_RingBuffer[a_offset],cv.size);
						memcpy(&H264_RingBuffer[b_offset],&H264_RingBuffer[a_offset],cv.size);
						
						//Wait if Media engine is decoding the previous frame
						sceKernelWaitSema(AVI_DecodeVideo_SemaID, 1,0);
						if(current_video_frame == 1) sceKernelEnableSubIntr(PSP_VBLANK_INT,AMG_Video_Interrupt_Number);
					}
					
					//Enable decoding thread and continue reading and playing audio
					sceKernelSignalSema(AVI_DecodeVideo_SemaID, 0);
					sceKernelSignalSema(AVI_Video_SemaID, 1);//Tell ME to decode a frame
					current_video_frame++;
					//j++;
				}
				size2+=16;
			}

			if(current_play_frame > 3){
				sceAudioSRCChReserve( 1024,44100,2 );
				sceAudioSRCOutputBlocking(PSP_AUDIO_VOLUME_MAX,&pcm_output_buffer[(play_frame&31)*aac_sample_per_frame]);
				play_frame+=audio_nframes;
			}
			SceCtrlData pad;
			sceCtrlReadBufferPositive(&pad, 1);
			if (pad.Buttons & PSP_CTRL_START) break;

			sceKernelDelayThread(audio_delay);
		}
	} else { 
		return 0;
	}

	sceKernelDisableSubIntr(PSP_VBLANK_INT,AMG_Video_Interrupt_Number);
	sceKernelReleaseSubIntrHandler(PSP_VBLANK_INT,AMG_Video_Interrupt_Number);
	sceKernelTerminateDeleteThread(AVI_H264_thread);

	h264_close(&v_decode);
	sceIoClose(_AVI_FILE);

	while (release < 0) {
		release = sceAudioSRCChRelease();
		sceKernelDelayThread(1000);
		//pspDebugScreenPrintf("RELEASE SOUND %i\n",release);
	}
	
	sceGuDepthMask(0);
	//Reset buffers and font
	sceGuDisplay(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
	sceGuDrawBuffer(AMG.PSM,(void*)AMG.FB1,AMG.ScreenStride);
	sceGuDispBuffer(AMG.ScreenWidth,AMG.ScreenHeight,(void*)AMG.FB0,AMG.ScreenStride);
	sceGuDisplay(1);

	AMG_InitFont();
	
	return 1;
}

int M3D_VIDEO_PlayH264(const char *path){
	int a = Load_Play_AVI_H264(path,0);
	return a;
}



}


	

M3D_SOUND *M3D_LoadMP3(const char *path,int type){
	OSL_SOUND *s = oslLoadSoundFileMP3(path,type);
	return (M3D_SOUND*) s;
}

M3D_SOUND *M3D_LoadMOD(const char *path,int type){
	OSL_SOUND *s = oslLoadSoundFileMOD(path,type);
	return (M3D_SOUND*) s;
}

M3D_SOUND *M3D_LoadWAV(const char *path,int type){
	OSL_SOUND *s = oslLoadSoundFileWAV(path,type);
	return (M3D_SOUND*) s;
}

void M3D_SOUND_Loop(M3D_SOUND *sound, int loop){
	OSL_SOUND *s = (OSL_SOUND*) sound;
	if (s) oslSetSoundLoop(s,loop);
}

void M3D_SOUND_Play(M3D_SOUND *sound, int voice){
	OSL_SOUND *s = (OSL_SOUND*) sound;
	if (s) oslPlaySound(s,voice);
}

void M3D_SOUND_Stop(M3D_SOUND *sound){
	OSL_SOUND *s = (OSL_SOUND*) sound;
	if (s) oslStopSound(s);
}

void M3D_SOUND_Delete(M3D_SOUND *sound){
	OSL_SOUND *s = (OSL_SOUND*) sound;
	if (s) oslDeleteSound(s);
}

int M3D_SOUND_Playing(M3D_SOUND *sound){
	OSL_SOUND *s = (OSL_SOUND*) sound;
	return oslGetSoundChannel(s);
}












