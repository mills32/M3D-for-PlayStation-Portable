/*
 * This functios are taken from "tri Engine".
 * Copyright (C) 2007 tri
 * Copyright (C) 2007 Alexander Berl 'Raphael' <raphael@fx-world.org>
 */
 
#ifdef __cplusplus
	extern "C" {
#endif
 
#include <pspkernel.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <psprtc.h>
#include <psptypes.h>

#include "AMG_triParticle.h"
#include "AMG_Model.h"
#include "AMG_Physics.h"	// + AMG_Texture.h
#include "AMG_Multimedia.h"
#include "AMG_3D.h"
extern int skip;
#define TRI_VERTFASTUVCF_FORMAT (GU_TEXTURE_8BIT|GU_COLOR_4444|GU_VERTEX_32BITF)


typedef struct triVertFastUVCf
{
	unsigned char		u, v;
	unsigned short		color;
	float	x, y, z;
} triVertFastUVCf;

//Struct to rotate the sprites
typedef struct{
	ScePspQuatMatrix		pos;		// the camera's position (triVec4 for vfpu accesses)
	ScePspQuatMatrix		dir;		// the camera's looking direction
	ScePspQuatMatrix		up;			// the camera's up vector
	ScePspQuatMatrix		right;		// the camera's right vector
} AMG_ParticleCamera ALIGN16;

typedef struct triParticleManager
{
	float				speed;
	triParticleSystem*		systems;
	signed long					numSystems;
	
	signed long					idCounter;
	
	unsigned long					numParticles;
	unsigned long					numVertices;
} triParticleManager;

ScePspQuatMatrix* triVec4Set( ScePspQuatMatrix* a, const float x, const float y, const float z, const float w )
{
	a->x = x;
	a->y = y;
	a->z = z;
	a->w = w;
	return a;
}

ScePspFColor* triColor4Set( ScePspFColor* c, const float r, const float g, const float b, const float a )
{
	c->r = r;
	c->g = g;
	c->b = b;
	c->a = a;
	return c;
}

ScePspQuatMatrix* triVec4Rndn( ScePspQuatMatrix* a );
ScePspFVector2* Vector2Rnd2( ScePspFVector2* a );
ScePspFVector3* triVec3Rnd2( ScePspFVector3* a );
ScePspQuatMatrix* triVec4Rnd( ScePspQuatMatrix* a );
ScePspQuatMatrix* triVec4Add3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c );
ScePspQuatMatrix* triVec4Sub3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c );
ScePspQuatMatrix* triVec4Mul3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c );
ScePspFVector3* triVec3Min(ScePspFVector3* a, const ScePspFVector3* b, const ScePspFVector3* c);
ScePspFVector3* triVec3Max(ScePspFVector3* a, const ScePspFVector3* b, const ScePspFVector3* c);
ScePspQuatMatrix* triVec4Scale3(ScePspQuatMatrix* a, const ScePspQuatMatrix* b, float t);
ScePspQuatMatrix* triQuatApply(ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c);
ScePspQuatMatrix* triQuatFromRotate(ScePspQuatMatrix* a, float angle, const ScePspQuatMatrix* b);
unsigned long triColor4f2RGBA8888( ScePspFColor* a );
float triVec4SquareLength3(const ScePspQuatMatrix* a);
void triParticleSystemConstructor( triParticleSystem* s );
void triParticleSystemFree( triParticleSystem* s );
signed long triParticleSystemRender( triParticleSystem* s );
void triParticleSystemInitialize( triParticleSystem* s, triParticleEmitter* e );
void triParticleSystemUpdate( triParticleSystem* s, float speed );
signed long triParticleVertexUVCListCreate( triParticleSystem* s);
void triParticleEmitterConstructor( triParticleEmitter *e, signed long emitterType );
triParticleSystem* triParticleManagerGet( signed long id );
triParticleSystem* AMG_ParticleSystem;
triParticleEmitter* AMG_ParticleEmitter;
AMG_ParticleCamera AMG_ParticleView;

triParticleManager TRI_PARTMAN = { 0, 0, 0 };
triBlendMode TRI_BLEND_MODE_ALPHA = { GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 };
triBlendMode TRI_BLEND_MODE_ADD = { GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF };
triBlendMode TRI_BLEND_MODE_GLENZ = { GU_ADD, GU_FIX, GU_FIX, 0x7F7F7F7F, 0x7F7F7F7F };
triBlendMode TRI_BLEND_MODE_ALPHA_ADD = { GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF };
triBlendMode TRI_BLEND_MODE_SUB = { GU_REVERSE_SUBTRACT, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF };
triBlendMode TRI_BLEND_MODE_ALPHA_SUB = { GU_REVERSE_SUBTRACT, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF };


void AMG_InitParticleSystem(int number){
	// FIXME: This is not the right way to do as it will cause the particle manager to try and free pointers from inside the array
	AMG_ParticleSystem = (triParticleSystem*)calloc( sizeof(triParticleSystem)*number,1);
	AMG_ParticleEmitter = (triParticleEmitter*)calloc( sizeof(triParticleEmitter)*number,1);
	
	int i;
	for (i = 0; i < number; i++) triParticleSystemConstructor( &AMG_ParticleSystem[i] );
}

void M3D_ParticleSystemInit(int number){
	AMG_InitParticleSystem(number);
}

void AMG_LoadParticleSystem(int number,char *path, signed long emitter_type, int fast, int blend){
	
	AMG_Texture *tex = AMG_LoadTexture(path,AMG_TEX_RAM,GU_PSM_4444);
	tex->WrapX = GU_CLAMP;tex->WrapY = GU_CLAMP;
	AMG_ParticleSystem[number].Texture = tex;

	triParticleEmitterConstructor(&AMG_ParticleEmitter[number], emitter_type);
	if(emitter_type == TRI_EMITTER_FLOATING) {
		//Simulate rain snow dust, use zbuffer to avoid weird things
		AMG_ParticleSystem[number].blendMode.op = 0;
	}
	AMG_ParticleSystem[number].blendMode.op = blend;
	if(fast) AMG_ParticleSystem[number].renderMode = GU_SPRITES;
	else AMG_ParticleSystem[number].renderMode = GU_TRIANGLES;
	
	triParticleSystem *p = &AMG_ParticleSystem[number];
	triParticleEmitter *e = &AMG_ParticleEmitter[number];
	triParticleSystemInitialize( p, e );
	TRI_PARTMAN.numSystems++;
	p->next = 0;
	TRI_PARTMAN.speed = 0;
	if (TRI_PARTMAN.systems==0) TRI_PARTMAN.systems = p;
	else {
		triParticleSystem* s = TRI_PARTMAN.systems;
		while (s->next!=0)
			s = s->next;
		s->next = p;
	}
	
	AMG_ParticleSystem[number].emitter_type = emitter_type;
	/*p->ID = ++TRI_PARTMAN.idCounter;
	p->updateFlag &= p->ID;
	return p->ID;*/
	
	//Default values
	e->pos = (ScePspQuatMatrix) {0,0,0,0};
	e->vel = (ScePspQuatMatrix) {0,0,0,0};
	e->vTexFrames = 1;
	e->hTexFrames = 1;
	e->nTexLoops = 0;
	e->gravity = (ScePspQuatMatrix) {0,0,0,0};
}

void M3D_LoadParticle(int number,const char *path, signed long emitter_type, int fast, int blend){
	AMG_LoadParticleSystem(number,(char*)path,emitter_type,fast,blend);
	AMG_ParticleEmitter[number].size = 0;
	AMG_ParticleEmitter[number].max = 0;
	AMG_ParticleEmitter[number].rate = 0;
	AMG_ParticleEmitter[number].life = 0;
	AMG_ParticleEmitter[number].lifetime = 0;
}

void AMG_SetTriEmitterPos(int emitter,float x, float y, float z){
	AMG_ParticleEmitter[emitter].pos = (ScePspQuatMatrix){x,y,z,0};
}

void M3D_ParticleSetPosition(int emitter,float x, float y, float z){
	AMG_ParticleEmitter[emitter].pos = (ScePspQuatMatrix){x,y,z,0};
}

