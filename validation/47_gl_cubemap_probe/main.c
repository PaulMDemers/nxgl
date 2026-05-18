#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[13];
static GLuint cube_tex;
static GLuint cube_tex_unit1;
static GLuint tex2d;
static uint8_t face_pixels[6][4 * 4 * 4];
static uint8_t readback[4 * 4 * 4];
static uint8_t mip_readback[2 * 2 * 4];

static const GLenum faces[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

static const uint8_t face_colors[6][4] = {
    { 255, 32, 32, 255 },
    { 32, 255, 32, 255 },
    { 32, 32, 255, 255 },
    { 255, 255, 32, 255 },
    { 255, 32, 255, 255 },
    { 32, 255, 255, 255 }
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

static bool pixel_matches(const uint8_t *actual, const uint8_t *expected)
{
    return actual[0] == expected[0] &&
           actual[1] == expected[1] &&
           actual[2] == expected[2] &&
           actual[3] == expected[3];
}

static void fill_face(int face)
{
    for (int i = 0; i < 4 * 4; ++i) {
        face_pixels[face][i * 4 + 0] = face_colors[face][0];
        face_pixels[face][i * 4 + 1] = face_colors[face][1];
        face_pixels[face][i * 4 + 2] = face_colors[face][2];
        face_pixels[face][i * 4 + 3] = face_colors[face][3];
    }
}

static void fill_solid(uint8_t *pixels, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    for (int i = 0; i < width * height; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = a;
    }
}

static void setup_textures(void)
{
    uint8_t texel[4] = { 180, 120, 60, 255 };

    for (int i = 0; i < 6; ++i) {
        fill_face(i);
    }

    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(faces[i], 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_pixels[i]);
    }

    glGenTextures(1, &tex2d);
    glBindTexture(GL_TEXTURE_2D, tex2d);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel);
}

