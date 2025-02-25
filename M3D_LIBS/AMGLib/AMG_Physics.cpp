// Includes
#include <stdio.h>
#include <pspgum.h>
#include "AMG_Physics.h"
#include "AMG_User.h"
extern int skip;


// Includes BULLET
#include <LinearMath/btVector3.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btQuickprof.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <LinearMath/btGeometryUtil.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

// Objetos principales
btDynamicsWorld* AMG_DynamicWorld;
btBroadphaseInterface* AMG_WorldBroadphase;
btCollisionDispatcher* AMG_WorldDispatcher;
btConstraintSolver*	AMG_PhysicsSolver;
btDefaultCollisionConfiguration* AMG_CollisionConfiguration;


typedef struct{
	void *md;
	btRigidBody *Body;
	btCollisionShape *Shape;
	btRaycastVehicle *m_vehicle;
	btVehicleRaycaster *m_vehicleRayCaster;
	btTypedConstraint *BallConstraint;
	btHingeConstraint *HingeConstraint;
	btTransform Transform;
	btVector3 Inertia;
	// Datos adicionales (btConvexHullShape)
	u32 ConvexVertices;
	btTriangleMesh *tri;
	btConvexShape *sh;
	btShapeHull *hull;
	int type; //0 = rigid; 1 = soft; 2 = Vehicle
	//Physics parameters
	float	wheelRadius;
	float	wheelWidth;
	float	wheelFriction;
	float	WheelsXDist;
	float	WheelsZDist;
	float	suspensionStiffness;//Rigidez de amortiguación
	float	suspensionDamping;//Valor de amortiguación
	float	suspensionCompression;//Fuerza de amortiguadores
	float	suspensionRestLength;//Longitud de amortiguador en reposo
	float	connectionHeight;//Altura de los ejes respecto al chasis
}amg_mdh;
amg_mdh amg_model_ptr[64];
u32 amg_max_objects = 0;

// Funciones locales
void amg_get_position(amg_mdh *obj, ScePspFVector3 &pos);
void amg_save_object_stack(void *md);
void amg_set_position(amg_mdh *obj, ScePspFVector3 &pos);

// Callback de BULLET
bool amg_bcallback(btManifoldPoint &cp, const btCollisionObjectWrapper *obj1, int id0, int idx0, const btCollisionObjectWrapper *obj2, int id1, int idx1){
	AMG_Object *o1 = (AMG_Object*) obj1->getCollisionObject()->getUserPointer();
	AMG_Object *o2 = (AMG_Object*) obj2->getCollisionObject()->getUserPointer();
	o1->Collision = true;
	o2->Collision = true;
	o1->CollidedWith = o2->material_name[0];
	o2->CollidedWith = o1->material_name[0];
	return false;
}

u32 M3D_ModelCheckCollision(M3D_Model *m,int obj_number,const void *name){
	AMG_Model *model = (AMG_Model*)m;
	if (!model) return 0;
	if (!model->Object[obj_number].Physics) return 0;
	u32 *n = (u32*) name;
	if (n[0] == model->Object[obj_number].CollidedWith) return 1;
	else return 0;
}

btVector3 AMG_world[2];
// Inicializa el motor BULLET
void AMG_InitBulletPhysics(int world_size, u32 max_objects){
	if(world_size){
		AMG_world[0] = btVector3(-world_size,-world_size,-world_size);
		AMG_world[1] = btVector3( world_size, world_size, world_size);
	}
	AMG_CollisionConfiguration = new btDefaultCollisionConfiguration();
	AMG_WorldDispatcher = new btCollisionDispatcher(AMG_CollisionConfiguration);
	if(world_size) AMG_WorldBroadphase = new btAxisSweep3(AMG_world[0], AMG_world[1], max_objects);
	else AMG_WorldBroadphase = new btDbvtBroadphase();
	AMG_PhysicsSolver = new btSequentialImpulseConstraintSolver();
	AMG_DynamicWorld = new btDiscreteDynamicsWorld (AMG_WorldDispatcher, AMG_WorldBroadphase, AMG_PhysicsSolver, AMG_CollisionConfiguration);
	AMG_DynamicWorld->setGravity(btVector3(0.0f, -9.8f, 0.0f));		// Gravedad por defecto (la terrestre)
	//Trying to inprove performance for the poor PSP
	btContactSolverInfo& info = AMG_DynamicWorld->getSolverInfo();
	info.m_numIterations = 1;
	info.m_timeStep = 1.0f;
	info.m_minimumSolverBatchSize = 4;
	AMG_DynamicWorld->setForceUpdateAllAabbs(false);
	// Crea la pila de modelos 3D
	amg_max_objects = 64;//max_objects;
	for(u32 i=0;i<amg_max_objects;i++){
		//amg_model_ptr[i] = (amg_mdh*) calloc (1, sizeof(amg_mdh));
		amg_model_ptr[i].md = NULL;
		amg_model_ptr[i].Body = NULL;
		amg_model_ptr[i].Shape = NULL;
		amg_model_ptr[i].m_vehicle = NULL;
		amg_model_ptr[i].m_vehicleRayCaster = NULL;
		amg_model_ptr[i].BallConstraint = NULL;
		amg_model_ptr[i].HingeConstraint = NULL;
		amg_model_ptr[i].tri = NULL;
		amg_model_ptr[i].sh = NULL;
		amg_model_ptr[i].hull = NULL;
	}
	// Establece el callback de colisión
	gContactAddedCallback = amg_bcallback;
}

void M3D_BulletInitPhysics(int world_size, u32 max_objects){
	AMG_InitBulletPhysics(world_size,max_objects);
}

// Establece la gravedad del mundo simulado
void AMG_SetWorldGravity(float x, float y, float z){
	AMG_DynamicWorld->setGravity(btVector3(x, y, z));
}

void M3D_BulletSetGravity(float x, float y, float z){
	AMG_DynamicWorld->setGravity(btVector3(x, y, z));
}

// Haz un test de Ray Tracing
u32 AMG_RayTracingTest(ScePspFVector3 *pos, ScePspFVector3 *vec){
	btCollisionWorld::ClosestRayResultCallback cb(btVector3(pos->x, pos->y, pos->z), btVector3(vec->x, vec->y, vec->z));
	AMG_DynamicWorld->rayTest(btVector3(pos->x, pos->y, pos->z), btVector3(pos->x+vec->x,pos->y+vec->y,pos->z+vec->z), cb);	
	if(cb.hasHit()){ //Collided
		AMG_Object *o = (AMG_Object*)(cb.m_collisionObject->getUserPointer());
		o->Collision = 1;
		o->CollidedWith = AMG_COLLISION_RAY;
		return o->material_name[0];//this is the same inside all structs
	} else return 0;
}

char M3D_NULL_MATERIAL[5] = {'N','O','N','E',0};

char *M3D_BulletRayTracingTest(ScePspFVector3 *pos, ScePspFVector3 *vec){
	btCollisionWorld::ClosestRayResultCallback cb(btVector3(pos->x, pos->y, pos->z), btVector3(vec->x, vec->y, vec->z));
	AMG_DynamicWorld->rayTest(btVector3(pos->x, pos->y, pos->z), btVector3(pos->x+vec->x,pos->y+vec->y,pos->z+vec->z), cb);	
	if(cb.hasHit()){ //Collided
		AMG_Object *o = (AMG_Object*)(cb.m_collisionObject->getUserPointer());
		o->Collision = 1;
		o->CollidedWith = AMG_COLLISION_RAY;
		return (char*)&o->material_name[0];//this is the same inside all structs
	} else return M3D_NULL_MATERIAL;
}

void AMG_ObjectConfPhysics(AMG_Object *obj, float mass, u32 shapetype){
	obj->Origin.x = 0; obj->Origin.y = 0; obj->Origin.z = 0;
	obj->Pos.x = 0; obj->Pos.y = 0; obj->Pos.z = 0;
	obj->Mass = mass; obj->ShapeType = shapetype;
	obj->isGround = (mass == 0.0f);
}

void M3D_ModelConfPhysics(M3D_Model *m, int obj_number, float mass, u32 shapetype){
	AMG_Model *model = (AMG_Model*)m;
	model->Object[obj_number].Origin.x = 0; model->Object[obj_number].Origin.y = 0; model->Object[obj_number].Origin.z = 0;
	model->Object[obj_number].Pos.x = 0; model->Object[obj_number].Pos.y = 0; model->Object[obj_number].Pos.z = 0;
	model->Object[obj_number].Mass = mass; model->Object[obj_number].ShapeType = shapetype;
	model->Object[obj_number].isGround = (mass == 0.0f);
}

