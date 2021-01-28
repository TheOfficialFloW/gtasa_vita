#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <kubridge.h>

#include "main.h"
#include "so_util.h"

#include "config.h"
#include "opengl_patch.h"


/*
 * RW
 */

typedef uint8_t RwUInt8;
typedef uint16_t RwUInt16;
typedef uint32_t RwUInt32;
typedef int8_t RwInt8;
typedef int16_t RwInt16;
typedef int32_t RwInt32;
typedef float RwReal;

typedef struct RwV3d RwV3d;
struct RwV3d
{
	RwReal x;   /**< X value */
	RwReal y;   /**< Y value */
	RwReal z;   /**< Z value */
};

typedef struct RwMatrix RwMatrix;
struct RwMatrix
{
	/* These are padded to be 16 byte quantities per line */
	RwV3d               right;
	RwUInt32            flags;
	RwV3d               up;
	RwUInt32            pad1;
	RwV3d               at;
	RwUInt32            pad2;
	RwV3d               pos;
	RwUInt32            pad3;
};

typedef enum RwOpCombineType RwOpCombineType;
enum RwOpCombineType
{
	rwCOMBINEREPLACE = 0,   /**<Replace - 
				all previous transformations are lost */
	rwCOMBINEPRECONCAT,     /**<Pre-concatenation - 
				the given transformation is applied 
				before all others */
	rwCOMBINEPOSTCONCAT,    /**<Post-concatenation - 
				the given transformation is applied 
				after all others */
};

typedef struct RwRGBAReal RwRGBAReal;
struct RwRGBAReal
{
	RwReal red;     /**< red component */
	RwReal green;   /**< green component */
	RwReal blue;    /**< blue component */
	RwReal alpha;   /**< alpha component */
};

typedef struct RwRGBA RwRGBA;
struct RwRGBA
{
	RwUInt8 red;    /**< red component */
	RwUInt8 green;  /**< green component */
	RwUInt8 blue;   /**< blue component */
	RwUInt8 alpha;  /**< alpha component */
};

typedef struct RwLLLink RwLLLink;
struct RwLLLink
{
	RwLLLink *next;
	RwLLLink *prev;
};

typedef struct RwLinkList RwLinkList;
struct RwLinkList
{
	RwLLLink link;
};

typedef struct RwObject RwObject;
struct RwObject
{
	RwUInt8 type;                /**< Internal Use */
	RwUInt8 subType;             /**< Internal Use */
	RwUInt8 flags;               /**< Internal Use */
	RwUInt8 privateFlags;        /**< Internal Use */
	void   *parent;              /**< Internal Use */
                                     /* Often a Frame  */
};
#define rwObjectGetParent(object)           (((const RwObject *)(object))->parent)

typedef struct RwFrame RwFrame;
struct RwFrame
{
	RwObject            object;
	
	RwLLLink            inDirtyListLink;
	
	/* Put embedded matrices here to ensure they remain 16-byte aligned */
	RwMatrix            modelling;
	RwMatrix            ltm;
	
	RwLinkList          objectList; /* List of objects connected to a frame */
	
	struct RwFrame      *child;
	struct RwFrame      *next;
	struct RwFrame      *root;   /* Root of the tree */
};
RwFrame *(*RwFrameTransform)(RwFrame * frame, const RwMatrix * m, RwOpCombineType combine);


typedef struct RwObjectHasFrame RwObjectHasFrame;
typedef RwObjectHasFrame * (*RwObjectHasFrameSyncFunction)(RwObjectHasFrame *object);
struct RwObjectHasFrame
{
	RwObject                     object;
	RwLLLink                     lFrame;
	RwObjectHasFrameSyncFunction sync;
};

typedef struct RpLight RpLight;
struct RpLight
{
	RwObjectHasFrame    object; /**< object */
	RwReal              radius; /**< radius */
	RwRGBAReal          color; /**< color */  /* Light color */
	RwReal              minusCosAngle; /**< minusCosAngle */  
	RwLinkList          WorldSectorsInLight; /**< WorldSectorsInLight */
	RwLLLink            inWorld; /**< inWorld */
	RwUInt16            lightFrame; /**< lightFrame */
	// war drum (?) change
	RwUInt8             spec;
	RwUInt8             pad;
//	RwUInt16            pad;
};
#define RpLightGetParent(light) ((RwFrame*)rwObjectGetParent(light))
RpLight *(*RpLightSetColor)(RpLight *light, const RwRGBAReal *color);



/*
 * GTA
 */

typedef struct CVector CVector;
struct CVector { float x, y, z; };

