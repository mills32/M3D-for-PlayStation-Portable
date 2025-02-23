#ifndef _AMG_TEXTURE_H_
#define _AMG_TEXTURE_H_

#define M3D_EFFECT_H_WAVE 0
#define M3D_EFFECT_V_WAVE 1

#ifdef __cplusplus
	extern "C" {
#endif

// Includes
#include <wchar.h>
#include <psptypes.h>
#include <stdarg.h>
#include <pspgu.h>
#include <oslib/oslib.h>
#include "M3D.h"
#ifdef AMG_DOC_ENGLISH
/**
 * @file AMG_Texture.h
 * @brief Functions related to texture loading, and 2D sprite management
 * @author Andrés Martínez (Andresmargar)
 */
#else
/**
 * @file AMG_Texture.h
 * @brief Funciones encargadas de la carga de texturas, y manejo de sprites 2D
 * @author Andrés Martínez (Andresmargar)
 */
#endif

extern int AMG_MipMapping;
extern float AMG_MipMapping_Bias;
/******************************************************/
/************** TEXTURAS ******************************/
/******************************************************/

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct that holds a texture
 */
#else
/**
 * @struct
 * @brief Estructura que almacena una textura
 */
#endif
typedef struct{
	u32 *Data;			//*< Datos de píxeles de la textura (1 elemento = 1 pixel RGBA) / Pixel data (1 element = 1 RGBA pixel) */
	u32 pad0[3];		//*< Padding to keep align 16
	u32 *Data1;			//*< For images bigger than 512x512
	u32 pad1[3];		//*< Padding to keep align 16
	u32 *MipData;		//*< Mipmap1
	u32 pad2[3];
	u32 *MipData1;		//*< Mipmap2
	u32 pad3[3];
	u32 *clut;			//*< Palette for 4/8 bit modes
	u32 pad4[3];
	u32 clut_anim[256];	//*< Palette animation
	u32 palcounter[32];	//Palette animation
	u32 palframe[32];	//Palette animation
	u8 Anim[64];		//Custom animated texture
	float ScaleX;		//*< Escala del Sprite / Sprite Scale
	u32 pad5[3];
	float ScaleY;		//*< Escala del Sprite / Sprite Scale				
	u32 pad6[3];
	float Rot;			//*< Rotacion del Sprite / Sprite Rotation
	u32 pad7[3];
	u8 Load;			/**< AMG_TEX_RAM o AMG_TEX_VRAM, no modificar / AMG_TEX_RAM or AMG_TEX_VRAM, don't write */
	u8 pad8[15];
	u32 Width;			/**< Ancho de la textura en píxeles / Texture width in pixels */
	u32 RWidth;			//512-Image width (second part of a huge image), Or actual width of small image (< 512)
	u32 Height;			/**< Alto de la textura en píxeles / Texture height in pixels */
	u32 RHeight;
	u32 TexFormat;		/**< Formato de la textura, no modificar / Texture Format, don't write*/
	u32 TFX;			/**< GU_TFX_MODULATE, GU_TFX_DECAL etc */
	u32 TCC;			/**< No modificar / Don't write */
	u32 EnvColor;		/**< Color de la textura si TFX != GU_TFX_DECAL / Texture color if TFX != GU_TFX_DECAL */
	u32 Filter;			/**< (GU_NEAREST, GU_LINEAR)*/
	u32 MipFilter;		/**< (GU_LINEAR_MIPMAP_NEAREST, GU_LINEAR_MIPMAP_LINEAR)*/
	u8 Swizzle;
	u8 NMipmaps;		/**< Número de Mipmaps / Number of Mipmaps */
	u32 M1_Width;
	u32 M1_Height;	
	u32 M2_Width;
	u32 M2_Height;
	u8 NFrames;			/**< Número de frames de la textura, no modificar / Number of frames, don't write */
	u32 Next_pow2;		//Next power of two for the second part of a huge image
	float Frame;		/**< Frame a mostrar de la textura / Current frame of the texture*/
	float Speed;		//Animation speed
	u8 Tile_count_x;	//X tiles for animation
	u8 Tile_count_y; 	//Y tiles for animation
	float U;			/**< Coordenada U de traslación / U translation*/
	float V;			/**< Coordenada V de traslación / V translation */
	s16 X;			/**< Coordenada X del sprite / Sprite X */
	s16 Y;			/**< Coordenada Y del sprite / Sprite Y */
	u32 WrapX, WrapY;	/**< Valor de repetición de la textura / Wrapping value */
	u32 Mapping;		/**< Mapeo de la textura (normal o environment mapping) / Texture mapping (normal or environment mapping) */
	u8 MappingLights[2];/**< Las luces que harán de columnas de la matriz en environment mapping / Light which will work as env-map matrix's columns */
	float EnvMapRot, EnvMapX, EnvMapY;	/**< Parámetros del Environment Map / Env-Map parameters */
	u32 SprColor;		/**< El color del Sprite / Sprite color */			/**< Rotación del Sprite / Sprite rotation */
	u16 isVideo;
	u16 DemoEffect_PlasmaFrame; /**< Frame for plasma or twister effects*/
	// USO INTERNO, NO MODIFICAR
	u32 rw, rh;
}__attribute__((aligned(16))) AMG_Texture;


void AMG_InitFont();	//Internal use

// Defines
#define AMG_TEX_RAM 0
#define AMG_TEX_VRAM 1
#define AMG_RGBA5650(r, g, b)		(((b) << 11) | ((g) << 5) | (r))
#define AMG_RGBA4444(r, g, b, a)	(((a) << 12) | ((b) << 8) | ((g) << 4) | (r))
#define AMG_RGBA5551(r, g, b, a)	(((a) << 15) | ((b) << 10) | ((g) << 5) | (r))

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Enables a texture
 * @param tex Texture to enable
 * @return It returns nothing
 */
#else
/**
 * @brief Activa una textura
 * @param tex La textura a activar
 * @return No devuelve nada
 */
#endif
void AMG_EnableTexture(AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Allocates a VRAM block for shared textures
 * @param w Maximum texture width
 * @param h Maximum texture height
 * @param psm Maximum pixelformat of shared textures
 * @return It returns nothing
 * @note Due to RAM textures are slow to show, and not always there is enough VRAM, you can create
 * @note a reserved block of VRAM to transfer via GE in real time, every RAM texture you want, increasing speed
 */
#else
/**
 * @brief Reserva un espacio de VRAM para texturas compartidas
 * @param w El ancho de la textura (como máximo)
 * @param h El alto de la textura (como máximo)
 * @param psm La calidad máxima de las texturas compartidas
 * @return No devuelve nada
 * @note Debido a que las texturas en RAM son lentas de mostrar, y no siempre hay espacio en VRAM, puedes crear 
 * @note un espacio reservado en VRAM para transferir en tiempo real vía GE, todas las texturas de RAM que desees, aumentando la velocidad
 */
#endif
void AMG_AllocateSharedTexture(u32 w, u32 h, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Deletes the reserved block of VRAM for shared textures
 * @return It returns nothing
 */
#else
/**
 * @brief Elimina el espacio reservado para texturas compartidas
 * @return No devuelve nada
 */
#endif
void AMG_FreeSharedTexture(void);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a texture
 * @param path Texture file path
 * @param load AMG_TEX_RAM or AMG_TEX_VRAM
 * @param psm image quality
 * @return Pointer where the texture is loaded
 */
#else
/**
 * @brief Carga una textura
 * @param path Ruta donde está la textura
 * @param load AMG_TEX_RAM o AMG_TEX_VRAM
 * @param psm calidad de la imagen
 * @return Puntero a la textura donde se carga
 */
#endif
AMG_Texture *AMG_LoadTexture(const char *path, u8 load, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Changes texture pixelformat
 * @param tex Texture to convert
 * @param psm GU_PSM_4444, GU_PSM_5650 or GU_PSM_5551
 * @return It returns nothing
 * @note Texture quality MUST be GU_PSM_8888
 * @note To transfer a texture to VRAM, use AMG_TransferTextureVram();
 */
#else
/**
 * @brief Convierte el formato de una textura
 * @param tex La textura a convertir
 * @param psm El PSM: GU_PSM_4444, GU_PSM_5650 o GU_PSM_5551
 * @return No devuelve nada
 * @note Su calidad debe ser GU_PSM_8888
 * @note Para pasar la textura convertida a VRAM, usar la función AMG_TransferTextureVram();
 */
#endif
void AMG_ConvertTexture(AMG_Texture *tex, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Transfers a texture from RAM to VRAM
 * @param tex Texture to transfer
 * @return It returns nothing
 */
#else
/**
 * @brief Transfiere una textura en RAM a VRAM
 * @param tex La textura a transferir
 * @return No devuelve nada
 */
#endif
void AMG_TransferTextureVram(AMG_Texture *tex);
void AMG_TransferTextureRam(AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Deletes a texture
 * @param tex Texture to delete
 * @return It returns nothing
 */
#else
/**
 * @brief Elimina una textura
 * @param tex La textura a eliminar
 * @return No devuelve nada
 */
#endif
void M3D_UnloadTexture(M3D_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Load images bigger than 512x512, only up to 768x512
 * @param filename image to load
 * @return An AMG_Texture pointer
 */
#else
/**
 * @brief Carga imagenes mayores a 512x512, solo hasta 768x512
 * @param filename La imagen que se carga
 * @return Un puntero a una imagen AMG_Texture
 */
#endif
M3D_Texture *M3D_LoadHugeImage(const char *filename);

void M3D_DrawHugeImage(M3D_Texture *image, s16 x, s16 y);



#ifdef AMG_DOC_ENGLISH
/**
 * @brief Prints text in screen
 * @param tex Texture to enable
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Text color (GU_RGBA(r, g, b, a) macro)
 * @param text Text to write
 * @return It returns nothing
 */
#else
/**
 * @brief Imprime texto en pantalla
 * @param tex La textura a activar
 * @param x Coordenada X donde comienza
 * @param y Coordenada Y donde comienza
 * @param color El color del texto (macro GU_RGBA(r, g, b, a))
 * @param text El texto a escribir
 * @return No devuelve nada
 */
#endif
void AMG_Print(AMG_Texture *tex, int x, int y, u32 color, int wave_amp, int wave_speed, float wave_val, char *text);

#define AMG_Printf(tex, x, y, color, wave_amp, wave_speed, wave_val, ...){\
	char __str[1000];\
	sprintf(__str , __VA_ARGS__);\
	AMG_Print(tex, x, y, color, wave_amp, wave_speed, wave_val,  __str);\
}

#ifndef AMG_COMPILE_ONELUA
#ifdef AMG_DOC_ENGLISH
/**
 * @brief Draws a 2D Sprite (using texture-cache)
 * @param tex Texture which works as a sprite
 * @return It returns nothing
 * @note Important: Convert the texture to sprite using AMG_Create2dObject(), this version does not allow rotation/scaling
 */
#else
/**
 * @brief Dibuja un Sprite 2D (optimizando el uso del caché de texturas)
 * @param tex La textura que hace de sprite
 * @return No devuelve nada
 * @note Importante convertir la textura en sprite con la función AMG_Create2dObject(), esta versión no soporta rotación/escalado
 */
#endif
void AMG_DrawSpriteCache(AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Converts a texture to a 2D object
 * @param tex Texture to convert
 * @param psm If you want to change pixelformat to 16 bits (more FPS)
 * @param vram If you want to transfer the texture to VRAM (more FPS)
 * @return It returns nothing
 */
#else
/**
 * @brief Convierte una textura en un objeto 2D
 * @param tex La textura a convertir
 * @param psm Si quieres convertir la imagen a un formato de menor calidad (más FPS)
 * @param vram Si quieres pasar la textura a VRAM (siempre se recomienda para no perder FPS)
 * @return No devuelve nada
 */
#endif
void AMG_Create2dObject(AMG_Texture *tex, u32 psm, u8 vram);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief "Deletes" a color in a texture (makes it transparent)
 * @param tex Texture to modify
 * @param color Color to delete
 * @return It returns nothing
 */
#else
/**
 * @brief "Borra" un color de una textura (lo hace transparente)
 * @param tex La textura a modificar
 * @param color El color a transparentar
 * @return No devuelve nada
 */
#endif
void AMG_DeleteColor(AMG_Texture *tex, u32 color);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Takes a screenshot in PNG format
 * @param path Where the image will be saved
 * @return It returns nothing
 */
#else
/**
 * @brief Toma una captura de pantalla en formato PNG
 * @param path La ruta donde se guardará la captura
 * @return No devuelve nada
 */
#endif
void AMG_Screenshot(const char *path);

#endif

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Swizzles a texture (optimization for the GE)
 * @param tex Texture to swizzle
 * @return It returns nothing
 * @note When a texture is not going to be modified in real time, it's highly recommended to optimize it
 */
#else
/**
 * @brief Haz Swizzle a una textura (optimiza la textura para el GE)
 * @param tex La textura a optimizar
 * @return No devuelve nada
 * @note Siempre que la textura no se vaya a modificar en tiempo real, es altamente recomendable optimizarla
 */
#endif
void AMG_SwizzleTexture(AMG_Texture *tex);
void AMG_UnswizzleTexture(AMG_Texture *tex);
#ifdef AMG_DOC_ENGLISH
/**
 * @brief Disables textures
 * @return It returns nothing
 * @note This function only calls to sceGuDisable(GU_TEXTURE_2D);
 * @see <pspgu.h>
 */
#else
/**
 * @brief Desactiva las texturas
 * @return No devuelve nada
 * @note Esta función solo llama a sceGuDisable(GU_TEXTURE_2D);
 * @see <pspgu.h>
 */
#endif
static inline void AMG_DisableTexture(void){
	sceGuDisable(GU_TEXTURE_2D);
}

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Set texture animation
 * @param tex Texture
 * @param xframes Number of x frames
 * @param yframes Number of y frames
 * @param speed animation speed
 * @return It returns nothing
 * @note Only for textures used on 3D models, (not for sprites)
 */
#else
/**
 * @brief Configura una animacion de textura
 * @param tex La textura
 * @param xframes El número de frames x
 * @param xframes El número de frames y
 * @param speed velocidad de animacion
 * @return No devuelve nada
 * @note Solamente para texturas que utilian los modelos 3D, (no para sprites)
 */
#endif
void AMG_Texture3D_Animate(AMG_Texture *tex, u8 xframes, u8 yframes, u8 *anim, float speed);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Set the texture mapping
 * @param tex Texture
 * @param mapping Mapping (GU_ENVIRONMENT_MAP, GU_TEXTURE_COORDS...)
 * @param l0 Light to use (these don't work as lights, only as columns of the mapping matrix)
 * @param l1 Second light to use (that matrix is 2x3)
 */
#else
/**
 * @brief Establece el mapping de una textura
 * @param tex La textura
 * @param mapping El tipo de mapeado (GU_ENVIRONMENT_MAP, GU_TEXTURE_COORDS...)
 * @param l0 La luz a usar (no funcionan como luces, sino como columnas de la matriz de mapeado)
 * @param l1 La segunda luz a usar (dicha matriz es de 2x3)
 */
#endif
void AMG_SetTextureMapping(AMG_Texture *tex, u32 mapping, u8 l0, u8 l1);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Creates an empty texture
 * @param width Texture width
 * @param height Texture height
 * @param psm Texture pixelformat
 * @param load Where it's located (RAM or VRAM)
 * @return The created texture
 */
#else
/**
 * @brief Crea una textura vacía
 * @param width El ancho de la textura
 * @param height El alto de la textura
 * @param psm La calidad de imagen
 * @param load Dónde está la imagen (RAM o VRAM)
 * @return La textura
 */
#endif
AMG_Texture *AMG_CreateTexture(u16 width, u16 height, u32 psm, u8 load);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Creates a texture to be used for rendering (example a TV or screen)
 * @param width Texture width
 * @param height Texture height
 * @return The created texture
 * @note PSM is always 5650 and it is always uploaded to VRAM
 */
#else
/**
 * @brief Crea una textura para renderizar (Por ejemplo una pantalla o TV)
 * @param width El ancho de la textura
 * @param height El alto de la textura
 * @return La textura
 * @note PSM es siempre 5650 y siempre se sube a VRAM
 */
#endif
AMG_Texture *AMG_CreateRenderTexture(u16 width, u16 height);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Enables "Render to Texture"
 * @param tex Texture where rendering is performed
 * @return It returns nothing
 */
#else
/**
 * @brief Activa el "Render to Texture"
 * @param tex La textura donde se va a renderizar
 * @return No devuelve nada
 */
#endif
void AMG_EnableRenderToTexture(AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Disables "Render to Texture"
 * @return It returns nothing
 */
#else
/**
 * @brief Desactiva el "Render to Texture"
 * @return No devuelve nada
 */
#endif
void AMG_DisableRenderToTexture(void);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Changes a texture pixel
 * @param tex Texture
 * @param x X coordinate
 * @param y Y coordinate
 * @param color New color
 * @return It returns nothing
 */
#else
/**
 * @brief Cambia el pixel de una textura
 * @param tex La textura
 * @param x Coordenada X
 * @param y Coordenada Y
 * @param color El nuevo color
 * @return No devuelve nada
 */
#endif
void AMG_ChangeTexturePixel(AMG_Texture *tex, u32 x, u32 y, u32 color);

void AMG_SetTextureFilter(AMG_Texture *tex, int filter);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Get a texture pixel
 * @param tex Texture
 * @param x X coordinate
 * @param y Y coordinate
 * @return Color at this pixel
 */
#else
/**
 * @brief Obtén el pixel de una textura
 * @param tex La textura
 * @param x Coordenada X
 * @param y Coordenada Y
 * @return El color obtenido
 */
#endif
u32 AMG_GetTexturePixel(AMG_Texture *tex, u32 x, u32 y);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads an image in RAM for hardware memcpy
 * @param path path to file
 * @return a loaded AMG_Texture
 * @note Loads only PNG images
 */
#else
/**
 * @brief Carga una imagen en RAM para memcpy por hardware
 * @param path Ruta al archivo
 * @return La imagen cargada AMG_Texture
 * @note Sólamente carga imágenes PNG
 */
#endif
M3D_Texture *M3D_LoadRawImage(const char *path);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Non GPU related hardware memcpy (using sceGuCopyImage) VERY fast
 * @param tex an AMG_Texture
 * @return It returns nothing
 * @note OSL_IMAGE must be in RAM, unswizzled and pixel frmat must be the same used by M3D_Init 
 */
#else
/**
 * @brief "memcpy" por hardware sin GPU (utiliza sceGuCopyImage) Muy rapido
 * @param tex Una imagen AMG_Texture
 * @return No devuelve nada
 * @note OSL_IMAGE debe estar en RAM, "unswizzled" y en el mismo formato que M3D_Init
 */
#endif
void M3D_DrawImage(M3D_Texture *tex, int x, int y);


void AMG_UnloadTexture(AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a TMX format map (and tiles) into an OSLib map 
 * @param path TMX file
 * @param load load tiles to RAM or VRAM (OSL_IN_RAM or OSL_IN_VRAM
 * @param psm tiles image mode (OSL_PF_5650, OSL_PF_5551, OSL_PF_4444, OSL_PF_8888)
 * @return It returns nothing
 * @note Loads maps in TMX format, created witg TILED. Version "1.10.1" CSV map format.
 */
#else
/**
 * @brief Carga un mapa en formato TMX (y los tiles) a un mapa de OSLib 
 * @param path archivo TMX
 * @param load cargar tiles a RAM o VRAM (OSL_IN_RAM or OSL_IN_VRAM
 * @param psm modo de imagen de tiles (OSL_PF_5650, OSL_PF_5551, OSL_PF_4444, OSL_PF_8888)
 * @return No devuelve nada
 * @note Carga mapas formato TMX creados con TILED version "1.10.1" formato de mapa CSV.
 */
#endif
M3D_MAP *M3D_LoadMapTMX(const char *path, u8 mode, u8 load, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Animates one tile in an OSLib map
 * @param slot animation slot, from 1 to 16 different animations at the same time
 * @param tile number of the tile to animate
 * @param start_tile first tile used for the animation.
 * @param anim_size number of tiles after the start_tile that will be displayed one after another
 * @param speed animation speed
 * @return It returns nothing
 * @note Be careful not to include the original tile inside the animation.
 */
#else
/**
 * @brief Animaciones the un tile en un mapa OSLib
 * @param slot número de animación, de 1 a 16 animaciones diferentes a la vez.
 * @param tile número de tile que será animado
 * @param start_tile primer tile usado en la animación
 * @param anim_size número de tiles despues del primero que seran mostrados uno tras otro
 * @param speed Velocidad de animación
 * @return No devuelve nada.
 * @note Asegurate de no incluir el tile original en la animación.
*/
#endif
void M3D_Animate_MapTiles(M3D_MAP *m, int slot, int tile, int start_tile, u8 anim_size, float speed);

void M3D_UnloadMap(M3D_MAP *map);

//PALETTES

u32 AMG_BlendColor(u32 col_src,u32 col_dst,float blend);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Animates a palette from an AMG_Texture, OSL_IMAGE or an OSL_MAP image
 * @param type 0 = AMG_Texture; 1 = OSL_IMAGE; 2 = OSL_MAP
 * @param image Pointer to AMG_Texture, OSL_IMAGE or OSL_MAP
 * @param setpalettes Pointer to array defining animations.
 * @param blend Blend colors to smooth animation, 0 = off, 1 = on. Only AMG Textures support this.
 * @return It returns nothing
 * @note The first value of the setpalettes array, defines the number of animations, the rest 
	are the animations themselves, each defined by 4 values:
	invert(0,1),rate,start_color,number_of_colors
 */
#else
/**
 * @brief Animacion de paleta de colores de AMG_Texture, OSL_IMAGE o una imagen de OSL_MAP
 * @param type 0 = AMG_Texture; 1 = OSL_IMAGE; 2 = OSL_MAP
 * @param image Puntero a AMG_Texture, OSL_IMAGE o OSL_MAP
 * @param setpalettes Puntero a tabla de definicion de animaciones.
 * @param blend Transicion suave de colores, 0 = no, 1 si. solo para AMG_Texture
 * @return No devuelve nada.
 * @note El primer valor de la tabla setpalettes define el numero de animaciones, el resto 
	son las propias animaciones, definidas por 4 valores cada una: 
	invertido(0,1),velocidad,color_inicial,numero_de_colores
 */
#endif

void M3D_Cycle_Palettes(int type, void *image, u8 *setpalettes, u8 blend);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a font from an image and deletes the existing one from RAM or VRAM
 * @param slot 1 or 2 (font 1 or font 2)
 * @param path Path to file
 * @param load where to store the font AMG_TEX_RAM or AMG_TEX_VRAM
 * @param psm Image mode (GU_PSM_5650, GU_PSM_5551, GU_PSM_4444, GU_PSM_8888)
 * @return It returns nothing
 * @note Images must be power of two squares (128x128, 256x256)
 */
#else
/**
 * @brief Carga una fuente a partir de una imagen, y elimina la existente de RAM o VRAM
 * @param slot 1 o 2 (fuente 1 o fuente 2)
 * @param path Ruta al archivo
 * @param load Donde cargar la fuente, AMG_TEX_RAM o AMG_TEX_VRAM
 * @param psm Modo de imagen (GU_PSM_5650, GU_PSM_5551, GU_PSM_4444, GU_PSM_8888)
 * @return No devuelve nada
 * @note Las imágenes deben ser cuadrados y potencias de 2 (128x128, 256x256)
 */
#endif
void AMG_LoadFont(int slot, char *path, u8 load, u32 psm);


M3D_Texture *M3D_GetFont(int n);


//DEMO EFFECTS

void M3D_DrawImageWave(M3D_Texture *tex, int x, int y, float wave_val, u32 mode, s16 amp, s16 len);

void M3D_DrawRotoZoom(M3D_Texture *tex, u32 alpha, int tx, int ty, float rot, float scale);

M3D_Texture *M3D_CreatePlasmaTexture(const char *path);

void M3D_UpdateTexturePlasma(M3D_Texture *tex,int px,int py,float scale,int speed);

void M3D_Set2DPlasma(const char *path);

void M3D_Draw2DPlasma(u8 alpha, int px,int py,float scale,float speed);

void M3D_DrawCopperBars(u8 number,u8 size,u16 center,u16 amplitude,u8 alpha,float speed);

#ifdef __cplusplus
	}
#endif

#endif
