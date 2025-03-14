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
extern int skip;

void AMG_UpdateSkinnedActorBody(AMG_Skinned_Actor *actor);
void AMG_UpdateMorphingActorBody(AMG_Morphing_Actor *actor);
//void AMG_UpdateActorBody(AMG_Actor *actor);
ScePspFVector3 s1;		// Lo siento, no me gustan las variables locales :P
extern AMG_Object *amg_curfloor;	// El suelo actual (para reflejos y sombras)

extern u32 l;
//////////////////////

extern u8 Render_Style;//Mills 04/08/15

// Cambia el modo de renderizado 3d //Mills 04/08/15
void AMG_RenderStyle(u8 Render_M);

// Obten el directorio de un archivo
char *getdir(const char *path);

AMG_Skinned_Actor *AMG_LoadSkinnedActor(const char *path, float outline, u32 psm) {
	FILE *file;
	unsigned long Faces_offset;
	char line[128];
	int frame = 0;
	//int nbones = 0;
	//int nframes = 0;
	//int nfaces = 0;
	//int facegroups = 0;
	int group = 0;
	u32 vertex_offset = 0;
	int i = 0;
	int j = 0;
  	file = fopen(path,"r");
	if (!file) /*error*/;
	
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*) calloc (1, sizeof(AMG_Skinned_Actor));
	actor->Object = (SkinnedModel*) calloc (1, sizeof(SkinnedModel));
	actor->Object[0].bullet_id = 0;
	actor->Object[0].collisionreset = 0;
	actor->Object[0].material_name[0] = 0x00000000;
	actor->Object[0].texture = NULL;
	actor->Object[0].MultiTexture = NULL;
	actor->Object[0].Data = NULL;
	actor->Object[0].Border = NULL;
	//actor->Object[0].Frame = NULL;
	actor->Object[0].Group = NULL;
	//////////////////////////////////
	u32 header = 0; 
	fseek(file, 0, SEEK_SET);
	fread (&header,1,4,file);
	//PSP_
	if (header != 0x20505350) {AMG_Error((char*)"Wrong format / Formato incorrecto",path);}
	fread (&header,1,4,file);
	//M3A:
	if (header != 0x3A41334D) {AMG_Error((char*)"Wrong format / Formato incorrecto",path);}
	Face_Group *ngroups[32][32];
	for (i = 0; i < 32; i++){
		for (j = 0; j < 32; j++){
			ngroups[i][j] = (Face_Group*) calloc(1,sizeof(Face_Group));
			ngroups[i][j]->group_state = -1; 
			ngroups[i][j]->vertex_offset = 0;
		}
	}
	fseek(file, 0, SEEK_SET);
	fgets(line, 128, file);
	fgets(line, 128, file);
	fgets(line, 128, file);

	//LET'S READ BONE STRUCTURE
	sscanf(line,"NBONES %i",&actor->Object[0].n_bones);
	for (i = 0; i < actor->Object[0].n_bones; i++){
		int index,parent;
		char b[128];
		fgets(line, 128, file);
		if (line[0] != 'b') /*error*/;
		//b Bone.001 index 1 parent 0
		sscanf(line,"b %s index %i parent %i",b,&index,&parent);
		actor->Object[0].Frame[0].Bone[index].parent = parent;
	}

	fgets(line, 128, file);
	//LET'S READ BONE FRAMES (POS ROT)
	sscanf(line,"NFRAMES %i",&actor->Object[0].frameCount);
	if (line[0] != 'N') /*error*/;
	for (frame = 0; frame < actor->Object->frameCount; frame++){
		int bone = 0;
		fgets(line, 128, file);//skip "f\n"
		for(bone = 0;bone<actor->Object[0].n_bones;bone++){
			fgets(line, 128, file);
			sscanf(line," %f %f %f %f %f %f %f",
				&actor->Object[0].Frame[frame].Bone[bone].position.x,
				&actor->Object[0].Frame[frame].Bone[bone].position.y,
				&actor->Object[0].Frame[frame].Bone[bone].position.z,
				&actor->Object[0].Frame[frame].Bone[bone].orient.x,
				&actor->Object[0].Frame[frame].Bone[bone].orient.y,
				&actor->Object[0].Frame[frame].Bone[bone].orient.z,
				&actor->Object[0].Frame[frame].Bone[bone].orient.w);
		}
	}

	fgets(line, 128, file);
	
	//COUNT FACES
	sscanf(line,"NFACES %i",&actor->Object[0].polyCount);
	actor->Object[0].Data = (AMG_Vertex_W2TNV*) malloc(sizeof(AMG_Vertex_W2TNV)*3*actor->Object->polyCount);
	
	//Get faces offset in file
	Faces_offset = ftell(file);
	
	//CREATE FACE GROUPS
	for (i = 0; i < actor->Object[0].polyCount; i++){
		int g[3];
		int min = 32; int max = 0;
		fgets(line, 128, file); if (line[0] != 't') {/*error*/;return 0;}
		fgets(line, 128, file); sscanf(line," %i",&g[0]);
		fgets(line, 128, file); sscanf(line," %i",&g[1]);
		fgets(line, 128, file); sscanf(line," %i",&g[2]);

		//What group is this face in?
		for (j=0;j<3;j++){ if (min > g[j]) min = g[j]; if (max < g[j]) max = g[j];}
		//is this group empty? create group
		if (ngroups[min][max]->group_state == -1){
			ngroups[min][max]->min_bone = min;
			ngroups[min][max]->max_bone = max;
			if (min == max) ngroups[min][max]->group_state = 1;
			else ngroups[min][max]->group_state = 2;
			ngroups[min][max]->nfaces++;
			actor->Object[0].n_groups++;
		} 
		//group already exists, add new face
		else ngroups[min][max]->nfaces++;
	}
	
	//ALLOCATE GROUP DATA
	actor->Object[0].Group = (AMG_SkinGroup*) malloc (sizeof(AMG_SkinGroup)*actor->Object[0].n_groups);
	for (i = 0; i < 32; i++){
		for (j = 0; j < 32; j++){
			int gs = ngroups[i][j]->group_state;
			if (gs > 0){
				ngroups[i][j]->data = &actor->Object[0].Data[vertex_offset];
				actor->Object[0].Group[group].Start = vertex_offset;
				actor->Object[0].Group[group].bones[0] = i;
				actor->Object[0].Group[group].bones[1] = j;
				vertex_offset += ngroups[i][j]->nfaces*3;
				actor->Object[0].Group[group].End = vertex_offset;
				group++;
			}
		}
	}
	
	//GO BACK TO READ FACES
	fseek(file,Faces_offset,SEEK_SET);
	for (i = 0; i < actor->Object[0].polyCount; i++){
		int g[3];
		int min = 32; int max = 0;
		float nx0,ny0,nz0,nx1,ny1,nz1,nx2,ny2,nz2;
		AMG_Face_W2TNV FACE;
		fgets(line, 128, file);
		if (line[0] != 't') ;//error
		fgets(line, 128, file);
		sscanf(line," %i %f %f %f %f %f %f %f %f",
		&g[0],&FACE.u0,&FACE.v0,&nx0,&ny0,&nz0,&FACE.x0,&FACE.y0,&FACE.z0);
		fgets(line, 128, file);
		sscanf(line," %i %f %f %f %f %f %f %f %f",
		&g[1],&FACE.u1,&FACE.v1,&nx1,&ny1,&nz1,&FACE.x1,&FACE.y1,&FACE.z1);
		fgets(line, 128, file);
		sscanf(line," %i %f %f %f %f %f %f %f %f",
		&g[2],&FACE.u2,&FACE.v2,&nx2,&ny2,&nz2,&FACE.x2,&FACE.y2,&FACE.z2);
		
		FACE.nx0 = nx0*128;FACE.ny0 = ny0*128;FACE.nz0 = nz0*128;
		FACE.nx1 = nx1*128;FACE.ny1 = ny1*128;FACE.nz1 = nz1*128;
		FACE.nx2 = nx2*128;FACE.ny2 = ny2*128;FACE.nz2 = nz2*128;

		//What group is this face in?
		for (j=0;j<3;j++){if (min > g[j]) min = g[j]; if (max < g[j]) max = g[j];}
		
		//WEIGHTS
		if(min == max){//Face uses one bone only
			FACE.skinWeight0[0] = 255;FACE.skinWeight0[1] = 0;
			FACE.skinWeight1[0] = 255;FACE.skinWeight1[1] = 0;
			FACE.skinWeight2[0] = 255;FACE.skinWeight2[1] = 0;
		} else {//Face uses two bones
			if (g[0] == min) {FACE.skinWeight0[0] = 255;FACE.skinWeight0[1] = 0;}
			else {FACE.skinWeight0[0] = 0;FACE.skinWeight0[1] = 255;}
			if (g[1] == min) {FACE.skinWeight1[0] = 255;FACE.skinWeight1[1] = 0;}
			else {FACE.skinWeight1[0] = 0;FACE.skinWeight1[1] = 255;}
			if (g[2] == min) {FACE.skinWeight2[0] = 255;FACE.skinWeight2[1] = 0;}
			else {FACE.skinWeight2[0] = 0;FACE.skinWeight2[1] = 255;}
		}
		
		//Store face in this group
		memcpy(&ngroups[min][max]->data[ngroups[min][max]->vertex_offset],&FACE,sizeof(AMG_Vertex_W2TNV)*3);
		ngroups[min][max]->vertex_offset+=3;
	}

	group = 0;
	
	fclose(file);
	
	//actor->Object[0].BBox = (ScePspFVector3*) calloc (2, sizeof(ScePspFVector3));

	// Calcula la bounding box
	for(i=0;i<actor->Object[0].polyCount*3;i++){
		if(actor->Object[0].BBox[0].x >= actor->Object[0].Data[i].x) actor->Object[0].BBox[0].x = actor->Object[0].Data[i].x;	// XMIN
		if(actor->Object[0].BBox[0].y >= actor->Object[0].Data[i].y) actor->Object[0].BBox[0].y = actor->Object[0].Data[i].y;	// YMIN
		if(actor->Object[0].BBox[0].z >= actor->Object[0].Data[i].z) actor->Object[0].BBox[0].z = actor->Object[0].Data[i].z;	// ZMIN
		if(actor->Object[0].BBox[1].x <= actor->Object[0].Data[i].x) actor->Object[0].BBox[1].x = actor->Object[0].Data[i].x;	// XMAX
		if(actor->Object[0].BBox[1].y <= actor->Object[0].Data[i].y) actor->Object[0].BBox[1].y = actor->Object[0].Data[i].y;	// YMAX
		if(actor->Object[0].BBox[1].z <= actor->Object[0].Data[i].z) actor->Object[0].BBox[1].z = actor->Object[0].Data[i].z;	// ZMAX
	}
	actor->Object[0].BBox[0].x *= 2; actor->Object[0].BBox[0].y *= 2; actor->Object[0].BBox[0].z *= 2;
	actor->Object[0].BBox[1].x *= 2; actor->Object[0].BBox[1].y *= 2; actor->Object[0].BBox[1].z *= 2;
	
	//Generate an outline, only vertex postion and weights
	if (outline != 0){
		actor->Outline = 1;
		actor->Object[0].CelShadingScale = outline;
		actor->Object[0].Border = (AMG_Vertex_W2V*) malloc(sizeof(AMG_Vertex_W2V)*actor->Object->polyCount*3);
		for (i = 0; i < actor->Object->polyCount*3;i++){
			actor->Object[0].Border[i].skinWeight[0] = actor->Object[0].Data[i].skinWeight[0];
			actor->Object[0].Border[i].skinWeight[1] = actor->Object[0].Data[i].skinWeight[1];
			actor->Object[0].Border[i].x = actor->Object[0].Data[i].x + (((float)actor->Object[0].Data[i].nx * actor->Object[0].CelShadingScale)/127);
			actor->Object[0].Border[i].y = actor->Object[0].Data[i].y + (((float)actor->Object[0].Data[i].ny * actor->Object[0].CelShadingScale)/127);
			actor->Object[0].Border[i].z = actor->Object[0].Data[i].z + (((float)actor->Object[0].Data[i].nz * actor->Object[0].CelShadingScale)/127);
		}
	}
	else{
		actor->Outline = 0;
		actor->Object[0].CelShadingScale = 0.2f;
	}
	
	sceKernelDcacheWritebackInvalidateRange(actor->Object[0].Data,sizeof(AMG_Vertex_W2TNV)*3*actor->Object->polyCount);
	sceKernelDcacheWritebackInvalidateRange(actor->Object[0].Data,sizeof(AMG_Vertex_W2V  )*3*actor->Object->polyCount);
	
	//get texture with the same name as the file
	char texturename[128] = {0};
	strncpy(texturename,path,127);
	texturename[strlen(path)-4] = 0;
	strcat(texturename,".png");
	AMG_Texture *texture = AMG_LoadTexture(texturename,M3D_IN_VRAM,psm);
	if (texture != NULL) actor->Object[0].texture = texture;
	
	actor->Pos = actor->Rot = (ScePspFVector3) {0,0,0};
	/////////////////////////
	//BUG, models size x2??. checked everything and can't find why
	//////////////////////////
	actor->Scale = (ScePspFVector3) {0.5,0.5,0.5};
	
	actor->Object[0].Lighting = 1;
	actor->Object[0].startFrame = 0;
	actor->Object[0].endFrame = actor->Object[0].frameCount-1;
	actor->Object[0].loop = 1;
	actor->Object[0].speed = 40;
	actor->Object[0].interpolation = 0;
	actor->Object[0].frame = 0;
	actor->smooth = 1;
	
	for (i = 0; i < 32; i++){
		for (j = 0; j < 32; j++){
			if(ngroups[i][j]) {free(ngroups[i][j]); ngroups[i][j] = NULL;}
		}
	}
	
	return actor;
}

