#ifndef _AMG_MODEL_H_
#define _AMG_MODEL_H_

#ifdef __cplusplus
	extern "C" {
#endif

// Includes
#include <psptypes.h>
#include "AMG_Texture.h"
#include "AMG_3D.h"
#ifdef AMG_DOC_ENGLISH
/**
 * @file AMG_Model.h
 * @brief Functions to load and render 3D models
 * @author Andrés Martínez (Andresmargar)
 */
#else
/**
 * @file AMG_Model.h
 * @brief Funciones encargadas de la carga y el renderizado de modelos 3D
 * @author Andrés Martínez (Andresmargar)
 */
#endif
 
/******************************************************/
/************** ANIMACIONES ***************************/
/******************************************************/

// Defines
#define AMG_MATRIX_FORMAT_DIRECTX	0
#define AMG_MATRIX_FORMAT_OPENGL	1
#define AMG_FRAME_DATAFORMAT_MATRIX 	(1 << 0)
#define AMG_FRAME_DATAFORMAT_POSITION 	(1 << 1)
#define AMG_FRAME_DATAFORMAT_ROTATION	(1 << 2)
#define AMG_FRAME_DATAFORMAT_SCALE		(1 << 3)
#define AMG_FRAME_DATAFORMAT_DIFFUSE	(1 << 4)

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct which packs a rgb color
 */
#else
/**
 * @struct
 * @brief Estructura que empaqueta un color rgb
 */
#endif
typedef struct Color_RGB
{
	u8 r;
	u8 g;
	u8 b;
}Color_RGB;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct which packs a rgba color
 */
#else
/**
 * @struct
 * @brief Estructura que empaqueta un color rgba
 */
#endif
typedef struct Color_RGBA
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
}Color_RGBA;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Animation frame structure
 */
#else
/**
 * @struct
 * @brief Estructura a un frame de la animación
 */
#endif
typedef struct{
	u32 DataFormat;	/**< Formato de los datos */
	void *Data;		/**< Datos del frame */
}AMG_AnimationFrame;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct to a single animation
 */
#else
/**
 * @struct
 * @brief Estructura a una sola animación
 */
#endif
typedef struct{
	char *Name;		/**< Nombre de la animación */
	u32 NFrames;	/**< Número de frames de la animación */
	u8 MatrixFormat;/**< Formato de las matrices (DirectX u OpenGL) */
	AMG_AnimationFrame *Frame;	/**< Puntero a las animaciones */
}AMG_AnimationChunk;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct of a single or multiple animation
 */
#else
/**
 * @struct
 * @brief Estructura de una animación o varias animaciones
 */
#endif
typedef struct{
	u8 NAnimations;					/**< Número de animaciones */
	AMG_AnimationChunk *Animation;	/**< Puntero a las animaciones */
}AMG_Animation;

/******************************************************/
/************** MODELOS 3D ****************************/
/******************************************************/

typedef struct{
	int v[3], t[3], n[3];
}AMG_FaceOBJ;


typedef struct {
	unsigned char skinWeight0[2];
	float u0, v0;
	signed char nx0,ny0,nz0;
	float x0,y0,z0;
	unsigned char skinWeight1[2];
	float u1, v1;
	signed char nx1,ny1,nz1;
	float x1,y1,z1;
	unsigned char skinWeight2[2];
	float u2, v2;
	signed char nx2,ny2,nz2;
	float x2,y2,z2;
}AMG_Face_W2TNV;


typedef struct{
	int group_state;
	int min_bone;
	int max_bone;
	int nfaces;
	int vertex_offset;
	AMG_Vertex_W2TNV *data;
}Face_Group;


#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Structure which hold a material group
 */
#else
/**
 * @struct
 * @brief Estructura de un grupo de materiales
 */
