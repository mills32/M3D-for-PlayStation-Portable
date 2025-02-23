#ifndef _AMG_PHYSICS_H_
#define _AMG_PHYSICS_H_

#ifdef __cplusplus
	extern "C" {
#endif

// Includes
#include "AMG_Model.h"


// Define los tipos de objetos
#define AMG_BULLET_SHAPE_NONE 0
#define AMG_BULLET_SHAPE_BOX 1
#define AMG_BULLET_SHAPE_SPHERE 2
#define AMG_BULLET_SHAPE_CONE 3
#define AMG_BULLET_SHAPE_CYLINDER 4
#define AMG_BULLET_SHAPE_CONVEXHULL 5
#define AMG_BULLET_SHAPE_HEIGHTMAP 6		// No funciona todavía...

// Define los tipos de colisión
#define AMG_COLLISION_RAY 0x43524159; 
#define AMG_COLLISION_NONE 0xFFFFFFFF;

#ifdef AMG_DOC_ENGLISH
/**
 * @file AMG_Physics.h
 * @brief Physics-related functions (using Bullet)
 * @author Andrés Martínez (Andresmargar)
 */
#else
/**
 * @file AMG_Physics.h
 * @brief Funciones encargadas del motor físico (con Bullet)
 * @author Andrés Martínez (Andresmargar)
 */
#endif

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Inits the physics engine using the Open Source Library "Bullet"
 * @param world_size 3D world size (send NULL for an unlimited world)
 * @param max_objects Maximum number of 3D objects
 * @return It returns nothing
 */
#else
/**
 * @brief Inicializa el motor de físicas usando la librería Open Source "Bullet"
 * @param world_size El tamaño del mundo 3D (pasa NULL para un mundo sin límites)
 * @param max_objects El número máximo de objetos 3D
 * @return No devuelve nada
 */
#endif
void AMG_InitBulletPhysics(int world_size, u32 max_objects);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Set world gravity
 * @param x X component
 * @param y Y component
 * @param z Z component
 * @return It returns nothing
 */
#else
/**
 * @brief Establece la gravedad del mundo 3D
 * @param x El componente X del vector gravitatorio
 * @param y El componente Y del vector gravitatorio
 * @param z El componente Z del vector gravitatorio
 * @return No devuelve nada
 */
#endif
void AMG_SetWorldGravity(float x, float y, float z);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Perform a Ray-Tracing test
 * @param pos Ray origin
 * @param vec Ray direction
 * @return (X Y Z) coordinate at which the ray intersects the first colided object
 */
#else
/**
 * @brief Haz un test de Ray Tracing
 * @param pos Origen del rayo
 * @param vec El vector que forma el rayo
 * @return (X Y Z) coordenada a la que el rayo intersecta el primer objeto encontrado
 */
#endif
u32 AMG_RayTracingTest(ScePspFVector3 *pos, ScePspFVector3 *vec);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Links a 3D model to start simulation
 * @param model 3D model to link
 * @return It returns nothing
 * @note Parameters like mass and origin must be set BEFORE calling this function
 */
#else
/**
 * @brief Enlaza un modelo 3D para ser simulado
 * @param model El modelo 3D a enlazar
 * @return No devuelve nada
 * @note Parámetros como la masa y el origen se tiene que especificar ANTES de llamar a esta función
 */
#endif
void AMG_InitModelPhysics(AMG_Model *model);
void AMG_InitSkinnedActorPhysics(AMG_Skinned_Actor *actor, float mass);
void AMG_InitMorphingActorPhysics(AMG_Morphing_Actor *actor, float mass);
void AMG_InitBinaryMeshPhysics(AMG_BinaryMesh *model);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Delinks a 3D model from the physics engine
 * @param model The 3D model to delink
 * @return It returns nothing
 */
#else
/**
 * @brief Desenlaza un modelo 3D del motor de físicas
 * @param model El modelo 3D a desenlazar
 * @return No devuelve nada
 */
#endif
void AMG_DeleteModelPhysics(AMG_Model *model);
void AMG_DeleteSkinnedActorPhysics(AMG_Skinned_Actor *actor);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief
 * @return
 */
#else
/**
 * @brief
 * @return
 */
#endif

void AMG_InitVehiclePhysics(AMG_Model *model, float mass, float wradius, float wwidth, float wfriction, float xd, float yd);
void AMG_MoveVehicle(AMG_Model *model);


void AMG_GetVehiclePos(AMG_Model *model, ScePspFVector3 *position);
void AMG_GetVehicleRot(AMG_Model *model, ScePspFVector3 *rotation);


#ifdef AMG_DOC_ENGLISH
/**
 * @brief Update 3D physics
 * @return It returns nothing
 */
#else
/**
 * @brief Actualiza las físicas 3D
 * @return No devuelve nada
 */
#endif
void AMG_UpdateBulletPhysics(void);

#ifdef AMG_DOC_ENGLISH
/**
 * @brief Finishes Bullet Engine
 * @return It returns nothing
 */
#else
/**
 * @brief Termina con el motor Bullet
 * @return No devuelve nada
 */
#endif
void AMG_FinishBulletPhysics(void);

#ifdef __cplusplus
	}
#endif

#endif