M3D_SkinnedActor *M3D_LoadSkinnedActor(const char *path, float outline, u32 psm){
	AMG_Skinned_Actor *a = AMG_LoadSkinnedActor(path,outline,psm);
	return (M3D_SkinnedActor*)a;
}

void AMG_RenderSkinnedActor(AMG_Skinned_Actor *actor){
	// Comprueba si es NULL
	if(actor == NULL) return;
	// Control de la iluminación
	u8 l2 = 0;

	int n_bones = actor->Object[0].n_bones;
	
	if(!actor->Object[0].Lighting){
		l2 = sceGuGetStatus(GU_LIGHTING);
		sceGuDisable(GU_LIGHTING);
	}
	
	//animation
	int frame,nextframe;
	float sp;
	if (actor->Object[0].speed < 0.01){actor->Object[0].interpolation = 0; sp = 0;}
	else sp = 1/actor->Object[0].speed;
	
	if (actor->Object[0].interpolation > 1){
		if(actor->Object[0].loop){
			actor->Object[0].interpolation = 0;
			actor->Object[0].frame++;
		} else {
			if (actor->Object[0].frame == actor->Object[0].endFrame)
				actor->Object[0].interpolation = 1;
			else {
				actor->Object[0].interpolation = 0;
				actor->Object[0].frame++;	
			}
		}
	}

	if(actor->Object[0].loop){
		if (actor->Object[0].frame == actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	}
	
	frame = actor->Object[0].frame;
	if (actor->Object[0].speed > 0.01) nextframe = frame+1;
	else nextframe = frame;
	
	if (frame == actor->Object[0].frameCount) frame = actor->Object[0].startFrame;
	
	actor->f = actor->Object[0].interpolation;
	actor->f2 = vfpu_ease_in_out(actor->Object[0].interpolation);

	// Aplica las transformaciones necesarias
	ScePspFVector3 qpos0 = actor->Object[0].Frame[0].Bone[0].position;
	actor->qpos1 = (ScePspQuatMatrix){actor->Object[0].Frame[frame].Bone[0].position.x,actor->Object[0].Frame[frame].Bone[0].position.y,actor->Object[0].Frame[frame].Bone[0].position.z,0};
	actor->qpos2 = (ScePspQuatMatrix){actor->Object[0].Frame[nextframe].Bone[0].position.x,actor->Object[0].Frame[nextframe].Bone[0].position.y,actor->Object[0].Frame[nextframe].Bone[0].position.z,0};
	
	if (actor->smooth == 0) 
		AMG_QuatSampleLinear(&actor->qpos_inter,&actor->qpos1,&actor->qpos2,actor->f);
	else
		AMG_QuatSampleLinear(&actor->qpos_inter,&actor->qpos1,&actor->qpos2,actor->f2);
	float pfix = (actor->qpos_inter.y-qpos0.y);
	actor->GPos = (ScePspFVector3){actor->Pos.x+actor->qpos_inter.x-qpos0.x,actor->Pos.y+pfix,actor->Pos.z+actor->qpos_inter.z-qpos0.z};

	//Mover
	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);
	AMG_Translate(GU_MODEL,&actor->GPos);
	AMG_Rotate(GU_MODEL, &actor->Rot);
	if(actor->Object[0].phys){
		//vfpu_scale_vector(&GPos,&GPos,2);
		AMG_UpdateSkinnedActorBody(actor);
	}
	AMG_Scale(GU_MODEL, &actor->Scale);
	AMG_UpdateMatrices();		// Actualiza las matrices
if (!skip){	
	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
	sceGuSpecular(AMG.WorldSpecular);	
	sceGuColor(0xffffffff); sceGuAmbient(0xff000000);
	u32 number;

	//MUEVE HUESOS
	int q = 0;
	for( q = 0; q < n_bones; ++q){
		//Toma la rotacion y posicion del hueso
		AMG_LoadIdentityUser(&actor->bones[q]);
		actor->pos = actor->Object[0].Frame[0].Bone[q].position;
		actor->pfix = (ScePspFVector3){-actor->pos.x,-actor->pos.y,-actor->pos.z};

		//interpolate quaternion
		if (actor->smooth == 0) 
			vfpu_quaternion_sample_linear(&actor->Object[0].Frame[frame].Bone[q].interpolated,&actor->Object[0].Frame[frame].Bone[q].orient,&actor->Object[0].Frame[nextframe].Bone[q].orient,actor->f);
		else 
			vfpu_quaternion_sample_linear(&actor->Object[0].Frame[frame].Bone[q].interpolated,&actor->Object[0].Frame[frame].Bone[q].orient,&actor->Object[0].Frame[nextframe].Bone[q].orient,actor->f2);//f2 not working
		
		vfpu_quaternion_normalize(&actor->Object[0].Frame[frame].Bone[q].interpolated);
	
		//Apply position and rotation to bone matrix
		AMG_TranslateUser(&actor->bones[q], &actor->pos);
		AMG_RotateQuatUser(&actor->Object[0].Frame[frame].Bone[q].interpolated, &actor->bones[q]);
		AMG_TranslateUser(&actor->bones[q], &actor->pfix);
		
		//Now apply child bones the rotation of the parents
		if (q != 0) AMG_MultMatrixUser(&actor->bones[actor->Object[0].Frame[0].Bone[q].parent],&actor->bones[q], &actor->bones[q]);
	}
	
	// setup texture
	AMG_EnableTexture(actor->Object[0].texture);

	//Draw groups
	sceGuEnable(GU_CULL_FACE);
	int g = 0;
	//AT LEAST.. DRAW THE STUPID THING
	for (g = 0; g != actor->Object[0].n_groups; g++){ 
		sceGuFrontFace(GU_CCW);
		sceGuEnable(GU_TEXTURE_2D);
		sceGuColor(0xffffffff); 
		number = actor->Object[0].Group[g].End - actor->Object[0].Group[g].Start;
		u32 offset = actor->Object[0].Group[g].Start;
		//Set bone matrices for every bone in the group... 
		int b0 = actor->Object[0].Group[g].bones[0];
		int b1 = actor->Object[0].Group[g].bones[1];
		sceGuBoneMatrix(0, &actor->bones[b0]);//sceGuMorphWeight( q, 1 );
		sceGuBoneMatrix(1, &actor->bones[b1]);
		//Draw group
		sceGuDrawArray(GU_TRIANGLES,GU_WEIGHTS(2)|GU_WEIGHT_8BIT|GU_TEXTURE_32BITF|GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,number,0,actor->Object[0].Data+offset);
		//Draw outline group if any
		if (actor->Outline == 1){
			sceGuDisable(GU_TEXTURE_2D);
			if(actor->Object[0].Lighting) sceGuDisable(GU_LIGHTING);
			sceGuDisable(GU_LIGHTING);
			sceGuFrontFace(GU_CW);
			sceGuColor(0xff000000); 
			sceGuDrawArray(GU_TRIANGLES,GU_WEIGHTS(2)|GU_WEIGHT_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,number,0,actor->Object[0].Border+offset);
			if(actor->Object[0].Lighting) sceGuEnable(GU_LIGHTING);
		}
	}
	
	u8 l = sceGuGetStatus(GU_LIGHTING);
	sceGuDisable(GU_CULL_FACE);
	if(l == GU_TRUE) sceGuEnable(GU_LIGHTING);
}
    AMG_PopMatrix(GU_MODEL);
	// Control de la iluminación
	if((!actor->Object[0].Lighting) && (l2)) sceGuEnable(GU_LIGHTING);

	actor->Object[0].interpolation += sp;
}

