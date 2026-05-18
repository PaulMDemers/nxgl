#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[8];

static bool nearf(GLfloat a, GLfloat b, GLfloat tolerance)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= tolerance;
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
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static GLint feedback_line(GLfloat ax, GLfloat ay, GLfloat az, GLfloat bx, GLfloat by, GLfloat bz, GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 64u);
    glFeedbackBuffer(64, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_LINES);
    glVertex3f(ax, ay, az);
    glVertex3f(bx, by, bz);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static GLint select_line(GLfloat ax, GLfloat ay, GLfloat az, GLfloat bx, GLfloat by, GLfloat bz, GLuint *buffer)
{
    memset(buffer, 0, sizeof(GLuint) * 32u);
    glSelectBuffer(32, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(78);
    glBegin(GL_LINES);
    glVertex3f(ax, ay, az);
    glVertex3f(bx, by, bz);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static void run_probe(void)
{
    GLfloat fb[64];
    GLuint select[32];
    GLdouble clip_x_positive[4] = { 1.0, 0.0, 0.0, 0.0 };
    GLint count;
    bool ok;

    reset_state();
    count = feedback_line(-20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, fb);
    ok = count == 7 && nearf(fb[0], (GLfloat)GL_LINE_TOKEN, 0.01f) &&
         fb[1] >= -1.0f && fb[1] <= 2.0f && nearf(fb[2], 240.0f, 2.0f) &&
         nearf(fb[4], 320.0f, 2.0f) && nearf(fb[5], 240.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback line left clipped", ok, 0);

    reset_state();
    count = feedback_line(0.0f, 0.0f, 5.5f, 0.0f, 0.0f, 0.0f, fb);
    ok = count == 7 && nearf(fb[1], 320.0f, 2.0f) && nearf(fb[2], 240.0f, 2.0f) &&
         fb[3] <= 0.01f && nearf(fb[4], 320.0f, 2.0f) && nearf(fb[5], 240.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback line near clipped", ok, 1);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    count = feedback_line(-1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, fb);
    ok = count == 7 && nearf(fb[1], 320.0f, 2.0f) && nearf(fb[2], 240.0f, 2.0f) &&
         fb[4] > 370.0f && nearf(fb[5], 240.0f, 2.0f) && consume_error(GL_NO_ERROR);
    expect_bool("feedback line user clip", ok, 2);

    reset_state();
    count = feedback_line(-20.0f, 0.0f, 5.5f, -10.0f, 0.0f, 5.6f, fb);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback fully clipped line", ok, 3);

    reset_state();
    count = select_line(0.0f, 0.0f, 5.5f, 0.0f, 0.0f, 0.0f, select);
    ok = count == 1 && select[0] == 1 && select[3] == 78 && select[1] <= select[2] && consume_error(GL_NO_ERROR);
    expect_bool("selection near-clipped line hit", ok, 4);

    reset_state();
    count = select_line(-20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, select);
    ok = count == 1 && select[0] == 1 && select[3] == 78 && consume_error(GL_NO_ERROR);
    expect_bool("selection left-clipped line hit", ok, 5);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    count = select_line(-2.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, select);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection user-clipped miss", ok, 6);

    reset_state();
    count = feedback_line(0.0f, -20.0f, 0.0f, 0.0f, 0.0f, 0.0f, fb);
    ok = count == 7 && fb[2] >= 478.0f && fb[2] <= 481.0f &&
         nearf(fb[5], 240.0f, 2.0f) && consume_error(GL_NO_ERROR);
    expect_bool("feedback line bottom clipped", ok, 7);
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
    debugPrint("NXGL feedback line clip probe starting\n");

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
        nxglSwapBuffers("NXGL line clipping", all_passed() ? "all checks passed" : "line clip check failed");
        Sleep(16);
    }
}
