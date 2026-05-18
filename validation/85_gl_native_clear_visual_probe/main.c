#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[6];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 10;
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
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
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

static void build_clear_pattern(void)
{
    reset_state();
    glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);
    glScissor(48, 250, 192, 150);
    glClearColor(0.85f, 0.10f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(248, 250, 144, 150);
    glClearColor(0.08f, 0.75f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glScissor(400, 250, 192, 150);
    glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(0.95f, 0.55f, 0.85f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glScissor(160, 80, 320, 100);
    glClearColor(0.15f, 0.28f, 0.85f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

static void run_probe(void)
{
    uint8_t pixel[4];
    bool ok;

    build_clear_pattern();
    read_color(80, 300, pixel);
    ok = pixel_rgb(pixel, 217, 25, 20) && consume_error(GL_NO_ERROR);
    expect_bool("left scissored clear", ok, 0);

    read_color(320, 300, pixel);
    ok = pixel_rgb(pixel, 20, 191, 30) && consume_error(GL_NO_ERROR);
    expect_bool("middle sequential clear", ok, 1);

    read_color(450, 300, pixel);
    ok = pixel_rgb(pixel, 5, 140, 217) && consume_error(GL_NO_ERROR);
    expect_bool("channel-masked clear", ok, 2);

    read_color(320, 120, pixel);
    ok = pixel_rgb(pixel, 38, 71, 217) && consume_error(GL_NO_ERROR);
    expect_bool("lower scissored clear", ok, 3);

    read_color(20, 20, pixel);
    ok = pixel_rgb(pixel, 5, 7, 12) && consume_error(GL_NO_ERROR);
    expect_bool("outside clear preserved", ok, 4);

    glClear(0x40000000u);
    ok = consume_error(GL_INVALID_VALUE);
    expect_bool("invalid clear mask", ok, 5);
}

static void draw_bar(float x, bool pass)
{
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
    glRectf(x - 0.18f, -1.42f, x + 0.18f, -1.68f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 6; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL native clear visual probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        build_clear_pattern();
        for (int i = 0; i < 6; ++i) {
            draw_bar(-1.55f + (float)i * 0.62f, results[i]);
        }
        nxglSwapBuffers("NXGL native clear visual", all_passed() ? "all checks passed" : "native clear check failed");
        Sleep(16);
    }
}
