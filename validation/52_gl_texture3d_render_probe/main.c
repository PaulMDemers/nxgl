#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];
static GLuint tex3d;

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
    return d <= 18;
}

static bool pixel_matches(const uint8_t *actual, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(actual[0], r) &&
           near_byte(actual[1], g) &&
           near_byte(actual[2], b);
}

static void fill_volume(uint8_t *volume)
{
    const uint8_t colors[3][3] = {
        { 255, 0, 0 },
        { 0, 255, 0 },
        { 0, 0, 255 }
    };

    for (int z = 0; z < 3; ++z) {
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                uint8_t *p = volume + (((z * 4 + y) * 4 + x) * 4);
                p[0] = colors[z][0];
                p[1] = colors[z][1];
                p[2] = colors[z][2];
                p[3] = 255;
            }
        }
    }
}

static void upload_texture(void)
{
    uint8_t volume[4 * 4 * 3 * 4];

    fill_volume(volume);
    glGenTextures(1, &tex3d);
    glBindTexture(GL_TEXTURE_3D, tex3d);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, volume);
}

static void draw_textured_quad(float x, float r)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord3f(0.0f, 0.0f, r);
    glVertex3f(x - 0.38f, 0.38f, 0.0f);
    glTexCoord3f(1.0f, 0.0f, r);
    glVertex3f(x + 0.38f, 0.38f, 0.0f);
    glTexCoord3f(1.0f, 1.0f, r);
    glVertex3f(x + 0.38f, -0.38f, 0.0f);
    glTexCoord3f(0.0f, 1.0f, r);
    glVertex3f(x - 0.38f, -0.38f, 0.0f);
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
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_TEXTURE_3D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_3D, tex3d);

    draw_textured_quad(-1.05f, 0.0f);
    draw_textured_quad(0.0f, 0.5f);
    draw_textured_quad(1.05f, 1.0f);
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLint value = 0;

    glBindTexture(GL_TEXTURE_3D, tex3d);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &value);
    expect_bool("3d depth query", value == 3 && consume_error(GL_NO_ERROR), 0);

    glGetTexParameteriv(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, &value);
    expect_bool("3d wrap r render state", value == GL_CLAMP_TO_EDGE && consume_error(GL_NO_ERROR), 1);

    glEnable(GL_TEXTURE_3D);
    expect_bool("3d texture enabled", glIsEnabled(GL_TEXTURE_3D) == GL_TRUE && consume_error(GL_NO_ERROR), 2);

    render_scene();
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(258, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("slice 0 red readback", pixel_matches(pixel, 255, 0, 0) && consume_error(GL_NO_ERROR), 3);

    memset(pixel, 0, sizeof(pixel));
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("slice 1 green readback", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 4);

    memset(pixel, 0, sizeof(pixel));
    glReadPixels(382, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("slice 2 blue readback", pixel_matches(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR), 5);

    glDisable(GL_TEXTURE_3D);
    render_scene();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("render scene reenables 3d", pixel_matches(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 6);

    glTexImage2D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    expect_bool("reject teximage2d on 3d target", consume_error(GL_INVALID_ENUM), 7);
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

static bool all_passed(void)
{
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL 3D texture render probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    upload_texture();
    run_probe();

    for (;;) {
        render_scene();
        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.05f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL 3D texture render", all_passed() ? "3d texture render checks passed" : "one or more 3d render checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
