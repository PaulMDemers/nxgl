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

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 10;
}

static bool pixel_is_green(const uint8_t p[4])
{
    return near_byte(p[0], 25) && near_byte(p[1], 230) && near_byte(p[2], 25);
}

static bool pixel_is_red(const uint8_t p[4])
{
    return near_byte(p[0], 230) && near_byte(p[1], 25) && near_byte(p[2], 25);
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
    glEnable(GL_DEPTH_TEST);
    glDepthRange(0.0f, 1.0f);
    glDepthMask(GL_TRUE);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
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

static bool run_depth_case(GLenum func, float candidate_z, bool should_pass)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_ALWAYS);
    draw_quad(0.0f, 0.9f, 0.1f, 0.1f);
    glDepthFunc(func);
    draw_quad(candidate_z, 0.1f, 0.9f, 0.1f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return (should_pass ? pixel_is_green(pixel) : pixel_is_red(pixel)) && consume_error(GL_NO_ERROR);
}

static void run_probe(void)
{
    GLint iv;
    bool ok;

    ok = run_depth_case(GL_NEVER, 1.0f, false);
    expect_bool("depth func never", ok, 0);

    ok = run_depth_case(GL_LESS, 1.0f, true);
    expect_bool("depth func less", ok, 1);

    ok = run_depth_case(GL_EQUAL, 0.0f, true);
    expect_bool("depth func equal", ok, 2);

    ok = run_depth_case(GL_LEQUAL, 0.0f, true);
    expect_bool("depth func lequal", ok, 3);

    ok = run_depth_case(GL_GREATER, -1.0f, true);
    expect_bool("depth func greater", ok, 4);

    ok = run_depth_case(GL_NOTEQUAL, -1.0f, true);
    expect_bool("depth func notequal", ok, 5);

    ok = run_depth_case(GL_GEQUAL, 0.0f, true);
    expect_bool("depth func gequal", ok, 6);

    ok = run_depth_case(GL_ALWAYS, -1.0f, true);
    expect_bool("depth func always", ok, 7);

    reset_state();
    glDepthFunc(GL_GREATER);
    glGetIntegerv(GL_DEPTH_FUNC, &iv);
    ok = iv == (GLint)GL_GREATER && consume_error(GL_NO_ERROR);
    expect_bool("depth func query", ok, 8);

    glDepthFunc(GL_TEXTURE_2D);
    ok = consume_error(GL_INVALID_ENUM);
    expect_bool("depth func validation", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
    glRectf(x - 0.14f, -1.42f, x + 0.14f, -1.68f);
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
    debugPrint("NXGL depth func probe starting\n");

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
        for (int i = 0; i < 10; ++i) {
            draw_bar(-2.65f + (float)i * 0.58f, results[i]);
        }
        nxglSwapBuffers("NXGL depth funcs", all_passed() ? "all checks passed" : "depth func check failed");
        Sleep(16);
    }
}
