#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static bool nearf(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.03f;
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
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_MAP1_VERTEX_3);
    glDisable(GL_MAP1_VERTEX_4);
    glDisable(GL_MAP1_COLOR_4);
    glDisable(GL_MAP1_TEXTURE_COORD_2);
    glDisable(GL_MAP2_VERTEX_3);
    glDisable(GL_MAP2_VERTEX_4);
    glDisable(GL_MAP2_COLOR_4);
    glDisable(GL_MAP2_TEXTURE_COORD_2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    GLfloat vertex1[] = {
        -0.60f, -0.25f, 0.0f,
         0.60f, -0.25f, 0.0f
    };
    GLfloat color1[] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f
    };
    GLfloat tex1[] = {
        0.2f, 0.3f,
        0.8f, 0.7f
    };
    GLfloat vertex2[] = {
        -0.45f, -0.45f, 0.0f,
         0.45f, -0.45f, 0.0f,
        -0.45f,  0.45f, 0.0f,
         0.45f,  0.45f, 0.0f
    };
    GLfloat color2[] = {
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f
    };
    GLfloat tex2[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };
    GLfloat buffer[256];
    GLfloat fv[32];
    GLdouble dv[8];
    GLint iv[8];
    GLint count;
    bool ok;

    reset_state();
    glGetIntegerv(GL_MAX_EVAL_ORDER, &iv[0]);
    glEnable(GL_MAP1_VERTEX_3);
    ok = iv[0] >= 8 && glIsEnabled(GL_MAP1_VERTEX_3) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("eval order enable", ok, 0);

    reset_state();
    glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, vertex1);
    glGetMapfv(GL_MAP1_VERTEX_3, GL_DOMAIN, fv);
    glGetMapiv(GL_MAP1_VERTEX_3, GL_ORDER, iv);
    glGetMapfv(GL_MAP1_VERTEX_3, GL_COEFF, &fv[4]);
    ok = nearf(fv[0], 0.0f) && nearf(fv[1], 1.0f) && iv[0] == 2 &&
         nearf(fv[4], -0.60f) && nearf(fv[7], 0.60f) && consume_error(GL_NO_ERROR);
    expect_bool("map1 query coeffs", ok, 1);

    glMap1f(GL_MAP1_COLOR_4, 0.0f, 1.0f, 4, 2, color1);
    glMap1f(GL_MAP1_TEXTURE_COORD_2, 0.0f, 1.0f, 2, 2, tex1);
    glEnable(GL_MAP1_VERTEX_3);
    glEnable(GL_MAP1_COLOR_4);
    glEnable(GL_MAP1_TEXTURE_COORD_2);
    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(256, GL_3D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glEvalCoord1f(0.5f);
    glEnd();
    count = glRenderMode(GL_RENDER);
    ok = count == 12 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[4], 0.5f) && nearf(buffer[5], 0.5f) && nearf(buffer[6], 0.0f) && nearf(buffer[7], 1.0f) &&
         nearf(buffer[8], 0.5f) && nearf(buffer[9], 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("evalcoord1 payload", ok, 2);

    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(256, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glMapGrid1f(2, 0.0f, 1.0f);
    glEvalMesh1(GL_POINT, 0, 2);
    count = glRenderMode(GL_RENDER);
    ok = count == 12 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[4], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[8], (GLfloat)GL_POINT_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("evalmesh1 point grid", ok, 3);

    reset_state();
    glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 6, 2, vertex2);
    glMap2f(GL_MAP2_COLOR_4, 0.0f, 1.0f, 4, 2, 0.0f, 1.0f, 8, 2, color2);
    glMap2f(GL_MAP2_TEXTURE_COORD_2, 0.0f, 1.0f, 2, 2, 0.0f, 1.0f, 4, 2, tex2);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_MAP2_COLOR_4);
    glEnable(GL_MAP2_TEXTURE_COORD_2);
    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(256, GL_3D_COLOR_TEXTURE, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glEvalCoord2f(0.5f, 0.5f);
    glEnd();
    count = glRenderMode(GL_RENDER);
    ok = count == 12 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[4], 0.5f) && nearf(buffer[5], 0.5f) && nearf(buffer[6], 0.25f) && nearf(buffer[7], 1.0f) &&
         nearf(buffer[8], 0.5f) && nearf(buffer[9], 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("evalcoord2 payload", ok, 4);

    glGetMapdv(GL_MAP2_VERTEX_3, GL_DOMAIN, dv);
    glGetMapiv(GL_MAP2_VERTEX_3, GL_ORDER, iv);
    ok = nearf((GLfloat)dv[0], 0.0f) && nearf((GLfloat)dv[1], 1.0f) &&
         nearf((GLfloat)dv[2], 0.0f) && nearf((GLfloat)dv[3], 1.0f) &&
         iv[0] == 2 && iv[1] == 2 && consume_error(GL_NO_ERROR);
    expect_bool("map2 query order domain", ok, 5);

    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(256, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glMapGrid2f(1, 0.0f, 1.0f, 1, 0.0f, 1.0f);
    glEvalMesh2(GL_POINT, 0, 1, 0, 1);
    count = glRenderMode(GL_RENDER);
    ok = count == 16 && nearf(buffer[0], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[4], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[8], (GLfloat)GL_POINT_TOKEN) &&
         nearf(buffer[12], (GLfloat)GL_POINT_TOKEN) && consume_error(GL_NO_ERROR);
    expect_bool("evalmesh2 point grid", ok, 6);

    glEnable(GL_AUTO_NORMAL);
    ok = glIsEnabled(GL_AUTO_NORMAL) == GL_TRUE && consume_error(GL_NO_ERROR);
    glDisable(GL_AUTO_NORMAL);
    ok = ok && glIsEnabled(GL_AUTO_NORMAL) == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("auto normal state", ok, 7);

    glMap1f(GL_TEXTURE_2D, 0.0f, 1.0f, 3, 2, vertex1);
    ok = consume_error(GL_INVALID_ENUM);
    glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 2, 2, vertex1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glMapGrid1f(0, 0.0f, 1.0f);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glEvalCoord1f(0.5f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    expect_bool("evaluator validation", ok, 8);

    glGetMapfv(GL_MAP1_VERTEX_3, GL_TEXTURE_2D, fv);
    ok = consume_error(GL_INVALID_ENUM);
    glGetMapfv(GL_MAP1_VERTEX_3, GL_DOMAIN, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glEnable(GL_MAP2_INDEX);
    ok = ok && glIsEnabled(GL_MAP2_INDEX) == GL_TRUE && consume_error(GL_NO_ERROR);
    expect_bool("getmap validation index state", ok, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.42f, 0.0f);
    glVertex3f(x + 0.16f, -1.42f, 0.0f);
    glVertex3f(x + 0.16f, -1.67f, 0.0f);
    glVertex3f(x - 0.16f, -1.67f, 0.0f);
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
    debugPrint("NXGL evaluator probe starting\n");

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

        glColor3f(0.85f, 0.18f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(-2.35f, 0.92f, 0.0f);
        glVertex3f(-0.85f, 0.92f, 0.0f);
        glVertex3f(-0.85f, -0.20f, 0.0f);
        glVertex3f(-2.35f, -0.20f, 0.0f);
        glEnd();

        glColor3f(0.10f, 0.64f, 0.95f);
        glBegin(GL_QUADS);
        glVertex3f(-0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, -0.42f, 0.0f);
        glVertex3f(-0.55f, -0.42f, 0.0f);
        glEnd();

        glColor3f(0.96f, 0.80f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(0.85f, 0.92f, 0.0f);
        glVertex3f(2.35f, 0.92f, 0.0f);
        glVertex3f(2.35f, -0.20f, 0.0f);
        glVertex3f(0.85f, -0.20f, 0.0f);
        glEnd();

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL evaluators", all_passed() ? "evaluator checks passed" : "one or more evaluator checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
