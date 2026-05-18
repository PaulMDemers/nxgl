#ifndef NXGL_BACKEND_H
#define NXGL_BACKEND_H

#include <stdbool.h>
#include <stdint.h>

typedef struct N3Color {
    float r;
    float g;
    float b;
    float a;
} N3Color;

typedef struct N3Vec3 {
    float x;
    float y;
    float z;
} N3Vec3;

typedef struct N3Vertex {
    N3Vec3 pos;
    N3Color color;
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
    N3Vec3 eye;
    N3Color base_color;
    N3Vec3 normal;
    float clip_x;
    float clip_y;
    float clip_z;
    float clip_w;
    float window_z;
} N3Vertex;

typedef struct N3Texture {
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t pitch;
    uint32_t format;
    bool cube_map;
    bool volume;
    void *addr;
} N3Texture;

typedef enum N3CompressedTextureFormat {
    N3_COMPRESSED_DXT1 = 0,
    N3_COMPRESSED_DXT3,
    N3_COMPRESSED_DXT5
} N3CompressedTextureFormat;

typedef enum N3TextureEnvMode {
    N3_TEXENV_MODULATE = 0,
    N3_TEXENV_REPLACE,
    N3_TEXENV_DECAL,
    N3_TEXENV_BLEND,
    N3_TEXENV_ADD,
    N3_TEXENV_SUBTRACT,
    N3_TEXENV_ADD_SIGNED,
    N3_TEXENV_INTERPOLATE
} N3TextureEnvMode;

int n3_init(void);
void n3_shutdown(void);
void n3_begin(uint32_t clear_color, bool blend);
void n3_begin_frame(bool blend);
void n3_clear_color(uint32_t clear_color, bool red, bool green, bool blue, bool alpha, int x, int y, int width, int height);
void n3_clear_depth_stencil(bool depth, float depth_value, bool stencil, uint8_t stencil_value, int x, int y, int width, int height);
void n3_set_depth(bool test, bool write);
void n3_set_cull(bool enabled);
void n3_set_cull_mode(uint32_t face, uint32_t front_face);
void n3_set_blend_func(uint32_t sfactor, uint32_t dfactor);
void n3_set_scissor(bool enabled, int x, int y, int width, int height);
void n3_set_projection(float fov_y_degrees, float near_z, float far_z);
void n3_set_camera(float x, float y, float z, float rx, float ry, float rz);
void n3_push_triangle(N3Vertex a, N3Vertex b, N3Vertex c);
void n3_push_quad(N3Vertex a, N3Vertex b, N3Vertex c, N3Vertex d);
int n3_texture_create_rgba(N3Texture *texture, uint16_t width, uint16_t height, const uint8_t *rgba);
int n3_texture_create_rgba3d(N3Texture *texture, uint16_t width, uint16_t height, uint16_t depth, const uint8_t *rgba);
int n3_texture_create_cube_rgba(N3Texture *texture, uint16_t size, const uint8_t *faces[6]);
int n3_texture_create_compressed(N3Texture *texture, uint16_t width, uint16_t height, N3CompressedTextureFormat format, const uint8_t *data, uint32_t data_size);
void n3_texture_destroy(N3Texture *texture);
void n3_bind_texture(N3Texture *texture);
void n3_bind_texture1(N3Texture *texture);
void n3_set_texture_env(N3TextureEnvMode mode, N3Color color);
void n3_flush(void);
int n3_back_buffer_width(void);
int n3_back_buffer_height(void);
void n3_finish(const char *title, const char *detail);

#endif
