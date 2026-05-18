#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[12];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static GLfloat cf(uint8_t value)
{
    return (GLfloat)value / 255.0f;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 14;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= 0.025f;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
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
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glLogicOp(GL_COPY);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDepthRange(0.0f, 1.0f);
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    (void)glGetError();
}

static void clear_all(uint8_t r, uint8_t g, uint8_t b, GLfloat depth, GLint stencil)
{
    glClearColor(cf(r), cf(g), cf(b), 1.0f);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void draw_quad(float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    glColor4f(cf(r), cf(g), cf(b), cf(a));
    glBegin(GL_QUADS);
    glVertex3f(-0.65f, 0.65f, z);
    glVertex3f(0.65f, 0.65f, z);
    glVertex3f(0.65f, -0.65f, z);
    glVertex3f(-0.65f, -0.65f, z);
    glEnd();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static GLfloat read_depth(GLint x, GLint y)
{
    GLfloat depth = 0.0f;
    glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    return depth;
}

static uint8_t read_stencil(GLint x, GLint y)
{
    uint8_t stencil = 0;
    glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    return stencil;
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat depth;
    uint8_t stencil;
    GLint iv[4];
    GLboolean bv[4];
    GLfloat fv;
    GLuint list;
    bool ok;

    reset_state();
    clear_all(0, 0, 0, 1.0f, 4);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 7, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad(0.0f, 230, 30, 30, 64);
    read_color(320, 240, pixel);
    depth = read_depth(320, 240);
    stencil = read_stencil(320, 240);
    ok = pixel_rgb(pixel, 0, 0, 0) && near_float(depth, 1.0f) && stencil == 4 && consume_error(GL_NO_ERROR);
    expect_bool("alpha fail blocks depth stencil", ok, 0);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 4);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 9, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad(0.0f, 40, 220, 80, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 40, 220, 80) && read_depth(320, 240) < 0.55f &&
         read_stencil(320, 240) == 9 && consume_error(GL_NO_ERROR);
    expect_bool("alpha pass allows depth stencil", ok, 1);

    reset_state();
    clear_all(10, 20, 30, 0.25f, 0);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    draw_quad(0.0f, 200, 200, 200, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 10, 20, 30) && near_float(read_depth(320, 240), 0.25f) && consume_error(GL_NO_ERROR);
    expect_bool("depth fail after alpha blocks blend", ok, 2);

    reset_state();
    clear_all(0x12, 0x34, 0x56, 1.0f, 0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    draw_quad(0.0f, 0xff, 0xff, 0xff, 64);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0xed, 0xcb, 0xa9) && consume_error(GL_NO_ERROR);
    expect_bool("logic op overrides blend factors", ok, 3);

    reset_state();
    clear_all(0x0f, 0xf0, 0xaa, 1.0f, 0);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_OR);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
    draw_quad(0.0f, 0xf0, 0x0f, 0x55, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0xff, 0xf0, 0xff) && consume_error(GL_NO_ERROR);
    expect_bool("color mask after logic op", ok, 4);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 0);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_SET);
    draw_quad(0.0f, 1, 1, 1, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 255, 255);
    read_color(100, 100, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("scissor gates logic op", ok, 5);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 2);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 3, 0xff);
    glStencilOp(GL_INCR, GL_KEEP, GL_REPLACE);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_SET);
    draw_quad(0.0f, 1, 1, 1, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && read_stencil(320, 240) == 3 && consume_error(GL_NO_ERROR);
    expect_bool("stencil fail blocks logic color", ok, 6);

    reset_state();
    clear_all(0, 0, 0, 1.0f, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_SET);
    draw_quad(0.0f, 1, 1, 1, 255);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 255, 255) && near_float(read_depth(320, 240), 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("depth mask preserves logic color", ok, 7);

    reset_state();
    clear_all(0, 0, 255, 1.0f, 0);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    draw_quad(0.0f, 255, 255, 255, 64);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR);
    draw_quad(0.0f, 255, 255, 255, 255);
    read_color(320, 240, pixel);
    ok = ok && pixel_rgb(pixel, 255, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("alpha gates logic op", ok, 8);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_LEQUAL, 0.75f);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_AND_INVERTED);
    glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_FALSE);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &iv[0]);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv);
    glGetIntegerv(GL_LOGIC_OP_MODE, &iv[1]);
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    ok = glIsEnabled(GL_ALPHA_TEST) == GL_TRUE && iv[0] == (GLint)GL_LEQUAL &&
         fv > 0.749f && fv < 0.751f &&
         glIsEnabled(GL_COLOR_LOGIC_OP) == GL_TRUE && iv[1] == (GLint)GL_AND_INVERTED &&
         bv[0] == GL_FALSE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_FALSE &&
         consume_error(GL_NO_ERROR);
    glDeleteLists(list, 1);
    expect_bool("combined display-list replay", ok, 9);

    reset_state();
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_SET);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glPopAttrib();
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    ok = glIsEnabled(GL_ALPHA_TEST) == GL_FALSE &&
         glIsEnabled(GL_COLOR_LOGIC_OP) == GL_FALSE &&
         bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("attrib restores alpha logic", ok, 10);

    reset_state();
    glAlphaFunc(GL_ALWAYS, -1.0f);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv);
    ok = fv == 0.0f && consume_error(GL_NO_ERROR);
    glAlphaFunc(GL_ALWAYS, 2.0f);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv);
    ok = ok && fv == 1.0f && consume_error(GL_NO_ERROR);
    glBegin(GL_POINTS);
    glEnable(GL_COLOR_LOGIC_OP);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glDisable(GL_ALPHA_TEST);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    expect_bool("alpha logic edge validation", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_bar(float x, bool pass)
{
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.78f : 0.12f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.10f, -0.75f, 0.0f);
    glVertex3f(x + 0.10f, -0.75f, 0.0f);
    glVertex3f(x + 0.10f, -0.98f, 0.0f);
    glVertex3f(x - 0.10f, -0.98f, 0.0f);
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
    debugPrint("NXGL alpha logic interlock probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_all(4, 4, 10, 1.0f, 0);
        glColor3f(0.14f, 0.42f, 0.82f);
        glRectf(-1.55f, 0.42f, 1.55f, -0.42f);
        for (int i = 0; i < 12; ++i) {
            draw_bar(-1.45f + (float)i * 0.26f, results[i]);
        }
        nxglSwapBuffers("NXGL alpha/color logic interlocks", all_passed() ? "all checks passed" : "alpha logic interlock failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
