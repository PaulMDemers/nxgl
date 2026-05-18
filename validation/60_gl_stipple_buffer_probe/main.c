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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 8;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b) && p[3] == 255;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
    glLineWidth(1.0f);
    glLineStipple(1, 0xffff);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_center_quad(float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, -0.55f, 0.0f);
    glVertex3f(-0.55f, -0.55f, 0.0f);
    glEnd();
}

static void draw_center_line(float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    glVertex3f(-0.75f, 0.0f, 0.0f);
    glVertex3f(0.75f, 0.0f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    GLubyte zeros[128];
    GLubyte ones[128];
    GLubyte queried[128];
    GLint iv[4] = { 0, 0, 0, 0 };
    GLboolean bv[2] = { GL_FALSE, GL_FALSE };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    memset(zeros, 0x00, sizeof(zeros));
    memset(ones, 0xff, sizeof(ones));

    reset_state();
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &iv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &iv[1]);
    glGetBooleanv(GL_LINE_STIPPLE, &bv[0]);
    glGetBooleanv(GL_POLYGON_STIPPLE, &bv[1]);
    ok = iv[0] == 1 && iv[1] == 0xffff && bv[0] == GL_FALSE && bv[1] == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("default stipple state", ok, 0);

    glLineStipple(3, 0x00ff);
    glEnable(GL_LINE_STIPPLE);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &iv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &iv[1]);
    glGetBooleanv(GL_LINE_STIPPLE, &bv[0]);
    ok = iv[0] == 3 && iv[1] == 0x00ff && bv[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("line stipple query", ok, 1);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glLineStipple(4, 0x0f0f);
    glEnable(GL_POLYGON_STIPPLE);
    glEndList();
    glLineStipple(1, 0xffff);
    glDisable(GL_POLYGON_STIPPLE);
    glCallList(list);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, &iv[0]);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, &iv[1]);
    glGetBooleanv(GL_POLYGON_STIPPLE, &bv[0]);
    ok = iv[0] == 4 && iv[1] == 0x0f0f && bv[0] == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("stipple display list", ok, 2);

    glPolygonStipple(zeros);
    glGetPolygonStipple(queried);
    ok = memcmp(queried, zeros, sizeof(zeros)) == 0 && consume_error(GL_NO_ERROR);
    glPolygonStipple(ones);
    glGetPolygonStipple(queried);
    ok = ok && memcmp(queried, ones, sizeof(ones)) == 0 && consume_error(GL_NO_ERROR);
    expect_bool("polygon stipple get", ok, 3);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(8.0f);
    glLineStipple(1, 0x0000);
    glEnable(GL_LINE_STIPPLE);
    draw_center_line(1.0f, 0.0f, 0.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("line stipple suppress", ok, 4);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLineWidth(8.0f);
    glLineStipple(1, 0xffff);
    glEnable(GL_LINE_STIPPLE);
    draw_center_line(0.0f, 1.0f, 0.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("line stipple draw", ok, 5);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonStipple(zeros);
    glEnable(GL_POLYGON_STIPPLE);
    draw_center_quad(0.0f, 1.0f, 0.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    glPolygonStipple(ones);
    draw_center_quad(0.0f, 1.0f, 0.0f);
    read_center(pixel);
    ok = ok && pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("polygon stipple shadow", ok, 6);

    reset_state();
    glDrawBuffer(GL_FRONT_AND_BACK);
    glReadBuffer(GL_FRONT);
    glGetIntegerv(GL_DRAW_BUFFER, &iv[0]);
    glGetIntegerv(GL_READ_BUFFER, &iv[1]);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_BACK_LEFT);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_DRAW_BUFFER, &iv[2]);
    glGetIntegerv(GL_READ_BUFFER, &iv[3]);
    ok = iv[0] == (GLint)GL_FRONT_AND_BACK && iv[1] == (GLint)GL_FRONT &&
         iv[2] == (GLint)GL_NONE && iv[3] == (GLint)GL_BACK_LEFT && consume_error(GL_NO_ERROR);
    expect_bool("draw read buffer list", ok, 7);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawBuffer(GL_NONE);
    draw_center_quad(1.0f, 0.0f, 0.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("draw buffer none", ok, 8);

    glLineStipple(0, 0xffff);
    ok = consume_error(GL_INVALID_VALUE);
    glPolygonStipple(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetPolygonStipple(NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glDrawBuffer(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glReadBuffer(GL_FRONT_AND_BACK);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("stipple buffer validation", ok, 9);
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
    debugPrint("NXGL stipple/buffer probe starting\n");

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
        nxglSwapBuffers("NXGL stipple/buffer", all_passed() ? "all checks passed" : "stipple/buffer check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
