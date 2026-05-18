#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];
static GLuint tex3d;
static GLuint tex3d_unit1;
static uint8_t expected[4 * 4 * 2 * 4];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected_error)
{
    return glGetError() == expected_error;
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

static uint8_t *pixel_at(uint8_t *volume, int x, int y, int z)
{
    return volume + (((z * 4 + y) * 4 + x) * 4);
}

static void set_pixel(uint8_t *volume, int x, int y, int z, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t *p = pixel_at(volume, x, y, z);
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

static void fill_volume(uint8_t *volume)
{
    for (int z = 0; z < 2; ++z) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                if (z == 0) {
                    set_pixel(volume, x, y, z, (uint8_t)(180 + x * 8), (uint8_t)(28 + y * 9), 36, 255);
                } else {
                    set_pixel(volume, x, y, z, 32, (uint8_t)(52 + x * 11), (uint8_t)(185 + y * 8), 255);
                }
            }
        }
    }
}

static void run_probe(void)
{
    uint8_t out[sizeof(expected)];
    uint8_t sub[2 * 2 * 1 * 4];
    GLint value = 0;
    bool ok;

    fill_volume(expected);

    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &value);
    expect_bool("max 3d texture size", value >= 256 && consume_error(GL_NO_ERROR), 0);

    glGenTextures(1, &tex3d);
    glBindTexture(GL_TEXTURE_3D, tex3d);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &value);
    expect_bool("3d binding query", value == (GLint)tex3d && consume_error(GL_NO_ERROR), 1);

    glEnable(GL_TEXTURE_3D);
    expect_bool("3d enable state", glIsEnabled(GL_TEXTURE_3D) == GL_TRUE && consume_error(GL_NO_ERROR), 2);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGetTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, &value);
    expect_bool("wrap r state", value == GL_CLAMP_TO_EDGE && consume_error(GL_NO_ERROR), 3);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, expected);
    ok = consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &value);
    ok = ok && value == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &value);
    ok = ok && value == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &value);
    ok = ok && value == 2 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
    ok = ok && value == GL_RGBA && consume_error(GL_NO_ERROR);
    expect_bool("3d level metadata", ok, 4);

    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("3d exact readback", bytes_equal(out, expected, sizeof(expected)) && consume_error(GL_NO_ERROR), 5);

    for (int i = 0; i < 4; ++i) {
        sub[i * 4 + 0] = 240;
        sub[i * 4 + 1] = 220;
        sub[i * 4 + 2] = 40;
        sub[i * 4 + 3] = 255;
    }
    glTexSubImage3D(GL_TEXTURE_3D, 0, 1, 1, 1, 2, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, sub);
    for (int y = 1; y <= 2; ++y) {
        for (int x = 1; x <= 2; ++x) {
            set_pixel(expected, x, y, 1, 240, 220, 40, 255);
        }
    }
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("3d subimage readback", bytes_equal(out, expected, sizeof(expected)) && consume_error(GL_NO_ERROR), 6);

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &tex3d_unit1);
    glBindTexture(GL_TEXTURE_3D, tex3d_unit1);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &value);
    ok = value == (GLint)tex3d_unit1 && consume_error(GL_NO_ERROR);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &value);
    ok = ok && value == (GLint)tex3d && consume_error(GL_NO_ERROR);
    expect_bool("per-unit 3d binding", ok, 7);

    glTexImage3D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, expected);
    expect_bool("reject wrong 3d target", consume_error(GL_INVALID_ENUM), 8);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, expected);
    ok = consume_error(GL_INVALID_VALUE);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 2, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, sub);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("reject bad 3d sizes", ok, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static void draw_swatch(float x, float y, float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.44f, y + 0.40f, 0.0f);
    glVertex3f(x + 0.44f, y + 0.40f, 0.0f);
    glVertex3f(x + 0.44f, y - 0.40f, 0.0f);
    glVertex3f(x - 0.44f, y - 0.40f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 10; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL 3D texture probe starting\n");

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
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

        draw_swatch(-0.58f, 0.25f, 0.85f, 0.14f, 0.10f);
        draw_swatch(0.58f, 0.25f, 0.94f, 0.86f, 0.16f);
        draw_swatch(0.0f, -0.70f, 0.10f, 0.32f, 0.90f);
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL 3D textures", all_passed() ? "3d texture storage checks passed" : "one or more 3d texture checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
