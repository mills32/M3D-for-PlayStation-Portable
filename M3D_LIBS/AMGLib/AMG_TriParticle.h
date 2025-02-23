/*
 * triParticle.h: Header for particle engine
 * This file is part of the "tri Engine".
 *
 * Copyright (C) 2007 tri
 * Copyright (C) 2007 Alexander Berl 'Raphael' <raphael@fx-world.org>
 *
 * $Id: $
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __cplusplus
	extern "C" {
#endif

#include <psptypes.h>
#include <pspmath.h>
#include <pspgum.h>

#define ALIGN16 __attribute__((aligned(16)))


typedef struct triParticle
{
	ScePspQuatMatrix		pos;			// (x,y,z,rotation)	- rotation only applicable in GU_TRIANGLES mode
	ScePspQuatMatrix		vel;			// (x,y,z,rotation) - rotation speed in degree/second

	ScePspFColor		col;
	float		rand;			// randomness factor of this particle (to randomize different emitter parameters)
	float		size;			// size of the particle, only applicable in GU_SPRITES and GU_TRIANGLES mode

	float		age;
	float		lifetime;
	//float		pad16;			// padding to 16byte alignment
} ALIGN16 triParticle;	// struct size: 16*4 bytes = 64 bytes


#define TRI_VORTEX_RANGE (3.0f)
typedef struct triVortex
{
	ScePspQuatMatrix		pos;
	ScePspQuatMatrix		vel;

	float		confinement;	// vorticity confinement value (how far the vortex ranges)
	float		dir;			// vortex rotation direction (+-) and speed

	float		age;
	float		lifetime;
} ALIGN16 triVortex;	// struct size: 16*4 bytes = 48 bytes


enum triParticleEmitterTypes
{
	TRI_EMITTER_DEFAULT = 0,
	TRI_EMITTER_FLOATING,
	TRI_EMITTER_EXPLOSION,
	TRI_EMITTER_SPRINKLE,
	TRI_EMITTER_SMOKE,
	TRI_EMITTER_NUM_TYPES
};

/*
 * General usage of values with randomness:
 *   result = value + (-1.0,..,1.0) * valueRand
 *
 * To generate values in range X - Y, set as follows:
 *   valueRand = (Y-X) / 2.0
 *   value = X + valueRand
 */
typedef struct triParticleEmitter
{
	ScePspQuatMatrix		pos;			// position of emitter and initial position of all emitted particles (x,y,z,rotation)
	ScePspQuatMatrix		posRand;		// length in all directions of random position (0 for no random placement) (x,y,z,rotation)
	ScePspQuatMatrix		lastpos;		// internal: last position of emitter (for moving particles with emitter)

	ScePspQuatMatrix		vel;			// initial velocity of all emitted particles (x,y,z,rotation)
	ScePspQuatMatrix		velRand;		// randomness of particle velocity (x,y,z,rotation)
	
	ScePspQuatMatrix		gravity;		// Gravital force on emitter particles

	ScePspQuatMatrix		wind;			// wind direction vector
	ScePspQuatMatrix		windRand;		// wind randomness

	ScePspFColor		cols[8];		// Color shades during lifetime of particle, max 8 currently
	signed long			numCols;		// Number of color fades

	float		size;			// Mean size of particles
	float		sizeRand;		// Random size (0 means no randomness)
	
	float		burnout;		// Burnout of the emitter with age. 1.0 means the emitters rate gradually turns towards 0 (and particles life towards ~20%) with age, 0 means no burnout

	float		friction;		// air friction to damp particle velocity, 0 = no friction, 1.0 = stop immediately (same as vel = 0)

	float		growth;			// Amount to grow particles (size/second) - Size after end of life = size + rand()*sizeRand + growth*life

	float		glitter;		// Amount of glitter on particles (sinusform brightening) - 0 means no glitter, 1.0 means glitter in full intensity range
	float		glitterSpeed;	// Speed of glitter (number of wavelengths inside particles age)

	float		life;			// Lifetime of particles to be created (lifeRand is 0.2 by default, ie 20%)
	float		lifeRand;		// Lifetime of particles randomness
	
	float		binding;		// binding of particles to emitter, 0 = no binding, 1.0 = particles move with emitter
	float		loosen;			// loosening of particles with age, 0 = no loosening, 1.0 = particles move completely free at end of life

	signed long			hTexFrames;		// number of horizontal texture frames (texture animation)
	signed long			vTexFrames;		// number of vertical texture frames (texture animation)
	signed long			nTexLoops;		// number of loops to do per particle Lifetime (0 means no texture animation at all)
	signed long			fixedTexRate;	// fixed texture animation frame rate in frames/second (0 if rate dependent on life - use nTexLoops*vTexFrames*hTexFrames/life)
	
	signed long			min;			// minimum number of particles at same time
	signed long			max;			// maximum number of particles at same time
	signed long			minVortex;		// minimum number of vortex particles at same time
	signed long			maxVortex;		// maximum number of vortex particles at same time
	
	float		vortexRange;	// The squared range of the vortices influence (+- 20%)
	float		vortexDir;		// Vortex direction (and speed)
	float		vortexDirRand;	// Vortex direction randomness

	signed long			rate;			// emission rate (particles/second) - rate*lifetime is amount of emitted particles in total
	signed long			rateVortex;		// vortex emission rate (vortices/second)
	
	float		lifetime;		// lifetime of emitter in seconds, 0 if eternal (fueled flame)
	float		age;			// internal: age of the emitter, if age > lifetime it will die unless lifetime = 0
	
	float		lastemission;	// internal: time of last emission (for proper emittance rate)
	signed long			emitted;		// internal: particles emitted since last emission
	signed long			emittedVortex;	// internal: vortices emitted since last emission
	float		padding[3];
} ALIGN16 triParticleEmitter;



