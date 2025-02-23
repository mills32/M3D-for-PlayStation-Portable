//NOTE!!
//ulv and usv instructions are broken on PSP 1000
//use lv and sv instead, they need 16 byte alignement 
//just use this: "int __attribute__((aligned(16))) i"
//Or use latest linux sdk (or wsl on windows) 
//Latest sdk aligns data for you and fixes all issues


// Includes
#include "AMG_3D.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pspkernel.h>
#include <malloc.h>
#include <pspgum.h>

// Cabeceras
void __vfpu_perspective_matrix(ScePspFMatrix4 *m, float fov, float aspect, float near, float far);

// Sistema de matrices
typedef struct{
	ScePspFMatrix4 __attribute__((aligned(16))) *matrix[4];
	u8 sp[4];
	u8 update[4];
	u8 stackDepth;
} __attribute__((aligned(16))) amg_mtxsys;
amg_mtxsys amg_matrix_sys;
u8 amg_mtx_inited = 0;

#define SP(n) amg_matrix_sys.sp[n]
#define AMG_PROJECTION GU_PROJECTION][SP(GU_PROJECTION)
#define AMG_VIEW GU_VIEW][SP(GU_VIEW)
#define AMG_MODEL GU_MODEL][SP(GU_MODEL)
#define AMG_TEXTURE GU_TEXTURE][SP(GU_TEXTURE)


float AMG_Temp_perspective_mtx[4*4] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

// Replaces global perspective matrix (stores original)
void AMG_Push_Perspective_Matrix(int w,int h,float fov){
	memcpy(AMG_Temp_perspective_mtx,&amg_matrix_sys.matrix[AMG_PROJECTION], sizeof(ScePspFMatrix4));
	for(u8 i=0;i<4;i++) amg_matrix_sys.update[i] = true;
	AMG_LoadIdentity(GU_PROJECTION);
	__vfpu_perspective_matrix(&amg_matrix_sys.matrix[AMG_PROJECTION], fov, ((float)w / (float)h),1000, 0.1);
	AMG_UpdateMatrices();
}

// Restores global perspective matrix
void AMG_Pop_Perspective_Matrix(){
	for(u8 i=0;i<4;i++) amg_matrix_sys.update[i] = true;
	AMG_LoadIdentity(GU_PROJECTION);
	memcpy(&amg_matrix_sys.matrix[AMG_PROJECTION],AMG_Temp_perspective_mtx, sizeof(ScePspFMatrix4));
	AMG_UpdateMatrices();
}


// Inicializa las matrices
void AMG_InitMatrixSystem(float fov,float near,float far,int clipping){
	// Inicializa los buffers
	if(!amg_mtx_inited){
		memset(&amg_matrix_sys, 0, sizeof(amg_mtxsys));
		amg_matrix_sys.stackDepth = AMG_MATRIX_STACKSIZE;
		for(u8 i=0;i<4;i++) amg_matrix_sys.matrix[i] = (ScePspFMatrix4*) memalign (16, amg_matrix_sys.stackDepth * sizeof(ScePspFMatrix4));
		amg_mtx_inited = 1;
	}
	// Inicializa las matrices
	for(u8 i=0;i<4;i++) amg_matrix_sys.update[i] = true;
	AMG_LoadIdentity(GU_PROJECTION);
	AMG_LoadIdentity(GU_VIEW);
	AMG_LoadIdentity(GU_MODEL);
	AMG_LoadIdentity(GU_TEXTURE);
	if(fov){
		//__vfpu_perspective_matrix(&amg_matrix_sys.matrix[AMG_PROJECTION], fov, ((float)AMG.ScreenWidth / (float)AMG.ScreenHeight), near, far);
		gumPerspective(&amg_matrix_sys.matrix[AMG_PROJECTION], fov, ((float)AMG.ScreenWidth / (float)AMG.ScreenHeight), near, far);
	} else if(!fov) {
		float s = far/1000;
		ScePspFMatrix4 m = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1},};// left,right,bottom,top, near,far 
		gumOrtho(&m,-(AMG.ScreenWidth/2)*s,(AMG.ScreenWidth/2)*s,-(AMG.ScreenHeight/2)*s,(AMG.ScreenHeight/2)*s,near,far); 
		AMG_SetMatrix(GU_PROJECTION,&m);

	}
	// Actualiza las matrices
	AMG_UpdateMatrices();
	
	if (clipping == 0) sceGuDisable(GU_CLIP_PLANES);
	else sceGuEnable(GU_CLIP_PLANES);
}


