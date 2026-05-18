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
    return d <= 9;
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

static GLfloat cf(uint8_t v)
{
    return (GLfloat)v / 255.0f;
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
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glLogicOp(GL_COPY);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void clear_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(cf(r), cf(g), cf(b), 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void draw_quad_z(float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    glColor4f(cf(r), cf(g), cf(b), cf(a));
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, z);
    glVertex3f(0.55f, 0.55f, z);
    glVertex3f(0.55f, -0.55f, z);
    glVertex3f(-0.55f, -0.55f, z);
    glEnd();
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static bool logic_case(GLenum op, uint8_t dr, uint8_t dg, uint8_t db,
                       uint8_t sr, uint8_t sg, uint8_t sb,
                       uint8_t er, uint8_t eg, uint8_t eb)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    reset_state();
    clear_rgb(dr, dg, db);
    glDisable(GL_BLEND);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(op);
    draw_quad_z(0.0f, sr, sg, sb, 255);
    read_center(pixel);
    return pixel_rgb(pixel, er, eg, eb) && consume_error(GL_NO_ERROR);
}

static void run_probe(void)
{
    GLint iv[4] = { 0, 0, 0, 0 };
    GLfloat fv = 0.0f;
    GLboolean bv[4] = { 0, 0, 0, 0 };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &iv[0]);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv);
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    glGetIntegerv(GL_LOGIC_OP_MODE, &iv[1]);
    ok = iv[0] == (GLint)GL_ALWAYS && fv == 0.0f &&
         bv[0] == GL_TRUE && bv[1] == GL_TRUE && bv[2] == GL_TRUE && bv[3] == GL_TRUE &&
         iv[1] == (GLint)GL_COPY && consume_error(GL_NO_ERROR);
    expect_bool("default alpha logic state", ok, 0);

    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_LESS, 0.5f);
    draw_quad_z(0.0f, 230, 20, 20, 64);
    read_center(pixel);
    ok = pixel_rgb(pixel, 230, 20, 20) && consume_error(GL_NO_ERROR);
    clear_rgb(0, 0, 0);
    glAlphaFunc(GL_GEQUAL, 0.5f);
    draw_quad_z(0.0f, 20, 220, 40, 128);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 20, 220, 40) && consume_error(GL_NO_ERROR);
    expect_bool("alpha compare pass funcs", ok, 1);

    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_NEVER, 0.0f);
    draw_quad_z(0.0f, 230, 20, 20, 255);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    draw_quad_z(0.0f, 20, 220, 40, 0);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 20, 220, 40) && consume_error(GL_NO_ERROR);
    expect_bool("alpha never always", ok, 2);

    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glAlphaFunc(GL_GREATER, 0.5f);
    draw_quad_z(0.0f, 230, 20, 20, 64);
    draw_quad_z(-1.0f, 20, 220, 40, 255);
    read_center(pixel);
    ok = pixel_rgb(pixel, 20, 220, 40) && consume_error(GL_NO_ERROR);
    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glAlphaFunc(GL_GREATER, 0.5f);
    draw_quad_z(0.0f, 230, 20, 20, 255);
    draw_quad_z(-1.0f, 20, 220, 40, 255);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 230, 20, 20) && consume_error(GL_NO_ERROR);
    expect_bool("alpha depth write interaction", ok, 3);

    reset_state();
    clear_rgb(20, 80, 140);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    draw_quad_z(0.0f, 230, 200, 30, 255);
    read_center(pixel);
    ok = pixel_rgb(pixel, 20, 200, 140) && consume_error(GL_NO_ERROR);
    clear_rgb(20, 80, 140);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
    draw_quad_z(0.0f, 230, 200, 30, 255);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 230, 80, 30) && consume_error(GL_NO_ERROR);
    expect_bool("color mask primitive channels", ok, 4);

    ok = logic_case(GL_NOOP, 0x12, 0x34, 0x56, 0xff, 0xff, 0xff, 0x12, 0x34, 0x56) &&
         logic_case(GL_CLEAR, 0x12, 0x34, 0x56, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00) &&
         logic_case(GL_SET, 0x12, 0x34, 0x56, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff);
    expect_bool("logic noop clear set", ok, 5);

    ok = logic_case(GL_XOR, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00) &&
         logic_case(GL_INVERT, 0x12, 0x34, 0x56, 0x00, 0x00, 0x00, 0xed, 0xcb, 0xa9);
    expect_bool("logic xor invert", ok, 6);

    ok = logic_case(GL_AND, 0xf0, 0x0f, 0xaa, 0xcc, 0x33, 0x55, 0xc0, 0x03, 0x00) &&
         logic_case(GL_OR, 0x12, 0x34, 0x56, 0x80, 0x0f, 0xa0, 0x92, 0x3f, 0xf6) &&
         logic_case(GL_COPY_INVERTED, 0x12, 0x34, 0x56, 0x0f, 0x33, 0x55, 0xf0, 0xcc, 0xaa);
    expect_bool("logic and or copy inverted", ok, 7);

    reset_state();
    clear_rgb(0, 0, 255);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    draw_quad_z(0.0f, 255, 255, 255, 64);
    read_center(pixel);
    ok = pixel_rgb(pixel, 255, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("logic op ignores blend path", ok, 8);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.25f);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_OR);
    glEndList();
    glCallList(list);
    glDeleteLists(list, 1);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &iv[0]);
    glGetFloatv(GL_ALPHA_TEST_REF, &fv);
    glGetBooleanv(GL_COLOR_WRITEMASK, bv);
    glGetIntegerv(GL_LOGIC_OP_MODE, &iv[1]);
    ok = glIsEnabled(GL_ALPHA_TEST) == GL_TRUE &&
         glIsEnabled(GL_COLOR_LOGIC_OP) == GL_TRUE &&
         iv[0] == (GLint)GL_GEQUAL && fv > 0.249f && fv < 0.251f &&
         bv[0] == GL_FALSE && bv[1] == GL_TRUE && bv[2] == GL_FALSE &&
         iv[1] == (GLint)GL_OR && consume_error(GL_NO_ERROR);
    expect_bool("display-list alpha logic state", ok, 9);

    reset_state();
    glAlphaFunc(GL_TEXTURE_2D, 0.0f);
    ok = consume_error(GL_INVALID_ENUM);
    glLogicOp(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glBegin(GL_TRIANGLES);
    glAlphaFunc(GL_ALWAYS, 0.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLogicOp(GL_XOR);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glColor3f(0.2f, 0.7f, 0.9f);
    glVertex3f(-0.2f, -0.2f, 0.0f);
    glVertex3f(0.2f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.2f, 0.0f);
    glEnd();
    expect_bool("alpha logic validation", ok && consume_error(GL_NO_ERROR), 10);

    reset_state();
    clear_rgb(0, 0, 0);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_EQUAL, cf(128));
    draw_quad_z(0.0f, 200, 80, 20, 128);
    read_center(pixel);
    ok = pixel_rgb(pixel, 200, 80, 20) && consume_error(GL_NO_ERROR);
    glAlphaFunc(GL_NOTEQUAL, cf(128));
    draw_quad_z(0.0f, 20, 200, 80, 128);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 200, 80, 20) && consume_error(GL_NO_ERROR);
    expect_bool("alpha equal notequal", ok, 11);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
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
    for (int i = 0; i < 12; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL alpha logic deep probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_rgb(4, 4, 10);
        draw_quad_z(0.0f, all_passed() ? 30 : 160, all_passed() ? 120 : 20, all_passed() ? 230 : 20, 255);
        for (int i = 0; i < 12; ++i) {
            draw_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL alpha logic deep", all_passed() ? "all checks passed" : "alpha logic check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