void AMG_SetTriEmitterVel(int emitter,float x, float y, float z){
	AMG_ParticleEmitter[emitter].vel = (ScePspQuatMatrix){x,y,z,0};
}

void M3D_ParticleSetVelocity(int emitter,float vx, float vy, float vz){
	AMG_ParticleEmitter[emitter].vel = (ScePspQuatMatrix){vx,vy,vz,0};
}

void AMG_SetTriEmitterWnd(int emitter,float x, float y, float z){
	AMG_ParticleEmitter[emitter].vel = (ScePspQuatMatrix){x,y,z,0};
}

void AMG_SetTriEmitterGra(int emitter,float x, float y, float z){
	AMG_ParticleEmitter[emitter].gravity = (ScePspQuatMatrix){x,y,z,0};
}

void M3D_ParticleSetGravity(int emitter,float gx, float gy, float gz){
	AMG_ParticleEmitter[emitter].gravity = (ScePspQuatMatrix){gx,gy,gz,0};
}

void AMG_SetTriEmitterAni(int emitter,int hframes, int vframes, int loops){
	AMG_ParticleEmitter[emitter].hTexFrames = hframes;
	AMG_ParticleEmitter[emitter].vTexFrames = vframes;
	AMG_ParticleEmitter[emitter].nTexLoops = loops;
}

void M3D_ParticleSetAnimation(int emitter,int hframes, int vframes, int loops){
	AMG_ParticleEmitter[emitter].hTexFrames = hframes;
	AMG_ParticleEmitter[emitter].vTexFrames = vframes;
	AMG_ParticleEmitter[emitter].nTexLoops = loops;
}

void M3D_ParticleSetFilter(int emitter,int filter){
	AMG_Texture *tex = (AMG_Texture *) AMG_ParticleSystem[emitter].Texture;
	if (filter == 0){
		tex->Filter = GU_NEAREST; 
		tex->MipFilter = GU_NEAREST_MIPMAP_LINEAR;
	}
	if (filter == 1){
		tex->Filter = GU_LINEAR;
		tex->MipFilter = GU_LINEAR_MIPMAP_LINEAR;
	}
}


//Very simplified opentri particle system settings
void AMG_SetTriEmitterSet(int emitter, float size){
	u32 i;
	triParticleSystem *p = &AMG_ParticleSystem[emitter];
	triParticleEmitter *e = &AMG_ParticleEmitter[emitter];
	signed long emitter_type = AMG_ParticleSystem[emitter].emitter_type;
	AMG_ParticleEmitter[emitter].size = size;
	e->size = size;
	triParticleEmitterConstructor(&AMG_ParticleEmitter[emitter], emitter_type);
	
	if(emitter_type == M3D_PARTICLE_EXPLOSION) triParticleSystemInitialize( p, e );

}

void M3D_ParticleStart(int emitter, float size){
	AMG_SetTriEmitterSet(emitter,size);
}

void AMG_RenderParticles(float speed ){
	
	speed/=100;
	
	//Get view matrix
	ScePspFMatrix4 TRI_Viewmat;
	AMG_GetMatrix(GU_VIEW,&TRI_Viewmat);
	AMG_ParticleView.right.x = TRI_Viewmat.x.x;
	AMG_ParticleView.right.y = TRI_Viewmat.y.x;
	AMG_ParticleView.right.z = TRI_Viewmat.z.x;
	AMG_ParticleView.up.x = TRI_Viewmat.x.y;
	AMG_ParticleView.up.y = TRI_Viewmat.y.y;
	AMG_ParticleView.up.z = TRI_Viewmat.z.y;
	AMG_ParticleView.dir.x = TRI_Viewmat.x.z;
	AMG_ParticleView.dir.y = TRI_Viewmat.y.z;
	AMG_ParticleView.dir.z = TRI_Viewmat.z.z;
	
	triParticleSystem* s = TRI_PARTMAN.systems;
	TRI_PARTMAN.numParticles = 0;
	TRI_PARTMAN.numVertices = 0;
	//sceGuEnable(GU_LIGHT0);
	AMG_PushMatrix(GU_MODEL);
	//AMG_Rotate(GU_MODEL, &actor->Rot);
	AMG_UpdateMatrices();
	while (s!=0){
		triParticleSystemUpdate( s, speed+TRI_PARTMAN.speed );
		TRI_PARTMAN.speed = 0;
		if (!skip) triParticleSystemRender(s);
		TRI_PARTMAN.numParticles += s->numParticles;
		TRI_PARTMAN.numVertices += s->numVertices;
		s = s->next;
	}
	AMG_PopMatrix(GU_MODEL);
}

void M3D_ParticlesRender(float speed ){
	AMG_RenderParticles(speed);
}

void M3D_ParticleStop(int emitter){
	AMG_ParticleEmitter[emitter].size = 0;
	AMG_ParticleEmitter[emitter].max = 0;
	AMG_ParticleEmitter[emitter].rate = 0;
	AMG_ParticleEmitter[emitter].life = 0;
	AMG_ParticleEmitter[emitter].lifetime = 0;
}


//?????????????????????
void AMG_RemoveParticleManager( signed long id ){
	triParticleSystem* s = TRI_PARTMAN.systems;
	while (s!=0){
		if (s->ID == id){
			free(s->emitter);
			s->emitter = NULL;
			triParticleSystemFree(s);
			free(s);s = NULL;
			TRI_PARTMAN.numSystems--;
			return;
		}
		s = s->next;
	}
}

void triParticleEmitterConstructor( triParticleEmitter *e, signed long emitterType ){
	// Init to default values
	
	e->posRand = (ScePspQuatMatrix) {0,0,0,0};
	e->velRand = (ScePspQuatMatrix) {0.5,0.5,0.5,0};
	e->cols[0] = (ScePspFColor) {1,1,1,1};
	e->numCols = 1;
	//e->size = 0.5f;
	e->life = 5.0f;
	e->lifeRand = 0.2f;
	
	e->fixedTexRate = 0;
	e->min = 0;
	e->max = 32;
	e->rate = e->max/e->life;
	e->lifetime = 0;	// Eternal
	// Default no wind and no vortices
	e->wind = (ScePspQuatMatrix) {0,0,0,0};
	e->windRand = (ScePspQuatMatrix) {0,0,0,0};
	
	e->rateVortex = 0;
	e->minVortex = 0;
	e->maxVortex = 0;
	e->growth = 0.0f;
	e->sizeRand = 0.0f;
	e->burnout = 0.0f;
	e->binding = 0.0f;
	e->loosen = 0.0f;
	e->friction = 0.0f;
	e->vortexRange = 0.66f;
	e->glitter = 0.0f;
	e->glitterSpeed = 3.0f;
	e->lastpos = e->pos;
	e->age = 0;
	e->lastemission = 0;
	e->emitted = 0;
	e->emittedVortex = 0;
	int i;
	//If you ser color number >1 particles will fade out by defoult
	for(i = 1; i<8;i++) e->cols[i] = (ScePspFColor) {0,0,0,0};
	
	switch(emitterType){
		case TRI_EMITTER_FLOATING:
			e->posRand = (ScePspQuatMatrix) {e->size*10.0f,e->size*10.0f,e->size*10.0f,e->size*10.0f};
			e->vel = (ScePspQuatMatrix) {0,-0.9,0,0};
			e->velRand = (ScePspQuatMatrix) {0.05f, 0, 0.05f, 9.0f};
			e->sizeRand = 0.1f;
			e->friction = 0.01f;
			e->min = 0;
			e->minVortex = 0;
			e->maxVortex = 1;
			e->rateVortex = 1;			
			e->vortexDir = 0.1f;
			e->vortexDirRand = 0.5f;
			e->vortexRange = 1.33f;
			e->life = e->size*32.0f;	
			e->lifetime = 0;	// Eternal
		break;
		case TRI_EMITTER_EXPLOSION:
			e->velRand = (ScePspQuatMatrix){2.0f, 2.0f, 2.0f, 0.0f};
			e->growth = 2.0f;		// size of particle is 2 at end of life
			e->size = 0.1f;
			e->life = 1.0f;			//
			e->min = 0;
			e->max = 16;
			e->lifetime = 0.25f;		// lifetime of the emitter is 0.25 second - enough to blow out all particles and die before any respawn
			e->rate = e->max / e->lifetime;	// blow out all particles during the given lifetime
			e->cols[0] = (ScePspFColor){1,1,1,1};
			e->cols[1] = (ScePspFColor){1,1,1,1};
			e->cols[2] = (ScePspFColor){1,1,1,1};
			e->cols[3] = (ScePspFColor){1,1,1,0};
			e->numCols = 4;
		break;
		case TRI_EMITTER_SPRINKLE:
			e->glitter = 1;
			e->glitterSpeed = 30;
			e->velRand = (ScePspQuatMatrix) {0.2,0.2,0.2,0};
			e->growth = 0; //Final size
			e->gravity = (ScePspQuatMatrix) {0,-0.8,0};
			e->size = 0.3f;
			e->max = 64;
			e->rate = 12;
			e->life = 5;
			e->cols[0] = (ScePspFColor){1,1,1,1};
			e->cols[1] = (ScePspFColor){0,1,1,1};
			e->cols[2] = (ScePspFColor){0,0,1,1};
			e->cols[3] = (ScePspFColor){0,1,0,1};
			e->cols[4] = (ScePspFColor){1,1,1,1};
			e->cols[5] = (ScePspFColor){1,1,0,1};
			e->cols[6] = (ScePspFColor){1,1,1,1};
			e->cols[7] = (ScePspFColor){0,1,1,0};
			e->numCols = 8;
		break;
		case TRI_EMITTER_SMOKE:
			e->vel = (ScePspQuatMatrix) {0,0.4,0,0};
			e->cols[0] = (ScePspFColor){1,1,1,1};	// fade in
			e->cols[1] = (ScePspFColor){1,1,1,1};
			e->cols[2] = (ScePspFColor){1,1,1,1};
			e->cols[3] = (ScePspFColor){1,1,1,0};	// fade out
			e->numCols = 4;
			e->sizeRand = 0.2f;
			e->friction = 0.01f;
			e->min = 0;
			e->max = 32;
			e->rate = 4;
			e->size = 0.6;
			e->max = 32;
			e->lifetime = 0;
			e->life = 4;
			e->gravity.y = 0;
			e->velRand = (ScePspQuatMatrix) {0.2,0.2,0,4};
			e->posRand = (ScePspQuatMatrix) {0.1f, 0.1f, 0.1f, 2.0f};
			e->growth = 0.01;
			e->vortexRange = 0.33f;
		break;
	}
}