void M3D_InitMatrixSystem(float fov,float near,float far,int clipping){
	AMG_InitMatrixSystem(fov,near,far,clipping);
}


// Destruye las matrices
void AMG_DestroyMatrixSystem(void){
	if(!amg_mtx_inited) return;
	for(u8 i=0;i<4;i++) free(amg_matrix_sys.matrix[i]);
	memset(&amg_matrix_sys, 0, sizeof(amg_mtxsys));
	amg_mtx_inited = false;
}

// Actualiza las matrices
void AMG_UpdateMatrices(void){
	if(AMG.Rendering){
		for(u8 i=0;i<4;i++){
			if(amg_matrix_sys.update[i]){
				sceGuSetMatrix(i, &amg_matrix_sys.matrix[i][SP(i)]);
				amg_matrix_sys.update[i] = false;
			}
		}
	}
}

// Guarda una matriz en la pila
void AMG_PushMatrix(u8 mt){
	if(SP(mt) > amg_matrix_sys.stackDepth) return;
	memcpy(&amg_matrix_sys.matrix[mt][SP(mt)+1], &amg_matrix_sys.matrix[mt][SP(mt)], sizeof(ScePspFMatrix4));
	SP(mt) ++;
}

// Restaura una matriz de la pila
void AMG_PopMatrix(u8 mt){
	if(SP(mt) == 0) return;
	SP(mt) --;
	amg_matrix_sys.update[mt] = true;
}

/**
 * THE FOLLOWING FUNCTIONS HAVE BEEN TAKEN FROM: libpspmath, libpspgum
 * I'M NOT THE AUTHOR OF THESE FUNCTIONS
 */

float M3D_fabsf(float x) {
	float result;
	__asm__ volatile (
		"mtv     %1, S000\n"
		"vmov.s  S000, S000[|x|]\n"
		"mfv     %0, S000\n"
	: "=r"(result) : "r"(x));
	return result;
}

//from libmath
float vfpu_fminf(float x, float y) {
	float result;
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vmin.s   S002, S000, S001\n"
		"mfv      %0, S002\n"
	: "=r"(result) : "r"(x), "r"(y));
	return result;
}
//from libmath
float vfpu_fmaxf(float x, float y) {
	float result;
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vmax.s   S002, S000, S001\n"
		"mfv      %0, S002\n"
	: "=r"(result) : "r"(x), "r"(y));
	return result;
}

// Normaliza un vector
void AMG_Normalize(ScePspFVector3 *v){
	 __asm__ volatile (
       "lv.q   C000, %0\n"
       "vdot.t S010, C000, C000\n"
       "vrsq.s S010, S010\n"
       "vscl.t C000, C000, S010\n"
       "sv.q   C000, %0\n"
	: "+m"(*v));
}

// Calcula el producto mixto entre 2 vectores
float AMG_DotProduct(ScePspFVector4 *v1, ScePspFVector4 *v2){
	float dot;
	__asm__ volatile (
		"lv.q C010, %1\n"
		"lv.q C020, %2\n"
		"vdot.t S000, C010, C020\n"
		"mfv %0, S000\n"
	: "=r"(dot) : "m"(*v1), "m"(*v2) : "memory");
	return dot;
}

// Haz el producto cruz
void AMG_CrossProduct(ScePspFVector3 *v1, ScePspFVector3 *v2, ScePspFVector3 *r){
	__asm__ volatile (
		"lv.q C000, %1\n"
		"lv.q C010, %2\n"
		"vcrsp.t C020, C000, C010\n"
		"sv.q C020, %0\n"
	:: "m"(*r), "m"(*v1), "m"(*v2));
}

