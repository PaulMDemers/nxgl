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

static void draw_select_quad(float x)
{
    glBegin(GL_QUADS);
    glVertex3f(x - 0.25f,  0.25f, 0.0f);
    glVertex3f(x + 0.25f,  0.25f, 0.0f);
    glVertex3f(x + 0.25f, -0.25f, 0.0f);
    glVertex3f(x - 0.25f, -0.25f, 0.0f);
    glEnd();
}

static void draw_select_triangle(float x)
{
    glBegin(GL_TRIANGLES);
    glVertex3f(x, 0.35f, 0.0f);
    glVertex3f(x + 0.35f, -0.25f, 0.0f);
    glVertex3f(x - 0.35f, -0.25f, 0.0f);
    glEnd();
}

static void run_static_probe(void)
{
    GLuint buffer[32];
    GLuint overflow_buffer[4];
    GLfloat feedback[8];
    GLint value = 0;
    GLint hits = 0;

    memset(buffer, 0, sizeof(buffer));
    glSelectBuffer(32, buffer);
    glGetIntegerv(GL_SELECTION_BUFFER_SIZE, &value);
    expect_bool("select buffer query", value == 32 && glGetError() == GL_NO_ERROR, 0);

    glRenderMode(GL_SELECT);
    glGetIntegerv(GL_RENDER_MODE, &value);
    expect_bool("enter select mode", value == GL_SELECT && glGetError() == GL_NO_ERROR, 1);

    glInitNames();
    glPushName(11);
    glGetIntegerv(GL_NAME_STACK_DEPTH, &value);
    expect_bool("name stack push/query", value == 1 && glGetError() == GL_NO_ERROR, 2);

    draw_select_quad(-1.0f);
    glLoadName(22);
    draw_select_triangle(0.0f);
    glPushName(33);
    draw_select_quad(1.0f);
    glPopName();
    hits = glRenderMode(GL_RENDER);

    bool records_ok = hits == 3 &&
                      buffer[0] == 1 && buffer[3] == 11 &&
                      buffer[4] == 1 && buffer[7] == 22 &&
                      buffer[8] == 2 && buffer[11] == 22 && buffer[12] == 33;
    expect_bool("selection hit records", records_ok && glGetError() == GL_NO_ERROR, 3);

    glSelectBuffer(4, overflow_buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(101);
    draw_select_quad(-0.4f);
    draw_select_quad(0.4f);
    hits = glRenderMode(GL_RENDER);
    expect_bool("selection overflow return", hits == -1 && glGetError() == GL_NO_ERROR, 4);

    glLoadName(9);
    expect_bool("load name outside select rejected", glGetError() == GL_INVALID_OPERATION, 5);

    glSelectBuffer(32, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(1);
    glPopName();
    glPopName();
    expect_bool("name stack underflow rejected", glGetError() == GL_STACK_UNDERFLOW, 6);
    glRenderMode(GL_RENDER);

    glFeedbackBuffer(8, GL_3D_COLOR, feedback);
    glGetIntegerv(GL_FEEDBACK_BUFFER_SIZE, &value);
    expect_bool("feedback buffer query", value == 8 && glGetError() == GL_NO_ERROR, 7);

    glRenderMode(GL_FEEDBACK);
    hits = glRenderMode(GL_RENDER);
    expect_bool("feedback mode stub return", hits == 0 && glGetError() == GL_NO_ERROR, 8);

    glRenderMode(0xffffffffu);
    expect_bool("invalid render mode rejected", glGetError() == GL_INVALID_ENUM, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
}

static void draw_visual_shapes(void)
{
    glColor3f(0.95f, 0.18f, 0.12f);
    glBegin(GL_QUADS);
    glVertex3f(-2.4f, 1.10f, 0.0f);
    glVertex3f(-1.2f, 1.10f, 0.0f);
    glVertex3f(-1.2f, 0.10f, 0.0f);
    glVertex3f(-2.4f, 0.10f, 0.0f);
    glEnd();

    glColor3f(0.15f, 0.85f, 0.25f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0f, 1.20f, 0.0f);
    glVertex3f(0.75f, 0.05f, 0.0f);
    glVertex3f(-0.75f, 0.05f, 0.0f);
    glEnd();

    glColor3f(0.22f, 0.38f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(1.2f, 1.10f, 0.0f);
    glVertex3f(2.4f, 1.10f, 0.0f);
    glVertex3f(2.4f, 0.10f, 0.0f);
    glVertex3f(1.2f, 0.10f, 0.0f);
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
    debugPrint("NXGL selection probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.6f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

        draw_visual_shapes();
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL selection", all_passed() ? "select/name checks passed" : "one or more selection checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
