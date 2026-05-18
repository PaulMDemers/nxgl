#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[12];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat a, GLfloat b, GLfloat tolerance)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= tolerance;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 8;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b) && p[3] == 255;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_ALPHA_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_center_quad(float z, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, z);
    glVertex3f(0.55f, 0.55f, z);
    glVertex3f(0.55f, -0.55f, z);
    glVertex3f(-0.55f, -0.55f, z);
    glEnd();
}

static void read_center_color(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    GLint iv[4] = { 0, 0, 0, 0 };
    GLfloat fv[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat raster[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat depth = 0.0f;
    GLfloat depth_src = 0.0f;
    GLushort depth16 = 0;
    uint8_t stencil = 0;
    uint8_t stencil_src = 0;
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_DEPTH_BITS, &iv[0]);
    glGetIntegerv(GL_STENCIL_BITS, &iv[1]);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fv[0]);
    ok = iv[0] == 24 && iv[1] == 8 && near_float(fv[0], 1.0f, 0.0001f) && consume_error(GL_NO_ERROR);
    expect_bool("default depth stencil query", ok, 0);

    glClearDepth(0.25f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = near_float(depth, 0.25f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("depth float clear read", ok, 1);

    glClearDepth(0.5f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, &depth16);
    ok = depth16 >= 32760 && depth16 <= 32776 && consume_error(GL_NO_ERROR);
    expect_bool("depth ushort read", ok, 2);

    reset_state();
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_center_quad(0.0f, 0.9f, 0.1f, 0.1f);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = depth > 0.03f && depth < 0.08f && consume_error(GL_NO_ERROR);
    expect_bool("primitive depth write", ok, 3);

    reset_state();
    glClearDepth(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_center_quad(0.0f, 0.9f, 0.1f, 0.1f);
    draw_center_quad(-1.0f, 0.1f, 0.9f, 0.1f);
    read_center_color(pixel);
    ok = pixel_rgb(pixel, 230, 25, 25) && consume_error(GL_NO_ERROR);
    expect_bool("farther depth reject", ok, 4);

    reset_state();
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_FALSE);
    draw_center_quad(0.0f, 0.1f, 0.1f, 0.9f);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = near_float(depth, 1.0f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("depth mask false", ok, 5);

    reset_state();
    glClearStencil(7);
    glClear(GL_STENCIL_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    ok = stencil == 7 && consume_error(GL_NO_ERROR);
    expect_bool("stencil clear read", ok, 6);

    reset_state();
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 3, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_center_quad(0.0f, 0.8f, 0.8f, 0.1f);
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    ok = stencil == 3 && consume_error(GL_NO_ERROR);
    expect_bool("stencil replace write", ok, 7);

    reset_state();
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    depth_src = 0.33f;
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glDrawPixels(1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_src);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = near_float(depth, 0.33f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("draw depth pixels", ok, 8);

    reset_state();
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    depth_src = 0.42f;
    glRasterPos3f(-0.18f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    GLint src_x = (GLint)raster[0];
    GLint src_y = (GLint)raster[1];
    glDrawPixels(1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_src);
    glRasterPos3f(0.18f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glCopyPixels(src_x, src_y, 1, 1, GL_DEPTH);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = near_float(depth, 0.42f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("copy depth pixels", ok, 9);

    reset_state();
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    stencil_src = 11;
    glRasterPos3f(-0.18f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    src_x = (GLint)raster[0];
    src_y = (GLint)raster[1];
    glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_src);
    glRasterPos3f(0.18f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glCopyPixels(src_x, src_y, 1, 1, GL_STENCIL);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    ok = stencil == 11 && consume_error(GL_NO_ERROR);
    expect_bool("draw copy stencil pixels", ok, 10);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glClearDepth(0.75f);
    glClearStencil(5);
    glEndList();
    glClearDepth(1.0f);
    glClearStencil(0);
    glCallList(list);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fv[0]);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, iv);
    ok = near_float(fv[0], 0.75f, 0.0001f) && iv[0] == 5 && consume_error(GL_NO_ERROR);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_BYTE, &depth);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_FLOAT, &depth);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glReadPixels(-1, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glDrawPixels(1, 1, GL_DEPTH_COMPONENT, GL_BYTE, &depth_src);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glCopyPixels(320, 240, 1, 1, GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("display-list validation", ok, 11);
}

static void draw_result_bar(float x, bool pass)
{
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
    for (int i = 0; i < 12; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL depth/stencil pixels probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClearDepth(1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        draw_center_quad(-0.6f, 0.12f, 0.45f, 0.9f);
        draw_center_quad(0.0f, 0.9f, 0.72f, 0.16f);
        glDisable(GL_DEPTH_TEST);
        for (int i = 0; i < 12; ++i) {
            draw_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL depth/stencil pixels", all_passed() ? "all checks passed" : "depth/stencil check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
