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
    return d <= 8;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.002f;
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
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat depth_inside = 0.0f;
    GLfloat depth_outside = 0.0f;
    uint8_t stencil_inside = 0;
    uint8_t stencil_outside = 0;
    bool ok;

    reset_state();
    glClearColor(0.06f, 0.13f, 0.19f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 33, 48) && consume_error(GL_NO_ERROR);
    expect_bool("color-mask clear channels", ok, 0);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClearColor(0.0f, 0.9f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 33, 48) && consume_error(GL_NO_ERROR);
    expect_bool("all-masked color clear", ok, 1);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glClearColor(0.8f, 0.2f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 204, 51, 25) && consume_error(GL_NO_ERROR);
    expect_bool("scissored color clear inside", ok, 2);
    read_color(100, 100, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("scissored color clear outside", ok, 3);

    reset_state();
    glClearDepth(0.25f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_FALSE);
    glClearDepth(0.75f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_inside);
    ok = near_float(depth_inside, 0.25f) && consume_error(GL_NO_ERROR);
    expect_bool("depth-mask clear preserves", ok, 4);

    reset_state();
    glClearDepth(0.20f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glClearDepth(0.80f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_inside);
    glReadPixels(100, 100, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth_outside);
    ok = near_float(depth_inside, 0.80f) && near_float(depth_outside, 0.20f) && consume_error(GL_NO_ERROR);
    expect_bool("scissored depth clear", ok, 5);

    reset_state();
    glClearStencil(0x5a);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x0f);
    glClearStencil(0xa5);
    glClear(GL_STENCIL_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_inside);
    ok = stencil_inside == 0x55 && consume_error(GL_NO_ERROR);
    expect_bool("stencil-mask clear", ok, 6);

    reset_state();
    glClearStencil(1);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glClearStencil(7);
    glClear(GL_STENCIL_BUFFER_BIT);
    glReadPixels(320, 240, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_inside);
    glReadPixels(100, 100, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil_outside);
    ok = stencil_inside == 7 && stencil_outside == 1 && consume_error(GL_NO_ERROR);
    glClear(0x80000000u);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("scissored stencil and invalid mask", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_ALPHA_TEST);
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
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
    debugPrint("NXGL clear mask/scissor probe starting\n");

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
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL clear masks/scissor", all_passed() ? "all checks passed" : "clear mask/scissor check failed");
        Sleep(16);
    }
}
