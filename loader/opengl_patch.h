#ifndef __OPENGL_PATCH_H__
#define __OPENGL_PATCH_H__

#define FLAG_ALPHA_TEST           0x01
#define FLAG_LIGHTING             0x02
#define FLAG_ALPHA_MODULATE       0x04
#define FLAG_COLOR_EMISSIVE       0x08
#define FLAG_COLOR                0x10
#define FLAG_TEX0                 0x20
#define FLAG_ENVMAP               0x40          // normal envmap
#define FLAG_BONE3                0x80
#define FLAG_BONE4                0x100
#define FLAG_CAMERA_BASED_NORMALS 0x200
#define FLAG_FOG                  0x400
#define FLAG_TEXBIAS              0x800
#define FLAG_BACKLIGHT            0x1000
#define FLAG_LIGHT1               0x2000
#define FLAG_LIGHT2               0x4000
#define FLAG_LIGHT3               0x8000
#define FLAG_DETAILMAP            0x10000
#define FLAG_COMPRESSED_TEXCOORD  0x20000
#define FLAG_PROJECT_TEXCOORD     0x40000
#define FLAG_WATER                0x80000
#define FLAG_COLOR2               0x100000
#define FLAG_SPHERE_XFORM         0x800000      // this renders the scene as a sphere map for vehicle reflections
#define FLAG_SPHERE_ENVMAP        0x1000000     // spherical real-time envmap
#define FLAG_TEXMATRIX            0x2000000
#define FLAG_GAMMA                0x4000000

#define PXL_EMIT(...)                                          \
  do {                                                         \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__);                   \
    pxlcnt += snprintf(pxlbuf + pxlcnt, sizeof(pxlbuf), tmp);  \
    pxlcnt += snprintf(pxlbuf + pxlcnt, sizeof(pxlbuf), "\n"); \
  } while (0)


#define VTX_EMIT(...)                                          \
  do {                                                         \
    snprintf(tmp, sizeof(tmp), __VA_ARGS__);                   \
    vtxcnt += snprintf(vtxbuf + vtxcnt, sizeof(vtxbuf), tmp);  \
    vtxcnt += snprintf(vtxbuf + vtxcnt, sizeof(vtxbuf), "\n"); \
  } while (0)

typedef struct {
  // Checks for GL_OES_depth24
  char has24BitDepthCap;                   // 0x00
  // Checks for GL_OES_packed_depth_stencil
  char hasPackedDepthStencilCap;           // 0x01
  // Checks for GL_NV_depth_nonlinear
  char hasDepthNonLinearCap;               // 0x02
  // Checks for GL_EXT_texture_compression_dxt1 or GL_EXT_texture_compression_s3tc
  char hasTextureCompressionDXT1OrS3TCCap; // 0x03
  // Checks for GL_AMD_compressed_ATC_texture
  char hasTextureCompressionATCCap;        // 0x04
  // Checks for GL_IMG_texture_compression_pvrtc
  char hasTextureCompressionPVRTCCap;      // 0x05
  // Checks for GL_OES_rgb8_rgba8
  char has32BitRenderTargetCap;            // 0x06
  // Checks for GL_EXT_texture_filter_anisotropic
  char hasAnisotropicFilteringCap;         // 0x07
  // Set when OS_SystemChip() <= 1
  char unk_08;                             // 0x08
  // Always set to 0
  char unk_09;                             // 0x09
  // Checks for GL_QCOM_binning_control
  char hasBinningControlCap;               // 0x0A
  // Checks for GL_QCOM_alpha_test
  char hasAlphaTestCap;                    // 0x0B
  // Checks for Adreno (TM) 320 or GL_AMD_compressed_ATC_texture
  char isAdreno;                           // 0x0C
  // Set when there is no compression support
  char isMaliChip;                         // 0x0D
  // Checks for 225 or 540
  char isSlowGPU;                          // 0x0E
  char unk_0f;                             // 0x0F
} RQCapabilities;

extern RQCapabilities *RQCaps;
extern int *RQMaxBones;

extern char pxlbuf[8192];
extern char vtxbuf[8192];

extern int pxlcnt;
extern int vtxcnt;

extern int (* GetMobileEffectSetting)();

void patch_opengl(void);

#endif