#endif
typedef struct{
	u32 Emmision, Ambient, Specular, Diffuse;	/**< Valores de material / Material Components */
	AMG_Texture *Texture;						/**< La textura del objeto, si no tiene esto es NULL / Object texture (NULL if it does not exist) */
	AMG_Texture *MultiTexture;					/**< Textura creada para hacer  Multitextura / Second texture to combine with the first one */
	ScePspFVector4 *NormalMap;					/**< vectores de normales / Normal vectors */
	u32 Start, End;								/**< Las caras que se van a dibujar (número de triángulos) / Faces to draw (triangle number) */
	// INTERNO
	char *mtlname;								/**< Uso interno del sistema, no modificar / Internal usage */
	u8 sel;										/**< Uso interno del sistema, no modificar / Internal usage */
}AMG_ObjectGroup;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct which holds a 3D object (included in AMG_Model struct)
 */
#else
/**
 * @struct
 * @brief Estructura de un objeto 3D (incluido en AMG_Model)
 */
#endif
typedef struct{
	u32 bullet_id;
	u32 material_name[2] = {0,0};
	u8 Collision;								/**< Si el objeto está en colisión con otro / If this object collides with another */
	u32 CollidedWith;							/**< El objeto con el que colisiona nuestro cuerpo / Object which collides with this one */
	u8 collisionreset;
	ScePspFVector3 __attribute__((aligned(16))) Pos;	/**< La posición en el espacio del objeto / Object position*/
	ScePspFVector3 __attribute__((aligned(16))) Rot;	/**< La rotación en el espacio del objeto (Euler XYZ) / Object rotation (Euler XYZ)*/
	ScePspFVector3 __attribute__((aligned(16))) Scale;	/**< La escala del objeto 3D / Object scale*/
	ScePspFVector3 __attribute__((aligned(16))) Origin;
	ScePspQuatMatrix __attribute__((aligned(16))) VehicleRotation;	/**< Physics vehiche rot*/
	u32 NFaces;									/**< Número de caras de este objeto / Number of faces */
	u16 NGroups;								/**< Número de grupos de este objeto (tanto el comando "g" como el "usemtl") / Number of groups (commands "g" and "usemtl") */
	AMG_ObjectGroup *Group;						/**< Todos los materiales usados por este objeto / List of material groups */
	void *Data;									/**< Los datos de vértices del objeto (no escribir) / Vertex data (don't write) */
	void *Triangles;							/**< Triangulos para bullet / Triangles for bullet*/				
	AMG_Vertex_V *OutLine;                      /**< Borde del objeto */
	u32 Flags;									/**< Los flags de renderizado (no escribir) / Rendering flags (don't write) */
	u8 TriangleSize;							/**< Tamaño de un triángulo / Triangle size in bytes (don't write) */		
	ScePspFVector3 *BBox;						/**< Bounding Box */
	ScePspFVector3 *tBBox;						/**< Bounding Box transformada / transformed Bounding Box*/
	ScePspFVector3 Centre, tCentre;				/**< Centro del objeto 3D (transformado) */
	float CelShadingScale;						/**< Tamaño del contorno en Cel-Shading (1.0 - 1.5) / Outline size for Cel-Shading (1.0 - 1.5) */
	u32 OutlineColor;							/**< Color del contorno del Cel-Shading / Outline color for Cel-Shading */
	u8 DrawBehind;								/**< Draw object Silhouette if behind others? */
	u8 Lighting;								/**< Iluminamos el objeto o no? / Object affected by illumination? */
	// Motor fisico BULLET
	float Mass;									/**< Masa del objeto / Object mass */
	u8 Physics;
	u8 isGround;								/**< true/false si el objeto es suelo o no / Is this object a floor? */
	u32 ShapeType;								/**< El tipo de objeto que es: una caja, una esfera... / Kind of object (box, sphere...) */
	float	vmax;								// Maximum speed limit
	//drive parameters if the object is a vehicle
	float	Steering;
	float	SteeringInc;
	float	SteeringClamp;
	float	Engine;
	float	Brake;
	// USO INTERNO DEL SISTEMA (NO MODIFICAR)
	AMG_FaceOBJ *face;							/**< Uso interno del sistema, no modificar / Internal usage */
} AMG_Object;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Struct which holds a 3D model
 */
#else
/**
 * @struct
 * @brief Estructura de un modelo 3D
 */