CVector
CrossProduct(CVector *v1, CVector *v2)
{
	CVector cross;
	cross.x = v1->y*v2->z - v1->z*v2->y;
	cross.y = v1->z*v2->x - v1->x*v2->z;
	cross.z = v1->x*v2->y - v1->y*v2->x;
	return cross;
}

void
CVector__Normalise(CVector *vec)
{
	float lensq = vec->x*vec->x + vec->y*vec->y + vec->z*vec->z;
	if(lensq > 0.0f){
		float invsqrt = 1.0f/sqrtf(lensq);
		vec->x *= invsqrt;
		vec->y *= invsqrt;
		vec->z *= invsqrt;
	}else
		vec->x = 1.0f;
}

typedef struct CColourSet CColourSet;
struct CColourSet
{
	float ambr;
	float ambg;
	float ambb;
	float ambobjr;
	float ambobjg;
	float ambobjb;
	float ambBeforeBrightnessr;
	float ambBeforeBrightnessg;
	float ambBeforeBrightnessb;
	int16_t skytopr;
	int16_t skytopg;
	int16_t skytopb;
	int16_t skybotr;
	int16_t skybotg;
	int16_t skybotb;
	int16_t suncorer;
	int16_t suncoreg;
	int16_t suncoreb;
	int16_t suncoronar;
	int16_t suncoronag;
	int16_t suncoronab;
	float sunsz;
	float sprsz;
	float sprbght;
	int16_t shd;
	int16_t lightshd;
	int16_t poleshd;
	float farclp;
	float fogst;
	float lightonground;
	int16_t lowcloudr;
	int16_t lowcloudg;
	int16_t lowcloudb;
	int16_t fluffycloudr;
	int16_t fluffycloudg;
	int16_t fluffycloudb;
	float waterr;
	float waterg;
	float waterb;
	float watera;
	float postfx1r;
	float postfx1g;
	float postfx1b;
	float postfx1a;
	float postfx2r;
	float postfx2g;
	float postfx2b;
	float postfx2a;
	float cloudalpha;
	int intensityLimit;
	int16_t waterfogalpha;
	float directionalmult;
	float lodDistMult;

	// colorcycle grades here..
};
CColourSet *p_CTimeCycle__m_CurrentColours;
CVector *p_CTimeCycle__m_vecDirnLightToSun;
float *p_gfLaRiotsLightMult;
float *p_CCoronas__LightsMult;
uint8_t *p_CWeather__LightningFlash;

RpLight **p_pDirect;
RpLight **p_pAmbient;
RwRGBAReal *p_AmbientLightColourForFrame;
RwRGBAReal *p_AmbientLightColourForFrame_PedsCarsAndObjects;
RwRGBAReal *p_DirectionalLightColourForFrame;
RwRGBAReal *p_DirectionalLightColourFromDay;

void
SetLightsWithTimeOfDayColour(void *world)
{
	(*p_pDirect)->spec = 1;
	if(*p_pAmbient){
		float ambMult = *p_gfLaRiotsLightMult * *p_CCoronas__LightsMult;
		p_AmbientLightColourForFrame->red = p_CTimeCycle__m_CurrentColours->ambr * ambMult;
		p_AmbientLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->ambg * ambMult;
		p_AmbientLightColourForFrame->blue = p_CTimeCycle__m_CurrentColours->ambb * ambMult;
		RpLightSetColor(*p_pAmbient, p_AmbientLightColourForFrame);

		float ambObjMult = *p_CCoronas__LightsMult;
		p_AmbientLightColourForFrame_PedsCarsAndObjects->red = p_CTimeCycle__m_CurrentColours->ambobjr * ambObjMult;
		p_AmbientLightColourForFrame_PedsCarsAndObjects->green = p_CTimeCycle__m_CurrentColours->ambobjg * ambObjMult;
		p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = p_CTimeCycle__m_CurrentColours->ambobjb * ambObjMult;

		if(*p_CWeather__LightningFlash){
			p_AmbientLightColourForFrame->red = 1.0f;
			p_AmbientLightColourForFrame->green = 1.0f;
			p_AmbientLightColourForFrame->blue = 1.0f;

			p_AmbientLightColourForFrame_PedsCarsAndObjects->red = 1.0f;
			p_AmbientLightColourForFrame_PedsCarsAndObjects->green = 1.0f;
			p_AmbientLightColourForFrame_PedsCarsAndObjects->blue = 1.0f;
		}
		// this is used by objects with alpha test for whatever reason
		*p_DirectionalLightColourFromDay = *p_AmbientLightColourForFrame;
	}

	if(*p_pDirect){
		float dirMult = 256.0f/255.0f * *p_CCoronas__LightsMult;
		p_DirectionalLightColourForFrame->red = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
		p_DirectionalLightColourForFrame->green = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
		p_DirectionalLightColourForFrame->blue = p_CTimeCycle__m_CurrentColours->directionalmult * dirMult;
		RpLightSetColor(*p_pDirect, p_DirectionalLightColourForFrame);

		RwMatrix mat;
		memset(&mat, 0, sizeof(mat));
		CVector vecsun = *p_CTimeCycle__m_vecDirnLightToSun;
		CVector vec1 = { 0.0f, 0.0f, 1.0f };
		CVector vec2 = CrossProduct(&vec1, &vecsun);
		CVector__Normalise(&vec2);
		vec1 = CrossProduct(&vec2, &vecsun);
		mat.at.x = -vecsun.x;
		mat.at.y = -vecsun.y;
		mat.at.z = -vecsun.z;
		mat.right.x = vec1.x;
		mat.right.y = vec1.y;
		mat.right.z = vec1.z;
		mat.up.x = vec2.x;
		mat.up.y = vec2.y;
		mat.up.z = vec2.z;
		RwFrameTransform(RpLightGetParent(*p_pDirect), &mat, rwCOMBINEREPLACE);
	}
}


