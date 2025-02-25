// Includes
#include <oslib/oslib.h>
#include "AMG_Model.h"
#include "AMG_User.h"
#include "AMG_3D.h"
#include <pspgu.h>
#include <pspgum.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <pspmath.h>


extern float vfpu_fminf(float x,float y);//in vfpu
extern float vfpu_fmaxf(float x,float y);//in vfpu

int skip;

void AMG_UpdateBody(AMG_Object *model);
void AMG_UpdateBINBody(AMG_BinaryMesh *model);
void AMG_UpdateVehicleBody(AMG_Model *model);
void AMG_UpdateVehicleWheel(AMG_Model *model, int i);

u32 l = 0;

ScePspFVector3 s;		// Lo siento, no me gustan las variables locales :P
AMG_Object *amg_curfloor;	// El suelo actual (para reflejos y sombras)

//////////////////////

u8 Render_Style = GU_TRIANGLES;//Mills 04/08/15

// Cambia el modo de renderizado 3d //Mills 04/08/15
void AMG_RenderStyle(u8 Render_M){
	Render_Style = Render_M;
}

// Obten el directorio de un archivo
char *getdir(const char *path){
	char *dir = strdup(path);
	char *s = strrchr(dir, '/');
	if(s) s[1]   = '\0';
	else dir[0] = '\0';
	return dir;
}

//A copy of oslDrawLine but with one color per point
void M3D_DrawLine(int x0, int y0, u32 color0, int x1, int y1, u32 color1){
	OSL_LINE_VERTEX *vertices = (OSL_LINE_VERTEX*)sceGuGetMemory(2 * sizeof(OSL_LINE_VERTEX));
	
	vertices[0].color = color0;
	vertices[0].x = x0; vertices[0].y = y0; vertices[0].z = 0;

	vertices[1].color = color1; 
	vertices[1].x = x1; vertices[1].y = y1; vertices[1].z = 0;

	sceGuDisable(GU_TEXTURE_2D);
	sceGuDrawArray(GU_LINES, GU_COLOR_8888|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2, 0, vertices);
    sceKernelDcacheWritebackRange(vertices, 2 * sizeof(OSL_LINE_VERTEX)); //SAKYA
}

// Carga un modelo en formato binario
//Implement blue vertices as transparent in BLENDER 2.7 export plugin
AMG_BinaryMesh *AMG_LoadBinaryMesh(const char *path, u32 psm){
	// Crea el modelo 3D
	AMG_BinaryMesh *model = NULL;
	model = (AMG_BinaryMesh*) calloc (1, sizeof(AMG_BinaryMesh));
	FILE *f = fopen(path, "rb");
	
	if(f == NULL){ AMG_Error((char*)"File not found / Archivo no encontrado", path);}
{
	u32 header = 0; 
	fseek(f, 0, SEEK_SET);
	fread (&header,1,4,f);
	//M3BP
	if(header != 0x5042334D) {AMG_Error((char*)"wrong M3B format / Formato M3B incorrecto", path); }
	
	u32 nobj = 1;
	u32 data_size;
	//PREPARA MEMORIA
	model->Object = (MeshModel*) calloc (nobj, sizeof(MeshModel));
	
	model->Object[0].Texture = NULL;
	model->Object[0].Data = NULL;
	model->Object[0].polyCount = 0;
	model->Object[0].phys = 0;
	model->Object[0].bullet_id = 0;
	model->Object[0].collisionreset = 0;
	
	//CUENTA VERTICES
	fseek(f,0x18, SEEK_SET);
	fread(&model->Object[0].polyCount,1,4,f);
	fseek(f,0x26, SEEK_SET);
	fread(&data_size,1,4,f);
	model->Object[0].Data = (AMG_Vertex_TCV*) calloc(data_size,1);
	fseek(f,0x30, SEEK_SET);
	fread(&model->Object[0].Data[0],data_size,1,f);
	fclose(f);
	
	sceKernelDcacheWritebackInvalidateRange(model->Object[0].Data,data_size);
	
	//get texture with the same name as the file
	char *texturename = (char*) calloc (128, sizeof(char));
	strncpy(texturename,path,127);
	texturename[strlen(path)-4] = 0;
	strcat(texturename,".png");
	int result = access(texturename, F_OK);
	if (result == 0) {
		AMG_Texture *texture = AMG_LoadTexture(texturename,AMG_TEX_RAM,psm);
		if (texture) model->Object[0].Texture = texture;
		else model->Object[0].Texture = NULL;
	} else model->Object[0].Texture = NULL;
	
	model->Pos = (ScePspFVector3) {0,0,0};
	model->Rot = (ScePspFVector3) {0,0,0};
	model->Scale = (ScePspFVector3) {1,1,1};
	
	return model;
}

}

M3D_ModelBIN *M3D_LoadModelBIN(const char *path, u32 psm){
	AMG_BinaryMesh *mesh = AMG_LoadBinaryMesh(path,psm);
	return (M3D_ModelBIN*) mesh;
}

void AMG_RenderBinaryMesh(AMG_BinaryMesh *mesh, u32 offset){
	if(mesh == NULL) return;
  if(!skip){	
    AMG_PushMatrix(GU_MODEL);
	if(mesh->Object[0].Physics == 0){
		AMG_Translate(GU_MODEL, &mesh->Pos);
		AMG_Rotate(GU_MODEL, &mesh->Rot);
		AMG_Scale(GU_MODEL, &mesh->Scale);
	} else {
		AMG_UpdateBINBody(mesh);
	}
	AMG_UpdateMatrices();											// Actualiza las matrices
	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
	sceGuSpecular(AMG.WorldSpecular);	
	sceGuColor(0xffffffff); sceGuAmbient(0xffffffff);

	AMG_EnableTexture(mesh->Object[0].Texture);
	sceGuAmbientColor(0xffffffff);
	//DRAW
	
	sceGuEnable(GU_CULL_FACE);
	sceGuFrontFace(GU_CCW);
	sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,mesh->Object[0].polyCount*3,0,mesh->Object[0].Data);
	sceGuDisable(GU_CULL_FACE);
	AMG_PopMatrix(GU_MODEL); 
  }
}

void M3D_ModelBINRender(M3D_ModelBIN *mesh, u32 offset){
	AMG_BinaryMesh *m = (AMG_BinaryMesh*) mesh;
	AMG_RenderBinaryMesh(m,offset);
}

//Add set pos, set rot etc


// Carga un modelo en formato PLY
AMG_Model *AMG_LoadModelPLY(const char *path, float css, u32 psm){
	AMG_Vertex_V *v = NULL;		//Only vertex
	AMG_Vertex_NV *nv = NULL;	//vertex+nomals
	AMG_Vertex_TV *tv = NULL;	//vertex+texture
	AMG_Vertex_CV *cv = NULL;	//vertex+vertex color
	AMG_Vertex_TNV *tnv = NULL; //vertex+normal+texture
	AMG_Vertex_CNV *cnv = NULL;	//vertex+normal+vertex color
	AMG_Vertex_CTV *ctv = NULL;	//vertex+texture+vertex color
	AMG_Vertex_TCNV *ttcnv = NULL;//vertex+normal+texture+vertex color
	AMG_Vertex_TCNV *ftcnv = NULL;
	AMG_Vertex_V *outl = NULL;

	// Crea el modelo 3D
	AMG_Model *model = NULL;
	model = (AMG_Model*) calloc (1, sizeof(AMG_Model));
	FILE *f = fopen(path, "rb");
	
	if(f == NULL){ AMG_Error((char*)"File not found / Archivo no encontrado", path);}
{
	model->CelShading = 0;
	
	uint32_t *vindex; 
	uint32_t vtx = 0;
	uint32_t nvindex = 0;
	uint32_t nvtx = 0;
	uint32_t nobj = 0;
	u8 vertex_xyz  = 0;
	u8 vertex_n = 0;
	u8 vertex_st = 0;
	u8 vertex_rgb  = 0;
	u8 _mode = 0;
	uint32_t i = 0;

	char *line = (char*) calloc (128, sizeof(char));

	int colorR,colorG,colorB = 0;
	
	//CUENTA OBJETOS 
	nobj = 1;
	
	//PREPARA MEMORIA
	model->NObjects = nobj;
	model->Object = (AMG_Object*) calloc (model->NObjects, sizeof(AMG_Object));

	//model->Object[i].NFaces = 0;
	model->Object[0].Group = NULL;
	model->Object[0].Data = NULL;
	model->Object[0].Triangles = NULL;
	model->Object[0].Flags = (GU_VERTEX_32BITF | GU_TRANSFORM_3D);	// Flags por defecto
	model->Object[0].BBox = NULL;
	model->Object[0].BBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));
	model->Object[0].tBBox = NULL;
	model->Object[0].tBBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));
	model->Object[0].CelShadingScale = 1.025f;
	model->Object[0].OutLine = NULL;
	model->Object[0].DrawBehind = 0;
	model->Object[0].OutlineColor = GU_RGBA(0, 0, 0, 0xFF);
	model->Object[0].Lighting = 1;
	model->Object[0].Pos.x = 0.0f; model->Object[i].Pos.y = 0.0f; model->Object[i].Pos.z = 0.0f;
	model->Object[0].Origin.x = 0.0f; model->Object[i].Origin.y = 0.0f; model->Object[i].Origin.z = 0.0f;
	model->Object[0].Rot.x = 0.0f; model->Object[i].Rot.y = 0.0f; model->Object[i].Rot.z = 0.0f;
	model->Object[0].Scale.x = 1.0f; model->Object[i].Scale.y = 1.0f; model->Object[i].Scale.z = 1.0f;
	model->Object[0].Physics = 0;
	model->Object[0].collisionreset = 0;
	if (css)
		model->Object[0].CelShadingScale = css;
	else
		model->Object[0].CelShadingScale = 0.2f; ///cambio tamaño del borde por
	// BULLET
	model->Object[0].Mass = 0.0f;
	model->Object[0].isGround = 0;
	model->Object[0].ShapeType = 0;
	model->Object[0].Collision = 0;
	model->Object[0].CollidedWith = 0;
	model->Object[i].vmax = 300.0f;

	nvtx = 0; nobj = 0; 
	//CUENTA MATERIALES DE CADA OBJETO

	vtx = 0;
	// Crea los grupos de materiales
	model->Object[0].NGroups = 1;
	model->Object[0].Group = (AMG_ObjectGroup*) calloc (model->Object[0].NGroups, sizeof(AMG_ObjectGroup));

	model->Object[0].Group[0].Ambient = GU_RGBA(0x00, 0x00, 0x00, 0xFF);
	model->Object[0].Group[0].Emmision = GU_RGBA(0, 0, 0, 0xFF);
	model->Object[0].Group[0].Diffuse = GU_RGBA(0xFF, 0xFF, 0xFF, 0xFF);
	model->Object[0].Group[0].Specular = GU_RGBA(0xFF, 0xFF, 0xFF, 0xFF);
	model->Object[0].Group[0].Texture = NULL;
	model->Object[0].Group[0].MultiTexture = NULL;
	model->Object[0].Group[0].Start = 0;
	model->Object[0].Group[0].mtlname = (char*) calloc (64, sizeof(char));

	//model->Object[i].NFaces = model->Object[i].Group[model->Object[i].NGroups-1].End;
	model->Object[0].Triangles = memalign (16, model->Object[0].NFaces*3*sizeof(AMG_Vertex_V));	// Sombra para Cel-Shading
	model->Object[0].Data = NULL;
	model->Object[0].TriangleSize = sizeof(AMG_Vertex_TCNV)*3;
	
	// Crea el buffer temporal de caras
	model->Object[0].face = NULL;
	model->Object[0].face = (AMG_FaceOBJ*) calloc (model->Object[0].NFaces, sizeof(AMG_FaceOBJ));
	
	nobj = 0;
	nvtx = 0;
	nvindex = 0;
	//CUENTA VERTICES Y CARAS
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if (line[0] == 'e'){
			if(line[8] == 'v'){			
				sscanf(line, "element vertex %lu",&nvtx);
			}
			if(line[8] == 'f'){			
				sscanf(line, "element face %lu",&model->Object[0].NFaces);
				model->Object[0].Group[0].End = model->Object[0].NFaces;
				break;
			}			
		}
		if (line[0] == 'p'){
			if(line[ 8] == 'v') sscanf(line, "element vertex %lu",&nvtx);
			//property float z
			if(line[15] == 'z') vertex_xyz = 1;
			//property float nz
			if(line[16] == 'z') vertex_n = 1;
			//property float t
			if(line[15] == 't') vertex_st = 1;
			//property uchar blue
			if(line[15] == 'b') vertex_rgb = 1;
		}
	}
	
	if (!vertex_xyz) ;//Error
	if(!vertex_n && !vertex_st && !vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_V *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_V));
		v = (AMG_Vertex_V*)model->Object[0].Data;
		model->Object[0].Lighting = 0;
		_mode = 0;
	} else if (vertex_n && !vertex_st && !vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_NV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_NV));
		nv = (AMG_Vertex_NV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_NORMAL_8BIT;
		_mode = 1;
	} else if (!vertex_n && vertex_st && !vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_TV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_TV));
		tv = (AMG_Vertex_TV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_TEXTURE_32BITF;
		model->Object[0].Lighting = 0;
		_mode = 2;
	} else if (!vertex_n && !vertex_st && vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_CV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_CV));
		cv = (AMG_Vertex_CV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_COLOR_8888;
		model->Object[0].Lighting = 0;
		_mode = 3;
	} else if (vertex_n && vertex_st && !vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_TNV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_TNV));
		tnv = (AMG_Vertex_TNV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_NORMAL_8BIT | GU_TEXTURE_32BITF;
		_mode = 4;
	} else if (vertex_n && !vertex_st && vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_CNV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_CNV));
		cnv = (AMG_Vertex_CNV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_NORMAL_8BIT | GU_COLOR_8888;
		_mode = 5;
	} else if (!vertex_n && vertex_st && vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_CTV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_CTV));
		ctv = (AMG_Vertex_CTV*)model->Object[0].Data;
		model->Object[0].Flags |= GU_TEXTURE_32BITF | GU_COLOR_8888;
		model->Object[0].Lighting = 0;
		_mode = 6;
	} else if (vertex_n && vertex_st && vertex_rgb){
		model->Object[0].Data = (AMG_Vertex_TCNV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_TCNV));
		model->Object[0].Flags |= (GU_NORMAL_8BIT|GU_TEXTURE_32BITF|GU_COLOR_8888);
		_mode = 7;
	}
	//READ VERTICES HERE                   (*3?)
	ttcnv = (AMG_Vertex_TCNV *) malloc (nvtx*3*sizeof(AMG_Vertex_TCNV));

	//LEE VERTICES
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if ((line[0] < 58)&&(line[1] != ' ')){//ascii, not a letter, and not a space in 1.
			float nx,ny,nz;
			// n, t, c, nt, nc, tc, ntc,
			switch (_mode){
				case 0:
				sscanf(line,"%f %f %f",&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z);
				break;
				case 1:
				sscanf(line,"%f %f %f %f %f %f",&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,&nx, &ny, &nz);
				break;
				case 2:
				sscanf(line,"%f %f %f %f %f",&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,&ttcnv[vtx].u, &ttcnv[vtx].v);
				break;
				case 3:
				sscanf(line,"%f %f %f %d %d %d",&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,&colorR, &colorG, &colorB);
				ttcnv[vtx].color = GU_RGBA(colorR,colorG,colorB,255);
				break;
				case 4:
				sscanf(line,"%f %f %f %f %f %f %f %f ",
				&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,
				&nx, &ny, &nz,	
				&ttcnv[vtx].u, &ttcnv[vtx].v);
				break;
				case 5:
				sscanf(line,"%f %f %f %f %f %f %d %d %d ",
				&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,
				&nx, &ny, &nz,	
				&colorR, &colorG, &colorB);
				break;
				case 6:
				sscanf(line,"%f %f %f %f %f %d %d %d ",
				&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,
				&ttcnv[vtx].u, &ttcnv[vtx].v,	
				&colorR, &colorG, &colorB);
				break;
				case 7:
				sscanf(line,"%f %f %f %f %f %f %f %f %d %d %d ",
				&ttcnv[vtx].x, &ttcnv[vtx].y,&ttcnv[vtx].z,
				&nx, &ny, &nz,	
				&ttcnv[vtx].u, &ttcnv[vtx].v,
				&colorR, &colorG, &colorB);
				break;
			}
			//8 bit normals
			if (vertex_n) {
				ttcnv[vtx].nx = (signed char) (127*nx);
				ttcnv[vtx].ny = (signed char) (127*ny);
				ttcnv[vtx].nz = (signed char) (127*nz);
			}
			if (vertex_st) {
				//invert texture v coords
				ttcnv[vtx].v = -1*ttcnv[vtx].v;
			}
			//Pure blue vertices are transparent
			if (vertex_rgb){
				ttcnv[vtx].color = GU_RGBA(colorR,colorG,colorB,255);
				if (!colorR && !colorG && colorB == 0xFF) ttcnv[vtx].color = 0x00000000;
			}
			//next vertex
			vtx++;
		}
	}
	vtx = 0;
	rewind(f);

	//INDICES
	vindex = (u32 *) malloc (model->Object[0].NFaces*3*sizeof(u32));
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if ((line[0] == '3')&&(line[1] == ' ')){//ascii, equal 3 (avoid 3.00...)
			sscanf(line,"3 %ld %ld %ld",&vindex[nvindex],&vindex[nvindex+1],&vindex[nvindex+2]);
			nvindex+=3;
			vtx++;
		}
	}
	fclose(f);
	
	model->Object[0].Mass = vtx;
	//ARRANGE VERTICES
	model->Object[0].Triangles = (AMG_Vertex_V *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_V));
	AMG_Vertex_V *s = (AMG_Vertex_V*) model->Object[0].Triangles;
	
	ftcnv =(AMG_Vertex_TCNV *) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_TCNV));
	for (i = 0; i < model->Object[0].NFaces*3;i++){
		u32 index = vindex[i];
		ftcnv[i] = ttcnv[index];
		s[i].x = ftcnv[i].x; 
		s[i].y = ftcnv[i].y;
		s[i].z = ftcnv[i].z;
	}
	
	//GET SIZE
	model->Object[0].BBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));
	// Calcula la bounding box
	for(i=0;i<model->Object[0].NFaces*3;i++) {
		model->Object[0].BBox[0].x = vfpu_fminf((float)model->Object[0].BBox[0].x,(float)ftcnv[i].x); //MIN X
		model->Object[0].BBox[0].y = vfpu_fminf((float)model->Object[0].BBox[0].y,(float)ftcnv[i].y); //MIN Y
		model->Object[0].BBox[0].z = vfpu_fminf((float)model->Object[0].BBox[0].z,(float)ftcnv[i].z); //MIN Z
		model->Object[0].BBox[1].x = vfpu_fmaxf((float)model->Object[0].BBox[1].x,(float)ftcnv[i].x); //MAX X
		model->Object[0].BBox[1].y = vfpu_fmaxf((float)model->Object[0].BBox[1].y,(float)ftcnv[i].y); //MAX Y
		model->Object[0].BBox[1].z = vfpu_fmaxf((float)model->Object[0].BBox[1].z,(float)ftcnv[i].z); //MAX Z
		/*
		if (model->Object[0].BBox[0].x >= ftcnv[i].x) model->Object[0].BBox[0].x = ftcnv[i].x; //MIN X
		if (model->Object[0].BBox[0].y >= ftcnv[i].y) model->Object[0].BBox[0].y = ftcnv[i].y; //MIN Y
		if (model->Object[0].BBox[0].z >= ftcnv[i].z) model->Object[0].BBox[0].z = ftcnv[i].z; //MIN Z
		if (model->Object[0].BBox[1].x <= ftcnv[i].x) model->Object[0].BBox[1].x = ftcnv[i].x; //MAX X
		if (model->Object[0].BBox[1].y <= ftcnv[i].y) model->Object[0].BBox[1].y = ftcnv[i].y; //MAX Y
		if (model->Object[0].BBox[1].z <= ftcnv[i].z) model->Object[0].BBox[1].z = ftcnv[i].z; //MAX Z
		*/
	}
	
	//COPY TO DATA
	switch (_mode){
		case 0:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				v[i].x = ftcnv[i].x; v[i].y = ftcnv[i].y; v[i].z = ftcnv[i].z;
			}
		break;
		case 1:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				nv[i].x = ftcnv[i].x; nv[i].y = ftcnv[i].y; nv[i].z = ftcnv[i].z;
				nv[i].nx = ftcnv[i].nx; nv[i].ny = ftcnv[i].ny; nv[i].nz = ftcnv[i].nz;
			}
		break;
		case 2:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				tv[i].x = ftcnv[i].x; tv[i].y = ftcnv[i].y; tv[i].z = ftcnv[i].z;
				tv[i].u = ftcnv[i].u; tv[i].v = ftcnv[i].v;
			}
		break;
		case 3:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				cv[i].x = ftcnv[i].x; cv[i].y = ftcnv[i].y; cv[i].z = ftcnv[i].z;
				cv[i].color = ftcnv[i].color;
			}
		break;
		case 4:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				tnv[i].x = ftcnv[i].x; tnv[i].y = ftcnv[i].y; tnv[i].z = ftcnv[i].z;
				tnv[i].u = ftcnv[i].u; tnv[i].v = ftcnv[i].v;
				tnv[i].nx = ftcnv[i].nx; tnv[i].ny = ftcnv[i].ny; tnv[i].nz = ftcnv[i].nz;
			}
		break;
		case 5:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				cnv[i].x = ftcnv[i].x; cnv[i].y = ftcnv[i].y; cnv[i].z = ftcnv[i].z;
				cnv[i].nx = ftcnv[i].nx; cnv[i].ny = ftcnv[i].ny; cnv[i].nz = ftcnv[i].nz;
				cnv[i].color = ftcnv[i].color;
			}
		break;
		case 6:
			for (i = 0; i < model->Object[0].NFaces*3;i++){
				ctv[i].x = ftcnv[i].x; ctv[i].y = ftcnv[i].y; ctv[i].z = ftcnv[i].z;
				ctv[i].u = ftcnv[i].u; ctv[i].v = ftcnv[i].v;
				ctv[i].color = ftcnv[i].color;
			}
		break;
		case 7:
			memcpy(model->Object[0].Data,ftcnv,model->Object[0].NFaces*3*sizeof(AMG_Vertex_TCNV));
		break;
	}

	if((model->Object[0].Data == NULL) || (model->Object[0].Triangles == NULL) || (model->Object[0].face == NULL)){
		AMG_Error((char*)"Load error / Error cargando", path);
	}
	
	//outline
	if (css && css>0){
		model->Object[0].OutLine = (AMG_Vertex_V*) malloc (model->Object[0].NFaces*3*sizeof(AMG_Vertex_V));
		for (i = 0; i < model->Object[0].NFaces*3;i++){
			model->Object[0].OutLine[i].x = s[i].x + (((float)ftcnv[i].nx * model->Object[0].CelShadingScale)/127);
			model->Object[0].OutLine[i].y = s[i].y + (((float)ftcnv[i].ny * model->Object[0].CelShadingScale)/127);
			model->Object[0].OutLine[i].z = s[i].z + (((float)ftcnv[i].nz * model->Object[0].CelShadingScale)/127);
		}	
	}
	
	//get texture with the same name as the file, if ply has tex coords
	if(vertex_st){
		char *texturename = (char*) calloc (128, sizeof(char));
		strncpy(texturename,path,127);
		texturename[strlen(path)-4] = 0;
		strcat(texturename,".png");
		int result = access(texturename, F_OK);
		if (result == 0) {
			AMG_Texture *texture = AMG_LoadTexture(texturename,AMG_TEX_RAM,psm);
			model->Object[0].Group[0].Texture = texture;
		} else model->Object[0].Group[0].Texture = NULL;
	}
	nvtx = 0;

	//Free used ram
	if(line) {free(line); line = NULL;     }
	if(ttcnv) {free(ttcnv); ttcnv = NULL;  }
	if(ftcnv) {free(ftcnv); ftcnv = NULL;  }
	if (outl) {free(outl); outl = NULL;    }
	i = 0;
	
	return model;
}
	
}
 