void M3D_SkinnedActorRender(M3D_SkinnedActor *actor){
	AMG_RenderSkinnedActor((AMG_Skinned_Actor*)actor);
}

void AMG_ConfigSkinnedActor(AMG_Skinned_Actor *actor, int begin, int end, float speed, int loop, int smooth){
	actor->Object[0].speed = speed;
	actor->Object[0].startFrame = begin;
	actor->Object[0].endFrame = end;
	actor->Object[0].loop = loop;
	//Be sure the model frame is inside the animation
	if (actor->Object[0].frame < actor->Object[0].startFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	if (actor->Object[0].frame > actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	actor->smooth = smooth;
}

void M3D_SkinnedActorConfig(M3D_SkinnedActor *act, int begin, int end, float speed, int loop, int smooth){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Object[0].speed = speed;
	actor->Object[0].startFrame = begin;
	actor->Object[0].endFrame = end;
	actor->Object[0].loop = loop;
	//Be sure the model frame is inside the animation
	if (actor->Object[0].frame < actor->Object[0].startFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	if (actor->Object[0].frame > actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	actor->smooth = smooth;
}

void M3D_SkinnedActorSetPosition(M3D_SkinnedActor *act, float x, float y, float z){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Pos.x = x;
	actor->Pos.y = y;
	actor->Pos.z = z;
}

void M3D_SkinnedActorCopyPosition(M3D_SkinnedActor *m0,M3D_SkinnedActor *m1){
	AMG_Skinned_Actor *model0 = (AMG_Skinned_Actor*)m0;
	AMG_Skinned_Actor *model1 = (AMG_Skinned_Actor*)m1;
	model0->Pos = model1->Pos; 
}

void M3D_SkinnedActorSetRotation(M3D_SkinnedActor *act, float rx, float ry, float rz){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Rot.x = M3D_Deg2Rad(rx);
	actor->Rot.y = M3D_Deg2Rad(ry);
	actor->Rot.z = M3D_Deg2Rad(rz);
}

void M3D_SkinnedActorSetScale(M3D_SkinnedActor *act, float sx, float sy, float sz){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Scale.x = sx;
	actor->Scale.y = sy;
	actor->Scale.z = sz;
}

void M3D_SkinnedActorTranslate(M3D_SkinnedActor *act, float dx, float dy, float dz){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Pos.x += dx;
	actor->Pos.y += dy;
	actor->Pos.z += dz;
}

void M3D_SkinnedActorRotate(M3D_SkinnedActor *act, float rdx, float rdy, float rdz){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Rot.x += M3D_Deg2Rad(rdx);
	actor->Rot.y += M3D_Deg2Rad(rdy);
	actor->Rot.z += M3D_Deg2Rad(rdz);
}

void M3D_SkinnedActorScrollTexture(M3D_SkinnedActor* act,float u, float v){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	actor->Object[0].texture->U += u;
	actor->Object[0].texture->V += v;
}

ScePspFVector3 M3D_SkinnedActorGetPos(M3D_SkinnedActor *act){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*)act;
	return actor->Pos;
}


AMG_Morphing_Actor *AMG_LoadMorphingActor(char *path, float outline, u32 psm){
	int i;
	
	AMG_Morphing_Actor *actor = NULL;
	actor = (AMG_Morphing_Actor*) calloc (1, sizeof(AMG_Morphing_Actor));
	actor->Object = (MorphModel*) calloc (1, sizeof(MorphModel));
	actor->Object[0].texture = NULL;
	actor->Object[0].MultiTexture = NULL;
	
	//////////////////////////////////
	u32 header = 0; 
	u32 data_size;
	FILE *f = fopen(path, "rb");
	fseek(f, 0, SEEK_SET);
	fread (&header,1,4,f);
	//M3MP
	if(header != 0x504D334D) {AMG_Error((char*)"MorphingActor wrong format / Formato MorphingActor incorrecto", path); }

	fseek(f, 0x18, SEEK_SET);
	fread(&actor->Object[0].polyCount,1,4,f);
	fseek(f, 0x2A, SEEK_SET);
	fread(&actor->Object[0].frameCount,1,1,f);
	fseek(f, 0x36, SEEK_SET);
	fread(&data_size,1,4,f);
	
	//PREPARA MEMORIA
	actor->Object[0].Data = (AMG_Vertex_TCNV*) malloc(data_size);
	//READ
	fseek(f, 0x40, SEEK_SET);
	fread(&actor->Object[0].Data[0],data_size,1,f);
	fclose(f);
	
	//OUTLINE
	if (outline != 0){
		actor->Outline = 1;
		actor->Object[0].CelShadingScale = outline;
		actor->Object[0].Border = (AMG_Vertex_V*) malloc(sizeof(AMG_Vertex_V)*actor->Object->polyCount*3*actor->Object[0].frameCount);
		for (i = 0; i < actor->Object->polyCount*3*actor->Object[0].frameCount;i++){
			actor->Object[0].Border[i].x = actor->Object[0].Data[i].x + (((float)actor->Object[0].Data[i].nx * actor->Object[0].CelShadingScale)/127);
			actor->Object[0].Border[i].y = actor->Object[0].Data[i].y + (((float)actor->Object[0].Data[i].ny * actor->Object[0].CelShadingScale)/127);
			actor->Object[0].Border[i].z = actor->Object[0].Data[i].z + (((float)actor->Object[0].Data[i].nz * actor->Object[0].CelShadingScale)/127);
		}
	}

	// Calcula la bounding box
	for(i=0;i<actor->Object[0].polyCount*3;i++){
		if(actor->Object[0].BBox[0].x >= actor->Object[0].Data[i].x) actor->Object[0].BBox[0].x = actor->Object[0].Data[i].x;	// XMIN
		if(actor->Object[0].BBox[0].y >= actor->Object[0].Data[i].y) actor->Object[0].BBox[0].y = actor->Object[0].Data[i].y;	// YMIN
		if(actor->Object[0].BBox[0].z >= actor->Object[0].Data[i].z) actor->Object[0].BBox[0].z = actor->Object[0].Data[i].z;	// ZMIN
		if(actor->Object[0].BBox[1].x <= actor->Object[0].Data[i].x) actor->Object[0].BBox[1].x = actor->Object[0].Data[i].x;	// XMAX
		if(actor->Object[0].BBox[1].y <= actor->Object[0].Data[i].y) actor->Object[0].BBox[1].y = actor->Object[0].Data[i].y;	// YMAX
		if(actor->Object[0].BBox[1].z <= actor->Object[0].Data[i].z) actor->Object[0].BBox[1].z = actor->Object[0].Data[i].z;	// ZMAX
	}
	actor->Object[0].BBox[0].x *= 2; actor->Object[0].BBox[0].y *= 2; actor->Object[0].BBox[0].z *= 2;
	actor->Object[0].BBox[1].x *= 2; actor->Object[0].BBox[1].y *= 2; actor->Object[0].BBox[1].z *= 2;
	
	
	//get texture with the same name as the file
	char texturename[128] = {0};
	strncpy(texturename,path,127);
	texturename[strlen(path)-4] = 0;
	strcat(texturename,".png");
	AMG_Texture *texture = AMG_LoadTexture(texturename,M3D_IN_VRAM,psm);
	if (texture != NULL) actor->Object[0].texture = texture;
	
	actor->Pos = actor->Rot = (ScePspFVector3) {0,0,0};
	actor->Scale = (ScePspFVector3) {1,1,1};
	actor->Object[0].Lighting = 1;
	actor->Object[0].startFrame = 0;
	actor->Object[0].endFrame = actor->Object[0].frameCount;
	actor->Object[0].loop = 1;
	actor->Object[0].speed = 60;
	actor->Object[0].fr = 1;
	actor->Object[0].frame = 0;
	actor->smooth = 1;
	actor->Object[0].collisionreset = 0;
	return actor;

}

M3D_MorphingActor *M3D_LoadMorphingActor(const char *path, float outline, u32 psm){
	AMG_Morphing_Actor *a = AMG_LoadMorphingActor((char*) path,outline,psm);
	return (M3D_MorphingActor*)a;
}

void AMG_RenderMorphingActor(AMG_Morphing_Actor *actor){
	// Comprueba si es NULL
	if(actor == NULL) return;
	// Control de la iluminación
	u8 l2 = 0;
	if(!actor->Object[0].Lighting){
		l2 = sceGuGetStatus(GU_LIGHTING);
		sceGuDisable(GU_LIGHTING);
	}
	
	//Mover
	AMG_PushMatrix(GU_MODEL);
	if(actor->Object[0].phys == 0){
		AMG_Translate(GU_MODEL, &actor->Pos);
		AMG_Rotate(GU_MODEL, &actor->Rot);
		AMG_Scale(GU_MODEL, &actor->Scale);
	}else{
		AMG_Translate(GU_MODEL, &actor->Pos);
		AMG_UpdateMorphingActorBody(actor);
	}
	AMG_UpdateMatrices();											// Actualiza las matrices
	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
	sceGuSpecular(AMG.WorldSpecular);	
	sceGuColor(0xffffffff); sceGuAmbient(0xff000000);
	
	//Animate if start and end are different
	if (actor->Object[0].endFrame != actor->Object[0].startFrame){
		//Frames animate
		float sp = 1/actor->Object[0].speed;
		//Be sure the model frame is inside the animation
		if (actor->Object[0].frame < actor->Object[0].startFrame) actor->Object[0].frame = actor->Object[0].startFrame;
		if (actor->Object[0].frame > actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;	
		
		for (int i = 0; i < actor->Object[0].frameCount; i++) sceGuMorphWeight(i,0);
		
		sceGuMorphWeight(actor->Object[0].frame,actor->Object[0].fr);
		if (actor->Object[0].frame < actor->Object[0].endFrame)
			sceGuMorphWeight(actor->Object[0].frame+1,1-actor->Object[0].fr);
		else sceGuMorphWeight(0,1-actor->Object[0].fr);
		actor->Object[0].fr-=sp;
		if (actor->Object[0].fr < 0) {
			actor->Object[0].frame++;
			actor->Object[0].fr = 1;
		}
		if (actor->Object[0].frame == 8) actor->Object[0].frame = 0;
	} else {
		for (int i = 0; i < actor->Object[0].frameCount; i++) sceGuMorphWeight(i,0);
		sceGuMorphWeight(actor->Object[0].startFrame,1);
	}
	// setup texture
	AMG_EnableTexture(actor->Object[0].texture);
	sceGuEnable(GU_CULL_FACE);
	sceGuFrontFace(GU_CCW);

	sceGuDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_VERTICES(actor->Object[0].frameCount)|GU_TRANSFORM_3D,actor->Object[0].polyCount*3,0,actor->Object[0].Data);

	if (actor->Outline){
		if(l == GU_TRUE) sceGuDisable(GU_LIGHTING);
		sceGuDisable(GU_TEXTURE_2D);
		sceGuEnable(GU_CULL_FACE);
		sceGuFrontFace(GU_CW);
		sceGuColor(0xff000000);
		sceGuDrawArray(GU_TRIANGLES,GU_VERTEX_32BITF|GU_VERTICES(actor->Object[0].frameCount)|GU_TRANSFORM_3D,actor->Object[0].polyCount*3,0,actor->Object[0].Border);
	}
	u8 l = sceGuGetStatus(GU_LIGHTING);
	
	sceGuDisable(GU_CULL_FACE);
	if(l == GU_TRUE) sceGuEnable(GU_LIGHTING);
	sceGuColor(0xffffffff);
	
	sceGuDisable(GU_TEXTURE_2D);
    AMG_PopMatrix(GU_MODEL);
	// Control de la iluminación
	if((!actor->Object[0].Lighting) && (l2)) sceGuEnable(GU_LIGHTING);
}

void M3D_MorphingActorRender(M3D_MorphingActor *actor){
	AMG_RenderMorphingActor((AMG_Morphing_Actor *)actor);
}

void AMG_ConfigMorphingActor(AMG_Morphing_Actor *actor, int begin, int end, float speed, int smooth){
	actor->Object[0].speed = speed;
	actor->smooth = smooth;
	actor->Object[0].startFrame = begin;
	actor->Object[0].endFrame = end;
}

void M3D_MorphingActorConfig(M3D_MorphingActor *act, int begin, int end, float speed, int smooth){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Object[0].speed = speed;
	actor->smooth = smooth;
	actor->Object[0].startFrame = begin;
	actor->Object[0].endFrame = end;
}

void M3D_MorphingActorSetPosition(M3D_MorphingActor *act, float x, float y, float z){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Pos.x = x;
	actor->Pos.y = y;
	actor->Pos.z = z;
}

void M3D_MorphingActorCopyPosition(M3D_MorphingActor *m0,M3D_MorphingActor *m1){
	AMG_Morphing_Actor *model0 = (AMG_Morphing_Actor*)m0;
	AMG_Morphing_Actor *model1 = (AMG_Morphing_Actor*)m1;
	model0->Pos = model1->Pos; 
}

void M3D_MorphingActorSetRotation(M3D_MorphingActor *act, float rx, float ry, float rz){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Rot.x = M3D_Deg2Rad(rx);
	actor->Rot.y = M3D_Deg2Rad(ry);
	actor->Rot.z = M3D_Deg2Rad(rz);
}

void M3D_MorphingActorSetScale(M3D_MorphingActor *act, float sx, float sy, float sz){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Scale.x = sx;
	actor->Scale.y = sy;
	actor->Scale.z = sz;
}

void M3D_MorphingActorTranslate(M3D_MorphingActor *act, float dx, float dy, float dz){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Pos.x += dx;
	actor->Pos.y += dy;
	actor->Pos.z += dz;
}

void M3D_MorphingActorRotate(M3D_MorphingActor *act, float rdx, float rdy, float rdz){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Rot.x += M3D_Deg2Rad(rdx);
	actor->Rot.y += M3D_Deg2Rad(rdy);
	actor->Rot.z += M3D_Deg2Rad(rdz);
}

ScePspFVector3 M3D_MorphingActorGetPosition(M3D_MorphingActor *act){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	return actor->Pos;
}

void M3D_MorphingActorUnload(M3D_MorphingActor *a){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)a;
	if(actor == NULL) return;
	MorphModel *Object = &actor->Object[0];
	if(Object->texture) AMG_UnloadTexture(Object->texture);
	if(Object->MultiTexture) AMG_UnloadTexture(Object->MultiTexture);
	if(Object->Data)  {free(Object->Data);Object->Data = NULL;}
	if(Object->Border){free(Object->Border);Object->Border = NULL;}
	if(actor->Object) {free(actor->Object);actor->Object = NULL;}
	free(actor); actor = NULL;
}

void M3D_SkinnedActorUnload(M3D_SkinnedActor *a){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor *)a;
	if(actor == NULL)return;
	SkinnedModel *Object = &actor->Object[0];
	if(Object->texture) AMG_UnloadTexture(Object->texture);
	if(Object->MultiTexture) AMG_UnloadTexture(Object->MultiTexture);
	if(Object->Data)  {free(Object->Data);Object->Data = NULL;}
	if(Object->Border){free(Object->Border);Object->Border = NULL;}
	if(Object->Group) {free(Object->Group);Object->Group = NULL;}
	if(actor->Object) {free(actor->Object);actor->Object = NULL;}
	if(actor) {free(actor);actor = NULL;}
}


void M3D_SkinnedActorSetLighting(M3D_SkinnedActor *a, int light){
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor *)a;
	actor->Object[0].Lighting = light;
}

