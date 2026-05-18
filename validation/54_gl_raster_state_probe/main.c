#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d < 0.001f;
}

static void draw_full_quad(float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-30.0f, 30.0f, 0.0f);
    glVertex3f(30.0f, 30.0f, 0.0f);
    glVertex3f(30.0f, -30.0f, 0.0f);
    glVertex3f(-30.0f, -30.0f, 0.0f);
    glEnd();
}

static bool pixel_is_red(const uint8_t *rgba)
{
    return rgba[0] > 180 && rgba[1] < 80 && rgba[2] < 80;
}

static bool pixel_is_black(const uint8_t *rgba)
{
    return rgba[0] < 30 && rgba[1] < 30 && rgba[2] < 30;
}

static void run_probe(void)
{
    GLint iv[4] = { -1, -1, -1, -1 };
    GLfloat fv[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
    uint8_t inside[4] = { 0, 0, 0, 0 };
    uint8_t outside[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    glGetIntegerv(GL_SCISSOR_BOX, iv);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 640 && iv[3] == 480 &&
         glIsEnabled(GL_SCISSOR_TEST) == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("default scissor state", ok, 0);

    glScissor(220, 150, 200, 120);
    glEnable(GL_SCISSOR_TEST);
    glGetIntegerv(GL_SCISSOR_BOX, iv);
    ok = iv[0] == 220 && iv[1] == 150 && iv[2] == 200 && iv[3] == 120 &&
         glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_SCISSOR_BOX, fv);
    ok = ok && near_float(fv[0], 220.0f) && near_float(fv[1], 150.0f) &&
         near_float(fv[2], 200.0f) && near_float(fv[3], 120.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("scissor query paths", ok, 1);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glScissor(220, 150, 200, 120);
    draw_full_quad(0.92f, 0.08f, 0.08f);
    glDisable(GL_SCISSOR_TEST);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, inside);
    ok = pixel_is_red(inside) && consume_error(GL_NO_ERROR);
    glReadPixels(80, 80, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, outside);
    ok = ok && pixel_is_black(outside) && consume_error(GL_NO_ERROR);
    expect_bool("scissor render readback", ok, 2);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(260, 170, 120, 100);
    glEndList();
    glDisable(GL_SCISSOR_TEST);
    glScissor(0, 0, 640, 480);
    glCallList(list);
    glGetIntegerv(GL_SCISSOR_BOX, iv);
    ok = iv[0] == 260 && iv[1] == 170 && iv[2] == 120 && iv[3] == 100 &&
         glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("display-list scissor replay", ok, 3);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.25f, -2.5f);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &fv[0]);
    ok = near_float(fv[0], 1.25f) && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &fv[0]);
    ok = ok && near_float(fv[0], -2.5f) &&
         glIsEnabled(GL_POLYGON_OFFSET_FILL) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("polygon offset state", ok, 4);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-3.0f, 4.0f);
    glEndList();
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(0.0f, 0.0f);
    glCallList(list);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &fv[0]);
    ok = near_float(fv[0], -3.0f) && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &fv[0]);
    ok = ok && near_float(fv[0], 4.0f) &&
         glIsEnabled(GL_POLYGON_OFFSET_LINE) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("display-list offset replay", ok, 5);

    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, iv);
    ok = iv[0] == 0 && glIsEnabled(GL_STENCIL_TEST) == GL_FALSE && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_BITS, iv);
    ok = ok && iv[0] == 8 && consume_error(GL_NO_ERROR);
    glClearStencil(7);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, iv);
    ok = ok && iv[0] == 7 && consume_error(GL_NO_ERROR);
    expect_bool("stencil defaults clear", ok, 6);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 3, 0x55u);
    glGetIntegerv(GL_STENCIL_FUNC, iv);
    ok = iv[0] == GL_EQUAL && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_REF, iv);
    ok = ok && iv[0] == 3 && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, iv);
    ok = ok && iv[0] == 0x55 && glIsEnabled(GL_STENCIL_TEST) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("stencil func query", ok, 7);

    glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
    glStencilMask(0xaau);
    glGetIntegerv(GL_STENCIL_FAIL, iv);
    ok = iv[0] == GL_KEEP && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, iv);
    ok = ok && iv[0] == GL_INCR && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, iv);
    ok = ok && iv[0] == GL_REPLACE && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_STENCIL_WRITEMASK, iv);
    ok = ok && iv[0] == 0xaa && consume_error(GL_NO_ERROR);
    expect_bool("stencil op mask query", ok, 8);

    glScissor(0, 0, -1, 10);
    ok = consume_error(GL_INVALID_VALUE);
    glStencilFunc(GL_TEXTURE_2D, 0, 0);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glStencilOp(GL_TEXTURE_2D, GL_KEEP, GL_KEEP);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetFloatv(GL_STENCIL_BITS, fv);
    ok = ok && near_float(fv[0], 8.0f) && consume_error(GL_NO_ERROR);
    expect_bool("raster validation paths", ok, 9);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
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
    for (int i = 0; i < 10; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL raster state probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        draw_full_quad(0.08f, 0.14f, 0.28f);
        glEnable(GL_SCISSOR_TEST);
        glScissor(220, 150, 200, 120);
        draw_full_quad(0.92f, 0.08f, 0.08f);
        glDisable(GL_SCISSOR_TEST);

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL raster state", all_passed() ? "all checks passed" : "raster-state check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
