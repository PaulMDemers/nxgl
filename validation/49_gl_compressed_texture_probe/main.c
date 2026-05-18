#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[16];
static GLuint tex1d;
static GLuint tex2d;
static GLuint tex3d;
static GLuint cube_tex;

static const uint8_t dxt1_block[8] = {
    0x00, 0xF8, 0xE0, 0x07, 0x00, 0x55, 0xAA, 0xFF
};

static const uint8_t dxt1_block_alt[8] = {
    0x1F, 0x00, 0xFF, 0xFF, 0x12, 0x34, 0x56, 0x78
};

static const uint8_t dxt1_volume[16] = {
    0x00, 0xF8, 0xE0, 0x07, 0x00, 0x55, 0xAA, 0xFF,
    0x1F, 0x00, 0xFF, 0xFF, 0x12, 0x34, 0x56, 0x78
};

static const uint8_t dxt3_blocks[32] = {
    0xFF, 0x0F, 0xF0, 0x00, 0xAA, 0x55, 0x33, 0xCC,
    0x00, 0xF8, 0xE0, 0x07, 0x44, 0x44, 0x88, 0x88,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x1F, 0x00, 0xFF, 0xFF, 0x99, 0xAA, 0xBB, 0xCC
};

static const uint8_t dxt3_sub_block[16] = {
    0x08, 0x19, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F,
    0x34, 0x12, 0xCD, 0xAB, 0x01, 0x23, 0x45, 0x67
};