M3D_Model *M3D_LoadModelPLY(const char *path, float css, u32 psm){
	AMG_Model *model = AMG_LoadModelPLY(path,css,psm);
	return (M3D_Model*) model;
}


// Exporta un modelo PLY en formato RAW para PSP NV
void ExportPLY_RawModel(AMG_Model *model){
	FILE *f = fopen("exportedRaw.raw", "w");
	
	AMG_Vertex_NV *Triangles = (AMG_Vertex_NV *) model->Object[0].Data;
	unsigned int i;
	for (i = 0; i < model->Object[0].NFaces*3;i++){
		fprintf(f,"{%i, %i, %i, ",Triangles[i].nx,Triangles[i].ny,Triangles[i].nz);
		fprintf(f,"%f, %f, %f},\n",Triangles[i].x,Triangles[i].y,Triangles[i].z);
	}
	
	fclose(f);
}

// Carga un modelo en formato OBJ y MTL
AMG_Model *AMG_LoadModel(const char *path, float css, u32 psm){
	// Define variables
	u32 j=0;
	AMG_Vertex_TV *tcv = NULL; AMG_Vertex_TNV *tcnv = NULL; AMG_Vertex_NV *cnv = NULL; AMG_Vertex_V *cv = NULL;
	u32 face_idx = 0;
	u8 ng = 0;
	int i = 0, faceformat = 0;
	float *vtx = NULL, *nrm = NULL, *txc = NULL;
	u8 noObjects = 0;
	u32 nobj = 0, nvtx = 0, nnrm = 0, ntxc = 0;
	char *line = (char*) calloc (128, sizeof(char));
	char *mtlpath = (char*) calloc (128, sizeof(char));
	char *straux = (char*) calloc (64, sizeof(char));
	
	// Crea el modelo 3D
	AMG_Model *model = NULL;
	model = (AMG_Model*) calloc (1, sizeof(AMG_Model));
	// Abre el archivo OBJ
	FILE *f = fopen(path, "rb");
	if(f == NULL) AMG_Error((char*)"File not found / Archivo no encontrado",(char*) path);
	model->CelShading = 0;
	
	// Primera lectura
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		switch(line[0]){
			case '#': break;
			case 'm': // mtllib
				sscanf(line, "mtllib %s", straux);
				sprintf(mtlpath, "%s%s", getdir(path), straux);
				break;
			case 'o': nobj ++; break;
			case 'v':
				switch(line[1]){
					case ' ': nvtx ++; break;
					case 'n': nnrm ++; break;
					case 't': ntxc ++; break;
					default: break;
				} break;
			default: break;
		}
	}
	
	// Si no hay objetos, pon uno al menos (para los modelos exportados con Sketchup)
	if(nobj == 0){
		nobj = 1;
		noObjects = 1;
	}
	
	// Prepara buffers
	vtx = (float*) calloc (nvtx*3, sizeof(float));
	if(ntxc) txc = (float*) calloc (ntxc<<1, sizeof(float));
	if(nnrm) nrm = (float*) calloc (nnrm*3, sizeof(float));
	model->NObjects = nobj;
	model->Object = (AMG_Object*) calloc (model->NObjects, sizeof(AMG_Object));
	nobj = nvtx = nnrm = ntxc = 0;
	for(i=0;i<model->NObjects;i++){
		model->Object[i].NGroups = 0;
		model->Object[i].Group = NULL;
		model->Object[i].Data = NULL;
		model->Object[i].Triangles = NULL;
		model->Object[i].Flags = (GU_VERTEX_32BITF | GU_TRANSFORM_3D);	// Flags por defecto
		model->Object[i].BBox = NULL;
		model->Object[i].BBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));
		model->Object[i].tBBox = NULL;
		model->Object[i].tBBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));
		model->Object[i].Physics = 0;
		model->Object[i].collisionreset = 0;
		if (css)
			model->Object[i].CelShadingScale = css;
		else
			model->Object[i].CelShadingScale = 0.2f; ///cambio tamaño del borde por defecto
