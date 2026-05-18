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

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d < 0.001f;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 5;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b) && p[3] == 255;
}

static void reset_pixel_transfer(void)
{
    GLfloat identity[2] = { 0.0f, 1.0f };
    glPixelZoom(1.0f, 1.0f);
    glPixelTransferf(GL_RED_SCALE, 1.0f);
    glPixelTransferf(GL_GREEN_SCALE, 1.0f);
    glPixelTransferf(GL_BLUE_SCALE, 1.0f);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0f);
    glPixelTransferf(GL_RED_BIAS, 0.0f);
    glPixelTransferf(GL_GREEN_BIAS, 0.0f);
    glPixelTransferf(GL_BLUE_BIAS, 0.0f);
    glPixelTransferf(GL_ALPHA_BIAS, 0.0f);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 2, identity);
    glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 2, identity);
    glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 2, identity);
    glPixelMapfv(GL_PIXEL_MAP_A_TO_A, 2, identity);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_ALPHA_TEST);
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_probe(void)
{
    uint8_t src[4 * 4 * 4];
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLfloat fv[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat raster[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint iv[4] = { 0, 0, 0, 0 };
    GLushort usv[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    reset_pixel_transfer();
    glGetFloatv(GL_ZOOM_X, &fv[0]);
    glGetFloatv(GL_ZOOM_Y, &fv[1]);
    glGetFloatv(GL_RED_SCALE, &fv[2]);
    glGetIntegerv(GL_PIXEL_MAP_R_TO_R_SIZE, iv);
    ok = near_float(fv[0], 1.0f) && near_float(fv[1], 1.0f) &&
         near_float(fv[2], 1.0f) && iv[0] == 2 && consume_error(GL_NO_ERROR);
    expect_bool("default pixel transfer", ok, 0);

    memset(src, 0, sizeof(src));
    for (int i = 0; i < 4 * 4; ++i) {
        src[i * 4 + 0] = 230;
        src[i * 4 + 1] = 20;
        src[i * 4 + 2] = 20;
        src[i * 4 + 3] = 255;
    }
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(-0.02f, -0.02f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glPixelZoom(2.0f, 2.0f);
    glDrawPixels(2, 2, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glReadPixels((GLint)raster[0] + 3, (GLint)raster[1] + 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb(pixel, 230, 20, 20) && consume_error(GL_NO_ERROR);
    expect_bool("pixel zoom draw", ok, 1);

    reset_pixel_transfer();
    src[0] = 128; src[1] = 128; src[2] = 0; src[3] = 255;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glPixelTransferf(GL_RED_SCALE, 0.5f);
    glPixelTransferf(GL_GREEN_BIAS, 0.25f);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb(pixel, 64, 192, 0) && consume_error(GL_NO_ERROR);
    expect_bool("scale bias draw", ok, 2);

    reset_pixel_transfer();
    src[0] = 0; src[1] = 0; src[2] = 0; src[3] = 255;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(-0.20f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    GLint src_x = (GLint)raster[0];
    GLint src_y = (GLint)raster[1];
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glPixelTransferf(GL_BLUE_BIAS, 1.0f);
    glCopyPixels(src_x, src_y, 1, 1, GL_COLOR);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR);
    expect_bool("copy pixels transfer", ok, 3);

    reset_pixel_transfer();
    {
        GLfloat invert[2] = { 1.0f, 0.0f };
        GLfloat queried[2] = { 0.0f, 0.0f };
        glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 2, invert);
        glGetPixelMapfv(GL_PIXEL_MAP_R_TO_R, queried);
        src[0] = 255; src[1] = 255; src[2] = 255; src[3] = 255;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glRasterPos3f(0.0f, 0.0f, 0.0f);
        glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
        glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
        glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        ok = near_float(queried[0], 1.0f) && near_float(queried[1], 0.0f) &&
             pixel_rgb(pixel, 0, 255, 255) && consume_error(GL_NO_ERROR);
    }
    expect_bool("pixel map draw query", ok, 4);

    reset_pixel_transfer();
    {
        GLushort map[3] = { 0, 32768, 65535 };
        glPixelMapusv(GL_PIXEL_MAP_G_TO_G, 3, map);
        glGetPixelMapusv(GL_PIXEL_MAP_G_TO_G, usv);
        glGetIntegerv(GL_PIXEL_MAP_G_TO_G_SIZE, iv);
        ok = iv[0] == 3 && usv[0] == 0 && near_byte((uint8_t)(usv[1] >> 8), 128) &&
             usv[2] == 65535 && consume_error(GL_NO_ERROR);
    }
    expect_bool("pixel map ushort query", ok, 5);

    reset_pixel_transfer();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glPixelZoom(2.0f, 3.0f);
    glPixelTransferf(GL_RED_BIAS, 0.5f);
    glEndList();
    glPixelZoom(1.0f, 1.0f);
    glPixelTransferf(GL_RED_BIAS, 0.0f);
    glCallList(list);
    glGetFloatv(GL_ZOOM_X, &fv[0]);
    glGetFloatv(GL_ZOOM_Y, &fv[1]);
    glGetFloatv(GL_RED_BIAS, &fv[2]);
    ok = near_float(fv[0], 2.0f) && near_float(fv[1], 3.0f) &&
         near_float(fv[2], 0.5f) && consume_error(GL_NO_ERROR);
    expect_bool("display-list transfer", ok, 6);

    reset_pixel_transfer();
    {
        GLfloat invert[2] = { 1.0f, 0.0f };
        GLfloat queried[2] = { 0.0f, 0.0f };
        list = glGenLists(1);
        glNewList(list, GL_COMPILE);
        glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 2, invert);
        glEndList();
        glCallList(list);
        glGetPixelMapfv(GL_PIXEL_MAP_B_TO_B, queried);
        ok = near_float(queried[0], 1.0f) && near_float(queried[1], 0.0f) && consume_error(GL_NO_ERROR);
    }
    expect_bool("display-list pixel map", ok, 7);

    reset_pixel_transfer();
    glPixelZoom(0.0f, 1.0f);
    src[0] = 255; src[1] = 255; src[2] = 255; src[3] = 255;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glDrawPixels(1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("zero zoom skips draw", ok, 8);

    glPixelTransferf(GL_TEXTURE_2D, 1.0f);
    ok = consume_error(GL_INVALID_ENUM);
    glPixelMapfv(GL_TEXTURE_2D, 2, fv);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 0, fv);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetPixelMapfv(GL_PIXEL_MAP_R_TO_R, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("pixel transfer validation", ok, 9);

    reset_pixel_transfer();
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static void draw_visual_quad(float x, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.34f, 0.34f, 0.0f);
    glVertex3f(x + 0.34f, 0.34f, 0.0f);
    glVertex3f(x + 0.34f, -0.34f, 0.0f);
    glVertex3f(x - 0.34f, -0.34f, 0.0f);
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
    debugPrint("NXGL pixel transfer probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_pixel_transfer();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

        draw_visual_quad(-0.75f, 0.15f, 0.30f, 0.90f);
        draw_visual_quad(0.0f, 0.90f, 0.80f, 0.18f);
        draw_visual_quad(0.75f, 0.85f, 0.16f, 0.20f);
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL pixel transfer", all_passed() ? "all checks passed" : "pixel transfer check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