// Simula una cámara
void AMG_LookAt(ScePspFVector3 *eye, ScePspFVector3 *center, ScePspFVector3 *up){
	ScePspFMatrix4 *m = &amg_matrix_sys.matrix[GU_VIEW][SP(GU_VIEW)];
	__asm__ volatile (
		// Carga la matriz VIEW
		"lv.q C300,  0 + %0\n"
		"lv.q C310, 16 + %0\n"
		"lv.q C320, 32 + %0\n"
		"lv.q C330, 48 + %0\n"
		// Calcula la cámara
		"vmidt.q M100\n"
		"lv.q C000, %1\n"
		"lv.q C010, %2\n"
		"lv.q C020, %3\n"
		"vsub.t R102, C010, C000\n"
		"vdot.t S033, R102, R102\n"
		"vrsq.s S033, S033\n"
		"vscl.t R102, R102, S033\n"
		"vcrsp.t R100, R102, C020\n"
		"vdot.t S033, R100, R100\n"
		"vrsq.s S033, S033\n"
		"vscl.t R100, R100, S033\n"
		"vcrsp.t R101, R100, R102\n"
		"vneg.t R102, R102\n"
		"vmidt.q M200\n"
		"vneg.t C230, C000\n"
		// Multiplica la cámara por la vista
		"vmmul.q M300, M100, M200\n"
		"sv.q C300,  0 + %0\n"
		"sv.q C310, 16 + %0\n"
		"sv.q C320, 32 + %0\n"
		"sv.q C330, 48 + %0\n"
		:: "m"(*m), "m"(*eye), "m"(*center), "m"(*up));
	amg_matrix_sys.update[GU_VIEW] = true;
}

// Track / look at: rotates matrix to look at a point
void AMG_Track(ScePspFVector3 *eye, ScePspFVector3 *center, ScePspFVector3 *up, ScePspFMatrix4 *m1){
	ScePspFMatrix4 *m = m1;
	__asm__ volatile (
		// Carga la matriz VIEW
		"lv.q C300,  0 + %0\n"
		"lv.q C310, 16 + %0\n"
		"lv.q C320, 32 + %0\n"
		"lv.q C330, 48 + %0\n"
		// Calcula la cámara
		"vmidt.q M100\n"
		"lv.q C000, %1\n"
		"lv.q C010, %2\n"
		"lv.q C020, %3\n"
		"vsub.t R102, C010, C000\n"
		"vdot.t S033, R102, R102\n"
		"vrsq.s S033, S033\n"
		"vscl.t R102, R102, S033\n"
		"vcrsp.t R100, R102, C020\n"
		"vdot.t S033, R100, R100\n"
		"vrsq.s S033, S033\n"
		"vscl.t R100, R100, S033\n"
		"vcrsp.t R101, R100, R102\n"
		"vneg.t R102, R102\n"
		"vmidt.q M200\n"
		"vneg.t C230, C000\n"
		// Multiplica la cámara por la vista
		"vmmul.q M300, M100, M200\n"
		"sv.q C300,  0 + %0\n"
		"sv.q C310, 16 + %0\n"
		"sv.q C320, 32 + %0\n"
		"sv.q C330, 48 + %0\n"
		:: "m"(*m), "m"(*eye), "m"(*center), "m"(*up));
}

// Traslada una matriz
void AMG_SetMatrix(u32 mt, ScePspFMatrix4 *mtx){
	if(mt > 3) return;
	amg_matrix_sys.update[mt] = true;
	memcpy(&amg_matrix_sys.matrix[mt][SP(mt)], mtx, sizeof(ScePspFMatrix4));
}

// Guarda una matriz
void AMG_GetMatrix(u32 mt, ScePspFMatrix4 *mtx){
	memcpy(mtx,&amg_matrix_sys.matrix[mt][SP(mt)], sizeof(ScePspFMatrix4));
}

