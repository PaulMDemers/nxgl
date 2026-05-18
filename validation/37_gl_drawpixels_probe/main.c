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

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static void fill_rgba(uint8_t *pixels, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < w * h; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }
}

static void fill_bgra(uint8_t *pixels, int w, int h, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < w * h; ++i) {
        pixels[i * 4 + 0] = b;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = r;
        pixels[i * 4 + 3] = 255;
    }
}

static void run_static_probe(void)
{
    uint8_t rgba[8 * 8 * 4];
    uint8_t bgra[6 * 6 * 4];
    uint8_t rgb_padded[2 * 12];
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLfloat raster[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint first_x = 0;
    GLint first_y = 0;

    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.6f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos3f(-2.60f, -2.00f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    expect_bool("raster position query", glIsEnabled(GL_CURRENT_RASTER_POSITION_VALID) == GL_TRUE && raster[0] > 100.0f && glGetError() == GL_NO_ERROR, 0);

    fill_rgba(rgba, 8, 8, 230, 30, 20);
    glDrawPixels(8, 8, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glReadPixels((GLint)raster[0] + 3, (GLint)raster[1] + 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("drawpixels rgba readback", pixel_rgb(pixel, 230, 30, 20) && glGetError() == GL_NO_ERROR, 1);
    first_x = (GLint)raster[0];
    first_y = (GLint)raster[1];

    glRasterPos3f(-2.20f, -2.00f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    fill_bgra(bgra, 6, 6, 20, 220, 70);
    glDrawPixels(6, 6, GL_BGRA, GL_UNSIGNED_BYTE, bgra);
    glReadPixels((GLint)raster[0] + 2, (GLint)raster[1] + 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("drawpixels bgra readback", pixel_rgb(pixel, 20, 220, 70) && glGetError() == GL_NO_ERROR, 2);

    memset(rgb_padded, 0, sizeof(rgb_padded));
    rgb_padded[0] = 50; rgb_padded[1] = 70; rgb_padded[2] = 240;
    rgb_padded[3] = 50; rgb_padded[4] = 70; rgb_padded[5] = 240;
    rgb_padded[6] = 50; rgb_padded[7] = 70; rgb_padded[8] = 240;
    rgb_padded[12] = 240; rgb_padded[13] = 230; rgb_padded[14] = 30;
    rgb_padded[15] = 240; rgb_padded[16] = 230; rgb_padded[17] = 30;
    rgb_padded[18] = 240; rgb_padded[19] = 230; rgb_padded[20] = 30;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glRasterPos3f(-1.80f, -2.00f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glDrawPixels(3, 2, GL_RGB, GL_UNSIGNED_BYTE, rgb_padded);
    glReadPixels((GLint)raster[0] + 1, (GLint)raster[1] + 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("drawpixels unpack alignment", pixel_rgb(pixel, 240, 230, 30) && glGetError() == GL_NO_ERROR, 3);

    glRasterPos3f(-1.20f, -2.00f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glCopyPixels(first_x, first_y, 8, 8, GL_COLOR);
    glReadPixels((GLint)raster[0] + 3, (GLint)raster[1] + 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("copy pixels color", pixel_rgb(pixel, 230, 30, 20) && glGetError() == GL_NO_ERROR, 4);

    glDrawPixels(1, 1, GL_RGBA, GL_FLOAT, rgba);
    expect_bool("invalid drawpixels type", glGetError() == GL_INVALID_ENUM, 5);

    glDrawPixels(1, 1, GL_TEXTURE_2D, GL_UNSIGNED_BYTE, rgba);
    expect_bool("invalid drawpixels format", glGetError() == GL_INVALID_ENUM, 6);

    glCopyPixels(0, 0, 1, 1, GL_TEXTURE_2D);
    expect_bool("invalid copypixels type", glGetError() == GL_INVALID_ENUM, 7);

    glDrawPixels(-1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    expect_bool("invalid drawpixels size", glGetError() == GL_INVALID_VALUE, 8);

    glRasterPos3f(0.0f, 0.0f, 10.0f);
    expect_bool("invalid raster position", glIsEnabled(GL_CURRENT_RASTER_POSITION_VALID) == GL_FALSE && glGetError() == GL_NO_ERROR, 9);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
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
    glColor3f(0.90f, 0.15f, 0.12f);
    glBegin(GL_QUADS);
    glVertex3f(-2.4f, 1.15f, 0.0f);
    glVertex3f(-1.1f, 1.15f, 0.0f);
    glVertex3f(-1.1f, 0.10f, 0.0f);
    glVertex3f(-2.4f, 0.10f, 0.0f);
    glEnd();

    glColor3f(0.15f, 0.80f, 0.25f);
    glBegin(GL_QUADS);
    glVertex3f(-0.65f, 1.15f, 0.0f);
    glVertex3f(0.65f, 1.15f, 0.0f);
    glVertex3f(0.65f, 0.10f, 0.0f);
    glVertex3f(-0.65f, 0.10f, 0.0f);
    glEnd();

    glColor3f(0.20f, 0.35f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(1.1f, 1.15f, 0.0f);
    glVertex3f(2.4f, 1.15f, 0.0f);
    glVertex3f(2.4f, 0.10f, 0.0f);
    glVertex3f(1.1f, 0.10f, 0.0f);
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
    debugPrint("NXGL drawpixels probe starting\n");

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

        nxglSwapBuffers("NXGL draw/copy pixels", all_passed() ? "draw/copy/read checks passed" : "one or more pixel checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
