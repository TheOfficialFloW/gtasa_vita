#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"

uintptr_t find_addr_by_symbol(char *symbol);
void hook_thumb(uintptr_t addr, uintptr_t dst);

static char pxlbuf[8192];
static char vtxbuf[8192];

static int disable_amp = 0;
static int is_mali_chip = 0;
static int byte_5D6B42 = 0;
static int *RQMaxBones;

int GetMobileEffectSetting() {
  return 0; // 3
}

int OS_SystemChip() {
  return 0; // 7, 8, 9, 10
}

#define FLAG_ALPHA_TEST           0x01
#define FLAG_LIGHTING             0x02
#define FLAG_ALPHA_MODULATE       0x04
#define FLAG_COLOR_EMISSIVE       0x08
#define FLAG_COLOR                0x10
#define FLAG_TEX0                 0x20
#define FLAG_TEX1                 0x40
#define FLAG_BONE3                0x80
#define FLAG_BONE4                0x100
#define FLAG_CAMERA_BASED_NORMALS 0x200
#define FLAG_FOG                  0x400
#define FLAG_TEXBIAS              0x800
#define FLAG_BACKLIGHT            0x1000
#define FLAG_LIGHT1               0x2000
#define FLAG_LIGHT2               0x4000
#define FLAG_LIGHT3               0x8000
#define FLAG_ENVMAP               0x10000
#define FLAG_COMPRESSED_TEXCOORD  0x20000
#define FLAG_PROJECT_TEXCOORD     0x40000
#define FLAG_WATER                0x80000
#define FLAG_COLOR2               0x100000
#define FLAG_REFLECTION           0x800000
#define FLAG_REFL_OUT             0x1000000
#define FLAG_NORMAL               0x2000000
#define FLAG_GAMMA                0x4000000

#define PXL_EMIT(...)                       \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    strcat(pxlbuf, tmp);                     \
  } while (0)


#define VTX_EMIT(...)                       \
  do {                                       \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__); \
    strcat(vtxbuf, tmp);                     \
  } while (0)

// glBindAttribLocation(*((_DWORD *)v3 + 250), 0, "Position");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 1, "TexCoord0");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 2, "Normal");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 3, "GlobalColor");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 4, "BoneWeight");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 5, "BoneIndices");
// glBindAttribLocation(*((_DWORD *)v3 + 250), 6, "Color2");