#endif
typedef struct{
	AMG_Object *Object;							/**< Puntero a los objetos 3D, no modificar / Pointer to 3D objects (don't write) */
	u8 FaceFormat;								/**< El formato de caras del modelo, no modificar / Face Format (don't write) */
	u16 NObjects;								/**< El número de objetos del modelo 3D, no modificar / Number of objects (don't write) */
	u8 CelShading;								/**< true/false para activar/desactivar el Cel-Shading (activar luces antes de usarlo) / true/false to enable/disable Cel-Shading (enable light before using this) */
}AMG_Model;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Structs for bone animted mesh
 */
#else
/**
 * @struct
 * @brief Estructuras para modelos animados con huesos
 */
#endif
typedef struct {
	ScePspQuatMatrix orient; 
	ScePspQuatMatrix interpolated;
	ScePspFVector3 position;
	signed char parent;
	signed char padd[3];
}__attribute__((aligned(16))) AMG_Bone;

typedef struct {
	AMG_Bone Bone[64];
}__attribute__((aligned(16))) AMG_SkinFrame;

typedef struct {
	u32 Start,End;
	int bones[8];
}AMG_SkinGroup;

typedef struct {
	u32 bullet_id;
	u32 material_name[2] = {0,0};
	u8 Collision;								/**< Si el objeto está en colisión con otro / If this object collides with another */
	u32 CollidedWith;							/**< El objeto con el que colisiona nuestro cuerpo / Object which collides with this one */
	u8 collisionreset;
	AMG_Texture *texture;
	AMG_Texture *MultiTexture;
	AMG_Vertex_W2TNV *Data;
	AMG_Vertex_W2V *Border;
	AMG_SkinGroup *Group;
	AMG_SkinFrame Frame[32];
	float CelShadingScale;
	u8 Lighting;
	int n_bones;
	int n_groups;
	int frameCount;
	int polyCount;
	int frame;
	int lastFrame;
	int startFrame;
	int endFrame;
	int loop;
	float speed;
	float interpolation;
	int fps;
	float morphoweight;
	ScePspFVector3 BBox[2];
// Motor fisico BULLET
	int phys;
	float Mass;									/**< Masa del objeto / Object mass */
}SkinnedModel;

typedef struct{
	SkinnedModel *Object;
	ScePspFVector3 __attribute__((aligned(16))) Pos;
	ScePspFVector3 __attribute__((aligned(16))) Rot;
	ScePspFVector3 __attribute__((aligned(16))) Scale;
	ScePspFVector3 __attribute__((aligned(16))) Origin;
	u8 Outline;
	ScePspFMatrix4 __attribute__((aligned(16))) bones[64];
	ScePspQuatMatrix __attribute__((aligned(16))) qpos1;
	ScePspQuatMatrix __attribute__((aligned(16))) qpos2;
	ScePspQuatMatrix __attribute__((aligned(16))) qpos_inter;
	ScePspFVector3 __attribute__((aligned(16))) pos;
	ScePspFVector3 __attribute__((aligned(16))) pfix;
	ScePspFVector3 __attribute__((aligned(16))) GPos;
	float f;
	float f2;
	int smooth;
	int Optimized;				//Activate Optimization to save ram
}AMG_Skinned_Actor;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Structs which holds morphing animated mesh
 */
#else
/**
 * @struct
 * @brief Estructuras de un modelo animado con "morphing"
 */
#endif

typedef struct {
	u32 bullet_id;
	u32 material_name[2] = {0,0};
	u8 Collision;								/**< Si el objeto está en colisión con otro / If this object collides with another */
	u32 CollidedWith;							/**< El objeto con el que colisiona nuestro cuerpo / Object which collides with this one */
	u8 collisionreset;
	AMG_Texture *texture;
	AMG_Texture *MultiTexture;
	AMG_Vertex_TCNV *Data;
	AMG_Vertex_V *Border;
	float CelShadingScale;
	u8 Lighting;
	int frameCount;
	int polyCount;
	int frame;
	float fr;
	int startFrame;
	int endFrame;
	int loop;
	float speed;
	float morphoweight;
	ScePspFVector3 *BBox;
// Motor fisico BULLET
	int phys;
	float Mass;									/**< Masa del objeto / Object mass */
}MorphModel;