// Inicializa las fisicas de un modelo 3D
void AMG_InitModelPhysics(AMG_Model *model){
	for(u8 i=0;i<model->NObjects;i++){
		if(model->Object[i].ShapeType == AMG_BULLET_SHAPE_NONE) continue;
		// Guarda el objeto en la pila
		amg_save_object_stack((void*)&model->Object[i]);
		// Obtén el objeto
		amg_mdh *obj = &amg_model_ptr[model->Object[i].bullet_id];
		// Calcula los márgenes
		float x, y, z;
		x = ((model->Object[i].BBox[1].x - model->Object[i].BBox[0].x)/2.0f)*model->Object[i].Scale.x;
		y = ((model->Object[i].BBox[1].y - model->Object[i].BBox[0].y)/2.0f)*model->Object[i].Scale.y;
		z = ((model->Object[i].BBox[1].z - model->Object[i].BBox[0].z)/2.0f)*model->Object[i].Scale.z;
		// Según el tipo de objeto que sea...
		switch(model->Object[i].ShapeType){
			case AMG_BULLET_SHAPE_BOX:
				obj->Shape = new btBoxShape(btVector3(btScalar(x), btScalar(y), btScalar(z)));
				break;
			case AMG_BULLET_SHAPE_SPHERE:
				obj->Shape = new btSphereShape(x);
				break;
			case AMG_BULLET_SHAPE_CONE:
				model->Object[i].Origin.y -= y;
				obj->Shape = new btConeShape(x, y*2.0f);
				break;
			case AMG_BULLET_SHAPE_CYLINDER:
				obj->Shape = new btCylinderShape(btVector3(x, y, z));
				break;
			case AMG_BULLET_SHAPE_CONVEXHULL:
			{
				// Guarda los triángulos en un buffer
				obj->tri = new btTriangleMesh();
				for(u32 a=0;a<(model->Object[i].NFaces);a++){
					ScePspFVector3 *t = &(((ScePspFVector3*)model->Object[i].Triangles)[a*3]);
					obj->tri->addTriangle(btVector3(t[0].x, t[0].y, t[0].z),
										  btVector3(t[1].x, t[1].y, t[1].z),
										  btVector3(t[2].x, t[2].y, t[2].z));
				}
				
				// Crea el Convex Hull
				if(model->Object[i].isGround){
					btBvhTriangleMeshShape *trimesh = new btBvhTriangleMeshShape(obj->tri, false);
					trimesh->buildOptimizedBvh();
					trimesh->setMargin(0); 
					obj->Shape = trimesh;
				}else{
					obj->sh = new btConvexTriangleMeshShape(obj->tri);
					obj->sh->setMargin(0); 
					obj->hull = new btShapeHull(obj->sh);
					obj->hull->buildHull(0);//obj->sh->getMargin()
					obj->sh->setUserPointer(obj->hull);
					btConvexHullShape *convexHull = new btConvexHullShape((btScalar*)obj->hull->getVertexPointer(), obj->hull->numVertices());
					convexHull->initializePolyhedralFeatures();
					obj->ConvexVertices = obj->hull->numVertices();
					obj->Shape = convexHull;
					
/*					
					float collisionMargin = 0.01f;
					
					btAlignedObjectArray<btVector3> planeEquations;
					btAlignedObjectArray<btVector3>
					btGeometryUtil::getPlaneEquationsFromVertices(obj->tri,planeEquations);

					btAlignedObjectArray<btVector3> shiftedPlaneEquations;
					for (int p=0;p<planeEquations.size();p++)
					{
						btVector3 plane = planeEquations[p];
						plane[3] += collisionMargin;
						shiftedPlaneEquations.push_back(plane);
					}
					btAlignedObjectArray<btVector3> shiftedVertices;
					btGeometryUtil::getVerticesFromPlaneEquations(shiftedPlaneEquations,shiftedVertices);

					
					btConvexHullShape* convexShape = new btConvexHullShape(&(shiftedVertices[0].getX()),shiftedVertices.size());
				*/
				}
				
				// Configura la escala
				obj->Shape->setLocalScaling(btVector3(model->Object[i].Scale.x, model->Object[i].Scale.y, model->Object[i].Scale.z));
			} break;
			default: AMG_Error((char*)"Wrong BULLET shape / Forma BULLET incorrecta",(char*)"model->Object[i].ShapeType");
		}
		obj->Transform.setIdentity();
		obj->Transform.setOrigin(btVector3(model->Object[i].Pos.x, model->Object[i].Pos.y, model->Object[i].Pos.z));
		btDefaultMotionState* MotionState = new btDefaultMotionState(obj->Transform);
		obj->Inertia = btVector3(0.0f, 0.0f, 0.0f);
		if(model->Object[i].Mass > 0.0f) obj->Shape->calculateLocalInertia(model->Object[i].Mass, obj->Inertia);
		obj->Body = new btRigidBody(model->Object[i].Mass, MotionState, obj->Shape, obj->Inertia);
		AMG_DynamicWorld->addRigidBody(obj->Body);
		if(!model->Object[i].isGround) obj->Body->activate();		// Activa el objeto
		obj->Body->setActivationState(WANTS_DEACTIVATION);
		obj->Body->setSleepingThresholds(0.6,0.6);
		
		// Establece el Callback
		obj->Body->setCollisionFlags(obj->Body->getCollisionFlags() | btCollisionObject::CF_DISABLE_SPU_COLLISION_PROCESSING/*| btCollisionObject::CF_STATIC_OBJECT*/);
		obj->Body->setUserPointer(&model->Object[i]);
		// Corrige el centro de rotación si es un cono
		if(model->Object[i].ShapeType == AMG_BULLET_SHAPE_CONE) model->Object[i].Origin.y += y;
	
		obj->Body->setDamping(0.1,0.1);
		//obj->Body->setMargin(0.21);
		//obj->Body->setFriction(0.2);
		//obj->Body->setRollingFriction(0.1);
		//obj->Body->setRestitution(0.1);
		model->Object[i].Physics = 1;
		obj->type = 0; //Rigid
	}
}

void M3D_ModelInitPhysics(M3D_Model *model){
	AMG_InitModelPhysics((AMG_Model*) model);
}

// Inicializa las fisicas de un modelo 3D
void AMG_InitBinaryMeshPhysics(AMG_BinaryMesh *model){
	// Guarda un objeto en la pila
	amg_save_object_stack((void*)&model->Object[0]);
	// Obtén el objeto
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	//ShapeType = AMG_BULLET_SHAPE_CONVEXHULL;
	//AMG_Vertex_TCV *Data;
	obj->tri = new btTriangleMesh();
	for(u32 a=0;a<model->Object[0].polyCount*3;a+=3){
		AMG_Vertex_TCV *t = &model->Object[0].Data[a];
		obj->tri->addTriangle(btVector3(t[0].x,t[0].y,t[0].z),
							  btVector3(t[1].x,t[1].y,t[1].z),
							  btVector3(t[2].x,t[2].y,t[2].z));
	}

	// Crea el Convex Hull
	//isGround
	btBvhTriangleMeshShape *trimesh = new btBvhTriangleMeshShape(obj->tri, false);
	trimesh->buildOptimizedBvh();
	trimesh->setMargin(0); 
	obj->Shape = trimesh;
	
	// Configura la escala
	obj->Shape->setLocalScaling(btVector3(model->Scale.x, model->Scale.y, model->Scale.z));

	obj->Transform.setIdentity();
	obj->Transform.setOrigin(btVector3(model->Pos.x, model->Pos.y, model->Pos.z));
	btDefaultMotionState* MotionState = new btDefaultMotionState(obj->Transform);
	obj->Inertia = btVector3(0.0f, 0.0f, 0.0f);
	obj->Body = new btRigidBody(0.0f, MotionState, obj->Shape, obj->Inertia);
	AMG_DynamicWorld->addRigidBody(obj->Body);
	obj->Body->setActivationState(WANTS_DEACTIVATION);
	obj->Body->setSleepingThresholds(0.6,0.6);
	
	// Establece el Callback
	obj->Body->setCollisionFlags(obj->Body->getCollisionFlags() | btCollisionObject::CF_DISABLE_SPU_COLLISION_PROCESSING| btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK| btCollisionObject::CF_STATIC_OBJECT);
	obj->Body->setUserPointer(&model->Object[0]);
	
	obj->Body->setDamping(0.1,0.1);
	//obj->Body->setMargin(0.21);
	//obj->Body->setFriction(0.2);
	//obj->Body->setRollingFriction(0.1);
	//obj->Body->setRestitution(0.1);
	model->Object[0].Physics = 1;
	obj->type = 0; //Rigid
}