typedef struct triBlendMode
{
	signed long		op;
	signed long		src_op;
	signed long		dst_op;
	unsigned long		src_fix;
	unsigned long		dst_fix;
} triBlendMode;


extern triBlendMode TRI_BLEND_MODE_ALPHA /*= { GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0 }*/;
extern triBlendMode TRI_BLEND_MODE_ADD /*= { GU_ADD, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF }*/;
extern triBlendMode TRI_BLEND_MODE_GLENZ /*= { GU_ADD, GU_FIX, GU_FIX, 0x7F7F7F7F, 0x7F7F7F7F }*/;
extern triBlendMode TRI_BLEND_MODE_ALPHA_ADD /*= { GU_ADD, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF }*/;
extern triBlendMode TRI_BLEND_MODE_SUB /* = { GU_SUBTRACT, GU_FIX, GU_FIX, 0xFFFFFFFF, 0xFFFFFFFF }*/;
extern triBlendMode TRI_BLEND_MODE_ALPHA_SUB /* = { GU_SUBTRACT, GU_SRC_ALPHA, GU_FIX, 0, 0xFFFFFFFF }*/;


enum triParticleActions {
	triApplyForce,					// apply external force (gravity or similar)
	triInternalGravity,				// calculate internal gravity and move particles accordingly
	triInternalCollide,				// collide particles against each other
	triEmitterBound,				// bind particles to the emitter - ie move them along with it based on age of particle
	triCollide,						// collide particles against exterior mesh
	triDie,							// let particles die (always applied)
	triNumActions
	};


typedef struct triParticleSystem triParticleSystem;


struct triParticleSystem
{
	signed long					ID;							// Particle system ID
	signed long					typeID;						// Particle system type ID - see triParticleEmitterTypes

	triParticleEmitter		*emitter;					// The emitter attached to this system
	signed long			emitter_type;

	void				*Texture;					// Texture to use for this particle system
	signed long					texMode;					// one of GU_TFX_*
	unsigned long					texColor;					// sceGuTexEnvColor
	triBlendMode			blendMode;
	signed long					renderMode;					// GU_POINTS, GU_LINES, GU_SPRITES, GU_TRIANGLES

	signed long					useBillboards;				// make particles always point towards camera, only applicable in GU_TRIANGLES_MODE

	unsigned long					actions[triNumActions];		// actions to apply every frame


	triParticle* 			particles;					// particle list for update/movement
	triVortex* 				vortices;					// vortex particle list
	signed long*					particleStack;
	signed long*					vorticesStack;
	signed long					numParticles;
	signed long					numVortices;

	signed long					numVerticesPerParticle;		// numbers to allocate the right amount of memory

	void* 					vertices[2];				// particle vertice list for rendering (created during update)
	signed long					numVertices;
	signed long					vertexFormat;
	signed long					vindex;

	ScePspFVector3				boundingBox[8];
	signed long					updateFlag;
	
	struct triParticleSystem*	next;
};

extern triParticleSystem *AMG_ParticleSystem;
extern triParticleEmitter *AMG_ParticleEmitter;


void AMG_InitParticleSystem(int number);
void AMG_LoadParticleSystem(int number,char *path, signed long emitter_type, int fast, int blend);

void AMG_SetTriEmitterPos(int emitter,float x, float y, float z);
void AMG_SetTriEmitterVel(int emitter,float x, float y, float z);
void AMG_SetTriEmitterWnd(int emitter,float x, float y, float z);
void AMG_SetTriEmitterGra(int emitter,float x, float y, float z);
void AMG_SetTriEmitterAni(int emitter,int hframes, int vframes, int loops);


void triParticleManagerDestroy();
void AMG_RenderParticles(float speed );
void AMG_RemoveParticleManager( signed long id );

#ifdef __cplusplus
	}
#endif