//##### añade la siguiente
        model->Object[i].OutLine = NULL;
		model->Object[i].OutlineColor = GU_RGBA(0, 0, 0, 0xFF);
		model->Object[i].DrawBehind = 0;
		model->Object[i].Lighting = true;
		model->Object[i].Pos.x = 0.0f; model->Object[i].Pos.y = 0.0f; model->Object[i].Pos.z = 0.0f;
		model->Object[i].Origin.x = 0.0f; model->Object[i].Origin.y = 0.0f; model->Object[i].Origin.z = 0.0f;
		model->Object[i].Rot.x = 0.0f; model->Object[i].Rot.y = 0.0f; model->Object[i].Rot.z = 0.0f;
		model->Object[i].Scale.x = 1.0f; model->Object[i].Scale.y = 1.0f; model->Object[i].Scale.z = 1.0f;
		// BULLET
		model->Object[i].Mass = 0.0f;
		model->Object[i].isGround = 0;
		model->Object[i].ShapeType = 0;
		model->Object[i].Collision = 0;
		model->Object[i].CollidedWith = 0;
		model->Object[i].vmax = 300.0f;
	}
	
	// Variables temporales de lectura de caras
	int tmp_v0, tmp_v1, tmp_v2, tmp_n0, tmp_n1, tmp_n2, tmp_t0, tmp_t1, tmp_t2;
	nobj = noObjects;
	
	// Segunda lectura
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		switch(line[0]){
			case '#': break;
			case 'g':	// Grupos de materiales (ahora soportados :D )
				break;
			case 'o': nobj ++; break;
			case 'u':	// usemtl
				model->Object[nobj-1].NGroups ++;
				break;
			case 'v':
				switch(line[1]){
					case ' ': sscanf(line, "v %f %f %f", &vtx[nvtx*3], &vtx[(nvtx*3)+1], &vtx[(nvtx*3)+2]); nvtx ++; break;
					case 'n': sscanf(line, "vn %f %f %f", &nrm[nnrm*3], &nrm[(nnrm*3)+1], &nrm[(nnrm*3)+2]); nnrm ++; break;
					case 't': 
						sscanf(line, "vt %f %f", &txc[ntxc<<1], &txc[(ntxc<<1)+1]); 
						txc[(ntxc<<1)+1] = -txc[(ntxc<<1)+1];		// Invierte la V
						ntxc ++; break;
					default: break;
				} break;
			default: break;
		}
	}
	
	// Crea los grupos de materiales
	for(i=0;i<model->NObjects;i++){
		model->Object[i].Group = (AMG_ObjectGroup*) calloc (model->Object[i].NGroups, sizeof(AMG_ObjectGroup));
		for(u8 k=0;k<model->Object[i].NGroups;k++){
			model->Object[i].Group[k].Ambient = GU_RGBA(0x7F, 0x7F, 0x7F, 0xFF);
			model->Object[i].Group[k].Emmision = GU_RGBA(0, 0, 0, 0xFF);
			model->Object[i].Group[k].Diffuse = GU_RGBA(0xFF, 0xFF, 0xFF, 0xFF);
			model->Object[i].Group[k].Specular = GU_RGBA(0xFF, 0xFF, 0xFF, 0xFF);
			model->Object[i].Group[k].Texture = NULL;
			model->Object[i].Group[k].MultiTexture = NULL;
			model->Object[i].Group[k].Start = 0;
			model->Object[i].Group[k].End = 0;
			model->Object[i].Group[k].mtlname = (char*) calloc (64, sizeof(char));
		}
	}
	
	// Lee el archivo y obtén el número de caras
	nobj = noObjects;
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		switch(line[0]){
			case '#': break;
			case 'g':		// Grupos de materiales (ahora soportados :D )
				break;
			case 'o':
				nobj ++; ng = 0; break;
			case 'u':	// usemtl (guardamos el nombre del material)
				ng ++;
				sscanf(line, "usemtl %s", model->Object[nobj-1].Group[ng-1].mtlname);
				if(ng > 1){
					model->Object[nobj-1].Group[ng-1].Start = (model->Object[nobj-1].Group[ng-2].End);
					model->Object[nobj-1].Group[ng-1].End = (model->Object[nobj-1].Group[ng-2].End);
				} break;
			case 'f': 	// Averigua el formato de caras
				if(strstr(line, "//")){		// v//n o v//
					if(sscanf(line, "f %d//%d %d//%d %d//%d", &tmp_v0, &tmp_n0, &tmp_v1, &tmp_n1, &tmp_v2, &tmp_n2) == 6){
						faceformat = 0;
					}else{
						faceformat = 1;
					}
				}else if(sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &tmp_v0, &tmp_t0, &tmp_n0, &tmp_v1, &tmp_t1, &tmp_n1, &tmp_v2, &tmp_t2, &tmp_n2) == 9){
					faceformat = 2;
				}else if(sscanf(line, "f %d/%d/ %d/%d/ %d/%d/", &tmp_v0, &tmp_t0, &tmp_v1, &tmp_t1, &tmp_v2, &tmp_t2) == 6) faceformat = 3;
				else if(sscanf(line, "f %d/%d %d/%d %d/%d", &tmp_v0, &tmp_t0, &tmp_v1, &tmp_t1, &tmp_v2, &tmp_t2) == 6) faceformat = 4;
				else if(sscanf(line, "f %d %d %d", &tmp_v0, &tmp_v1, &tmp_v2) == 3) faceformat = 5;
				else AMG_Error((char*)"Incorrect face format / Formato de caras incorrecto",(char*) path);
				model->Object[nobj-1].Group[ng-1].End ++;
				break;
			default: break;
		}
	}
	
	// Crea buffers de vertices finales
	for(i=0;i<model->NObjects;i++){
		model->Object[i].NFaces = model->Object[i].Group[model->Object[i].NGroups-1].End;
		model->Object[i].Triangles = memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_V));	// Sombra para Cel-Shading

		switch(faceformat){
			case 0:			// vertices y normales
				model->Object[i].Flags |= GU_NORMAL_8BIT;
				model->Object[i].Data = (AMG_Vertex_NV*) memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_NV));
				model->Object[i].TriangleSize = sizeof(AMG_Vertex_NV)*3;
				break;
			case 1: case 5: // vertices
				model->Object[i].Data = (AMG_Vertex_V*) memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_V));
				model->Object[i].TriangleSize = sizeof(AMG_Vertex_V)*3;
				break;
			case 2: // vertices, texcoords y normales
				model->Object[i].Flags |= (GU_TEXTURE_32BITF | GU_NORMAL_8BIT);
				model->Object[i].Data = (AMG_Vertex_TNV*) memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_TNV));
				model->Object[i].TriangleSize = sizeof(AMG_Vertex_TNV)*3;
				break;
			case 3: case 4:	// vertices y texcoords
				model->Object[i].Flags |= GU_TEXTURE_32BITF;
				model->Object[i].Data = (AMG_Vertex_TV*) memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_TV));
				model->Object[i].TriangleSize = sizeof(AMG_Vertex_TV)*3;
				break;
			default: break;
		}
		if (css && css>0)
			model->Object[i].OutLine = (AMG_Vertex_V*) memalign (16, model->Object[i].NFaces*3*sizeof(AMG_Vertex_V));
		
		
		// Crea el buffer temporal de caras
		model->Object[i].face = NULL;
		model->Object[i].face = (AMG_FaceOBJ*) calloc (model->Object[i].NFaces, sizeof(AMG_FaceOBJ));
		if((model->Object[i].Data == NULL) || (model->Object[i].Triangles == NULL) || (model->Object[i].face == NULL)){
			AMG_Error((char*)"Out of RAM / RAM llena",(char*)path);
		}
	}
	
	// Tercera lectura, leer las caras
	nobj = noObjects;
	fseek(f, 0, SEEK_SET);
	while(!feof(f)){
		memset(line, 0, 128);
		fgets(line, 128, f);
		if(line[0] == 'o'){
			nobj ++; face_idx = 0;
		}else if(line[0] == 'f' && line[1] == ' '){
			// Segun el formato de caras...
			switch(faceformat){
				case 0: sscanf(line, "f %d//%d %d//%d %d//%d", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].n[0], 
															   &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].n[1], 
															   &model->Object[nobj-1].face[face_idx].v[2], &model->Object[nobj-1].face[face_idx].n[2]); break;
				case 1: sscanf(line, "f %d// %d// %d//", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].v[2]); break;
				case 2: sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].t[0], &model->Object[nobj-1].face[face_idx].n[0],
																	 &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].t[1], &model->Object[nobj-1].face[face_idx].n[1],
																	 &model->Object[nobj-1].face[face_idx].v[2], &model->Object[nobj-1].face[face_idx].t[2], &model->Object[nobj-1].face[face_idx].n[2]);
																	 break;
				case 3: sscanf(line, "f %d/%d/ %d/%d/ %d/%d/", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].t[0], 
															   &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].t[1], 
															   &model->Object[nobj-1].face[face_idx].v[2], &model->Object[nobj-1].face[face_idx].t[2]); break;
				case 4: sscanf(line, "f %d/%d %d/%d %d/%d", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].t[0], 
															   &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].t[1], 
															   &model->Object[nobj-1].face[face_idx].v[2], &model->Object[nobj-1].face[face_idx].t[2]); break;
				case 5: sscanf(line, "f %d %d %d", &model->Object[nobj-1].face[face_idx].v[0], &model->Object[nobj-1].face[face_idx].v[1], &model->Object[nobj-1].face[face_idx].v[2]);
															   break;
				default: break;
			}
			// Resta 1 a cada indice de la cara
			for(i=0;i<3;i++){
				model->Object[nobj-1].face[face_idx].v[i] --;
				model->Object[nobj-1].face[face_idx].t[i] --;
				model->Object[nobj-1].face[face_idx].n[i] --;
			}
			// Suma el indice de caras para este objeto
			face_idx ++;
		}
	}

	// Compila el modelo para AMGLib
	for(i=0;i<model->NObjects;i++){
		for(face_idx=0;face_idx<model->Object[i].NFaces;face_idx++){
			AMG_Vertex_V *s = (AMG_Vertex_V*) model->Object[i].Triangles;

			switch(faceformat){
				case 0:					// vertices y normales
					cnv = (AMG_Vertex_NV*) model->Object[i].Data;
					for(j=0;j<3;j++){
						s[(face_idx*3)+j].x = cnv[(face_idx*3)+j].x = vtx[model->Object[i].face[face_idx].v[j] * 3];
						s[(face_idx*3)+j].y = cnv[(face_idx*3)+j].y = vtx[(model->Object[i].face[face_idx].v[j] * 3)+1];
						s[(face_idx*3)+j].z = cnv[(face_idx*3)+j].z = vtx[(model->Object[i].face[face_idx].v[j] * 3)+2];
						cnv[(face_idx*3)+j].nx = (nrm[model->Object[i].face[face_idx].n[j] * 3])*127;
						cnv[(face_idx*3)+j].ny = (nrm[(model->Object[i].face[face_idx].n[j] * 3)+1])*127;
						cnv[(face_idx*3)+j].nz = (nrm[(model->Object[i].face[face_idx].n[j] * 3)+2])*127;
					} break;
				case 1: case 5:			// vertices
					cv = (AMG_Vertex_V*) model->Object[i].Data;
					for(j=0;j<3;j++){
						cv[(face_idx*3)+j].x = vtx[model->Object[i].face[face_idx].v[j] * 3];
						cv[(face_idx*3)+j].y = vtx[(model->Object[i].face[face_idx].v[j] * 3)+1];
						cv[(face_idx*3)+j].z = vtx[(model->Object[i].face[face_idx].v[j] * 3)+2];
					}
					memcpy(s, cv, model->Object[i].NFaces*3*sizeof(AMG_Vertex_V));				
					break;
				case 2:					// todo
					tcnv = (AMG_Vertex_TNV*) model->Object[i].Data;
					for(j=0;j<3;j++){
						s[(face_idx*3)+j].x = tcnv[(face_idx*3)+j].x = vtx[model->Object[i].face[face_idx].v[j] * 3];
						s[(face_idx*3)+j].y = tcnv[(face_idx*3)+j].y = vtx[(model->Object[i].face[face_idx].v[j] * 3)+1];
						s[(face_idx*3)+j].z = tcnv[(face_idx*3)+j].z = vtx[(model->Object[i].face[face_idx].v[j] * 3)+2];
						tcnv[(face_idx*3)+j].nx = nrm[model->Object[i].face[face_idx].n[j] * 3]*127;
						tcnv[(face_idx*3)+j].ny = nrm[(model->Object[i].face[face_idx].n[j] * 3)+1]*127;
						tcnv[(face_idx*3)+j].nz = nrm[(model->Object[i].face[face_idx].n[j] * 3)+2]*127;
						tcnv[(face_idx*3)+j].u = txc[(model->Object[i].face[face_idx].t[j] << 1)];
						tcnv[(face_idx*3)+j].v = txc[(model->Object[i].face[face_idx].t[j] << 1)+1];
						if (css && css>0){ ///cambio toma vertices y escala usando normales
							model->Object[i].OutLine[(face_idx*3)+j].x = s[(face_idx*3)+j].x + nrm[model->Object[i].face[face_idx].n[j] * 3] * model->Object[i].CelShadingScale;
							model->Object[i].OutLine[(face_idx*3)+j].y = s[(face_idx*3)+j].y + nrm[(model->Object[i].face[face_idx].n[j] * 3)+1] * model->Object[i].CelShadingScale;
							model->Object[i].OutLine[(face_idx*3)+j].z = s[(face_idx*3)+j].z + nrm[(model->Object[i].face[face_idx].n[j] * 3)+2] * model->Object[i].CelShadingScale;
						}
					} break;
				case 3: case 4:			// vertices y texcoords
					tcv = (AMG_Vertex_TV*) model->Object[i].Data;
					for(j=0;j<3;j++){
						s[(face_idx*3)+j].x = tcv[(face_idx*3)+j].x = vtx[model->Object[i].face[face_idx].v[j] * 3];
						s[(face_idx*3)+j].y = tcv[(face_idx*3)+j].y = vtx[(model->Object[i].face[face_idx].v[j] * 3)+1];
						s[(face_idx*3)+j].z = tcv[(face_idx*3)+j].z = vtx[(model->Object[i].face[face_idx].v[j] * 3)+2];
						tcv[(face_idx*3)+j].u = txc[(model->Object[i].face[face_idx].t[j] << 1)];
						tcv[(face_idx*3)+j].v = txc[(model->Object[i].face[face_idx].t[j] << 1)+1];					
					}
					break;
				default: break;
			}
		}
	}
	
	// Abre el archivo MTL
	fclose(f); f = NULL;
	f = fopen(mtlpath, "rb");
	if(f == NULL) AMG_Error((char*)"File not found / Archivo no encontrado", mtlpath);
	
	// Leelo linea a linea
	float clr[3];
	u8 first;
	while(!feof(f)){
		fgets(line, 128, f);
		switch(line[0]){
			case '#': break;
			case 'n':	// newmtl
				sscanf(line, "newmtl %s", straux);
				for(i=0;i<model->NObjects;i++) for(u8 k=0;k<model->Object[i].NGroups;k++) model->Object[i].Group[k].sel = 0;
				// Busca los grupos que tengan ese material...
				for(i=0;i<model->NObjects;i++){
					for(u8 k=0;k<model->Object[i].NGroups;k++){
						if(strcmp(straux, model->Object[i].Group[k].mtlname) == 0){
							model->Object[i].Group[k].sel = 1;
						}
					}
				} break;
			case 'm':	// map_Kd
				sscanf(line, "map_Kd %s", straux);
				first = 1;
				for(i=0;i<model->NObjects;i++){
					for(u8 k=0;k<model->Object[i].NGroups;k++){
						if(model->Object[i].Group[k].sel){
							if(first){
								sprintf(model->Object[i].Group[k].mtlname, "%s%s", getdir(path), straux);
								if(!model->Object[i].Group[k].Texture){
									model->Object[i].Group[k].Texture = AMG_LoadTexture(model->Object[i].Group[k].mtlname, AMG_TEX_RAM,psm);
								}
								first = 0;
							}else{
								// Busca la primera textura
								for(u8 i0=0;i0<model->NObjects;i0++){
									for(u8 i1=0;i1<model->Object[i0].NGroups;i1++){
										if(model->Object[i0].Group[i1].sel){
											model->Object[i].Group[k].Texture = model->Object[i0].Group[i1].Texture;
											i1 = model->Object[i0].NGroups; i0 = model->NObjects;	// Para salir del bucle doble...
										}
									}
								}
							}
						}
					}
				} break;
			case 'K':
				switch(line[1]){
					case 'd':		// Kd
						sscanf(line, "Kd %f %f %f", &clr[0], &clr[1], &clr[2]);
						for(i=0;i<model->NObjects;i++){
							for(u8 k=0;k<model->Object[i].NGroups;k++){
								if(model->Object[i].Group[k].sel) model->Object[i].Group[k].Diffuse = GU_COLOR(clr[0], clr[1], clr[2], 1.0f);
							}
						} break;
					case 'a':		// Ka
						sscanf(line, "Ka %f %f %f", &clr[0], &clr[1], &clr[2]);
						for(i=0;i<model->NObjects;i++){
							for(u8 k=0;k<model->Object[i].NGroups;k++){
								if(model->Object[i].Group[k].sel) model->Object[i].Group[k].Ambient = GU_COLOR(clr[0], clr[1], clr[2], 1.0f);
							}
						} break;
					case 's':		// Ks
						sscanf(line, "Ks %f %f %f", &clr[0], &clr[1], &clr[2]);
						for(i=0;i<model->NObjects;i++){
							for(u8 k=0;k<model->Object[i].NGroups;k++){
								if(model->Object[i].Group[k].sel) model->Object[i].Group[k].Specular = GU_COLOR(clr[0], clr[1], clr[2], 1.0f);
							}
						} break;
					default: break;
				} break;
			case 'd':
				sscanf(line, "d %f", &clr[0]);
				for(i=0;i<model->NObjects;i++){
					for(u8 k=0;k<model->Object[i].NGroups;k++){
						if(model->Object[i].Group[k].sel){
							model->Object[i].Group[k].Diffuse &= 0x00FFFFFF;
							model->Object[i].Group[k].Diffuse |= ((u8)(clr[0] * 255.0f) << 24);
						}
					}
				} break;
			default: break;
		}
	}
	
	// Cierra el archivo MTL
	fclose(f); f = NULL;
	model->FaceFormat = faceformat;
	
	// Calcula las bounding boxes
	u8 k;
	for(i=0;i<model->NObjects;i++){
		// Calcula la bounding box
		model->Object[i].BBox[0].x = vtx[(model->Object[i].face[0].v[0]*3)];
		model->Object[i].BBox[0].y = vtx[(model->Object[i].face[0].v[0]*3)+1];
		model->Object[i].BBox[0].z = vtx[(model->Object[i].face[0].v[0]*3)+2];
		model->Object[i].BBox[1].x = model->Object[i].BBox[0].x;
		model->Object[i].BBox[1].y = model->Object[i].BBox[0].y;
		model->Object[i].BBox[1].z = model->Object[i].BBox[0].z;
		for(j=1;j<model->Object[i].NFaces;j++){
			for(k=0;k<3;k++){
//##### esto lo podemos usar para calcular el tamaño del borde por defecto (parece que calcula el tamaño del objeto)
				if(model->Object[i].BBox[0].x >= vtx[(model->Object[i].face[j].v[k]*3)]) model->Object[i].BBox[0].x = vtx[(model->Object[i].face[j].v[k]*3)];	// XMIN
				if(model->Object[i].BBox[0].y >= vtx[(model->Object[i].face[j].v[k]*3)+1]) model->Object[i].BBox[0].y = vtx[(model->Object[i].face[j].v[k]*3)+1];	// YMIN
				if(model->Object[i].BBox[0].z >= vtx[(model->Object[i].face[j].v[k]*3)+2]) model->Object[i].BBox[0].z = vtx[(model->Object[i].face[j].v[k]*3)+2];	// ZMIN
				if(model->Object[i].BBox[1].x <= vtx[(model->Object[i].face[j].v[k]*3)]) model->Object[i].BBox[1].x = vtx[(model->Object[i].face[j].v[k]*3)];	// XMAX
				if(model->Object[i].BBox[1].y <= vtx[(model->Object[i].face[j].v[k]*3)+1]) model->Object[i].BBox[1].y = vtx[(model->Object[i].face[j].v[k]*3)+1];	// YMAX
				if(model->Object[i].BBox[1].z <= vtx[(model->Object[i].face[j].v[k]*3)+2]) model->Object[i].BBox[1].z = vtx[(model->Object[i].face[j].v[k]*3)+2];	// ZMAX
			}
		}
	}
	
	//AMG_MessageBox(AMG_MESSAGE_STRING, 0, 0, "Gotcha!");
	/*f = fopen("log.txt", "wb");
	fprintf(f, "FaceFormat: %d\n", (int)model->FaceFormat);
	fprintf(f, "NObjects: %d noObjects: %d\n", (int)model->NObjects, (int)noObjects);
	fprintf(f, "NV: %d, NT: %d, NN: %d\n", (int)nvtx, (int)ntxc, (int)nnrm);
	for(u16 i=0;i<model->NObjects;i++){
		fprintf(f, "\nObject[%d]\n\n", (int)i);
		fprintf(f, "NFaces: %d\nNGroups: %d\nTriangleSize: %d bytes\n", (int)model->Object[i].NFaces, (int)model->Object[i].NGroups, (int)model->Object[i].TriangleSize);
		fprintf(f, "Data: %p\n", model->Object[i].Data);
		for(u8 k=0;k<model->Object[i].NGroups;k++){
			fprintf(f, "\n\tGroup[%d]\n\n\t", (int)k);
			fprintf(f, "%d to %d\n", (int)model->Object[i].Group[k].Start, (int)model->Object[i].Group[k].End); 
		}
	}
	fclose(f); f = NULL;*/
	
	// Libera TODOS los datos temporales usados
//##### añade outl
	tcnv = NULL; tcv = NULL; cv = NULL; cnv =  NULL;
	free(line); line = NULL; free(mtlpath); mtlpath = NULL; free(straux); straux = NULL;
	free(vtx); vtx = NULL;
	if(ntxc) {free(txc); txc = NULL;}
	if(nnrm) {free(nrm); nrm = NULL;}
	for(i=0;i<model->NObjects;i++){
		free(model->Object[i].face); model->Object[i].face = NULL;
		for(u8 k=0;k<model->Object[i].NGroups;k++){
			free(model->Object[i].Group[k].mtlname); model->Object[i].Group[k].mtlname = NULL;
		}
	}
	
	// Devuelve el modelo creado
	return model;
