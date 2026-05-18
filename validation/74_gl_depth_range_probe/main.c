#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[8];

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.025f;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
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
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_quad(float z)
{
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, z);
    glVertex3f(0.55f, 0.55f, z);
    glVertex3f(0.55f, -0.55f, z);
    glVertex3f(-0.55f, -0.55f, z);
    glEnd();
}

static void run_probe(void)
{
    GLfloat fv[64];
    GLdouble dv[4];
    GLfloat depth = 0.0f;
    GLuint select[16];
    GLuint list;
    GLint count;
    bool ok;

    reset_state();
    glGetFloatv(GL_DEPTH_RANGE, fv);
    ok = nearf(fv[0], 0.0f) && nearf(fv[1], 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("default depth range query", ok, 0);

    glDepthRange(-0.5f, 1.5f);
    glGetDoublev(GL_DEPTH_RANGE, dv);
    ok = dv[0] == 0.0 && dv[1] == 1.0 && consume_error(GL_NO_ERROR);
    expect_bool("depth range clamps", ok, 1);

    reset_state();
    glDepthRange(0.25f, 0.75f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_quad(0.0f);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = nearf(depth, 0.25f + 0.052f * 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("primitive depth range write", ok, 2);

    reset_state();
    glDepthRange(0.2f, 0.6f);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, fv);
    ok = nearf(fv[2], 0.2f + 0.052f * 0.4f) && consume_error(GL_NO_ERROR);
    expect_bool("raster depth range", ok, 3);

    reset_state();
    glDepthRange(0.1f, 0.3f);
    memset(fv, 0, sizeof(fv));
    glFeedbackBuffer(64, GL_3D, fv);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    count = glRenderMode(GL_RENDER);
    ok = count == 4 && nearf(fv[3], 0.1f + 0.052f * 0.2f) && consume_error(GL_NO_ERROR);
    expect_bool("feedback depth range", ok, 4);

    reset_state();
    memset(select, 0, sizeof(select));
    glDepthRange(0.4f, 0.8f);
    glSelectBuffer(16, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(74);
    draw_quad(0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 1 && select[3] == 74 && select[1] > (GLuint)(0.40f * 4294967295.0f) &&
         select[1] < (GLuint)(0.45f * 4294967295.0f) && consume_error(GL_NO_ERROR);
    expect_bool("selection depth range", ok, 5);

    reset_state();
    glDepthRange(0.8f, 0.2f);
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    draw_quad(0.0f);
    glReadPixels(320, 240, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    ok = nearf(depth, 0.8f - 0.052f * 0.6f) && consume_error(GL_NO_ERROR);
    expect_bool("reversed depth range", ok, 6);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glDepthRange(0.33f, 0.66f);
    glEndList();
    glCallList(list);
    glGetFloatv(GL_DEPTH_RANGE, fv);
    ok = nearf(fv[0], 0.33f) && nearf(fv[1], 0.66f) && consume_error(GL_NO_ERROR);
    expect_bool("display-list depth range", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
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
    debugPrint("NXGL depth range probe starting\n");

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
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL depth range", all_passed() ? "all checks passed" : "depth range check failed");
        Sleep(16);
    }
}