// Carga la matriz identidad 
void AMG_LoadIdentityUser(ScePspFMatrix4 *mtx){
	__asm__ volatile (
		"vmidt.q M000\n"
		"sv.q C000, 0 + %0\n"
		"sv.q C010, 16 + %0\n"
		"sv.q C020, 32 + %0\n"
		"sv.q C030, 48 + %0\n"
	:"=m"(*mtx));
}

// Carga la matriz identidad 
void AMG_LoadIdentity(u32 mt){
	if(mt > 3) return;
	ScePspFMatrix4 *mtx = &amg_matrix_sys.matrix[mt][SP(mt)];
	amg_matrix_sys.update[mt] = true;
	AMG_LoadIdentityUser(mtx);
}

// Traslada una matriz
void AMG_TranslateUser(ScePspFMatrix4 *mtx, ScePspFVector3 *v){
	__asm__ volatile (
		"lv.q C630,  0 + %1\n"
		//"lv.s S631,  4 + %1\n"
		//"lv.s S632,  8 + %1\n"
		"lv.q C700,  0 + %0\n"
		"lv.q C710, 16 + %0\n"
		"lv.q C720, 32 + %0\n"
		"lv.q C730, 48 + %0\n"
		"vscl.q	C600, C700, S630\n"
		"vscl.q	C610, C710, S631\n"
		"vscl.q	C620, C720, S632\n"
		"vadd.q	C730, C730, C600\n"
		"vadd.q	C730, C730, C610\n"
		"vadd.q	C730, C730, C620\n"
		"sv.q C730, 48 + %0\n"	// only C730 has changed
	: "+m"(*mtx) : "m"(*v));
}

// Traslada una matriz
void AMG_Translate(u32 mt, ScePspFVector3 *v){
	if(mt > 3) return;
	//ScePspFMatrix4 *mtx = &amg_matrix_sys.matrix[mt][SP(mt)];
	amg_matrix_sys.update[mt] = true;
	AMG_TranslateUser(&amg_matrix_sys.matrix[mt][SP(mt)], v);
}

// Escala una matriz
void AMG_ScaleUser(ScePspFMatrix4 *mtx, ScePspFVector3 *v){
	__asm__ volatile (
		"lv.q C700,  0 + %0\n"
		"lv.q C710, 16 + %0\n"
		"lv.q C720, 32 + %0\n"
		"lv.q C730, 48 + %0\n"
		"lv.q C600,  0 + %1\n"
		//"lv.s S601,  4 + %1\n"
		//"lv.s S602,  8 + %1\n"
		"vscl.t C700, C700, S600\n"
		"vscl.t C710, C710, S601\n"
		"vscl.t C720, C720, S602\n"
		"sv.q C700,  0 + %0\n"
		"sv.q C710, 16 + %0\n"
		"sv.q C720, 32 + %0\n"
		"sv.q C730, 48 + %0\n"
	: "+m"(*mtx) : "m"(*v));
}

// Escala una matriz
void AMG_Scale(u32 mt, ScePspFVector3 *v){
	if(mt > 3) return;
	//ScePspFMatrix4 *mtx = &amg_matrix_sys.matrix[mt][SP(mt)];
	amg_matrix_sys.update[mt] = true;
	AMG_ScaleUser(&amg_matrix_sys.matrix[mt][SP(mt)], v);
}