//Disabled, if error we already exited the game
//error:
	// Libera datos del modelo
	if(f) fclose(f);
	if(model){
		if(model->Object){
			for(i=0;i<model->NObjects;i++){
				if(model->Object[i].face) free(model->Object[i].face);
				if(model->Object[i].Data) free(model->Object[i].Data);
				if(model->Object[i].Triangles) free(model->Object[i].Triangles);
//##### borra tambien el outline si hay error (tambien habra que meter esto en la funcion de borrar objeto)
				if(model->Object[i].OutLine) free(model->Object[i].OutLine);
				if(model->Object[i].BBox) free(model->Object[i].BBox);
				if(model->Object[i].tBBox) free(model->Object[i].tBBox);
				for(u8 k=0;k<model->Object[i].NGroups;k++){
					if(model->Object[i].Group[k].Texture){
						AMG_UnloadTexture(model->Object[i].Group[k].Texture);
						free(model->Object[i].Group[k].Texture);
					}
					if(model->Object[i].Group[k].mtlname) free(model->Object[i].Group[k].mtlname);
				}
			}
			free(model->Object);
		}
		free(model);
	}
	// Libera buffers temporales
	if(vtx) free(vtx);
	if(txc) free(txc);
	if(nrm) free(nrm);
	if(straux) free(straux);
	if(line) free(line);
	if(mtlpath) free(mtlpath);
	// Devuelve NULL
	return NULL;
}

M3D_Model *M3D_LoadModel(const char *path, float css, u32 psm){
	AMG_Model *model = AMG_LoadModel(path,css,psm);
	return (M3D_Model*) model;
}

M3D_Model *M3D_ModelClone(M3D_Model *m){
	if (!m)return 0;
	AMG_Model *cmodel = (AMG_Model*)m;
	//Create another struct
	AMG_Model *model = (AMG_Model*) calloc(sizeof(AMG_Model),1);
	//Copy data, including Object pointers
	memcpy(model,cmodel,sizeof(AMG_Model));
	//Replace Object pointers with new ones
	model->Object = (AMG_Object*) calloc(sizeof(AMG_Object),model->NObjects);	
	int i = 0;
	//Copy data, including data pointers (textures, models)
	for(i = 0;i<model->NObjects;i++){				
		memcpy(&model->Object[i],&cmodel->Object[i],sizeof(AMG_Object));
		//Disable physics in case the original model had them
		model->Object[i].bullet_id = 0;
		model->Object[i].Physics = 0;
	}
	return (M3D_Model*)model;
}

//All pointers are cloned, the only 
M3D_Model *M3D_ModelClone(const char *path){
	// Crea el modelo 3D
	AMG_Model *model = NULL;
	model = (AMG_Model*) calloc (1, sizeof(AMG_Model));
	return (M3D_Model*)model;
}

M3D_Model *M3D_ModelArray(int number){
	M3D_Model *models = (M3D_Model*) calloc(number, sizeof(M3D_Model));
	return models;
}

void AMG_RenderModel(AMG_Model *model, int transparent){
	if(model == NULL) return;
	if (transparent <0 || transparent >7) transparent=0;
	for(u8 i=0;i<model->NObjects;i++){
		AMG_RenderObject(&model->Object[i], model->CelShading, transparent);
	}
}

void M3D_ModelRender(M3D_Model *model, int transparent){
	AMG_RenderModel((AMG_Model*)model,transparent);
}

// Renderiza un objeto 3D
void AMG_RenderObject(AMG_Object *model, u8 cs, int transparent){
	if (transparent <0 && transparent >3) transparent=0;
	// Comprueba si es NULL
	if(model == NULL) return;
	// Control de la iluminación
	u8 l2 = 0;

	
	if(!model->Lighting){
		l2 = sceGuGetStatus(GU_LIGHTING);
		sceGuDisable(GU_LIGHTING);
	}
	// Aplica las transformaciones necesarias	
	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);

	//If no bullet, just apply position and rotation defined by user
	if (model->Physics == 0){
		AMG_Translate(GU_MODEL, &model->Pos);
		//AMG_Translate(GU_MODEL, &model->Origin);
		AMG_Rotate(GU_MODEL, &model->Rot);
	}
	//If has bullet physics, apply position and rotation from bullet physics
	else AMG_UpdateBody(model);
	AMG_Scale(GU_MODEL, &model->Scale);
	//model->Origin.x = -model->Origin.x; model->Origin.y = -model->Origin.y; model->Origin.z = -model->Origin.z;
	//AMG_Translate(GU_MODEL, &model->Origin);
	//model->Origin.x = -model->Origin.x; model->Origin.y = -model->Origin.y; model->Origin.z = -model->Origin.z;

	AMG_UpdateMatrices();	// Actualiza las matrices
	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
	sceGuSpecular(AMG.WorldSpecular);								// Define el valor especular del mundo 3D

  if (!skip){
	// Dibuja cada grupo de este objeto
	for(u8 i=0;i<model->NGroups;i++){
		// Define los materiales
		sceGuMaterial(GU_DIFFUSE, model->Group[i].Diffuse); 
		sceGuMaterial(GU_SPECULAR, model->Group[i].Specular); 
		sceGuMaterial(GU_AMBIENT, model->Group[i].Ambient);
		sceGuColor(model->Group[i].Diffuse); sceGuAmbient(model->Group[i].Ambient);
		if(model->Group[i].Texture != NULL){
			if(model->Group[i].Texture->isVideo) {
				model->Group[i].Texture->TCC = GU_TCC_RGB;
			}
			AMG_EnableTexture(model->Group[i].Texture);	// Activa la textura
		}else{
			sceGuDisable(GU_TEXTURE_2D);
		}
		u16 nfaces = (model->Group[i].End - model->Group[i].Start);

		//////////////////////////////////////////////////////////////////////////////////////
		//begin object will only draw the object if it is inside viewport, 
		//Do not use for big models
		//sceGuBeginObject(GU_VERTEX_32BITF,nfaces*3,0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize])); 
		switch(transparent){
			case 0:
				//dibuja las caras que miran a la camara
				sceGuEnable(GU_CULL_FACE);
					sceGuFrontFace(GU_CCW);
					sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
			break;
			case 1:
				//sceGuDisable(GU_DEPTH_TEST);
				sceGuDepthMask(1);//Disable Z buffer writes
				sceGuEnable(GU_CULL_FACE);
				//dibuja las caras que no miran a la camara
					sceGuFrontFace(GU_CW);
					sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
				//dibuja las caras que miran a la camara
					sceGuFrontFace(GU_CCW);
					sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
				sceGuDepthMask(0);//Enable Z buffer writes
				//sceGuEnable(GU_DEPTH_TEST);
			break;
			case 2:
				sceGuDisable(GU_CULL_FACE);
				sceGuDepthMask(1);//Disable Z buffer writes
				sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
				sceGuDepthMask(0);//Enable Z buffer writes
			break;
			case 3: //solo para renderizar objectos reflejados
				sceGuStencilFunc(GU_EQUAL,0xFF, 1);
				sceGuStencilOp(GU_KEEP, GU_KEEP, GU_KEEP);
				sceGuEnable(GU_CULL_FACE);
				sceGuFrontFace(GU_CW);
				sceGuColor(0x66FFFFFF);
				sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0,(void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));	
			break;
			case 4: //solo para renderizar el objeto que hace de espejo
				sceGuDepthMask(1);//Disable Z buffer writes
				sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0,(void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));					
				sceGuDepthMask(0);//Enable Z buffer writes
			break;
		}
		if(model->Group[i].Texture != NULL){
			if(model->Group[i].Texture->isVideo) {
				model->Group[i].Texture->TCC = GU_TCC_RGBA;
			}
		}
		
		// Dibuja la parte MultiTextura
		
		if((model->Group[i].MultiTexture != NULL) && (model->Group[i].Texture != NULL)){
			AMG_EnableTexture(model->Group[i].MultiTexture);
			// Dibuja solo las que miran hacia la cámara
			sceGuEnable(GU_CULL_FACE);
			//sceGuFrontFace(GU_CW);
			sceGuFrontFace(GU_CCW);
			sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
			AMG.DrawnVertices += (nfaces*3);
		}
		
		//sceGuEndObject();
		
		//sceGuBeginObject(GU_VERTEX_32BITF,nfaces*3,0,model->OutLine); 
		// Dibuja la parte Cel-Shading (el outline)
		if(model->OutLine){
			AMG_Light[2].Pos.x = AMG_Light[3].Pos.y = 1.0f;		// Configura la matriz del envmap
			sceGuDisable(GU_TEXTURE_2D);
			l = sceGuGetStatus(GU_LIGHTING);
			sceGuDisable(GU_LIGHTING);
			sceGuEnable(GU_CULL_FACE);
	        sceGuFrontFace(GU_CW);
			sceGuColor(model->OutlineColor);
			AMG_UpdateMatrices();
				sceGuDrawArray(GU_TRIANGLES, (GU_VERTEX_32BITF | GU_TRANSFORM_3D), nfaces*3, 0, model->OutLine);
			sceGuDisable(GU_CULL_FACE);
			if(l == GU_TRUE) sceGuEnable(GU_LIGHTING);
			AMG.DrawnVertices += (nfaces*3);
			sceGuColor(GU_RGBA(255, 255, 255, 0xFF));
		}
	
		//Draw object Silhouette when it is behind other objects
		if (model->DrawBehind){
			sceGuEnable(GU_COLOR_LOGIC_OP);
			sceGuLogicalOp(GU_CLEAR);//Set color
			sceGuDisable(GU_TEXTURE_2D);
			sceGuDisable(GU_LIGHTING);
			sceGuDepthFunc(GU_LESS);
			sceGuDrawArray(Render_Style, model->Flags, nfaces*3, 0, (void*)&(((u8*)model->Data)[model->Group[i].Start*model->TriangleSize]));
			sceGuDisable(GU_COLOR_LOGIC_OP);
			sceGuDepthFunc(GU_GEQUAL);//restore depth function
		}
		//sceGuEndObject();
	}
  }
	// Actualiza el número de vértices dibujados
	AMG.DrawnVertices += (model->NFaces*3);
	//Actualiza la Bounding Box
	model->tBBox[0].x = model->BBox[0].x + model->Pos.x;
	model->tBBox[0].y = model->BBox[0].y + model->Pos.y;
	model->tBBox[0].z = model->BBox[0].z + model->Pos.z;
	model->tBBox[1].x = model->BBox[1].x + model->Pos.x;
	model->tBBox[1].y = model->BBox[1].y + model->Pos.y;
	model->tBBox[1].z = model->BBox[1].z + model->Pos.z;
	model->Centre.x = (model->BBox[0].x + model->BBox[1].x)/2.0f;
	model->Centre.y = (model->BBox[0].y + model->BBox[1].y)/2.0f;
	model->Centre.z = (model->BBox[0].z + model->BBox[1].z)/2.0f;
	model->tCentre.x = model->Centre.x + model->Pos.x;
	model->tCentre.y = model->Centre.y + model->Pos.y;
	model->tCentre.z = model->Centre.z + model->Pos.z;
	///////
	
	sceGuDisable(GU_CULL_FACE);

    AMG_PopMatrix(GU_MODEL);
	// Control de la iluminación
	if((!model->Lighting) && (l2)) sceGuEnable(GU_LIGHTING);
	sceGuColor(0xffffffff);
}

//Model Transformations
//Values of x y z = 0 won't change that parameter
void M3D_ModelSetPosition(M3D_Model* m,int obj,float x, float y, float z){
	AMG_Model *model = (AMG_Model*)m;
	if (x) model->Object[obj].Pos.x = x;
	if (y) model->Object[obj].Pos.y = y;
	if (z) model->Object[obj].Pos.z = z;
}

ScePspFVector3 M3D_ModelGetPosition(M3D_Model* m,int obj){
	AMG_Model *model = (AMG_Model*)m;
	ScePspFVector3 pos;
	pos.x = model->Object[obj].Pos.x;
	pos.y = model->Object[obj].Pos.y;
	pos.z = model->Object[obj].Pos.z;
	return pos;
}

void M3D_ModelCopyPosition(M3D_Model* m0, int obj0, AMG_Model* m1,int obj1){
	AMG_Model *model0 = (AMG_Model*)m0;
	AMG_Model *model1 = (AMG_Model*)m1;
	model0->Object[obj0].Pos = model1->Object[obj1].Pos; 
}

//Values of x y z = 0 won't change that parameter
void M3D_ModelSetRotation(M3D_Model* m,int obj,float x, float y, float z){
	AMG_Model *model = (AMG_Model*)m;
	if (x) model->Object[obj].Rot.x = M3D_Deg2Rad(x);
	if (y) model->Object[obj].Rot.y = M3D_Deg2Rad(y);
	if (z) model->Object[obj].Rot.z = M3D_Deg2Rad(z);
}

void M3D_ModelTranslate(M3D_Model* m,int obj,float x, float y, float z){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].Pos.x += x;
	model->Object[obj].Pos.y += y;
	model->Object[obj].Pos.z += z;
}

void M3D_ModelRotate(M3D_Model* m,int obj,float x, float y, float z){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].Rot.x += M3D_Deg2Rad(x);
	model->Object[obj].Rot.y += M3D_Deg2Rad(y);
	model->Object[obj].Rot.z += M3D_Deg2Rad(z);
}

void M3D_ModelSetScale(M3D_Model* m,int obj,float x, float y, float z){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].Scale = (ScePspFVector3) {x,y,z};
}

void M3D_ModelResetPosition(M3D_Model* m,int obj){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].Pos = (ScePspFVector3) {0,0,0};
}

void M3D_ModelResetRotation(M3D_Model* m,int obj){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].Rot = (ScePspFVector3) {0,0,0};
}

void M3D_ModelScrollTexture(M3D_Model* m,int obj_number,int group_number,float x, float y){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj_number].Group[group_number].Texture->U+=x;
	model->Object[obj_number].Group[group_number].Texture->V+=y;
}

void M3D_ModelSetLighting(M3D_Model* m,int obj_number, u8 light){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj_number].Lighting = light;
}

void M3D_ModelSetOcclusion(M3D_Model* m,int obj,int value){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj].DrawBehind = value;
}

void M3D_ModelBINSetPosition(M3D_ModelBIN* m,float x, float y, float z){
	AMG_BinaryMesh *model = (AMG_BinaryMesh *)m;
	model->Pos.x = x;
	model->Pos.y = y;
	model->Pos.z = z;
}

void M3D_ModelBINSetRotation(M3D_ModelBIN* m,float rx, float ry, float rz){
	AMG_BinaryMesh *model = (AMG_BinaryMesh *)m;
	model->Rot.x = M3D_Deg2Rad(rx);
	model->Rot.y = M3D_Deg2Rad(ry);
	model->Rot.z = M3D_Deg2Rad(rz);
}

void M3D_ModelBINTranslate(M3D_ModelBIN* m,float dx, float dy, float dz){
	AMG_BinaryMesh *model = (AMG_BinaryMesh *)m;
	model->Pos.x+=dx;
	model->Pos.y+=dy;
	model->Pos.z+=dz;
}

void M3D_ModelBINRotate(M3D_ModelBIN* m,float drx, float dry, float drz){
	AMG_BinaryMesh *model = (AMG_BinaryMesh *)m;
	model->Rot.x+=M3D_Deg2Rad(drx);
	model->Rot.y+=M3D_Deg2Rad(dry);
	model->Rot.z+=M3D_Deg2Rad(drz);
}

void M3D_ModelBINScrollTexture(M3D_ModelBIN* m,float du, float dv){
	AMG_BinaryMesh *model = (AMG_BinaryMesh *)m;
	model->Object[0].Texture->U+=du;
	model->Object[0].Texture->V+=dv;
}


//
void AMG_TransferObjectVram(AMG_Object *model){
	u16 nfaces = (model->Group[0].End - model->Group[0].Start);
	u32 size = nfaces*model->TriangleSize;
	u8 *vram_ptr = NULL;

	if ((vram_ptr = (u8*)oslVramMgrAllocBlock(size)) == NULL) return;
	memcpy(vram_ptr,model->Data,size);
	free(model->Data); model->Data = (void*)vram_ptr;
}

// Elimina un objeto 3D
void AMG_UnloadObject(AMG_Object *model){
	// Libera el objeto
	if(model == NULL) return;
	if(model->Data != NULL)  {free(model->Data); model->Data = NULL;  }
	if(model->BBox != NULL)  {free(model->BBox); model->BBox = NULL;  }
	if(model->tBBox != NULL) {free(model->tBBox); model->tBBox = NULL;}
	if(model->Triangles) {free(model->Triangles); model->Triangles = NULL;}
	if(model->OutLine) {free(model->OutLine); model->OutLine = NULL;}
	// Libera los grupos de materiales
	for(u8 i=0;i<model->NGroups;i++){
		if(model->Group[i].Texture != NULL){
			AMG_UnloadTexture(model->Group[i].Texture);
			free(model->Group[i].Texture); model->Group[i].Texture = NULL;
		}
		if(model->Group[i].MultiTexture != NULL){
			AMG_UnloadTexture(model->Group[i].MultiTexture);
			free(model->Group[i].MultiTexture); model->Group[i].MultiTexture = NULL;
		}
		if(model->Group[i].NormalMap != NULL){
			free(model->Group[i].NormalMap); model->Group[i].NormalMap = NULL;
		}
	}
	free(model->Group); model->Group = NULL;
}

// Elimina un modelo 3D
void AMG_UnloadModel(AMG_Model *model){
	if(model == NULL) return;
	u16 i;
	for(i=0;i<model->NObjects;i++){
		AMG_UnloadObject(&model->Object[i]);
	}
	model->FaceFormat = 0; model->NObjects = 0; model->CelShading = 0;
	free(model->Object); model->Object = NULL;
} 





