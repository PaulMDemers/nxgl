#include "nxgl_backend.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <math.h>
#include <pbkit/pbkit.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#define NXGL_BACKEND_SCREEN_W 640
#define NXGL_BACKEND_SCREEN_H 480
#define NXGL_BACKEND_MAX_VERTICES 32768
#define NXGL_BACKEND_MAX_BATCHES 256
#define NXGL_BACKEND_MAXRAM 0x03FFAFFF
#define MASK(mask, val) (((val) << (ffs(mask) - 1)) & (mask))
#define NXGL_BACKEND_TEXTURE_FORMAT_RGBA 0x0001122a
#define NXGL_BACKEND_TEXTURE_FORMAT_RGBA3D 0x0001123a
#define NXGL_BACKEND_TEXTURE_FORMAT_DXT1 0x00010c2a
#define NXGL_BACKEND_TEXTURE_FORMAT_DXT3 0x00010e2a
#define NXGL_BACKEND_TEXTURE_FORMAT_DXT5 0x00010f2a
#define NXGL_BACKEND_TEXTURE_WRAP_REPEAT 0x00010101

typedef float Matrix[16];
typedef float Vector[4];

enum {
    M11 = 0, M12, M13, M14,
    M21, M22, M23, M24,
    M31, M32, M33, M34,
    M41, M42, M43, M44
};

typedef struct NxglBackendGpuVertex {
    float pos[3];
    float color[4];
    float tex0[3];
    float tex1[3];
} __attribute__((packed)) NxglBackendGpuVertex;

typedef struct NxglBackendBatch {
    unsigned int start;
    unsigned int count;
    NxglBackendTexture *texture;
    NxglBackendTexture *texture1;
    NxglBackendTextureEnvMode texture_env_mode;
    NxglBackendColor texture_env_color;
    bool depth_test;
    bool depth_write;
    bool cull;
    uint32_t cull_face;
    uint32_t front_face;
    bool blend;
    uint32_t blend_sfactor;
    uint32_t blend_dfactor;
    bool scissor;
    int scissor_x;
    int scissor_y;
    int scissor_w;
    int scissor_h;
} NxglBackendBatch;

static NxglBackendGpuVertex *vertex_buffer;
static unsigned int vertex_count;
static NxglBackendBatch batches[NXGL_BACKEND_MAX_BATCHES];
static unsigned int batch_count;
static unsigned int submitted_vertex_count;
static int back_width;
static int back_height;
static NxglBackendTexture *bound_texture;
static NxglBackendTexture *bound_texture1;
static NxglBackendTextureEnvMode bound_texture_env_mode = NXGL_BACKEND_TEXENV_MODULATE;
static NxglBackendColor bound_texture_env_color = { 0.0f, 0.0f, 0.0f, 0.0f };
static bool depth_test_enabled = true;
static bool depth_write_enabled = true;
static bool cull_enabled;
static uint32_t cull_face_mode = NV097_SET_CULL_FACE_V_BACK;
static uint32_t front_face_mode = NV097_SET_FRONT_FACE_V_CCW;
static bool blend_enabled;
static uint32_t blend_sfactor = NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA;
static uint32_t blend_dfactor = NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA;
static bool scissor_enabled;
static int scissor_x;
static int scissor_y;
static int scissor_w = NXGL_BACKEND_SCREEN_W;
static int scissor_h = NXGL_BACKEND_SCREEN_H;

extern unsigned int pb_ColorFmt;
static bool scene_dirty;
static Vector camera_pos = { 0.0f, 0.0f, -2.5f, 1.0f };
static Vector camera_rot = { 0.0f, 0.0f, 0.0f, 1.0f };
static float projection_fov_y_degrees = 90.0f;
static float projection_near_z = 1.0f;
static float projection_far_z = 100.0f;

static void matrix_identity(Matrix out)
{
    memset(out, 0, sizeof(Matrix));
    out[M11] = 1.0f;
    out[M22] = 1.0f;
    out[M33] = 1.0f;
    out[M44] = 1.0f;
}

static void matrix_copy(Matrix out, const Matrix in)
{
    memcpy(out, in, sizeof(Matrix));
}

