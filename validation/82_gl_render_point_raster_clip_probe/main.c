#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 14;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.03f;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    for (int i = 0; i < 6; ++i) {
        glDisable(GL_CLIP_PLANE0 + i);
    }
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);
    glPointSize(16.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_point(GLfloat x, GLfloat y, GLfloat z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat depth = 0.0f;
    GLboolean valid = GL_FALSE;
    GLfloat raster[4];
    GLdouble clip_x_positive[4] = { 1.0, 0.0, 0.0, 0.0 };
    bool ok;

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.9f, 0.1f, 0.1f);
    draw_point(0.0f, 0.0f, 0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 230, 25, 25) && consume_error(GL_NO_ERROR);
    expect_bool("render visible point", ok, 0);

    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = depth > 0.02f && depth < 0.09f && consume_error(GL_NO_ERROR);
    expect_bool("point depth shadow", ok, 1);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.1f, 0.7f, 0.9f);
    draw_point(0.0f, 0.0f, 5.5f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("near-clipped point miss", ok, 2);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.1f, 0.9f, 0.1f);
    draw_point(20.0f, 0.0f, 0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("offscreen point miss", ok, 3);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    glColor3f(0.9f, 0.9f, 0.1f);
    draw_point(-0.5f, 0.0f, 0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("user-clipped point miss", ok, 4);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.9f, 0.2f, 0.8f);
    draw_point(-5.15f, 0.0f, 0.0f);
    read_color(2, 240, pixel);
    ok = pixel_rgb(pixel, 230, 51, 204) && consume_error(GL_NO_ERROR);
    expect_bool("edge-visible point render", ok, 5);

    reset_state();
    glRasterPos3f(-5.15f, 0.0f, 0.0f);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = valid == GL_TRUE && raster[0] >= 0.0f && raster[0] < 8.0f && consume_error(GL_NO_ERROR);
    expect_bool("raster left edge valid", ok, 6);

    glRasterPos3f(-5.30f, 0.0f, 0.0f);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
    ok = valid == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("raster outside invalid", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
    glRectf(x - 0.17f, -1.42f, x + 0.17f, -1.68f);
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
    debugPrint("NXGL point/raster clip probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL point/raster clipping", all_passed() ? "all checks passed" : "point/raster clip check failed");
        Sleep(16);
    }
}