void ColorFilter(void *sp) {
	// grading values (i.e. the color matrix)
	RwRGBAReal *red = (RwRGBAReal *)(sp + 0x30);
	RwRGBAReal *green = (RwRGBAReal *)(sp + 0x20);
	RwRGBAReal *blue = (RwRGBAReal *)(sp + 0x10);

	// postfx values from timecycle
	RwRGBA *postfx1 = (RwRGBA *)(sp + 0x0c);
	RwRGBA *postfx2 = (RwRGBA *)(sp + 0x08);

	// NB: this assumes PS2 timecycle alphas

	// PS2
	float a = postfx2->alpha/128.0f;
	red->red = postfx1->red/128.0f + a*postfx2->red/128.0f;
	green->green = postfx1->green/128.0f + a*postfx2->green/128.0f;
	blue->blue = postfx1->blue/128.0f + a*postfx2->blue/128.0f;
	red->green = red->blue = red->alpha = 0.0f;
	green->red = green->blue = green->alpha = 0.0f;
	blue->red = blue->green = blue->alpha = 0.0f;

	// PC
/*
	float a1 = postfx1->alpha/128.0f;
	float a2 = postfx2->alpha/128.0f;
	red->red = 1.0f + a1*postfx1->red/255.0f + a2*postfx2->red/255.0f;
	green->green = 1.0f + a1*postfx1->green/255.0f + a2*postfx2->green/255.0f;
	blue->blue = 1.0f + a1*postfx1->blue/255.0f + a2*postfx2->blue/255.0f;
	red->green = red->blue = red->alpha = 0.0f;
	green->red = green->blue = green->alpha = 0.0f;
	blue->red = blue->green = blue->alpha = 0.0f;
*/
}

__attribute__((naked)) void _ColorFilter_stub(void) {
	asm volatile(
		"add r0, sp, 0x10\n"
		"bl ColorFilter\n"
		"vldr s2, [sp, #(0x30+0x10)]\n" // red.r
		"vldr s4, [sp, #(0x24+0x10)]\n" // green.g
		"vldr s6, [sp, #(0x18+0x10)]\n" // blue.b
		"pop {r0-r3}\n"
		"mov lr, %0\n"
		"bx lr\n"
	:: "r" (text_base + 0x005B6444 + 0x1) : "r0", "r1", "r2", "r3");
}

__attribute__((naked)) void ColorFilter_stub(void) {
	asm volatile(
		"push {r0-r3}\n"
		"b _ColorFilter_stub\n"
	);
}


