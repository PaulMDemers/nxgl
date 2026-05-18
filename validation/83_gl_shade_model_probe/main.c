#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 16;
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

static void reset_draw_state(void)
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
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_color_triangle(float x)
{
    glBegin(GL_TRIANGLES);
    glColor3f(0.95f, 0.10f, 0.10f);
    glVertex3f(x - 0.55f, -0.45f, 0.0f);
    glColor3f(0.10f, 0.90f, 0.15f);
    glVertex3f(x + 0.55f, -0.45f, 0.0f);
    glColor3f(0.15f, 0.25f, 0.95f);
    glVertex3f(x, 0.55f, 0.0f);
    glEnd();
}

static GLuint build_flat_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glShadeModel(GL_FLAT);
    draw_color_triangle(0.0f);
    glEndList();
    return list;
}

static void run_probe(void)
{
    GLint ivalue = 0;
    uint8_t pixel[4];
    GLuint list;
    bool ok;

    reset_draw_state();
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("default shade smooth", ivalue == GL_SMOOTH && glGetError() == GL_NO_ERROR, 0);

    glShadeModel(GL_TEXTURE_2D);
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("invalid shade enum rejected", ivalue == GL_SMOOTH && glGetError() == GL_INVALID_ENUM, 1);

    glShadeModel(GL_FLAT);
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("direct flat query", ivalue == GL_FLAT && glGetError() == GL_NO_ERROR, 2);

    glPushAttrib(GL_LIGHTING_BIT);
    glShadeModel(GL_SMOOTH);
    glPopAttrib();
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("attrib restores shade", ivalue == GL_FLAT && glGetError() == GL_NO_ERROR, 3);

    glShadeModel(GL_SMOOTH);
    list = build_flat_list();
    glCallList(list);
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("display list replays shade", ivalue == GL_FLAT && glGetError() == GL_NO_ERROR, 4);
    glDeleteLists(list, 1);

    reset_draw_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glShadeModel(GL_FLAT);
    draw_color_triangle(0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 38, 64, 242) && glGetError() == GL_NO_ERROR;
    expect_bool("flat triangle last color", ok, 5);

    reset_draw_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    list = build_flat_list();
    glShadeModel(GL_SMOOTH);
    glCallList(list);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 38, 64, 242) && glGetError() == GL_NO_ERROR;
    expect_bool("listed flat triangle color", ok, 6);
    glDeleteLists(list, 1);

    glShadeModel(GL_SMOOTH);
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("smooth restore query", ivalue == GL_SMOOTH && glGetError() == GL_NO_ERROR, 7);
}

static void draw_bar(float x, bool pass)
{
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
    debugPrint("NXGL shade-model probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_draw_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glShadeModel(GL_SMOOTH);
        draw_color_triangle(-1.15f);
        glShadeModel(GL_FLAT);
        draw_color_triangle(1.15f);
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL shade model", all_passed() ? "all checks passed" : "shade-model check failed");
        Sleep(16);
    }
}
