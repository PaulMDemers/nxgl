#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[11];
static GLuint tex_dxt1;
static GLuint tex_dxt3;
static GLuint tex_dxt5;
static GLuint tex_dxt1_mip;
static GLuint tex_dxt3_mip;
static GLuint tex_dxt5_mip;

static const uint8_t dxt1_red[8] = {
    0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt1_green[8] = {
    0xE0, 0x07, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt1_blue[8] = {
    0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt3_green[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xE0, 0x07, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt3_red[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt3_blue[16] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt5_blue[16] = {
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt5_red[16] = {
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t dxt5_green[16] = {
    0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xE0, 0x07, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00
};

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 12;
}

static bool pixel_matches(const uint8_t *actual, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(actual[0], r) &&
           near_byte(actual[1], g) &&
           near_byte(actual[2], b);
}

static void upload_texture(GLuint *texture, GLenum format, const uint8_t *block, GLsizei size)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, format, 4, 4, 0, size, block);
}

static void upload_dxt1_mip_texture(GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 4, 0, 8, dxt1_red);
    glCompressedTexImage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 2, 2, 0, 8, dxt1_green);
    glCompressedTexImage2D(GL_TEXTURE_2D, 2, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 1, 1, 0, 8, dxt1_blue);
}

static void upload_dxt3_mip_texture(GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 4, 4, 0, 16, dxt3_red);
    glCompressedTexImage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 2, 2, 0, 16, dxt3_green);
    glCompressedTexImage2D(GL_TEXTURE_2D, 2, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 1, 1, 0, 16, dxt3_blue);
}

static void upload_dxt5_mip_texture(GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 4, 4, 0, 16, dxt5_red);
    glCompressedTexImage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 2, 2, 0, 16, dxt5_green);
    glCompressedTexImage2D(GL_TEXTURE_2D, 2, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 1, 1, 0, 16, dxt5_blue);
}

static void setup_textures(void)
{
    upload_texture(&tex_dxt1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxt1_red, 8);
    upload_texture(&tex_dxt3, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, dxt3_green, 16);
    upload_texture(&tex_dxt5, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, dxt5_blue, 16);
    upload_dxt1_mip_texture(&tex_dxt1_mip);
    upload_dxt3_mip_texture(&tex_dxt3_mip);
    upload_dxt5_mip_texture(&tex_dxt5_mip);
}

static void draw_textured_quad(float x)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x - 0.36f, 0.36f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(x + 0.36f, 0.36f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(x + 0.36f, -0.36f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x - 0.36f, -0.36f, 0.0f);
    glEnd();
}

static void render_scene(void)
{
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glBindTexture(GL_TEXTURE_2D, tex_dxt1);
    draw_textured_quad(-1.05f);
    glBindTexture(GL_TEXTURE_2D, tex_dxt3);
    draw_textured_quad(0.0f);
    glBindTexture(GL_TEXTURE_2D, tex_dxt5);
    draw_textured_quad(1.05f);
}

static void render_bound_center_quad(void)
{
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    draw_textured_quad(0.0f);
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLint value = 0;

    glBindTexture(GL_TEXTURE_2D, tex_dxt1);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &value);
    expect_bool("dxt1 compressed level", value == GL_TRUE && consume_error(GL_NO_ERROR), 0);
    glBindTexture(GL_TEXTURE_2D, tex_dxt3);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &value);
    expect_bool("dxt3 image size", value == 16 && consume_error(GL_NO_ERROR), 1);
    glBindTexture(GL_TEXTURE_2D, tex_dxt5);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
    expect_bool("dxt5 internal format", value == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT && consume_error(GL_NO_ERROR), 2);

    render_scene();
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(258, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt1 red readback", pixel_matches(pixel, 255, 0, 0) && consume_error(GL_NO_ERROR), 3);

    memset(pixel, 0, sizeof(pixel));
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt3 green readback", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 4);

    memset(pixel, 0, sizeof(pixel));
    glReadPixels(382, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt5 blue readback", pixel_matches(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR), 5);

    glDisable(GL_TEXTURE_2D);
    render_scene();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("render scene reenables tex2d", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 6);

    glBindTexture(GL_TEXTURE_2D, tex_dxt1);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("compressed rejects teximage", consume_error(GL_INVALID_OPERATION), 7);

    glBindTexture(GL_TEXTURE_2D, tex_dxt1_mip);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 2.0f);
    render_bound_center_quad();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt1 mip bias readback", pixel_matches(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR), 8);

    glBindTexture(GL_TEXTURE_2D, tex_dxt3_mip);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    render_bound_center_quad();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt3 base level readback", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 9);

    glBindTexture(GL_TEXTURE_2D, tex_dxt5_mip);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 2.0f);
    render_bound_center_quad();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("dxt5 max lod clamp readback", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 10);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 11; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL compressed render probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    setup_textures();
    run_probe();

    for (;;) {
        render_scene();
        for (int i = 0; i < 11; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.53f, results[i]);
        }

        nxglSwapBuffers("NXGL DXT render", all_passed() ? "compressed render checks passed" : "one or more compressed render checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
