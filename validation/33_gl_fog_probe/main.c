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

static bool near_float(float a, float b)
{
    float d = a - b;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.001f;
}

static void run_static_probe(void)
{
    GLfloat value = 0.0f;
    GLfloat color[4] = { 0.22f, 0.30f, 0.38f, 1.0f };
    GLfloat out_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint ivalue = 0;
    GLuint list = 0;

    glEnable(GL_FOG);
    expect_bool("fog enable", glIsEnabled(GL_FOG) == GL_TRUE && glGetError() == GL_NO_ERROR, 0);

    glFogf(GL_FOG_DENSITY, 0.35f);
    glGetFloatv(GL_FOG_DENSITY, &value);
    expect_bool("fog density query", near_float(value, 0.35f) && glGetError() == GL_NO_ERROR, 1);

    glFogi(GL_FOG_MODE, GL_LINEAR);
    glGetIntegerv(GL_FOG_MODE, &ivalue);
    expect_bool("fog mode query", ivalue == GL_LINEAR && glGetError() == GL_NO_ERROR, 2);

    glFogfv(GL_FOG_COLOR, color);
    glGetFloatv(GL_FOG_COLOR, out_color);
    expect_bool("fog color query", near_float(out_color[0], color[0]) && near_float(out_color[1], color[1]) && near_float(out_color[2], color[2]) && glGetError() == GL_NO_ERROR, 3);

    glFogf(GL_FOG_START, 0.5f);
    glFogf(GL_FOG_END, 3.5f);
    glGetFloatv(GL_FOG_START, &value);
    bool start_ok = near_float(value, 0.5f);
    glGetFloatv(GL_FOG_END, &value);
    expect_bool("fog range query", start_ok && near_float(value, 3.5f) && glGetError() == GL_NO_ERROR, 4);

    glFogi(GL_FOG_MODE, GL_EXP2);
    glGetIntegerv(GL_FOG_MODE, &ivalue);
    expect_bool("exp2 mode accepted", ivalue == GL_EXP2 && glGetError() == GL_NO_ERROR, 5);

    glFogi(GL_FOG_MODE, GL_NEAREST);
    expect_bool("invalid fog mode rejected", glGetError() == GL_INVALID_ENUM, 6);

    glFogf(GL_FOG_DENSITY, -0.1f);
    expect_bool("invalid fog density rejected", glGetError() == GL_INVALID_VALUE, 7);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogf(GL_FOG_DENSITY, 0.45f);
    glEndList();
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 0.10f);
    glCallList(list);
    glGetIntegerv(GL_FOG_MODE, &ivalue);
    glGetFloatv(GL_FOG_DENSITY, &value);
    expect_bool("fog display list replay", ivalue == GL_EXP && near_float(value, 0.45f) && glGetError() == GL_NO_ERROR, 8);
    glDeleteLists(list, 1);

    glDisable(GL_FOG);
    expect_bool("fog disable", glIsEnabled(GL_FOG) == GL_FALSE && glGetError() == GL_NO_ERROR, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_FOG);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.18f, 0.0f);
    glVertex3f(x + 0.16f, -2.18f, 0.0f);
    glVertex3f(x + 0.16f, -2.42f, 0.0f);
    glVertex3f(x - 0.16f, -2.42f, 0.0f);
    glEnd();
    glEnable(GL_FOG);
}

static void draw_fog_quad(float x, float y, float z, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.32f, y + 0.22f, z);
    glVertex3f(x + 0.32f, y + 0.22f, z);
    glVertex3f(x + 0.32f, y - 0.22f, z);
    glVertex3f(x - 0.32f, y - 0.22f, z);
    glEnd();
}

static void draw_fog_row(GLenum mode, float y, float density)
{
    GLfloat fog_color[4] = { 0.08f, 0.12f, 0.18f, 1.0f };

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, (GLint)mode);
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, density);
    glFogf(GL_FOG_START, 0.2f);
    glFogf(GL_FOG_END, 4.0f);

    for (int i = 0; i < 6; ++i) {
        float x = -2.45f + (float)i * 0.95f;
        float z = (float)i * 0.75f;
        draw_fog_quad(x, y, z, 0.95f, 0.35f + (float)i * 0.08f, 0.15f);
    }
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
    char detail[64];

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL fog probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.08f, 0.12f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.8f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);

        draw_fog_row(GL_LINEAR, 1.30f, 0.65f);
        draw_fog_row(GL_EXP, 0.35f, 0.42f);
        draw_fog_row(GL_EXP2, -0.60f, 0.34f);

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        if (all_passed()) {
            strcpy(detail, "fog state/render checks passed");
        } else {
            strcpy(detail, "one or more fog checks failed");
        }
        nxglSwapBuffers("NXGL fog", detail);
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
