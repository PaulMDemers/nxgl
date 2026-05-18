#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.0005f;
}

static bool pixel_bright(const uint8_t *p)
{
    return p[0] > 90 && p[1] > 90 && p[2] > 90;
}

static void draw_probe_quad(void)
{
    GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat specular[4] = { 0.85f, 0.85f, 0.85f, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glEnd();
}

static void run_static_probe(void)
{
    GLfloat ambient[4] = { 0.30f, 0.25f, 0.20f, 1.0f };
    GLfloat queried[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat light_diffuse[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
    GLfloat light_specular[4] = { 0.80f, 0.80f, 0.80f, 1.0f };
    GLfloat light_position[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLint ivalue = 0;
    GLuint list;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL, &ivalue);
    expect_bool("default single color", ivalue == GL_SINGLE_COLOR && glGetError() == GL_NO_ERROR, 0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, queried);
    expect_bool("light model ambient query",
                near_float(queried[0], ambient[0]) &&
                near_float(queried[1], ambient[1]) &&
                near_float(queried[2], ambient[2]) &&
                near_float(queried[3], ambient[3]) &&
                glGetError() == GL_NO_ERROR,
                1);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER, &ivalue);
    results[2] = ivalue == GL_TRUE && glGetError() == GL_NO_ERROR;
    glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &ivalue);
    results[2] = results[2] && ivalue == GL_TRUE && glGetError() == GL_NO_ERROR;
    expect_bool("local viewer/two side state", results[2], 2);

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL, &ivalue);
    expect_bool("separate specular state", ivalue == GL_SEPARATE_SPECULAR_COLOR && glGetError() == GL_NO_ERROR, 3);

    glLightModelf(GL_LIGHT_MODEL_COLOR_CONTROL, (GLfloat)GL_SINGLE_COLOR);
    glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL, &ivalue);
    expect_bool("float color control setter", ivalue == GL_SINGLE_COLOR && glGetError() == GL_NO_ERROR, 4);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glEndList();
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
    glCallList(list);
    glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL, &ivalue);
    expect_bool("display list light model replay", ivalue == GL_SEPARATE_SPECULAR_COLOR && glGetError() == GL_NO_ERROR, 5);
    glDeleteLists(list, 1);

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_TEXTURE_2D);
    expect_bool("invalid color control rejected", glGetError() == GL_INVALID_ENUM, 6);

    glLightModelfv(GL_TEXTURE_2D, ambient);
    expect_bool("invalid light model pname rejected", glGetError() == GL_INVALID_ENUM, 7);

    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    draw_probe_quad();
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("separate specular render smoke", pixel_bright(pixel) && glGetError() == GL_NO_ERROR, 8);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER, &ivalue);
    results[9] = ivalue == GL_FALSE && glGetError() == GL_NO_ERROR;
    glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &ivalue);
    results[9] = results[9] && ivalue == GL_FALSE && glGetError() == GL_NO_ERROR;
    expect_bool("local viewer/two side reset", results[9], 9);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_LIGHTING);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
    glEnable(GL_LIGHTING);
}

static void draw_visual_scene(void)
{
    GLfloat ambient[4] = { 0.12f, 0.12f, 0.12f, 1.0f };
    GLfloat light_position[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    GLfloat light_diffuse[4] = { 0.30f, 0.25f, 0.20f, 1.0f };
    GLfloat light_specular[4] = { 0.90f, 0.90f, 0.85f, 1.0f };
    GLfloat red[4] = { 0.80f, 0.08f, 0.06f, 1.0f };
    GLfloat teal[4] = { 0.05f, 0.60f, 0.75f, 1.0f };
    GLfloat specular[4] = { 0.95f, 0.95f, 0.90f, 1.0f };

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 18.0f);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-2.25f, 1.00f, 0.0f);
    glVertex3f(-0.45f, 1.00f, 0.0f);
    glVertex3f(-0.45f, -0.65f, 0.0f);
    glVertex3f(-2.25f, -0.65f, 0.0f);
    glEnd();

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, teal);
    glBegin(GL_QUADS);
    glVertex3f(0.45f, 1.00f, 0.0f);
    glVertex3f(2.25f, 1.00f, 0.0f);
    glVertex3f(2.25f, -0.65f, 0.0f);
    glVertex3f(0.45f, -0.65f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 10; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL light model probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

        draw_visual_scene();
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL light model", all_passed() ? "light model checks passed" : "one or more light model checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