// Rota una matriz
void AMG_RotateUser(ScePspFMatrix4 *mtx, ScePspFVector3 *v){
	__asm__ volatile (
		// Carga la matriz y el vector
		"lv.q C500,  0 + %0\n"
		"lv.q C510, 16 + %0\n"
		"lv.q C520, 32 + %0\n"
		"lv.q C530, 48 + %0\n"
		"lv.q C400,  0 + %1\n"
		//"lv.s S401,  4 + %1\n"
		//"lv.s S402,  8 + %1\n"
		"vcst.s S410, VFPU_2_PI\n"
		"vmul.s S400, S410, S400\n"
		"vmul.s S401, S410, S401\n"
		"vmul.s S402, S410, S402\n"
		// Rotación X
		"vmidt.q M600\n"
		"vrot.q C610, S400, [0, c, s, 0]\n"
		"vrot.q C620, S400, [0, -s, c, 0]\n"
		"vmmul.q M700, M500, M600\n"
		// Rotación Y
		"vmidt.q M600\n"
		"vrot.q C600, S401, [c, 0,-s, 0]\n"
		"vrot.q C620, S401, [s, 0, c, 0]\n"
		"vmmul.q M500, M700, M600\n"
		// Rotación Z
		"vmidt.q M600\n"
		"vrot.q C600, S402, [ c, s, 0, 0]\n"
		"vrot.q C610, S402, [-s, c, 0, 0]\n"
		"vmmul.q M700, M500, M600\n"
		// Guarda la matriz
		"sv.q C700,  0 + %0\n"
		"sv.q C710, 16 + %0\n"
		"sv.q C720, 32 + %0\n"
		"sv.q C730, 48 + %0\n"
	: "+m"(*mtx) : "m"(*v));
}

// Rota una matriz
void AMG_Rotate(u32 mt, ScePspFVector3 *v){
	if(mt > 3) return;
	//ScePspFVector3 __attribute__((aligned(64))) *v1;
	//ScePspFMatrix4 __attribute__((aligned(16))) *mtx = &amg_matrix_sys.matrix[mt][SP(mt)];
	//memcpy(&v1,&v,sizeof(ScePspFVector3));
	amg_matrix_sys.update[mt] = true;
	AMG_RotateUser(&amg_matrix_sys.matrix[mt][SP(mt)], v);	
}


//rota una matriz usando un quaternion
void AMG_RotateQuatUser(ScePspQuatMatrix *q, ScePspFMatrix4 *m) {
	__asm__ volatile (
       "lv.q      C000, %1\n"                               // C000 = [x,  y,  z,  w ]
       "vmul.q    C010, C000, C000\n"                       // C010 = [x2, y2, z2, w2]
       "vcrs.t    C020, C000, C000\n"                       // C020 = [yz, xz, xy ]
       "vmul.q    C030, C000[x,y,z,1], C000[w,w,w,2]\n"	    // C030 = [wx, wy, wz ]

       "vadd.q    C100, C020[0,z,y,0], C030[0,z,-y,0]\n"    // C100 = [0,     xy+wz, xz-wy]
       "vadd.s    S100, S011, S012\n"                       // C100 = [y2+z2, xy+wz, xz-wy]

       "vadd.q    C110, C020[z,0,x,0], C030[-z,0,x,0]\n"    // C110 = [xy-wz, 0,     yz+wx]
       "vadd.s    S111, S010, S012\n"                       // C110 = [xy-wz, x2+z2, yz+wx]

       "vadd.q    C120, C020[y,x,0,0], C030[y,-x,0,0]\n"    // C120 = [xz+wy, yz-wx, 0    ]
       "vadd.s    S122, S010, S011\n"                       // C120 = [xz+wy, yz-wx, x2+y2]

       "vmscl.t   M100, M100, S033\n"                       // C100 = [2*(y2+z2), 2*(xy+wz), 2*(xz-wy)]
                                                            // C110 = [2*(xy-wz), 2*(x2+z2), 2*(yz+wx)]
                                                            // C120 = [2*(xz+wy), 2*(yz-wx), 2*(x2+y2)]

       "vocp.s    S100, S100\n"                             // C100 = [1-2*(y2+z2), 2*(xy+wz),   2*(xz-wy)  ]
       "vocp.s    S111, S111\n"                             // C110 = [2*(xy-wz),   1-2*(x2+z2), 2*(yz+wx)  ]
       "vocp.s    S122, S122\n"                             // C120 = [2*(xz+wy),   2*(yz-wx),   1-2*(x2+y2)]

       "vidt.q    C130\n"                                   // C130 = [0, 0, 0, 1]

       "sv.q      R100, 0  + %0\n"
       "sv.q      R101, 16 + %0\n"
       "sv.q      R102, 32 + %0\n"
       
	: "=m"(*m) : "m"(*q));
}

