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
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void clear_rgb(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_quad_ccw(float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, -0.55f, 0.0f);
    glVertex3f(0.55f, -0.55f, 0.0f);
    glVertex3f(0.55f, 0.55f, 0.0f);
    glVertex3f(-0.55f, 0.55f, 0.0f);
    glEnd();
}

static void draw_quad_cw(float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, -0.55f, 0.0f);
    glVertex3f(-0.55f, -0.55f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    GLint iv[4] = { 0, 0, 0, 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_BLEND_SRC, &iv[0]);
    glGetIntegerv(GL_BLEND_DST, &iv[1]);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[2]);
    glGetIntegerv(GL_FRONT_FACE, &iv[3]);
    ok = glIsEnabled(GL_BLEND) == GL_FALSE &&
         glIsEnabled(GL_CULL_FACE) == GL_FALSE &&
         iv[0] == (GLint)GL_SRC_ALPHA &&
         iv[1] == (GLint)GL_ONE_MINUS_SRC_ALPHA &&
         iv[2] == (GLint)GL_BACK &&
         iv[3] == (GLint)GL_CCW &&
         consume_error(GL_NO_ERROR);
    expect_bool("default blend cull state", ok, 0);

    reset_state();
    clear_rgb(0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_quad_ccw(1.0f, 0.0f, 0.0f, 0.25f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 63, 0, 191) && consume_error(GL_NO_ERROR);
    expect_bool("src alpha blend readback", ok, 1);

    reset_state();
    clear_rgb(0.2f, 0.3f, 0.4f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    draw_quad_ccw(0.5f, 0.5f, 0.7f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 178, 204, 255) && consume_error(GL_NO_ERROR);
    expect_bool("additive blend clamp", ok, 2);

    reset_state();
    clear_rgb(0.2f, 0.3f, 0.4f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE);
    draw_quad_ccw(0.9f, 0.1f, 0.1f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 51, 76, 102) && consume_error(GL_NO_ERROR);
    expect_bool("zero one preserves dst", ok, 3);

    reset_state();
    clear_rgb(0.25f, 0.5f, 0.75f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    draw_quad_ccw(0.8f, 0.4f, 0.2f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 51, 51, 38) && consume_error(GL_NO_ERROR);
    expect_bool("dst color source factor", ok, 4);

    reset_state();
    clear_rgb(0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
    draw_quad_ccw(1.0f, 0.0f, 0.0f, 0.25f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 191, 0, 63) && consume_error(GL_NO_ERROR);
    expect_bool("reverse alpha factors", ok, 5);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    draw_quad_ccw(0.0f, 0.8f, 0.0f, 1.0f);
    draw_quad_cw(0.9f, 0.0f, 0.0f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 204, 0) && consume_error(GL_NO_ERROR);
    expect_bool("back-face cull default winding", ok, 6);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    draw_quad_cw(0.8f, 0.0f, 0.0f, 1.0f);
    draw_quad_ccw(0.0f, 0.8f, 0.0f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 204, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("front-face winding flips", ok, 7);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
    draw_quad_ccw(0.9f, 0.0f, 0.0f, 1.0f);
    draw_quad_cw(0.0f, 0.7f, 0.0f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 178, 0) && consume_error(GL_NO_ERROR);
    expect_bool("front-face cull leaves back", ok, 8);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT_AND_BACK);
    draw_quad_ccw(0.9f, 0.9f, 0.0f, 1.0f);
    draw_quad_cw(0.0f, 0.9f, 0.9f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("front and back culls all", ok, 9);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CW);
    glEndList();
    reset_state();
    glCallList(list);
    glGetIntegerv(GL_BLEND_SRC, &iv[0]);
    glGetIntegerv(GL_BLEND_DST, &iv[1]);
    glGetIntegerv(GL_CULL_FACE_MODE, &iv[2]);
    glGetIntegerv(GL_FRONT_FACE, &iv[3]);
    glDeleteLists(list, 1);
    ok = glIsEnabled(GL_BLEND) == GL_TRUE &&
         glIsEnabled(GL_CULL_FACE) == GL_TRUE &&
         iv[0] == (GLint)GL_ONE &&
         iv[1] == (GLint)GL_ONE &&
         iv[2] == (GLint)GL_FRONT &&
         iv[3] == (GLint)GL_CW &&
         consume_error(GL_NO_ERROR);
    expect_bool("display-list blend cull replay", ok, 10);

    reset_state();
    glBlendFunc(GL_TEXTURE_2D, GL_ONE);
    ok = consume_error(GL_INVALID_ENUM);
    glBlendFunc(GL_ONE, GL_SRC_ALPHA_SATURATE);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glCullFace(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glFrontFace(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glBegin(GL_TRIANGLES);
    glEnable(GL_BLEND);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glDisable(GL_CULL_FACE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glBlendFunc(GL_ONE, GL_ONE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glCullFace(GL_BACK);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glFrontFace(GL_CCW);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColor3f(0.2f, 0.7f, 0.9f);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("blend cull validation", ok && consume_error(GL_NO_ERROR), 11);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
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
    debugPrint("NXGL blend/cull probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_rgb(0.02f, 0.02f, 0.04f);
        draw_quad_ccw(all_passed() ? 0.05f : 0.45f, all_passed() ? 0.45f : 0.08f, all_passed() ? 0.9f : 0.08f, 1.0f);
        for (int i = 0; i < 12; ++i) {
            draw_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL blend/cull", all_passed() ? "all checks passed" : "blend/cull check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