void BuildVertexSource(int flags) {
  char tmp[512];
  char *arg;

  flags |= FLAG_TEX0;

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
        if (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)) {
          VTX_EMIT("uniform float3 DirBackLightDirection,");
        }
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

  // if (flags & (FLAG_BONE3 | FLAG_BONE4))
    // VTX_EMIT("uniform float4x4 Bones[%d],", *RQMaxBones);
  if (flags & (FLAG_BONE3 | FLAG_BONE4))
    VTX_EMIT("uniform float4 Bones[%d],", *RQMaxBones * 3 + 4);

  if (flags & FLAG_NORMAL)
    VTX_EMIT("uniform float3x3 NormalMatrix,");

  if (flags & (FLAG_REFL_OUT | FLAG_TEX1))
    VTX_EMIT("uniform half EnvMapCoefficient,");

  if (flags & (FLAG_TEX1 | FLAG_BONE3 | FLAG_BONE4 | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_WATER | FLAG_REFLECTION | FLAG_REFL_OUT))
    VTX_EMIT("uniform float3 CameraPosition,");

  if (flags & FLAG_FOG) {
    VTX_EMIT("uniform float3 FogDistances,");
  }

  if (flags & FLAG_WATER)
    VTX_EMIT("uniform float3 WaterSpecs,");

  if (flags & FLAG_COLOR2) {
    VTX_EMIT("uniform half ColorInterp,");
  }

  if (flags & FLAG_TEX0)
    VTX_EMIT("half2 out Out_Tex0 : TEXCOORD0,");

  if (flags & FLAG_TEX1)
    VTX_EMIT("half2 out Out_Tex1 : TEXCOORD1,");
  else if (flags & FLAG_REFL_OUT)
    VTX_EMIT("half3 out Out_Refl : TEXCOORD1,");

  if (flags & FLAG_FOG)
    VTX_EMIT("half out Out_FogAmt : FOG,");

  if (flags & FLAG_WATER) {
    VTX_EMIT("half2 out Out_WaterDetail : TEXCOORD2,");
    VTX_EMIT("half2 out Out_WaterDetail2 : TEXCOORD3,");
    VTX_EMIT("half out Out_WaterAlphaBlend : TEXCOORD4,");
  }

  if (flags & (FLAG_COLOR | FLAG_LIGHTING))
    VTX_EMIT("half4 out Out_Color : COLOR0,");

  if ((flags & FLAG_LIGHT1) && (flags & (FLAG_TEX1 | FLAG_BONE3 | FLAG_BONE4 | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_REFL_OUT)))
    VTX_EMIT("half3 out Out_Spec : COLOR1,");

  VTX_EMIT("float4 out gl_Position : POSITION,");

  if (vtxbuf[strlen(vtxbuf)-1] == ',')
    vtxbuf[strlen(vtxbuf)-1] = '\0';

  VTX_EMIT(") {");

  if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
    VTX_EMIT("int4 BlendIndexArray = int4(BoneIndices);");
    VTX_EMIT("float4x4 BoneToLocal;");
    // VTX_EMIT("BoneToLocal = Bones[BlendIndexArray.x] * BoneWeight.x;");
    // VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.y] * BoneWeight.y;");
    // VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.z] * BoneWeight.z;");
    // if (flags & FLAG_BONE4)
      // VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.w] * BoneWeight.w;");
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
    VTX_EMIT("float4 BoneVertex = mul(float4(Position, 1.0), transpose(BoneToLocal));");
    VTX_EMIT("float4 WorldPos = mul(BoneVertex, ObjMatrix);");
  } else {
    VTX_EMIT("float4 WorldPos = mul(float4(Position, 1.0), ObjMatrix);");
  }

  if (flags & FLAG_REFLECTION) {
    VTX_EMIT("float3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
    VTX_EMIT("float3 ReflPos = normalize(ReflVector);");
    VTX_EMIT("ReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
    VTX_EMIT("gl_Position = float4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
  } else {
#ifdef WVP_OPTIMIZATION
    // With WVP optimization, ProjMatrix is the WVP matrix.
    if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
      VTX_EMIT("float4 ViewPos = mul(BoneVertex, ProjMatrix);");
    } else {
      VTX_EMIT("float4 ViewPos = mul(float4(Position, 1.0), ProjMatrix);");
    }
#else
    VTX_EMIT("float4 ViewPos = mul(WorldPos, ProjMatrix);");
#endif
    VTX_EMIT("gl_Position = ViewPos;");
  }

  if (flags & FLAG_LIGHTING) {
    if (((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) && (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3))) {
      VTX_EMIT("float3 WorldNormal = normalize(float3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
    } else {
      if (flags & (FLAG_BONE3 | FLAG_BONE4))
        VTX_EMIT("float3 WorldNormal = mul(mul(Normal, float3x3(transpose(BoneToLocal))), float3x3(ObjMatrix));"); // TODO: optimize tranposition
      else
        VTX_EMIT("float3 WorldNormal = (mul(float4(Normal, 0.0), ObjMatrix)).xyz;");
    }
  } else {
    if (flags & (FLAG_REFL_OUT | FLAG_TEX1))
      VTX_EMIT("float3 WorldNormal = float3(0.0, 0.0, 0.0);");
  }

  if (!disable_amp && (flags & FLAG_FOG)) {
    VTX_EMIT("Out_FogAmt = clamp((length(WorldPos.xyz - CameraPosition.xyz) - FogDistances.x) * FogDistances.z, 0.0, 0.90);");
  }

  if (flags & FLAG_TEX0) {
    if (flags & FLAG_PROJECT_TEXCOORD)
      arg = "TexCoord0.xy / TexCoord0.w";
    else if (flags & FLAG_COMPRESSED_TEXCOORD)
      arg = "TexCoord0 / 512.0";
    else
      arg = "TexCoord0";

    if (flags & FLAG_NORMAL) {
      VTX_EMIT("Out_Tex0 = (mul(float3(%s, 1.0), NormalMatrix).xy;", arg);
    } else {
      VTX_EMIT("Out_Tex0 = %s;", arg);
    }
  }

  if (flags & (FLAG_REFL_OUT | FLAG_TEX1)) {
    VTX_EMIT("float3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
    VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
    if (flags & FLAG_REFL_OUT)
      VTX_EMIT("Out_Refl = reflVector;");
    else
      VTX_EMIT("Out_Tex1 = float2(length(reflVector.xy), (reflVector.z * 0.5) + 0.25);");
  }

  if (flags & FLAG_COLOR2) {
    VTX_EMIT("half4 InterpColor = lerp(GlobalColor, Color2, ColorInterp);");
    arg = "InterpColor";
  } else {
    arg = "GlobalColor";
  }

  if (flags & FLAG_LIGHTING) {
    VTX_EMIT("float3 Out_LightingColor;");
    if (flags & FLAG_COLOR_EMISSIVE) {
      if (flags & FLAG_CAMERA_BASED_NORMALS) {
        VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz * 1.5;");
      } else {
        VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz + %s.xyz;", arg);
      }
    } else {
      VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz + MaterialEmissive.xyz;");
    }

    if (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3)) {
      if (flags & FLAG_LIGHT1) {
        if (GetMobileEffectSetting() == 3 && (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4)))
          VTX_EMIT("Out_LightingColor += (max(dot(DirLightDirection, WorldNormal), 0.0) + max(dot(DirBackLightDirection, WorldNormal), 0.0)) * DirLightDiffuseColor;");
        else
          VTX_EMIT("Out_LightingColor += max(dot(DirLightDirection, WorldNormal), 0.0) * DirLightDiffuseColor;");
      }
      if (flags & FLAG_LIGHT2)
        VTX_EMIT("Out_LightingColor += max(dot(DirLight2Direction, WorldNormal), 0.0) * DirLight2DiffuseColor;");
      if (flags & FLAG_LIGHT3)
        VTX_EMIT("Out_LightingColor += max(dot(DirLight3Direction, WorldNormal), 0.0) * DirLight3DiffuseColor;");
    }

    if (flags & (FLAG_COLOR | FLAG_LIGHTING)) {
      if (flags & FLAG_COLOR) {
        VTX_EMIT("Out_Color = float4((Out_LightingColor.xyz + %s.xyz * 1.5) * MaterialDiffuse.xyz, (MaterialAmbient.w) * %s.w);", arg, arg);
      } else {
        VTX_EMIT("Out_Color = float4(Out_LightingColor * MaterialDiffuse.xyz, MaterialAmbient.w * %s.w);", arg);
      }
      VTX_EMIT("Out_Color = clamp(Out_Color, 0.0, 1.0);");
    }
  } else {
    if (flags & (FLAG_COLOR | FLAG_LIGHTING))
      VTX_EMIT("Out_Color = %s;", arg);
  }

  if (!disable_amp && (flags & FLAG_LIGHT1)) {
    if (flags & (FLAG_REFL_OUT | FLAG_TEX1)) {
      VTX_EMIT("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", is_mali_chip ? 9.0f : 10.0f);
      VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");
    } else if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
      VTX_EMIT("float3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
      VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
      VTX_EMIT("float specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", is_mali_chip ? 5.0f : 4.0f);
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

void BuildPixelSource(int flags) {
  char tmp[512];

  PXL_EMIT("float4 main(");

  if (flags & FLAG_TEX0)
    PXL_EMIT("half2 Out_Tex0 : TEXCOORD0,");

  if (flags & (FLAG_REFL_OUT | FLAG_TEX1)) {
    if (flags & FLAG_TEX1)
      PXL_EMIT("half2 Out_Tex1 : TEXCOORD1,");
    else
      PXL_EMIT("half3 Out_Refl : TEXCOORD1,");
  }

  if (flags & FLAG_FOG)
    PXL_EMIT("half Out_FogAmt : FOG,");

  if (flags & (FLAG_COLOR | FLAG_LIGHTING))
    PXL_EMIT("half4 Out_Color : COLOR0,");

  if ((flags & FLAG_LIGHT1) && (flags & (FLAG_TEX1 | FLAG_BONE3 | FLAG_BONE4 | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_REFL_OUT)))
    PXL_EMIT("half3 Out_Spec : COLOR1,");

  if (flags & FLAG_WATER) {
    PXL_EMIT("half2 Out_WaterDetail : TEXCOORD2,");
    PXL_EMIT("half2 Out_WaterDetail2 : TEXCOORD3,");
    PXL_EMIT("half Out_WaterAlphaBlend : TEXCOORD4,");
  }

  if (flags & FLAG_TEX0)
    PXL_EMIT("uniform sampler2D Diffuse,");

  if (flags & (FLAG_REFL_OUT | FLAG_TEX1)) {
    PXL_EMIT("uniform sampler2D EnvMap,");
    PXL_EMIT("uniform half EnvMapCoefficient,");
  } else if (flags & FLAG_ENVMAP) {
    PXL_EMIT("uniform sampler2D EnvMap,");
    PXL_EMIT("uniform float DetailTiling,");
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
      PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, float4(Out_Tex0, 0.0, -1.5));");
    else if (!byte_5D6B42)
      PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, float4(Out_Tex0, 0.0, -0.5));");
    else
      PXL_EMIT("half4 diffuseColor = tex2D(Diffuse, Out_Tex0);");

    if (flags & FLAG_ALPHA_MODULATE)
      PXL_EMIT("fcolor = float4(diffuseColor.xyz, diffuseColor.w * AlphaModulate);");
    else
      PXL_EMIT("fcolor = diffuseColor;");

    if (!(flags & (FLAG_COLOR | FLAG_LIGHTING))) {
      if (flags & FLAG_WATER)
        PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
    } else if (!(flags & FLAG_ENVMAP)) {
      PXL_EMIT("fcolor *= Out_Color;");
      if (flags & FLAG_WATER)
        PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
    } else {
      if (flags & FLAG_WATER) {
        PXL_EMIT("float waterDetail = tex2Dbias(EnvMap, float4(Out_WaterDetail, 0.0, -1.0)).x + tex2Dbias(EnvMap, float4(Out_WaterDetail2, 0.0, -1.0)).x;");
        PXL_EMIT("fcolor *= float4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
        PXL_EMIT("fcolor.a += Out_WaterAlphaBlend;");
      } else {
        PXL_EMIT("fcolor *= float4(Out_Color.xyz * tex2Dbias(EnvMap, float4(Out_Tex0.xy * DetailTiling, 0.0, -0.5)).xyz * 2.0, Out_Color.w);");
      }
    }
  } else {
    if (flags & (FLAG_COLOR | FLAG_LIGHTING))
      PXL_EMIT("fcolor = Out_Color;");
    else
      PXL_EMIT("fcolor = 0.0;");

    if (flags & FLAG_ALPHA_MODULATE)
      PXL_EMIT("fcolor.w *= AlphaModulate;");
  }

  if (flags & FLAG_TEX1)
    PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, tex2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");

  if (flags & FLAG_REFL_OUT) {
    PXL_EMIT("float2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);");
    PXL_EMIT("ReflPos = (ReflPos * float2(0.5,0.5)) + float2(0.5,0.5);");
    PXL_EMIT("half4 ReflTexture = tex2D(EnvMap, ReflPos);");
    PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz,ReflTexture.xyz, EnvMapCoefficient);");
    PXL_EMIT("fcolor.w += ReflTexture.b * 0.125;");
  }

  if (!disable_amp) {
    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_TEX1 | FLAG_BONE3 | FLAG_BONE4 | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_REFL_OUT)))
      PXL_EMIT("fcolor.xyz += Out_Spec;");
    if (flags & FLAG_FOG)
      PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, FogColor, Out_FogAmt);");
  }

  if (flags & FLAG_GAMMA)
    PXL_EMIT("fcolor.xyz += fcolor.xyz * 0.5;");

  PXL_EMIT("float4 gl_FragColor = fcolor;");

  if (flags & FLAG_ALPHA_TEST) {
    PXL_EMIT("/*ATBEGIN*/");
    if ((OS_SystemChip() == 8) && (flags & FLAG_TEX0)) {
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

  PXL_EMIT("return gl_FragColor;");
  PXL_EMIT("}");
}

void (* _Z16BuildPixelSourcej)(int flags);
void (* _Z17BuildVertexSourcej)(int flags);
void (* _Z16OutputShaderCodePKc)(char *code);

char **pxlbuf_orig, **vtxbuf_orig;

int counter = 0;

int debugPrintf(char *text, ...);

void OutputShaderCode(const char *a1)
{
  const char *v1; // r6
  int v2; // r0
  signed int v3; // r1
  int v4; // r11
  const char *v5; // r1
  int v6; // r4
  signed int v7; // r0
  const char *v8; // r4
  const char *i; // r5
  signed int v10; // r1
  int v11; // zf
  static char v13[8192]; // [sp+0h] [bp-220h]

  v1 = a1;
  v2 = 0;
LABEL_9:
  v8 = v1;
  for ( i = v1; ; ++i )
  {
    v10 = *(unsigned char *)i;
    if ( v10 <= 'z' )
      break;
    v11 = v10 == '{';
    if ( v10 != '{' )
      v11 = v10 == '}';
    if ( v11 )
    {
LABEL_2:
      v11 = v10 == '}';
      v3 = 0;
      if ( !v11 )
        v3 = 1;
      v4 = v2 & v3;
      v11 = (v2 & v3) == 0;
      v5 = "\n";
      if ( !v11 )
        v5 = "\n    ";
      strcpy(v13, v5);
      v1 = i + 1;
      strncat(v13, v8, i + 1 - v8);
      v6 = *(unsigned char *)i;
      debugPrintf(v13);
      v7 = 0;
      if ( v6 == '{' )
        v7 = 1;
      v2 = v7 | v4;
      goto LABEL_9;
    }
LABEL_10:
    ;
  }
  if ( *i )
  {
    if ( v10 == ';' )
      goto LABEL_2;
    goto LABEL_10;
  }
}

int BuildSource(int flags, char **pxlsrc, char **vtxsrc) {
  pxlbuf[0] = '\0';
  vtxbuf[0] = '\0';

  *pxlbuf_orig = '\0';
  *vtxbuf_orig = '\0';

  BuildPixelSource(flags);
  BuildVertexSource(flags);
  _Z16BuildPixelSourcej(flags);
  _Z17BuildVertexSourcej(flags);

  // debugPrintf("======== INFO: COUNT: %d, FLAGS: 0x%08x ========\n", counter, flags);

  // debugPrintf("======== ORIG PIXEL SOURCE ========\n");
  // OutputShaderCode(pxlbuf_orig);
  // debugPrintf("\n");

  // debugPrintf("======== VITA PIXEL SOURCE ========\n");
  // OutputShaderCode(pxlbuf);
  // debugPrintf("\n");

  // debugPrintf("======== ORIG VERTEX SOURCE ========\n");
  // OutputShaderCode(vtxbuf_orig);
  // debugPrintf("\n");

  // debugPrintf("======== VITA VERTEX SOURCE ========\n");
  // OutputShaderCode(vtxbuf);
  // debugPrintf("\n");

  // counter++;

  *pxlsrc = pxlbuf;
  *vtxsrc = vtxbuf;

  return 1;
}

// precision mediump float;
// uniform sampler2D Diffuse;
// varying mediump vec2 Out_Tex0;
// uniform mediump vec4 RedGrade;
// void main(){
  // vec4 color = texture2D(Diffuse, Out_Tex0);
  // gl_FragColor = vec4(0, 0, 0, (1.0 - color.x) * RedGrade.a);
// }
static char *shadowResolvePShader = R"(
float4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade
){
  float4 color = tex2D(Diffuse, Out_Tex0);
  float4 gl_FragColor = half4(0, 0, 0, (1.0 - color.x) * RedGrade.a);
  return gl_FragColor;
}
)";