void triParticleSystemConstructor( triParticleSystem* s )
{
	if (s==0) return;

	s->emitter = 0;
	s->particles = 0;
	s->numParticles = 0;
	s->particleStack = 0;
	s->vortices = 0;
	s->vorticesStack = 0;
	s->numVortices = 0;
	
	s->vertices[0] = 0;
	s->vertices[1] = 0;
	s->numVertices = 0;
	
	s->Texture = NULL;
	s->renderMode = GU_SPRITES;
	s->texMode = GU_TFX_MODULATE;
	s->useBillboards = 1;
	s->updateFlag = 1;
	s->vindex = 0;
	memset( s->actions, 0, sizeof(s->actions) );
}


void triParticleSystemFree( triParticleSystem* s )
{
	if (s==0) return;

	s->emitter = 0;
	if (s->particles!=0) free(s->particles);
	s->particles = 0;
	s->numParticles = 0;
	if (s->particleStack!=0) free(s->particleStack);
	s->particleStack = 0;
	if (s->vortices!=0) free(s->vortices);
	s->vortices = 0;
	if (s->vorticesStack!=0) free(s->vorticesStack);
	s->vorticesStack = 0;
	s->numVortices = 0;
	
	if (s->vertices[0]!=0) free(s->vertices[0]);
	if (s->vertices[1]!=0) free(s->vertices[1]);
	s->vertices[0] = 0;
	s->vertices[1] = 0;
	s->numVertices = 0;
	
	s->Texture = NULL;
	s->renderMode = GU_SPRITES;
	s->texMode = GU_TFX_MODULATE;
	s->useBillboards = 0;
	memset( s->actions, 0, sizeof(s->actions) );
}


void triParticleSystemInitialize( triParticleSystem* s, triParticleEmitter* e )
{	
	s->emitter = e;

	s->particles = (triParticle*)malloc( sizeof(triParticle)*e->max );
	if (s->particles==0) return;
	
	s->particleStack = (long int*)malloc( sizeof(signed long)*e->max );
	if (s->particleStack==0) return;

	if (e->maxVortex>0)
	{
		s->vortices = (triVortex*)malloc( sizeof(triVortex)*e->maxVortex );
		if (s->vortices==0) return;
		
		s->vorticesStack = (long int*)malloc( sizeof(signed long)*e->maxVortex );
		if (s->vorticesStack==0) return;
	}
	s->numParticles = 0;
	s->numVortices = 0;
	
	signed long i;
	for (i=0;i<e->max;i++)
		s->particleStack[i] = i;
	for (i=0;i<e->maxVortex;i++)
		s->vorticesStack[i] = i;

	switch (s->renderMode)
	{
		case GU_POINTS: s->numVerticesPerParticle = 1; break;
		case GU_LINES:
		case GU_SPRITES: s->numVerticesPerParticle = 2; break;
		case GU_TRIANGLES:
		case GU_TRIANGLE_FAN:
		case GU_TRIANGLE_STRIP: s->numVerticesPerParticle = 6; break;
	}

		if (s->Texture==NULL) return;
		s->vertices[0] = malloc( sizeof(triVertFastUVCf)*e->max*s->numVerticesPerParticle );
		s->vertices[1] = malloc( sizeof(triVertFastUVCf)*e->max*s->numVerticesPerParticle );
		s->vertexFormat = TRI_VERTFASTUVCF_FORMAT;

		if (s->vertices[0]==0 || s->vertices[1]==0) return;
}


signed long triParticleSystemRender(triParticleSystem* s){
	if (s==0) return(0);
	
	if (s->numVertices==0) return(0);
	sceGumUpdateMatrix();

	sceGuColorMaterial(GU_DIFFUSE | GU_SPECULAR | GU_AMBIENT);		// Define los componentes materiales a usar	
	sceGuMaterial(GU_DIFFUSE, 0xffffffff); 
	sceGuMaterial(GU_SPECULAR, 0xff000000); 
	sceGuMaterial(GU_AMBIENT, 0xffffffff);
	sceGuColor(0x0fffffff); sceGuAmbient(0xffffffff); sceGuSpecular(0xff000000);
	AMG_EnableTexture((AMG_Texture*)s->Texture);
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	
	if (s->blendMode.op)sceGuDepthMask(1);//Disable Z buffer writes
	else sceGuDisable(GU_BLEND);
	//sceGuDisable(GU_CULL_FACE);
	sceGumDrawArray(s->renderMode,s->vertexFormat|GU_TRANSFORM_3D,s->numVertices,0,s->vertices[s->vindex]);
	//sceGuEndObject();
	//sceGuEnable(GU_CULL_FACE);
	if (s->blendMode.op)sceGuDepthMask(0);//Enable Z buffer writes
	else sceGuEnable(GU_BLEND);
	sceGuColor(0xffffffff);
	return(1);
}


static void triParticleRemove( triParticleSystem* s, signed long idx )
{
	if (s->numParticles<=0 || idx<0) return;
	s->numParticles--;
	if (s->numParticles==idx) return;
	signed long temp = s->particleStack[s->numParticles];
	s->particleStack[s->numParticles] = s->particleStack[idx];
	s->particleStack[idx] = temp;
	
}


static signed long triParticleAdd( triParticleSystem* s )
{
	if (s->numParticles>=s->emitter->max) return (-1);
	signed long idx = s->numParticles++;
	return idx;
}


static void triVortexRemove( triParticleSystem* s, signed long idx )
{
	if (s->numVortices<=0 || idx<0) return;
	s->numVortices--;
	if (s->numVortices==idx) return;
	signed long temp = s->vorticesStack[s->numVortices];
	s->vorticesStack[s->numVortices] = s->vorticesStack[idx];
	s->vorticesStack[idx] = temp;
	
}

