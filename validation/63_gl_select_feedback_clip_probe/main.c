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
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_point_at(float x, float y, float z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void draw_line_at(float x, float y, float z)
{
    glBegin(GL_LINES);
    glVertex3f(x - 0.25f, y, z);
    glVertex3f(x + 0.25f, y, z);
    glEnd();
}

static void draw_quad_at(float x, float y, float z)
{
    glBegin(GL_QUADS);
    glVertex3f(x - 0.25f, y + 0.25f, z);
    glVertex3f(x + 0.25f, y + 0.25f, z);
    glVertex3f(x + 0.25f, y - 0.25f, z);
    glVertex3f(x - 0.25f, y - 0.25f, z);
    glEnd();
}

static void run_probe(void)
{
    GLfloat feedback[64];
    GLuint select[32];
    GLint count;
    bool ok;

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 3 && nearf(feedback[0], (GLfloat)GL_POINT_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("feedback visible point", ok, 0);

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(20.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback offscreen point culled", ok, 1);

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(0.0f, 0.0f, 10.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback behind point culled", ok, 2);

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_line_at(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 5 && nearf(feedback[0], (GLfloat)GL_LINE_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("feedback visible line", ok, 3);

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_line_at(20.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback offscreen line culled", ok, 4);

    reset_state();
    memset(select, 0, sizeof(select));
    glSelectBuffer(32, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(10);
    draw_quad_at(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 1 && select[0] == 1 && select[3] == 10 && consume_error(GL_NO_ERROR);
    expect_bool("selection visible hit", ok, 5);

    reset_state();
    glSelectBuffer(32, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(20);
    draw_quad_at(20.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection offscreen culled", ok, 6);

    reset_state();
    glSelectBuffer(32, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(30);
    draw_quad_at(0.0f, 0.0f, 10.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection behind culled", ok, 7);

    reset_state();
    memset(select, 0, sizeof(select));
    glSelectBuffer(32, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(40);
    draw_quad_at(0.0f, 0.0f, 3.0f);
    glLoadName(41);
    draw_quad_at(0.0f, 0.0f, -3.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && select[3] == 40 && select[7] == 41 && select[1] < select[5] && consume_error(GL_NO_ERROR);
    expect_bool("selection eye depth order", ok, 8);

    reset_state();
    glFeedbackBuffer(64, GL_2D, feedback);
    glRenderMode(GL_FEEDBACK);
    draw_point_at(20.0f, 0.0f, 0.0f);
    glPassThrough(12.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 2 && nearf(feedback[0], (GLfloat)GL_PASS_THROUGH_TOKEN) && nearf(feedback[1], 12.0f) && consume_error(GL_NO_ERROR);
    expect_bool("pass through survives cull", ok, 9);
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
    debugPrint("NXGL select/feedback clip probe starting\n");

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
        nxglSwapBuffers("NXGL select/feedback clip", all_passed() ? "all checks passed" : "clip/depth check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