void
patch_gfx(void)
{
	p_pAmbient = (RpLight**)so_find_addr("pAmbient");
	p_pDirect = (RpLight**)so_find_addr("pDirect");
	p_AmbientLightColourForFrame = (RwRGBAReal*)so_find_addr("AmbientLightColourForFrame");
	p_AmbientLightColourForFrame_PedsCarsAndObjects = (RwRGBAReal*)so_find_addr("AmbientLightColourForFrame_PedsCarsAndObjects");
	p_DirectionalLightColourForFrame = (RwRGBAReal*)so_find_addr("DirectionalLightColourForFrame");
	p_DirectionalLightColourFromDay = (RwRGBAReal*)so_find_addr("DirectionalLightColourFromDay");
	p_CTimeCycle__m_CurrentColours = (CColourSet*)so_find_addr("_ZN10CTimeCycle16m_CurrentColoursE");
	p_CTimeCycle__m_vecDirnLightToSun = (CVector*)so_find_addr("_ZN10CTimeCycle19m_vecDirnLightToSunE");
	p_gfLaRiotsLightMult = (float*)so_find_addr("gfLaRiotsLightMult");
	p_CCoronas__LightsMult = (float*)so_find_addr("_ZN8CCoronas10LightsMultE");
	p_CWeather__LightningFlash = (uint8_t*)so_find_addr("_ZN8CWeather14LightningFlashE");

	RwFrameTransform = (RwFrame *(*)(RwFrame*,const RwMatrix*,RwOpCombineType))so_find_addr("_Z16RwFrameTransformP7RwFramePK11RwMatrixTag15RwOpCombineType");
	RpLightSetColor = (RpLight *(*)(RpLight*, const RwRGBAReal*))so_find_addr("_Z15RpLightSetColorP7RpLightPK10RwRGBAReal");

	hook_thumb(so_find_addr("_Z28SetLightsWithTimeOfDayColourP7RpWorld"), (uintptr_t)SetLightsWithTimeOfDayColour);

#ifdef ENABLE_COLOR_FILTER_PS2
	// .text:005B63DC                 LDRB            R0, [R3] ; CPostEffects::m_bDarknessFilter
	// .text:005B63DE                 STRB.W          R5, [SP,#0x68+rgba2+2]
	// .text:005B63E2                 VADD.F32        S6, S6, S12
	// .text:005B63E6                 VLDR            S12, [SP,#0x68+var_48+4]
	// .text:005B63EA                 CMP             R0, #0
	// ...
	// .text:005B643C                 VSTR            S2, [SP,#0x30]
	// .text:005B6440                 VSTR            S4, [SP,#0x24]
	// .text:005B6444                 VSTR            S6, [SP,#0x18]
	// .text:005B6448                 BEQ             loc_5B64EC
	hook_thumb((uintptr_t)text_base + 0x005B643C, (uintptr_t)ColorFilter_stub);

	kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x005B6444), (void *)(text_base + 0x005B63DC), sizeof(uint16_t));
	kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x005B6446), (void *)(text_base + 0x005B63EA), sizeof(uint16_t));
#endif
#ifdef ENABLE_SUN_CORONA_PS2
	const uint32_t nop = 0xbf00bf00;
	kuKernelCpuUnrestrictedMemcpy((void *)(text_base + 0x005A26B0), &nop, sizeof(uint32_t));
#endif
}


/*
 * Shader builder
 */


