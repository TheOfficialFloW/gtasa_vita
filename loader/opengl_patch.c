/* opengl_patch.c -- reverse engineered shader generator translated for cg
 *
 * Copyright (C) 2021 Andy Nguyen
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <string.h>

#include "main.h"
#include "config.h"
#include "so_util.h"
#include "opengl_patch.h"
#include "gfx_patch.h"

RQCapabilities *RQCaps;
int *RQMaxBones;

char pxlbuf[8192];
char vtxbuf[8192];

int pxlcnt = 0;
int vtxcnt = 0;

int (* GetMobileEffectSetting)();

void BuildVertexSource(int flags) {
  char tmp[512];
  char *vertexColor, *tex;

  int ped_spec = config.disable_ped_spec ? 0 : (FLAG_BONE3 | FLAG_BONE4);

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
      VTX_EMIT("uniform half3 DirLightDirection,");
      if (GetMobileEffectSetting() == 3) {
        if (flags & (FLAG_BACKLIGHT | FLAG_BONE3 | FLAG_BONE4))
          VTX_EMIT("uniform half3 DirBackLightDirection,");
      }
    }
    if (flags & FLAG_LIGHT2) {
      VTX_EMIT("uniform half3 DirLight2DiffuseColor,");
      VTX_EMIT("uniform half3 DirLight2Direction,");
    }
    if (flags & FLAG_LIGHT3) {
      VTX_EMIT("uniform half3 DirLight3DiffuseColor,");
      VTX_EMIT("uniform half3 DirLight3Direction,");
    }
  }

  if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
    if (config.enable_bones_optimization)
      VTX_EMIT("uniform float4x4 Bones[%d],", 96);
    else
      VTX_EMIT("uniform float4 Bones[%d],", *RQMaxBones * 3);
  }

  if (flags & FLAG_TEXMATRIX)
    VTX_EMIT("uniform float3x3 NormalMatrix,");

  if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP))
    VTX_EMIT("uniform half EnvMapCoefficient,");

  if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | FLAG_CAMERA_BASED_NORMALS | FLAG_FOG | FLAG_WATER | FLAG_SPHERE_XFORM | ped_spec))
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
    VTX_EMIT("half3 out Out_WaterDetail2 : TEXCOORD3,");
  }

  if (flags & (FLAG_COLOR | FLAG_LIGHTING))
    VTX_EMIT("half4 out Out_Color : COLOR0,");

  if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
    VTX_EMIT("half3 out Out_Spec : COLOR1,");

  if (flags & FLAG_FOG)
    VTX_EMIT("half out Out_FogAmt : FOG,");

  VTX_EMIT("float4 out gl_Position : POSITION,");

  if (vtxbuf[strlen(vtxbuf)-2] == ',')
    vtxbuf[strlen(vtxbuf)-2] = ' ';

  VTX_EMIT(") {");

  if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
    VTX_EMIT("int4 BlendIndexArray = int4(BoneIndices);");
    VTX_EMIT("float4x4 BoneToLocal;");
    if (config.enable_bones_optimization) {
      VTX_EMIT("BoneToLocal = Bones[BlendIndexArray.x] * BoneWeight.x;");
      VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.y] * BoneWeight.y;");
      VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.z] * BoneWeight.z;");
      if (flags & FLAG_BONE4)
        VTX_EMIT("BoneToLocal += Bones[BlendIndexArray.w] * BoneWeight.w;");
      VTX_EMIT("BoneToLocal[0][3] = 0.0;");
      VTX_EMIT("BoneToLocal[1][3] = 0.0;");
      VTX_EMIT("BoneToLocal[2][3] = 0.0;");
      VTX_EMIT("BoneToLocal[3][3] = 1.0;");
    } else {
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
    }
    if (config.enable_bones_optimization)
      VTX_EMIT("float4 BoneVertex = mul(float4(Position, 1.0), BoneToLocal);");
    else
      VTX_EMIT("float4 BoneVertex = mul(BoneToLocal, float4(Position, 1.0));");
    VTX_EMIT("float4 WorldPos = mul(BoneVertex, ObjMatrix);");
  } else {
    VTX_EMIT("float4 WorldPos = mul(float4(Position, 1.0), ObjMatrix);");
  }

  if (flags & FLAG_SPHERE_XFORM) {
    VTX_EMIT("float3 ReflVector = WorldPos.xyz - CameraPosition.xyz;");
    VTX_EMIT("float3 ReflPos = normalize(ReflVector);");
    VTX_EMIT("ReflPos.xy = normalize(ReflPos.xy) * (ReflPos.z * 0.5 + 0.5);");
    VTX_EMIT("gl_Position = float4(ReflPos.xy, length(ReflVector) * 0.002, 1.0);");
  } else {
    if (config.enable_mvp_optimization) {
      if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
        VTX_EMIT("gl_Position = mul(BoneVertex, ProjMatrix);");
      } else {
        VTX_EMIT("gl_Position = mul(float4(Position, 1.0), ProjMatrix);");
      }
    } else {
      VTX_EMIT("float4 ViewPos = mul(WorldPos, ViewMatrix);");
      VTX_EMIT("gl_Position = mul(ViewPos, ProjMatrix);");
    }
  }

  if (flags & FLAG_LIGHTING) {
    if (((flags & (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) == (FLAG_CAMERA_BASED_NORMALS | FLAG_ALPHA_TEST)) && (flags & (FLAG_LIGHT1 | FLAG_LIGHT2 | FLAG_LIGHT3))) {
      VTX_EMIT("float3 WorldNormal = normalize(float3(WorldPos.xy - CameraPosition.xy, 0.0001)) * 0.85;");
    } else {
      if (flags & (FLAG_BONE3 | FLAG_BONE4)) {
        if (config.enable_bones_optimization)
          VTX_EMIT("float3 WorldNormal = mul(mul(Normal, float3x3(BoneToLocal)), float3x3(ObjMatrix));");
        else
          VTX_EMIT("float3 WorldNormal = mul(mul(float3x3(BoneToLocal), Normal), float3x3(ObjMatrix));");
      } else {
        VTX_EMIT("float3 WorldNormal = mul(Normal, float3x3(ObjMatrix));");
      }
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
      VTX_EMIT("Out_Tex0 = mul(%s, float2x2(NormalMatrix));", tex);
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

  if (flags & FLAG_LIGHTING) {
    VTX_EMIT("half3 Out_LightingColor;");
    if (flags & FLAG_COLOR_EMISSIVE) {
      if (flags & FLAG_CAMERA_BASED_NORMALS)
        VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz * 1.5;");
      else
        VTX_EMIT("Out_LightingColor = AmbientLightColor * MaterialAmbient.xyz + %s.xyz;", vertexColor);
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
      if (flags & FLAG_COLOR)
        VTX_EMIT("Out_Color = half4((Out_LightingColor.xyz + %s.xyz * 1.5) * MaterialDiffuse.xyz, (MaterialAmbient.w) * %s.w);", vertexColor, vertexColor);
      else
        VTX_EMIT("Out_Color = half4(Out_LightingColor * MaterialDiffuse.xyz, MaterialAmbient.w * %s.w);", vertexColor);
      VTX_EMIT("Out_Color = clamp(Out_Color, 0.0, 1.0);");
    }
  } else {
    if (flags & (FLAG_COLOR | FLAG_LIGHTING))
      VTX_EMIT("Out_Color = %s;", vertexColor);
  }

  if (!RQCaps->unk_08 && (flags & FLAG_LIGHT1)) {
    if (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP)) {
      VTX_EMIT("half specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * EnvMapCoefficient * 2.0;", RQCaps->isMaliChip ? 9.0f : 10.0f);
      VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");
    } else if (flags & ped_spec) {
      VTX_EMIT("half3 reflVector = normalize(WorldPos.xyz - CameraPosition.xyz);");
      VTX_EMIT("reflVector = reflVector - 2.0 * dot(reflVector, WorldNormal) * WorldNormal;");
      VTX_EMIT("half specAmt = max(pow(dot(reflVector, DirLightDirection), %.1f), 0.0) * 0.125;", RQCaps->isMaliChip ? 5.0f : 4.0f);
      VTX_EMIT("Out_Spec = specAmt * DirLightDiffuseColor;");
    }
  }

  if (flags & FLAG_WATER) {
    VTX_EMIT("Out_WaterDetail = (Out_Tex0 * 4.0) + float2(WaterSpecs.x * -0.3, WaterSpecs.x * 0.21);");
    VTX_EMIT("Out_WaterDetail2.xy = (Out_Tex0 * -8.0) + float2(WaterSpecs.x * 0.12, WaterSpecs.x * -0.05);");
    VTX_EMIT("Out_WaterDetail2.z = distance(WorldPos.xy, CameraPosition.xy) * WaterSpecs.y;");
  }

  VTX_EMIT("}");
}

void BuildPixelSource(int flags) {
  char tmp[512];

  int ped_spec = config.disable_ped_spec ? 0 : (FLAG_BONE3 | FLAG_BONE4);

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
    PXL_EMIT("half3 Out_WaterDetail2 : TEXCOORD3,");
  }

  if (flags & (FLAG_COLOR | FLAG_LIGHTING))
    PXL_EMIT("half4 Out_Color : COLOR0,");

  if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
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

  if (pxlbuf[strlen(pxlbuf)-2] == ',')
    pxlbuf[strlen(pxlbuf)-2] = ' ';

  PXL_EMIT(") {");

  PXL_EMIT("half4 fcolor;");
  if (flags & FLAG_TEX0) {
    if (flags & FLAG_TEXBIAS)
      PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, half4(Out_Tex0, 0.0, -1.5));");
    else if (!config.disable_tex_bias && !RQCaps->isSlowGPU)
      PXL_EMIT("half4 diffuseColor = tex2Dbias(Diffuse, half4(Out_Tex0, 0.0, -0.5));");
    else
      PXL_EMIT("half4 diffuseColor = tex2D(Diffuse, Out_Tex0);");

    PXL_EMIT("fcolor = diffuseColor;");

    if (!(flags & (FLAG_COLOR | FLAG_LIGHTING))) {
      if (flags & FLAG_WATER)
        PXL_EMIT("fcolor.a += Out_WaterDetail2.z;");
    } else if (!(flags & FLAG_DETAILMAP)) {
      PXL_EMIT("fcolor *= Out_Color;");
      if (flags & FLAG_WATER)
        PXL_EMIT("fcolor.a += Out_WaterDetail2.z;");
    } else {
      if (flags & FLAG_WATER) {
        PXL_EMIT("half waterDetail = tex2Dbias(EnvMap, half4(Out_WaterDetail, 0.0, -1.0)).x + tex2Dbias(EnvMap, half4(Out_WaterDetail2.xy, 0.0, -1.0)).x;");
        PXL_EMIT("fcolor *= half4(Out_Color.xyz * waterDetail * 1.1, Out_Color.w);");
        PXL_EMIT("fcolor.a += Out_WaterDetail2.z;");
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

  if (flags & FLAG_ENVMAP)
    PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, tex2D(EnvMap, Out_Tex1).xyz, EnvMapCoefficient);");

  if (flags & FLAG_SPHERE_ENVMAP) {
    PXL_EMIT("half2 ReflPos = normalize(Out_Refl.xy) * (Out_Refl.z * 0.5 + 0.5);");
    PXL_EMIT("ReflPos = (ReflPos * 0.5) + 0.5;");
    PXL_EMIT("half4 ReflTexture = tex2D(EnvMap, ReflPos);");
    PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, ReflTexture.xyz, EnvMapCoefficient);");
    PXL_EMIT("fcolor.w += ReflTexture.b * 0.125;");
  }

  if (!RQCaps->unk_08) {
    if ((flags & FLAG_LIGHT1) && (flags & (FLAG_ENVMAP | FLAG_SPHERE_ENVMAP | ped_spec)))
      PXL_EMIT("fcolor.xyz += Out_Spec;");
    if (flags & FLAG_FOG)
      PXL_EMIT("fcolor.xyz = lerp(fcolor.xyz, FogColor, Out_FogAmt);");
  }

  if (flags & FLAG_GAMMA)
    PXL_EMIT("fcolor.xyz += fcolor.xyz * 0.5;");

  PXL_EMIT("half4 gl_FragColor = fcolor;");

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

  if (flags & FLAG_ALPHA_MODULATE)
    PXL_EMIT("gl_FragColor.a *= AlphaModulate;");

  PXL_EMIT("return gl_FragColor;");
  PXL_EMIT("}");
}

int RQShader__BuildSource(int flags, char **pxlsrc, char **vtxsrc) {
  pxlbuf[0] = '\0';
  vtxbuf[0] = '\0';
  pxlcnt = 0;
  vtxcnt = 0;

  if (config.skygfx_ps2_shading) {
    BuildPixelSource_SkyGfx(flags);
    BuildVertexSource_SkyGfx(flags);
  } else {
    BuildPixelSource(flags);
    BuildVertexSource(flags);
  }

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
half4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade
){
  half4 color = tex2D(Diffuse, Out_Tex0);
  return half4(0.0, 0.0, 0.0, (1.0 - color.x) * RedGrade.a);
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
half4 main(
  half2 Out_Tex0 : TEXCOORD0,
  half Out_Z : TEXCOORD1,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade,
  uniform half4 GreenGrade,
  uniform half4 BlueGrade
){
  half4 color = tex2D(Diffuse, Out_Tex0) * 0.25;
  half2 dist = half2(0.001, 0.001) * Out_Z;
  color += (tex2D(Diffuse, Out_Tex0 + dist) + tex2D(Diffuse, Out_Tex0 - dist)) * 0.175;
  color += (tex2D(Diffuse, Out_Tex0 + half2(dist.x, -dist.y)) + tex2D(Diffuse, Out_Tex0 + half2(-dist.x, dist.y))) * 0.2;
  return half4(dot(color, RedGrade), dot(color, GreenGrade), dot(color, BlueGrade), 0.0);
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
half4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half4 RedGrade,
  uniform half4 GreenGrade,
  uniform half4 BlueGrade
){
  half4 color = tex2D(Diffuse, Out_Tex0);
  return half4(dot(color, RedGrade), dot(color, GreenGrade), dot(color, BlueGrade), 0.0);
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
half4 main(
  half2 Out_Tex0 : TEXCOORD0,
  uniform sampler2D Diffuse,
  uniform half3 ContrastMult,
  uniform half3 ContrastAdd
){
  return half4(tex2D(Diffuse, Out_Tex0).xyz * ContrastMult + ContrastAdd, 0.0);
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

void patch_opengl(void) {
  *(char **)so_symbol(&gtasa_mod, "shadowResolvePShader") = shadowResolvePShader;
  *(char **)so_symbol(&gtasa_mod, "blurPShader") = blurPShader;
  *(char **)so_symbol(&gtasa_mod, "gradingPShader") = gradingPShader;
  *(char **)so_symbol(&gtasa_mod, "contrastPShader") = contrastPShader;
  *(char **)so_symbol(&gtasa_mod, "contrastVShader") = contrastVShader;

  GetMobileEffectSetting = (void *)so_symbol(&gtasa_mod, "_Z22GetMobileEffectSettingv");

  RQCaps = (RQCapabilities *)so_symbol(&gtasa_mod, "RQCaps");
  RQMaxBones = (int *)so_symbol(&gtasa_mod, "RQMaxBones");
  hook_addr(so_symbol(&gtasa_mod, "_ZN8RQShader11BuildSourceEjPPKcS2_"), (uintptr_t)RQShader__BuildSource);
}
