#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 8;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPixelZoom(1.0f, 1.0f);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void clear_frame(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

static void raster_at(GLint x, GLint y)
{
    GLfloat wx = ((GLfloat)x - 320.0f) * 5.2f / 320.0f;
    GLfloat wy = ((GLfloat)y - 240.0f) * 5.2f / 240.0f;
    glRasterPos3f(wx, wy, 0.0f);
}

static void read_pixel(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void fill_rgba(uint8_t *pixels, int count, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < count; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }
}

static void run_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    uint8_t pixels[4 * 4 * 4];
    uint8_t packed[32];
    GLuint list;
    bool ok;

    reset_state();
    clear_frame();
    glReadPixels(0, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    ok = consume_error(GL_NO_ERROR);
    glReadPixels(0, 0, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    ok = ok && consume_error(GL_NO_ERROR);
    glDrawPixels(0, 1, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    ok = ok && consume_error(GL_NO_ERROR);
    glDrawPixels(1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    ok = ok && consume_error(GL_NO_ERROR);
    glCopyPixels(-4, -4, 0, 1, GL_COLOR);
    ok = ok && consume_error(GL_NO_ERROR);
    expect_bool("zero-area pixel no-ops", ok, 0);

    reset_state();
    clear_frame();
    glReadPixels(-1, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = consume_error(GL_INVALID_VALUE);
    glReadPixels(639, 0, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glReadPixels(0, 479, 1, 2, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glReadPixels(0, 0, -1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("read source bounds strict", ok, 1);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 230, 20, 20);
    raster_at(638, 240);
    glDrawPixels(4, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_pixel(637, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    read_pixel(638, 240, pixel);
    ok = ok && pixel_rgb(pixel, 230, 20, 20) && consume_error(GL_NO_ERROR);
    read_pixel(639, 240, pixel);
    ok = ok && pixel_rgb(pixel, 230, 20, 20) && consume_error(GL_NO_ERROR);
    expect_bool("draw clips right edge", ok, 2);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 20, 220, 40);
    glPixelZoom(-1.0f, 1.0f);
    raster_at(1, 240);
    glDrawPixels(4, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_pixel(0, 240, pixel);
    ok = pixel_rgb(pixel, 20, 220, 40) && consume_error(GL_NO_ERROR);
    read_pixel(1, 240, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("negative zoom clips left", ok, 3);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 30, 60, 235);
    glPixelZoom(1.0f, -1.0f);
    raster_at(320, 1);
    glDrawPixels(1, 4, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_pixel(320, 0, pixel);
    ok = pixel_rgb(pixel, 30, 60, 235) && consume_error(GL_NO_ERROR);
    read_pixel(320, 1, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("negative zoom clips bottom", ok, 4);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 245, 225, 30);
    raster_at(10, 10);
    glDrawPixels(4, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    raster_at(638, 241);
    glCopyPixels(10, 10, 4, 1, GL_COLOR);
    read_pixel(638, 241, pixel);
    ok = pixel_rgb(pixel, 245, 225, 30) && consume_error(GL_NO_ERROR);
    read_pixel(639, 241, pixel);
    ok = ok && pixel_rgb(pixel, 245, 225, 30) && consume_error(GL_NO_ERROR);
    read_pixel(637, 241, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("copy clips destination", ok, 5);

    reset_state();
    clear_frame();
    glCopyPixels(-1, 0, 1, 1, GL_COLOR);
    ok = consume_error(GL_INVALID_VALUE);
    glCopyPixels(639, 0, 2, 1, GL_COLOR);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glCopyPixels(0, 479, 1, 2, GL_COLOR);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glCopyPixels(0, 0, -1, 1, GL_COLOR);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("copy source bounds strict", ok, 6);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 90, 180, 240);
    glPixelZoom(2.0f, 2.0f);
    raster_at(638, 478);
    glDrawPixels(2, 2, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    read_pixel(638, 478, pixel);
    ok = pixel_rgb(pixel, 90, 180, 240) && consume_error(GL_NO_ERROR);
    read_pixel(639, 479, pixel);
    ok = ok && pixel_rgb(pixel, 90, 180, 240) && consume_error(GL_NO_ERROR);
    read_pixel(637, 477, pixel);
    ok = ok && pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("positive zoom clips corner", ok, 7);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 200, 70, 180);
    raster_at(638, 240);
    glDrawPixels(2, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    memset(packed, 0x55, sizeof(packed));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 4);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 1);
    glReadPixels(638, 240, 2, 1, GL_RGBA, GL_UNSIGNED_BYTE, packed);
    ok = packed[0] == 0x55 &&
         pixel_rgb(packed + 4, 200, 70, 180) &&
         pixel_rgb(packed + 8, 200, 70, 180) &&
         consume_error(GL_NO_ERROR);
    expect_bool("edge read honors pack skip", ok, 8);

    reset_state();
    clear_frame();
    fill_rgba(pixels, 4, 210, 120, 40);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    raster_at(638, 242);
    glDrawPixels(4, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glEndList();
    glCallList(list);
    glDeleteLists(list, 1);
    read_pixel(638, 242, pixel);
    ok = pixel_rgb(pixel, 210, 120, 40) && consume_error(GL_NO_ERROR);
    read_pixel(639, 242, pixel);
    ok = ok && pixel_rgb(pixel, 210, 120, 40) && consume_error(GL_NO_ERROR);
    expect_bool("display-list edge draw", ok, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
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
    debugPrint("NXGL pixel bounds probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        clear_frame();
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL pixel bounds", all_passed() ? "all checks passed" : "pixel-bounds check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