static const uint8_t dxt5_block[16] = {
    0x00, 0xFF, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92,
    0x00, 0xF8, 0x1F, 0x00, 0xE4, 0xE4, 0x1B, 0x1B
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

static bool bytes_equal(const uint8_t *a, const uint8_t *b, int count)
{
    for (int i = 0; i < count; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

static void run_probe(void)
{
    GLint values[8] = { 0 };
    uint8_t out[64];
    uint8_t expected3d[16];
    bool ok;

    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, values);
    ok = values[0] == 4 && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, values);
    ok = ok &&
         values[0] == GL_COMPRESSED_RGB_S3TC_DXT1_EXT &&
         values[1] == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT &&
         values[2] == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT &&
         values[3] == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT &&
         consume_error(GL_NO_ERROR);
    expect_bool("compressed format list", ok, 0);

    glGenTextures(1, &tex2d);
    glBindTexture(GL_TEXTURE_2D, tex2d);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 4, 0, 8, dxt1_block);
    expect_bool("dxt1 upload", consume_error(GL_NO_ERROR), 1);

    ok = true;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, values);
    ok = ok && values[0] == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, values);
    ok = ok && values[0] == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, values);
    ok = ok && values[0] == GL_COMPRESSED_RGB_S3TC_DXT1_EXT && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, values);
    ok = ok && values[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, values);
    ok = ok && values[0] == 8 && consume_error(GL_NO_ERROR);
    expect_bool("compressed level queries", ok, 2);

    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, out);
    expect_bool("dxt1 exact readback", bytes_equal(out, dxt1_block, 8) && consume_error(GL_NO_ERROR), 3);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 8, 4, 0, 32, dxt3_blocks);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, out);
    expect_bool("dxt3 two-block upload", bytes_equal(out, dxt3_blocks, 32) && consume_error(GL_NO_ERROR), 4);

    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 4, 0, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 16, dxt3_sub_block);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, out);
    ok = bytes_equal(out, dxt3_blocks, 16) && bytes_equal(out + 16, dxt3_sub_block, 16) && consume_error(GL_NO_ERROR);
    expect_bool("dxt3 subimage block", ok, 5);

    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 4, 4, 0, 16, dxt5_block);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, out);
    expect_bool("cube face dxt5 storage", bytes_equal(out, dxt5_block, 16) && consume_error(GL_NO_ERROR), 6);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 4, 0, 7, dxt1_block);
    expect_bool("reject bad image size", consume_error(GL_INVALID_VALUE), 7);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, 8, dxt1_block);
    expect_bool("reject bad compressed format", consume_error(GL_INVALID_ENUM), 8);

    glBindTexture(GL_TEXTURE_2D, tex2d);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, 4, 0, 16, dxt3_blocks);
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 2, 0, 4, 4, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, dxt1_block);
    expect_bool("reject unaligned subimage", consume_error(GL_INVALID_OPERATION), 9);

    glGenTextures(1, &tex3d);
    glBindTexture(GL_TEXTURE_3D, tex3d);
    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 4, 2, 0, 16, dxt1_volume);
    ok = consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, values);
    ok = ok && values[0] == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, values);
    ok = ok && values[0] == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, values);
    ok = ok && values[0] == 2 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED, values);
    ok = ok && values[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, values);
    ok = ok && values[0] == 16 && consume_error(GL_NO_ERROR);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_3D, 0, out);
    ok = ok && bytes_equal(out, dxt1_volume, 16) && consume_error(GL_NO_ERROR);
    expect_bool("dxt1 3d upload/readback", ok, 10);

    memcpy(expected3d, dxt1_volume, sizeof(expected3d));
    memcpy(expected3d + 8, dxt1_block, 8);
    glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 1, 4, 4, 1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, dxt1_block);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_3D, 0, out);
    expect_bool("dxt1 3d subimage slice", bytes_equal(out, expected3d, 16) && consume_error(GL_NO_ERROR), 11);

    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 4, 4, 2, 0, 15, dxt1_volume);
    expect_bool("reject bad 3d image size", consume_error(GL_INVALID_VALUE), 12);

    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, 4, 2, 0, 32, dxt3_blocks);
    glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 2, 0, 0, 4, 4, 1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, dxt1_block);
    expect_bool("reject unaligned 3d subimage", consume_error(GL_INVALID_OPERATION), 13);

    glGenTextures(1, &tex1d);
    glBindTexture(GL_TEXTURE_1D, tex1d);
    glCompressedTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, 0, 16, dxt3_blocks);
    ok = consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, values);
    ok = ok && values[0] == 8 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_HEIGHT, values);
    ok = ok && values[0] == 1 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED, values);
    ok = ok && values[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, values);
    ok = ok && values[0] == 16 && consume_error(GL_NO_ERROR);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_1D, 0, out);
    ok = ok && bytes_equal(out, dxt3_blocks, 16) && consume_error(GL_NO_ERROR);
    expect_bool("dxt1 1d upload/readback", ok, 14);

    memcpy(expected3d, dxt3_blocks, 16);
    memcpy(expected3d + 8, dxt1_block_alt, 8);
    glCompressedTexSubImage1D(GL_TEXTURE_1D, 0, 4, 4, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, 8, dxt1_block_alt);
    memset(out, 0, sizeof(out));
    glGetCompressedTexImage(GL_TEXTURE_1D, 0, out);
    expect_bool("dxt1 1d subimage block", bytes_equal(out, expected3d, 16) && consume_error(GL_NO_ERROR), 15);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
}

static void draw_format_swatch(float x, float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.32f, 0.35f, 0.0f);
    glVertex3f(x + 0.32f, 0.35f, 0.0f);
    glVertex3f(x + 0.32f, -0.35f, 0.0f);
    glVertex3f(x - 0.32f, -0.35f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 16; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL compressed texture probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        draw_format_swatch(-1.05f, 0.9f, 0.2f, 0.15f);
        draw_format_swatch(0.0f, 0.15f, 0.8f, 0.25f);
        draw_format_swatch(1.05f, 0.2f, 0.4f, 0.95f);
        for (int i = 0; i < 16; ++i) {
            draw_result_bar(-2.85f + (float)i * 0.38f, results[i]);
        }

        nxglSwapBuffers("NXGL compressed tex", all_passed() ? "compressed texture checks passed" : "one or more compressed texture checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