// precision mediump float;
// uniform sampler2D Diffuse;
// varying mediump vec2 Out_Tex0;
// varying mediump float Out_Z;
// uniform mediump vec4 RedGrade;
// uniform mediump vec4 GreenGrade;
// uniform mediump vec4 BlueGrade;
// void main(){
  // vec4 color = texture2D(Diffuse, Out_Tex0) * 0.25;
  // mediump vec2 dist = vec2(0.001, 0.001) * Out_Z;
  // color += texture2D(Diffuse, Out_Tex0 + dist) * 0.175;
  // color += texture2D(Diffuse, Out_Tex0 - dist) * 0.175;
  // color += texture2D(Diffuse, Out_Tex0 + vec2(dist.x, -dist.y)) * 0.2;
  // color += texture2D(Diffuse, Out_Tex0 + vec2(-dist.x, dist.y)) * 0.2;
  // gl_FragColor.x = dot(color, RedGrade);
  // gl_FragColor.y = dot(color, GreenGrade);
  // gl_FragColor.z = dot(color, BlueGrade);
// }
static char *blurPShader = R"(
float4 main(
  half2 Out_Tex0 : TEXCOORD0,
  half Out_Z : TEXCOORD1,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade,
  uniform half4 GreenGrade,
  uniform half4 BlueGrade
){
  float4 color = tex2D(Diffuse, Out_Tex0) * 0.25;
  half2 dist = half2(0.001, 0.001) * Out_Z;
  color += tex2D(Diffuse, Out_Tex0 + dist) * 0.175;
  color += tex2D(Diffuse, Out_Tex0 - dist) * 0.175;
  color += tex2D(Diffuse, Out_Tex0 + half2(dist.x, -dist.y)) * 0.2;
  color += tex2D(Diffuse, Out_Tex0 + half2(-dist.x, dist.y)) * 0.2;
  float4 gl_FragColor;
  gl_FragColor.x = dot(color, RedGrade);
  gl_FragColor.y = dot(color, GreenGrade);
  gl_FragColor.z = dot(color, BlueGrade);
  return gl_FragColor;
}
)";