static signed long triVortexAdd( triParticleSystem* s )
{
	if (s->numVortices>=s->emitter->maxVortex) return (-1);
	signed long idx = s->numVortices++;
	return idx;
}

/*
static float vcosf( float s )
{
	float ret;
	__asm__(
		"mtv	%1, s000\n"
		"vcst.s  s001, VFPU_2_PI\n"
        "vmul.s  s000, s000, s001\n"
		"vcos.s	s000, s000\n"
		"mfv	%0, s000\n"
	:"=r"(ret)
	:"r"(s)
	);
	return ret;
}*/


signed long triParticleUpdateLoadEmitter( triParticleEmitter* e, float speed )
{
	__asm__(
	"lv.q	c320, 32 +%0\n"			// e->lastPos
	"lv.s	s110, 256+%0\n"			// e->numCols
	"lv.q	c100, 272+%0\n"			// e->friction, e->growth, e->glitter, e->glitterSpeed
	
	"lv.s	s310, 296+%0\n"			// e->binding
	"lv.s	s311, 300+%0\n"			// e->loosen
	
	"vmov.p	c120, c102\n"			// c120 = e->glitter, e->glitterSpeed
	"mtv	%1, s102\n"				// speed
	
	"vmul.s s120, s120, s120[1/2]\n"// e->glitter*0.5
	"vmul.s s121, s121, s121[2]\n"	// e->glitterSpeed*2
	
	"vi2f.s s110, s110, 0\n"		// e->numCols
	"vsub.s	s110, s110, s110[1]\n"	// e->numCols-1
	
	"vscl.t	c100, c100[Y,1,X], s102\n"		// e->growth*speed, speed, e->friction*speed
	"vocp.s s102, s102\n"			// 1.0 - e->friction*speed
	::"m"(*e),"r"(speed)
	);
	return 1;
}

unsigned long min = 99999, max = 0;

ScePspQuatMatrix* triVec4Lerp(ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c, float t)
{
	__asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"mfc1			$8,   %3\n"				// t0   = t
		"mtv			$8,   s030\n"			// s030 = t0
		"lv.q			c010, %1\n"				// c010 = *b
		"lv.q			c020, %2\n"				// c020 = *c
		"vsub.q			c000, c020, c010\n"		// c000 = c020 - c010 = (v2 - v1)
		"vscl.q			c000, c000, s030\n"		// c000 = c000 * s030 = (v2 - v1) * t
		"vadd.q			c010, c010, c000\n"		// c010 = c010 + c000 = v1 + t * (v2 - v1)
		"sv.q			c010, %0\n"				// *a = c010
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c), "f"(t)
		: "$8"
	);
	return (a);
}

ScePspQuatMatrix temp_vel, temp_force;
signed long triParticleUpdate( triParticleEmitter* e, triParticle* p, ScePspQuatMatrix* f, float speed )
{
	if (p->age>=p->lifetime || p->size<=0.0f)
		return 0;

	// Faster at around 10-20 times without I-Cache and even more with I-Cache
	__asm__(
	// M000 = particle
	"lv.q	c000, 0+%1\n"
	"lv.q	c010, 16+%1\n"
	"lv.q	c030, 48+%1\n"			// p->size, p->age

	//"lv.q	c320, 32 +%2\n"			// e->lastPos
	"lv.q	c020, 128+%2\n"			// c020 = e->cols[0]
	//"lv.s	s110, 256+%2\n"			// e->numCols
	//"lv.q	c100, 272+%2\n"			// e->friction, e->growth, e->glitter, e->glitterSpeed

	"lv.q	c200, %3\n"				// force

	//"vmov.p	c120, c102\n"			// c120 = e->glitter, e->glitterSpeed

	//"mtv	%4, s102\n"				// speed

	"vrcp.s	s303, s033\n"			// 1.0 / p->lifetime

	//"vi2f.s s110, s110, 0\n"		// e->numCols
	//"vsub.s	s110, s110, s110[1]\n"	// e->numCols-1
	"vcmp.s	EZ, s110\n"
	
	//"vscl.t	c100, c100[Y,1,X], s102\n"		// e->growth*speed, speed, e->friction*speed
	//"vocp.s s102, s102\n"			// 1.0 - e->friction*speed
	"vadd.t c030, c030, c100[0,X,Y]\n"		// p->size += e->growth*speed, p->age += speed
	"vscl.q c200, c200, s101\n"		// force*e->firction
	"vmul.s s302, s032, s303\n"		// p->age / p->lifetime

	// Interpolate color (broken, done at the end )
	/*"bvt	4,	0f\n"
	"vmul.s	s410, s110, s302\n"		// s410 = age * (e->numCols-1)
	"vf2iz.s	s411, s410,	0\n"	// s411 = trunc(s410)
	"la		$9, %2\n"
	"mfv	$8, s411\n"
	"vi2f.s	s411, s411, 0\n"
	"sll	$8, $8, 4\n"			// $8 * 16
	"vsub.s s410, s410, s411\n"		// s410 = s410 - trunc(s410) = frac(s410)
	"addu	$8, $9, $8\n"
	"addiu  $8, $8, 128\n"

	"lv.q	c020,  0($8)\n"
	"lv.q	c130, 16($8)\n"

	"vocp.s	s411, s410\n"
	"vscl.q	c020, c020, s411\n"
	"vscl.q c130, c130, s410\n"
	"vadd.q c020, c130, c020\n"
	"0:\n"*/
	
	"vcmp.s EZ, s120\n"		// e->glitter = 0?
	//"lv.s	s310, 296+%2\n"			// e->binding
	//"lv.s	s311, 300+%2\n"			// e->loosen


	"vscl.q			c010, c010, s102\n"		// c010 = p->vel * friction
	
	// if (e->binding > 0.0f)
	//	triVec4Add3( &p->pos, &p->pos, triVec4Scale3( &temp_force, &e->lastpos, e->binding*(1.0f - (age * e->loosen)) ) );
	"vmul.s	s411, s311, s302\n"		// e->loosen*age
	"vscl.q			c210, c010, s101\n"
	"vocp.s s411, s411\n"
	"vadd.t			c010, c010, c200\n"
	"vmul.s s411, s411, s310\n"		// e->binding*(1.0f - (age * e->loosen))
	"vadd.q			c000, c000, c210\n"
	"vscl.t	c320, c320, s411\n"
	"vadd.t c000, c000, c320\n"

	// Glitter
	"bvtl	4,	1f\n"
	"vfim.s s123, 0.1\n"	// NOTE: precision problem?
	// p->col.a += e->glitter*0.5f*vcosf( (p->rand*0.1f+1.0f)*e->glitterSpeed*GU_PI*age );
	"vmul.s s122, s020, s123\n"	// p->rand*0.1
	"vmul.s s421, s121, s302\n"	// e->glitterSpeed * age
	//"vmul.s s121, s121, s123[2]\n" // e->glitterSpeed * age * 2
	"vadd.s s122, s122, s122[1]\n"	// p->rand*0.1+1.0
	
	"vmul.s s421, s421, s122\n"
	"vcos.s s421, s421\n"
	"vmul.s s420, s120, s421\n"	// e->glitter * 0.5 * vcos( ... )
	"vadd.s s023, s023, s420\n"
	"1:\n"

	"sv.q	c000, 0+ %0\n"
	"sv.q	c010, 16+%0\n"
	"sv.q	c020, 32+%0\n"
	"sv.q	c030, 48+%0\n"
	:"=m"(*p):"m"(*p),"m"(*e),"m"(*f)
	);

	//Update color here (asm part is broken on last sdk)
	float age = p->age / p->lifetime;
	if (e->numCols > 1){
		ScePspQuatMatrix col;
		float t = age * (e->numCols - 1);
		signed long i = (signed long)t;
		t -= i;
		triVec4Lerp(&col, (ScePspQuatMatrix*)&e->cols[i], (ScePspQuatMatrix*)&e->cols[i+1],t);
		memcpy(&p->col,&col,4*3);//Leave alpha (modified by glitter)
		p->col.a *= col.w;  
	}
	/*else p->col = e->cols[0];*/

	return 1;
}