void M3D_MorphingActorSetLighting(M3D_MorphingActor *a, int light){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)a;
	actor->Object[0].Lighting = light;
}

void M3D_MorphingActorScrollTexture(M3D_MorphingActor* act,float u, float v){
	AMG_Morphing_Actor *actor = (AMG_Morphing_Actor *)act;
	actor->Object[0].texture->U += u;
	actor->Object[0].texture->V += v;
}

// Clone an actor
AMG_Skinned_Actor *AMG_CloneSkinnedActor(AMG_Skinned_Actor *actorc){
	if(!actorc) return 0;
	//Create another actor struct
	AMG_Skinned_Actor *actor = (AMG_Skinned_Actor*) calloc (1, sizeof(AMG_Skinned_Actor));
	//Copy all data (including Object pointer)
	memcpy(actor,actorc,sizeof(AMG_Skinned_Actor));
	//We have to replace the Object pointer with another Object pointer so they are different objects
	actor->Object = (SkinnedModel*) calloc (1, sizeof(SkinnedModel));
	//Copy all data, and now we also want all original (model and texture) data pointers, which will be reused
	memcpy(&actor->Object[0],&actorc->Object[0],sizeof(SkinnedModel));
	//Disable physics in case original model had them
	actor->Object[0].bullet_id = 0;
	actor->Object[0].phys = 0;
	/*
	actor->Object[0].bullet_id = 0;
	actor->Object[0].material_name = actorc->Object[0].material_name;
	actor->Object[0].texture = actorc->Object[0].texture;
	actor->Object[0].MultiTexture = NULL;
	actor->Object[0].Data = actorc->Object[0].Data;
	actor->Object[0].Border = actorc->Object[0].Border;
	actor->Object[0].Group = actorc->Object[0].Group;
	actor->Object[0].n_bones = actorc->Object[0].n_bones;
	memcpy(actor->Object[0].Frame,actorc->Object[0].Frame,32*sizeof(AMG_SkinFrame));
	actor->Object[0].polyCount = actorc->Object[0].polyCount;
	actor->Object[0].Data = actorc->Object[0].Data;
	actor->Object[0].n_groups = actorc->Object[0].n_groups;
	actor->Object[0].Group = actorc->Object[0].Group;
	actor->Object[0].BBox[0] = actorc->Object[0].BBox[0];
	actor->Object[0].BBox[1] = actorc->Object[0].BBox[1];
	actor->Outline = actorc->Outline;
	actor->Object[0].CelShadingScale = actorc->Object[0].CelShadingScale;
	actor->Object[0].Border = actorc->Object[0].Border;
	actor->Object[0].texture = actorc->Object[0].texture;
	actor->Pos = actor->Rot = (ScePspFVector3) {0,0,0};
	/////////////////////////
	//BUG, models size x2??. checked everything and can't find why
	//////////////////////////
	actor->Scale = (ScePspFVector3) {0.5,0.5,0.5};
	actor->Object[0].Lighting = 1;
	actor->Object[0].startFrame = 0;
	actor->Object[0].frameCount = actorc->Object[0].frameCount;
	actor->Object[0].endFrame = actorc->Object[0].endFrame;
	actor->Object[0].loop = actorc->Object[0].loop;
	actor->Object[0].speed = actorc->Object[0].speed;
	actor->Object[0].interpolation = actorc->Object[0].interpolation;
	actor->Object[0].frame = 0;
	actor->Object[0].collisionreset = 0;
	actor->smooth = actorc->smooth;
	*/
	return actor;
}	