typedef struct{
	MorphModel *Object;
	ScePspFVector3 __attribute__((aligned(16))) Pos;
	ScePspFVector3 __attribute__((aligned(16))) Rot;
	ScePspFVector3 __attribute__((aligned(16))) Scale;
	ScePspFVector3 __attribute__((aligned(16))) Origin;
	u8 Outline;
	int smooth;
}AMG_Morphing_Actor;

#ifdef AMG_DOC_ENGLISH
/**
 * @struct
 * @brief Structs which holds optimized mesh for fast loading and rendering
 * @notes No lighting
 */
#else
/**
 * @struct
 * @brief Estructuras para modelos optimizados para carga y dibujado rapido
 * @notes Sin iluminación
 */
#endif

typedef struct {
	u32 bullet_id;
	u32 material_name[2] = {0,0};
	u8 Collision;								/**< Si el objeto está en colisión con otro / If this object collides with another */
	u32 CollidedWith;							/**< El objeto con el que colisiona nuestro cuerpo / Object which collides with this one */
	u8 collisionreset;
	AMG_Texture *Texture;
	AMG_Vertex_TCV *Data;
	u32 polyCount;
// Motor fisico BULLET
	int phys;
	u8 Physics;
}MeshModel;

typedef struct{
	MeshModel *Object;
	ScePspFVector3 __attribute__((aligned(16))) Pos;
	ScePspFVector3 __attribute__((aligned(16))) Rot;
	ScePspFVector3 __attribute__((aligned(16))) Scale;
	ScePspFVector3 __attribute__((aligned(16))) Origin;
	u8 Outline;
} AMG_BinaryMesh;


typedef struct {
	unsigned char resolution;
	int mode;
	float size;
	float px0;
	float py0;
	float px1;
	float py1;
	float angle;
	float strength;
	ScePspFVector3 __attribute__((aligned(64))) Pos;
	ScePspFVector3 __attribute__((aligned(64))) Rot;
	ScePspFVector3 __attribute__((aligned(64))) Scale;
	AMG_Texture *Texture;
	AMG_Vertex_TNV *vertices;
	float *points;
} AMG_NurbsSurface;


// Defines
#define AMG_SHADE_LIGHT(n) ((n &0x7) << 1)


#ifdef AMG_DOC_ENGLISH
/**
 * @brief Draws a line with colors
 * @param x0 First point x position
 * @param y0 First point y position
 * @param color0 First point RGBA color
 * @param x1 Last point x position
 * @param y1 Last point y position
 * @param color1 Last point RGBA color
 * @return It returns nothing
 * @note It's just the osl line function but it can set 
	different colors for the start and end points
 */
#else
/**
 * @brief Dibuja una linea con colores
 * @param x0 Posicion x del primer punto
 * @param y0 Posicion y del primer punto
 * @param color0 Color RGBA del primer punto
 * @param x1 Posicion x del segundo punto
 * @param y1 Posicion y del segundo punto
 * @param color1 Color RGBA del segundo punto
 * @return No devuelve nada
 * @note Es solamente la funcion de oslib, pero con colores 
	ddiferentes para cada punto
 */
#endif
void M3D_DrawLine(int x0, int y0, u32 color0, int x1, int y1, u32 color1);


#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a 3D model in OBJ and MTL format
 * @param path OBJ file path (supports directories)
 * @param psm calidad de texturas
 * @return Pointer to the loaded 3D model
 * @note Model faces MUST be triangles
 */
#else
/**
 * @brief Carga un modelo 3D en formato OBJ y MTL
 * @param path La ruta del archivo .OBJ (soporta directorios)
 * @param psm textures quality
 * @return El modelo 3D cargado
 * @note Las caras del modelo deben estar trianguladas
 */
#endif
AMG_Model *AMG_LoadModel(const char *path, float css, u32 psm); ///cambio  Añadidos parámetros

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a 3D model in PLY format includes vertex colors
 * @param path .ply file path (supports directories)
 * @param psm calidad de texturas
 * @return Pointer to the loaded 3D model
 * @note Model faces MUST be triangles
 */