static inline signed long triVortexUpdate( triParticleEmitter* e, triVortex* v, ScePspQuatMatrix* f, float speed )
{
	if (v->age>=v->lifetime)
		return 0;

	float friction = 1.0f - (e->friction * speed);
	triVec4Scale3( &v->vel, &v->vel, friction );
	triVec4Scale3( &temp_vel, &v->vel, speed );
	triVec4Scale3( &temp_force, f, speed );
	triVec4Add3( &v->vel, &v->vel, &temp_force );
	triVec4Add3( &v->pos, &v->pos, &temp_vel );

	//if (e->binding > 0.0)
	//	triVec4Add3( &v->pos, &v->pos, triVec4Scale3( &temp_force, &e->lastpos, e->binding*(1.0f - (age * e->loosen)) ) );

	v->age += speed;

	return 1;
}


//#define randf() ((float)(rand()-RAND_MAX/2)/(RAND_MAX/2))

// FIXME: Put the whole function in VFPU code
ScePspQuatMatrix __attribute__((aligned(16))) randVec;
static inline void triParticleCreate( triParticleEmitter* e, triParticle* p )
{
	p->col = e->cols[0];
	__asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q			c030, 0 + %1\n"			// c030 = e->pos
		"lv.q			c020, 16+ %1\n"			// c020 = e->posRand
		"lv.q			c130, 48+ %1\n"			// c130 = e->vel
		"lv.q			c120, 64+ %1\n"			// c020 = e->velRand
		"vrndf2.q		c000\n"					// c000 = rnd(2.0, 4.0)
		"vrndf2.q		c100\n"					// c100 = rnd(2.0, 4.0)
		"vsub.q			c000, c000, c000[3,3,3,3]\n"// c000 = | -1.0 ... 1.0 |
		"vsub.q			c100, c100, c100[3,3,3,3]\n"// c100 = | -1.0 ... 1.0 |
		"vdot.t			s010, c000, c000\n"		// s010 = x*x + y*y + z*z
		"vdot.t			s110, c100, c100\n"		// s110 = x*x + y*y + z*z
		"vrsq.s			s010, s010\n"			// s010 = 1.0 / s010
		"vrsq.s			s110, s110\n"			// s110 = 1.0 / s110
		"vscl.t			c000[-1:1,-1:1,-1:1], c000, s010\n"		// c000 = c000 / s010
		"vscl.t			c100[-1:1,-1:1,-1:1], c100, s110\n"		// c100 = c100 / s110
		"vmul.q			c000, c000, c020\n"
		"vmul.q			c100, c100, c120\n"
		"vadd.q			c030, c030, c000\n"
		"vadd.q			c130, c130, c100\n"
		"sv.q			c030, 0 + %0\n"
		"sv.q			c130, 16+ %0\n"
		
		/*
		"vrndf2.t		c000\n"					// c000 = rnd(2.0, 4.0)
		"vsub.t			c000, c000, c000[3,3,3]\n"	// c000 = | -1.0 ... 1.0 |
		
		"vadd.q			c010, c010, c000\n"
		"sv.q			c010, 32+ %0\n"*/		// (rand, size, age, lifetime)
		".set			pop\n"					// restore assember option
		: "=m"(*p)
		: "m"(*e)
	);
	//p->pos = e->pos;
	//p->vel = e->vel;
	// All four component since last component includes the rotation parameter
	//triVec4Add( &p->pos, &p->pos, triVec4Mul( &randVec, triVec4Rndn3( &randVec ), &e->posRand ) );
	//triVec4Add( &p->vel, &p->vel, triVec4Mul( &randVec, triVec4Rndn3( &randVec ), &e->velRand ) );
	ScePspFVector3 rv;
	triVec3Rnd2(&rv);
	memcpy(&randVec,&rv,3*4); 
	p->size = e->size + randVec.x*e->sizeRand;
	
	p->lifetime = e->life + randVec.y*e->lifeRand;	// lifetime
	p->age = 0.0f;
	p->rand = randVec.z;
	
	if (e->lifetime>0 && e->burnout>0)
	{
		// Have shorter particle lifes depending on emitter age
		float ef = e->age / e->lifetime;
		if (ef>1.0f) ef = 1.0f;

		p->lifetime *= (1.0f - e->burnout*ef*0.8f);		// lifetime * [0.2, 1.0]
	}
}

static inline void triVortexCreate( triParticleEmitter* e, triVortex* v )
{
	__asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q			c030, 0 + %1\n"			// c030 = e->pos
		"lv.q			c020, 16+ %1\n"			// c020 = e->posRand
		"lv.q			c130, 48+ %1\n"			// c130 = e->vel
		"lv.q			c120, 64+ %1\n"			// c020 = e->velRand
		"vrndf2.t		c000\n"					// c000 = rnd(2.0, 4.0)
		"vrndf2.t		c100\n"					// c100 = rnd(2.0, 4.0)
		"vsub.t			c000, c000, c000[3,3,3]\n"// c000 = | -1.0 ... 1.0 |
		"vsub.t			c100, c100, c100[3,3,3]\n"// c100 = | -1.0 ... 1.0 |
		"vdot.t			s010, c000, c000\n"		// s010 = x*x + y*y + z*z
		"vdot.t			s110, c100, c100\n"		// s110 = x*x + y*y + z*z
		"vrsq.s			s010, s010\n"			// s010 = 1.0 / s010
		"vrsq.s			s110, s110\n"			// s110 = 1.0 / s110
		"vscl.t			c000[-1:1,-1:1,-1:1], c000, s010\n"		// c000 = c000 / s010
		"vscl.t			c100[-1:1,-1:1,-1:1], c100, s110\n"		// c100 = c100 / s110
		"vmul.t			c000, c000, c020\n"
		"vmul.t			c100, c100, c120\n"
		"vadd.t			c030, c030, c000\n"
		"vadd.t			c130, c130, c100\n"
		"sv.q			c030, 0 + %0\n"
		"sv.q			c130, 16+ %0\n"

		".set			pop\n"					// restore assember option
		: "=m"(*v)
		: "m"(*e)
	);
	//v->pos = e->pos;
	//v->vel = e->vel;
	//triVec4Add3( &v->pos, &v->pos, triVec4Mul3( &randVec, triVec4Rndn3( &randVec ), &e->posRand ) );
	//triVec4Add3( &v->vel, &v->vel, triVec4Mul3( &randVec, triVec4Rndn3( &randVec ), &e->velRand ) );
	ScePspFVector2 rv;
	Vector2Rnd2(&rv);
	memcpy(&randVec,&rv,2*4);
	v->lifetime = e->life*1.5 + randVec.x*e->lifeRand*0.2f;	// lifetime (a tad bit longer than the particles and less random)
	v->age = 0.0f;
	
	v->confinement = e->vortexRange + randVec.y*e->vortexRange*0.2f;
	v->dir = e->vortexDir + randVec.y*e->vortexDirRand; // (randVec.y<0?(-0.6f + randVec.y*0.4f):(0.6f + randVec.y*0.4f));

	if (e->lifetime>0)
	{
		// Have shorter particle lifes depending on emitter age
		float ef = e->age / e->lifetime;
		if (ef>1.0f) ef = 1.0f;

		v->lifetime *= (1.0f - ef*0.8f);
	}
}

ScePspQuatMatrix force, localforce, tempvec, wind, diff, view, cross;
ScePspQuatMatrix vortRot;