M3D_SkinnedActor *M3D_SkinnedActorClone(M3D_SkinnedActor *actorc){
	AMG_Skinned_Actor *a = AMG_CloneSkinnedActor((AMG_Skinned_Actor *)actorc);
	return (M3D_SkinnedActor*)a;
}


void M3D_SkinnedActorRenderMirror(M3D_SkinnedActor* act, u8 axis){
	AMG_Skinned_Actor* actor = (AMG_Skinned_Actor*)act; 

	// Comprueba si es NULL
	if(actor == NULL) return;
	// Control de la iluminación
	u8 l2 = 0;

	int n_bones = actor->Object[0].n_bones;
	
	l2 = sceGuGetStatus(GU_LIGHTING);
	if((!actor->Object[0].Lighting) && (l2)) sceGuDisable(GU_LIGHTING);
	if((actor->Object[0].Lighting) && (l2)) sceGuEnable(GU_LIGHTING);
	
	//store light
	u8 light = 0;
	ScePspFVector3 tmp_lgt = (ScePspFVector3){AMG_Light[light].Pos.x,AMG_Light[light].Pos.y,AMG_Light[light].Pos.z};
	ScePspFVector3 lpos;
	// scale and invert light
	switch(axis){
		case 0:			// X
			lpos = (ScePspFVector3){-1*tmp_lgt.x,tmp_lgt.y,tmp_lgt.z};
			sceGuLight(light, AMG_Light[light].Type, AMG_Light[light].Component, &lpos);
			break;
		case 1:			// Y
			lpos = (ScePspFVector3){tmp_lgt.x,-1*tmp_lgt.y,tmp_lgt.z};
			sceGuLight(light, AMG_Light[light].Type, AMG_Light[light].Component, &lpos);
			break;
		case 2:			// Z
			lpos = (ScePspFVector3){tmp_lgt.x,tmp_lgt.y,-1*tmp_lgt.z};
			sceGuLight(light, AMG_Light[light].Type, AMG_Light[light].Component, &lpos);
			break;
		default: return;
	}
	// Render

	//animation
	int frame,nextframe;
	float sp;
	if (actor->Object[0].speed < 0.01){actor->Object[0].interpolation = 0; sp = 0;}
	else sp = 1/actor->Object[0].speed;
	
	if (actor->Object[0].interpolation > 1){
		if(actor->Object[0].loop){
			actor->Object[0].interpolation = 0;
			actor->Object[0].frame++;
		} else {
			if (actor->Object[0].frame == actor->Object[0].endFrame)
				actor->Object[0].interpolation = 1;
			else {
				actor->Object[0].interpolation = 0;
				actor->Object[0].frame++;	
			}
		}
	}

	if(actor->Object[0].loop){
		if (actor->Object[0].frame == actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	}
	
	frame = actor->Object[0].frame;
	if (actor->Object[0].speed > 0.01) nextframe = frame+1;
	else nextframe = frame;
	
	if (frame == actor->Object[0].frameCount) frame = actor->Object[0].startFrame;
	
	actor->f = actor->Object[0].interpolation;
	actor->f2 = vfpu_ease_in_out(actor->Object[0].interpolation);

	// Aplica las transformaciones necesarias
	ScePspFVector3 qpos0 = actor->Object[0].Frame[0].Bone[0].position;
	actor->qpos1 = (ScePspQuatMatrix){actor->Object[0].Frame[frame].Bone[0].position.x,actor->Object[0].Frame[frame].Bone[0].position.y,actor->Object[0].Frame[frame].Bone[0].position.z,0};
	actor->qpos2 = (ScePspQuatMatrix){actor->Object[0].Frame[nextframe].Bone[0].position.x,actor->Object[0].Frame[nextframe].Bone[0].position.y,actor->Object[0].Frame[nextframe].Bone[0].position.z,0};
	
	if (actor->smooth == 0) 
		AMG_QuatSampleLinear(&actor->qpos_inter,&actor->qpos1,&actor->qpos2,actor->f);
	else
		AMG_QuatSampleLinear(&actor->qpos_inter,&actor->qpos1,&actor->qpos2,actor->f2);
	float pfix = (actor->qpos_inter.y-qpos0.y);
	actor->GPos = (ScePspFVector3){actor->Pos.x+actor->qpos_inter.x-qpos0.x,actor->Pos.y+pfix,actor->Pos.z+actor->qpos_inter.z-qpos0.z};

	//Mover
	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);
	AMG_Translate(GU_MODEL,&actor->GPos);
	AMG_Rotate(GU_MODEL, &actor->Rot);
	if(actor->Object[0].phys){
		//vfpu_scale_vector(&GPos,&GPos,2);
		AMG_UpdateSkinnedActorBody(actor);
	}
	AMG_Scale(GU_MODEL, &actor->Scale);
	
	ScePspFMatrix4 m0 = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};
	ScePspFMatrix4 __attribute__((aligned(16))) m1;
	ScePspFMatrix4 __attribute__((aligned(16))) m2;
	if (axis == 0){m0.x.x = -1; m0.w.x = amg_curfloor->Pos.x*2;}
	if (axis == 1){m0.y.y = -1; m0.w.y = amg_curfloor->Pos.y*2;}
	if (axis == 2){m0.z.z = -1; m0.w.z = amg_curfloor->Pos.z*2;}
	AMG_GetMatrix(GU_MODEL, &m1);
	AMG_MultMatrixUser(&m0,&m1,&m2);
	AMG_SetMatrix(GU_MODEL, &m2);
	
	
	AMG_UpdateMatrices();		// Actualiza las matrices
