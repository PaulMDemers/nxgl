#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[10];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool nearf(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d <= 0.02f;
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_point(void)
{
    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
}

static void draw_line(void)
{
    glBegin(GL_LINES);
    glVertex3f(-0.25f, 0.0f, 0.0f);
    glVertex3f(0.25f, 0.0f, 0.0f);
    glEnd();
}

static void draw_triangle(void)
{
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.3f, -0.2f, 0.0f);
    glVertex3f(0.3f, -0.2f, 0.0f);
    glVertex3f(0.0f, 0.3f, 0.0f);
    glEnd();
}

static void draw_quad(void)
{
    glBegin(GL_QUADS);
    glVertex3f(-0.3f, -0.3f, 0.0f);
    glVertex3f(0.3f, -0.3f, 0.0f);
    glVertex3f(0.3f, 0.3f, 0.0f);
    glVertex3f(-0.3f, 0.3f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    GLfloat buffer[128];
    GLint iv = 0;
    GLuint list;
    GLint count;
    bool ok;

    reset_state();
    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glGetIntegerv(GL_RENDER_MODE, &iv);
    count = glRenderMode(GL_RENDER);
    ok = iv == (GLint)GL_FEEDBACK && count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback mode transition", ok, 0);

    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_point();
    count = glRenderMode(GL_RENDER);
    ok = count == 3 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("point token gl2d", ok, 1);

    glFeedbackBuffer(128, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_line();
    count = glRenderMode(GL_RENDER);
    ok = count == 7 && nearf(buffer[0], (GLfloat)GL_LINE_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("line token gl3d", ok, 2);

    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_triangle();
    count = glRenderMode(GL_RENDER);
    ok = count == 8 && nearf(buffer[0], (GLfloat)GL_POLYGON_TOKEN) && nearf(buffer[1], 3.0f) && consume_error(GL_NO_ERROR);
    expect_bool("triangle polygon token", ok, 3);

    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_quad();
    count = glRenderMode(GL_RENDER);
    ok = count == 10 && nearf(buffer[0], (GLfloat)GL_POLYGON_TOKEN) && nearf(buffer[1], 4.0f) && consume_error(GL_NO_ERROR);
    expect_bool("quad polygon token", ok, 4);

    glFeedbackBuffer(128, GL_3D_COLOR, buffer);
    glColor4f(0.25f, 0.5f, 0.75f, 0.6f);
    glRenderMode(GL_FEEDBACK);
    draw_point();
    count = glRenderMode(GL_RENDER);
    ok = count == 8 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[4], 0.25f) && nearf(buffer[5], 0.5f) && nearf(buffer[6], 0.75f) && nearf(buffer[7], 0.6f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("color vertex payload", ok, 5);

    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glPassThrough(42.5f);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && nearf(buffer[0], (GLfloat)GL_PASS_THROUGH_TOKEN) && nearf(buffer[1], 42.5f) && consume_error(GL_NO_ERROR);
    expect_bool("pass through token", ok, 6);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPassThrough(7.25f);
    glEndList();
    glFeedbackBuffer(128, GL_2D, buffer);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && nearf(buffer[0], (GLfloat)GL_PASS_THROUGH_TOKEN) && nearf(buffer[1], 7.25f) && consume_error(GL_NO_ERROR);
    expect_bool("pass through display list", ok, 7);

    glFeedbackBuffer(2, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_point();
    count = glRenderMode(GL_RENDER);
    ok = count == -1 && consume_error(GL_NO_ERROR);
    expect_bool("feedback overflow", ok, 8);

    glFeedbackBuffer(-1, GL_2D, buffer);
    ok = consume_error(GL_INVALID_VALUE);
    glFeedbackBuffer(4, GL_TEXTURE_2D, buffer);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glFeedbackBuffer(4, GL_2D, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glRenderMode(GL_TEXTURE_2D);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glPassThrough(1.0f);
    ok = ok && consume_error(GL_NO_ERROR);
    expect_bool("feedback validation", ok, 9);
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
    debugPrint("NXGL feedback probe starting\n");

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
        nxglSwapBuffers("NXGL feedback", all_passed() ? "all checks passed" : "feedback check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
