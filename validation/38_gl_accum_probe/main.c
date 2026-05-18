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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) {
        d = -d;
    }
    return d <= 3;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static void read_origin(uint8_t *pixel)
{
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void load_clear_color(float r, float g, float b)
{
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void run_static_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLfloat clear_value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLuint list;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glClearAccum(0.10f, 0.20f, 0.30f, 0.40f);
    glGetFloatv(GL_ACCUM_CLEAR_VALUE, clear_value);
    expect_bool("accum clear value query",
                clear_value[0] > 0.09f && clear_value[0] < 0.11f &&
                clear_value[3] > 0.39f && clear_value[3] < 0.41f &&
                glGetError() == GL_NO_ERROR,
                0);

    load_clear_color(0.0f, 0.0f, 0.0f);
    glClear(GL_ACCUM_BUFFER_BIT);
    glAccum(GL_RETURN, 1.0f);
    read_origin(pixel);
    expect_bool("clear accum return", pixel_rgb(pixel, 25, 51, 76) && glGetError() == GL_NO_ERROR, 1);

    load_clear_color(0.20f, 0.40f, 0.60f);
    glAccum(GL_LOAD, 0.5f);
    load_clear_color(0.0f, 0.0f, 0.0f);
    glAccum(GL_RETURN, 1.0f);
    read_origin(pixel);
    expect_bool("load half color", pixel_rgb(pixel, 25, 51, 76) && glGetError() == GL_NO_ERROR, 2);

    load_clear_color(0.40f, 0.10f, 0.0f);
    glAccum(GL_LOAD, 1.0f);
    load_clear_color(0.0f, 0.20f, 0.40f);
    glAccum(GL_ACCUM, 0.5f);
    glAccum(GL_ADD, 0.1f);
    glAccum(GL_MULT, 0.5f);
    glAccum(GL_RETURN, 1.0f);
    read_origin(pixel);
    expect_bool("accum add mult", pixel_rgb(pixel, 63, 38, 38) && glGetError() == GL_NO_ERROR, 3);

    load_clear_color(0.75f, 0.75f, 0.75f);
    glAccum(GL_LOAD, 1.0f);
    glAccum(GL_ADD, -0.5f);
    glAccum(GL_RETURN, 1.0f);
    read_origin(pixel);
    expect_bool("negative add clamps on return", pixel_rgb(pixel, 63, 63, 63) && glGetError() == GL_NO_ERROR, 4);

    load_clear_color(0.30f, 0.00f, 0.00f);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glAccum(GL_LOAD, 1.0f);
    glAccum(GL_ADD, 0.1f);
    glEndList();
    load_clear_color(0.0f, 0.0f, 0.0f);
    glCallList(list);
    glAccum(GL_RETURN, 1.0f);
    read_origin(pixel);
    expect_bool("display list accum replay", pixel_rgb(pixel, 25, 25, 25) && glGetError() == GL_NO_ERROR, 5);
    glDeleteLists(list, 1);

    glAccum(GL_TEXTURE_2D, 1.0f);
    expect_bool("invalid accum op rejected", glGetError() == GL_INVALID_ENUM, 6);

    glBegin(GL_POINTS);
    glAccum(GL_LOAD, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    expect_bool("accum inside begin rejected", glGetError() == GL_INVALID_OPERATION, 7);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
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

    glColor3f(0.80f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(-2.60f, 1.10f, 0.0f);
    glVertex3f(-1.20f, 1.10f, 0.0f);
    glVertex3f(-1.20f, 0.10f, 0.0f);
    glVertex3f(-2.60f, 0.10f, 0.0f);
    glEnd();

    glColor3f(0.45f, 0.30f, 0.85f);
    glBegin(GL_QUADS);
    glVertex3f(-0.70f, 1.10f, 0.0f);
    glVertex3f(0.70f, 1.10f, 0.0f);
    glVertex3f(0.70f, 0.10f, 0.0f);
    glVertex3f(-0.70f, 0.10f, 0.0f);
    glEnd();

    glColor3f(0.10f, 0.70f, 0.65f);
    glBegin(GL_QUADS);
    glVertex3f(1.20f, 1.10f, 0.0f);
    glVertex3f(2.60f, 1.10f, 0.0f);
    glVertex3f(2.60f, 0.10f, 0.0f);
    glVertex3f(1.20f, 0.10f, 0.0f);
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
    debugPrint("NXGL accumulation probe starting\n");

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

        nxglSwapBuffers("NXGL accumulation", all_passed() ? "accum buffer checks passed" : "one or more accum checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