if (!skip){	
	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar
	sceGuSpecular(AMG.WorldSpecular);	
	sceGuColor(0xffffffff); sceGuAmbient(0xff000000);
	u32 number;

	//MUEVE HUESOS
	int q = 0;
	for( q = 0; q < n_bones; ++q){
		//Toma la rotacion y posicion del hueso
		AMG_LoadIdentityUser(&actor->bones[q]);
		actor->pos = actor->Object[0].Frame[0].Bone[q].position;
		actor->pfix = (ScePspFVector3){-actor->pos.x,-actor->pos.y,-actor->pos.z};

		//interpolate quaternion
		if (actor->smooth == 0) 
			vfpu_quaternion_sample_linear(&actor->Object[0].Frame[frame].Bone[q].interpolated,&actor->Object[0].Frame[frame].Bone[q].orient,&actor->Object[0].Frame[nextframe].Bone[q].orient,actor->f);
		else 
			vfpu_quaternion_sample_linear(&actor->Object[0].Frame[frame].Bone[q].interpolated,&actor->Object[0].Frame[frame].Bone[q].orient,&actor->Object[0].Frame[nextframe].Bone[q].orient,actor->f2);//f2 not working
		
		vfpu_quaternion_normalize(&actor->Object[0].Frame[frame].Bone[q].interpolated);
	
		//Apply position and rotation to bone matrix
		AMG_TranslateUser(&actor->bones[q], &actor->pos);
		AMG_RotateQuatUser(&actor->Object[0].Frame[frame].Bone[q].interpolated, &actor->bones[q]);
		AMG_TranslateUser(&actor->bones[q], &actor->pfix);
		
		//Now apply child bones the rotation of the parents
		if (q != 0) AMG_MultMatrixUser(&actor->bones[actor->Object[0].Frame[0].Bone[q].parent],&actor->bones[q], &actor->bones[q]);
	}
	
	// setup texture
	AMG_EnableTexture(actor->Object[0].texture);

	//Draw groups
	sceGuEnable(GU_CULL_FACE);
	sceGuFrontFace(GU_CW);
	sceGuEnable(GU_TEXTURE_2D);
	int g = 0;
	//AT LEAST.. DRAW THE STUPID THING
	for (g = 0; g != actor->Object[0].n_groups; g++){ 
		number = actor->Object[0].Group[g].End - actor->Object[0].Group[g].Start;
		u32 offset = actor->Object[0].Group[g].Start;
		//Set bone matrices for every bone in the group... 
		int b0 = actor->Object[0].Group[g].bones[0];
		int b1 = actor->Object[0].Group[g].bones[1];
		sceGuBoneMatrix(0, &actor->bones[b0]);//sceGuMorphWeight( q, 1 );
		sceGuBoneMatrix(1, &actor->bones[b1]);
		//sceGuDepthMask(1);
		//Draw group
		//draw if stencil is zero, do not mask writes (0xff)
		sceGuStencilFunc(GU_EQUAL,0, 0xFF);
		sceGuStencilOp(GU_KEEP, GU_KEEP, GU_KEEP); //Do not change stencil
		//draw
		sceGuDrawArray(GU_TRIANGLES,GU_WEIGHTS(2)|GU_WEIGHT_8BIT|GU_TEXTURE_32BITF|GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,number,0,actor->Object[0].Data+offset);
		//sceGuDepthMask(0);
	}
	sceGuDisable(GU_CULL_FACE);
}
    AMG_PopMatrix(GU_MODEL);
	// Control de la iluminación
	if((!actor->Object[0].Lighting) && (l2)) sceGuEnable(GU_LIGHTING);
	//reset light
	sceGuLight(light, AMG_Light[light].Type, AMG_Light[light].Component, &tmp_lgt);
}

// Borra