void AMG_RenderVehicle(AMG_Model *model,AMG_Model *wheel){
	int i;
	
	//draw chasis
	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);

	AMG_UpdateVehicleBody(model);
	AMG_UpdateMatrices();
	if(!skip){
		sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
		sceGuSpecular(AMG.WorldSpecular);
		sceGuMaterial(GU_DIFFUSE, model->Object[0].Group[0].Diffuse); 
		sceGuMaterial(GU_SPECULAR, model->Object[0].Group[0].Specular); 
		sceGuMaterial(GU_AMBIENT, model->Object[0].Group[0].Ambient);
		sceGuColor(model->Object[0].Group[0].Diffuse); sceGuAmbient(model->Object[0].Group[0].Ambient);
		if(model->Object[0].Group[0].Texture != NULL){
			AMG_EnableTexture(model->Object[0].Group[0].Texture);	// Activa la textura
		}else{
			sceGuDisable(GU_TEXTURE_2D);
		}
		u16 nface = (model->Object[0].Group[0].End - model->Object[0].Group[0].Start);
		sceGuDrawArray(GU_TRIANGLES, model->Object[0].Flags, nface*3, 0, (void*)&(((u8*)model->Object[0].Data)[model->Object[0].Group[0].Start*model->Object[0].TriangleSize]));
	}
	AMG_PopMatrix(GU_MODEL);
	
	if(!skip){
		//set up wheels material
		sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
		sceGuSpecular(AMG.WorldSpecular);
		sceGuMaterial(GU_DIFFUSE, wheel->Object[0].Group[0].Diffuse); 
		sceGuMaterial(GU_SPECULAR, wheel->Object[0].Group[0].Specular); 
		sceGuMaterial(GU_AMBIENT, wheel->Object[0].Group[0].Ambient);
		sceGuColor(wheel->Object[0].Group[0].Diffuse); sceGuAmbient(wheel->Object[0].Group[0].Ambient);
		if(wheel->Object[0].Group[0].Texture != NULL){
			AMG_EnableTexture(wheel->Object[0].Group[0].Texture);	// Activa la textura
		}else{
			sceGuDisable(GU_TEXTURE_2D);
		}
	}
	//Draw wheels
	for (i=0;i!=4;i++){
		AMG_PushMatrix(GU_MODEL);
		AMG_LoadIdentity(GU_MODEL);
		AMG_UpdateVehicleWheel(model,i);
		AMG_UpdateMatrices();
		if(!skip){
			u16 nface = (wheel->Object[0].Group[0].End - wheel->Object[0].Group[0].Start);
			sceGuDrawArray(GU_TRIANGLES, wheel->Object[0].Flags, nface*3, 0, (void*)&(((u8*)wheel->Object[0].Data)[wheel->Object[0].Group[0].Start*wheel->Object[0].TriangleSize]));
		}
		AMG_PopMatrix(GU_MODEL);
	}
}

void M3D_VehicleRender(M3D_Model *model,M3D_Model *wheel){
	AMG_RenderVehicle((AMG_Model*)model,(AMG_Model*)wheel);
}

// Comprueba la colision entre 2 objetos con Bounding Box
u8 AMG_CheckBBoxCollision(const AMG_Object *obj1, const AMG_Object *obj2){
	if((obj1 == NULL) || (obj2 == NULL)) return 0;
	if(((obj1->tBBox[0].x) <= (obj2->tBBox[1].x))  // Borde derecho obj1 > Borde izquierdo obj2
	 && ((obj1->tBBox[1].x) >= (obj2->tBBox[0].x))  // Borde izquierdo obj1 < Borde derecho obj2
	 && ((obj1->tBBox[0].y) <= (obj2->tBBox[1].y))  // Borde inferior obj1 < Borde superior obj2
	 && ((obj1->tBBox[1].y) >= (obj2->tBBox[0].y))  // Borde superior obj1 > Borde inferior obj2
	 && ((obj1->tBBox[0].z) <= (obj2->tBBox[1].z))  
	 && ((obj1->tBBox[1].z) >= (obj2->tBBox[0].z))) return 1;
	return 0;
}

// Normaliza un modelo 3D
void AMG_NormalizeModel(AMG_Model *model){
	if(model == NULL) return;
	u8 i; u32 j;
	ScePspFVector3 vec;
	AMG_Vertex_TNV *tcnv; AMG_Vertex_NV *cnv; AMG_Vertex_V *cv; AMG_Vertex_TV *tcv;
	// Procesa cada vertice
	for(i=0;i<model->NObjects;i++){
		// Normaliza las caras
		switch(model->FaceFormat){
			case 0:				// v+n
				cnv = (AMG_Vertex_NV*) model->Object[i].Data;
				for(j=0;j<(model->Object[i].NFaces*3);j++){
					vec.x = cnv[j].x; vec.y = cnv[j].y; vec.z = cnv[j].z;
					AMG_Normalize(&vec);
					cnv[j].nx = vec.x; cnv[j].ny = vec.y; cnv[j].nz = vec.z;
				}
				break;
			case 2:				// v+n+t
				tcnv = (AMG_Vertex_TNV*) model->Object[i].Data;
				for(j=0;j<(model->Object[i].NFaces*3);j++){
					vec.x = tcnv[j].x; vec.y = tcnv[j].y; vec.z = tcnv[j].z;
					AMG_Normalize(&vec);
					tcnv[j].nx = vec.x; tcnv[j].ny = vec.y; tcnv[j].nz = vec.z;
				}
				break;
			case 1: case 5:		// v
				// Transforma a CNV
				cnv = (AMG_Vertex_NV*) calloc (model->Object[i].NFaces*3, sizeof(AMG_Vertex_NV));
				cv = (AMG_Vertex_V*) model->Object[i].Data;
				model->FaceFormat = 1;
				for(j=0;j<(model->Object[i].NFaces*3);j++){
					vec.x = cv[j].x; vec.y = cv[j].y; vec.z = cv[j].z;
					AMG_Normalize(&vec);
					cnv[j].nx = vec.x; cnv[j].ny = vec.y; cnv[j].nz = vec.z;
					cnv[j].x = cv[j].x; cnv[j].y = cv[j].y; cnv[j].z = cv[j].z;
				}
				model->Object[i].Flags = (GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D);
				free(model->Object[i].Data); model->Object[i].Data = (void*) cnv;
				break;
			case 3: case 4:		// v+t
				// Transforma a TCNV
				tcnv = (AMG_Vertex_TNV*) calloc (model->Object[i].NFaces*3, sizeof(AMG_Vertex_TNV));
				tcv = (AMG_Vertex_TV*) model->Object[i].Data;
				model->FaceFormat = 3;
				for(j=0;j<(model->Object[i].NFaces*3);j++){
					vec.x = tcv[j].x; vec.y = tcv[j].y; vec.z = tcv[j].z;
					AMG_Normalize(&vec);
					tcnv[j].nx = vec.x; tcnv[j].ny = vec.y; tcnv[j].nz = vec.z;
					tcnv[j].x = tcv[j].x; tcnv[j].y = tcv[j].y; tcnv[j].z = tcv[j].z;
				}
				model->Object[i].Flags = (GU_TEXTURE_32BITF | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_3D);
				free(model->Object[i].Data); model->Object[i].Data = (void*) tcnv;
				break;
			default: return;
		}
	}
}

// Renderiza un objeto (mirror)
void AMG_RenderMirrorObject(AMG_Model *model, u8 number, u8 axis){
    AMG_Object *obj = &model->Object[number];
	// Cuidado con la iluminacion
	if(obj == NULL) return;
	u8 l = sceGuGetStatus(GU_LIGHTING);
	sceGuDisable(GU_LIGHTING);
	// Guarda valores temporales
	ScePspFVector3 tmp_pos, tmp_scl, tmp_rot;
	tmp_pos.x = obj->Pos.x; tmp_pos.y = obj->Pos.y; tmp_pos.z= obj->Pos.z;
	tmp_scl.x = obj->Scale.x; tmp_scl.y = obj->Scale.y; tmp_scl.z= obj->Scale.z;
	tmp_rot.x = obj->Rot.x; tmp_rot.y = obj->Rot.y; tmp_rot.z= obj->Rot.z;
	// Cambia la posición
	float d = 0.0f;		// Distancia del objeto al suelo
	switch(axis){
		case 0:			// Eje X
			d = ((obj->Origin.x + obj->Pos.x) - (amg_curfloor->Origin.x + amg_curfloor->Pos.x));
			obj->Pos.x -= (d * 2.0f);
			obj->Scale.x = -obj->Scale.x;
			obj->Rot.y = -obj->Rot.z;
			obj->Rot.z = -obj->Rot.z;
			break;
		case 1:			// Eje Y
			d = ((obj->Origin.y + obj->Pos.y) - (amg_curfloor->Origin.y + amg_curfloor->Pos.y));
			obj->Pos.y -= (d * 2.0f);
			obj->Scale.y = -obj->Scale.y;
			obj->Rot.x = -obj->Rot.x;
			obj->Rot.z = -obj->Rot.z;
			break;
		case 2:			// Eje Z
			d = ((obj->Origin.z + obj->Pos.z) - (amg_curfloor->Origin.z + amg_curfloor->Pos.z));
			obj->Pos.z -= (d * 2.0f);
			obj->Scale.z = -obj->Scale.z;
			obj->Rot.x = -obj->Rot.x;
			obj->Rot.y = -obj->Rot.y;
			break;
		default: return;
	}
	// Renderiza el objeto
	AMG_RenderObject(obj, 0, 3);
	obj->Pos = tmp_pos;
	obj->Scale = tmp_scl; 
	obj->Rot = tmp_rot; 
	if(l) sceGuEnable(GU_LIGHTING);
}

void M3D_ModelRenderMirror(M3D_Model *model, u8 number, u8 axis){
	AMG_RenderMirrorObject((AMG_Model*)model,number,axis);
}

// Comienza el motor de reflejos
void AMG_StartReflection(AMG_Model *model, u8 number){
	AMG_Object *obj = &model->Object[number];
	sceGuEnable(GU_STENCIL_TEST);
	sceGuClear(GU_STENCIL_BUFFER_BIT);
	sceGuStencilFunc(GU_ALWAYS,0xFF, 1);
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_REPLACE);
	
	AMG_RenderObject(obj, 0, 4);

	amg_curfloor = obj;
}

void M3D_StartReflection(M3D_Model *model, u8 number){
	AMG_StartReflection((AMG_Model*) model,number);
}

// Termina el motor de reflejos
void AMG_FinishReflection(){
	sceGuDisable(GU_STENCIL_TEST);
	sceGuClear(GU_STENCIL_BUFFER_BIT);
	//sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_MAX ,GU_SRC_COLOR, GU_FIX, 0x22222222,0);
	AMG_RenderObject(amg_curfloor, 0, 0);
	sceGuBlendFunc(GU_ADD , GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	//sceGuDisable(GU_BLEND);
}

void M3D_FinishReflection(){
	sceGuDisable(GU_STENCIL_TEST);
	sceGuClear(GU_STENCIL_BUFFER_BIT);
	//sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD ,GU_SRC_COLOR, GU_ONE_MINUS_SRC_COLOR, 0,0x66FFFFFF);
	AMG_RenderObject(amg_curfloor, 0, 0);
	sceGuBlendFunc(GU_ADD , GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	//sceGuDisable(GU_BLEND);
}


void AMG_SetModelTexture(AMG_Model *model, int obj_number, int group_number, AMG_Texture *tex){
	AMG_Texture *tex0 = model->Object[obj_number].Group[group_number].Texture;
	if(tex0){
		if (tex == tex0) return;
		else AMG_UnloadTexture(tex0);
	}
	model->Object[obj_number].Group[group_number].Texture = tex;
}

void M3D_ModelSetTexture(M3D_Model *m, int obj_number, int group_number, M3D_Texture *t){
	AMG_Model *model = (AMG_Model*)m;
	AMG_Texture *tex = (AMG_Texture*)t;
	AMG_SetModelTexture(model,obj_number,group_number,tex);
}

void AMG_SetModelMultiTexture(AMG_Model *model, int obj_number, int group_number, AMG_Texture *tex){
	AMG_Texture *tex0 = model->Object[obj_number].Group[group_number].MultiTexture;
	if(tex0){
		if (tex == tex0) return;
		else AMG_UnloadTexture(tex0);
	}
	tex->TFX = GU_TFX_MODULATE;
	model->Object[obj_number].Group[group_number].MultiTexture = tex;
}

void M3D_ModelSetMultiTexture(M3D_Model *model, int obj_number, int group_number, M3D_Texture *tex){
	AMG_SetModelMultiTexture((AMG_Model*)model,obj_number,group_number,(AMG_Texture*)tex);
}

void AMG_SetModelTextureMapping(AMG_Model *model, int obj_number, int group_number, int ntex, u32 mapping){
	if (ntex == 0) {
		model->Object[obj_number].Group[group_number].Texture->Mapping = mapping;
		model->Object[obj_number].Group[group_number].Texture->MappingLights[0] = 1;
		model->Object[obj_number].Group[group_number].Texture->MappingLights[1] = 2;
	}
	if (ntex == 1) {
		model->Object[obj_number].Group[group_number].MultiTexture->Mapping = mapping;
		model->Object[obj_number].Group[group_number].Texture->MappingLights[0] = 1;
		model->Object[obj_number].Group[group_number].Texture->MappingLights[1] = 2;
	}
}

void M3D_ModelSetTextureMapping(M3D_Model *model, int obj_number, int group_number, int ntex, u32 mapping){
	AMG_SetModelTextureMapping((AMG_Model*)model,obj_number,group_number,ntex,mapping);
}

void AMG_SetModelTextureFilter(AMG_Model *model, int obj_number, int group_number, int ntex, int filter){
	if (ntex == 0) {
		if (filter == 0){
			model->Object[obj_number].Group[group_number].Texture->Filter = GU_NEAREST; 
			model->Object[obj_number].Group[group_number].Texture->MipFilter = GU_NEAREST_MIPMAP_LINEAR;
		}
		if (filter == 1){
			model->Object[obj_number].Group[group_number].Texture->Filter = GU_LINEAR;
			model->Object[obj_number].Group[group_number].Texture->MipFilter = GU_LINEAR_MIPMAP_LINEAR;
		}
	}
	if (ntex == 1) {
		if (filter == 0){
			model->Object[obj_number].Group[group_number].MultiTexture->Filter = GU_NEAREST; 
			model->Object[obj_number].Group[group_number].MultiTexture->MipFilter = GU_NEAREST_MIPMAP_LINEAR;
		}
		if (filter == 1){
			model->Object[obj_number].Group[group_number].MultiTexture->Filter = GU_LINEAR;
			model->Object[obj_number].Group[group_number].MultiTexture->MipFilter = GU_LINEAR_MIPMAP_LINEAR;
		}		
	}
}

void M3D_ModelSetTextureFilter(M3D_Model *model, int obj_number, int group_number, int ntex, int filter){
	AMG_SetModelTextureFilter((AMG_Model*)model,obj_number,group_number,ntex,filter);
}

void M3D_ModelTexture3D_Animate(M3D_Model *model, int obj_number, int group_number, u8 xframes, u8 yframes, u8 *anim, float speed){
	AMG_Model *m = (AMG_Model*)model;
	AMG_Texture *tex = m->Object[obj_number].Group[group_number].Texture;
	int i;
	if(tex == NULL) return;
	tex->NFrames = xframes*yframes;
	tex->Frame = 0;
	tex->Speed = speed;
	tex->Tile_count_x = xframes;
	tex->Tile_count_y = yframes;
	memset(tex->Anim,2,64);
	if (anim[0]){
		i = anim[0];
		if (i < 64) memcpy(tex->Anim,anim,i+1);
	}
}

void AMG_DrawSkyBox(AMG_Model *model,float fov){
	float perspective_mtx[4*4] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	ScePspFMatrix4 __attribute__((aligned(16))) m;
	AMG_GetMatrix(GU_VIEW,&m);
	gumFastInverse(&m,&m);
	ScePspFVector3 __attribute__((aligned(16))) p = (ScePspFVector3) {m.w.x,m.w.y,m.w.z};
	
	if(model->Object[0].Group[0].Texture != NULL){
		if(model->Object[0].Group[0].Texture->isVideo) {
			model->Object[0].Group[0].Texture->TCC = GU_TCC_RGB;
		}
		AMG_EnableTexture(model->Object[0].Group[0].Texture);	// Activa la textura
	}else{
		sceGuDisable(GU_TEXTURE_2D);
	}
	u16 nfaces = (model->Object[0].Group[0].End - model->Object[0].Group[0].Start);
	AMG_Push_Perspective_Matrix(AMG.ScreenWidth,AMG.ScreenHeight,fov);
	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);
	AMG_Translate(GU_MODEL, &p);
	AMG_UpdateMatrices();	// Actualiza las matrices
	
	u8 l2 = sceGuGetStatus(GU_LIGHTING);
	sceGuDisable(GU_LIGHTING);
	
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(1);//Disable Z buffer writes
	sceGuDisable(GU_CULL_FACE); //Draw all faces
	//AMG_RenderModel(model,0);
	sceGuColor(0xffffffff);
	sceGuDrawArray(Render_Style, model->Object[0].Flags, nfaces*3, 0, (void*)&(((u8*)model->Object[0].Data)[model->Object[0].Group[0].Start*model->Object[0].TriangleSize]));
	sceGuEnable(GU_CULL_FACE);
	sceGuDepthMask(0);//Enable Z buffer writes
	sceGuEnable(GU_DEPTH_TEST);
	//sceGuClear(GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);
	
	AMG_PopMatrix(GU_MODEL);
	
	AMG_Pop_Perspective_Matrix();
	
	if(l2) sceGuEnable(GU_LIGHTING);
}

void M3D_DrawSkyBox(M3D_Model *model,float fov){
	AMG_DrawSkyBox((AMG_Model*)model,fov);
}


//VEHICLE CAM
ScePspQuatMatrix __attribute__((aligned(16))) M3D_Follow_Camera_Q[60] = {0};
int M3D_veiclecam_pos_r = 0;
int M3D_veiclecam_pos_w = 8;

void M3D_VehicleSetCam(M3D_Model *model, float y, float z, float rot){
	AMG_Model *mo = (AMG_Model*)model;
	ScePspQuatMatrix __attribute__((aligned(16))) Camera_Interpolated;
	ScePspFMatrix4 __attribute__((aligned(16))) m;
	ScePspFVector3 __attribute__((aligned(16))) v = (ScePspFVector3) {0,y,z};
	ScePspFVector3 __attribute__((aligned(16))) r = (ScePspFVector3) {-rot,0,0};
	AMG_LoadIdentityUser(&m);

	AMG_TranslateUser(&m,&mo->Object[0].Pos);

	memcpy(&M3D_Follow_Camera_Q[M3D_veiclecam_pos_w++],&mo->Object[0].VehicleRotation,sizeof(ScePspQuatMatrix));
	if (M3D_veiclecam_pos_w==59)M3D_veiclecam_pos_w = 0;
	memcpy(&Camera_Interpolated,&M3D_Follow_Camera_Q[M3D_veiclecam_pos_r++],sizeof(ScePspQuatMatrix));
	if (M3D_veiclecam_pos_r==59)M3D_veiclecam_pos_r = 0;

	AMG_RotateQuatUser(&Camera_Interpolated,&m);
	
	AMG_RotateUser(&m,&r);
	AMG_TranslateUser(&m,&v);
	gumFastInverse(&m,&m);
	AMG_SetMatrix(GU_VIEW,&m);
}



//***************
//SHADOW ENGINE
/////////////////

extern int AMG_FogState;
extern AMG_Vertex_NV Volumetric_Shadow[];

void M3D_ModelCastShadow(M3D_Model *cast, int obj, int alpha, int type, int light){    
	AMG_Model *c = (AMG_Model *)cast;
	//Prepare to Draw Shadows  
	sceGuEnable(GU_CULL_FACE);
    sceGuDisable(GU_TEXTURE_2D);
	sceGuDepthMask(GU_TRUE);
	
	float sizex = fabs(c->Object->BBox[0].x - c->Object->BBox[1].x);
	float sizez = fabs(c->Object->BBox[0].z - c->Object->BBox[1].z);
	float sizey = fabs(c->Object->BBox[0].y - c->Object->BBox[1].y);
	float s = (sizex+sizey+sizez)/2.8;
	ScePspFVector3 __attribute__((aligned(16))) size = (ScePspFVector3) {s,s,s};

	//Look at
	ScePspFMatrix4 __attribute__((aligned(16))) mtx_shadow;
	ScePspFVector3 __attribute__((aligned(16))) mtx_from = (ScePspFVector3){c->Object[0].Pos.x,c->Object[0].Pos.y,c->Object[0].Pos.z};
	ScePspFVector3 __attribute__((aligned(16))) mtx_rot = (ScePspFVector3){1.5708,0,0};
	ScePspFVector3 __attribute__((aligned(16))) mtx_to = (ScePspFVector3) {
		c->Object[0].Pos.x+AMG_Light[light].Pos.x,
		c->Object[0].Pos.y+AMG_Light[light].Pos.y,
		c->Object[0].Pos.z+AMG_Light[light].Pos.z,
	};
	ScePspFVector3 __attribute__((aligned(16))) mtx_up = (ScePspFVector3) {0,1,0};
	
	if (type == 0) {
		AMG_PushMatrix(GU_MODEL);
		AMG_LoadIdentity(GU_MODEL);
		AMG_Translate(GU_MODEL,&c->Object[0].Pos);
		AMG_Rotate(GU_MODEL,&mtx_rot);
		AMG_Scale(GU_MODEL, &size);
	}
	if (type == 1) {
		AMG_LoadIdentityUser(&mtx_shadow);
		gumLookAt(&mtx_shadow,&mtx_from,&mtx_to,&mtx_up);
		gumFastInverse(&mtx_shadow,&mtx_shadow);
		AMG_ScaleUser(&mtx_shadow, &size);
		AMG_PushMatrix(GU_MODEL);
		AMG_SetMatrix(GU_MODEL,&mtx_shadow);
	}

	AMG_UpdateMatrices();
	sceGuDisable(GU_LIGHTING);	
	if (!skip){
		sceGuEnable(GU_STENCIL_TEST);
		
		//1 pass
		sceGuEnable(GU_COLOR_LOGIC_OP);
		sceGuStencilFunc(GU_ALWAYS,0xFF, 0);//draw always (so 0 does not matter). Mask writes (set stencil to 0xFF)
		sceGuStencilOp(0,GU_REPLACE,0);//stencil wont fail nor zfail (0,0), so replace with FF
		sceGuLogicalOp(GU_AND);//And color (keep the color)
		sceGuColor(0xFFFFFFFF);
		sceGuFrontFace(GU_CW);
		sceGuDrawArray(GU_TRIANGLES,GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,384, 0, Volumetric_Shadow);
		sceGuDisable(GU_COLOR_LOGIC_OP);

		//2 pass
		sceGuStencilFunc(GU_EQUAL,0xFF, 0xFF);//draw if stencil is 0xFF, do not mask writes (0xff)
		sceGuStencilOp(GU_KEEP, GU_KEEP, GU_KEEP); // keep the stencil buffer unchanged
		sceGuColor(GU_RGBA(0,0,0,alpha));
		sceGuFrontFace(GU_CCW);
		sceGuDrawArray(GU_TRIANGLES,GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,384, 0, Volumetric_Shadow);
		
		sceGuDisable(GU_STENCIL_TEST);
	}

	AMG_PopMatrix(GU_MODEL);

	sceGuClear(GU_STENCIL_BUFFER_BIT);//GU_DEPTH_BUFFER_BIT

	//Restore drawing to normal
	sceGuDisable(GU_STENCIL_TEST); // Stencil test
	sceGuDepthMask(GU_FALSE);
	sceGuDisable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_LIGHTING);
}