static void matrix_multiply(Matrix out, const Matrix a, const Matrix b)
{
    Matrix w;
    w[M11] = a[M11] * b[M11] + a[M12] * b[M21] + a[M13] * b[M31] + a[M14] * b[M41];
    w[M12] = a[M11] * b[M12] + a[M12] * b[M22] + a[M13] * b[M32] + a[M14] * b[M42];
    w[M13] = a[M11] * b[M13] + a[M12] * b[M23] + a[M13] * b[M33] + a[M14] * b[M43];
    w[M14] = a[M11] * b[M14] + a[M12] * b[M24] + a[M13] * b[M34] + a[M14] * b[M44];
    w[M21] = a[M21] * b[M11] + a[M22] * b[M21] + a[M23] * b[M31] + a[M24] * b[M41];
    w[M22] = a[M21] * b[M12] + a[M22] * b[M22] + a[M23] * b[M32] + a[M24] * b[M42];
    w[M23] = a[M21] * b[M13] + a[M22] * b[M23] + a[M23] * b[M33] + a[M24] * b[M43];
    w[M24] = a[M21] * b[M14] + a[M22] * b[M24] + a[M23] * b[M34] + a[M24] * b[M44];
    w[M31] = a[M31] * b[M11] + a[M32] * b[M21] + a[M33] * b[M31] + a[M34] * b[M41];
    w[M32] = a[M31] * b[M12] + a[M32] * b[M22] + a[M33] * b[M32] + a[M34] * b[M42];
    w[M33] = a[M31] * b[M13] + a[M32] * b[M23] + a[M33] * b[M33] + a[M34] * b[M43];
    w[M34] = a[M31] * b[M14] + a[M32] * b[M24] + a[M33] * b[M34] + a[M34] * b[M44];
    w[M41] = a[M41] * b[M11] + a[M42] * b[M21] + a[M43] * b[M31] + a[M44] * b[M41];
    w[M42] = a[M41] * b[M12] + a[M42] * b[M22] + a[M43] * b[M32] + a[M44] * b[M42];
    w[M43] = a[M41] * b[M13] + a[M42] * b[M23] + a[M43] * b[M33] + a[M44] * b[M43];
    w[M44] = a[M41] * b[M14] + a[M42] * b[M24] + a[M43] * b[M34] + a[M44] * b[M44];
    matrix_copy(out, w);
}

static void matrix_rotate(Matrix out, const Matrix in, Vector rot)
{
    Matrix work;
    matrix_identity(work);
    work[M11] = cosf(rot[2]);
    work[M12] = sinf(rot[2]);
    work[M21] = -sinf(rot[2]);
    work[M22] = cosf(rot[2]);
    matrix_multiply(out, in, work);

    matrix_identity(work);
    work[M11] = cosf(rot[1]);
    work[M13] = -sinf(rot[1]);
    work[M31] = sinf(rot[1]);
    work[M33] = cosf(rot[1]);
    matrix_multiply(out, out, work);

    matrix_identity(work);
    work[M22] = cosf(rot[0]);
    work[M23] = sinf(rot[0]);
    work[M32] = -sinf(rot[0]);
    work[M33] = cosf(rot[0]);
    matrix_multiply(out, out, work);
}

static void matrix_translate(Matrix out, const Matrix in, Vector trans)
{
    Matrix work;
    matrix_identity(work);
    work[M41] = trans[0];
    work[M42] = trans[1];
    work[M43] = trans[2];
    matrix_multiply(out, in, work);
}

static void matrix_viewport(Matrix out, float x, float y, float width, float height, float z_min, float z_max)
{
    memset(out, 0, sizeof(Matrix));
    out[M11] = width / 2.0f;
    out[M22] = height / -2.0f;
    out[M33] = z_max - z_min;
    out[M44] = 1.0f;
    out[M41] = x + width / 2.0f;
    out[M42] = y + height / 2.0f;
    out[M43] = z_min;
}

static void matrix_projection(Matrix out, float aspect, float near_z, float far_z)
{
    float y_scale = 1.0f / tanf(projection_fov_y_degrees * 3.14159265358979323846f / 360.0f);
    float x_scale = y_scale / aspect;

    matrix_identity(out);
    out[M11] = x_scale;
    out[M22] = y_scale;
    out[M31] = 0.0f;
    out[M32] = 0.0f;
    out[M33] = -far_z / (far_z - near_z);
    out[M34] = -1.0f;
    out[M43] = near_z * far_z / (far_z - near_z);
    out[M44] = 0.0f;
}

static void load_color_shader(void)
{
    uint32_t *p;
    uint32_t vs_program[] = {
        #include "../common3d/vs.inl"
    };

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    pb_end(p);

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    for (unsigned int i = 0; i < sizeof(vs_program) / 16; ++i) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i * 4], 4 * 4);
        p += 4;
        pb_end(p);
    }

    p = pb_begin();
    p = pb_push1(p, NV097_SET_SHADER_OTHER_STAGE_INPUT, 0);
    p = pb_push1(p, NV097_SET_SHADER_STAGE_PROGRAM,
                 MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE0, NV097_SET_SHADER_STAGE_PROGRAM_STAGE0_PROGRAM_NONE) |
                 MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE1, NV097_SET_SHADER_STAGE_PROGRAM_STAGE1_PROGRAM_NONE) |
                 MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE2, NV097_SET_SHADER_STAGE_PROGRAM_STAGE2_PROGRAM_NONE) |
                 MASK(NV097_SET_SHADER_STAGE_PROGRAM_STAGE3, NV097_SET_SHADER_STAGE_PROGRAM_STAGE3_PROGRAM_NONE));
    #include "../common3d/ps.inl"
    pb_end(p);
}

static uint8_t color_byte(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 1.0f) {
        return 255;
    }
    return (uint8_t)(value * 255.0f + 0.5f);
}