/*
// Tracking Modelo-Modelo
void Track3DObj(AMG_Model *model, AMG_Model *model1, int track,int x, int y, int z){
	if (model->Object[0].bullet_id != NULL);
	ScePspFVector3 Pos;
	ScePspFVector3 Rot;
}
// Tracking Modelo-Actor
void Track3DObj(AMG_Model *model, AMG_Actor *actor, int track,int x, int y, int z){
	bullet_id
	phys
	ScePspFVector3 Pos;
	ScePspFVector3 Rot;
}
*/
// Tracking Actor-Actor
/*
void Track3DObj(AMG_Actor *actor, AMG_Actor *actor1, float speed, ScePspFVector3 ax){
	{}
	if (actor->Object[0].phys == 0){
		float x =(actor->Pos.x-actor1->Pos.x);
		float y =(actor->Pos.y-Purpl1->Pos.y);
		float z =(actor->Pos.z-Purple->Pos.z);
		actor->Rot.y = vfpu_atan2f(x,z);
		actor->Rot.z = vfpu_atan2f(x,z);
		actor->Pos.x += x/10 *speed;
		actor->Pos.z += z/10 *speed;
	}
	if (actor->Object[0].phys == 1){
		float x =(actor->Pos.x-actor->Pos.x);
		float z =(actor->Pos.z-Purple->Pos.z);
		actor->Rot.y = vfpu_atan2f(x,z);
		AMG_SetActorLinearVelocity(actor, (x/10) *speed, 0,(z/10)*speed);			
	}
}*/


//***************
//3D object (cylinder) for STENCIL Shadow
extern AMG_Vertex_NV Volumetric_Shadow[240];

void AMG_Track(ScePspFVector3 *to, ScePspFVector3 *from, ScePspFVector3 *up, ScePspFMatrix4 *m);