//Real shadow
/////////////
//This is a border made on a triangle strip, to delete texture borders very fast
u16 AMG_Shadow_Fix[30] = {
	0,0,0,
	4,4,0,
	256,0,0,
	252,4,0,
	256,256,0,
	252,252,0,
	0,256,0,
	4,252,0,
	0,0,0,
	4,4,0
};	
ScePspFMatrix4 textureProjScaleTrans = {
	{0.5,0,0,0},
	{0,-0.5,0,0},
	{0,0,1,0},
	{0.5,0.5,0,0},
};
ScePspFMatrix4 AMG_ShadowlightView;
ScePspFMatrix4 AMG_ShadowlightMatrix;
ScePspFMatrix4 AMG_ShadowlightProjection;
float AMG_ShadowSize = 16;
AMG_Texture *AMG_ShadowMap = NULL;
void *AMG_ShadowReceiver;
int AMG_ShadowReceiverType = 0;
u8 AMG_ShadowReceiver_alpha = 255;

void AMG_Set_Shadowprojection(int psize,float size){
	if (psize < 32) psize = 32;
	if (psize > 256) psize = 256;
	AMG_ShadowSize = size;
	//This makes the perspective nearly ortographyc
	gumLoadIdentity(&AMG_ShadowlightProjection);
	gumPerspective(&AMG_ShadowlightProjection,1.0f,1,0.5f,2048.0f);
	if (psize > 256) AMG_Error((char*)"Shadowprojection > 256",(char*)"AMG_Set_Shadowprojection");
	AMG_ShadowMap = AMG_CreateTexture(psize,psize,GU_PSM_4444,AMG_TEX_VRAM);
	oslVramMgrAllocBlock(psize*(272-psize));
	if(AMG_ShadowMap->Load != AMG_TEX_VRAM) AMG_Error((char*)"NO VRAM",(char*)"AMG_ShadowMap");
	//Scale the shadow border according to shadow size
	int i;
	for (i = 0; i<30;i++) AMG_Shadow_Fix[i]/=(256/psize);
}

void M3D_ShadowprojectionSet(int psize,float size){
	AMG_Set_Shadowprojection(psize,size);
}

void AMG_StartShadowReceiver(void *receiver, ScePspFVector3 *lpos, u8 alpha, u8 light){
	if (AMG_ShadowMap == NULL) return;
	
	if (AMG_FogState) sceGuDisable(GU_FOG);
	
	//Draw receiver
	AMG_ShadowReceiver = receiver;
	AMG_ShadowReceiver_alpha = alpha;
	
	//Now... c'mon... do math...
	//store current camera view and projection
	AMG_PushMatrix(GU_PROJECTION);
	AMG_PushMatrix(GU_VIEW);
	
	//Enable render target
	AMG_EnableRenderToTexture(AMG_ShadowMap);
	
	//AMG_RenderObject(receiver,0, 0);
	//sceGuClearColor(0x00000000);
	//sceGuClearStencil(0);//clearing stencil 
	//sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT|GU_STENCIL_BUFFER_BIT);
	
	// setup view/projection from light
	ScePspFVector3 __attribute__((aligned(16))) mtx_from = (ScePspFVector3){
		lpos->x+(AMG_Light[light].Pos.x*AMG_ShadowSize*32),
		lpos->y+(AMG_Light[light].Pos.y*AMG_ShadowSize*32),
		lpos->z+(AMG_Light[light].Pos.z*AMG_ShadowSize*32)
	};
	ScePspFVector3 __attribute__((aligned(16))) mtx_up = (ScePspFVector3) {0,1,0};
	AMG_LoadIdentityUser(&AMG_ShadowlightView);
	gumLookAt(&AMG_ShadowlightView,&mtx_from,lpos,&mtx_up);
	
	sceGuSetMatrix(GU_VIEW,&AMG_ShadowlightView);
	sceGuSetMatrix(GU_PROJECTION,&AMG_ShadowlightProjection);
	
	//Dibuja la sombra del objeto "caster" desde la luz "0" a la textura "Shadow"
	//Dibuja el objeto y verás la forma que proyecta sobre los demás objetos (sombra)
	sceGuDisable(GU_LIGHTING);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuEnable(GU_CULL_FACE);
	sceGuFrontFace(GU_CCW);
}

void M3D_ModelStartShadowReceiver(M3D_Model *receiver, int objnumber, float x, float y, float z, u8 alpha, u8 light){
	ScePspFVector3 lpos = (ScePspFVector3) {x,y,z};
	AMG_Model *r = (AMG_Model*)receiver;
	AMG_ShadowReceiverType = 0;
	AMG_StartShadowReceiver(&r->Object[objnumber],&lpos,alpha,light);
}

void M3D_ModelBINStartShadowReceiver(M3D_ModelBIN *receiver, float x, float y, float z, u8 alpha, u8 light){
	ScePspFVector3 lpos = (ScePspFVector3) {x,y,z};
	AMG_ShadowReceiverType = 1;
	AMG_StartShadowReceiver(receiver,&lpos,alpha,light);
}

void M3D_NurbsStartShadowReceiver(M3D_NurbsSurface *receiver, float x, float y, float z, u8 alpha, u8 light){
	ScePspFVector3 lpos = (ScePspFVector3) {x,y,z};
	AMG_ShadowReceiverType = 2;
	AMG_StartShadowReceiver(receiver,&lpos,alpha,light);
}

void AMG_RenderModelShadow(AMG_Object *caster){
	if (!skip){
	AMG_PushMatrix(GU_MODEL);
	AMG_Translate(GU_MODEL, &caster->Pos);
	AMG_Translate(GU_MODEL, &caster->Origin);
	AMG_Rotate(GU_MODEL, &caster->Rot);
	AMG_Scale(GU_MODEL, &caster->Scale);
	AMG_Translate(GU_MODEL, &caster->Origin);
	AMG_UpdateMatrices();
	
	sceGuColor(0xFF000000);

	sceGuEnable(GU_STENCIL_TEST); // Stencil test
	sceGuStencilFunc(GU_ALWAYS,AMG_ShadowReceiver_alpha,0); // set stencil/alpha if a pixel is rendered
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_REPLACE); // keep value on failed test (fail and zfail) and replace on pass
	
	sceGuDrawArray(GU_TRIANGLES, caster->Flags, caster->NFaces*3, 0, (void*)&(((u8*)caster->Data)[caster->Group[0].Start*caster->TriangleSize]));

	sceGuDisable(GU_STENCIL_TEST);
	
	AMG_PopMatrix(GU_MODEL);
	
	//sceGuEnable(GU_COLOR_LOGIC_OP);
	//sceGuLogicalOp(GU_INVERT);
	//sceGuDisable(GU_COLOR_LOGIC_OP);
	
	//sceGuColorFunc(GU_EQUAL,ref,mask): Passes if ( ref & mask ) = ( color & mask ).
	//sceGuEnable(GU_COLOR_TEST);
	//sceGuColorFunc(GU_NOTEQUAL,0,0xff0000ff);
	//sceGuDisable(GU_COLOR_TEST);
	}
}

void M3D_ModelRenderShadow(M3D_Model *caster, int obj_number){
	AMG_Model *r = (AMG_Model*)caster;
	AMG_RenderModelShadow(&r->Object[obj_number]);
}

void AMG_RenderVehicleShadow(AMG_Object *caster){
	if (!skip){
	AMG_PushMatrix(GU_MODEL);
	AMG_Translate(GU_MODEL, &caster->Pos);
	AMG_Translate(GU_MODEL, &caster->Origin);
	AMG_RotateQuat(GU_MODEL, &caster->VehicleRotation);
	AMG_Scale(GU_MODEL, &caster->Scale);
	AMG_Translate(GU_MODEL, &caster->Origin);
	AMG_UpdateMatrices();

	sceGuColor(0xFF000000);
	
	sceGuEnable(GU_STENCIL_TEST); // Stencil test
	sceGuStencilFunc(GU_ALWAYS,AMG_ShadowReceiver_alpha,0); // Drawn pixels will be non transparent
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_REPLACE); // keep value on failed test (fail and zfail) and replace on pass

	sceGuDrawArray(GU_TRIANGLES, caster->Flags, caster->NFaces*3, 0, (void*)&(((u8*)caster->Data)[caster->Group[0].Start*caster->TriangleSize]));
	
	sceGuDisable(GU_STENCIL_TEST);
	
	AMG_PopMatrix(GU_MODEL);
	}
}	

void M3D_VehicleRenderShadow(M3D_Model *caster, int obj_number){
	AMG_Model *r = (AMG_Model*)caster;
	AMG_RenderVehicleShadow(&r->Object[obj_number]);
}

u16 border_pixels[256*256] = {0};