static void run_probe(void)
{
    GLint value = 0;
    bool ok = true;
    uint8_t yellow[4] = { 255, 255, 0, 255 };
    uint8_t texel_out[4] = { 0, 0, 0, 0 };
    uint8_t mip1[6][2 * 2 * 4];
    uint8_t mip2[6][1 * 1 * 4];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
    expect_bool("cube map binding query", value == (GLint)cube_tex && consume_error(GL_NO_ERROR), 0);

    glEnable(GL_TEXTURE_CUBE_MAP);
    expect_bool("cube map enable query", glIsEnabled(GL_TEXTURE_CUBE_MAP) == GL_TRUE && consume_error(GL_NO_ERROR), 1);

    for (int i = 0; i < 6; ++i) {
        glGetTexLevelParameteriv(faces[i], 0, GL_TEXTURE_WIDTH, &value);
        ok = ok && value == 4 && consume_error(GL_NO_ERROR);
        glGetTexLevelParameteriv(faces[i], 0, GL_TEXTURE_HEIGHT, &value);
        ok = ok && value == 4 && consume_error(GL_NO_ERROR);
        glGetTexLevelParameteriv(faces[i], 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
        ok = ok && value == GL_RGBA && consume_error(GL_NO_ERROR);
    }
    expect_bool("cube face level queries", ok, 2);

    ok = true;
    for (int i = 0; i < 6; ++i) {
        memset(readback, 0, sizeof(readback));
        glGetTexImage(faces[i], 0, GL_RGBA, GL_UNSIGNED_BYTE, readback);
        ok = ok && pixel_matches(readback, face_colors[i]) && consume_error(GL_NO_ERROR);
    }
    expect_bool("cube face readback", ok, 3);

    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, yellow);
    memset(readback, 0, sizeof(readback));
    glGetTexImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, GL_UNSIGNED_BYTE, readback);
    expect_bool("cube face subimage", pixel_matches(readback + ((1 * 4 + 1) * 4), yellow) && consume_error(GL_NO_ERROR), 4);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &cube_tex_unit1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex_unit1);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
    ok = value == (GLint)cube_tex_unit1 && consume_error(GL_NO_ERROR);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
    ok = ok && value == (GLint)cube_tex && consume_error(GL_NO_ERROR);
    expect_bool("per-unit cube binding", ok, 5);

    glBindTexture(GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube_tex);
    expect_bool("reject cube face bind", consume_error(GL_INVALID_ENUM), 6);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, 4, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_pixels[0]);
    expect_bool("reject nonsquare face", consume_error(GL_INVALID_VALUE), 7);

    glGetTexImage(GL_TEXTURE_CUBE_MAP, 0, GL_RGBA, GL_UNSIGNED_BYTE, readback);
    expect_bool("reject cube get target", consume_error(GL_INVALID_ENUM), 8);

    glBindTexture(GL_TEXTURE_2D, tex2d);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel_out);
    expect_bool("2d texture path intact", pixel_matches(texel_out, (uint8_t[4]){ 180, 120, 60, 255 }) && consume_error(GL_NO_ERROR), 9);

    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 2);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 0.0f);
    for (int i = 0; i < 6; ++i) {
        fill_solid(mip1[i], 2, 2, (uint8_t)(40 + i * 20), (uint8_t)(160 - i * 12), (uint8_t)(70 + i * 15), 255);
        fill_solid(mip2[i], 1, 1, (uint8_t)(220 - i * 18), (uint8_t)(35 + i * 22), (uint8_t)(180 - i * 11), 255);
        glTexImage2D(faces[i], 1, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, mip1[i]);
        glTexImage2D(faces[i], 2, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mip2[i]);
    }
    ok = consume_error(GL_NO_ERROR);
    for (int i = 0; i < 6; ++i) {
        glGetTexLevelParameteriv(faces[i], 1, GL_TEXTURE_WIDTH, &value);
        ok = ok && value == 2 && consume_error(GL_NO_ERROR);
        glGetTexLevelParameteriv(faces[i], 1, GL_TEXTURE_HEIGHT, &value);
        ok = ok && value == 2 && consume_error(GL_NO_ERROR);
        glGetTexLevelParameteriv(faces[i], 2, GL_TEXTURE_WIDTH, &value);
        ok = ok && value == 1 && consume_error(GL_NO_ERROR);
    }
    expect_bool("cube mip level queries", ok, 10);

    ok = true;
    for (int i = 0; i < 6; ++i) {
        memset(mip_readback, 0, sizeof(mip_readback));
        glGetTexImage(faces[i], 1, GL_RGBA, GL_UNSIGNED_BYTE, mip_readback);
        ok = ok && pixel_matches(mip_readback, mip1[i]) && consume_error(GL_NO_ERROR);
        memset(readback, 0, sizeof(readback));
        glGetTexImage(faces[i], 2, GL_RGBA, GL_UNSIGNED_BYTE, readback);
        ok = ok && pixel_matches(readback, mip2[i]) && consume_error(GL_NO_ERROR);
    }
    expect_bool("cube mip readback", ok, 11);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 1);
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, &value);
    ok = value == 1 && consume_error(GL_NO_ERROR);
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &value);
    ok = ok && value == 1 && consume_error(GL_NO_ERROR);
    expect_bool("cube base/max mip params", ok, 12);
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

static void draw_face_swatch(int face, float x, float y)
{
    const uint8_t *c = face_colors[face];
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f((float)c[0] / 255.0f, (float)c[1] / 255.0f, (float)c[2] / 255.0f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.28f, y + 0.28f, 0.0f);
    glVertex3f(x + 0.28f, y + 0.28f, 0.0f);
    glVertex3f(x + 0.28f, y - 0.28f, 0.0f);
    glVertex3f(x - 0.28f, y - 0.28f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 13; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL cube-map probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    setup_textures();
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (int i = 0; i < 6; ++i) {
            draw_face_swatch(i, -1.75f + (float)i * 0.70f, 0.45f);
        }
        for (int i = 0; i < 13; ++i) {
            draw_result_bar(-2.80f + (float)i * 0.46f, results[i]);
        }

        nxglSwapBuffers("NXGL cube map", all_passed() ? "cube map checks passed" : "one or more cube map checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
