#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

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

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_render_state(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    static const GLenum depth_funcs[] = {
        GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
        GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS
    };
    GLint iv[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    GLfloat fv[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    GLdouble dv[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    GLboolean bv[4] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
    GLuint list;
    bool ok;

    glClearColor(0.25f, 0.50f, 0.75f, 1.0f);
    glGetFloatv(GL_CLEAR_COLOR, fv);
    glGetDoublev(GL_CLEAR_COLOR, dv);
    glGetBooleanv(GL_CLEAR_COLOR, bv);
    ok = near_float(fv[0], 0.25f) && near_float(fv[1], 0.50f) &&
         near_float(fv[2], 0.75f) && near_float(fv[3], 1.0f) &&
         dv[0] > 0.24 && dv[1] > 0.49 && dv[2] > 0.74 && dv[3] > 0.99 &&
         bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("clear color queries", ok, 0);

    glViewport(11, 22, 333, 222);
    glGetIntegerv(GL_VIEWPORT, iv);
    glGetFloatv(GL_VIEWPORT, fv);
    glGetDoublev(GL_VIEWPORT, dv);
    ok = iv[0] == 11 && iv[1] == 22 && iv[2] == 333 && iv[3] == 222 &&
         near_float(fv[0], 11.0f) && near_float(fv[1], 22.0f) &&
         dv[2] == 333.0 && dv[3] == 222.0 && consume_error(GL_NO_ERROR);
    expect_bool("viewport queries", ok, 1);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glViewport(8, 9, 321, 223);
    glDepthFunc(GL_GREATER);
    glEndList();
    glViewport(0, 0, 640, 480);
    glDepthFunc(GL_LESS);
    glCallList(list);
    glGetIntegerv(GL_VIEWPORT, iv);
    glGetIntegerv(GL_DEPTH_FUNC, &iv[4]);
    ok = iv[0] == 8 && iv[1] == 9 && iv[2] == 321 && iv[3] == 223 &&
         iv[4] == (GLint)GL_GREATER && consume_error(GL_NO_ERROR);
    expect_bool("display-list state", ok, 2);

    ok = true;
    for (int i = 0; i < 8; ++i) {
        glDepthFunc(depth_funcs[i]);
        glGetIntegerv(GL_DEPTH_FUNC, iv);
        ok = ok && iv[0] == (GLint)depth_funcs[i] && consume_error(GL_NO_ERROR);
    }
    expect_bool("all depth funcs", ok, 3);

    glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
    glGetIntegerv(GL_BLEND_SRC, &iv[0]);
    glGetIntegerv(GL_BLEND_DST, &iv[1]);
    ok = iv[0] == (GLint)GL_DST_COLOR && iv[1] == (GLint)GL_ONE_MINUS_SRC_COLOR;
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
    glEndList();
    glBlendFunc(GL_ONE, GL_ZERO);
    glCallList(list);
    glGetIntegerv(GL_BLEND_SRC, &iv[0]);
    glGetIntegerv(GL_BLEND_DST, &iv[1]);
    ok = ok && iv[0] == (GLint)GL_SRC_ALPHA_SATURATE && iv[1] == (GLint)GL_ONE && consume_error(GL_NO_ERROR);
    expect_bool("blend factor state", ok, 4);

    glGetIntegerv(GL_RED_BITS, &iv[0]);
    glGetIntegerv(GL_GREEN_BITS, &iv[1]);
    glGetIntegerv(GL_BLUE_BITS, &iv[2]);
    glGetIntegerv(GL_ALPHA_BITS, &iv[3]);
    glGetIntegerv(GL_DEPTH_BITS, &iv[4]);
    glGetIntegerv(GL_STENCIL_BITS, &iv[5]);
    ok = iv[0] == 8 && iv[1] == 8 && iv[2] == 8 && iv[3] == 8 && iv[4] == 24 && iv[5] == 8 && consume_error(GL_NO_ERROR);
    expect_bool("framebuffer bit queries", ok, 5);

    glGetBooleanv(GL_DOUBLEBUFFER, &bv[0]);
    glGetBooleanv(GL_STEREO, &bv[1]);
    glGetBooleanv(GL_RGBA_MODE, &bv[2]);
    glGetIntegerv(GL_AUX_BUFFERS, &iv[0]);
    glGetIntegerv(GL_DRAW_BUFFER, &iv[1]);
    glGetIntegerv(GL_READ_BUFFER, &iv[2]);
    ok = bv[0] == GL_TRUE && bv[1] == GL_FALSE && bv[2] == GL_TRUE &&
         iv[0] == 0 && iv[1] == (GLint)GL_BACK && iv[2] == (GLint)GL_BACK && consume_error(GL_NO_ERROR);
    expect_bool("buffer mode queries", ok, 6);

    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, iv);
    glGetIntegerv(GL_SUBPIXEL_BITS, &iv[2]);
    ok = iv[0] >= 640 && iv[1] >= 480 && iv[2] == 4 && consume_error(GL_NO_ERROR);
    expect_bool("viewport limits", ok, 7);

    glViewport(0, 0, 640, 480);
    glScissor(3, 4, 123, 234);
    glGetDoublev(GL_SCISSOR_BOX, dv);
    ok = dv[0] == 3.0 && dv[1] == 4.0 && dv[2] == 123.0 && dv[3] == 234.0 && consume_error(GL_NO_ERROR);
    expect_bool("scissor double query", ok, 8);

    glViewport(0, 0, -1, 480);
    ok = consume_error(GL_INVALID_VALUE);
    glDepthFunc(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_SRC_ALPHA_SATURATE);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glGetIntegerv(GL_TEXTURE_ENV, iv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("state validation", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static void draw_quad(float x, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.32f, 0.28f, 0.0f);
    glVertex3f(x + 0.32f, 0.28f, 0.0f);
    glVertex3f(x + 0.32f, -0.28f, 0.0f);
    glVertex3f(x - 0.32f, -0.28f, 0.0f);
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
    debugPrint("NXGL state query probe starting\n");

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
        draw_quad(-0.65f, 0.14f, 0.45f, 0.86f);
        draw_quad(0.0f, 0.90f, 0.72f, 0.16f);
        draw_quad(0.65f, 0.74f, 0.18f, 0.52f);
        for (int i = 0; i < 10; ++i) {
            draw_bar(-2.65f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL state query", all_passed() ? "all checks passed" : "state query check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