void AMG_EndShadowReceiver(){
	//Delete texture edges
	sceGuEnable(GU_STENCIL_TEST); // Stencil test
	sceGuStencilFunc(GU_ALWAYS,0,0); // clear stencil/alpha if a pixel is rendered
	sceGuStencilOp(GU_REPLACE,GU_REPLACE, GU_REPLACE); // keep value on failed test (fail and zfail) and replace on pass
	sceGuDrawArray(GU_TRIANGLE_STRIP,GU_VERTEX_16BIT|GU_TRANSFORM_2D,10, 0,AMG_Shadow_Fix);
	sceGuDisable(GU_STENCIL_TEST);

	AMG_DisableRenderToTexture();

	//restore camera
	AMG_PopMatrix(GU_PROJECTION);
	AMG_PopMatrix(GU_VIEW);

	//draw receiver
	if (AMG_FogState) sceGuEnable(GU_FOG);
	sceGuEnable(GU_LIGHTING);
	if(AMG_ShadowReceiverType == 0) AMG_RenderObject((AMG_Object*)AMG_ShadowReceiver,0,0);
	if(AMG_ShadowReceiverType == 1) AMG_RenderBinaryMesh((AMG_BinaryMesh*)AMG_ShadowReceiver,0);
	if(AMG_ShadowReceiverType == 2) {
		M3D_NurbsSurface *s = (M3D_NurbsSurface*)AMG_ShadowReceiver;
		M3D_NurbsSurfaceRender(s);
	}
	sceGuDisable(GU_LIGHTING);

	//Draw receiver again but with the shadow texture on it.
    // This is the mother of the lamb			
	gumMultMatrix(&AMG_ShadowlightView, &AMG_ShadowlightProjection, &AMG_ShadowlightView);
	gumMultMatrix(&AMG_ShadowlightView, &textureProjScaleTrans, &AMG_ShadowlightView);
	
	// draw grid receiving shadow
	ScePspFMatrix4 grid_matrix;
	gumLoadIdentity(&grid_matrix);
	sceGuSetMatrix(GU_MODEL,&grid_matrix);
	// multiply shadowmap projection texture by geometry world matrix
	// since geometry coords are in object space
	gumMultMatrix(&AMG_ShadowlightView, &AMG_ShadowlightView, &grid_matrix);
	sceGuSetMatrix(GU_TEXTURE,&AMG_ShadowlightView);
	
	// setup texture 
	sceGuTexFilter(GU_LINEAR,GU_LINEAR);
	sceGuTexWrap(GU_CLAMP,GU_CLAMP);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMapMode( GU_TEXTURE_MATRIX, 0, 0 );
	sceGuTexProjMapMode( GU_POSITION );
	sceGuTexScale(16.0f, 16.0f);
	sceGuTexMode(GU_PSM_4444,0,0,0);
	
	sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);
	sceGuTexImage(0,AMG_ShadowMap->Width,AMG_ShadowMap->Height,AMG_ShadowMap->Width,AMG_ShadowMap->Data);
	AMG_UpdateMatrices();
	
	sceGuColor(0xFFFFFFFF);
	
	if(AMG_ShadowReceiverType == 0){
		AMG_Object *obj = (AMG_Object*)AMG_ShadowReceiver;
		sceGuDrawArray(GU_TRIANGLES, obj->Flags, obj->NFaces*3, 0, (void*)&(((u8*)obj->Data)[obj->Group[0].Start*obj->TriangleSize]));
	}
	if(AMG_ShadowReceiverType == 1){
		//sceGuEnable(GU_COLOR_TEST);
		//Reject any pixel which is not 0x00000000
		//sceGuColorFunc(GU_ALWAYS,0,0xffffff88);
		
		//sceGuEnable(GU_COLOR_LOGIC_OP);
		//sceGuLogicalOp(GU_SET);
		
		AMG_BinaryMesh *obj = (AMG_BinaryMesh*)AMG_ShadowReceiver;
		sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,obj->Object[0].polyCount*3,0,obj->Object[0].Data);
		//sceGuDisable(GU_COLOR_LOGIC_OP);
		//sceGuDisable(GU_COLOR_TEST);
	}
	if(AMG_ShadowReceiverType == 2){
		M3D_NurbsSurface *s = (M3D_NurbsSurface*) AMG_ShadowReceiver;
		AMG_NurbsSurface *obj = (AMG_NurbsSurface *) s->NurbsSurface;
		sceGuFrontFace(GU_CW);
		sceGumDrawSpline(GU_TEXTURE_32BITF |GU_NORMAL_8BIT|GU_VERTEX_32BITF,obj->resolution,obj->resolution,-1,1,0,obj->vertices);
		sceGuFrontFace(GU_CCW);
	}
	sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGBA);
	//sceGuDisable(GU_COLOR_TEST);
	sceGuEnable(GU_LIGHTING);
	
	if (AMG_FogState) sceGuEnable(GU_FOG);
}

void M3D_EndShadowReceiver(){
	AMG_EndShadowReceiver();
}


//TEXTURE PIXEL SHADER
//Enable Bump Texture
void M3D_ModelLoadNormalTexture(M3D_Model *m,int obj_number, int group,const char *path, u32 psm){
	AMG_Model *model = (AMG_Model*)m;
	int ncolors = 0;
	int i;
	//variables to store colors
	u8 red,green,blue,alpha;
	//get texture array size
	u32 tex_size = model->Object[obj_number].Group[group].Texture->Width*model->Object[obj_number].Group[group].Texture->Width;
	//Load Normal Texture
	if (psm == GU_PSM_T4) ncolors = 16;
	else if (psm == GU_PSM_T8) ncolors = 256;
	else return;
	model->Object[obj_number].Group[group].MultiTexture = AMG_LoadTexture(path,AMG_TEX_RAM,psm);
	if(model->Object[obj_number].Group[group].MultiTexture == NULL) return;

	//store normalized normals here
	model->Object[obj_number].Group[group].NormalMap = (ScePspFVector4*) calloc(ncolors,sizeof(ScePspFVector4));
	//get normals from palette
	for (i = 0; i <ncolors; i++){
		oslRgbaGet8888(model->Object[obj_number].Group[group].MultiTexture->clut[i], red, green, blue, alpha);
		ScePspFVector3 normal_CS = {(float)(red-128)/128,(float)(blue-128)/128,(float)(green-128)/128};
		model->Object[obj_number].Group[group].NormalMap[i] = (ScePspFVector4){normal_CS.x,normal_CS.y,normal_CS.z,0};
	}
}

void M3D_ModelSetNormalTexture(M3D_Model *mod,int obj_number, int group, M3D_Camera *cam,int l){
	AMG_Model *model = (AMG_Model*)mod;
	AMG_Camera *camera = (AMG_Camera*)cam;
	int i;
	int ncolors = 0;
	ScePspFMatrix4 __attribute__((aligned(16))) m;
	ScePspFVector3 __attribute__((aligned(16))) lvector3a = {AMG_Light[l].Pos.x, AMG_Light[l].Pos.y, AMG_Light[l].Pos.z};
	ScePspFVector3 __attribute__((aligned(16))) lvector3b;
	ScePspFVector3 __attribute__((aligned(16))) cvector3;
	ScePspFVector4 __attribute__((aligned(16))) cvector4;
	ScePspFVector4 __attribute__((aligned(16))) tvector;
	
	AMG_Normalize(&lvector3a);
	
	//Get model rotation (physics models use quaternions, so they won't update this ->rot vector)
	AMG_LoadIdentityUser(&m);
	AMG_RotateUser(&m,&model->Object[obj_number].Rot);
	gumFastInverse(&m,&m);
	
	//Rotate light vector
	lvector3b.x = m.x.x*lvector3a.x + m.y.x*lvector3a.y + m.z.x*lvector3a.z;
	lvector3b.y = m.x.y*lvector3a.x + m.y.y*lvector3a.y + m.z.y*lvector3a.z;
	lvector3b.z = m.x.z*lvector3a.x + m.y.z*lvector3a.y + m.z.z*lvector3a.z;
	ScePspFVector4 __attribute__((aligned(16))) lvector4 = {lvector3b.x,lvector3b.y,lvector3b.z,0};
	
	//get camera vector
	cvector3.x = camera->Pos.x-camera->Eye.x;
	cvector3.y = camera->Pos.y-camera->Eye.y;
    cvector3.z = camera->Pos.z-camera->Eye.z;
	AMG_Normalize(&cvector3);
	cvector4 = (ScePspFVector4) {cvector3.x,cvector3.y,cvector3.z,0};
	
	if (model->Object[obj_number].Group[group].MultiTexture->TexFormat == GU_PSM_T4) ncolors = 16;
	else if (model->Object[obj_number].Group[group].MultiTexture->TexFormat == GU_PSM_T8) ncolors = 256;
	else return;
	
	//"PIXEl SHADER"
	for (i = 0; i <ncolors; i++){
		//VFPU Calculate dot product (pixel normal * light vector)
		//This will fail on real PSP if vectors are not normalized!
		tvector = model->Object[obj_number].Group[group].NormalMap[i];
		float dot = AMG_DotProduct(&tvector,&lvector4);
		float dot1 = 0;//AMG_DotProduct(&cvector4,&lvector4);
		s16 colour = (float)dot*255;
		if (colour < 0) colour = 0;
		if (colour > 255) colour = 255;
		u8 alpha = (u8)(M3D_fabsf(colour-128));
		model->Object[obj_number].Group[group].MultiTexture->clut[i] = RGBA((u8)colour,(u8)colour,(u8)colour,alpha);
	}
	sceKernelDcacheWritebackInvalidateRange(model->Object[obj_number].Group[group].MultiTexture->clut,ncolors*sizeof(u32));
}