void M3D_ModelBINInitPhysics(M3D_ModelBIN *model){
	AMG_InitBinaryMeshPhysics((AMG_BinaryMesh*)model);
}

// Inicializa las fisicas de un actor 3D
void AMG_InitSkinnedActorPhysics(AMG_Skinned_Actor *actor, float mass){
	// Guarda el objeto en la pila
	amg_save_object_stack((void*)&actor->Object[0]);
	// Obtén el objeto
	amg_mdh *obj = &amg_model_ptr[actor->Object[0].bullet_id];
	// Calcula los márgenes
	float x, y, z;
	x = ((actor->Object[0].BBox[1].x - actor->Object[0].BBox[0].x)/2.0f)*actor->Scale.x;
	y = ((actor->Object[0].BBox[1].y - actor->Object[0].BBox[0].y)/2.0f)*actor->Scale.y;
	z = ((actor->Object[0].BBox[1].z - actor->Object[0].BBox[0].z)/2.0f)*actor->Scale.z;
	//Siempre una esfera
	obj->Shape = new btSphereShape((x+y+z)/3);

	obj->Transform.setIdentity();
	obj->Transform.setOrigin(btVector3(actor->Pos.x, actor->Pos.y, actor->Pos.z));
	btDefaultMotionState* MotionState = new btDefaultMotionState(obj->Transform);
	obj->Inertia = btVector3(0.0f, 0.0f, 0.0f);
	actor->Object[0].Mass = mass;
	if(mass > 0.0f) obj->Shape->calculateLocalInertia(mass, obj->Inertia);
	obj->Body = new btRigidBody(mass, MotionState, obj->Shape, obj->Inertia);
	AMG_DynamicWorld->addRigidBody(obj->Body);
	obj->Body->activate();		// Activa el objeto
	obj->Body->setActivationState(WANTS_DEACTIVATION);
	obj->Body->setSleepingThresholds(0.6,0.6);
	
	// Establece el Callback
	obj->Body->setCollisionFlags(obj->Body->getCollisionFlags() | btCollisionObject::CF_DISABLE_SPU_COLLISION_PROCESSING| btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);//
	obj->Body->setUserPointer(&actor->Object[0]);
	
	obj->Body->setDamping(0,0);
	//obj->Body->setMargin(0.21);
	obj->Body->setFriction(0);
	obj->Body->setRollingFriction(10);
	obj->Body->setRestitution(0);
	actor->Object[0].phys = 1;
	obj->type = 0; //Rigid
}

void M3D_SkinnedActorInitPhysics(M3D_SkinnedActor *actor, float mass){
	AMG_InitSkinnedActorPhysics((AMG_Skinned_Actor*)actor,mass);
}

// Inicializa las fisicas de un actor Morphing 3D
void AMG_InitMorphingActorPhysics(AMG_Morphing_Actor *actor, float mass){
	actor->Object[0].Mass = mass;
	// Guarda un objeto en la pila
	amg_save_object_stack((void*)&actor->Object[0]);
	// Obtén el objeto
	amg_mdh *obj = &amg_model_ptr[actor->Object[0].bullet_id];
	
	// Calcula los márgenes
	float x, y, z;
	x = ((actor->Object[0].BBox[1].x - actor->Object[0].BBox[0].x)/2.0f)*actor->Scale.x;
	y = ((actor->Object[0].BBox[1].y - actor->Object[0].BBox[0].y)/2.0f)*actor->Scale.y;
	z = ((actor->Object[0].BBox[1].z - actor->Object[0].BBox[0].z)/2.0f)*actor->Scale.z;
	//Siempre una esfera
	obj->Shape = new btSphereShape((x+y+z)/3);
	
	obj->Transform.setIdentity();
	obj->Transform.setOrigin(btVector3(actor->Pos.x, actor->Pos.y, actor->Pos.z));
	
	btDefaultMotionState* MotionState = new btDefaultMotionState(obj->Transform);
	obj->Inertia = btVector3(0.0f, 0.0f, 0.0f);
	if(actor->Object[0].Mass > 0.0f) obj->Shape->calculateLocalInertia(actor->Object[0].Mass, obj->Inertia);
	
	obj->Body = new btRigidBody(actor->Object[0].Mass, MotionState, obj->Shape, obj->Inertia);
	AMG_DynamicWorld->addRigidBody(obj->Body);
	obj->Body->activate();		// Activa el objeto
	obj->Body->setActivationState(WANTS_DEACTIVATION);
	obj->Body->setSleepingThresholds(0.1,0.1);
	// Establece el Callback CF_DISABLE_SPU_COLLISION_PROCESSING
	obj->Body->setCollisionFlags(obj->Body->getCollisionFlags() | btCollisionObject::CF_DISABLE_SPU_COLLISION_PROCESSING | btCollisionObject::CF_CHARACTER_OBJECT/*CF_CUSTOM_MATERIAL_CALLBACK*/);
	obj->Body->setUserPointer(&actor->Object[0]);
	//setWorldTransform
	actor->Object[0].phys = 1;
	
	obj->Body->setDamping(0,1);
	obj->Body->setFriction(10);
	obj->Body->setRollingFriction(0);
	obj->Body->setRestitution(0);
}

void M3D_MorphingActorInitPhysics(M3D_MorphingActor *actor, float mass){
	AMG_InitMorphingActorPhysics((AMG_Morphing_Actor*)actor,mass);
}

// Quita un modelo 3D de la pila
void AMG_DeleteModelPhysics(AMG_Model *model){
	for(u8 i=0;i<model->NObjects;i++){		// Elimina de Bullet cada objeto 3D
		if(!model->Object[i].Physics) return;;
		if(model->Object[i].ShapeType == AMG_BULLET_SHAPE_NONE) return;
		amg_mdh *obj = &amg_model_ptr[model->Object[i].bullet_id];
		// Elimina datos adicionales
		if(obj->tri) delete obj->tri;
		if(obj->sh) delete obj->sh;
		if(obj->hull) delete obj->hull;
		// Eliminalos del motor Bullet
		AMG_DynamicWorld->removeRigidBody(obj->Body);
		delete obj->Body; obj->Body = NULL;
		delete obj->Shape; obj->Shape = NULL;
		model->Object[i].Mass = 0.0f; model->Object[i].isGround = 0;
		// Quitalos de la pila
		amg_model_ptr[model->Object[i].bullet_id].md = NULL;
		model->Object[i].bullet_id = 0;
		model->Object[i].Physics = 0;
	}
}

void M3D_ModelDeletePhysics(M3D_Model *model){
	AMG_DeleteModelPhysics((AMG_Model *)model);
}

// Quita un Actor 3D de la pila
void AMG_DeleteSkinnedActorPhysics(AMG_Skinned_Actor *actor){

	amg_mdh *obj = &amg_model_ptr[actor->Object[0].bullet_id];
	// Elimina datos adicionales
	if(obj->tri) delete obj->tri;
	if(obj->sh) delete obj->sh;
	if(obj->hull) delete obj->hull;
	// Eliminalos del motor Bullet
	AMG_DynamicWorld->removeRigidBody(obj->Body);
	delete obj->Body; obj->Body = NULL;
	delete obj->Shape; obj->Shape = NULL;
	actor->Object[0].Mass = 0.0f;
	// Quitalos de la pila
	amg_model_ptr[actor->Object[0].bullet_id].md = NULL;
	actor->Object[0].bullet_id = 0;
	actor->Object[0].phys = 0;
}