// Rota una matriz por un quaternion
void AMG_RotateQuat(u32 mt, ScePspQuatMatrix *q){
	if(mt > 3) return;
	ScePspFMatrix4 *mtx = &amg_matrix_sys.matrix[mt][SP(mt)];
	amg_matrix_sys.update[mt] = true;
	AMG_RotateQuatUser(q,mtx);
}

void AMG_QuatSampleLinear(ScePspQuatMatrix *qout, ScePspQuatMatrix *a, ScePspQuatMatrix *b, float t) {
	__asm__ volatile (
		"lv.q     C000, %1\n"
		"lv.q     C010, %2\n"
		"mtv      %3, S020\n"
		"vocp.s   S021, S020\n"
		"vscl.q   C000, C000, S021\n"
		"vscl.q   C010, C010, S020\n"
		"vadd.q   C000, C000, C010\n"
		"sv.q     C000, 0 + %0\n"
	: "=m"(*qout) : "m"(*a), "m"(*b), "r"(t));
}

// Multiplica dos matrices
void AMG_MultMatrixUser(ScePspFMatrix4 *a, ScePspFMatrix4 *b, ScePspFMatrix4 *result){
	__asm__ volatile(
		"lv.q C000,  0 + %1\n"
		"lv.q C010, 16 + %1\n"
		"lv.q C020, 32 + %1\n"
		"lv.q C030, 48 + %1\n"
		"lv.q C100,  0 + %2\n"
		"lv.q C110, 16 + %2\n"
		"lv.q C120, 32 + %2\n"
		"lv.q C130, 48 + %2\n"
		"vmmul.q M200, M000, M100\n"
		"sv.q C200,  0 + %0\n"
		"sv.q C210, 16 + %0\n"
		"sv.q C220, 32 + %0\n"
		"sv.q C230, 48 + %0\n"
	: "=m"(*result) : "m"(*a), "m"(*b) : "memory");
}

// Multiplica dos matrices (oficial)
void AMG_MultMatrix(u8 mt, ScePspFMatrix4 *m){
	if(mt > 3) return;
	ScePspFMatrix4 *_m = &amg_matrix_sys.matrix[mt][SP(mt)];
	amg_matrix_sys.update[mt] = true;
	AMG_MultMatrixUser(_m, m, _m);
}

 
// Carga la perspectiva en una matriz
void __vfpu_perspective_matrix(ScePspFMatrix4 *m, float fov, float aspect, float near, float far){
	__asm__ volatile (
		"vmzero.q M100\n"					// set M100 to all zeros
		"mtv     %1, S000\n"				// S000 = fovy
		"viim.s  S001, 90\n"				// S002 = 90.0f
		"vrcp.s  S001, S001\n"				// S002 = 1/90
		"vmul.s  S000, S000, S000[1/2]\n"	// S000 = fovy * 0.5 = fovy/2
		"vmul.s  S000, S000, S001\n"		// S000 = (fovy/2)/90
		"vrot.p  C002, S000, [c, s]\n"		// S002 = cos(angle), S003 = sin(angle)
		"vdiv.s  S100, S002, S003\n"		// S100 = m->x.x = cotangent = cos(angle)/sin(angle)
		"mtv     %3, S001\n"				// S001 = near
		"mtv     %4, S002\n"				// S002 = far
		"vsub.s  S003, S001, S002\n"		// S003 = deltaz = near-far
		"vrcp.s  S003, S003\n"				// S003 = 1/deltaz
		"mtv     %2, S000\n"				// S000 = aspect
		"vmov.s  S111, S100\n"				// S111 = m->y.y = cotangent
		"vdiv.s  S100, S100, S000\n"		// S100 = m->x.x = cotangent / aspect
		"vadd.s  S122, S001, S002\n"        // S122 = m->z.z = far + near
		"vmul.s  S122, S122, S003\n"		// S122 = m->z.z = (far+near)/deltaz
		"vmul.s  S132, S001, S002\n"        // S132 = m->w.z = far * near
		"vmul.s  S132, S132, S132[2]\n"     // S132 = m->w.z = 2 * (far*near)
		"vmul.s  S132, S132, S003\n"        // S132 = m->w.z = 2 * (far*near) / deltaz
		"vsub.s   S123, S123, S123[1]\n"	// S123 = m->z.w = -1.0
		"sv.q	 C100, 0  + %0\n"
		"sv.q	 C110, 16 + %0\n"
		"sv.q	 C120, 32 + %0\n"
		"sv.q	 C130, 48 + %0\n"
	:"=m"(*m): "r"(fov),"r"(aspect),"r"(near),"r"(far));
}