//3D object (cylinder) for STENCIL Shadow
AMG_Vertex_NV Volumetric_Shadow[] = {
{15, -32, -121, 0.000000, -0.225000, -0.389711},
{15, -32, -121, 0.000000, 0.000000, -0.450000},
{15, -32, -121, 0.153678, -0.153678, -0.389711},
{23, -88, -88, 0.000000, -0.225000, -0.389711},
{23, -88, -88, 0.194856, -0.337500, -0.225000},
{23, -88, -88, 0.000000, -0.389711, -0.225000},
{31, -118, -31, 0.000000, -0.450000, 0.000000},
{31, -118, -31, 0.194856, -0.337500, -0.225000},
{31, -118, -31, 0.225000, -0.389711, 0.000000},
{64, -64, -88, 0.194856, -0.337500, -0.225000},
{64, -64, -88, 0.153678, -0.153678, -0.389711},
{64, -64, -88, 0.337500, -0.194856, -0.225000},
{86, -86, -31, 0.194856, -0.337500, -0.225000},
{86, -86, -31, 0.389711, -0.225000, 0.000000},
{86, -86, -31, 0.225000, -0.389711, 0.000000},
{88, -23, -88, 0.337500, -0.194856, -0.225000},
{88, -23, -88, 0.225000, 0.000000, -0.389711},
{88, -23, -88, 0.389711, 0.000000, -0.225000},
{118, -31, -31, 0.337500, -0.194856, -0.225000},
{118, -31, -31, 0.450000, 0.000000, 0.000000},
{118, -31, -31, 0.389711, -0.225000, 0.000000},
{32, -15, -121, 0.153678, -0.153678, -0.389711},
{32, -15, -121, 0.000000, 0.000000, -0.450000},
{32, -15, -121, 0.225000, 0.000000, -0.389711},
{88, 23, -88, 0.225000, 0.000000, -0.389711},
{88, 23, -88, 0.337500, 0.194856, -0.225000},
{88, 23, -88, 0.389711, 0.000000, -0.225000},
{118, 31, -31, 0.389711, 0.000000, -0.225000},
{118, 31, -31, 0.389711, 0.225000, 0.000000},
{118, 31, -31, 0.450000, 0.000000, 0.000000},
{32, 15, -121, 0.225000, 0.000000, -0.389711},
{32, 15, -121, 0.000000, 0.000000, -0.450000},
{32, 15, -121, 0.153678, 0.153678, -0.389711},
{86, 86, -31, 0.337500, 0.194856, -0.225000},
{86, 86, -31, 0.225000, 0.389711, 0.000000},
{86, 86, -31, 0.389711, 0.225000, 0.000000},
{64, 64, -88, 0.337500, 0.194856, -0.225000},
{64, 64, -88, 0.153678, 0.153678, -0.389711},
{64, 64, -88, 0.194856, 0.337500, -0.225000},
{31, 118, -31, 0.194856, 0.337500, -0.225000},
{31, 118, -31, 0.000000, 0.450000, 0.000000},
{31, 118, -31, 0.225000, 0.389711, 0.000000},
{15, 32, -121, 0.153678, 0.153678, -0.389711},
{15, 32, -121, 0.000000, 0.000000, -0.450000},
{15, 32, -121, 0.000000, 0.225000, -0.389711},
{23, 88, -88, 0.194856, 0.337500, -0.225000},
{23, 88, -88, 0.000000, 0.225000, -0.389711},
{23, 88, -88, 0.000000, 0.389711, -0.225000},
{-31, 118, -31, 0.000000, 0.389711, -0.225000},
{-31, 118, -31, -0.225000, 0.389711, 0.000000},
{-31, 118, -31, 0.000000, 0.450000, 0.000000},
{-15, 32, -121, 0.000000, 0.225000, -0.389711},
{-15, 32, -121, 0.000000, 0.000000, -0.450000},
{-15, 32, -121, -0.153678, 0.153678, -0.389711},
{-23, 88, -88, 0.000000, 0.225000, -0.389711},
{-23, 88, -88, -0.194856, 0.337500, -0.225000},
{-23, 88, -88, 0.000000, 0.389711, -0.225000},
{-86, 86, -31, -0.194856, 0.337500, -0.225000},
{-86, 86, -31, -0.389711, 0.225000, 0.000000},
{-86, 86, -31, -0.225000, 0.389711, 0.000000},
{-64, 64, -88, -0.194856, 0.337500, -0.225000},
{-64, 64, -88, -0.153678, 0.153678, -0.389711},
{-64, 64, -88, -0.337500, 0.194856, -0.225000},
{-118, 31, -31, -0.337500, 0.194856, -0.225000},
{-118, 31, -31, -0.450000, 0.000000, 0.000000},
{-118, 31, -31, -0.389711, 0.225000, 0.000000},
{-32, 15, -121, -0.153678, 0.153678, -0.389711},
{-32, 15, -121, 0.000000, 0.000000, -0.450000},
{-32, 15, -121, -0.225000, 0.000000, -0.389711},
{-88, 23, -88, -0.337500, 0.194856, -0.225000},
{-88, 23, -88, -0.225000, 0.000000, -0.389711},
{-88, 23, -88, -0.389711, 0.000000, -0.225000},
{-32, -15, -121, -0.225000, 0.000000, -0.389711},
{-32, -15, -121, 0.000000, 0.000000, -0.450000},
{-32, -15, -121, -0.153678, -0.153678, -0.389711},
{-88, -23, -88, -0.225000, 0.000000, -0.389711},
{-88, -23, -88, -0.337500, -0.194856, -0.225000},
{-88, -23, -88, -0.389711, 0.000000, -0.225000},
{-118, -31, -31, -0.389711, 0.000000, -0.225000},
{-118, -31, -31, -0.389711, -0.225000, 0.000000},
{-118, -31, -31, -0.450000, 0.000000, 0.000000},
{-64, -64, -88, -0.337500, -0.194856, -0.225000},
{-64, -64, -88, -0.153678, -0.153678, -0.389711},
{-64, -64, -88, -0.194856, -0.337500, -0.225000},
{122, -32, 0, 0.389711, -0.225000, 0.500000},
{122, -32, 0, 0.450000, 0.000000, 0.000000},
{122, -32, 0, 0.450000, 0.000000, 0.500000},
{-86, -86, -31, -0.337500, -0.194856, -0.225000},
{-86, -86, -31, -0.225000, -0.389711, 0.000000},
{-86, -86, -31, -0.389711, -0.225000, 0.000000},
{-89, -89, 0, -0.389711, -0.225000, 0.500000},
{-89, -89, 0, -0.225000, -0.389711, 0.000000},
{-89, -89, 0, -0.225000, -0.389711, 0.500000},
{32, 122, 0, 0.225000, 0.389711, 0.500000},
{32, 122, 0, 0.000000, 0.450000, 0.000000},
{32, 122, 0, 0.000000, 0.450000, 0.500000},
{-23, -88, -88, -0.194856, -0.337500, -0.225000},
{-23, -88, -88, 0.000000, -0.225000, -0.389711},
{-23, -88, -88, 0.000000, -0.389711, -0.225000},
{89, -89, 0, 0.225000, -0.389711, 0.500000},
{89, -89, 0, 0.389711, -0.225000, 0.000000},
{89, -89, 0, 0.389711, -0.225000, 0.500000},
{-31, -118, -31, -0.194856, -0.337500, -0.225000},
{-31, -118, -31, 0.000000, -0.450000, 0.000000},
{-31, -118, -31, -0.225000, -0.389711, 0.000000},
{-15, -32, -121, -0.153678, -0.153678, -0.389711},
{-15, -32, -121, 0.000000, 0.000000, -0.450000},
{-15, -32, -121, 0.000000, -0.225000, -0.389711},
{35, -76, -94, 0.000000, -0.225000, -0.389711},
{35, -76, -94, 0.153678, -0.153678, -0.389711},
{35, -76, -94, 0.194856, -0.337500, -0.225000},
{122, 32, 0, 0.450000, 0.000000, 0.500000},
{122, 32, 0, 0.389711, 0.225000, 0.000000},
{122, 32, 0, 0.389711, 0.225000, 0.500000},
{31, -118, -31, 0.000000, -0.450000, 0.000000},
{31, -118, -31, 0.000000, -0.389711, -0.225000},
{31, -118, -31, 0.194856, -0.337500, -0.225000},
{86, -86, -31, 0.194856, -0.337500, -0.225000},
{86, -86, -31, 0.337500, -0.194856, -0.225000},
{86, -86, -31, 0.389711, -0.225000, 0.000000},
{76, -35, -94, 0.337500, -0.194856, -0.225000},
{76, -35, -94, 0.153678, -0.153678, -0.389711},
{76, -35, -94, 0.225000, 0.000000, -0.389711},
{89, 89, 0, 0.389711, 0.225000, 0.500000},
{89, 89, 0, 0.225000, 0.389711, 0.000000},
{89, 89, 0, 0.225000, 0.389711, 0.500000},
{118, -31, -31, 0.337500, -0.194856, -0.225000},
{118, -31, -31, 0.389711, 0.000000, -0.225000},
{118, -31, -31, 0.450000, 0.000000, 0.000000},
{76, 35, -94, 0.225000, 0.000000, -0.389711},
{76, 35, -94, 0.153678, 0.153678, -0.389711},
{76, 35, -94, 0.337500, 0.194856, -0.225000},
{-32, -122, 0, -0.225000, -0.389711, 0.500000},
{-32, -122, 0, 0.000000, -0.450000, 0.000000},
{-32, -122, 0, 0.000000, -0.450000, 0.500000},
{118, 31, -31, 0.389711, 0.000000, -0.225000},
{118, 31, -31, 0.337500, 0.194856, -0.225000},
{118, 31, -31, 0.389711, 0.225000, 0.000000},
{86, 86, -31, 0.337500, 0.194856, -0.225000},
{86, 86, -31, 0.194856, 0.337500, -0.225000},
{86, 86, -31, 0.225000, 0.389711, 0.000000},
{32, -122, 0, 0.000000, -0.450000, 0.500000},
{32, -122, 0, 0.225000, -0.389711, 0.000000},
{32, -122, 0, 0.225000, -0.389711, 0.500000},
{31, 118, -31, 0.194856, 0.337500, -0.225000},
{31, 118, -31, 0.000000, 0.389711, -0.225000},
{31, 118, -31, 0.000000, 0.450000, 0.000000},
{35, 76, -94, 0.194856, 0.337500, -0.225000},
{35, 76, -94, 0.153678, 0.153678, -0.389711},
{35, 76, -94, 0.000000, 0.225000, -0.389711},
{-32, 122, 0, 0.000000, 0.450000, 0.500000},
{-32, 122, 0, -0.225000, 0.389711, 0.000000},
{-32, 122, 0, -0.225000, 0.389711, 0.500000},
{-31, 118, -31, 0.000000, 0.389711, -0.225000},
{-31, 118, -31, -0.194856, 0.337500, -0.225000},
{-31, 118, -31, -0.225000, 0.389711, 0.000000},
{-35, 76, -94, 0.000000, 0.225000, -0.389711},
{-35, 76, -94, -0.153678, 0.153678, -0.389711},
{-35, 76, -94, -0.194856, 0.337500, -0.225000},
{-86, 86, -31, -0.194856, 0.337500, -0.225000},
{-86, 86, -31, -0.337500, 0.194856, -0.225000},
{-86, 86, -31, -0.389711, 0.225000, 0.000000},
{-118, 31, -31, -0.337500, 0.194856, -0.225000},
{-118, 31, -31, -0.389711, 0.000000, -0.225000},
{-118, 31, -31, -0.450000, 0.000000, 0.000000},
{-76, 35, -94, -0.337500, 0.194856, -0.225000},
{-76, 35, -94, -0.153678, 0.153678, -0.389711},
{-76, 35, -94, -0.225000, 0.000000, -0.389711},
{-89, 89, 0, -0.225000, 0.389711, 0.500000},
{-89, 89, 0, -0.389711, 0.225000, 0.000000},
{-89, 89, 0, -0.389711, 0.225000, 0.500000},
{-76, -35, -94, -0.225000, 0.000000, -0.389711},
{-76, -35, -94, -0.153678, -0.153678, -0.389711},
{-76, -35, -94, -0.337500, -0.194856, -0.225000},
{-122, -32, 0, -0.450000, 0.000000, 0.500000},
{-122, -32, 0, -0.389711, -0.225000, 0.000000},
{-122, -32, 0, -0.389711, -0.225000, 0.500000},
{-118, -31, -31, -0.389711, 0.000000, -0.225000},
{-118, -31, -31, -0.337500, -0.194856, -0.225000},
{-118, -31, -31, -0.389711, -0.225000, 0.000000},
{-86, -86, -31, -0.337500, -0.194856, -0.225000},
{-86, -86, -31, -0.194856, -0.337500, -0.225000},
{-86, -86, -31, -0.225000, -0.389711, 0.000000},
{-35, -76, -94, -0.194856, -0.337500, -0.225000},
{-35, -76, -94, -0.153678, -0.153678, -0.389711},
{-35, -76, -94, 0.000000, -0.225000, -0.389711},
{-122, 32, 0, -0.450000, 0.000000, 0.500000},
{-122, 32, 0, -0.389711, 0.225000, 0.000000},
{-122, 32, 0, -0.450000, 0.000000, 0.000000},
{-31, -118, -31, -0.194856, -0.337500, -0.225000},
{-31, -118, -31, 0.000000, -0.389711, -0.225000},
{-31, -118, -31, 0.000000, -0.450000, 0.000000},
{25, -55, 111, 0.000000, 0.000000, 1.592644},
{25, -55, 111, 0.000000, -0.130911, 1.527476},
{25, -55, 111, 0.089413, -0.089414, 1.527476},
{31, -118, 32, 0.000000, -0.450000, 0.500000},
{31, -118, 32, 0.118906, -0.205952, 1.284160},
{31, -118, 32, 0.000000, -0.237813, 1.284160},
{48, -103, 55, 0.000000, -0.130911, 1.527476},
{48, -103, 55, 0.118906, -0.205952, 1.284160},
{48, -103, 55, 0.089413, -0.089414, 1.527476},
{86, -86, 32, 0.225000, -0.389711, 0.500000},
{86, -86, 32, 0.205952, -0.118907, 1.284160},
{86, -86, 32, 0.118906, -0.205952, 1.284160},
{82, -82, 49, 0.089413, -0.089414, 1.527476},
{82, -82, 49, 0.118906, -0.205952, 1.284160},
{82, -82, 49, 0.205952, -0.118907, 1.284160},
{118, -31, 32, 0.389711, -0.225000, 0.500000},
{118, -31, 32, 0.237813, 0.000000, 1.284160},
{118, -31, 32, 0.205952, -0.118907, 1.284160},
{103, -48, 55, 0.205952, -0.118907, 1.284160},
{103, -48, 55, 0.130910, 0.000000, 1.527476},
{103, -48, 55, 0.089413, -0.089414, 1.527476},
{55, -25, 111, 0.000000, 0.000000, 1.592644},
{55, -25, 111, 0.089413, -0.089414, 1.527476},
{55, -25, 111, 0.130910, 0.000000, 1.527476},
{103, 48, 55, 0.130910, 0.000000, 1.527476},
{103, 48, 55, 0.205952, 0.118906, 1.284160},
{103, 48, 55, 0.089413, 0.089413, 1.527476},
{55, 25, 111, 0.000000, 0.000000, 1.592644},
{55, 25, 111, 0.130910, 0.000000, 1.527476},
{55, 25, 111, 0.089413, 0.089413, 1.527476},
{118, 31, 32, 0.450000, 0.000000, 0.500000},
{118, 31, 32, 0.205952, 0.118906, 1.284160},
{118, 31, 32, 0.237813, 0.000000, 1.284160},
{82, 82, 49, 0.089413, 0.089413, 1.527476},
{82, 82, 49, 0.205952, 0.118906, 1.284160},
{82, 82, 49, 0.118906, 0.205952, 1.284160},
{86, 86, 32, 0.389711, 0.225000, 0.500000},
{86, 86, 32, 0.118906, 0.205952, 1.284160},
{86, 86, 32, 0.205952, 0.118906, 1.284160},
{48, 103, 55, 0.118906, 0.205952, 1.284160},
{48, 103, 55, 0.000000, 0.130910, 1.527476},
{48, 103, 55, 0.089413, 0.089413, 1.527476},
{25, 55, 111, 0.000000, 0.000000, 1.592644},
{25, 55, 111, 0.089413, 0.089413, 1.527476},
{25, 55, 111, 0.000000, 0.130910, 1.527476},
{31, 118, 32, 0.225000, 0.389711, 0.500000},
{31, 118, 32, 0.000000, 0.237813, 1.284160},
{31, 118, 32, 0.118906, 0.205952, 1.284160},
{-48, 103, 55, 0.000000, 0.130910, 1.527476},
{-48, 103, 55, -0.118907, 0.205952, 1.284160},
{-48, 103, 55, -0.089413, 0.089413, 1.527476},
{-25, 55, 111, 0.000000, 0.000000, 1.592644},
{-25, 55, 111, 0.000000, 0.130910, 1.527476},
{-25, 55, 111, -0.089413, 0.089413, 1.527476},
{-31, 118, 32, 0.000000, 0.450000, 0.500000},
{-31, 118, 32, -0.118907, 0.205952, 1.284160},
{-31, 118, 32, 0.000000, 0.237813, 1.284160},
{-82, 82, 49, -0.089413, 0.089413, 1.527476},
{-82, 82, 49, -0.118907, 0.205952, 1.284160},
{-82, 82, 49, -0.205952, 0.118906, 1.284160},
{-86, 86, 32, -0.225000, 0.389711, 0.500000},
{-86, 86, 32, -0.205952, 0.118906, 1.284160},
{-86, 86, 32, -0.118907, 0.205952, 1.284160},
{-55, 25, 111, 0.000000, 0.000000, 1.592644},
{-55, 25, 111, -0.089413, 0.089413, 1.527476},
{-55, 25, 111, -0.130910, 0.000000, 1.527476},
{-118, 31, 32, -0.389711, 0.225000, 0.500000},
{-118, 31, 32, -0.237813, 0.000000, 1.284160},
{-118, 31, 32, -0.205952, 0.118906, 1.284160},
{-103, 48, 55, -0.205952, 0.118906, 1.284160},
{-103, 48, 55, -0.130910, 0.000000, 1.527476},
{-103, 48, 55, -0.089413, 0.089413, 1.527476},
{-55, -25, 111, 0.000000, 0.000000, 1.592644},
{-55, -25, 111, -0.130910, 0.000000, 1.527476},
{-55, -25, 111, -0.089413, -0.089414, 1.527476},
{-118, -31, 32, -0.450000, 0.000000, 0.500000},
{-118, -31, 32, -0.205952, -0.118907, 1.284160},
{-118, -31, 32, -0.237813, 0.000000, 1.284160},
{-103, -48, 55, -0.130910, 0.000000, 1.527476},
{-103, -48, 55, -0.205952, -0.118907, 1.284160},
{-103, -48, 55, -0.089413, -0.089414, 1.527476},
{-86, -86, 32, -0.389711, -0.225000, 0.500000},
{-86, -86, 32, -0.118906, -0.205952, 1.284160},
{-86, -86, 32, -0.205952, -0.118907, 1.284160},
{-82, -82, 49, -0.089413, -0.089414, 1.527476},
{-82, -82, 49, -0.205952, -0.118907, 1.284160},
{-82, -82, 49, -0.118906, -0.205952, 1.284160},
{-25, -55, 111, 0.000000, 0.000000, 1.592644},
{-25, -55, 111, -0.089413, -0.089414, 1.527476},
{-25, -55, 111, 0.000000, -0.130911, 1.527476},
{-31, -118, 32, -0.118906, -0.205952, 1.284160},
{-31, -118, 32, 0.000000, -0.450000, 0.500000},
{-31, -118, 32, 0.000000, -0.237813, 1.284160},
{-48, -103, 55, -0.118906, -0.205952, 1.284160},
{-48, -103, 55, 0.000000, -0.130911, 1.527476},
{-48, -103, 55, -0.089413, -0.089414, 1.527476},
{31, -118, 32, 0.000000, -0.450000, 0.500000},
{31, -118, 32, 0.225000, -0.389711, 0.500000},
{31, -118, 32, 0.118906, -0.205952, 1.284160},
{30, -112, 49, 0.000000, -0.130911, 1.527476},
{30, -112, 49, 0.000000, -0.237813, 1.284160},
{30, -112, 49, 0.118906, -0.205952, 1.284160},
{86, -86, 32, 0.225000, -0.389711, 0.500000},
{86, -86, 32, 0.389711, -0.225000, 0.500000},
{86, -86, 32, 0.205952, -0.118907, 1.284160},
{118, -31, 32, 0.389711, -0.225000, 0.500000},
{118, -31, 32, 0.450000, 0.000000, 0.500000},
{118, -31, 32, 0.237813, 0.000000, 1.284160},
{112, -30, 49, 0.205952, -0.118907, 1.284160},
{112, -30, 49, 0.237813, 0.000000, 1.284160},
{112, -30, 49, 0.130910, 0.000000, 1.527476},
{112, 30, 49, 0.130910, 0.000000, 1.527476},
{112, 30, 49, 0.237813, 0.000000, 1.284160},
{112, 30, 49, 0.205952, 0.118906, 1.284160},
{118, 31, 32, 0.450000, 0.000000, 0.500000},
{118, 31, 32, 0.389711, 0.225000, 0.500000},
{118, 31, 32, 0.205952, 0.118906, 1.284160},
{86, 86, 32, 0.389711, 0.225000, 0.500000},
{86, 86, 32, 0.225000, 0.389711, 0.500000},
{86, 86, 32, 0.118906, 0.205952, 1.284160},
{30, 112, 49, 0.118906, 0.205952, 1.284160},
{30, 112, 49, 0.000000, 0.237813, 1.284160},
{30, 112, 49, 0.000000, 0.130910, 1.527476},
{31, 118, 32, 0.225000, 0.389711, 0.500000},
{31, 118, 32, 0.000000, 0.450000, 0.500000},
{31, 118, 32, 0.000000, 0.237813, 1.284160},
{-30, 112, 49, 0.000000, 0.130910, 1.527476},
{-30, 112, 49, 0.000000, 0.237813, 1.284160},
{-30, 112, 49, -0.118907, 0.205952, 1.284160},
{-31, 118, 32, 0.000000, 0.450000, 0.500000},
{-31, 118, 32, -0.225000, 0.389711, 0.500000},
{-31, 118, 32, -0.118907, 0.205952, 1.284160},
{-86, 86, 32, -0.225000, 0.389711, 0.500000},
{-86, 86, 32, -0.389711, 0.225000, 0.500000},
{-86, 86, 32, -0.205952, 0.118906, 1.284160},
{-118, 31, 32, -0.389711, 0.225000, 0.500000},
{-118, 31, 32, -0.450000, 0.000000, 0.500000},
{-118, 31, 32, -0.237813, 0.000000, 1.284160},
{-112, 30, 49, -0.205952, 0.118906, 1.284160},
{-112, 30, 49, -0.237813, 0.000000, 1.284160},
{-112, 30, 49, -0.130910, 0.000000, 1.527476},
{-118, -31, 32, -0.450000, 0.000000, 0.500000},
{-118, -31, 32, -0.389711, -0.225000, 0.500000},
{-118, -31, 32, -0.205952, -0.118907, 1.284160},
{-112, -30, 49, -0.130910, 0.000000, 1.527476},
{-112, -30, 49, -0.237813, 0.000000, 1.284160},
{-112, -30, 49, -0.205952, -0.118907, 1.284160},
{-86, -86, 32, -0.389711, -0.225000, 0.500000},
{-86, -86, 32, -0.225000, -0.389711, 0.500000},
{-86, -86, 32, -0.118906, -0.205952, 1.284160},
{-31, -118, 32, -0.118906, -0.205952, 1.284160},
{-31, -118, 32, -0.225000, -0.389711, 0.500000},
{-31, -118, 32, 0.000000, -0.450000, 0.500000},
{-30, -112, 49, -0.118906, -0.205952, 1.284160},
{-30, -112, 49, 0.000000, -0.237813, 1.284160},
{-30, -112, 49, 0.000000, -0.130911, 1.527476},
{122, -32, 0, 0.389711, -0.225000, 0.500000},
{122, -32, 0, 0.389711, -0.225000, 0.000000},
{122, -32, 0, 0.450000, 0.000000, 0.000000},
{-89, -89, 0, -0.389711, -0.225000, 0.500000},
{-89, -89, 0, -0.389711, -0.225000, 0.000000},
{-89, -89, 0, -0.225000, -0.389711, 0.000000},
{32, 122, 0, 0.225000, 0.389711, 0.500000},
{32, 122, 0, 0.225000, 0.389711, 0.000000},
{32, 122, 0, 0.000000, 0.450000, 0.000000},
{89, -89, 0, 0.225000, -0.389711, 0.500000},
{89, -89, 0, 0.225000, -0.389711, 0.000000},
{89, -89, 0, 0.389711, -0.225000, 0.000000},
{122, 32, 0, 0.450000, 0.000000, 0.500000},
{122, 32, 0, 0.450000, 0.000000, 0.000000},
{122, 32, 0, 0.389711, 0.225000, 0.000000},
{89, 89, 0, 0.389711, 0.225000, 0.500000},
{89, 89, 0, 0.389711, 0.225000, 0.000000},
{89, 89, 0, 0.225000, 0.389711, 0.000000},
{-32, -122, 0, -0.225000, -0.389711, 0.500000},
{-32, -122, 0, -0.225000, -0.389711, 0.000000},
{-32, -122, 0, 0.000000, -0.450000, 0.000000},
{32, -122, 0, 0.000000, -0.450000, 0.500000},
{32, -122, 0, 0.000000, -0.450000, 0.000000},
{32, -122, 0, 0.225000, -0.389711, 0.000000},
{-32, 122, 0, 0.000000, 0.450000, 0.500000},
{-32, 122, 0, 0.000000, 0.450000, 0.000000},
{-32, 122, 0, -0.225000, 0.389711, 0.000000},
{-89, 89, 0, -0.225000, 0.389711, 0.500000},
{-89, 89, 0, -0.225000, 0.389711, 0.000000},
{-89, 89, 0, -0.389711, 0.225000, 0.000000},
{-122, -32, 0, -0.450000, 0.000000, 0.500000},
{-122, -32, 0, -0.450000, 0.000000, 0.000000},
{-122, -32, 0, -0.389711, -0.225000, 0.000000},
{-122, 32, 0, -0.450000, 0.000000, 0.500000},
{-122, 32, 0, -0.389711, 0.225000, 0.500000},
{-122, 32, 0, -0.389711, 0.225000, 0.000000},

};
