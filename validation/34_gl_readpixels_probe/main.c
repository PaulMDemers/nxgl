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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) {
        d = -d;
    }
    return d <= 3;
}

static bool pixel_is_rgba(const uint8_t *pixel, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return near_byte(pixel[0], r) &&
           near_byte(pixel[1], g) &&
           near_byte(pixel[2], b) &&
           near_byte(pixel[3], a);
}

static void draw_center_quad(float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-3.0f,  2.2f, 0.0f);
    glVertex3f( 3.0f,  2.2f, 0.0f);
    glVertex3f( 3.0f, -2.2f, 0.0f);
    glVertex3f(-3.0f, -2.2f, 0.0f);
    glEnd();
}

static void run_static_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    uint8_t rows[32];
    GLint value = 0;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetIntegerv(GL_PACK_ALIGNMENT, &value);
    expect_bool("pack alignment query", value == 1 && glGetError() == GL_NO_ERROR, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &value);
    expect_bool("unpack alignment query", value == 8 && glGetError() == GL_NO_ERROR, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glPixelStorei(GL_PACK_ALIGNMENT, 3);
    expect_bool("invalid pack alignment rejected", glGetError() == GL_INVALID_VALUE, 2);

    glClearColor(0.25f, 0.50f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("clear read rgba", pixel_is_rgba(pixel, 63, 127, 191, 255) && glGetError() == GL_NO_ERROR, 3);

    memset(pixel, 0, sizeof(pixel));
    glReadPixels(0, 0, 1, 1, GL_BGRA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("clear read bgra", near_byte(pixel[0], 191) && near_byte(pixel[1], 127) && near_byte(pixel[2], 63) && pixel[3] == 255 && glGetError() == GL_NO_ERROR, 4);

    memset(rows, 0xcd, sizeof(rows));
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadPixels(0, 0, 3, 2, GL_RGB, GL_UNSIGNED_BYTE, rows);
    expect_bool("rgb pack padding preserved", rows[9] == 0xcd && rows[10] == 0xcd && rows[11] == 0xcd && glGetError() == GL_NO_ERROR, 5);

    memset(rows, 0xcd, sizeof(rows));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 3, 2, GL_RGB, GL_UNSIGNED_BYTE, rows);
    expect_bool("rgb pack alignment one", rows[9] != 0xcd && rows[10] != 0xcd && rows[11] != 0xcd && glGetError() == GL_NO_ERROR, 6);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.6f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    draw_center_quad(0.95f, 0.10f, 0.12f);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("draw flush read center", pixel[0] > 180 && pixel[1] < 80 && pixel[2] < 80 && glGetError() == GL_NO_ERROR, 7);

    glReadPixels(0, 0, 1, 1, GL_TEXTURE_2D, GL_UNSIGNED_BYTE, pixel);
    expect_bool("invalid read format rejected", glGetError() == GL_INVALID_ENUM, 8);

    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, pixel);
    expect_bool("invalid read type rejected", glGetError() == GL_INVALID_ENUM, 9);
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
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);

    glColor3f(0.95f, 0.10f, 0.12f);
    glBegin(GL_QUADS);
    glVertex3f(-2.2f, 1.20f, 0.0f);
    glVertex3f(-0.7f, 1.20f, 0.0f);
    glVertex3f(-0.7f, 0.15f, 0.0f);
    glVertex3f(-2.2f, 0.15f, 0.0f);
    glEnd();

    glColor3f(0.15f, 0.80f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-0.45f, 1.20f, 0.0f);
    glVertex3f(1.05f, 1.20f, 0.0f);
    glVertex3f(1.05f, 0.15f, 0.0f);
    glVertex3f(-0.45f, 0.15f, 0.0f);
    glEnd();

    glColor3f(0.20f, 0.35f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(1.30f, 1.20f, 0.0f);
    glVertex3f(2.80f, 1.20f, 0.0f);
    glVertex3f(2.80f, 0.15f, 0.0f);
    glVertex3f(1.30f, 0.15f, 0.0f);
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
    char detail[64];

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL readpixels probe starting\n");

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

        if (all_passed()) {
            strcpy(detail, "readpixels/pack checks passed");
        } else {
            strcpy(detail, "one or more readpixels checks failed");
        }
        nxglSwapBuffers("NXGL readpixels", detail);
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