// precision mediump float;
// uniform sampler2D Diffuse;
// varying mediump vec2 Out_Tex0;
// uniform mediump vec4 RedGrade;
// uniform mediump vec4 GreenGrade;
// uniform mediump vec4 BlueGrade;
// void main(){
  // vec4 color = texture2D(Diffuse, Out_Tex0);
  // gl_FragColor.x = dot(color, RedGrade);
  // gl_FragColor.y = dot(color, GreenGrade);
  // gl_FragColor.z = dot(color, BlueGrade);
// }

static char *gradingPShader = R"(
float4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade,
  uniform half4 GreenGrade,
  uniform half4 BlueGrade
){
  half4 color = tex2D(Diffuse, Out_Tex0);
  float4 gl_FragColor;
  gl_FragColor.x = dot(color, RedGrade);
  gl_FragColor.y = dot(color, GreenGrade);
  gl_FragColor.z = dot(color, BlueGrade);
  return gl_FragColor;
}
)";

// precision mediump float;
// uniform sampler2D Diffuse;
// varying mediump vec2 Out_Tex0;
// uniform mediump vec3 ContrastMult;
// uniform mediump vec3 ContrastAdd;
// void main(){
  // gl_FragColor = texture2D(Diffuse, Out_Tex0);
  // gl_FragColor.xyz = gl_FragColor.xyz * ContrastMult + ContrastAdd;