void BuildVertexSource_SkyGfx(int flags) {
	char tmp[512];
	char *vertexColor, *tex;

	VTX_EMIT("void main(");

	VTX_EMIT("float3 Position,");
	if (flags & FLAG_TEX0) {
		if (flags & FLAG_PROJECT_TEXCOORD)
			VTX_EMIT("float4 TexCoord0,");
		else
			VTX_EMIT("float2 TexCoord0,");
	}
	VTX_EMIT("float3 Normal,");
	VTX_EMIT("float4 GlobalColor,");

	if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
		VTX_EMIT("float4 BoneWeight,");
		VTX_EMIT("float4 BoneIndices,");
	}

	if (flags & FLAG_COLOR2)
		VTX_EMIT("float4 Color2,");

	VTX_EMIT("uniform float4x4 ProjMatrix,");
	VTX_EMIT("uniform float4x4 ViewMatrix,");
	VTX_EMIT("uniform float4x4 ObjMatrix,");

	if (flags & FLAG_LIGHTING) {
		VTX_EMIT("uniform half3 AmbientLightColor,");
		VTX_EMIT("uniform half4 MaterialEmissive,");
		VTX_EMIT("uniform half4 MaterialAmbient,");
		VTX_EMIT("uniform half4 MaterialDiffuse,");
		if (flags & FLAG_LIGHT1) {
			VTX_EMIT("uniform half3 DirLightDiffuseColor,");
			VTX_EMIT("uniform float3 DirLightDirection,");
			if (GetMobileEffectSetting() == 3) {
				if (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4))
					VTX_EMIT("uniform float3 DirBackLightDirection,");
			}
		}
		if (flags & FLAG_LIGHT2) {
			VTX_EMIT("uniform half3 DirLight2DiffuseColor,");
			VTX_EMIT("uniform float3 DirLight2Direction,");
		}
		if (flags & FLAG_LIGHT3) {
			VTX_EMIT("uniform half3 DirLight3DiffuseColor,");
			VTX_EMIT("uniform float3 DirLight3Direction,");
		}
	}

	if (flags & (FLAG_BONE3 | FLAG_BONE4))
		VTX_EMIT("uniform float4 Bones[%d],", *RQMaxBones * 3 + 4);

	if (flags & FLAG_TEXMATRIX)
		VTX_EMIT("uniform float3x3 NormalMatrix,");

	if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
		VTX_EMIT("uniform half EnvMapCoefficient,");

	if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | PED_SPEC | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_WATER | FLAG_SPHERE_XFORM))
		VTX_EMIT("uniform float3 CameraPosition,");

	if (flags & FLAG_FOG)
		VTX_EMIT("uniform float3 FogDistances,");

	if (flags & FLAG_WATER)
		VTX_EMIT("uniform float3 WaterSpecs,");

	if (flags & FLAG_COLOR2)
		VTX_EMIT("uniform half ColorInterp,");

	if (flags & FLAG_TEX0)
		VTX_EMIT("half2 out Out_Tex0 : TEXCOORD0,");

	if (flags & FLAG_ENVMAP)
		VTX_EMIT("half2 out Out_Tex1 : TEXCOORD1,");
	else if (flags & FLAG_SPHERE_ENVMAP)
		VTX_EMIT("half3 out Out_Refl : TEXCOORD1,");

	if (flags & FLAG_WATER) {
		VTX_EMIT("half2 out Out_WaterDetail : TEXCOORD2,");
		VTX_EMIT("half2 out Out_WaterDetail2 : TEXCOORD3,");
		VTX_EMIT("half out Out_WaterAlphaBlend : TEXCOORD4,");
	}

	if (flags & (FLAG_COLOR | FLAG_LIGHTING))
		VTX_EMIT("half4 out Out_Color : COLOR0,");

	if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | PED_SPEC)))
		VTX_EMIT("half3 out Out_Spec : COLOR1,");

	if (flags & FLAG_FOG)
		VTX_EMIT("half out Out_FogAmt : FOG,");

	VTX_EMIT("float4 out gl_Position : POSITION,");

	if (vtxbuf[strlen(vtxbuf)-1] == ',')
		vtxbuf[strlen(vtxbuf)-1] = '\0';

	VTX_EMIT(") {");

	if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
		VTX_EMIT("int4 BlendIndexArray = int4(BoneIndices);");
		VTX_EMIT("float4x4 BoneToLocal;");
		VTX_EMIT("BoneToLocal[0] = Bones[BlendIndexArray.x*3] * BoneWeight.x;");
		VTX_EMIT("BoneToLocal[1] = Bones[BlendIndexArray.x*3+1] * BoneWeight.x;");
		VTX_EMIT("BoneToLocal[2] = Bones[BlendIndexArray.x*3+2] * BoneWeight.x;");
		VTX_EMIT("BoneToLocal[3] = float4(0.0,0.0,0.0,1.0);");
		VTX_EMIT("BoneToLocal[0] += Bones[BlendIndexArray.y*3] * BoneWeight.y;");
		VTX_EMIT("BoneToLocal[1] += Bones[BlendIndexArray.y*3+1] * BoneWeight.y;");
		VTX_EMIT("BoneToLocal[2] += Bones[BlendIndexArray.y*3+2] * BoneWeight.y;");
		VTX_EMIT("BoneToLocal[0] += Bones[BlendIndexArray.z*3] * BoneWeight.z;");
		VTX_EMIT("BoneToLocal[1] += Bones[BlendIndexArray.z*3+1] * BoneWeight.z;");
		VTX_EMIT("BoneToLocal[2] += Bones[BlendIndexArray.z*3+2] * BoneWeight.z;");
		if (flags & FLAG_BONE4) {
			VTX_EMIT("BoneToLocal[0] += Bones[BlendIndexArray.w*3] * BoneWeight.w;");
			VTX_EMIT("BoneToLocal[1] += Bones[BlendIndexArray.w*3+1] * BoneWeight.w;");
			VTX_EMIT("BoneToLocal[2] += Bones[BlendIndexArray.w*3+2] * BoneWeight.w;");
		}
		VTX_EMIT("float4 WorldPos = mul(mul(BoneToLocal, float4(Position, 1.0)), ObjMatrix);");
	} else {
		VTX_EMIT("float4 WorldPos = mul(float4(Position, 1.0), ObjMatrix);");
	}

	if (flags & FLAG_SPHERE_XFORM) {
		VTX_EMIT("float3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
		VTX_EMIT("float3 ReflPos = normalize(ReflVector);");
		VTX_EMIT("ReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
		VTX_EMIT("gl_Position = float4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
	} else {
		VTX_EMIT("float4 ViewPos = mul(WorldPos, ViewMatrix);");
		VTX_EMIT("gl_Position = mul(ViewPos, ProjMatrix);");
	}

	if (flags & FLAG_LIGHTING) {
		if (((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) && (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3))) {
			// unused
			VTX_EMIT("float3 WorldNormal = normalize(float3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
		} else {
			if (flags & (FLAG_BONE3 | FLAG_BONE4))
				VTX_EMIT("float3 WorldNormal = mul(mul(float3x3(BoneToLocal), Normal), float3x3(ObjMatrix));");
			else
				VTX_EMIT("float3 WorldNormal = (mul(float4(Normal, 0.0), ObjMatrix)).xyz;");
		}
	} else {
		if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
			VTX_EMIT("float3 WorldNormal = float3(0.0, 0.0, 0.0);");
	}

	if (flags & FLAG_FOG)
		VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");

	if (flags & FLAG_TEX0) {
		if (flags & FLAG_PROJECT_TEXCOORD)
			tex = "TexCoord0.xy / TexCoord0.w";
		else if (flags & FLAG_COMPRESSED_TEXCOORD)
			tex = "TexCoord0 / 512.0";
		else
			tex = "TexCoord0";

		if (flags & FLAG_TEXMATRIX)
			VTX_EMIT("Out_Tex0 = mul(float3(%s, 1.0), NormalMatrix).xy;", tex);
		else
			VTX_EMIT("Out_Tex0 = %s;", tex);
	}

	if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
		VTX_EMIT("float3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
		VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
		if (flags & FLAG_SPHERE_ENVMAP)
			VTX_EMIT("Out_Refl = reflVector;");
		else
			VTX_EMIT("Out_Tex1 = float2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);");
	}

	if (flags & FLAG_COLOR2) {
		VTX_EMIT("half4 InterpColor = lerp(GlobalColor, Color2, ColorInterp);");
		vertexColor = "InterpColor";
	} else {
		vertexColor = "GlobalColor";
	}

	// Lighting
	if (flags & FLAG_LIGHTING) {
		//VTX_EMIT("half3 Out_LightingColor;");

		VTX_EMIT("half3 ambEmissLight;");
		VTX_EMIT("half3 diffColor = half3(0.0, 0.0, 0.0);");

		// Ambient and Emissive Light
		if (flags & FLAG_COLOR_EMISSIVE) {
			if(flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)){
				// TOTAL HACK for 3d markers (looks like we can catch them here).
				// material color is not white but prelight wasn't adjusted.
				// we happen to know that diffuse light is white, so MaterialDiffuse should be
				// the unmodified materialcolor and we can multiply with it.
				VTX_EMIT("ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + %s.xyz * MaterialDiffuse.xyz;", vertexColor);
			}else
			if (flags & FLAG_CAMERA_BASED_NORMALS){
				// This happens to objects with alpha test.
				// trees in particular tend have their vertex colors cranked to white
				// so let's try to tone that down a bit
				VTX_EMIT("half3 vertClamped = clamp(%s.xyz, 0.0, 0.5);", vertexColor);
				// NB: AmbientLightColor is DirectionalLightColourFromDay here
				VTX_EMIT("ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + vertClamped;");
				// dunno what they were going for with this. flat shading because they messed up the prelight?
			//	VTX_EMIT("ambEmissLight = AmbientLightColor * MaterialAmbient.xyz * 1.5;");
			}else
				VTX_EMIT("ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + %s.xyz;", vertexColor);
		} else {
			VTX_EMIT("ambEmissLight = AmbientLightColor * MaterialAmbient.xyz + MaterialEmissive.xyz;");
		}

		// Diffuse Light
		if (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)) {
			if (flags & FLAG_LIGHT1) {
				if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
					VTX_EMIT("diffColor += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;");
				else
					VTX_EMIT("diffColor += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;");
			}
			if (flags & FLAG_LIGHT2)
				VTX_EMIT("diffColor += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;");
			if (flags & FLAG_LIGHT3)
				VTX_EMIT("diffColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
			VTX_EMIT("diffColor *= MaterialDiffuse.xyz;");
		}

		// Final Color
		if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
			// this makes no sense
//			if (flags & FLAG_COLOR)
//				VTX_EMIT("Out_Color = half4((Out_LightingColor.xyz + %s.xyz * 1.5) * MaterialDiffuse.xyz, (MaterialAmbient.w) * %s.w);", vertexColor, vertexColor);
//			else
				VTX_EMIT("Out_Color = half4(ambEmissLight + diffColor, MaterialAmbient.w * %s.w);", vertexColor);
			VTX_EMIT("Out_Color = clamp(Out_Color, 0.0, 1.0);");
		}
	} else {
		if (flags & (FLAG_COLOR | FLAG_LIGHTING))
			VTX_EMIT("Out_Color = %s;", vertexColor);
	}

	if (!RQCaps->unk_08 && (flags & FLAG_LIGHT1)) {
		if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
			// original, but fixed clamp for pow
			//VTX_EMIT("half specAmt = pow(max(dot(reflVector, DirLightDirection), 0.0), %.1f) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
			//VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");

			// just testing doing it differently
			//VTX_EMIT("float3 specVector = normalize(CameraPosition.xyz - WorldPos.xyz);");
			//VTX_EMIT("half specAmt = pow(max(dot(WorldNormal, normalize(specVector + DirLightDirection)), 0.0), 16.0);");
			//VTX_EMIT("specAmt *= 2.0 * EnvMapCoefficient;");
			//VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");

			// ps2 specdot - reflect in view space
			VTX_EMIT("half3 ViewNormal = (mul(float4(WorldNormal, 0.0), ViewMatrix)).xyz;");
			VTX_EMIT("half3 ViewLight = (mul(float4(DirLightDirection, 0.0), ViewMatrix)).xyz;");
			VTX_EMIT("half3 V = ViewLight - 2.0*ViewNormal*dot(ViewNormal, ViewLight);");
			// find some nice specular value -- not the real thing unfortunately
			VTX_EMIT("half specAmt = 1.0 * EnvMapCoefficient * DirLightDiffuseColor.x;");
			// NB: this is not a color here!!
			VTX_EMIT("Out_Spec = (V + half3(1.0, 1.0, 0.0))/2.0;");
			VTX_EMIT("if(Out_Spec.z < 0.0) Out_Spec.z = specAmt; else Out_Spec.z = 0.0;");
		} else if (flags & PED_SPEC) {
			VTX_EMIT("half3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
			VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
			VTX_EMIT("half specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
			VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");
		}
	}

	if (flags & FLAG_WATER) {
		VTX_EMIT("Out_WaterDetail = (Out_Tex0 * 4.0) + float2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);");
		VTX_EMIT("Out_WaterDetail2 = (Out_Tex0 * -8.0) + float2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);");
		VTX_EMIT("Out_WaterAlphaBlend = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;");
	}

	VTX_EMIT("}");
}



void BuildPixelSource_SkyGfx(int flags) {
	char tmp[512];

	PXL_EMIT("half4 main(");

	if (flags & FLAG_TEX0)
		PXL_EMIT("half2 Out_Tex0 : TEXCOORD0,");

	if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
		if (flags & FLAG_ENVMAP)
			PXL_EMIT("half2 Out_Tex1 : TEXCOORD1,");
		else
			PXL_EMIT("half3 Out_Refl : TEXCOORD1,");
	}

	if (flags & FLAG_WATER) {
		PXL_EMIT("half2 Out_WaterDetail : TEXCOORD2,");
		PXL_EMIT("half2 Out_WaterDetail2 : TEXCOORD3,");
		PXL_EMIT("half Out_WaterAlphaBlend : TEXCOORD4,");
	}

	if (flags & (FLAG_COLOR | FLAG_LIGHTING))
		PXL_EMIT("half4 Out_Color : COLOR0,");

	if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | PED_SPEC)))
		PXL_EMIT("half3 Out_Spec : COLOR1,");

	if (flags & FLAG_FOG)
		PXL_EMIT("half Out_FogAmt : FOG,");

	if (flags & FLAG_TEX0)
		PXL_EMIT("uniform sampler2D Diffuse,");

	if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
		PXL_EMIT("uniform sampler2D EnvMap,");
		PXL_EMIT("uniform half EnvMapCoefficient,");
	} else if (flags & FLAG_DETAILMAP) {
		PXL_EMIT("uniform sampler2D EnvMap,");
		PXL_EMIT("uniform half DetailTiling,");
	}

	if (flags & FLAG_FOG)
		PXL_EMIT("uniform half3 FogColor,");

	if (flags & FLAG_ALPHA_MODULATE)
		PXL_EMIT("uniform half AlphaModulate,");

	if (pxlbuf[strlen(pxlbuf)-1] == ',')
		pxlbuf[strlen(pxlbuf)-1] = '\0';

	PXL_EMIT(") {");

	PXL_EMIT("half4 fcolor;");
	if (flags & FLAG_TEX0) {
		if (flags & FLAG_TEXBIAS)
			PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, half4(Out_Tex0, 0.0, -1.5));");
