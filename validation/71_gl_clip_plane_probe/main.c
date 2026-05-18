#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[8];

static bool neard(GLdouble a, GLdouble b)
{
    GLdouble d = a - b;
    if (d < 0.0) d = -d;
    return d <= 0.03;
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
    for (int i = 0; i < 6; ++i) {
        glDisable(GL_CLIP_PLANE0 + i);
    }
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static GLint feedback_point(GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat buffer[32];

    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(32, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static GLint selection_point(GLfloat x, GLfloat y, GLfloat z)
{
    GLuint select_buffer[16];

    memset(select_buffer, 0, sizeof(select_buffer));
    glSelectBuffer(16, select_buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(7);
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static void run_probe(void)
{
    const GLdouble x_plane[4] = { 1.0, 0.0, 0.0, 0.0 };
    const GLdouble y_plane[4] = { 0.0, 1.0, 0.0, 0.0 };
    GLdouble dv[4];
    GLboolean bv;
    GLint iv;
    GLuint list;
    bool ok;

    reset_state();
    glGetIntegerv(GL_MAX_CLIP_PLANES, &iv);
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glEnable(GL_CLIP_PLANE0);
    glGetClipPlane(GL_CLIP_PLANE0, dv);
    ok = iv == 6 && glIsEnabled(GL_CLIP_PLANE0) == GL_TRUE &&
         neard(dv[0], 1.0) && neard(dv[1], 0.0) && neard(dv[2], 0.0) && neard(dv[3], 0.0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("clip plane state", ok, 0);

    reset_state();
    glTranslatef(0.5f, 0.0f, 0.0f);
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glGetClipPlane(GL_CLIP_PLANE0, dv);
    ok = neard(dv[0], 1.0) && neard(dv[3], -0.5) && consume_error(GL_NO_ERROR);
    expect_bool("clip plane modelview transform", ok, 1);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glEnable(GL_CLIP_PLANE0);
    ok = feedback_point(-0.25f, 0.0f, 0.0f) == 0 &&
         feedback_point(0.25f, 0.0f, 0.0f) == 4 &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback clipping", ok, 2);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glEnable(GL_CLIP_PLANE0);
    ok = selection_point(-0.25f, 0.0f, 0.0f) == 0 &&
         selection_point(0.25f, 0.0f, 0.0f) == 1 &&
         consume_error(GL_NO_ERROR);
    expect_bool("selection clipping", ok, 3);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glEnable(GL_CLIP_PLANE0);
    glRasterPos3f(-0.25f, 0.0f, 0.0f);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &bv);
    ok = bv == GL_FALSE;
    glRasterPos3f(0.25f, 0.0f, 0.0f);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &bv);
    ok = ok && bv == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("raster position clipping", ok, 4);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_plane);
    glEnable(GL_CLIP_PLANE0);
    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT);
    glClipPlane(GL_CLIP_PLANE0, y_plane);
    glDisable(GL_CLIP_PLANE0);
    glPopAttrib();
    glGetClipPlane(GL_CLIP_PLANE0, dv);
    ok = glIsEnabled(GL_CLIP_PLANE0) == GL_TRUE &&
         neard(dv[0], 1.0) && neard(dv[1], 0.0) && consume_error(GL_NO_ERROR);
    expect_bool("attrib clip restore", ok, 5);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glClipPlane(GL_CLIP_PLANE1, y_plane);
    glEnable(GL_CLIP_PLANE1);
    glEndList();
    glCallList(list);
    glGetClipPlane(GL_CLIP_PLANE1, dv);
    ok = glIsEnabled(GL_CLIP_PLANE1) == GL_TRUE &&
         neard(dv[0], 0.0) && neard(dv[1], 1.0) && consume_error(GL_NO_ERROR);
    expect_bool("display list clip replay", ok, 6);

    reset_state();
    glClipPlane(GL_TEXTURE_2D, x_plane);
    ok = consume_error(GL_INVALID_ENUM);
    glClipPlane(GL_CLIP_PLANE0, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetClipPlane(GL_TEXTURE_2D, dv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("clip validation", ok, 7);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.2f, -1.42f, 0.0f);
    glVertex3f(x + 0.2f, -1.42f, 0.0f);
    glVertex3f(x + 0.2f, -1.68f, 0.0f);
    glVertex3f(x - 0.2f, -1.68f, 0.0f);
    glEnd();
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
    debugPrint("NXGL clip plane probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        reset_state();

        glColor3f(0.92f, 0.18f, 0.18f);
        glBegin(GL_QUADS);
        glVertex3f(-2.30f, 0.88f, 0.0f);
        glVertex3f(-0.30f, 0.88f, 0.0f);
        glVertex3f(-0.30f, -0.28f, 0.0f);
        glVertex3f(-2.30f, -0.28f, 0.0f);
        glEnd();

        glColor3f(0.14f, 0.78f, 0.45f);
        glBegin(GL_QUADS);
        glVertex3f(0.30f, 0.88f, 0.0f);
        glVertex3f(2.30f, 0.88f, 0.0f);
        glVertex3f(2.30f, -0.28f, 0.0f);
        glVertex3f(0.30f, -0.28f, 0.0f);
        glEnd();

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.1f + (float)i * 0.6f, results[i]);
        }

        nxglSwapBuffers("NXGL clip planes", all_passed() ? "clip checks passed" : "one or more clip checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
