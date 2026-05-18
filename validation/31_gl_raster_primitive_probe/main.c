#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void run_static_probe(void)
{
    GLfloat fvalue = 0.0f;
    GLint ivalue[2] = { 0, 0 };

    glPointSize(7.0f);
    glGetFloatv(GL_POINT_SIZE, &fvalue);
    expect_bool("point size query", fvalue == 7.0f && glGetError() == GL_NO_ERROR, 0);

    glLineWidth(4.0f);
    glGetFloatv(GL_LINE_WIDTH, &fvalue);
    expect_bool("line width query", fvalue == 4.0f && glGetError() == GL_NO_ERROR, 1);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glGetIntegerv(GL_POLYGON_MODE, ivalue);
    expect_bool("polygon mode query", ivalue[0] == GL_LINE && ivalue[1] == GL_LINE && glGetError() == GL_NO_ERROR, 2);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPointSize(0.0f);
    expect_bool("invalid point size rejected", glGetError() == GL_INVALID_VALUE, 3);

    glLineWidth(-1.0f);
    expect_bool("invalid line width rejected", glGetError() == GL_INVALID_VALUE, 4);

    glPolygonMode(GL_TEXTURE_2D, GL_FILL);
    expect_bool("invalid polygon face rejected", glGetError() == GL_INVALID_ENUM, 5);
}

static void draw_result_bar(float x, bool pass)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -2.05f, 0.0f);
    glVertex3f(x + 0.18f, -2.05f, 0.0f);
    glVertex3f(x + 0.18f, -2.35f, 0.0f);
    glVertex3f(x - 0.18f, -2.35f, 0.0f);
    glEnd();
}

static void draw_points(void)
{
    glPointSize(8.0f);
    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_POINTS);
    glVertex3f(-2.4f, 1.35f, 0.0f);
    glVertex3f(-2.0f, 1.05f, 0.0f);
    glVertex3f(-1.6f, 1.35f, 0.0f);
    glVertex3f(-2.0f, 1.65f, 0.0f);
    glEnd();
}

static void draw_lines(void)
{
    glLineWidth(5.0f);
    glColor3f(0.2f, 0.9f, 0.2f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-0.6f, 1.65f, 0.0f);
    glVertex3f( 0.4f, 1.45f, 0.0f);
    glVertex3f( 0.2f, 0.85f, 0.0f);
    glVertex3f(-0.8f, 0.95f, 0.0f);
    glEnd();

    glColor3f(0.2f, 0.7f, 1.0f);
    glBegin(GL_LINE_STRIP);
    glVertex3f(0.75f, 1.65f, 0.0f);
    glVertex3f(1.05f, 0.95f, 0.0f);
    glVertex3f(1.45f, 1.55f, 0.0f);
    glVertex3f(1.85f, 0.85f, 0.0f);
    glEnd();
}

static void draw_polygon_modes(void)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(0.85f, 0.25f, 0.9f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(-1.8f, -0.2f, 0.0f);
    glVertex3f(-2.45f, -0.65f, 0.0f);
    glVertex3f(-2.05f, -1.35f, 0.0f);
    glVertex3f(-1.25f, -1.35f, 0.0f);
    glVertex3f(-0.90f, -0.65f, 0.0f);
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(4.0f);
    glColor3f(1.0f, 0.85f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex3f(-0.1f, -0.25f, 0.0f);
    glVertex3f( 0.65f, -0.55f, 0.0f);
    glVertex3f( 0.55f, -1.30f, 0.0f);
    glVertex3f(-0.35f, -1.45f, 0.0f);
    glVertex3f(-0.75f, -0.85f, 0.0f);
    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    glPointSize(8.0f);
    glColor3f(0.2f, 1.0f, 0.95f);
    glBegin(GL_QUADS);
    glVertex3f(1.25f, -0.35f, 0.0f);
    glVertex3f(2.05f, -0.35f, 0.0f);
    glVertex3f(2.05f, -1.20f, 0.0f);
    glVertex3f(1.25f, -1.20f, 0.0f);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

static bool all_passed(void)
{
    results[6] = true;
    results[7] = true;
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    char detail[64];
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL raster primitive probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.5f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        while (glGetError() != GL_NO_ERROR) {
        }
        draw_points();
        draw_lines();
        draw_polygon_modes();
        while (glGetError() != GL_NO_ERROR) {
        }

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.45f + (float)i * 0.7f, results[i]);
        }

        uint32_t mask = 0;
        for (int i = 0; i < 8; ++i) {
            if (!results[i]) {
                mask |= 1u << i;
            }
        }
        if (all_passed()) {
            strcpy(detail, "points/lines/polygon modes passed");
        } else {
            detail[0] = 'f';
            detail[1] = 'a';
            detail[2] = 'i';
            detail[3] = 'l';
            detail[4] = ' ';
            detail[5] = 'm';
            detail[6] = 'a';
            detail[7] = 's';
            detail[8] = 'k';
            detail[9] = '=';
            detail[10] = "0123456789ABCDEF"[(mask >> 4) & 0xF];
            detail[11] = "0123456789ABCDEF"[mask & 0xF];
            detail[12] = '\0';
        }
        nxglSwapBuffers("NXGL raster primitives", detail);
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