static uint32_t packed_color(NxglBackendColor color)
{
    uint8_t r = color_byte(color.r);
    uint8_t g = color_byte(color.g);
    uint8_t b = color_byte(color.b);
    uint8_t a = color_byte(color.a);
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void load_texture_shader(NxglBackendTextureEnvMode mode, NxglBackendColor env_color)
{
    uint32_t *p;
    uint32_t vs_program[] = {
        #include "nxgl_tex_vs.inl"
    };

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    pb_end(p);

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    for (unsigned int i = 0; i < sizeof(vs_program) / 16; ++i) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i * 4], 4 * 4);
        p += 4;
        pb_end(p);
    }

    p = pb_begin();
    if (mode == NXGL_BACKEND_TEXENV_REPLACE) {
        #include "nxgl_tex_replace_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_DECAL) {
        #include "nxgl_tex_decal_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_BLEND) {
        uint32_t factor = packed_color(env_color);
        p = pb_push1(p, NV097_SET_COMBINER_FACTOR0, factor);
        p = pb_push1(p, NV097_SET_COMBINER_FACTOR1, factor);
        #include "nxgl_tex_blend_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_ADD) {
        #include "nxgl_tex_add_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_SUBTRACT) {
        #include "nxgl_tex_subtract_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_ADD_SIGNED) {
        #include "nxgl_tex_add_signed_ps.inl"
    } else if (mode == NXGL_BACKEND_TEXENV_INTERPOLATE) {
        uint32_t factor = packed_color(env_color);
        p = pb_push1(p, NV097_SET_COMBINER_FACTOR0, factor);
        p = pb_push1(p, NV097_SET_COMBINER_FACTOR1, factor);
        #include "nxgl_tex_interpolate_ps.inl"
    } else {
        #include "nxgl_tex_modulate_ps.inl"
    }
    pb_end(p);
}

static void load_multitexture_shader(void)
{
    uint32_t *p;
    uint32_t vs_program[] = {
        #include "nxgl_tex2_vs.inl"
    };

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    pb_end(p);

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    for (unsigned int i = 0; i < sizeof(vs_program) / 16; ++i) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i * 4], 4 * 4);
        p += 4;
        pb_end(p);
    }

    p = pb_begin();
    #include "nxgl_tex2_modulate_ps.inl"
    pb_end(p);
}

static void load_cube_texture_shader(void)
{
    uint32_t *p;
    uint32_t vs_program[] = {
        #include "nxgl_tex_vs.inl"
    };

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    pb_end(p);

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    for (unsigned int i = 0; i < sizeof(vs_program) / 16; ++i) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i * 4], 4 * 4);
        p += 4;
        pb_end(p);
    }

    p = pb_begin();
    #include "nxgl_cube_ps.inl"
    pb_end(p);
}

static void load_texture3d_shader(void)
{
    uint32_t *p;
    uint32_t vs_program[] = {
        #include "nxgl_tex_vs.inl"
    };

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM) |
                 MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);
    pb_end(p);

    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    for (unsigned int i = 0; i < sizeof(vs_program) / 16; ++i) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i * 4], 4 * 4);
        p += 4;
        pb_end(p);
    }

    p = pb_begin();
    #include "nxgl_tex3d_ps.inl"
    pb_end(p);
}

static void setup_render_state(bool blend, uint32_t sfactor, uint32_t dfactor,
                               bool depth_test, bool depth_write, bool cull,
                               uint32_t cull_face, uint32_t front_face,
                               bool scissor, int sx, int sy, int sw, int sh)
{
    uint32_t *p = pb_begin();
    int x1 = scissor ? sx : 0;
    int y1 = scissor ? sy : 0;
    int x2 = scissor ? sx + sw : back_width;
    int y2 = scissor ? sy + sh : back_height;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 < x1) x2 = x1;
    if (y2 < y1) y2 = y1;
    if (x2 > back_width) x2 = back_width;
    if (y2 > back_height) y2 = back_height;
    p = pb_push1(p, NV097_SET_DEPTH_TEST_ENABLE, depth_test ? 1 : 0);
    p = pb_push1(p, NV097_SET_DEPTH_FUNC, NV097_SET_DEPTH_FUNC_V_LEQUAL);
    p = pb_push1(p, NV097_SET_DEPTH_MASK, depth_write ? 1 : 0);
    p = pb_push1(p, NV097_SET_CULL_FACE_ENABLE, cull ? 1 : 0);
    p = pb_push1(p, NV097_SET_CULL_FACE, cull_face);
    p = pb_push1(p, NV097_SET_FRONT_FACE, front_face);
    p = pb_push1(p, NV097_SET_BLEND_ENABLE, blend ? 1 : 0);
    p = pb_push1(p, NV097_SET_BLEND_FUNC_SFACTOR, sfactor);
    p = pb_push1(p, NV097_SET_BLEND_FUNC_DFACTOR, dfactor);
    p = pb_push1(p, NV097_SET_BLEND_EQUATION, NV097_SET_BLEND_EQUATION_V_FUNC_ADD);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_HORIZONTAL, ((uint32_t)(x2 - x1) << 16) | (uint32_t)x1);
    p = pb_push1(p, NV097_SET_SURFACE_CLIP_VERTICAL, ((uint32_t)(y2 - y1) << 16) | (uint32_t)y1);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_HORIZ(0), ((uint32_t)(x2 <= x1 ? x1 : x2 - 1) << 16) | (uint32_t)x1);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_VIEWPORT_CLIP_VERT(0), ((uint32_t)(y2 <= y1 ? y1 : y2 - 1) << 16) | (uint32_t)y1);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_SCISSOR_X2_X1, ((uint32_t)x2 << 16) | (uint32_t)x1);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_SCISSOR_Y2_Y1, ((uint32_t)y2 << 16) | (uint32_t)y1);
    pb_end(p);
}

