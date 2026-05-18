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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 16;
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
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDepthRange(0.0f, 1.0f);
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
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

static void clear_all(float r, float g, float b, GLfloat depth, GLint stencil)
{
    glClearColor(r, g, b, 1.0f);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void draw_quad_ccw(float x0, float y0, float x1, float y1, float z, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(x0, y0, z);
    glVertex3f(x1, y0, z);
    glVertex3f(x1, y1, z);
    glVertex3f(x0, y1, z);
    glEnd();
}

static void draw_quad_cw(float x0, float y0, float x1, float y1, float z, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(x0, y1, z);
    glVertex3f(x1, y1, z);
    glVertex3f(x1, y0, z);
    glVertex3f(x0, y0, z);
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
    uint8_t value = 0;
    glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &value);
    return value;
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat depth_inside;
    GLfloat depth_outside;
    uint8_t stencil_inside;
    uint8_t stencil_outside;
    GLint iv[4];
    GLuint list;
    bool ok;

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 1.0f, 0);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    draw_quad_ccw(-1.2f, -1.0f, 1.2f, 1.0f, 0.0f, 0.1f, 0.8f, 0.2f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 26, 204, 51);
    read_color(120, 120, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0);
    depth_inside = read_depth(320, 240);
    depth_outside = read_depth(120, 120);
    ok = ok && depth_inside < 0.55f && depth_outside > 0.95f && consume_error(GL_NO_ERROR);
    expect_bool("scissor gates primitive color depth", ok, 0);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 1.0f, 1);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 7, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad_ccw(-1.2f, -1.0f, 1.2f, 1.0f, 0.0f, 0.8f, 0.8f, 0.1f, 1.0f);
    stencil_inside = read_stencil(320, 240);
    stencil_outside = read_stencil(120, 120);
    ok = stencil_inside == 7 && stencil_outside == 1 && consume_error(GL_NO_ERROR);
    expect_bool("scissor gates primitive stencil", ok, 1);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 0.25f, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.9f, 0.1f, 0.1f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && near_float(read_depth(320, 240), 0.25f) && consume_error(GL_NO_ERROR);
    expect_bool("depth fail blocks blend color", ok, 2);

    reset_state();
    clear_all(0.0f, 0.0f, 0.4f, 1.0f, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 1.0f, 0.0f, 0.0f, 0.50f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 128, 0, 51) && read_depth(320, 240) < 0.55f && consume_error(GL_NO_ERROR);
    expect_bool("depth pass blends and writes", ok, 3);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 1.0f, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.2f, 0.6f, 0.9f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 51, 153, 230) && near_float(read_depth(320, 240), 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("depth mask blocks primitive write", ok, 4);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 0.25f, 4);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_INCR, GL_REPLACE);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.9f, 0.7f, 0.1f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && read_stencil(320, 240) == 5 && consume_error(GL_NO_ERROR);
    expect_bool("depth fail applies zfail stencil", ok, 5);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 1.0f, 2);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 5, 0xff);
    glStencilOp(GL_INCR, GL_KEEP, GL_REPLACE);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.9f, 0.2f, 0.8f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && read_stencil(320, 240) == 3 && consume_error(GL_NO_ERROR);
    expect_bool("stencil fail blocks depth color", ok, 6);

    reset_state();
    clear_all(0.0f, 0.0f, 0.0f, 1.0f, 9);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 4, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad_cw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.9f, 0.9f, 0.1f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && near_float(read_depth(320, 240), 1.0f) &&
         read_stencil(320, 240) == 9 && consume_error(GL_NO_ERROR);
    expect_bool("cull prevents shadow writes", ok, 7);

    reset_state();
    clear_all(0.2f, 0.3f, 0.4f, 1.0f, 0);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    draw_quad_ccw(-0.7f, -0.7f, 0.7f, 0.7f, 0.0f, 0.2f, 0.5f, 0.1f, 1.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 102, 77, 128) && consume_error(GL_NO_ERROR);
    expect_bool("color mask after blend", ok, 8);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 3, 0x0f);
    glStencilOp(GL_INCR, GL_DECR, GL_REPLACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ONE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_SCISSOR_BOX, iv);
    ok = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE && iv[0] == 300 && iv[2] == 80;
    glGetIntegerv(GL_DEPTH_FUNC, &iv[0]);
    glGetIntegerv(GL_DEPTH_WRITEMASK, &iv[1]);
    ok = ok && glIsEnabled(GL_DEPTH_TEST) == GL_TRUE && iv[0] == (GLint)GL_LEQUAL && iv[1] == GL_FALSE;
    glGetIntegerv(GL_STENCIL_FUNC, &iv[0]);
    glGetIntegerv(GL_STENCIL_REF, &iv[1]);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &iv[2]);
    ok = ok && glIsEnabled(GL_STENCIL_TEST) == GL_TRUE && iv[0] == (GLint)GL_NOTEQUAL && iv[1] == 3 && iv[2] == 0x0f;
    glGetIntegerv(GL_BLEND_SRC, &iv[0]);
    glGetIntegerv(GL_BLEND_DST, &iv[1]);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[2]);
    glGetIntegerv(GL_FRONT_FACE, &iv[3]);
    ok = ok && glIsEnabled(GL_BLEND) == GL_TRUE && iv[0] == (GLint)GL_DST_COLOR && iv[1] == (GLint)GL_ONE &&
         glIsEnabled(GL_CULL_FACE) == GL_TRUE && iv[2] == (GLint)GL_FRONT && iv[3] == (GLint)GL_CW &&
         consume_error(GL_NO_ERROR);
    glDeleteLists(list, 1);
    expect_bool("combined display-list replay", ok, 9);

    reset_state();
    glScissor(0, 0, -1, 1);
    ok = consume_error(GL_INVALID_VALUE);
    glDepthFunc(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glStencilFunc(GL_TEXTURE_2D, 0, 0xff);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glStencilOp(GL_TEXTURE_2D, GL_KEEP, GL_KEEP);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("interlock validation", ok, 10);

    reset_state();
    glBegin(GL_TRIANGLES);
    glScissor(0, 0, 10, 10);
    ok = consume_error(GL_INVALID_OPERATION);
    glDepthFunc(GL_ALWAYS);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glDepthMask(GL_FALSE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("interlocks rejected in begin", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_bar(float x, bool pass)
{
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
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
    debugPrint("NXGL depth state interlock probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glColor3f(0.14f, 0.42f, 0.82f);
        glRectf(-1.55f, 0.42f, 1.55f, -0.42f);
        for (int i = 0; i < 12; ++i) {
            draw_bar(-1.45f + (float)i * 0.26f, results[i]);
        }
        nxglSwapBuffers("NXGL depth/blend/stencil interlocks", all_passed() ? "all checks passed" : "state interlock failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