// Funcion seno
float M3D_Sin(float angle){
	float a; 
	__asm__ volatile (
	"mtv %1, S000\n"
	"vcst.s S001, VFPU_2_PI\n"
	"vmul.s S000, S000, S001\n"
	"vsin.s S000, S000\n"
	"mfv %0, S000\n"
	: "=r"(a)
	: "r"(angle));
   return a;
}

// Funcion coseno
float M3D_Cos(float angle){
	float a; 
	__asm__ volatile (
	"mtv %1, S000\n"
	"vcst.s S001, VFPU_2_PI\n"
	"vmul.s S000, S000, S001\n"
	"vcos.s S000, S000\n"
	"mfv %0, S000\n"
	: "=r"(a)
	: "r"(angle));
   return a;
}

float M3D_Deg2Rad(float n){
	return (n*GU_PI)/180.0f;
	/*float ret;
	__asm__ volatile (
		"mtv     %1, S002\n"		//Load n in S000
		"vcst.s S001, VFPU_2_PI\n"	//Load PI in S001
		"vmul.s S001, S001, S002\n"	//S001 = n*PI
		"viim.s S002, 180\n"		//S002 = 180.0f
		"vdiv.s S000, S001, S002\n"	//S000 = n*PI / 180
		"mfv %0, S000\n"
	: "=r"(ret) : "r"(n));
	return ret;*/
}

float M3D_Rad2Deg(float n){
	return (n*180.0f)/GU_PI;
	//(((n)*180.0f)/GU_PI)
	/*float ret;
	__asm__ volatile (
		"mtv     %1, S002\n"		//Load n in S000
		"vcst.s S001, VFPU_2_PI\n"	//Load PI in S001
		"vmul.s S001, S001, S002\n"	//S001 = n*PI
		"viim.s S002, 180\n"		//S002 = 180.0f
		"vdiv.s S000, S001, S002\n"	//S000 = n*PI / 180
		"mfv %0, S000\n"
	: "=r"(ret) : "r"(n));
	return ret;*/
}

// Raíz cuadrada
float M3D_SquareRoot(float val){
	float ret;
	__asm__ volatile (
		"mtv     %1, S000\n"
		"vsqrt.s S000, S000\n"
		"mfv     %0, S000\n"
	: "=r"(ret) : "r"(val));
	return ret;
}

// Número aleatorio
float M3D_Randf(float min, float max){ 
	float ret;
	__asm__ volatile (
		"mtv      %1, S000\n"
		"mtv      %2, S001\n"
		"vsub.s   S001, S001, S000\n"
		"vone.s   S002\n"
		"vrndf1.s S003\n"
		"vsub.s   S003, S003, S002\n"
		"vmul.s   S001, S003, S001\n"
		"vadd.s   S000, S000, S001\n"
		"mfv      %0, S000\n"
    : "=r"(ret) : "r"(min), "r"(max));
	return ret;
}

// Longitud de un vector
float M3D_VectorLength(float x, float y, float z){
	return M3D_SquareRoot((x*x)+(y*y)+(z*z));
}


