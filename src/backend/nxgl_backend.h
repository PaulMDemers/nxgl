#ifndef NXGL_BACKEND_H
#define NXGL_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct NxglBackendColor {
    float r;
    float g;
    float b;
    float a;
} NxglBackendColor;

typedef struct NxglBackendVec3 {
    float x;
    float y;
    float z;
} NxglBackendVec3;

typedef struct NxglBackendVertex {
    NxglBackendVec3 pos;
    NxglBackendColor color;
    float u;
    float v;
    float r;
    float u1;
    float v1;
    float r1;
    float u2;
    float v2;
    float r2;
    float u3;
    float v3;
    float r3;
    NxglBackendVec3 eye;
    NxglBackendColor base_color;
    NxglBackendVec3 normal;
    float clip_x;
    float clip_y;
    float clip_z;
    float clip_w;
    float window_z;
} NxglBackendVertex;

typedef struct NxglBackendTexture {
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t pitch;
    uint32_t format;
    bool cube_map;
    bool volume;
    void *addr;
} NxglBackendTexture;

typedef enum NxglBackendPrimitive {
    NXGL_BACKEND_PRIMITIVE_TRIANGLES = 0,
    NXGL_BACKEND_PRIMITIVE_TRIANGLE_STRIP,
    NXGL_BACKEND_PRIMITIVE_TRIANGLE_FAN,
    NXGL_BACKEND_PRIMITIVE_QUADS,
    NXGL_BACKEND_PRIMITIVE_QUAD_STRIP
} NxglBackendPrimitive;

typedef enum NxglBackendCompressedTextureFormat {
    NXGL_BACKEND_COMPRESSED_DXT1 = 0,
    NXGL_BACKEND_COMPRESSED_DXT3,
    NXGL_BACKEND_COMPRESSED_DXT5
} NxglBackendCompressedTextureFormat;

typedef enum NxglBackendTextureEnvMode {
    NXGL_BACKEND_TEXENV_MODULATE = 0,
    NXGL_BACKEND_TEXENV_REPLACE,
    NXGL_BACKEND_TEXENV_DECAL,
    NXGL_BACKEND_TEXENV_BLEND,
    NXGL_BACKEND_TEXENV_ADD,
    NXGL_BACKEND_TEXENV_SUBTRACT,
    NXGL_BACKEND_TEXENV_ADD_SIGNED,
    NXGL_BACKEND_TEXENV_INTERPOLATE
} NxglBackendTextureEnvMode;

typedef struct NxglBackendPerfCounters {
    uint32_t shader_uploads;
    uint32_t shader_cache_hits;
    uint32_t render_state_uploads;
    uint32_t render_state_cache_hits;
    uint32_t texture_stage_uploads;
    uint32_t texture_stage_cache_hits;
    uint32_t texture_stage_disables;
    uint32_t texture_stage_disable_hits;
} NxglBackendPerfCounters;

int nxgl_backend_init(void);
void nxgl_backend_shutdown(void);
void nxgl_backend_begin(uint32_t clear_color, bool blend);
void nxgl_backend_begin_frame(bool blend);
void nxgl_backend_clear_color(uint32_t clear_color, bool red, bool green, bool blue, bool alpha, int x, int y, int width, int height);
void nxgl_backend_clear_depth_stencil(bool depth, float depth_value, bool stencil, uint8_t stencil_value, int x, int y, int width, int height);
void nxgl_backend_set_depth(bool test, bool write);
void nxgl_backend_set_cull(bool enabled);
void nxgl_backend_set_cull_mode(uint32_t face, uint32_t front_face);
void nxgl_backend_set_blend(bool enabled);
void nxgl_backend_set_blend_func(uint32_t sfactor, uint32_t dfactor);
void nxgl_backend_set_scissor(bool enabled, int x, int y, int width, int height);
void nxgl_backend_set_viewport(int x, int y, int width, int height);
void nxgl_backend_set_projection(float fov_y_degrees, float near_z, float far_z);
void nxgl_backend_set_camera(float x, float y, float z, float rx, float ry, float rz);
void nxgl_backend_push_triangle(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c);
void nxgl_backend_push_quad(NxglBackendVertex a, NxglBackendVertex b, NxglBackendVertex c, NxglBackendVertex d);
void nxgl_backend_push_primitive(NxglBackendPrimitive primitive, const NxglBackendVertex *vertices, unsigned int count);
bool nxgl_backend_push_indexed_primitive(NxglBackendPrimitive primitive,
                                         const NxglBackendVertex *vertices,
                                         unsigned int unique_vertex_count,
                                         const uint16_t *indices,
                                         unsigned int index_count);
int nxgl_backend_texture_create_rgba(NxglBackendTexture *texture, uint16_t width, uint16_t height, const uint8_t *rgba);
int nxgl_backend_texture_create_rgba3d(NxglBackendTexture *texture, uint16_t width, uint16_t height, uint16_t depth, const uint8_t *rgba);
int nxgl_backend_texture_create_cube_rgba(NxglBackendTexture *texture, uint16_t size, const uint8_t *faces[6]);
int nxgl_backend_texture_create_compressed(NxglBackendTexture *texture, uint16_t width, uint16_t height, NxglBackendCompressedTextureFormat format, const uint8_t *data, uint32_t data_size);
void nxgl_backend_texture_destroy(NxglBackendTexture *texture);
void nxgl_backend_bind_texture(NxglBackendTexture *texture);
void nxgl_backend_bind_texture1(NxglBackendTexture *texture);
void nxgl_backend_set_texture_env(NxglBackendTextureEnvMode mode, NxglBackendColor color);
void nxgl_backend_flush(void);
int nxgl_backend_back_buffer_width(void);
int nxgl_backend_back_buffer_height(void);
void nxgl_backend_reset_perf_counters(void);
void nxgl_backend_get_perf_counters(NxglBackendPerfCounters *counters);
void nxgl_backend_finish(const char *title, const char *detail);

#endif
