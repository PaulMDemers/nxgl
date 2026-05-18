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
    return d <= 1.5f;
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
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_point(float x, float y, float z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void draw_quad(float x, float y, float z)
{
    glBegin(GL_QUADS);
    glVertex3f(x - 0.2f, y + 0.2f, z);
    glVertex3f(x + 0.2f, y + 0.2f, z);
    glVertex3f(x + 0.2f, y - 0.2f, z);
    glVertex3f(x - 0.2f, y - 0.2f, z);
    glEnd();
}

static void run_probe(void)
{
    GLfloat fb[64];
    GLfloat rp[4];
    GLint iv[4];
    GLuint select[16];
    GLuint list;
    GLint count;
    bool ok;

    reset_state();
    glViewport(160, 120, 320, 240);
    glGetIntegerv(GL_VIEWPORT, iv);
    ok = iv[0] == 160 && iv[1] == 120 && iv[2] == 320 && iv[3] == 240 && consume_error(GL_NO_ERROR);
    expect_bool("viewport query", ok, 0);

    reset_state();
    glViewport(160, 120, 320, 240);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, rp);
    ok = nearf(rp[0], 320.0f) && nearf(rp[1], 240.0f) && consume_error(GL_NO_ERROR);
    expect_bool("viewport raster center", ok, 1);

    reset_state();
    glViewport(160, 120, 100, 100);
    glRasterPos3f(10.0f, 0.0f, 0.0f);
    glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, (GLboolean *)iv);
    ok = ((GLboolean *)iv)[0] == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("viewport raster cull", ok, 2);

    reset_state();
    memset(fb, 0, sizeof(fb));
    glViewport(160, 120, 100, 100);
    glFeedbackBuffer(64, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_point(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 3 && nearf(fb[1], 210.0f) && nearf(fb[2], 240.0f) && consume_error(GL_NO_ERROR);
    expect_bool("feedback viewport coords", ok, 3);

    reset_state();
    glViewport(160, 120, 100, 100);
    glFeedbackBuffer(64, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_point(10.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback viewport cull", ok, 4);

    reset_state();
    memset(select, 0, sizeof(select));
    glViewport(160, 120, 100, 100);
    glSelectBuffer(16, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(76);
    draw_quad(10.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection viewport cull", ok, 5);

    reset_state();
    glViewport(0, 0, 0, 0);
    glFeedbackBuffer(64, GL_2D, fb);
    glRenderMode(GL_FEEDBACK);
    draw_point(0.0f, 0.0f, 0.0f);
    count = glRenderMode(GL_RENDER);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("zero viewport culls", ok, 6);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glViewport(100, 90, 200, 180);
    glEndList();
    glCallList(list);
    glGetIntegerv(GL_VIEWPORT, iv);
    ok = iv[0] == 100 && iv[1] == 90 && iv[2] == 200 && iv[3] == 180 && consume_error(GL_NO_ERROR);
    expect_bool("display-list viewport replay", ok, 7);
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
    debugPrint("NXGL viewport clip probe starting\n");

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
        nxglSwapBuffers("NXGL viewport clip", all_passed() ? "all checks passed" : "viewport check failed");
        Sleep(16);
    }
}