// }
static char *contrastPShader = R"(
float4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half3 ContrastMult,
  uniform half3 ContrastAdd
){
  float4 gl_FragColor = tex2D(Diffuse, Out_Tex0);
  gl_FragColor.xyz = gl_FragColor.xyz * ContrastMult + ContrastAdd;
  return gl_FragColor;
}
)";

// precision highp float;
// attribute vec3 Position;
// attribute vec2 TexCoord0;
// varying mediump vec2 Out_Tex0;
// varying mediump float Out_Z;
// void main() {
  // gl_Position = vec4(Position.xy, 0.0, 1.0);
  // Out_Z = Position.z;
  // Out_Tex0 = TexCoord0;
// }
static char *contrastVShader = R"(
void main(
  float3 Position,
  float2 TexCoord0,
  half2 out Out_Tex0 : TEXCOORD0,
  half out Out_Z : TEXCOORD1,
  float4 out gl_Position : POSITION
) {
  gl_Position = float4(Position.xy, 0.0, 1.0);
  Out_Z = Position.z;
  Out_Tex0 = TexCoord0;
}
)";

extern void *text_base;

void opengl_patch() {
  pxlbuf_orig = (char **)(text_base + 0x005D6B74);
  vtxbuf_orig = (char **)(text_base + 0x005D8B78);

  _Z16BuildPixelSourcej = (void *)find_addr_by_symbol("_Z16BuildPixelSourcej");
  _Z17BuildVertexSourcej = (void *)find_addr_by_symbol("_Z17BuildVertexSourcej");
  _Z16OutputShaderCodePKc = (void *)find_addr_by_symbol("_Z16OutputShaderCodePKc");

  *(char **)find_addr_by_symbol("shadowResolvePShader") = shadowResolvePShader;
  *(char **)find_addr_by_symbol("blurPShader") = blurPShader;
  *(char **)find_addr_by_symbol("gradingPShader") = gradingPShader;
  *(char **)find_addr_by_symbol("contrastPShader") = contrastPShader;
  *(char **)find_addr_by_symbol("contrastVShader") = contrastVShader;

  RQMaxBones = (int *)find_addr_by_symbol("RQMaxBones");
  hook_thumb(find_addr_by_symbol("_ZN8RQShader11BuildSourceEjPPKcS2_"), (uintptr_t)BuildSource);
}
