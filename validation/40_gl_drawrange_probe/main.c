#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const GLfloat quad_vertices[] = {
    -1.25f,  1.10f, 0.0f,
     1.25f,  1.10f, 0.0f,
     1.25f, -1.10f, 0.0f,
    -1.25f, -1.10f, 0.0f
};

static const GLfloat quad_colors[] = {
    0.90f, 0.12f, 0.10f, 1.0f,
    0.90f, 0.12f, 0.10f, 1.0f,
    0.90f, 0.12f, 0.10f, 1.0f,
    0.90f, 0.12f, 0.10f, 1.0f
};

static const GLubyte byte_indices[] = { 0, 1, 2, 3 };
static const GLushort short_indices[] = { 0, 1, 2, 3 };
static const GLuint int_indices[] = { 0, 1, 2, 3 };
static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) {
        d = -d;
    }
    return d <= 4;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static void setup_arrays(void)
{
    glVertexPointer(3, GL_FLOAT, 0, quad_vertices);
    glColorPointer(4, GL_FLOAT, 0, quad_colors);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

static void setup_frame(void)
{
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
}

static void run_static_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    setup_frame();
    setup_arrays();

    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_UNSIGNED_BYTE, byte_indices);
    expect_bool("drawrange unsigned byte", glGetError() == GL_NO_ERROR, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_UNSIGNED_SHORT, short_indices);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("drawrange short readback", pixel_rgb(pixel, 229, 30, 25) && glGetError() == GL_NO_ERROR, 1);

    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_UNSIGNED_INT, int_indices);
    expect_bool("drawrange unsigned int", glGetError() == GL_NO_ERROR, 2);

    glDrawRangeElements(GL_QUADS, 3, 2, 4, GL_UNSIGNED_SHORT, short_indices);
    expect_bool("drawrange invalid range", glGetError() == GL_INVALID_VALUE, 3);

    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_FLOAT, short_indices);
    expect_bool("drawrange invalid type", glGetError() == GL_INVALID_ENUM, 4);

    glDrawRangeElements(GL_TEXTURE_2D, 0, 3, 4, GL_UNSIGNED_SHORT, short_indices);
    expect_bool("drawrange invalid mode", glGetError() == GL_INVALID_ENUM, 5);

    glDrawRangeElements(GL_QUADS, 0, 3, -1, GL_UNSIGNED_SHORT, short_indices);
    expect_bool("drawrange invalid count", glGetError() == GL_INVALID_VALUE, 6);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_UNSIGNED_SHORT, short_indices);
    expect_bool("drawrange missing vertex array", glGetError() == GL_INVALID_OPERATION, 7);
    glEnableClientState(GL_VERTEX_ARRAY);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
}

static void draw_result_bar(float x, bool pass)
{
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.15f, 0.0f);
    glVertex3f(x + 0.18f, -2.40f, 0.0f);
    glVertex3f(x - 0.18f, -2.40f, 0.0f);
    glEnd();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
}

static void draw_visual_pattern(void)
{
    setup_arrays();
    glDrawRangeElements(GL_QUADS, 0, 3, 4, GL_UNSIGNED_SHORT, short_indices);

    for (int i = 0; i < 8; ++i) {
        draw_result_bar(-2.10f + (float)i * 0.60f, results[i]);
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
    debugPrint("NXGL drawrange probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        setup_frame();
        draw_visual_pattern();
        nxglSwapBuffers("NXGL drawrange", all_passed() ? "drawrange checks passed" : "one or more drawrange checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