#ifndef SLOW_GPU
		else if (!RQCaps->isSlowGPU)
			PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, half4(Out_Tex0, 0.0, -0.5));");
#endif
		else
			PXL_EMIT("half4 diffuseColor = tex2D(Diffuse, Out_Tex0);");

		PXL_EMIT("fcolor = diffuseColor;");
// no texture. for testing lighting
//if (flags & FLAG_LIGHTING)
//	PXL_EMIT("fcolor = half4(1.0, 1.0, 1.0, 1.0);");

		if (!(flags & (FLAG_COLOR | FLAG_LIGHTING))) {
			if (flags & FLAG_WATER)
				PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
		} else if (!(flags & FLAG_DETAILMAP)) {
			PXL_EMIT("fcolor *= Out_Color;");
			if (flags & FLAG_WATER)
				PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
		} else {
			if (flags & FLAG_WATER) {
				PXL_EMIT("half waterDetail = tex2Dbias(EnvMap, half4(Out_WaterDetail, 0.0, -1.0)).x + tex2Dbias(EnvMap, half4(Out_WaterDetail2, 0.0, -1.0)).x;");
				PXL_EMIT("fcolor *= half4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
				PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
			} else {
				PXL_EMIT("fcolor *= half4(Out_Color.xyz * tex2Dbias(EnvMap, half4(Out_Tex0.xy * DetailTiling, 0.0, -0.5)).xyz * 2.0, Out_Color.w);");
			}
		}
	} else {
		if (flags & (FLAG_COLOR | FLAG_LIGHTING))
			PXL_EMIT("fcolor = Out_Color;");
		else
			PXL_EMIT("fcolor = 0.0;");
	}

