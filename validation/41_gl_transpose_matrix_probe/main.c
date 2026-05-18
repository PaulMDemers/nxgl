#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.0005f;
}

static bool near_double(GLdouble actual, GLdouble expected)
{
    GLdouble d = actual - expected;
    if (d < 0.0) {
        d = -d;
    }
    return d < 0.0005;
}

static bool matrix_matches_transpose_f(const GLfloat *actual, const GLfloat *source)
{
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int dst = row * 4 + col;
            int src = col * 4 + row;
            if (!near_float(actual[dst], source[src])) {
                return false;
            }
        }
    }
    return true;
}

static bool matrix_matches_source_f(const GLfloat *actual, const GLfloat *source)
{
    for (int i = 0; i < 16; ++i) {
        if (!near_float(actual[i], source[i])) {
            return false;
        }
    }
    return true;
}

static bool matrix_matches_transpose_d(const GLdouble *actual, const GLdouble *source)
{
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            int dst = row * 4 + col;
            int src = col * 4 + row;
            if (!near_double(actual[dst], source[src])) {
                return false;
            }
        }
    }
    return true;
}

static void run_static_probe(void)
{
    GLfloat src_f[16];
    GLfloat src_f2[16];
    GLfloat out_f[16];
    GLdouble src_d[16];
    GLdouble src_d2[16];
    GLdouble out_d[16];

    for (int i = 0; i < 16; ++i) {
        src_f[i] = (GLfloat)(i + 1);
        src_f2[i] = (GLfloat)(32 - i);
        src_d[i] = (GLdouble)(101 + i);
        src_d2[i] = (GLdouble)(201 + i);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadTransposeMatrixf(src_f);
    glGetFloatv(GL_MODELVIEW_MATRIX, out_f);
    expect_bool("load transpose matrixf", matrix_matches_transpose_f(out_f, src_f) && glGetError() == GL_NO_ERROR, 0);

    glGetFloatv(GL_TRANSPOSE_MODELVIEW_MATRIX, out_f);
    expect_bool("transpose modelview query", matrix_matches_source_f(out_f, src_f) && glGetError() == GL_NO_ERROR, 1);

    glLoadIdentity();
    glMultTransposeMatrixf(src_f2);
    glGetFloatv(GL_MODELVIEW_MATRIX, out_f);
    expect_bool("mult transpose matrixf", matrix_matches_transpose_f(out_f, src_f2) && glGetError() == GL_NO_ERROR, 2);

    glMatrixMode(GL_PROJECTION);
    glLoadTransposeMatrixd(src_d);
    glGetDoublev(GL_PROJECTION_MATRIX, out_d);
    expect_bool("load transpose matrixd", matrix_matches_transpose_d(out_d, src_d) && glGetError() == GL_NO_ERROR, 3);

    glGetDoublev(GL_TRANSPOSE_PROJECTION_MATRIX, out_d);
    expect_bool("transpose projection query", near_double(out_d[0], src_d[0]) && near_double(out_d[7], src_d[7]) &&
                near_double(out_d[15], src_d[15]) && glGetError() == GL_NO_ERROR, 4);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMultTransposeMatrixd(src_d2);
    glGetDoublev(GL_TEXTURE_MATRIX, out_d);
    expect_bool("mult transpose matrixd", matrix_matches_transpose_d(out_d, src_d2) && glGetError() == GL_NO_ERROR, 5);

    glGetFloatv(GL_TRANSPOSE_COLOR_MATRIX, out_f);
    expect_bool("transpose color matrix identity", near_float(out_f[0], 1.0f) && near_float(out_f[5], 1.0f) &&
                near_float(out_f[10], 1.0f) && near_float(out_f[15], 1.0f) &&
                near_float(out_f[1], 0.0f) && glGetError() == GL_NO_ERROR, 6);

    glLoadTransposeMatrixf(NULL);
    results[7] = glGetError() == GL_INVALID_VALUE;
    glMultTransposeMatrixd(NULL);
    results[7] = results[7] && glGetError() == GL_INVALID_VALUE && glGetError() == GL_NO_ERROR;
    expect_bool("null transpose matrices rejected", results[7], 7);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.40f, 0.0f);
    glVertex3f(x - 0.18f, -2.40f, 0.0f);
    glEnd();
}

static void draw_visual_pattern(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.25f, 0.80f, 0.95f);
    glBegin(GL_QUADS);
    glVertex3f(-2.3f, 0.95f, 0.0f);
    glVertex3f(-0.4f, 0.95f, 0.0f);
    glVertex3f(-0.4f, -0.65f, 0.0f);
    glVertex3f(-2.3f, -0.65f, 0.0f);
    glEnd();

    glColor3f(0.95f, 0.70f, 0.15f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.6f, -0.65f, 0.0f);
    glVertex3f(1.6f, 1.05f, 0.0f);
    glVertex3f(2.6f, -0.65f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL transpose matrix probe starting\n");

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

        draw_visual_pattern();
        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.10f + (float)i * 0.60f, results[i]);
        }

        nxglSwapBuffers("NXGL transpose matrix", all_passed() ? "transpose matrix checks passed" : "one or more transpose checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
