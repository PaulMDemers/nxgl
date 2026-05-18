#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[11];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void run_static_probe(void)
{
    GLfloat values[4];
    GLfloat color[4] = { 0.7f, 0.2f, 0.1f, 1.0f };
    GLfloat light_position[4] = { 0.2f, 0.8f, 1.0f, 0.0f };
    GLfloat light_color[4] = { 0.9f, 0.9f, 0.85f, 1.0f };
    GLint ivalue = 0;

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    expect_bool("lighting and light0 enable", glIsEnabled(GL_LIGHTING) == GL_TRUE && glIsEnabled(GL_LIGHT0) == GL_TRUE && glGetError() == GL_NO_ERROR, 0);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glGetFloatv(GL_CURRENT_NORMAL, values);
    expect_bool("current normal query", values[0] == 0.0f && values[1] == 0.0f && values[2] == 1.0f && glGetError() == GL_NO_ERROR, 1);

    glShadeModel(GL_FLAT);
    glGetIntegerv(GL_SHADE_MODEL, &ivalue);
    expect_bool("shade model query", ivalue == GL_FLAT && glGetError() == GL_NO_ERROR, 2);
    glShadeModel(GL_SMOOTH);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
    expect_bool("light vector setup", glGetError() == GL_NO_ERROR, 3);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);
    expect_bool("material setup", glGetError() == GL_NO_ERROR, 4);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &ivalue);
    expect_bool("color material state", glIsEnabled(GL_COLOR_MATERIAL) == GL_TRUE && ivalue == GL_AMBIENT_AND_DIFFUSE && glGetError() == GL_NO_ERROR, 5);

    glEnable(GL_RESCALE_NORMAL);
    expect_bool("rescale normal state", glIsEnabled(GL_RESCALE_NORMAL) == GL_TRUE && glGetError() == GL_NO_ERROR, 6);
    glDisable(GL_RESCALE_NORMAL);

    glEnable(GL_LIGHT7);
    glDisable(GL_LIGHT7);
    expect_bool("light7 toggle", glIsEnabled(GL_LIGHT7) == GL_FALSE && glGetError() == GL_NO_ERROR, 7);

    glEnable(GL_LIGHT0 - 1);
    expect_bool("invalid light rejected", glGetError() == GL_INVALID_ENUM, 8);

    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 129.0f);
    expect_bool("invalid shininess rejected", glGetError() == GL_INVALID_VALUE, 9);
}

static void run_render_probe(void)
{
    GLubyte pixel[4] = { 0, 0, 0, 0 };
    GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_x[4] = { 1.0f, 0.0f, 0.0f, 0.0f };

    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -0.75, 0.75, -10.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, one);
    glLightfv(GL_LIGHT0, GL_POSITION, light_x);
    glColor3f(1.0f, 1.0f, 1.0f);

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-0.35f, 0.35f, 0.0f);
    glVertex3f(0.35f, 0.35f, 0.0f);
    glVertex3f(0.35f, -0.35f, 0.0f);
    glVertex3f(-0.35f, -0.35f, 0.0f);
    glEnd();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);

    expect_bool("rotated normal follows modelview", pixel[0] > 180 && pixel[1] > 180 && pixel[2] > 180 && glGetError() == GL_NO_ERROR, 10);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

static void draw_lit_quad(float x, float y, float z, float nx, float ny, float nz, const GLfloat material[4])
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
    glNormal3f(nx, ny, nz);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.55f, y + 0.45f, z);
    glVertex3f(x + 0.55f, y + 0.45f, z);
    glVertex3f(x + 0.55f, y - 0.45f, z);
    glVertex3f(x - 0.55f, y - 0.45f, z);
    glEnd();
}

static void draw_smooth_triangle(void)
{
    glShadeModel(GL_SMOOTH);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glBegin(GL_TRIANGLES);
    glColor3f(0.95f, 0.20f, 0.18f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.45f, -0.10f, 0.0f);
    glColor3f(0.15f, 0.85f, 0.25f);
    glNormal3f(0.0f, 0.7f, 0.35f);
    glVertex3f(0.65f, -0.10f, 0.0f);
    glColor3f(0.20f, 0.35f, 1.0f);
    glNormal3f(0.0f, -0.3f, 0.2f);
    glVertex3f(0.10f, 0.95f, 0.0f);
    glEnd();
    glDisable(GL_COLOR_MATERIAL);
}

static void draw_flat_triangle(void)
{
    GLfloat material[4] = { 0.95f, 0.75f, 0.15f, 1.0f };
    glShadeModel(GL_FLAT);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(1.05f, -0.05f, 0.0f);
    glNormal3f(0.0f, 0.7f, 0.15f);
    glVertex3f(2.10f, -0.05f, 0.0f);
    glNormal3f(0.0f, -0.8f, 0.10f);
    glVertex3f(1.58f, 0.90f, 0.0f);
    glEnd();
    glShadeModel(GL_SMOOTH);
}

static bool all_passed(void)
{
    for (int i = 0; i < 11; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    char detail[64];
    GLfloat light_position[4] = { -0.4f, 0.7f, 1.0f, 0.0f };
    GLfloat light_diffuse[4] = { 0.95f, 0.95f, 0.9f, 1.0f };
    GLfloat light_ambient[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
    GLfloat red[4] = { 0.95f, 0.16f, 0.12f, 1.0f };
    GLfloat teal[4] = { 0.10f, 0.85f, 0.95f, 1.0f };
    GLfloat violet[4] = { 0.75f, 0.35f, 0.95f, 1.0f };

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL lighting probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();
    run_render_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_NORMALIZE);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
        glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);

        draw_lit_quad(-1.85f, 1.05f, 0.0f, 0.0f, 0.0f, 1.0f, red);
        draw_lit_quad(-0.35f, 1.05f, 0.0f, 0.0f, 0.45f, 0.25f, teal);
        draw_lit_quad(1.15f, 1.05f, 0.0f, 0.0f, -0.65f, 0.18f, violet);
        draw_smooth_triangle();
        draw_flat_triangle();

        for (int i = 0; i < 11; ++i) {
            draw_result_bar(-2.85f + (float)i * 0.52f, results[i]);
        }

        if (all_passed()) {
            strcpy(detail, "lighting/material checks passed");
        } else {
            strcpy(detail, "one or more lighting checks failed");
        }
        nxglSwapBuffers("NXGL lighting", detail);
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