// only look at specular for the moment
//	if (flags & FLAG_ENVMAP)
//		PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, tex2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");

	if (flags & FLAG_SPHERE_ENVMAP) {
		PXL_EMIT("half2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);");
		PXL_EMIT("ReflPos = (ReflPos * half2(0.5, 0.5)) + half2(0.5, 0.5);");
		PXL_EMIT("half4 ReflTexture = tex2D(EnvMap, ReflPos);");
		PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, ReflTexture.xyz, EnvMapCoefficient);");
		PXL_EMIT("fcolor.w += ReflTexture.b * 0.125;");
	}

	if (!RQCaps->unk_08) {
		if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | PED_SPEC))){
			// When treating this as light
			//PXL_EMIT("fcolor.xyz += Out_Spec;");

			// PS2 has UV coors for specdot instead, but we don't actually have the texture. so simulate it
			PXL_EMIT("half2 unpack = (Out_Spec.xy-half2(0.5, 0.5))*2.0;");
			PXL_EMIT("half3 specColor = half3(Out_Spec.z, Out_Spec.z, Out_Spec.z);");
			PXL_EMIT("half dist = unpack.x*unpack.x + unpack.y*unpack.y;");
			// outside the dot
			PXL_EMIT("if(dist > 0.69*0.69) specColor *= 0.0;");
			// smooth the edge
			PXL_EMIT("else if(dist > 0.67*0.67) specColor *= (0.69*0.69 - dist)/(0.69*0.69 - 0.67*0.67);");
			PXL_EMIT("fcolor.xyz += specColor;");
		}
		if (flags & FLAG_FOG)
			PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, FogColor, Out_FogAmt);");
	}

	if (flags & FLAG_GAMMA)
		PXL_EMIT("fcolor.xyz += fcolor.xyz * 0.5;");

	PXL_EMIT("half4 gl_FragColor = fcolor;");

