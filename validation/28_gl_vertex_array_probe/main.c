#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const GLfloat quad_vertices[] = {
    -2.7f,  1.6f, 0.0f,
    -1.1f,  1.6f, 0.0f,
    -1.1f,  0.2f, 0.0f,
    -2.7f,  0.2f, 0.0f
};

static const GLfloat strip_vertices[] = {
     0.2f,  1.6f, 0.0f,
     1.7f,  1.6f, 0.0f,
     0.2f,  0.2f, 0.0f,
     1.7f,  0.2f, 0.0f
};

static const GLfloat indexed_vertices[] = {
    -2.7f, -0.2f, 0.0f,
    -1.1f, -0.2f, 0.0f,
    -1.1f, -1.6f, 0.0f,
    -2.7f, -1.6f, 0.0f,
     0.2f, -0.2f, 0.0f,
     1.7f, -0.2f, 0.0f,
     1.7f, -1.6f, 0.0f,
     0.2f, -1.6f, 0.0f
};

static const GLfloat colors[] = {
    0.95f, 0.10f, 0.10f, 1.0f,
    0.10f, 0.85f, 0.20f, 1.0f,
    0.10f, 0.35f, 0.95f, 1.0f,
    0.95f, 0.85f, 0.10f, 1.0f,
    0.95f, 0.25f, 0.80f, 1.0f,
    0.20f, 0.90f, 0.90f, 1.0f,
    0.95f, 0.45f, 0.10f, 1.0f,
    0.70f, 0.50f, 1.00f, 1.0f
};

static const GLubyte byte_colors[] = {
    255, 0, 0, 255,
    0, 255, 0, 255,
    0, 64, 255, 255,
    255, 255, 0, 255
};

static const GLushort quad_indices[] = { 0, 1, 2, 3 };
static const GLubyte fan_indices[] = { 4, 5, 6, 7 };

static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void run_static_probe(void)
{
    GLint value = 0;

    glVertexPointer(3, GL_FLOAT, 0, quad_vertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glGetIntegerv(GL_VERTEX_ARRAY_SIZE, &value);
    expect_bool("glVertexPointer size query", value == 3 && glGetError() == GL_NO_ERROR, 0);

    glColorPointer(4, GL_FLOAT, 0, colors);
    glEnableClientState(GL_COLOR_ARRAY);
    glGetIntegerv(GL_COLOR_ARRAY_TYPE, &value);
    expect_bool("glColorPointer type query", value == GL_FLOAT && glGetError() == GL_NO_ERROR, 1);

    glClientActiveTexture(GL_TEXTURE1);
    glTexCoordPointer(2, GL_FLOAT, 0, quad_vertices);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glGetIntegerv(GL_TEXTURE_COORD_ARRAY_SIZE, &value);
    expect_bool("client texture coord array query", value == 2 && glGetError() == GL_NO_ERROR, 2);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glClientActiveTexture(GL_TEXTURE0);

    glDrawArrays(GL_LINES, 0, 2);
    expect_bool("line primitive accepted", glGetError() == GL_NO_ERROR, 3);
}

static void draw_result_bar(float x, bool pass)
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.45f, 0.0f);
    glVertex3f(x - 0.18f, -2.45f, 0.0f);
    glEnd();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

static void draw_arrays_scene(void)
{
    glVertexPointer(3, GL_FLOAT, 0, quad_vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDrawArrays(GL_QUADS, 0, 4);
    results[4] = glGetError() == GL_NO_ERROR;

    glVertexPointer(3, GL_FLOAT, 0, strip_vertices);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, byte_colors);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    results[5] = glGetError() == GL_NO_ERROR;

    glVertexPointer(3, GL_FLOAT, 0, indexed_vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, quad_indices);
    results[6] = glGetError() == GL_NO_ERROR;

    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_BYTE, fan_indices);
    results[7] = glGetError() == GL_NO_ERROR;

    for (int i = 0; i < 8; ++i) {
        draw_result_bar(-2.4f + (float)i * 0.7f, results[i]);
    }
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
    debugPrint("NXGL vertex array probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -6.0f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        draw_arrays_scene();
        nxglSwapBuffers("NXGL vertex arrays", all_passed() ? "draw arrays/elements passed" : "one or more array checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