#else
/**
 * @brief Carga un modelo 3D en formato PLY, incluye colores de vertices.
 * @param path La ruta del archivo .ply (soporta directorios)
 * @param psm textures quality
 * @return El modelo 3D cargado
 * @note Las caras del modelo deben estar trianguladas
 */
#endif
AMG_Model *AMG_LoadModelPLY(const char *path, float css, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a 3D model in binary format includes vertex colors
 * @param path .o3b file path (supports directories)
 * @return Pointer to the loaded 3D model
 * @note for fast loading and rendering of big scenes, no lighting, and 256 color indexed textures.
 */
#else
/**
 * @brief Carga un modelo 3D en formato binario, incluye colores de vertices.
 * @param path La ruta del archivo .o3b (soporta directorios)
 * @return El modelo 3D cargado
 * @note para carga y renderizado ultra rapidos de escenas grandes, sin iluminacion y con texturas indexadas de 256 colores.
 */
#endif
AMG_BinaryMesh *AMG_LoadBinaryMesh(const char *path, u32 psm);

void AMG_RenderBinaryMesh(AMG_BinaryMesh *mesh, u32 offset);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a skinned model with animations from o3m file
 * @param path path to o3m + 8 bit texture

 * @return It returns an animated Actor
 */
#else
/**
 * @brief carga un modelo animado de un archivo o3m
 * @param path ruta al archivo o3m, + textura en 8 bits

 * @return devuelve un actor animado
 */
#endif
AMG_Skinned_Actor *AMG_LoadSkinnedActor(const char *path, float outline, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief draws a skinned model 
 * @param actor animated Actor
 * @return It returns nothing
 */
#else
/**
 * @brief dibuja un modelo animado 
 * @param actor actor animado
 * @return no devuelve nada
 */
#endif
void AMG_RenderSkinnedActor(AMG_Skinned_Actor *actor);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief configures an animated actor 
 * @param actor animated Actor
 * @param speed animation speed
 * @param begin start frame
 * @param end last frame
 * @param smooth hardware motion smooth
 * @return It returns nothing
 */
#else
/**
 * @brief configura un actor animado
 * @param actor actor animado
 * @param speed velocidad de animacion
 * @param begin posicion inicial
 * @param end posicion final
 * @param smooth suavizado de movimientos por hardware
 * @return no devuelve nada
 */
#endif
void AMG_ConfigSkinnedActor(AMG_Skinned_Actor *actor, int begin, int end, float speed, int loop, int smooth);

AMG_Skinned_Actor *AMG_CloneSkinnedActor(AMG_Skinned_Actor *actor);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Loads a morphing model from o3f file 
 * @param path file
 * @param outline load border
 * @param psm textures quality
 * @return It returns nothing
 */
#else
/**
 * @brief Carga un model con morphing de un archivo o3f
 * @param path ruta al archivo
 * @param outline borde del modelo
 * @param psm calidad de texturas
 * @return no devuelve nada
 */
#endif
AMG_Morphing_Actor *AMG_LoadMorphingActor(char *path, float outline, u32 psm);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Renders a morphing model 
 * @param actor AMG_Morphing_Actor
 * @return It returns nothing
 */
#else
/**
 * @brief dibuja un modelo con morphing
 * @param actor AMG_Morphing_Actor
 * @return no devuelve nada
 */
#endif
void AMG_RenderMorphingActor(AMG_Morphing_Actor *actor);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief configures an morphing model 
 * @param actor AMG_Morphing_Actor
 * @param speed animation speed
 * @param begin start frame
 * @param end last frame
 * @param smooth hardware motion smooth
 * @return It returns nothing
 */
#else
/**
 * @brief configura un modelo con morphing
 * @param actor AMG_Morphing_Actor
 * @param speed velocidad de animacion
 * @param begin posicion inicial
 * @param end posicion final
 * @param smooth suavizado de movimientos por hardware
 * @return no devuelve nada
 */
#endif
void AMG_ConfigMorphingActor(AMG_Morphing_Actor *actor, int begin, int end, float speed, int smooth);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief 
 * @param 
 * @return 
 * @note 
 */
#else
/**
 * @brief 
 * @param
 * @return 
 * @note 
 */
#endif
void AMG_RenderVehicle(AMG_Model *model,AMG_Model *wheel);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Renders a 3D model (OBJ & PLY)
 * @param model Pointer to the 3D model loaded with AMG_LoadModel();
 * @return It returns nothing
 */
#else
/**
 * @brief Renderiza un modelo 3D (OBJ & PLY)
 * @param model Puntero al modelo 3D cargado con AMG_LoadModel();
 * @return No devuelve nada
 */
#endif
void AMG_RenderModel(AMG_Model *model, int transparente);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Renders a 3D object (OBJ & PLY)
 * @param model Pointer to the 3D object
 * @param cs Render it using Cel-Shading?
 * @return It returns nothing
 */
#else
/**
 * @brief Renderiza un objeto 3D (OBJ & PLY)
 * @param model Puntero al objeto 3D
 * @param cs ¿Renderizar con Cel-Shading?
 * @return No devuelve nada
 */
#endif
void AMG_RenderObject(AMG_Object *model, u8 cs, int transparente);


#ifdef AMG_DOC_ENGLISH
/**
 * @brief Deletes a 3D model
 * @param model Model to delete
 * @return It returns nothing
 */
#else
/**
 * @brief Elimina un objeto 3D
 * @param model El objeto a borrar
 * @return No devuelve nada
 */
#endif
void AMG_UnloadObject(AMG_Object *model);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Deletes a 3D model previously loaded
 * @param model Pointer to the 3D model
 * @return It returns nothing
 */
#else
/**
 * @brief Elimina un modelo 3D previamente cargado
 * @param model Puntero al modelo 3D
 * @return No devuelve nada
 */
#endif
void AMG_UnloadModel(AMG_Model *model);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Calculates the lighting normals of a 3D model
 * @param model Pointer to the model
 * @return It returns nothing
 * @note It doesn't matter if the model doesn't have normals, they're created automatically
 */
#else
/**
 * @brief Calcula las normales de iluminación de un modelo 3D
 * @param model Puntero al modelo 3D
 * @return No devuelve nada
 * @note No importa si el modelo no tiene normales, se crean automáticamente
 */
#endif
void AMG_NormalizeModel(AMG_Model *model);


void AMG_DrawSkyBox(AMG_Model *model,float fov);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Check if two 3D objects are colliding
 * @param obj1 Object 1
 * @param obj2 Object 2
 * @return Wether there is collision between them
 */
#else
/**
 * @brief Comprueba si dos objetos 3D están en colisión
 * @param obj1 El objeto 1
 * @param obj2 El objeto 2
 * @return Si hay colisión o no entre ellos
 */
#endif
u8 AMG_CheckBBoxCollision(const AMG_Object *obj1, const AMG_Object *obj2);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Enables reflection and shadows using Stencil Buffer
 * @param obj The 3D object which is gonna be a "floor"
 * @return It returns nothing
 */
#else
/**
 * @brief Activa el motor de reflejos y sombras con Stencil Buffer
 * @param obj El objeto 3D que hará de "suelo"
 * @return No devuelve nada
 */
#endif
void AMG_StartReflection(AMG_Model *model, u8 number);//, int transparente);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Renders a 3D object which makes reflection with the "floor"
 * @param obj The reflected 3D object
 * @param axis The axis (0, 1 or 2 for X, Y and Z)
 * @param inv Flip the reflection?
 * @return It returns nothing
 */
#else
/**
 * @brief Renderiza un objeto 3D que refleje con el "suelo"
 * @param obj El objeto 3D que será reflejado
 * @param axis El eje donde se hará el reflejado (0, 1 o 2 para X, Y y Z)
 * @param inv Invertir el reflejado?
 * @return No devuelve nada
 */
#endif
void AMG_RenderMirrorObject(AMG_Model *model, u8 number, u8 axis);//, u8 inv, int transparente);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Disables reflection and shadows engine and Stencil Buffer
 * @return It returns nothing
 */
#else
/**
 * @brief Desactiva el motor de reflejos y sombras con Stencil Buffer
 * @return No devuelve nada
 */
#endif
/*
static inline void AMG_FinishReflection(void){
	sceGuDisable(GU_STENCIL_TEST);
}*/
void AMG_FinishReflection();

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Renders a real-time shadow of and object
 * @param obj The 3D object which will be projected as a shadow
 * @param l The light ID which affects the shadow (0-3)
 * @param plane Plane equation of the floor (see AMG_PlaneEquation)
 * @return It returns nothing
 */
#else
/**
 * @brief Renderiza la sombra de un objeto 3D en tiempo real
 * @param obj El objeto 3D del que se hará la sombra
 * @param l El ID de la luz que provoca esa sombra (0-3)
 * @param plane La ecuación del plano del suelo (mirar AMG_PlaneEquation)
 * @return No devuelve nada
 */
#endif
void AMG_RenderShadow(AMG_Object *obj, u8 l, ScePspFVector4 *plane);

void AMG_RenderSkinnedActorShadow(AMG_Skinned_Actor *actor);


void AMG_SetModelTexture(AMG_Model *model, int obj_number, int group_number, AMG_Texture *tex);

void AMG_SetModelTextureMapping(AMG_Model *model, int obj_number, int group_number, int ntex, u32 mapping);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Set a second texture of a 3D object
 * @param obj Material Group
 * @param tex The loaded texture
 * @return It returns nothing
 */
#else
/**
 * @brief Establece la segunda textura de un objeto 3D
 * @param obj El grupo de materiales
 * @param tex La textura ya cargada
 * @return No devuelve nada
 */
#endif
void AMG_SetModelMultiTexture(AMG_Model *model, int obj_number, int group_number, AMG_Texture *tex);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Set a second texture of a 3D actor
 * @param obj Material Group
 * @param tex The loaded texture
 * @return It returns nothing
 */
#else
/**
 * @brief Establece la segunda textura de un actor 3D
 * @param obj El grupo de materiales
 * @param tex La textura ya cargada
 * @return No devuelve nada
 */
#endif
/*static inline void AMG_SetActorMultiTexture(AMG_Actor *actor, AMG_Texture *tex){
	tex->TFX = GU_TFX_MODULATE;
	actor->Object[0].MultiTexture = tex;
}*/

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Calculates plane equation
 * @param points 3 points of the plane
 * @param p Where the plane equation will be saved
 * @return It returns nothing
 */
#else
/**
 * @brief Calcula la ecuación del plano
 * @param points 3 puntos del plano
 * @param p Donde se guardará la ecuación del plano
 * @return No devuelve nada
 */
#endif
void AMG_PlaneEquation(ScePspFVector3 *points, ScePspFVector4 *p);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Generate floor points (to calculate shadows)
 * @param obj The 3D object that works as a floor
 * @param points Where the calculated points will be saved (use them at AMG_PlaneEquation)
 * @return It returns nothing
 */
#else
/**
 * @brief Genera los puntos del suelo (para el cálculo de sombras)
 * @param obj El objeto 3D que hará de suelo
 * @param points Donde se almacenarán los puntos calculados (úsalos en AMG_PlaneEquation)
 * @return No devuelve nada
 */
#endif
void AMG_GenerateFloorPoints(AMG_Object *obj, ScePspFVector3 *points);

// Los TFX disponibles
#define AMG_NO_LIGHT 					GU_TFX_DECAL
#define AMG_LIGHT						GU_TFX_MODULATE
#define AMG_LIGHT_AND_COLOR_BLENDING	GU_TFX_BLEND

/*
AMG_Animation *AMG_LoadAnimation(char *path);

void AMG_UnloadAnimation(AMG_Animation *anim);
*/

void AMG_RenderStyle(u8 Render_M);//Mills 04/08/15


//Real shadow


void ExportPLY_RawModel(AMG_Model *model);

#ifdef __cplusplus
	}
#endif

#endif