#ifndef DISABLE_ALPHA_TESTING
	if (flags & FLAG_ALPHA_TEST) {
		PXL_EMIT("/*ATBEGIN*/");
		if ((OS_SystemChip() == 13) && (flags & FLAG_TEX0)) {
			if (flags & FLAG_TEXBIAS) {
				PXL_EMIT("if (diffuseColor.a < 0.8) { discard; }");
			} else {
				if (flags & FLAG_CAMERA_BASED_NORMALS) {
					PXL_EMIT("gl_FragColor.a = Out_Color.a;");
					PXL_EMIT("if (diffuseColor.a < 0.5) { discard; }");
				} else {
					PXL_EMIT("if (diffuseColor.a < 0.2) { discard; }");
				}
			}
		} else {
			if (flags & FLAG_TEXBIAS) {
				PXL_EMIT("if (gl_FragColor.a < 0.8) { discard; }");
			} else {
				if (flags & FLAG_CAMERA_BASED_NORMALS) {
					PXL_EMIT("if (gl_FragColor.a < 0.5) { discard; }");
					PXL_EMIT("gl_FragColor.a = Out_Color.a;");
				} else {
					PXL_EMIT("if (gl_FragColor.a < 0.2) { discard; }");
				}
			}
		}
		PXL_EMIT("/*ATEND*/");
	}
#endif

	if (flags & FLAG_ALPHA_MODULATE)
		PXL_EMIT("gl_FragColor.a *= AlphaModulate;");

	PXL_EMIT("return gl_FragColor;");
	PXL_EMIT("}");
}