static void setup_texture_stage(NxglBackendTexture *texture)
{
    uint32_t *p = pb_begin();
    uint32_t format = texture->format != 0 ? texture->format : NXGL_BACKEND_TEXTURE_FORMAT_RGBA;
    if (texture->cube_map) {
        format |= NV097_SET_TEXTURE_FORMAT_CUBEMAP_ENABLE;
    }
    p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(0), (uint32_t)texture->addr & 0x03ffffff, format);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_DEPTH_UNIT(0), texture->depth);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(0), texture->pitch << 16);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(0), (texture->width << 16) | texture->height);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(0), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0), 0x4003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(0), 0x04074000);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(1), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(2), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(3), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(1), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(2), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(3), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(1), 0x02022000);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(2), 0x02022000);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(3), 0x02022000);
    pb_end(p);
}

static void setup_texture_stage1(NxglBackendTexture *texture)
{
    uint32_t *p = pb_begin();
    uint32_t format = texture->format != 0 ? texture->format : NXGL_BACKEND_TEXTURE_FORMAT_RGBA;
    p = pb_push2(p, NV20_TCL_PRIMITIVE_3D_TX_OFFSET(1), (uint32_t)texture->addr & 0x03ffffff, format);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_DEPTH_UNIT(1), texture->depth);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_PITCH(1), texture->pitch << 16);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_NPOT_SIZE(1), (texture->width << 16) | texture->height);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(1), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(1), 0x4003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(1), 0x04074000);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(2), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(3), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(2), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_WRAP(3), NXGL_BACKEND_TEXTURE_WRAP_REPEAT);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(2), 0x02022000);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_FILTER(3), 0x02022000);
    pb_end(p);
}

static void disable_texture_stages(void)
{
    uint32_t *p = pb_begin();
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(0), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(1), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(2), 0x0003ffc0);
    p = pb_push1(p, NV20_TCL_PRIMITIVE_3D_TX_ENABLE(3), 0x0003ffc0);
    pb_end(p);
}

static void set_attrib_pointer(unsigned int index, unsigned int format, unsigned int size, unsigned int stride, const void *data)
{
    uint32_t *p = pb_begin();
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index * 4,
                 MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) |
                 MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |
                 MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index * 4, (uint32_t)data & 0x03ffffff);
    pb_end(p);
}

static void draw_arrays_range(unsigned int start, unsigned int count)
{
    const unsigned int max_vertices_per_draw = 255;

    while (count > 0) {
        unsigned int chunk = count > max_vertices_per_draw ? max_vertices_per_draw : count;
        if (chunk > 3 && chunk % 3 != 0) {
            chunk -= chunk % 3;
        }
        if (chunk == 0) {
            return;
        }

        uint32_t *p = pb_begin();
        p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_TRIANGLES);
        p = pb_push1(p, 0x40000000 | NV097_DRAW_ARRAYS,
                     MASK(NV097_DRAW_ARRAYS_COUNT, (chunk - 1)) |
                     MASK(NV097_DRAW_ARRAYS_START_INDEX, start));
        p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
        pb_end(p);

        start += chunk;
        count -= chunk;
    }
}

static void ensure_batch(void)
{
    NxglBackendBatch *last;

    if (batch_count > 0) {
        last = &batches[batch_count - 1];
        if (last->texture == bound_texture &&
            last->texture1 == bound_texture1 &&
            last->texture_env_mode == bound_texture_env_mode &&
            last->texture_env_color.r == bound_texture_env_color.r &&
            last->texture_env_color.g == bound_texture_env_color.g &&
            last->texture_env_color.b == bound_texture_env_color.b &&
            last->texture_env_color.a == bound_texture_env_color.a &&
            last->depth_test == depth_test_enabled &&
            last->depth_write == depth_write_enabled &&
            last->cull == cull_enabled &&
            last->cull_face == cull_face_mode &&
            last->front_face == front_face_mode &&
            last->blend == blend_enabled &&
            last->blend_sfactor == blend_sfactor &&
            last->blend_dfactor == blend_dfactor &&
            last->scissor == scissor_enabled &&
            last->scissor_x == scissor_x &&
            last->scissor_y == scissor_y &&
            last->scissor_w == scissor_w &&
            last->scissor_h == scissor_h) {
            return;
        }
    }

    if (batch_count >= NXGL_BACKEND_MAX_BATCHES) {
        return;
    }

    last = &batches[batch_count++];
    last->start = vertex_count;
    last->count = 0;
    last->texture = bound_texture;
    last->texture1 = bound_texture1;
    last->texture_env_mode = bound_texture_env_mode;
    last->texture_env_color = bound_texture_env_color;
    last->depth_test = depth_test_enabled;
    last->depth_write = depth_write_enabled;
    last->cull = cull_enabled;
    last->cull_face = cull_face_mode;
    last->front_face = front_face_mode;
    last->blend = blend_enabled;
    last->blend_sfactor = blend_sfactor;
    last->blend_dfactor = blend_dfactor;
    last->scissor = scissor_enabled;
    last->scissor_x = scissor_x;
    last->scissor_y = scissor_y;
    last->scissor_w = scissor_w;
    last->scissor_h = scissor_h;
}