float angle = 0;
void M3D_SkinnedActorCastShadow(M3D_SkinnedActor *act, int alpha, int type, int light){    
	AMG_Skinned_Actor *a = (AMG_Skinned_Actor *)act;
	//Prepare to Draw Shadows  
	sceGuEnable(GU_CULL_FACE);
    sceGuDisable(GU_TEXTURE_2D);
	sceGuDepthMask(GU_TRUE);
	
	float sizex = fabs((a->Object->BBox[0].x - a->Object->BBox[1].x)/2);
	float sizez = fabs((a->Object->BBox[0].z - a->Object->BBox[1].z)/2);
	float sizey = fabs((a->Object->BBox[0].y - a->Object->BBox[1].y)/2);
	float s = (sizex+sizey+sizez)/3;
	ScePspFVector3 __attribute__((aligned(16))) size = (ScePspFVector3) {s,s,s};

	//Look at
	ScePspFMatrix4 __attribute__((aligned(16))) mtx_shadow;
	ScePspFVector3 __attribute__((aligned(16))) mtx_from = (ScePspFVector3){a->Pos.x,a->Pos.y,a->Pos.z};
	ScePspFVector3 __attribute__((aligned(16))) mtx_rot = (ScePspFVector3){1.5708,0,0};
	ScePspFVector3 __attribute__((aligned(16))) mtx_to = (ScePspFVector3) {
		a->Pos.x+AMG_Light[light].Pos.x,
		a->Pos.y+AMG_Light[light].Pos.y,
		a->Pos.z+AMG_Light[light].Pos.z,
	};
	ScePspFVector3 __attribute__((aligned(16))) mtx_up = (ScePspFVector3) {0,1,0};
	
	if (type == 0) {
		AMG_PushMatrix(GU_MODEL);
		AMG_LoadIdentity(GU_MODEL);
		AMG_Translate(GU_MODEL,&a->Pos);
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

extern u8 AMG_ShadowReceiver_alpha;

void AMG_RenderSkinnedActorShadow(AMG_Skinned_Actor *actor){
	// Comprueba si es NULL
	if(actor == NULL) return;

	int n_bones = actor->Object[0].n_bones;

	//animation
	int frame,nextframe;
	//float sp = 1/actor->Object[0].speed;
	
	if (actor->Object[0].interpolation > 1){
		actor->Object[0].interpolation = 0;
		actor->Object[0].frame++;
	}
	
	//Be sure the model frame is inside the animation
	if (actor->Object[0].frame < actor->Object[0].startFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	if (actor->Object[0].frame > actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;	
	if (actor->Object[0].frame == actor->Object[0].endFrame) actor->Object[0].frame = actor->Object[0].startFrame;
	frame = actor->Object[0].frame;
	nextframe = frame+1;
	
	if (frame == actor->Object[0].frameCount) frame = actor->Object[0].startFrame;

	
	actor->f = actor->Object[0].interpolation;
if (!skip){	
	// Aplica las transformaciones necesarias
	ScePspFVector3 qpos0 = actor->Object[0].Frame[0].Bone[0].position;
	actor->qpos1 = (ScePspQuatMatrix){actor->Object[0].Frame[frame].Bone[0].position.x,actor->Object[0].Frame[frame].Bone[0].position.y,actor->Object[0].Frame[frame].Bone[0].position.z,0};
	actor->qpos2 = (ScePspQuatMatrix) {actor->Object[0].Frame[nextframe].Bone[0].position.x,actor->Object[0].Frame[nextframe].Bone[0].position.y,actor->Object[0].Frame[nextframe].Bone[0].position.z,0};
	AMG_QuatSampleLinear(&actor->qpos_inter,&actor->qpos1,&actor->qpos2,actor->f);
	actor->GPos = (ScePspFVector3){actor->Pos.x+actor->qpos_inter.x-qpos0.x,actor->Pos.y+(actor->qpos_inter.y-qpos0.y)*2,actor->Pos.z+actor->qpos_inter.z-qpos0.z};

	//Mover
	AMG_PushMatrix(GU_MODEL);
	if(actor->Object[0].phys == 0){
		AMG_Translate(GU_MODEL, &actor->GPos);
		AMG_Rotate(GU_MODEL, &actor->Rot);
	}else{
		//vfpu_scale_vector(&GPos,&GPos,2);
		AMG_Translate(GU_MODEL, &actor->GPos);
		AMG_UpdateSkinnedActorBody(actor);
	}
	AMG_Scale(GU_MODEL, &actor->Scale);
	AMG_UpdateMatrices();		// Actualiza las matrices
	u32 number;

	//MUEVE HUESOS
	int q = 0;
	for( q = 0; q < n_bones; ++q){
		//Toma la rotacion y posicion del hueso
		AMG_LoadIdentityUser(&actor->bones[q]);
		actor->pos = (ScePspFVector3) actor->Object[0].Frame[0].Bone[q].position;
		actor->pfix = (ScePspFVector3) {-1*actor->pos.x,-1*actor->pos.y,-1*actor->pos.z};

		//interpolate quaternion
		vfpu_quaternion_sample_linear(&actor->Object[0].Frame[frame].Bone[q].interpolated,&actor->Object[0].Frame[frame].Bone[q].orient,&actor->Object[0].Frame[nextframe].Bone[q].orient,actor->f);
		vfpu_quaternion_normalize(&actor->Object[0].Frame[frame].Bone[q].interpolated);
	
		//Apply position and rotation to bone matrix
		AMG_TranslateUser(&actor->bones[q], &actor->pos);
		AMG_RotateQuatUser(&actor->Object[0].Frame[frame].Bone[q].interpolated, &actor->bones[q]);
		AMG_TranslateUser(&actor->bones[q], &actor->pfix);
		
		//Now apply child bones the rotation of the parents
		if (q != 0) AMG_MultMatrixUser(&actor->bones[actor->Object[0].Frame[0].Bone[q].parent],&actor->bones[q], &actor->bones[q]);
	}

	//Draw groups
	int g = 0;

	sceGuColor(0xFF000000);
	sceGuEnable(GU_STENCIL_TEST); // Stencil test
	sceGuStencilFunc(GU_ALWAYS,AMG_ShadowReceiver_alpha,0); // Drawn pixels will be non transparent
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_REPLACE); // keep value on failed test (fail and zfail) and replace on pass
	
	//Draw
	for (g = 0; g != actor->Object[0].n_groups; g++){ 
		number = actor->Object[0].Group[g].End - actor->Object[0].Group[g].Start;
		u32 offset = actor->Object[0].Group[g].Start;
		//Set bone matrices for every bone in the group... 
		int b0 = actor->Object[0].Group[g].bones[0];
		int b1 = actor->Object[0].Group[g].bones[1];
		sceGuBoneMatrix(0, &actor->bones[b0]);//sceGuMorphWeight( q, 1 );
		sceGuBoneMatrix(1, &actor->bones[b1]);
		//Draw group
		sceGuDrawArray(GU_TRIANGLES,GU_WEIGHTS(2)|GU_WEIGHT_8BIT|GU_TEXTURE_32BITF|GU_NORMAL_8BIT|GU_VERTEX_32BITF|GU_TRANSFORM_3D,number,0,actor->Object[0].Data+offset);
	}
	
	sceGuDisable(GU_STENCIL_TEST); // Stencil test
    AMG_PopMatrix(GU_MODEL);
}
}

void M3D_SkinnedActorRenderShadow(M3D_SkinnedActor *actor){
	AMG_RenderSkinnedActorShadow((AMG_Skinned_Actor *)actor);
}


//NURBS SURFACES


M3D_NurbsSurface *M3D_CreateNurbsSurface(const char *texpath, u32 psm, float size,int steps){
	AMG_NurbsSurface *s = NULL;
	s = (AMG_NurbsSurface*) calloc (1, sizeof(AMG_NurbsSurface));
	if (!s) return 0;
	s->size = size;
	s->resolution = steps;
	float x = size/2; float y = -size/2;
	float u = 0; float v = 0;
	int currvtx = 0;
	s->vertices = (AMG_Vertex_TNV*) calloc(steps*steps,sizeof(AMG_Vertex_TNV));
	s->points = (float*) calloc(steps*steps,4*6);
	
	u32 i,j;
	for (i = 0; i < s->resolution; i++){
		u = 0;x = size/2;
		for (j = 0; j < s->resolution; j++){
			s->vertices[currvtx].u = u;
			s->vertices[currvtx].v = v;
			s->vertices[currvtx].nx = 0;
			s->vertices[currvtx].ny = 128;
			s->vertices[currvtx].nz = 0;
			s->vertices[currvtx].x = x;
			s->vertices[currvtx].y = 0;
			s->vertices[currvtx].z = y;
			currvtx++;
			u+= (float)1/(steps-1);
			x-= (float)size/(steps-1);
		}
		v+= (float)1/(steps-1);
		y+= (float)size/(steps-1);
	}
	//sceKernelDcacheWritebackInvalidateRange(s->vertices,s->resolution*s->resolution*sizeof(AMG_Vertex_TNV));
	s->Pos = (ScePspFVector3) {0,0,0};
	s->Rot = (ScePspFVector3) {0,0,0};
	s->Scale = (ScePspFVector3) {1,1,1};
	
	s->Texture = NULL;

	if (texpath) s->Texture = AMG_LoadTexture(texpath,M3D_IN_VRAM,psm);

	s->mode = 0;
	s->px0 = 0;
	s->py0 = 0;
	s->px1 = 0;
	s->py1 = 0;
	
	M3D_NurbsSurface *Surface = (M3D_NurbsSurface*) calloc (1, sizeof(M3D_NurbsSurface));
	if (!Surface) return 0;
	Surface->NurbsSurface = (void *) s;
	return Surface;
}

void M3D_NurbsSurfaceSet(M3D_NurbsSurface *surface, u32 mode, float px0, float py0, float px1, float py1, float strength, float angle){
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->mode = mode;
	s->px0 = px0;
	s->py0 = py0;
	s->px1 = px1;
	s->py1 = py1;
	s->strength = strength;
	s->angle = angle;
}

void M3D_NurbsSurfaceRender(M3D_NurbsSurface *surface){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	ScePspFMatrix4 __attribute__((aligned(64))) world;
	ScePspFMatrix4 __attribute__((aligned(64))) view;
	int currvtx = 0;
	int sprvtx = 0;
	float step = s->size / s->resolution * 4;
	unsigned int i,j;
	// Animate surface
if (!skip){	
	switch(s->mode){
		case 0://SINE
		for (i = 0; i < s->resolution; i++){
			for (j = 0; j < s->resolution; j++){
				s->vertices[currvtx].y = cosf((j*step)+s->angle)*s->strength;
				currvtx++;
			}
		}
		break;
		case 1://CIRCLE
		for (i = 0; i < s->resolution; i++){
			for (j = 0; j < s->resolution; j++){
				float dx = M3D_fabsf(s->vertices[currvtx].x - s->px0);
				float dy = M3D_fabsf(s->vertices[currvtx].z - s->py0);
				s->vertices[currvtx].y = M3D_Sin((M3D_VectorLength(dx,dy,0)*4)+s->angle)*s->strength;
				currvtx++;
			}
		}
		break;
		case 2://INTERFERENCE WAVES (LIQUID LIKE SURFACE)
		for (i = 0; i < s->resolution; i++){
			for (j = 0; j < s->resolution; j++){
				float dx0 = M3D_fabsf(s->vertices[currvtx].x - s->px0);
				float dy0 = M3D_fabsf(s->vertices[currvtx].z - s->py0);
				float dx1 = M3D_fabsf(s->vertices[currvtx].x - s->px1);
				float dy1 = M3D_fabsf(s->vertices[currvtx].z - s->py1);
				float wave0 = M3D_Sin((M3D_VectorLength(dx0,dy0,0)*5)+s->angle);
				float wave1 = M3D_Cos((M3D_VectorLength(dx1,dy1,0)*5)+(s->angle/1.3));
				s->vertices[currvtx].y = (wave0 + wave1) *s->strength;
				
				currvtx++;
			}
		}
		break;
	}

	AMG_PushMatrix(GU_MODEL);
	AMG_LoadIdentity(GU_MODEL);
	AMG_Translate(GU_MODEL, &s->Pos);
	AMG_Rotate(GU_MODEL, &s->Rot);
	AMG_Scale(GU_MODEL, &s->Scale);
	AMG_UpdateMatrices();
	AMG_GetMatrix(GU_MODEL,&world);
	AMG_GetMatrix(GU_VIEW,&view);
	gumFastInverse(&world,&world);
	AMG_MultMatrixUser(&world,&view,&world);
	
	sceKernelDcacheWritebackAll();

	if(!s->Texture) sceGuDisable(GU_TEXTURE_2D);
	else AMG_EnableTexture(s->Texture);

	//DRAW SURFACE
	sceGuFrontFace(GU_CW);
	sceGuDisable(GU_ALPHA_TEST);
	sceGuDisable(GU_BLEND);
	
	sceGuColor(0xFFFFFFFF);
	sceGuAmbientColor(0xffFFFFFF);
	sceGumDrawSpline(GU_TEXTURE_32BITF |GU_NORMAL_8BIT|GU_VERTEX_32BITF,s->resolution,s->resolution,-1,1,0,s->vertices);
	
	sceGuEnable(GU_BLEND);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuFrontFace(GU_CCW);
	
	AMG_PopMatrix(GU_MODEL);
}
}

void M3D_NurbsSetMapping(M3D_NurbsSurface *surface, int mode){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	if (mode) AMG_SetTextureMapping(s->Texture, GU_ENVIRONMENT_MAP, 1, 2);
	else AMG_SetTextureMapping(s->Texture, GU_TEXTURE_COORDS, 0, 0);
}

void M3D_NurbsDelete(M3D_NurbsSurface *surface){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	AMG_UnloadTexture(s->Texture);
	free(s->vertices); s->vertices = NULL;
	free(s->points); s->points = NULL;
	free(s); s = NULL;
	free(surface); surface = NULL;
}

void M3D_NurbsSetPosition(M3D_NurbsSurface *surface,float x, float y, float z){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->Pos.x = x;s->Pos.y = y;s->Pos.z = z;
}

void M3D_NurbsSetRotation(M3D_NurbsSurface *surface,float x, float y, float z){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->Rot.x = x;s->Rot.y = y;s->Rot.z = z;
}

void M3D_NurbsSetScale(M3D_NurbsSurface *surface,float x, float y, float z){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->Scale.x = x;s->Scale.y = y;s->Scale.z = z;
}

void M3D_NurbsTranslate(M3D_NurbsSurface *surface,float x, float y, float z){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->Pos.x +=x;s->Pos.y += y;s->Pos.z += z;
}

void M3D_NurbsRotate(M3D_NurbsSurface *surface,float x, float y, float z){
	if(!surface) return;
	AMG_NurbsSurface *s = (AMG_NurbsSurface *) surface->NurbsSurface;
	s->Rot.x +=x;s->Rot.y += y;s->Rot.z += z;
}

