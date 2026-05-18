#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[13];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool nearf(GLfloat actual, GLfloat expected, GLfloat tolerance)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= tolerance;
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
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_point_at(GLfloat x, GLfloat y, GLfloat z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void draw_line(void)
{
    glBegin(GL_LINES);
    glVertex3f(-0.25f, 0.0f, 0.0f);
    glVertex3f(0.25f, 0.0f, 0.0f);
    glEnd();
}

static void draw_quad_at(GLfloat x, GLfloat y, GLfloat z)
{
    glBegin(GL_QUADS);
    glVertex3f(x - 0.25f, y + 0.25f, z);
    glVertex3f(x + 0.25f, y + 0.25f, z);
    glVertex3f(x + 0.25f, y - 0.25f, z);
    glVertex3f(x - 0.25f, y - 0.25f, z);
    glEnd();
}

static void draw_triangle(void)
{
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.25f, -0.20f, 0.0f);
    glVertex3f(0.25f, -0.20f, 0.0f);
    glVertex3f(0.0f, 0.25f, 0.0f);
    glEnd();
}

static bool feedback_point(GLsizei size, GLfloat *buffer, GLint *count)
{
    memset(buffer, 0, sizeof(GLfloat) * 16u);
    glFeedbackBuffer(size, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(0.0f, 0.0f, 0.0f);
    *count = glRenderMode(GL_RENDER);
    return consume_error(GL_NO_ERROR);
}

static bool select_one(GLsizei size, GLuint *buffer, GLuint name, GLint *hits)
{
    memset(buffer, 0, sizeof(GLuint) * 16u);
    glSelectBuffer(size, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(name);
    draw_quad_at(0.0f, 0.0f, 0.0f);
    *hits = glRenderMode(GL_RENDER);
    return consume_error(GL_NO_ERROR);
}

static void run_probe(void)
{
    GLfloat fb[64];
    GLuint sb[64];
    GLint count;
    GLuint list;
    GLint ivalue = 0;
    bool ok;

    reset_state();
    ok = feedback_point(3, fb, &count);
    ok = ok && count == 3 && nearf(fb[0], (GLfloat)GL_POINT_TOKEN, 0.01f);
    expect_bool("feedback point exact fit", ok, 0);

    reset_state();
    ok = feedback_point(2, fb, &count);
    expect_bool("feedback point one short overflows", ok && count == -1, 1);

    reset_state();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(5, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_line();
    count = glRenderMode(GL_RENDER);
    ok = count == 5 && nearf(fb[0], (GLfloat)GL_LINE_TOKEN, 0.01f) && consume_error(GL_NO_ERROR);
    expect_bool("feedback line exact fit", ok, 2);

    reset_state();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(4, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_line();
    count = glRenderMode(GL_RENDER);
    ok = count == -1 && consume_error(GL_NO_ERROR);
    expect_bool("feedback line one short overflows", ok, 3);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPassThrough(10.5f);
    draw_point_at(0.0f, 0.0f, 0.0f);
    draw_line();
    glEndList();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(64, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    count = glRenderMode(GL_RENDER);
    glDeleteLists(list, 1);
    ok = count == 10 &&
         nearf(fb[0], (GLfloat)GL_PASS_THROUGH_TOKEN, 0.01f) &&
         nearf(fb[1], 10.5f, 0.01f) &&
         nearf(fb[2], (GLfloat)GL_POINT_TOKEN, 0.01f) &&
         nearf(fb[5], (GLfloat)GL_LINE_TOKEN, 0.01f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("display-list mixed feedback", ok, 4);

    reset_state();
    ok = select_one(4, sb, 100, &count);
    ok = ok && count == 1 && sb[0] == 1 && sb[3] == 100;
    expect_bool("selection one hit exact fit", ok, 5);

    reset_state();
    ok = select_one(3, sb, 101, &count);
    expect_bool("selection one hit one short overflows", ok && count == -1, 6);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(8, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(200);
    draw_quad_at(-0.45f, 0.0f, 0.0f);
    glLoadName(201);
    draw_quad_at(0.45f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && sb[0] == 1 && sb[3] == 200 && sb[4] == 1 && sb[7] == 201 && consume_error(GL_NO_ERROR);
    expect_bool("selection two hits exact fit", ok, 7);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(7, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(210);
    draw_quad_at(-0.45f, 0.0f, 0.0f);
    glLoadName(211);
    draw_quad_at(0.45f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == -1 && consume_error(GL_NO_ERROR);
    expect_bool("selection two hits one short overflows", ok, 8);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(16, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(300);
    draw_quad_at(0.0f, 0.0f, -3.0f);
    glLoadName(301);
    draw_quad_at(0.0f, 0.0f, 3.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && sb[3] == 300 && sb[7] == 301 && sb[1] > sb[5] && consume_error(GL_NO_ERROR);
    expect_bool("selection depth ordering near far", ok, 9);

    reset_state();
    memset(fb, 0, sizeof(fb));
    glFeedbackBuffer(64, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(5.15f, 0.0f, 0.0f);
    draw_point_at(20.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 3 && nearf(fb[0], (GLfloat)GL_POINT_TOKEN, 0.01f) && fb[1] >= 638.0f && fb[1] <= 641.0f && consume_error(GL_NO_ERROR);
    expect_bool("feedback point viewport edge", ok, 10);

    reset_state();
    memset(sb, 0, sizeof(sb));
    glSelectBuffer(64, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    for (GLuint i = 0; i < 64; ++i) {
        glPushName(1000u + i);
    }
    glGetIntegerv(GL_NAME_STACK_DEPTH, &ivalue);
    ok = ivalue == 64 && consume_error(GL_NO_ERROR);
    glPushName(2000);
    ok = ok && consume_error(GL_STACK_OVERFLOW);
    glRenderMode(GL_RENDER);
    expect_bool("selection name stack edge", ok && consume_error(GL_NO_ERROR), 11);
}

static void run_begin_guard_probe(void)
{
    GLfloat fb[16];
    GLuint sb[16];
    bool ok;

    reset_state();
    glSelectBuffer(16, sb);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(1);
    glBegin(GL_TRIANGLES);
    glSelectBuffer(16, sb);
    ok = consume_error(GL_INVALID_OPERATION);
    glFeedbackBuffer(16, GL_2D, fb);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glRenderMode(GL_RENDER);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glInitNames();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPushName(2);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glLoadName(3);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPopName();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glPassThrough(4.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, -0.1f, 0.0f);
    glVertex3f(0.0f, 0.1f, 0.0f);
    glEnd();
    glRenderMode(GL_RENDER);
    expect_bool("select feedback begin guards", ok && consume_error(GL_NO_ERROR), 12);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 13; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL select feedback edge probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();
    run_begin_guard_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (int i = 0; i < 13; ++i) {
            draw_bar(-2.85f + (float)i * 0.46f, results[i]);
        }
        nxglSwapBuffers("NXGL select feedback edge", all_passed() ? "all checks passed" : "select feedback edge check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