static float nxgl_backend_texel_coord(float coord, uint16_t size)
{
    return coord * (float)size;
}

static void push_vertex(NxglBackendVertex src)
{
    if (vertex_count >= NXGL_BACKEND_MAX_VERTICES) {
        return;
    }

    scene_dirty = true;
    ensure_batch();
    NxglBackendGpuVertex *dst = &vertex_buffer[vertex_count++];
    dst->pos[0] = src.pos.x;
    dst->pos[1] = src.pos.y;
    dst->pos[2] = src.pos.z;
    dst->color[0] = src.color.r;
    dst->color[1] = src.color.g;
    dst->color[2] = src.color.b;
    dst->color[3] = src.color.a;
    if (bound_texture != NULL && !bound_texture->cube_map && bound_texture->format == NXGL_BACKEND_TEXTURE_FORMAT_RGBA) {
        dst->tex0[0] = nxgl_backend_texel_coord(src.u, bound_texture->width);
        dst->tex0[1] = nxgl_backend_texel_coord(src.v, bound_texture->height);
        dst->tex0[2] = nxgl_backend_texel_coord(src.r, bound_texture->depth);
    } else {
        dst->tex0[0] = src.u;
        dst->tex0[1] = src.v;
        dst->tex0[2] = src.r;
    }
    if (bound_texture1 != NULL && !bound_texture1->cube_map && bound_texture1->format == NXGL_BACKEND_TEXTURE_FORMAT_RGBA) {
        dst->tex1[0] = nxgl_backend_texel_coord(src.u1, bound_texture1->width);
        dst->tex1[1] = nxgl_backend_texel_coord(src.v1, bound_texture1->height);
        dst->tex1[2] = nxgl_backend_texel_coord(src.r1, bound_texture1->depth);
    } else {
        dst->tex1[0] = src.u1;
        dst->tex1[1] = src.v1;
        dst->tex1[2] = src.r1;
    }
    if (batch_count > 0) {
        batches[batch_count - 1].count++;
    }
}

int nxgl_backend_init(void)
{
    XVideoSetMode(NXGL_BACKEND_SCREEN_W, NXGL_BACKEND_SCREEN_H, 32, REFRESH_DEFAULT);
    int status = pb_init();
    if (status != 0) {
        debugPrint("pb_init Error %d\n", status);
        return status;
    }

    pb_show_front_screen();
    back_width = pb_back_buffer_width();
    back_height = pb_back_buffer_height();
    load_color_shader();

    vertex_buffer = MmAllocateContiguousMemoryEx(sizeof(NxglBackendGpuVertex) * NXGL_BACKEND_MAX_VERTICES, 0, NXGL_BACKEND_MAXRAM, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (vertex_buffer == NULL) {
        debugPrint("nxgl_backend vertex allocation failed\n");
        return 1;
    }

    bound_texture = NULL;
    bound_texture1 = NULL;
    bound_texture_env_mode = NXGL_BACKEND_TEXENV_MODULATE;
    bound_texture_env_color = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
    scissor_enabled = false;
    scissor_x = 0;
    scissor_y = 0;
    scissor_w = back_width;
    scissor_h = back_height;

    return 0;
}

void nxgl_backend_shutdown(void)
{
    if (vertex_buffer != NULL) {
        MmFreeContiguousMemory(vertex_buffer);
        vertex_buffer = NULL;
    }
    pb_show_debug_screen();
    pb_kill();
}

static bool nxgl_backend_clip_clear_rect(int *x, int *y, int *width, int *height)
{
    int x1 = *x;
    int y1 = *y;
    int x2 = *x + *width;
    int y2 = *y + *height;

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > back_width) x2 = back_width;
    if (y2 > back_height) y2 = back_height;
    if (x1 >= x2 || y1 >= y2) {
        return false;
    }
    *x = x1;
    *y = y1;
    *width = x2 - x1;
    *height = y2 - y1;
    return true;
}

static uint32_t nxgl_backend_convert_clear_color(uint32_t color)
{
    switch (pb_ColorFmt) {
    case NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_Z1R5G5B5:
    case NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5:
        return ((color >> 16) & 0x8000) | ((color >> 7) & 0x7C00) | ((color >> 5) & 0x03E0) | ((color >> 3) & 0x001F);
    case NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5:
        return ((color >> 8) & 0xF800) | ((color >> 5) & 0x07E0) | ((color >> 3) & 0x001F);
    case NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8:
    default:
        return color;
    }
}

void nxgl_backend_begin_frame(bool blend)
{
    vertex_count = 0;
    batch_count = 0;
    submitted_vertex_count = 0;
    scene_dirty = false;
    depth_test_enabled = true;
    depth_write_enabled = true;
    cull_enabled = false;
    cull_face_mode = NV097_SET_CULL_FACE_V_BACK;
    front_face_mode = NV097_SET_FRONT_FACE_V_CCW;
    blend_enabled = blend;
    blend_sfactor = NV097_SET_BLEND_FUNC_SFACTOR_V_SRC_ALPHA;
    blend_dfactor = NV097_SET_BLEND_FUNC_DFACTOR_V_ONE_MINUS_SRC_ALPHA;
    scissor_enabled = false;
    scissor_x = 0;
    scissor_y = 0;
    scissor_w = back_width;
    scissor_h = back_height;
    projection_fov_y_degrees = 90.0f;
    projection_near_z = 1.0f;
    projection_far_z = 100.0f;
    pb_wait_for_vbl();
    pb_reset();
    pb_target_back_buffer();
    pb_erase_text_screen();
    bound_texture = NULL;
    bound_texture1 = NULL;
    bound_texture_env_mode = NXGL_BACKEND_TEXENV_MODULATE;
    bound_texture_env_color = (NxglBackendColor){ 0.0f, 0.0f, 0.0f, 0.0f };
}