void triParticleSystemUpdate( triParticleSystem* s, float speed )
{
	ScePspFMatrix4 RotMat;
	AMG_GetMatrix(GU_VIEW,&RotMat);
	if (s->emitter->lifetime>0 && s->emitter->age>=s->emitter->lifetime && s->numParticles==0)
		return;

	triVec4Sub3( &s->emitter->lastpos, &s->emitter->lastpos, &s->emitter->pos );
	
	force = s->emitter->gravity;
	// Update force according to global actions
	triVec4Add3( &s->emitter->wind, &s->emitter->wind, triVec4Mul3( &wind, triVec4Rnd( &randVec ), &s->emitter->windRand ) );
	wind = s->emitter->wind;
	triVec4Add3( &force, &force, &wind );

	signed long i, j;
	
	triParticleUpdateLoadEmitter( s->emitter, speed );
	for (i=0;i<s->numParticles;i++)
	{
		localforce = force;
		// Update localforce according to local actions
		
		if (triParticleUpdate( s->emitter, &s->particles[s->particleStack[i]], &localforce, speed )==0)
			triParticleRemove( s, i-- );
	}

	triVortex* v;
	for (j=0;j<s->numVortices;j++)
	{
		v = &s->vortices[s->vorticesStack[j]];

		triQuatFromRotate( &vortRot, v->dir*speed, &AMG_ParticleView.dir );
		
		triParticle* p;
		for (i=0;i<s->numParticles;i++)
		{
			p = &s->particles[s->particleStack[i]];
			if (triVec4SquareLength3(triVec4Sub3(&diff, &p->pos, &v->pos)) > v->confinement)
				continue;
			
			triQuatApply( &diff, &vortRot, &diff );
			float w = p->pos.w;
			triVec4Add3( &p->pos, &v->pos, &diff );
			p->pos.w = w;	// restore original rotation of particle
		}

		localforce = force;
		// Update localforce according to local actions
		if (triVortexUpdate( s->emitter, v, &localforce, speed )==0)
			triVortexRemove( s, j-- );
	}

	float rate = s->emitter->rate;
	float rateVortex = s->emitter->rateVortex;
	// Slow emitter rate based on emitter age
	if (s->emitter->lifetime>0)
	{
		rate *= (1.0f - s->emitter->burnout*s->emitter->age/s->emitter->lifetime);
		if (s->emitter->age >= s->emitter->lifetime)
		{
			rate = 0.0;
			rateVortex = 0.0;
		}
	}
	
	// Calculate correct emitter rate - needed for low rates at high frame rates -> rate*speed might be zero every frame!	
	if (s->emitter->lastemission>0)
	{
		float t = s->emitter->age + speed - s->emitter->lastemission;
		if (t>=1.0f)
		{
			s->emitter->emitted = 0;
			s->emitter->emittedVortex = 0;
			s->emitter->lastemission = s->emitter->age;
			t -= 1.0f;
		}
		rate = rate*t - s->emitter->emitted;
		rateVortex = rateVortex*t - s->emitter->emittedVortex;
	}
	else
	{
		s->emitter->emitted = 0;
		s->emitter->emittedVortex = 0;
		s->emitter->lastemission = s->emitter->age;
		rate = rate*speed;
		rateVortex = rateVortex*speed;
	}
	rate += 0.5f;
	rateVortex += 0.5f;

	for (i=0;i<(signed long)(rate);i++)
	{
		signed long idx = triParticleAdd( s );
		if (idx<0) break;
		
		triParticleCreate( s->emitter, &s->particles[s->particleStack[idx]] );
		s->emitter->emitted++;
	}


	for (i=0;i<(signed long)(rateVortex);i++)
	{
		signed long idx = triVortexAdd( s );
		if (idx<0) break;
		
		triVortexCreate( s->emitter, &s->vortices[s->vorticesStack[idx]] );
		s->emitter->emittedVortex++;
	}
	if (!skip) triParticleVertexUVCListCreate( s);
	
	s->emitter->lastpos = s->emitter->pos;
	s->emitter->age += speed;
}



ScePspQuatMatrix tleft, tright, templeft, tempright;
ScePspQuatMatrix rot;