void AMG_UpdateBody(AMG_Object *model){
	btQuaternion __attribute__((aligned(64))) q;
	btQuaternion __attribute__((aligned(64))) qe;
	ScePspQuatMatrix __attribute__((aligned(64))) q2;
	ScePspQuatMatrix __attribute__((aligned(64))) q3;
	btVector3 center;
	btVector3 pos;
	amg_mdh *obj = &amg_model_ptr[model->bullet_id];
	//If rigid body 
	if (obj->type == 0){
		obj->Body->activate();
		center = obj->Body->getWorldTransform().getOrigin();
		memcpy(&pos,&model->Pos,sizeof(btVector3));
		memcpy(&model->Pos,&center,sizeof(ScePspFVector3));
		model->Pos.x = (float)center[0];
		model->Pos.y = (float)center[1];
		model->Pos.z = (float)center[2];
		q = obj->Body->getOrientation();
		q2.x = q[0]; q2.y = q[1]; q2.z = q[2]; q2.w = -q[3];//WHY!!! -q[3] ? two days to realize
		AMG_Translate(GU_MODEL, &model->Pos);
		if (model->Mass != 0)AMG_RotateQuat(GU_MODEL,&q2);
		else {
			//Get rotation values from user
			qe.setEuler(model->Rot.y,model->Rot.x,model->Rot.z);
			memcpy(&q3,&qe,sizeof(ScePspQuatMatrix));
			//set rotation to body
			obj->Body->getWorldTransform().setRotation(qe);
			//set position to body
			obj->Body->getWorldTransform().setOrigin(pos);
			//Set rotation to model
			q3.w = -q3.w;
			AMG_RotateQuat(GU_MODEL,&q3);
		}
	}
	//Reset collisions
	//model->Collision = false;
	//model->CollidedWith = 0;
	//obj->md->Collision = 0;
	//obj->md->CollidedWith = 0xFFFF;
}

void AMG_UpdateBINBody(AMG_BinaryMesh *model){
	btQuaternion __attribute__((aligned(64))) q;
	btQuaternion __attribute__((aligned(64))) qe;
	ScePspQuatMatrix __attribute__((aligned(64))) q2;
	ScePspQuatMatrix __attribute__((aligned(64))) q3;
	btVector3 pos;
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];

	obj->Body->activate();
	memcpy(&pos,&model->Pos,sizeof(btVector3));
	AMG_Translate(GU_MODEL, &model->Pos);
	//Get rotation values from user
	qe.setEuler(model->Rot.y,model->Rot.x,model->Rot.z);
	memcpy(&q3,&qe,sizeof(ScePspQuatMatrix));
	//set rotation to body
	obj->Body->getWorldTransform().setRotation(qe);
	//set position to body
	obj->Body->getWorldTransform().setOrigin(pos);
	//Set rotation to model
	q3.w = -q3.w;
	AMG_RotateQuat(GU_MODEL,&q3);

	//Reset collisions
	//model->Collision = false;
	//model->CollidedWith = 0;
	//obj->md->Collision = 0;
	//obj->md->CollidedWith = 0xFFFF;
}

void AMG_UpdateSkinnedActorBody(AMG_Skinned_Actor *actor){
	float x, y, z;
	btVector3 center;
	btQuaternion __attribute__((aligned(64))) qe;
	ScePspQuatMatrix __attribute__((aligned(64))) q3;
	amg_mdh *obj = &amg_model_ptr[actor->Object[0].bullet_id];
	
	obj->Body->activate();
	center = obj->Body->getWorldTransform().getOrigin();
	actor->Pos.x = (float)center[0];
	actor->Pos.z = (float)center[2];
	x = ((actor->Object[0].BBox[1].x - actor->Object[0].BBox[0].x)/2.0f)*actor->Scale.x;
	y = ((actor->Object[0].BBox[1].y - actor->Object[0].BBox[0].y)/2.0f)*actor->Scale.y;
	z = ((actor->Object[0].BBox[1].z - actor->Object[0].BBox[0].z)/2.0f)*actor->Scale.z;
	actor->Pos.y = (float)center[1] - (float)((x+y+z)/3.0f);
	//Get rotation values from user
	/*qe.setEuler(actor->Rot.y,actor->Rot.x,actor->Rot.z);
	memcpy(&q3,&qe,sizeof(ScePspQuatMatrix));
	q3.w = -q3.w;*/
	//Set rotation to model
	//AMG_RotateQuat(GU_MODEL,&q3);
	//obj->md->Collision = 0;
	//obj->md->CollidedWith = 0xFFFF;
}

void AMG_UpdateMorphingActorBody(AMG_Morphing_Actor *actor){

	btVector3 center;
	btQuaternion __attribute__((aligned(64))) qe;
	ScePspQuatMatrix __attribute__((aligned(64))) q3;
	amg_mdh *obj = &amg_model_ptr[actor->Object[0].bullet_id];
	
	obj->Body->activate();
	center = obj->Body->getWorldTransform().getOrigin();
	memcpy(&actor->Pos,&center,sizeof(ScePspFVector3));
	float x, y, z;
	x = ((actor->Object[0].BBox[1].x - actor->Object[0].BBox[0].x)/2.0f)*actor->Scale.x;
	y = ((actor->Object[0].BBox[1].y - actor->Object[0].BBox[0].y)/2.0f)*actor->Scale.y;
	z = ((actor->Object[0].BBox[1].z - actor->Object[0].BBox[0].z)/2.0f)*actor->Scale.z;
	actor->Pos.y = actor->Pos.y-((x+y+z)/3);
	//Get rotation values from user
	qe.setEuler(actor->Rot.y,actor->Rot.x,actor->Rot.z);
	memcpy(&q3,&qe,sizeof(ScePspQuatMatrix));
	q3.w = -q3.w;
	//Set rotation to model
	AMG_RotateQuat(GU_MODEL,&q3);
	//obj->md->Collision = 0;
	//obj->md->CollidedWith = 0xFFFF;
}