void nxgl_backend_clear_color(uint32_t clear_color, bool red, bool green, bool blue, bool alpha, int x, int y, int width, int height)
{
    uint32_t trigger = 0;
    uint32_t *p;

    if (!nxgl_backend_clip_clear_rect(&x, &y, &width, &height)) {
        return;
    }
    if (red) trigger |= NV097_CLEAR_SURFACE_R;
    if (green) trigger |= NV097_CLEAR_SURFACE_G;
    if (blue) trigger |= NV097_CLEAR_SURFACE_B;
    if (alpha) trigger |= NV097_CLEAR_SURFACE_A;
    if (trigger == 0u) {
        return;
    }

    p = pb_begin();
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_HORIZ, 2);
    *(p++) = ((x + width - 1) << 16) | x;
    *(p++) = ((y + height - 1) << 16) | y;
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 3);
    *(p++) = 0;
    *(p++) = nxgl_backend_convert_clear_color(clear_color);
    *(p++) = trigger;
    pb_end(p);
}

void nxgl_backend_clear_depth_stencil(bool depth, float depth_value, bool stencil, uint8_t stencil_value, int x, int y, int width, int height)
{
    uint32_t clear_depth;
    uint32_t trigger = 0;
    uint32_t *p;

    if (!depth && !stencil) {
        return;
    }
    if (!nxgl_backend_clip_clear_rect(&x, &y, &width, &height)) {
        return;
    }
    if (depth_value < 0.0f) depth_value = 0.0f;
    if (depth_value > 1.0f) depth_value = 1.0f;
    if (depth && (!stencil || stencil_value == 0u) && depth_value >= 0.99999f) {
        pb_erase_depth_stencil_buffer(x, y, width, height);
        return;
    }
    clear_depth = (uint32_t)(depth_value * 16777215.0f + 0.5f);
    if (depth) trigger |= NV097_CLEAR_SURFACE_Z;
    if (stencil) trigger |= NV097_CLEAR_SURFACE_STENCIL;

    p = pb_begin();
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_HORIZ, 2);
    *(p++) = ((uint32_t)(x + width - 1) << 16) | ((uint32_t)x & 0xffffu);
    *(p++) = ((uint32_t)(y + height - 1) << 16) | ((uint32_t)y & 0xffffu);
    pb_push(p++, NV20_TCL_PRIMITIVE_3D_CLEAR_VALUE_DEPTH, 3);
    *(p++) = ((clear_depth & 0x00ffffffu) << 8) | stencil_value;
    *(p++) = 0;
    *(p++) = trigger;
    pb_end(p);
}

void nxgl_backend_begin(uint32_t clear_color, bool blend)
{
    nxgl_backend_begin_frame(blend);
    pb_erase_depth_stencil_buffer(0, 0, back_width, back_height);
    nxgl_backend_clear_color(clear_color, true, true, true, true, 0, 0, back_width, back_height);
}

void nxgl_backend_set_depth(bool test, bool write)
{
    depth_test_enabled = test;
    depth_write_enabled = write;
}

void nxgl_backend_set_cull(bool enabled)
{
    cull_enabled = enabled;
}

void nxgl_backend_set_cull_mode(uint32_t face, uint32_t front_face)
{
    cull_face_mode = face;
    front_face_mode = front_face;
}

void nxgl_backend_set_blend(bool enabled)
{
    blend_enabled = enabled;
}

void nxgl_backend_set_blend_func(uint32_t sfactor, uint32_t dfactor)
{
    blend_sfactor = sfactor;
    blend_dfactor = dfactor;
}

void nxgl_backend_set_scissor(bool enabled, int x, int y, int width, int height)
{
    scissor_enabled = enabled;
    scissor_x = x;
    scissor_y = y;
    scissor_w = width;
    scissor_h = height;
}

void nxgl_backend_set_projection(float fov_y_degrees, float near_z, float far_z)
{
    if (fov_y_degrees > 1.0f && fov_y_degrees < 179.0f) {
        projection_fov_y_degrees = fov_y_degrees;
    }
    if (near_z > 0.0f && far_z > near_z) {
        projection_near_z = near_z;
        projection_far_z = far_z;
    }
}

void nxgl_backend_set_camera(float x, float y, float z, float rx, float ry, float rz)
{
    camera_pos[0] = x;
    camera_pos[1] = y;
    camera_pos[2] = z;
    camera_rot[0] = rx;
    camera_rot[1] = ry;
    camera_rot[2] = rz;
}

void nxgl_backend_push_triangle(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c)
{
    push_vertex(a);
    push_vertex(b);
    push_vertex(c);
}

void nxgl_backend_push_quad(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d)
{
    nxgl_backend_push_triangle(a, b, c);
    nxgl_backend_push_triangle(a, c, d);
}

