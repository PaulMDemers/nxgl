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

static bool near_double(GLdouble actual, GLdouble expected)
{
    GLdouble d = actual - expected;
    if (d < 0.0) {
        d = -d;
    }
    return d < 0.0005;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.0005f;
}

static void run_static_probe(void)
{
    GLboolean bools[16];
    GLdouble doubles[16];
    GLfloat floats[16];
    GLint ints[4];
    GLdouble load_matrix[16];
    GLdouble scale_matrix[16];
    GLfloat verts[9] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         0.0f,  1.0f, 0.0f
    };
    GLfloat colors[12] = {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    };
    GLfloat texcoords[6] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.5f, 1.0f
    };
    GLvoid *ptr = NULL;

    glEnable(GL_BLEND);
    glGetBooleanv(GL_BLEND, bools);
    glGetBooleanv(GL_DEPTH_TEST, bools + 1);
    expect_bool("boolean enable queries", bools[0] == GL_TRUE && bools[1] == GL_TRUE && glGetError() == GL_NO_ERROR, 0);

    glColor4f(0.25f, 0.50f, 0.75f, 1.0f);
    glGetDoublev(GL_CURRENT_COLOR, doubles);
    expect_bool("double current color query",
                near_double(doubles[0], 0.25) && near_double(doubles[1], 0.50) &&
                near_double(doubles[2], 0.75) && near_double(doubles[3], 1.0) &&
                glGetError() == GL_NO_ERROR,
                1);

    glVertexPointer(3, GL_FLOAT, 0, verts);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    glGetPointerv(GL_VERTEX_ARRAY_POINTER, &ptr);
    expect_bool("vertex pointer query", ptr == verts && glGetError() == GL_NO_ERROR, 2);
    glGetPointerv(GL_COLOR_ARRAY_POINTER, &ptr);
    expect_bool("color pointer query", ptr == colors && glGetError() == GL_NO_ERROR, 3);
    glGetPointerv(GL_TEXTURE_COORD_ARRAY_POINTER, &ptr);
    expect_bool("texcoord pointer query", ptr == texcoords && glGetError() == GL_NO_ERROR, 4);
    glClientActiveTexture(GL_TEXTURE0);

    for (int i = 0; i < 16; ++i) {
        load_matrix[i] = 0.0;
        scale_matrix[i] = 0.0;
    }
    load_matrix[0] = 1.0;
    load_matrix[5] = 1.0;
    load_matrix[10] = 1.0;
    load_matrix[15] = 1.0;
    load_matrix[12] = 1.25;
    load_matrix[13] = -0.75;

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(load_matrix);
    glGetDoublev(GL_MODELVIEW_MATRIX, doubles);
    expect_bool("load matrixd query",
                near_double(doubles[12], 1.25) && near_double(doubles[13], -0.75) &&
                glGetError() == GL_NO_ERROR,
                5);

    scale_matrix[0] = 2.0;
    scale_matrix[5] = 3.0;
    scale_matrix[10] = 4.0;
    scale_matrix[15] = 1.0;
    glLoadIdentity();
    glMultMatrixd(scale_matrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, floats);
    expect_bool("mult matrixd query",
                near_float(floats[0], 2.0f) && near_float(floats[5], 3.0f) &&
                near_float(floats[10], 4.0f) && glGetError() == GL_NO_ERROR,
                6);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 6.0, -4.0, 4.0, 1.0, 9.0);
    glGetDoublev(GL_PROJECTION_MATRIX, doubles);
    expect_bool("ortho matrix query",
                near_double(doubles[0], 0.25) && near_double(doubles[5], 0.25) &&
                near_double(doubles[10], -0.25) && near_double(doubles[12], -0.5) &&
                near_double(doubles[14], -1.25) && glGetError() == GL_NO_ERROR,
                7);

    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 11.0);
    glGetDoublev(GL_PROJECTION_MATRIX, doubles);
    expect_bool("frustum matrix query",
                near_double(doubles[0], 1.0) && near_double(doubles[5], 1.0) &&
                near_double(doubles[10], -1.2) && near_double(doubles[11], -1.0) &&
                near_double(doubles[14], -2.2) && glGetError() == GL_NO_ERROR,
                8);

    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, ints);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, ints + 1);
    glGetIntegerv(GL_BLEND_SRC, ints + 2);
    glGetIntegerv(GL_DEPTH_WRITEMASK, ints + 3);
    results[9] = ints[0] == 32 && ints[1] == 8 && ints[2] == GL_SRC_ALPHA &&
                 ints[3] == GL_TRUE && glGetError() == GL_NO_ERROR;
    glOrtho(1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
    results[9] = results[9] && glGetError() == GL_INVALID_VALUE && glGetError() == GL_NO_ERROR;
    expect_bool("integer queries and invalid ortho", results[9], 9);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_BLEND);
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

static void draw_visual_pattern(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.95f, 0.25f, 0.15f);
    glBegin(GL_TRIANGLES);
    glVertex3f(-2.4f, -0.10f, 0.0f);
    glVertex3f(-1.2f, 1.15f, 0.0f);
    glVertex3f( 0.0f, -0.10f, 0.0f);
    glEnd();

    glColor3f(0.15f, 0.70f, 0.95f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.2f, -0.10f, 0.0f);
    glVertex3f(1.4f, 1.15f, 0.0f);
    glVertex3f(2.6f, -0.10f, 0.0f);
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
    debugPrint("NXGL matrix/query probe starting\n");

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
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL matrix/query", all_passed() ? "matrix/query checks passed" : "one or more query checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
