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

static void draw_center_quad(float r, float g, float b, float a)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glEnd();
}

static bool pixel_rgb_near(const uint8_t *rgba, uint8_t r, uint8_t g, uint8_t b)
{
    int dr = (int)rgba[0] - (int)r;
    int dg = (int)rgba[1] - (int)g;
    int db = (int)rgba[2] - (int)b;
    if (dr < 0) dr = -dr;
    if (dg < 0) dg = -dg;
    if (db < 0) db = -db;
    return dr <= 6 && dg <= 6 && db <= 6;
}

static void reset_render_state(void)
{
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glLogicOp(GL_COPY);
    glAlphaFunc(GL_ALWAYS, 0.0f);
}

static void run_probe(void)
{
    GLint iv[4] = { -1, -1, -1, -1 };
    GLfloat f = -1.0f;
    GLboolean bv[4] = { 0, 0, 0, 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_render_state();
    glGetIntegerv(GL_ALPHA_TEST_FUNC, iv);
    ok = iv[0] == GL_ALWAYS && glIsEnabled(GL_ALPHA_TEST) == GL_FALSE && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_ALPHA_TEST_REF, &f);
    ok = ok && near_float(f, 0.0f) && consume_error(GL_NO_ERROR);
    expect_bool("default alpha state", ok, 0);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, iv);
    ok = iv[0] == GL_GREATER && glIsEnabled(GL_ALPHA_TEST) == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetFloatv(GL_ALPHA_TEST_REF, &f);
    ok = ok && near_float(f, 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("alpha func query", ok, 1);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    draw_center_quad(1.0f, 0.0f, 0.0f, 0.25f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb_near(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    draw_center_quad(0.0f, 1.0f, 0.0f, 0.75f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && pixel_rgb_near(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("alpha render readback", ok, 2);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_LEQUAL, 0.25f);
    glEndList();
    glDisable(GL_ALPHA_TEST);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glCallList(list);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, iv);
    glGetFloatv(GL_ALPHA_TEST_REF, &f);
    ok = iv[0] == GL_LEQUAL && near_float(f, 0.25f) &&
         glIsEnabled(GL_ALPHA_TEST) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("display-list alpha replay", ok, 3);

    reset_render_state();
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    ok = bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("default color mask", ok, 4);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
    draw_center_quad(1.0f, 1.0f, 1.0f, 1.0f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb_near(pixel, 255, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("color mask readback", ok, 5);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
    glEndList();
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glCallList(list);
    glGetIntegerv(GL_COLOR_WRITEMASK, iv);
    ok = iv[0] == GL_FALSE && iv[1] == GL_TRUE && iv[2] == GL_FALSE && iv[3] == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("display-list mask replay", ok, 6);

    reset_render_state();
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glGetIntegerv(GL_LOGIC_OP_MODE, iv);
    ok = iv[0] == GL_XOR && glIsEnabled(GL_COLOR_LOGIC_OP) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("logic op query", ok, 7);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    draw_center_quad(1.0f, 1.0f, 1.0f, 1.0f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb_near(pixel, 255, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("logic op readback", ok, 8);

    glAlphaFunc(GL_TEXTURE_2D, 0.0f);
    ok = consume_error(GL_INVALID_ENUM);
    glLogicOp(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetIntegerv(0xdead, iv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("alpha logic validation", ok, 9);

    reset_render_state();
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
    debugPrint("NXGL alpha logic probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_render_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        draw_center_quad(0.10f, 0.16f, 0.30f, 1.0f);
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        draw_center_quad(0.90f, 0.84f, 0.20f, 1.0f);
        glDisable(GL_COLOR_LOGIC_OP);

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL alpha logic", all_passed() ? "all checks passed" : "alpha logic check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