int nxgl_backend_texture_create_rgba(NxglBackendTexture *texture, uint16_t width, uint16_t height, const uint8_t *rgba)
{
    if (texture == NULL || width == 0 || height == 0 || rgba == NULL) {
        return 1;
    }

    uint16_t native_width = width < 4 ? 4 : width;
    uint16_t native_height = height < 4 ? 4 : height;

    texture->width = native_width;
    texture->height = native_height;
    texture->depth = 1;
    texture->pitch = native_width * 4;
    texture->format = NXGL_BACKEND_TEXTURE_FORMAT_RGBA;
    texture->cube_map = false;
    texture->volume = false;
    texture->addr = MmAllocateContiguousMemoryEx(texture->pitch * texture->height, 0, NXGL_BACKEND_MAXRAM, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (texture->addr == NULL) {
        return 1;
    }

    uint32_t *pixels = (uint32_t *)texture->addr;
    for (uint32_t y = 0; y < native_height; ++y) {
        uint32_t src_y = y < height ? y : (uint32_t)height - 1u;
        for (uint32_t x = 0; x < native_width; ++x) {
            uint32_t src_x = x < width ? x : (uint32_t)width - 1u;
            uint32_t src = (src_y * (uint32_t)width + src_x) * 4u;
            uint8_t r = rgba[src + 0];
            uint8_t g = rgba[src + 1];
            uint8_t b = rgba[src + 2];
            uint8_t a = rgba[src + 3];
            pixels[y * (uint32_t)native_width + x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }

    return 0;
}

int nxgl_backend_texture_create_rgba3d(NxglBackendTexture *texture, uint16_t width, uint16_t height, uint16_t depth, const uint8_t *rgba)
{
    size_t size;
    uint32_t *pixels;

    if (texture == NULL || width == 0 || height == 0 || depth == 0 || rgba == NULL) {
        return -1;
    }
    texture->width = width;
    texture->height = height;
    texture->depth = depth;
    texture->pitch = width * 4;
    texture->format = NXGL_BACKEND_TEXTURE_FORMAT_RGBA3D;
    texture->cube_map = false;
    texture->volume = true;

    size = (size_t)texture->pitch * (size_t)texture->height * (size_t)texture->depth;
    texture->addr = MmAllocateContiguousMemoryEx(size, 0, NXGL_BACKEND_MAXRAM, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (texture->addr == NULL) {
        return -1;
    }
    pixels = (uint32_t *)texture->addr;
    for (uint32_t i = 0; i < (uint32_t)width * (uint32_t)height * (uint32_t)depth; ++i) {
        uint8_t r = rgba[i * 4 + 0];
        uint8_t g = rgba[i * 4 + 1];
        uint8_t b = rgba[i * 4 + 2];
        uint8_t a = rgba[i * 4 + 3];
        pixels[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
    return 0;
}

int nxgl_backend_texture_create_cube_rgba(NxglBackendTexture *texture, uint16_t size, const uint8_t *faces[6])
{
    if (texture == NULL || size == 0 || faces == NULL) {
        return 1;
    }
    for (int face = 0; face < 6; ++face) {
        if (faces[face] == NULL) {
            return 1;
        }
    }

    texture->width = size;
    texture->height = size;
    texture->depth = 6;
    texture->pitch = size * 4;
    texture->format = NXGL_BACKEND_TEXTURE_FORMAT_RGBA;
    texture->cube_map = true;
    texture->volume = false;
    texture->addr = MmAllocateContiguousMemoryEx((size_t)texture->pitch * (size_t)texture->height * 6u,
                                                 0,
                                                 NXGL_BACKEND_MAXRAM,
                                                 0,
                                                 PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (texture->addr == NULL) {
        return 1;
    }

    uint32_t *pixels = (uint32_t *)texture->addr;
    for (int face = 0; face < 6; ++face) {
        const uint8_t *rgba = faces[face];
        uint32_t *dst = pixels + (size_t)face * (size_t)size * (size_t)size;
        for (uint32_t i = 0; i < (uint32_t)size * (uint32_t)size; ++i) {
            uint8_t r = rgba[i * 4 + 0];
            uint8_t g = rgba[i * 4 + 1];
            uint8_t b = rgba[i * 4 + 2];
            uint8_t a = rgba[i * 4 + 3];
            dst[i] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        }
    }

    return 0;
}

int nxgl_backend_texture_create_compressed(NxglBackendTexture *texture, uint16_t width, uint16_t height, NxglBackendCompressedTextureFormat format, const uint8_t *data, uint32_t data_size)
{
    uint32_t native_format;
    uint16_t pitch;
    int block_size;

    if (texture == NULL || width == 0 || height == 0 || data == NULL || data_size == 0) {
        return 1;
    }

    if (format == NXGL_BACKEND_COMPRESSED_DXT1) {
        native_format = NXGL_BACKEND_TEXTURE_FORMAT_DXT1;
        block_size = 8;
    } else if (format == NXGL_BACKEND_COMPRESSED_DXT3) {
        native_format = NXGL_BACKEND_TEXTURE_FORMAT_DXT3;
        block_size = 16;
    } else if (format == NXGL_BACKEND_COMPRESSED_DXT5) {
        native_format = NXGL_BACKEND_TEXTURE_FORMAT_DXT5;
        block_size = 16;
    } else {
        return 1;
    }

    pitch = (uint16_t)(((width + 3u) / 4u) * (uint16_t)block_size);
    texture->width = width;
    texture->height = height;
    texture->depth = 1;
    texture->pitch = pitch;
    texture->format = native_format;
    texture->cube_map = false;
    texture->volume = false;
    texture->addr = MmAllocateContiguousMemoryEx(data_size, 0, NXGL_BACKEND_MAXRAM, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (texture->addr == NULL) {
        return 1;
    }
    memcpy(texture->addr, data, data_size);
    return 0;
}

void nxgl_backend_texture_destroy(NxglBackendTexture *texture)
{
    if (texture != NULL && texture->addr != NULL) {
        MmFreeContiguousMemory(texture->addr);
        texture->addr = NULL;
    }
    if (texture != NULL) {
        texture->width = 0;
        texture->height = 0;
        texture->depth = 0;
        texture->pitch = 0;
        texture->format = 0;
        texture->cube_map = false;
        texture->volume = false;
    }
}

void nxgl_backend_bind_texture(NxglBackendTexture *texture)
{
    bound_texture = texture;
}

void nxgl_backend_bind_texture1(NxglBackendTexture *texture)
{
    bound_texture1 = texture;
}

void nxgl_backend_set_texture_env(NxglBackendTextureEnvMode mode, NxglBackendColor color)
{
    bound_texture_env_mode = mode;
    bound_texture_env_color = color;
}

void nxgl_backend_flush(void)
{
    if (!scene_dirty || vertex_count == 0 || batch_count == 0) {
        return;
    }

    while (pb_busy()) {
    }

    Matrix view;
    Matrix proj;
    Matrix viewport;
    Matrix proj_viewport;
    Matrix mvp;

    matrix_identity(view);
    matrix_rotate(view, view, camera_rot);
    matrix_translate(view, view, camera_pos);
    matrix_projection(proj, (float)back_width / (float)back_height, projection_near_z, projection_far_z);
    matrix_viewport(viewport, 0, 0, (float)back_width, (float)back_height, 0.0f, 65536.0f);
    matrix_multiply(proj_viewport, proj, viewport);
    matrix_multiply(mvp, view, proj_viewport);

    uint32_t *p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96);
    pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, 16);
    memcpy(p, mvp, 16 * 4);
    p += 16;
    pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT, 16);
    for (int i = 0; i < 16; ++i) {
        *(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
    }
    pb_end(p);

    for (unsigned int i = 0; i < batch_count; ++i) {
        NxglBackendBatch *batch = &batches[i];
        bool textured = batch->texture != NULL && batch->texture->addr != NULL;
        bool cube_textured = textured && batch->texture->cube_map;
        bool volume_textured = textured && batch->texture->volume;
        bool multitextured = textured && !cube_textured && !volume_textured && batch->texture1 != NULL && batch->texture1->addr != NULL;

        setup_render_state(batch->blend, batch->blend_sfactor, batch->blend_dfactor,
                           batch->depth_test, batch->depth_write, batch->cull,
                           batch->cull_face, batch->front_face,
                           batch->scissor, batch->scissor_x, batch->scissor_y,
                           batch->scissor_w, batch->scissor_h);
        if (multitextured) {
            setup_texture_stage(batch->texture);
            setup_texture_stage1(batch->texture1);
            load_multitexture_shader();
        } else if (cube_textured) {
            setup_texture_stage(batch->texture);
            load_cube_texture_shader();
        } else if (volume_textured) {
            setup_texture_stage(batch->texture);
            load_texture3d_shader();
        } else if (textured) {
            setup_texture_stage(batch->texture);
            load_texture_shader(batch->texture_env_mode, batch->texture_env_color);
        } else {
            disable_texture_stages();
            load_color_shader();
        }

        set_attrib_pointer(0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
                           3, sizeof(NxglBackendGpuVertex), &vertex_buffer[0].pos[0]);
        set_attrib_pointer(3, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
                           4, sizeof(NxglBackendGpuVertex), &vertex_buffer[0].color[0]);
        set_attrib_pointer(9, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
                           3, sizeof(NxglBackendGpuVertex), &vertex_buffer[0].tex0[0]);
        set_attrib_pointer(10, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
                           3, sizeof(NxglBackendGpuVertex), &vertex_buffer[0].tex1[0]);

        draw_arrays_range(batch->start, batch->count);
    }

    while (pb_busy()) {
    }

    submitted_vertex_count += vertex_count;
    vertex_count = 0;
    batch_count = 0;
    scene_dirty = false;
}

int nxgl_backend_back_buffer_width(void)
{
    return back_width;
}

int nxgl_backend_back_buffer_height(void)
{
    return back_height;
}

void nxgl_backend_finish(const char *title, const char *detail)
{
    nxgl_backend_flush();

    if (title != NULL) {
        pb_print("%s\n", title);
    }
    if (detail != NULL) {
        pb_print("%s\n", detail);
    }
    pb_print("vertices=%u\n", submitted_vertex_count);
    pb_draw_text_screen();

    while (pb_busy()) {
    }
    while (pb_finished()) {
    }
}
