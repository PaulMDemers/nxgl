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

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
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
    glStencilMask(0xff);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void draw_quad(float z, float r, float g, float b)
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

static uint8_t read_center_stencil(void)
{
    uint8_t value = 0;
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &value);
    return value;
}

static void clear_frame(uint8_t stencil, float depth)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(depth);
    glClearStencil(stencil);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static bool stencil_color_case(GLenum func, GLint ref, GLuint mask, uint8_t initial, bool expect_draw)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    reset_state();
    clear_frame(initial, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(func, ref, mask);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    draw_quad(0.0f, 0.1f, 0.8f, 0.2f);
    read_center_color(pixel);
    return pixel_rgb(pixel, expect_draw ? 26 : 0, expect_draw ? 204 : 0, expect_draw ? 51 : 0);
}

static void run_probe(void)
{
    GLint iv[4] = { 0, 0, 0, 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    uint8_t stencil = 0;
    GLuint list = 0;
    bool ok;

    reset_state();
    glGetIntegerv(GL_STENCIL_FUNC, &iv[0]);
    glGetIntegerv(GL_STENCIL_REF, &iv[1]);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &iv[2]);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &iv[3]);
    ok = iv[0] == (GLint)GL_ALWAYS && iv[1] == 0 && iv[2] == 0xff && iv[3] == 0xff && consume_error(GL_NO_ERROR);
    expect_bool("default stencil state query", ok, 0);

    reset_state();
    clear_frame(0, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 7, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad(0.0f, 0.9f, 0.8f, 0.1f);
    stencil = read_center_stencil();
    ok = stencil == 7 && consume_error(GL_NO_ERROR);
    expect_bool("zpass replace writes ref", ok, 1);

    reset_state();
    clear_frame(0xa5, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0x0f);
    glStencilFunc(GL_ALWAYS, 0x3c, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    draw_quad(0.0f, 0.8f, 0.7f, 0.1f);
    stencil = read_center_stencil();
    ok = stencil == 0xac && consume_error(GL_NO_ERROR);
    expect_bool("stencil write mask merges", ok, 2);

    reset_state();
    clear_frame(1, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 2, 0xff);
    glStencilOp(GL_INCR, GL_KEEP, GL_KEEP);
    draw_quad(0.0f, 0.9f, 0.1f, 0.1f);
    read_center_color(pixel);
    stencil = read_center_stencil();
    ok = stencil == 2 && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("stencil fail op blocks color", ok, 3);

    reset_state();
    clear_frame(10, 0.25f);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
    draw_quad(0.0f, 0.9f, 0.1f, 0.1f);
    read_center_color(pixel);
    stencil = read_center_stencil();
    ok = stencil == 11 && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("depth fail stencil op", ok, 4);

    reset_state();
    clear_frame(12, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    draw_quad(0.0f, 0.1f, 0.6f, 0.9f);
    read_center_color(pixel);
    stencil = read_center_stencil();
    ok = stencil == 11 && pixel_rgb(pixel, 26, 153, 230) && consume_error(GL_NO_ERROR);
    expect_bool("depth pass stencil op", ok, 5);

    ok = stencil_color_case(GL_LESS, 4, 0xff, 5, true)
      && stencil_color_case(GL_LESS, 6, 0xff, 5, false)
      && stencil_color_case(GL_GREATER, 6, 0xff, 5, true)
      && stencil_color_case(GL_GREATER, 4, 0xff, 5, false)
      && stencil_color_case(GL_LEQUAL, 5, 0xff, 5, true)
      && stencil_color_case(GL_GEQUAL, 5, 0xff, 5, true)
      && consume_error(GL_NO_ERROR);
    expect_bool("ref func stencil ordering", ok, 6);

    ok = stencil_color_case(GL_EQUAL, 0x05, 0x0f, 0xa5, true)
      && stencil_color_case(GL_NOTEQUAL, 0x05, 0x0f, 0xa5, false)
      && stencil_color_case(GL_NEVER, 0x05, 0xff, 0x05, false)
      && stencil_color_case(GL_ALWAYS, 0x05, 0xff, 0x05, true)
      && consume_error(GL_NO_ERROR);
    expect_bool("stencil func mask variants", ok, 7);

    reset_state();
    clear_frame(0x0f, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
    draw_quad(0.0f, 0.4f, 0.4f, 0.4f);
    stencil = read_center_stencil();
    ok = stencil == 0xf0 && consume_error(GL_NO_ERROR);
    expect_bool("invert stencil op", ok, 8);

    reset_state();
    clear_frame(0xff, 1.0f);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    draw_quad(0.0f, 0.4f, 0.4f, 0.4f);
    ok = read_center_stencil() == 0xff;
    clear_frame(0x00, 1.0f);
    draw_quad(0.0f, 0.4f, 0.4f, 0.4f);
    ok = ok && read_center_stencil() == 0x01;
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    draw_quad(0.0f, 0.4f, 0.4f, 0.4f);
    ok = ok && read_center_stencil() == 0x00 && consume_error(GL_NO_ERROR);
    expect_bool("stencil incr decr clamp", ok, 9);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glStencilMask(0x0f);
    glStencilFunc(GL_EQUAL, 0x05, 0x0f);
    glStencilOp(GL_INCR, GL_DECR, GL_REPLACE);
    glEndList();
    reset_state();
    glCallList(list);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &iv[0]);
    glGetIntegerv(GL_STENCIL_FUNC, &iv[1]);
    glGetIntegerv(GL_STENCIL_REF, &iv[2]);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &iv[3]);
    ok = iv[0] == 0x0f && iv[1] == (GLint)GL_EQUAL && iv[2] == 0x05 && iv[3] == 0x0f;
    glGetIntegerv(GL_STENCIL_FAIL, &iv[0]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &iv[1]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &iv[2]);
    glDeleteLists(list, 1);
    ok = ok && iv[0] == (GLint)GL_INCR && iv[1] == (GLint)GL_DECR && iv[2] == (GLint)GL_REPLACE && consume_error(GL_NO_ERROR);
    expect_bool("display-list stencil state", ok, 10);

    reset_state();
    glBegin(GL_TRIANGLES);
    glStencilFunc(GL_ALWAYS, 1, 0xff);
    ok = consume_error(GL_INVALID_OPERATION);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glStencilMask(0x0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColor3f(0.2f, 0.7f, 0.9f);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("stencil ops rejected in begin", ok && consume_error(GL_NO_ERROR), 11);
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
    debugPrint("NXGL stencil ops probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_frame(0, 1.0f);
        draw_quad(0.0f, all_passed() ? 0.08f : 0.45f, all_passed() ? 0.44f : 0.06f, all_passed() ? 0.88f : 0.06f);
        glDisable(GL_DEPTH_TEST);
        for (int i = 0; i < 12; ++i) {
            draw_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL stencil ops", all_passed() ? "all checks passed" : "stencil check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