void AMG_InitVehiclePhysics(AMG_Model *model, float mass, float wradius, float wwidth, float wfriction, float xd, float yd){
	model->Object[0].Mass = mass;
	model->Object[0].Physics = 1;
	// Guarda el objeto en la pila
	amg_save_object_stack(&model->Object[0]);
	/// create vehicle
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	
	//drive parameters
	model->Object[0].Steering = 0;
	model->Object[0].SteeringInc = 0;
	model->Object[0].SteeringClamp = 0.1;
	model->Object[0].Engine = 0;
	model->Object[0].Brake = 0;
	//Physics parameters
	obj->wheelRadius = wradius;
	obj->wheelWidth = wwidth;
	obj->WheelsXDist = xd;
	obj->WheelsZDist = yd;
	obj->wheelFriction = wfriction;

	obj->suspensionDamping = 2;//Valor de amortiguación
	obj->suspensionStiffness = 40;//Rigidez de amortiguación
	obj->suspensionCompression = 10;//Fuerza de amortiguación
	obj->connectionHeight = -0.04;//Altura de los ejes respecto al chasis
	
	
	obj->Transform.setIdentity();
	//Box shape has a size defined by wheels position
	float X = obj->WheelsXDist*2; float Y = obj->connectionHeight/3; float Z = obj->WheelsZDist*2;
	obj->Shape = new btBoxShape(btVector3(btScalar(X), btScalar(Y), btScalar(Z)));
	btDefaultMotionState* myMotionState = new btDefaultMotionState(obj->Transform);
	btVector3 localInertia(0,0,0);
	if (model->Object[0].Mass != 0) obj->Shape->calculateLocalInertia(model->Object[0].Mass,localInertia);
	obj->Body = new btRigidBody(model->Object[0].Mass,myMotionState,obj->Shape,localInertia);
	obj->Transform.setOrigin(btVector3(model->Object[0].Pos.x,model->Object[0].Pos.y,model->Object[0].Pos.z));
	
	obj->Body->setWorldTransform(obj->Transform);
	AMG_DynamicWorld->addRigidBody(obj->Body);
	
	btRaycastVehicle::btVehicleTuning m_tuning;
	obj->m_vehicleRayCaster = new btDefaultVehicleRaycaster(AMG_DynamicWorld);
	obj->m_vehicle = new btRaycastVehicle(m_tuning,obj->Body,obj->m_vehicleRayCaster);
	
	//Deactivate the vehicle if not moving.
	//"WANTS_DEACTIVATION" always disables vehicle if I compile the libs with my windows 10 x64.
	obj->Body->setActivationState(DISABLE_DEACTIVATION);
	obj->Body->setSleepingThresholds(0.1,0.1);
	
	AMG_DynamicWorld->addVehicle(obj->m_vehicle);
	//btBroadphaseProxy::m_collisionFilterGroup
	//choose coordinate system
	obj->m_vehicle->setCoordinateSystem(0,1,2);
	
	//Add wheels in this order: Front left, front right, back left, back right
	//Params: position,axis0,axis1,suspensionRestLength,wheelRadius,vehicle tuning
	btVector3 wheelAx0(0,-1,0);
	btVector3 wheelAx1(-1,0,0);
	float suspensionRestLength = abs(obj->connectionHeight)/2;//Longitud de amortiguador en reposo
	//Position
	obj->m_vehicle->addWheel(btVector3(-obj->WheelsXDist,obj->connectionHeight,-obj->WheelsZDist),wheelAx0,wheelAx1,suspensionRestLength,obj->wheelRadius,m_tuning,1);
	obj->m_vehicle->addWheel(btVector3(obj->WheelsXDist,obj->connectionHeight,-obj->WheelsZDist),wheelAx0,wheelAx1,suspensionRestLength,obj->wheelRadius,m_tuning,1);
	obj->m_vehicle->addWheel(btVector3(-obj->WheelsXDist,obj->connectionHeight,obj->WheelsZDist),wheelAx0,wheelAx1,suspensionRestLength,obj->wheelRadius,m_tuning,0);
	obj->m_vehicle->addWheel(btVector3(obj->WheelsXDist,obj->connectionHeight,obj->WheelsZDist),wheelAx0,wheelAx1,suspensionRestLength,obj->wheelRadius,m_tuning,0);

	for (int i=0;i<obj->m_vehicle->getNumWheels();i++){
		btWheelInfo& wheel = obj->m_vehicle->getWheelInfo(i);
		wheel.m_suspensionStiffness = obj->suspensionStiffness;
		wheel.m_wheelsDampingRelaxation = obj->suspensionDamping;
		wheel.m_wheelsDampingCompression = obj->suspensionCompression;
		wheel.m_frictionSlip = obj->wheelFriction;
		wheel.m_rollInfluence = 0;
	}
	// Establece el Callback
	obj->Body->setCollisionFlags(obj->Body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	obj->Body->setUserPointer(&model->Object[0]);
}

void M3D_VehicleInitPhysics(M3D_Model *model, float mass, float wradius, float wwidth, float wfriction, float xd, float yd){
	AMG_InitVehiclePhysics((AMG_Model*)model,mass,wradius,wwidth,wfriction, xd,yd);
}

void AMG_UpdateVehicleBody(AMG_Model *model){
	btScalar x, y, z;
	btQuaternion __attribute__((aligned(16))) q;
	ScePspQuatMatrix __attribute__((aligned(16))) q2;
	ScePspFVector3 __attribute__((aligned(16))) center;
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	memcpy(&center,&obj->m_vehicle->getChassisWorldTransform().getOrigin(),sizeof(ScePspFVector3));
	q = obj->m_vehicle->getChassisWorldTransform().getRotation();
	//obj->m_vehicle->getChassisWorldTransform().getBasis().getEulerZYX(z,y,x,1);
	q2.x = q[0]; q2.y = q[1]; q2.z = q[2]; q2.w = -q[3];

	//model->Object[0].Rot.x = x;
	//model->Object[0].Rot.y = y;
	//model->Object[0].Rot.z = z;
	AMG_Translate(GU_MODEL, &center);
	AMG_RotateQuat(GU_MODEL,&q2);
	//Update object position and rotation in struct
	memcpy(&model->Object[0].Pos,&center,sizeof(ScePspFVector3));
	memcpy(&model->Object[0].VehicleRotation,&q2,sizeof(ScePspQuatMatrix));
}

void AMG_UpdateVehicleWheel(AMG_Model *model, int i){
	btQuaternion __attribute__((aligned(16))) q;
	ScePspQuatMatrix __attribute__((aligned(16))) q2;
	ScePspFVector3 __attribute__((aligned(16))) c;
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	//Get position and rotation
	memcpy(&c,&obj->m_vehicle->getWheelInfo(i).m_worldTransform.getOrigin(),sizeof(ScePspFVector3));
	q = amg_model_ptr[model->Object[0].bullet_id].m_vehicle->getWheelInfo(i).m_worldTransform.getRotation();
	q2.x = q[0]; q2.y = q[1]; q2.z = q[2]; q2.w = -q[3];
	//Update wheel i
	obj->m_vehicle->updateWheelTransform(i,1);
	//Get size
	ScePspFVector3 wscal __attribute__((aligned(16))) = {obj->wheelWidth*2,obj->wheelRadius*2,obj->wheelRadius*2};
	AMG_Translate(GU_MODEL, &c);
	AMG_RotateQuat(GU_MODEL,&q2);
	AMG_Scale(GU_MODEL, &wscal);
}

void amg_get_rotation(amg_mdh *obj, ScePspFVector3 &rot);
void AMG_MoveVehicle(AMG_Model *model){
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	//float steering_inc = model->Object[0].SteeringInc;
	//if (steering_inc == 0) model->Object[0].Steering = 0;
	//model->Object[0].Steering += model->Object[0].SteeringInc;
	if (model->Object[0].Steering > model->Object[0].SteeringClamp) model->Object[0].Steering = model->Object[0].SteeringClamp;
	if (model->Object[0].Steering < -model->Object[0].SteeringClamp) model->Object[0].Steering = -model->Object[0].SteeringClamp;
	
	float vmax = model->Object[0].vmax;
	int max = 0;
	// Obtén la velocidad lineal de un objeto 3D
	btVector3 v = obj->Body->getLinearVelocity();
	float x = v.getX();float y = v.getY();float z = v.getZ();
	if ( x > vmax) {max = 1; obj->Body->setLinearVelocity(btVector3( vmax, y, z));}
	if ( x <-vmax) {max = 1; obj->Body->setLinearVelocity(btVector3(-vmax, y, z));}
	//if ( AMG_fabsf(y) > vmax) {max = 1; obj->Body->setLinearVelocity(btVector3( x, vmax, z));}
	if ( z > vmax) {max = 1; obj->Body->setLinearVelocity(btVector3( x, y, vmax));}
	if ( z <-vmax) {max = 1; obj->Body->setLinearVelocity(btVector3( x, y,-vmax));}

	if (!max){
		//Brake using rear wheels
		obj->m_vehicle->setBrake(model->Object[0].Brake,2);
		obj->m_vehicle->setBrake(model->Object[0].Brake,3);
		//Accelerate using front wheels (if you use rear wheels, car ussualy turns over)
		obj->m_vehicle->applyEngineForce(model->Object[0].Engine,0);
		obj->m_vehicle->applyEngineForce(model->Object[0].Engine,1);
	} else {
		obj->m_vehicle->setBrake(0,2);
		obj->m_vehicle->setBrake(0,3);
		obj->m_vehicle->applyEngineForce(0,0);
		obj->m_vehicle->applyEngineForce(0,1);
	}
	//Turn using front wheels
	obj->m_vehicle->setSteeringValue(model->Object[0].Steering,0);
	obj->m_vehicle->setSteeringValue(model->Object[0].Steering,1);
	
	//Reset collisions
	//obj->md->Collision = 0;
	//obj->md->CollidedWith = 0xFFFF;
}

//Mr Burns: I'm sure the manual will indicate which lever is the velocitator and which the deceleratrix.
void M3D_VehicleMove(M3D_Model *model, float velocitator, float deceleratrix, float steering){
	AMG_Model *m = (AMG_Model*) model;
	AMG_MoveVehicle(m);
	
	if (M3D_KEYS->held.left) m->Object[0].Steering += steering;
	else if (M3D_KEYS->held.right) m->Object[0].Steering -= steering;
	else {
		if (m->Object[0].Steering < 0) m->Object[0].Steering += 0.01;
		else if (m->Object[0].Steering > 0) m->Object[0].Steering -= 0.01;
		else m->Object[0].Steering = 0;
	}
	if (M3D_KEYS->held.cross) {m->Object[0].Engine = -velocitator; m->Object[0].Brake = 0;}
	if (M3D_KEYS->released.cross) {m->Object[0].Engine = 0;m->Object[0].Brake = 0;}
	if (M3D_KEYS->held.circle) {m->Object[0].Engine = 0;m->Object[0].Brake = -deceleratrix;} 
	if (M3D_KEYS->released.circle) {m->Object[0].Engine = 0;m->Object[0].Brake = 0;}
}

//constraints

void M3D_ConstraintBall(M3D_Model *m, int obj_number, float pivotx, float pivoty, float pivotz){
	AMG_Model* model = (AMG_Model*)m;
	//point to point constraint with a breaking threshold
	amg_mdh *obj = &amg_model_ptr[model->Object[obj_number].bullet_id];
	btVector3 pivotA(pivotx,pivoty,pivotz);
	obj->Body->setActivationState(DISABLE_DEACTIVATION);
	obj->BallConstraint = new btPoint2PointConstraint(*obj->Body,pivotA);
	obj->BallConstraint->setBreakingImpulseThreshold(10.2);
	AMG_DynamicWorld->addConstraint(obj->BallConstraint);
}

void M3D_ConstraintHinge(M3D_Model *m,int obj_number, float pivotx, float pivoty, float pivotz, int axis){
	AMG_Model* model = (AMG_Model*)m;
	//Hinge constraint fixed to world
	amg_mdh *obj = &amg_model_ptr[model->Object[obj_number].bullet_id];
	btVector3 btPivotA(pivotx,pivoty,pivotz); 
	btVector3 btAxisA(0,0,0); //axis
	btAxisA[axis] = 1;
	//btHingeConstraint* spHingeDynAB = NULL;
	obj->Body->setActivationState(DISABLE_DEACTIVATION);
	obj->HingeConstraint = new btHingeConstraint(*obj->Body, btPivotA, btAxisA);
	AMG_DynamicWorld->addConstraint(obj->HingeConstraint);
}

void M3D_ConstraintHinge2(M3D_Model *m1,int obj_number1, M3D_Model *m2, int obj_number2, float pivotx, float pivoty, float pivotz, float pivot1x, float pivot1y, float pivot1z, int axis){
	AMG_Model* model1 = (AMG_Model*)m1;
	AMG_Model* model2 = (AMG_Model*)m2;
	//Hinge constraint fixed to world
	amg_mdh *obj = &amg_model_ptr[model1->Object[obj_number1].bullet_id];
	amg_mdh *obj2 = &amg_model_ptr[model2->Object[obj_number2].bullet_id];
	btVector3 btPivotA(pivotx,pivoty,pivotz);
	btVector3 btAxisA(0,0,0); //axis
	btVector3 btPivotB(pivot1x,pivot1y,pivot1z);
	btVector3 btAxisB(0,0,0); //axis
	btAxisA[axis] = 1;
	btAxisB[axis] = 1;
	//btHingeConstraint* spHingeDynAB = NULL;
	obj->Body->setActivationState(DISABLE_DEACTIVATION);
	obj->HingeConstraint = new btHingeConstraint(*obj->Body, *obj2->Body, btPivotA, btPivotB, btAxisA, btAxisB);
	AMG_DynamicWorld->addConstraint(obj->HingeConstraint);
}

void AMG_HingeConstraintMotor(AMG_Model *model, float target_vel, float max_impulse){
	amg_model_ptr[model->Object[0].bullet_id].HingeConstraint->enableAngularMotor(true, target_vel, max_impulse);
}
	
// Actualiza el motor de fisicas
float AMG_BULLET_TIME = 0.016;
void AMG_UpdateBulletPhysics(void){
	if(AMG_DynamicWorld == NULL) return;
	//
	if (!skip)AMG_BULLET_TIME = 0.016;
	else if (skip)AMG_BULLET_TIME = 0.016*skip;
	AMG_DynamicWorld->stepSimulation(AMG_BULLET_TIME,1,AMG_BULLET_TIME);
	//ONLY FOR GROUND OBJECTS
	//if (amg_model_ptr[i].md->Mass == 0.0f) amg_set_rotation(&amg_model_ptr[i], amg_model_ptr[i].md->Rot);
}

void M3D_BulletUpdatePhysics(void){
	if(AMG_DynamicWorld == NULL) return;
	//RESET COLLISIONS
	u32 i;
	for(i=0;i<amg_max_objects;i++){
		if(amg_model_ptr[i].md){
			//amg_model_ptr[i].md.Collision = 0;
			//amg_model_ptr[i].md.CollidedWith = 0xFFFF;
			AMG_Object *o = (AMG_Object*)amg_model_ptr[i].md;
			if (o->Physics){
				o->Collision = false;
				o->CollidedWith = 0;
			}
		}
	}
	//Update
	if (!skip)AMG_BULLET_TIME = 0.016;
	else if (skip)AMG_BULLET_TIME = 0.016*skip;
	AMG_DynamicWorld->stepSimulation(AMG_BULLET_TIME,1,AMG_BULLET_TIME);
	
	
	
	//ONLY FOR GROUND OBJECTS
	//if (amg_model_ptr[i].md->Mass == 0.0f) amg_set_rotation(&amg_model_ptr[i], amg_model_ptr[i].md->Rot);
}

// Termina con el motor Bullet
void AMG_FinishBulletPhysics(void){
	// Elimina todos los objetos de la pila
	for(u32 i=0;i<amg_max_objects;i++){
		if(amg_model_ptr[i].md != NULL){
			// Eliminalo del motor Bullet
			AMG_DynamicWorld->removeRigidBody(amg_model_ptr[i].Body);
			delete amg_model_ptr[i].Body; amg_model_ptr[i].Body = NULL;
			delete amg_model_ptr[i].Shape; amg_model_ptr[i].Shape = NULL;
			//amg_model_ptr[i].md->Mass = 0.0f; 
			//amg_model_ptr[i].md->isGround = 0;
			// Quitalo de la pila
			//amg_model_ptr[i].md->bullet_id = 0;
			amg_model_ptr[i].md = NULL;
			amg_model_ptr[i].Shape = NULL;
			amg_model_ptr[i].Body = NULL;
		}
	}
	// Elimina la pila
	//free(amg_model_ptr); amg_model_ptr = NULL;
	amg_max_objects = 0;
	// Elimina los demas datos
	delete AMG_DynamicWorld; AMG_DynamicWorld = NULL;
	delete AMG_PhysicsSolver;
	delete AMG_WorldBroadphase;
	delete AMG_WorldDispatcher;
	delete AMG_CollisionConfiguration;
}

void M3D_BulletFinishPhysics(void){
	AMG_FinishBulletPhysics();
}

/******************************************************/
/************** FUNCIONES LOCALES *********************/
/******************************************************/

// Transforma un vector en modo Cuartenion (IJK) a modo Vectorial (XYZ)
void amg_quaternion2vector(const btQuaternion &quat, btVector3 &vec){
	float w = quat.getW();	float x = quat.getX();	float y = quat.getY();	float z = quat.getZ();
	float sqw = w*w; float sqx = x*x; float sqy = y*y; float sqz = z*z; 
	vec.setZ((atan2f(2.0 * (x*y + z*w),(sqx - sqy - sqz + sqw))));
	vec.setX((atan2f(2.0 * (y*z + x*w),(-sqx - sqy + sqz + sqw))));
	vec.setY((asinf(-2.0 * (x*z - y*w))));
}

// Obten la rotacion de un objeto simulado
void amg_get_rotation(amg_mdh *obj, ScePspFVector3 &rot){
	btVector3 btv;
	amg_quaternion2vector(obj->Body->getOrientation(), btv);
	rot.x = btv.getX();
	rot.y = btv.getY();
	rot.z = btv.getZ();
}

// Establece la posicion de un objeto simulado
void amg_set_position(amg_mdh *obj, ScePspFVector3 &pos){
	ScePspFVector3 tpos;
	amg_get_position(obj, tpos);
	float x = pos.x - tpos.x;
	float y = pos.y - tpos.y;
	float z = pos.z - tpos.z;
	obj->Body->translate(btVector3(x, y, z));
}

// Obten la posicion de un objeto simulado
void amg_get_position(amg_mdh *obj, ScePspFVector3 &pos){
	if(obj->Body && obj->Body->getMotionState()){
		btVector3 p = obj->Body->getCenterOfMassPosition();
		AMG_Object *o = (AMG_Object*) obj->Body->getUserPointer();
		pos.x = (p.getX() - o->Origin.x);
		pos.y = (p.getY() - o->Origin.y);
		pos.z = (p.getZ() - o->Origin.z);
	}
}


// Guarda un objeto 3D en la pila
void amg_save_object_stack(void *md){
	u32 i; u8 done = 0;
	for(i=0;i<amg_max_objects;i++){
		if(!amg_model_ptr[i].md){	// Busca el primer slot libre
			amg_model_ptr[i].md = md; done = 1;
			((AMG_Object*) md)->bullet_id = i; i = amg_max_objects;
		}
	}
	if(!done) ;	// Error si no se ha encontrado
}


////////////////////////

// Establece la velocidad lineal de un objeto 3D
void AMG_SetObjectLinearVelocity(AMG_Object *obj, float x, float y, float z){
	amg_mdh *o = &amg_model_ptr[obj->bullet_id];
	if(o->Body == NULL) return;
	o->Body->setLinearVelocity(btVector3(x, y, z));
}

// Obtén la velocidad lineal de un objeto 3D
void AMG_GetObjectLinearVelocity(AMG_Object *obj, ScePspFVector3 *vel){
	amg_mdh *o = &amg_model_ptr[obj->bullet_id];
	if(o->Body == NULL) return;
	btVector3 v = o->Body->getLinearVelocity();
	vel->x = (float)v.getX(); vel->y = (float)v.getY(); vel->z = (float)v.getZ();
}

void AMG_SetObjectMaxVelocity(AMG_Model *model, int n, float vel){
	model->Object[n].vmax = vel;
}

void M3D_ModelSetMaxVelocity(M3D_Model *model, int obj_number, float vel){
	AMG_Model *m = (AMG_Model*)model;
	m->Object[obj_number].vmax = vel;
}

// Aplica una fuerza a un objeto 3D
void M3D_ModelSetForce(M3D_Model *model, int obj_number, float x, float y, float z){
	AMG_Model* m = (AMG_Model*)model;
	amg_mdh *o = &amg_model_ptr[m->Object[obj_number].bullet_id];
	if(o->Body == NULL) return;
	o->Body->applyCentralImpulse(btVector3(x, y, z));
}

// Establece las propiedades de un objeto 3D
void M3D_ModelSetProperties(M3D_Model *m, int obj_number, btScalar friction, btScalar rollfriction, btScalar restitution){
	AMG_Model* model = (AMG_Model*)m;
	
	amg_mdh *o = &amg_model_ptr[model->Object[obj_number].bullet_id];
	if(o->Body == NULL) return;
	o->Body->setFriction(friction);
	o->Body->setRollingFriction(rollfriction);
	o->Body->setRestitution(restitution);
}

// Obtén las propiedades de un objeto 3D
void AMG_GetObjectProperties(AMG_Object *obj, float *friction, float *rollfriction, float *restitution){
	amg_mdh *o = &amg_model_ptr[obj->bullet_id];
	if(o->Body == NULL) return;
	*friction = (float)o->Body->getFriction();
	*rollfriction = (float)o->Body->getRollingFriction();
	*restitution = (float)o->Body->getRestitution();
}

// Establece la velocidad lineal de un actor 3D
void AMG_SetSkinnedActorLinearVelocity(AMG_Skinned_Actor *actor, float x, float y, float z){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->setLinearVelocity(btVector3(x, y, z));
}

// Obtén la velocidad lineal de un actor 3D
void AMG_GetSkinnedActorLinearVelocity(AMG_Skinned_Actor *actor, ScePspFVector3 *vel){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	btVector3 v = o->Body->getLinearVelocity();
	vel->x = (float)v.getX(); vel->y = (float)v.getY(); vel->z = (float)v.getZ();
}

// Aplica una fuerza a un actor 3D
void AMG_SetSkinnedActorForce(AMG_Skinned_Actor *actor, float x, float y, float z){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->applyCentralImpulse(btVector3(x, y, z));
}

// Establece las propiedades de un actor 3D
void AMG_SetSkinnedActorProperties(AMG_Skinned_Actor *actor, float friction, float restitution){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->setRollingFriction(friction);
	o->Body->setRestitution(restitution);
}

// Obtén las propiedades de un actor 3D
void AMG_GetSkinnedActorProperties(AMG_Skinned_Actor *actor, float *friction, float *restitution){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	*friction = (float)o->Body->getFriction();
	*restitution = (float)o->Body->getRestitution();
}

// Establece la velocidad lineal de un actor Morphing 3D
void AMG_SetMorphingActorLinearVelocity(AMG_Morphing_Actor *actor, float x, float y, float z){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->setLinearVelocity(btVector3(x, y, z));
}

// Obtén la velocidad lineal de un actor Morphing 3D
void AMG_GetMorphingActorLinearVelocity(AMG_Morphing_Actor *actor, ScePspFVector3 *vel){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	btVector3 v = o->Body->getLinearVelocity();
	vel->x = (float)v.getX(); vel->y = (float)v.getY(); vel->z = (float)v.getZ();
}

// Aplica una fuerza a un actor Morphing 3D
void AMG_SetMorphingActorForce(AMG_Morphing_Actor *actor, float x, float y, float z){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->applyCentralImpulse(btVector3(x, y, z));
}

// Establece las propiedades de un actor Morphing 3D
void AMG_SetMorphingActorProperties(AMG_Morphing_Actor *actor, float friction, float restitution){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	o->Body->setRollingFriction(friction);
	o->Body->setRestitution(restitution);
}

// Obtén las propiedades de un actor Morphing 3D
void AMG_GetMorphingActorProperties(AMG_Morphing_Actor *actor, float *friction, float *restitution){
	amg_mdh *o = &amg_model_ptr[actor->Object[0].bullet_id];
	if(o->Body == NULL) return;
	*friction = (float)o->Body->getFriction();
	*restitution = (float)o->Body->getRestitution();
}

void AMG_GetVehiclePos(AMG_Model *model, ScePspFVector3 *position){
	ScePspFVector3 c = {0,0,0};
	memcpy(&c,&amg_model_ptr[model->Object[0].bullet_id].m_vehicle->getChassisWorldTransform().getOrigin(),sizeof(ScePspFVector3));
	*position = c;
}

ScePspFVector3 M3D_VehicleGetPos(M3D_Model *model, int obj_number){
	AMG_Model *m = (AMG_Model*)model;
	ScePspFVector3 c = {0,0,0};
	memcpy(&c,&amg_model_ptr[m->Object[obj_number].bullet_id].m_vehicle->getChassisWorldTransform().getOrigin(),sizeof(ScePspFVector3));
	return c;
}

void AMG_GetVehicleRot(AMG_Model *model, ScePspFVector3 *rotation){
	ScePspFVector3 c = {0,0,0};
	amg_mdh *obj = &amg_model_ptr[model->Object[0].bullet_id];
	amg_get_rotation(obj,c);
	*rotation = c;
}




float M3D_cam_rot = 0;
u8 M3D_ActorCamMode = 0x00;
float M3D_ActorCamModeX = 0;
float M3D_ActorCamModeZ = 0;
//
void M3D_CameraFollowSkinnedActor(M3D_Camera *cam, M3D_SkinnedActor *act, float y, float max){
	AMG_Camera *camera = (AMG_Camera *)cam;
	AMG_Skinned_Actor *Actor = (AMG_Skinned_Actor *)act;
	
	//ScePspFVector3 v;
	//AMG_GetSkinnedActorLinearVelocity(Actor,&v);

	float cx = Actor->Pos.x+(-5*M3D_Sin(Actor->Rot.y));
	float cz = Actor->Pos.z+(-5*M3D_Cos(Actor->Rot.y));
	float dx0 = cx-camera->Pos.x; float dx1 = camera->Pos.x-cx;
	float dz0 = cz-camera->Pos.z; float dz1 = camera->Pos.z-cz;
	int min = 4;
	
	if ((M3D_fabsf(camera->Pos.x-Actor->Pos.x) < min) && (M3D_fabsf(camera->Pos.z-Actor->Pos.z) < min)){
		/*if (M3D_ActorCamMode == 0){
			M3D_ActorCamModeX = -1*(Actor->Pos.x-camera->Pos.x)/80.0f;
			M3D_ActorCamModeZ = -1*(Actor->Pos.z-camera->Pos.z)/80.0f;
			M3D_ActorCamMode = 1;
		}
		camera->Pos.x += M3D_ActorCamModeX;
		camera->Pos.z += M3D_ActorCamModeZ;*/
	} else {
		M3D_ActorCamMode = 0;
		if (camera->Pos.x<cx && dx0>max) camera->Pos.x += dx0/80;
		if (camera->Pos.x>cx && dx1>max) camera->Pos.x -= dx1/80;
		if (camera->Pos.z<cz && dz0>max) camera->Pos.z += dz0/80;
		if (camera->Pos.z>cz && dz1>max) camera->Pos.z -= dz1/80;
	}
	
	//camera->Pos.x = M3D_fabsf(Actor->Pos.x-camera->Pos.x)*-M3D_Sin(M3D_cam_rot);
	//camera->Pos.z = M3D_fabsf(Actor->Pos.z-camera->Pos.z)*M3D_Cos(M3D_cam_rot);
	
	float sizey = fabs(Actor->Object->BBox[0].y - Actor->Object->BBox[1].y);
	camera->Pos.y = Actor->Pos.y+y;
	camera->Eye = Actor->Pos;
	camera->Eye.y = Actor->Pos.y+(sizey/2);
	float rx = camera->Pos.x-Actor->Pos.x;
	float rz = camera->Pos.z-Actor->Pos.z;
	if (M3D_fabsf(rz)<0.01) return;
	if (osl_keys->held.L) {
		if (camera->Pos.x<Actor->Pos.x)camera->Pos.z-=0.2;
		else camera->Pos.z+=0.2;
	 	if (camera->Pos.z<Actor->Pos.z)camera->Pos.x+=0.2;
		else camera->Pos.x-=0.2;
	}
	if (osl_keys->held.R){
		if (camera->Pos.x<Actor->Pos.x)camera->Pos.z+=0.2;
		else camera->Pos.z-=0.2;
		if (camera->Pos.z<Actor->Pos.z)camera->Pos.x-=0.2;
		else camera->Pos.x+=0.2;
	}
	//M3D_cam_rot = vfpu_atan2f(rx,rz);
	AMG_SetCamera(camera);
}

u32 AMG_RayTracingTest(ScePspFVector3 *pos, ScePspFVector3 *vec);
void AMG_SetSkinnedActorLinearVelocity(AMG_Skinned_Actor *actor, float x, float y, float z);
void AMG_GetSkinnedActorLinearVelocity(AMG_Skinned_Actor *actor, ScePspFVector3 *vel);

void M3D_CharacterMove(M3D_SkinnedActor *act, M3D_Player *player, const void *bad_col,int item_slot, const void *item_col,float speed, float jump){
	AMG_Skinned_Actor *Actor = (AMG_Skinned_Actor *)act;
	float ControlX = osl_pad.analogX;
	float ControlY = osl_pad.analogY;
	ScePspFMatrix4 __attribute__((aligned(16))) mtx_m;
	ScePspFMatrix4 __attribute__((aligned(16))) mtx_view;
	ScePspFVector3 __attribute__((aligned(16))) mtx_from = (ScePspFVector3){Actor->Pos.x,Actor->Pos.y,Actor->Pos.z};
	ScePspFVector3 __attribute__((aligned(16))) mtx_to = (ScePspFVector3) {Actor->Pos.x+ControlX,Actor->Pos.y,Actor->Pos.z+ControlY};
	ScePspFVector3 __attribute__((aligned(16))) mtx_up = (ScePspFVector3) {0,1,0};
	ScePspFVector3 __attribute__((aligned(16))) Ray_O = {Actor->Pos.x,Actor->Pos.y+0.1f,Actor->Pos.z};
	ScePspFVector3 __attribute__((aligned(16))) Ray = {0,-0.2f,0};
	player->Ground = AMG_RayTracingTest(&Ray_O, &Ray);
	ScePspFVector3 Bot_vel;
	AMG_GetSkinnedActorLinearVelocity(Actor,&Bot_vel);

	AMG_LoadIdentityUser(&mtx_m);
	gumLookAt(&mtx_m,&mtx_from,&mtx_to,&mtx_up);
	AMG_GetMatrix(GU_VIEW,&mtx_view);
	AMG_MultMatrixUser(&mtx_m,&mtx_view,&mtx_m);
	
	int *pi = player->AnimIdle;
	int *ps = player->AnimWalkSlow;
	int *pf = player->AnimWalkFast;
	int *pj = player->AnimJump;
	int *pd = player->AnimDie;
	if (!player) return;
	if (player->Health){
		if ((M3D_fabsf(ControlX)>30 && M3D_fabsf(ControlY)>30) || (M3D_fabsf(ControlX)>30 || M3D_fabsf(ControlY)>30)){
			float sp = M3D_VectorLength(ControlX/32*speed,ControlY/32*speed,0); 
			AMG_SetSkinnedActorLinearVelocity(Actor,-1*sp*mtx_m.x.z,Bot_vel.y,-1*sp*mtx_m.x.x);
			if (player->Ground){
				if (M3D_fabsf(sp)<3) AMG_ConfigSkinnedActor(Actor,ps[0],ps[1],ps[2],ps[3],ps[4]);
				else AMG_ConfigSkinnedActor(Actor,pf[0],pf[1],pf[2],pf[3],pf[4]);
			}
			u8 abort = 0;
			if (M3D_fabsf(mtx_m.x.z)<0.02) abort = 1;
			if (M3D_fabsf(mtx_m.x.x)<0.02) abort = 1;
			if (!abort)Actor->Rot.y = /*vfpu_*/atan2f(-1*mtx_m.x.z,-1*mtx_m.x.x);
		} else {
			AMG_SetSkinnedActorLinearVelocity(Actor, 0,Bot_vel.y,0);
			if (player->Ground)AMG_ConfigSkinnedActor(Actor,pi[0],pi[1],pi[2],pi[3],pi[4]);
			
		}
		
		//JUMP
		if ((player->Ground)&&(osl_keys->pressed.cross)) AMG_SetSkinnedActorLinearVelocity(Actor, 0,jump,0);
		if (!player->Ground) AMG_ConfigSkinnedActor(Actor,pj[0],pj[1],pj[2],pj[3],pj[4]);
		u32 *bad = (u32*) bad_col;
		u32 *item = (u32*) item_col;
		
		//Collision with bad thing
		if(Actor->Object[0].CollidedWith == bad[0]) {
			if (Actor->Object[0].collisionreset == 0){
				AMG_SetSkinnedActorLinearVelocity(Actor, 0,jump,0);
				player->Health--;
			}
			Actor->Object[0].collisionreset++;
			if (Actor->Object[0].collisionreset == 20) {
				Actor->Object[0].CollidedWith = 0;
				Actor->Object[0].collisionreset = 0;
			}
		}
		//Collision with item
		else if (Actor->Object[0].CollidedWith == item[0]){
			if (Actor->Object[0].collisionreset == 0){
				player->Items[item_slot]++;
			}
			Actor->Object[0].collisionreset++;
			if (Actor->Object[0].collisionreset == 20) {
				Actor->Object[0].CollidedWith = 0;
				Actor->Object[0].collisionreset = 0;
			}
		} else Actor->Object[0].collisionreset = 0;
	} else AMG_ConfigSkinnedActor(Actor,pd[0],pd[1],pd[2],pd[3],pd[4]);
	
	player->CollidedWith = Actor->Object[0].CollidedWith;
}

//Sample enemy, it will see things in front of its "eyes" at the top part of the model
void M3D_EnemyMove(M3D_SkinnedActor *act){
	AMG_Skinned_Actor *Actor = (AMG_Skinned_Actor *)act;
	float size = M3D_fabsf(Actor->Object->BBox[0].y - Actor->Object->BBox[1].y);
	ScePspFVector3 Ray_origin = {Actor->Pos.x,Actor->Pos.y+(size/1.2),Actor->Pos.z};
	float Ray_Rot = Actor->Rot.y + M3D_Randf(-8,8);//To avoid getting stuck
	ScePspFVector3 Ray = {M3D_Sin(Ray_Rot)*(size/3),0,M3D_Cos(Ray_Rot)*(size/3)};
	u32 Wall = AMG_RayTracingTest(&Ray_origin, &Ray);
	ScePspFVector3 vel;
	AMG_GetSkinnedActorLinearVelocity(Actor,&vel);
	AMG_SetSkinnedActorLinearVelocity(Actor, sin(Actor->Rot.y)*2,vel.y,M3D_Cos(Actor->Rot.y)*2);

	//If wall, turn
	if (Wall) Actor->Rot.y += M3D_Randf(-90,90);
}

//Give unique material numbers to your models for identification in physics collisions.
//Or Give a 4 character name (1 byte per character) to your model
void M3D_MaterialSetName(void *model, int object, const void *name){
	AMG_Model *m = (AMG_Model*) model;
	u32 *n = (u32*) name;
	m->Object[object].material_name[0] = n[0];//all structs have this member at the same offset
}