signed long triParticleVertexUVCListCreate( triParticleSystem* s)
{
	s->vindex^=1;
	triVertFastUVCf* v = (triVertFastUVCf*)s->vertices[s->vindex];
	triParticle* p = s->particles;
	s->vertexFormat = TRI_VERTFASTUVCF_FORMAT;
	s->numVertices = 0;

	signed long nFrames = s->emitter->vTexFrames*s->emitter->hTexFrames;
	if (nFrames<1) nFrames = 1;
	
	float iu = 127.f / s->emitter->hTexFrames;
	float iv = 127.f / s->emitter->vTexFrames;
	
	signed long i = 0;

	ScePspFVector3 min = { 1e30f, 1e30f, 1e30f }, max = { -1e30f, -1e30f, -1e30f };
	
	// TODO: Add 16bit vertex positions by using last frames bounding box as approx. scale factor
	//       Needs an additional scale vector for the particle system so it can set scale matrix appropriately
	switch (s->renderMode)
	{
		case GU_SPRITES:
			s->numVertices = s->numParticles*2;
			triVec4Sub3( &tleft, &AMG_ParticleView.up, &AMG_ParticleView.right );
			for (;i<s->numParticles;i++)
			{
				p = &s->particles[s->particleStack[i]];
				signed long frame = 1;
				unsigned char u0 = 0, v0 = 0, u1 = 127, v1 = 127;
				// If there are more than 1 frame
				if (nFrames>1)
				{
					signed long hFrame = 0, vFrame = 0;
					// Animate the texture?
					if (s->emitter->nTexLoops>0)
					{
						if (s->emitter->fixedTexRate>0)
							frame = (signed long)(s->emitter->fixedTexRate*p->age) % nFrames;
						else
							frame = (signed long)(s->emitter->nTexLoops*nFrames*(p->age/p->lifetime)) % nFrames;
						
						hFrame = frame % s->emitter->hTexFrames;
						vFrame = frame / s->emitter->hTexFrames;
					}
					u0 = (unsigned char)(hFrame * iu);
					v0 = (unsigned char)(vFrame * iv);
					u1 = (unsigned char)((hFrame+1) * iu);
					v1 = (unsigned char)((vFrame+1) * iv);
				}
				float sz = p->size * 0.5f;
				unsigned long color = triColor4f2RGBA8888( &p->col );
				color = ((color & 0xF0) >> 4) | ((color &0xF000) >> 8) | ((color&0xF00000) >> 12) | ((color&0xF0000000) >> 16);
				v->u = u0;
				v->v = v0;
				v->color = color;
				triVec4Scale3( &tempvec, &tleft, sz );
				triVec3Max( &max, &max, (ScePspFVector3*)triVec4Add3( &templeft, &p->pos, &tempvec ) );
				ScePspFVector3 tl;
				memcpy(&tl,&templeft,3*4);
				triVec3Min( &min, &min, &tl);
				v->x = templeft.x;
				v->y = templeft.y;
				v->z = templeft.z;
				v++;
				v->u = u1;
				v->v = v1;
				v->color = color;
				ScePspFVector3 ts;
				ScePspQuatMatrix *tm = triVec4Sub3( &tempright, &p->pos, &tempvec );
				ts.x = tm->x; ts.y = tm->y; ts.z = tm->z;
				triVec3Min( &min, &min, &ts);
				triVec3Max( &max, &max, (ScePspFVector3*)&tempright );
				v->x = tempright.x;
				v->y = tempright.y;
				v->z = tempright.z;
				v++;
			}
			break;
		case GU_TRIANGLES:
		case GU_TRIANGLE_FAN:
		case GU_TRIANGLE_STRIP:
			s->renderMode = GU_TRIANGLES;
			s->numVertices = s->numParticles*6;
			triVec4Sub3( &tleft, &AMG_ParticleView.up, &AMG_ParticleView.right );
			triVec4Add3( &tright, &AMG_ParticleView.up,&AMG_ParticleView.right );
			// FIXME: Add quaternion based rotation for non-billboard particles
			for (;i<s->numParticles;i++)
			{
				p = &s->particles[s->particleStack[i]];
				float sz = p->size * 0.5f;
				unsigned long color = triColor4f2RGBA8888( &p->col );
				color = ((color & 0xF0) >> 4) | ((color &0xF000) >> 8) | ((color&0xF00000) >> 12) | ((color&0xF0000000) >> 16);
				
				signed long frame = 1;
				unsigned char u0 = 0, v0 = 0, u1 = 127, v1 = 127;
				// If there are more than 1 frame
				if (nFrames>1)
				{
					signed long hFrame = 0, vFrame = 0;
					// Animate the texture?
					if (s->emitter->nTexLoops>0)
					{
						if (s->emitter->fixedTexRate>0)
							frame = (signed long)(s->emitter->fixedTexRate*p->age) % nFrames;
						else
							frame = (signed long)(s->emitter->nTexLoops*nFrames*(p->age/p->lifetime)) % nFrames;
						
						hFrame = frame % s->emitter->hTexFrames;
						vFrame = frame / s->emitter->hTexFrames;
					}
					u0 = (unsigned char)(hFrame * iu);
					v0 = (unsigned char)(vFrame * iv);
					u1 = (unsigned char)((hFrame+1) * iu);
					v1 = (unsigned char)((vFrame+1) * iv);
				}
				
				if (p->pos.w!=0.0)
				{
					triQuatFromRotate( &rot, p->pos.w, &AMG_ParticleView.dir );
					triQuatApply( &templeft, &rot, triVec4Scale3( &templeft, &tleft, sz ) );
					triQuatApply( &tempright, &rot, triVec4Scale3( &tempright, &tright, sz ) );
					tempvec = templeft;
					v->u = u0;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + templeft.x;
					v->y = p->pos.y + templeft.y;
					v->z = p->pos.z + templeft.z;
					v++;
					v->u = u1;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + tempright.x;
					v->y = p->pos.y + tempright.y;
					v->z = p->pos.z + tempright.z;
					v++;
					triVec3Max( &max, &max, (ScePspFVector3*)triVec4Add3( &templeft, &p->pos, &templeft ) );
					ScePspFVector3 tl,tr;
					memcpy(&tl,&templeft,3*4);
					memcpy(&tr,&tempright,3*4);
					triVec3Min( &min, &min, &tl);
					triVec3Max( &max, &max, (ScePspFVector3*)triVec4Add3( &tempright, &p->pos, &tempright ) );
					triVec3Min( &min, &min, &tr);
					v->u = u1;
					v->v = v1;
					v->color = color;
					triQuatApply( &templeft, &rot, triVec4Scale3( &templeft, &tleft, -sz ) );
					triQuatApply( &tempright, &rot, triVec4Scale3( &tempright, &tright, -sz ) );
					v->x = p->pos.x + templeft.x;
					v->y = p->pos.y + templeft.y;
					v->z = p->pos.z + templeft.z;
					v++;
					
					
					v->u = u1;
					v->v = v1;
					v->color = color;
					v->x = p->pos.x + templeft.x;
					v->y = p->pos.y + templeft.y;
					v->z = p->pos.z + templeft.z;
					v++;
					v->u = u0;
					v->v = v1;
					v->color = color;
					v->x = p->pos.x + tempright.x;
					v->y = p->pos.y + tempright.y;
					v->z = p->pos.z + tempright.z;
					v++;
					v->u = u0;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + tempvec.x;
					v->y = p->pos.y + tempvec.y;
					v->z = p->pos.z + tempvec.z;
					v++;
					
					memcpy(&tl,&templeft,3*4);
					memcpy(&tr,&tempright,3*4);
					triVec3Max( &max, &max, (ScePspFVector3*)triVec4Add3( &templeft, &p->pos, &templeft ) );
					triVec3Min( &min, &min, &tl);
					triVec3Max( &max, &max, (ScePspFVector3*)triVec4Add3( &tempright, &p->pos, &tempright ) );
					triVec3Min( &min, &min, &tr );
				}
				else
				{
					v->u = u0;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + tleft.x*sz;
					v->y = p->pos.y + tleft.y*sz;
					v->z = p->pos.z + tleft.z*sz;
					triVec3Max( &max, &max, (ScePspFVector3*)&v->x );	// Hacky way of getting the vector - won't work with 16bit vertex positions any more
					triVec3Min( &min, &min, (ScePspFVector3*)&v->x );
					v++;
					v->u = u1;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + tright.x*sz;
					v->y = p->pos.y + tright.y*sz;
					v->z = p->pos.z + tright.z*sz;
					triVec3Max( &max, &max, (ScePspFVector3*)&v->x );
					triVec3Min( &min, &min, (ScePspFVector3*)&v->x );
					v++;
					v->u = u1;
					v->v = v1;
					v->color = color;
					v->x = p->pos.x - tleft.x*sz;
					v->y = p->pos.y - tleft.y*sz;
					v->z = p->pos.z - tleft.z*sz;
					triVec3Max( &max, &max, (ScePspFVector3*)&v->x );
					triVec3Min( &min, &min, (ScePspFVector3*)&v->x );
					v++;
	
					v->u = u1;
					v->v = v1;
					v->color = color;
					v->x = p->pos.x - tleft.x*sz;
					v->y = p->pos.y - tleft.y*sz;
					v->z = p->pos.z - tleft.z*sz;
					v++;
					v->u = u0;
					v->v = v1;
					v->color = color;
					v->x = p->pos.x - tright.x*sz;
					v->y = p->pos.y - tright.y*sz;
					v->z = p->pos.z - tright.z*sz;
					triVec3Max( &max, &max, (ScePspFVector3*)&v->x );
					triVec3Min( &min, &min, (ScePspFVector3*)&v->x );
					v++;
					v->u = u0;
					v->v = v0;
					v->color = color;
					v->x = p->pos.x + tleft.x*sz;
					v->y = p->pos.y + tleft.y*sz;
					v->z = p->pos.z + tleft.z*sz;
					v++;
				}
			}
			break;
	}
	s->boundingBox[0].x = min.x;
	s->boundingBox[0].y = min.y;
	s->boundingBox[0].z = min.z;

	s->boundingBox[1].x = max.x;
	s->boundingBox[1].y = min.y;
	s->boundingBox[1].z = min.z;

	s->boundingBox[2].x = max.x;
	s->boundingBox[2].y = min.y;
	s->boundingBox[2].z = max.z;

	s->boundingBox[3].x = min.x;
	s->boundingBox[3].y = min.y;
	s->boundingBox[3].z = max.z;


	s->boundingBox[4].x = min.x;
	s->boundingBox[4].y = max.y;
	s->boundingBox[4].z = min.z;

	s->boundingBox[5].x = max.x;
	s->boundingBox[5].y = max.y;
	s->boundingBox[5].z = min.z;

	s->boundingBox[6].x = max.x;
	s->boundingBox[6].y = max.y;
	s->boundingBox[6].z = max.z;

	s->boundingBox[7].x = min.x;
	s->boundingBox[7].y = max.y;
	s->boundingBox[7].z = max.z;
	
	sceKernelDcacheWritebackAll();
	return 1;
}


triParticleSystem* triParticleManagerGet( signed long id )
{
	triParticleSystem* s = TRI_PARTMAN.systems;
	while (s!=0)
	{
		if (s->ID == id)
		{
			return s;
		}
		s = s->next;
	}
	return(0);
}


void triParticleManagerDestroy()
{
	triParticleSystem* s = TRI_PARTMAN.systems;
	triParticleSystem* next = 0;
	while (s!=0)
	{
		next = s->next;
		free(s->emitter);
		s->emitter = NULL;
		AMG_UnloadTexture((AMG_Texture*)s->Texture);
		triParticleSystemFree(s);
		free(s);
		s = next;
	}
	TRI_PARTMAN.numSystems = 0;
	TRI_PARTMAN.systems = 0;
}


/******************************************************/
/* TRIPARTICLE ENGINE: VFPU FUNCTIONS *****************/
/******************************************************/

ScePspQuatMatrix* triVec4Rndn( ScePspQuatMatrix* a )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"vrndf2.q		c000\n"					// c000 = rnd(2.0, 4.0)
		"vsub.q			c000, c000, c000[3,3,3,3]\n"// c000 = | -1.0 ... 1.0 |
		"vdot.q			s010, c000, c000\n"		// s010 = x*x + y*y + z*z + w*w
		"vrsq.s			s010, s010\n"			// s010 = 1.0 / s010
		"vscl.q			c000[-1:1,-1:1,-1:1,-1:1], c000, s010\n"		// c000 = c000 / s010
		"sv.q			c000, %0\n"
		".set			pop\n"					// restore assember option
		: "=m"(*a)
	);
	return(a);
}


ScePspFVector2* Vector2Rnd2( ScePspFVector2* a )
{
	__asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"vrndf2.p		c000\n"					// c000 = rnd(2.0, 4.0)
		"vsub.p			c000, c000, c000[3,3]\n"// c000 = | -1.0 ... 1.0 |
		"sv.s			s000, %0\n"
		"sv.s			s001, 4 + %0\n"
		".set			pop\n"					// restore assember option
		: "=m"(*a)
	);
	return(a);
}

ScePspFVector3* triVec3Rnd2( ScePspFVector3* a )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"vrndf2.t		c000\n"					// c000 = rnd(2.0, 4.0)
		"vsub.t			c000, c000, c000[3,3,3]\n"	// c000 = | -1.0 ... 1.0 |
		"sv.s			s000, 0 + %0\n"
		"sv.s			s001, 4 + %0\n"
		"sv.s			s002, 8 + %0\n"
		".set			pop\n"					// restore assember option
		: "=m"(*a)
	);
	return(a);
}

ScePspQuatMatrix* triVec4Rnd( ScePspQuatMatrix* a )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"vone.q			c010\n"
		"vrndf1.q		c000\n"					// c000 = rnd(1.0, 2.0)
		"vsub.q			c000, c000, c010\n"// c000 = | 0.0 ... 1.0 |
		"sv.q			c000, %0\n"
		".set			pop\n"					// restore assember option
		: "=m"(*a)
	);
	return(a);
}


ScePspQuatMatrix* triVec4Add3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q 			c010, %1\n"				// c010 = *b
		"lv.q			c020, %2\n"				// c020 = *c
		"vadd.t			c010, c010, c020\n"		// c010 = c010 + c020
		"sv.q			c010, %0\n"				// *a = c010
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);
	return(a);
}

ScePspQuatMatrix* triVec4Sub3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q 			c010, %1\n"				// c010 = *b
		"lv.q			c020, %2\n"				// c020 = *c
		"vsub.t			c010, c010, c020\n"		// c010 = c010 - c020
		"sv.q			c010, %0\n"				// *a = c010
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);
	return(a);
}

ScePspQuatMatrix* triVec4Mul3( ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c )
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q 			c010, %1\n"				// c010 = *b
		"lv.q			c020, %2\n"				// c020 = *c
		"vmul.t			c010, c010, c020\n"		// c000 = c010 * c020
		"sv.q			c010, %0\n"				// *a = c000
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);
	return(a);
}

ScePspFVector3* triVec3Min(ScePspFVector3* a, const ScePspFVector3* b, const ScePspFVector3* c)
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.s			s010, 0 + %1\n"			// s010 = b->x
		"lv.s			s011, 4 + %1\n"			// s011 = b->y
		"lv.s			s012, 8 + %1\n"			// s012 = b->z
		"lv.s			s020, 0 + %2\n"			// s020 = c->x
		"lv.s			s021, 4 + %2\n"			// s021 = c->y
		"lv.s			s022, 8 + %2\n"			// s022 = c->z
		"vmin.t			c000, c010, c020\n"		// c000 = min(c010, c020)
		"sv.s			s000, 0 + %0\n"			// a->x = s000
		"sv.s			s001, 4 + %0\n"			// a->y = s001
		"sv.s			s002, 8 + %0\n"			// a->z = s002
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);

	return (a);
}

ScePspFVector3* triVec3Max(ScePspFVector3* a, const ScePspFVector3* b, const ScePspFVector3* c)
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.s			s010, 0 + %1\n"			// s010 = b->x
		"lv.s			s011, 4 + %1\n"			// s011 = b->y
		"lv.s			s012, 8 + %1\n"			// s012 = b->z
//		"lv.s			s020, 0 + %2\n"			// s020 = c->x
//		"lv.s			s021, 4 + %2\n"			// s021 = c->y
//		"lv.s			s022, 8 + %2\n"			// s022 = c->z
		"vmax.t			c000, c010, c020\n"		// c000 = max(c010, c020)
		"sv.s			s000, 0 + %0\n"			// a->x = s000
		"sv.s			s001, 4 + %0\n"			// a->y = s001
		"sv.s			s002, 8 + %0\n"			// a->z = s002
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);

	return (a);
}

ScePspQuatMatrix* triVec4Scale3(ScePspQuatMatrix* a, const ScePspQuatMatrix* b, float t)
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"mfc1			$8,   %2\n"				// t0   = t
		"mtv			$8,   s010\n"			// s010 = t0
		"lv.q			c000, %1\n"				// c000 = *b
		"vscl.t			c000, c000, s010\n"		// c000 = c000 * s010
		"sv.q			c000, %0\n"				// *a = c000
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "m"(*b), "f"(t)
		: "$8"
	);

	return (a);
}

ScePspQuatMatrix* triQuatApply(ScePspQuatMatrix* a, const ScePspQuatMatrix* b, const ScePspQuatMatrix* c)
{
	__asm__ volatile (
		".set			push\n"							// save assember option
		".set			noreorder\n"					// suppress reordering
		"lv.q			c100,  %1\n"					// c100 = *b
		"lv.q			c200,  %2\n"					// c200 = *c
		"vmov.q			c110, c100[-X,-Y,-Z,W]\n"		// c110 = (-s110, -s111, -s112, s113)
		"vqmul.q		c120, c100, c200\n"				// c120 = c200 * c100
		"vqmul.q		c000, c120, c110\n"				// c000 = c120 * c110
		"sv.q			c000, %0\n"						// *a = c000
		".set			pop\n"							// restore assember option
		: "=m"(*a)
		: "m"(*b), "m"(*c)
	);
	return (a);
}

ScePspQuatMatrix* triQuatFromRotate(ScePspQuatMatrix* a, float angle, const ScePspQuatMatrix* b)
{
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q			c000, %2\n"				// c000 = *b
		"mfc1			$8,   %1\n"				// t0   = angle
		"vdot.q			s010, c000, c000\n"		// s010 = s000*s000 + s001*s001 + s002*s002 + s003*s003
		"vcst.s			s020, VFPU_1_PI\n"		// s020 = VFPU_1_PI = 1 / PI
		"mtv			$8,   s021\n"			// s021 = t0 = angle
		"vmul.s			s020, s020, s021\n"		// s020 = s020 * s021 = angle * 0.5 * (2/PI)
		"vcos.s			s003, s020\n"			// s003 = cos(s020)
		"vsin.s			s020, s020\n"			// s020 = sin(s020)
		"vrsq.s			s010, s010\n"			// s010 = 1.0 / sqrt(s010)
		"vmul.s			s010, s010, s020\n"		// s010 = s010 * s020
		"vscl.t			c000, c000, s010\n"		// c000 = c000 * s010
		"sv.q			c000, %0\n"				// *a  = c000
		".set			pop\n"					// restore assember option
		: "=m"(*a)
		: "f"(angle), "m"(*b)
		: "$8"
	);

	return (a);
}

unsigned long triColor4f2RGBA8888( ScePspFColor* a )
{
	unsigned long c;
	__asm__ volatile (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q			c000, %1\n"				// c000 = *a
		"vsat0.q		c000,  c000\n"			// c000 = saturation to [0:1](c000)
		"viim.s			s010, 255\n"			// s010 = 255.0f
		"vscl.q			c000, c000, s010\n"		// c000 = c000 * 255.0f
		"vf2iz.q		c000, c000, 23\n"		// c000 = (int)c000 * 2^23
		"vi2uc.q		s000, c000\n"			// s000 = ((s003>>23)<<24) | ((s002>>23)<<16) | ((s001>>23)<<8) | (s000>>23)
		"mfv			%0,   s000\n"			// c    = s000
		".set			pop\n"					// restore assember option
		: "=&r"(c)
		: "m"(*a)
	);
	return (c);
}

float triVec4SquareLength3(const ScePspQuatMatrix* a)
{
	float f;
	__asm__ (
		".set			push\n"					// save assember option
		".set			noreorder\n"			// suppress reordering
		"lv.q			c000, %1\n"				// c000 = *a
		"vdot.t			s010, c000, c000\n"		// s010 = s000*s000 + s001*s001 + s002*s002
		"sv.s			s010, %0\n"				// f    = s010
		".set			pop\n"					// restore assember option
		: "=m"(f)
		: "m"(*a)
	);
	return (f);
}

#ifdef __cplusplus
	}
#endif