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
    return d <= 4;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= 0.002f;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_pixel_store(void)
{
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
}

static void reset_state(void)
{
    reset_pixel_store();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glPixelZoom(1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void set_rgba(uint8_t *pixels, int w, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p = pixels + ((y * w) + x) * 4;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
}

static void make_skipped_patch(uint8_t *pixels)
{
    memset(pixels, 0x11, 5 * 4 * 4);
    set_rgba(pixels, 5, 2, 1, 230, 20, 20);
    set_rgba(pixels, 5, 3, 1, 20, 220, 40);
    set_rgba(pixels, 5, 2, 2, 30, 60, 235);
    set_rgba(pixels, 5, 3, 2, 245, 245, 245);
}

static bool check_patch(const uint8_t *pixels, int stride_pixels, int x, int y)
{
    const uint8_t *p = pixels + ((y * stride_pixels) + x) * 4;
    return pixel_rgb(p + 0, 230, 20, 20) &&
           pixel_rgb(p + 4, 20, 220, 40) &&
           pixel_rgb(p + stride_pixels * 4, 30, 60, 235) &&
           pixel_rgb(p + stride_pixels * 4 + 4, 245, 245, 245);
}

static void set_unpack_skip(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 5);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
}

static void set_pack_skip(void)
{
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 5);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 2);
}

static void run_probe(void)
{
    uint8_t src[5 * 4 * 4];
    uint8_t tight[2 * 2 * 4];
    uint8_t packed[5 * 4 * 4];
    uint8_t black[2 * 2 * 4];
    uint8_t pixel[4];
    GLfloat raster[4];
    GLuint texture = 0;
    GLuint list = 0;
    GLint iv[12] = { 0 };
    GLfloat depth_src[5 * 4];
    GLfloat depth = 0.0f;
    GLubyte stencil_src[5 * 4];
    GLubyte stencil = 0;
    GLubyte bits[3] = { 0, 0x30, 0x20 };
    bool ok;

    reset_state();
    glPixelStorei(GL_PACK_ROW_LENGTH, 7);
    glPixelStorei(GL_PACK_SKIP_ROWS, 2);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 3);
    glPixelStorei(GL_PACK_SWAP_BYTES, GL_TRUE);
    glPixelStorei(GL_PACK_LSB_FIRST, GL_TRUE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 5);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);
    glGetIntegerv(GL_PACK_ROW_LENGTH, &iv[0]);
    glGetIntegerv(GL_PACK_SKIP_ROWS, &iv[1]);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &iv[2]);
    glGetIntegerv(GL_PACK_SWAP_BYTES, &iv[3]);
    glGetIntegerv(GL_PACK_LSB_FIRST, &iv[4]);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &iv[5]);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &iv[6]);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &iv[7]);
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &iv[8]);
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &iv[9]);
    ok = iv[0] == 7 && iv[1] == 2 && iv[2] == 3 && iv[3] == GL_TRUE && iv[4] == GL_TRUE &&
         iv[5] == 5 && iv[6] == 1 && iv[7] == 2 && iv[8] == GL_TRUE && iv[9] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("pixelstore state queries", ok, 0);

    reset_state();
    make_skipped_patch(src);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(-1.8f, -1.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    set_unpack_skip();
    glDrawPixels(2, 2, GL_RGBA, GL_UNSIGNED_BYTE, src);
    reset_pixel_store();
    memset(tight, 0, sizeof(tight));
    glReadPixels((GLint)raster[0], (GLint)raster[1], 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    ok = check_patch(tight, 2, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("drawpixels unpack row skip", ok, 1);

    reset_state();
    memset(tight, 0, sizeof(tight));
    set_rgba(tight, 2, 0, 0, 230, 20, 20);
    set_rgba(tight, 2, 1, 0, 20, 220, 40);
    set_rgba(tight, 2, 0, 1, 30, 60, 235);
    set_rgba(tight, 2, 1, 1, 245, 245, 245);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glRasterPos3f(-1.4f, -1.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glDrawPixels(2, 2, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    memset(packed, 0x55, sizeof(packed));
    set_pack_skip();
    glReadPixels((GLint)raster[0], (GLint)raster[1], 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, packed);
    ok = check_patch(packed, 5, 2, 1) && packed[0] == 0x55 && consume_error(GL_NO_ERROR);
    expect_bool("readpixels pack row skip", ok, 2);

    reset_state();
    make_skipped_patch(src);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    set_unpack_skip();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, src);
    reset_pixel_store();
    memset(tight, 0, sizeof(tight));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    ok = check_patch(tight, 2, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("teximage unpack row skip", ok, 3);

    memset(packed, 0x44, sizeof(packed));
    set_pack_skip();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, packed);
    ok = check_patch(packed, 5, 2, 1) && packed[0] == 0x44 && consume_error(GL_NO_ERROR);
    expect_bool("getteximage pack row skip", ok, 4);

    reset_state();
    memset(black, 0, sizeof(black));
    for (int i = 0; i < 4; ++i) black[i * 4 + 3] = 255;
    memset(src, 0x22, sizeof(src));
    set_rgba(src, 5, 2, 1, 240, 225, 30);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    set_unpack_skip();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, src);
    reset_pixel_store();
    memset(tight, 0, sizeof(tight));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    ok = pixel_rgb(tight + ((1 * 2 + 1) * 4), 240, 225, 30) && consume_error(GL_NO_ERROR);
    expect_bool("texsubimage unpack row skip", ok, 5);

    reset_state();
    for (int i = 0; i < 5 * 4; ++i) {
        depth_src[i] = 0.91f;
        stencil_src[i] = 3;
    }
    depth_src[1 * 5 + 2] = 0.37f;
    stencil_src[1 * 5 + 2] = 77;
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    set_unpack_skip();
    glDrawPixels(1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, depth_src);
    glDrawPixels(1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil_src);
    reset_pixel_store();
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glReadPixels((GLint)raster[0], (GLint)raster[1], 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    ok = near_float(depth, 0.37f) && stencil == 77 && consume_error(GL_NO_ERROR);
    expect_bool("depth stencil unpack row skip", ok, 6);

    reset_state();
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 0.85f, 0.9f);
    glRasterPos3f(0.6f, -1.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 8);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
    glBitmap(2, 2, 0.0f, 0.0f, 0.0f, 0.0f, bits);
    reset_pixel_store();
    memset(tight, 0, sizeof(tight));
    glReadPixels((GLint)raster[0], (GLint)raster[1], 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    ok = pixel_rgb(tight, 0, 217, 230) && pixel_rgb(tight + 4, 0, 217, 230) &&
         pixel_rgb(tight + 8, 0, 217, 230) && pixel_rgb(tight + 12, 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("bitmap unpack row skip", ok, 7);

    reset_state();
    make_skipped_patch(src);
    list = glGenLists(1);
    set_unpack_skip();
    glNewList(list, GL_COMPILE);
    glDrawPixels(2, 2, GL_RGBA, GL_UNSIGNED_BYTE, src);
    glEndList();
    reset_pixel_store();
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos3f(1.1f, -1.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    glCallList(list);
    memset(tight, 0, sizeof(tight));
    glReadPixels((GLint)raster[0], (GLint)raster[1], 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, tight);
    ok = check_patch(tight, 2, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("display list unpack capture", ok, 8);

    reset_state();
    glPixelStorei(GL_PACK_ROW_LENGTH, -1);
    ok = consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, -1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_PACK_ALIGNMENT, 3);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_UNPACK_LSB_FIRST, 2);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPixelStorei(0xdead, 0);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("pixelstore validation", ok, 9);
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
    debugPrint("NXGL pixelstore stride probe starting\n");

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

        glColor3f(0.85f, 0.20f, 0.15f);
        glBegin(GL_QUADS);
        glVertex3f(-2.4f, 1.05f, 0.0f);
        glVertex3f(-0.9f, 1.05f, 0.0f);
        glVertex3f(-0.9f, 0.10f, 0.0f);
        glVertex3f(-2.4f, 0.10f, 0.0f);
        glEnd();

        glColor3f(0.10f, 0.70f, 0.95f);
        glBegin(GL_QUADS);
        glVertex3f(-0.65f, 0.85f, 0.0f);
        glVertex3f(0.65f, 0.85f, 0.0f);
        glVertex3f(0.65f, -0.25f, 0.0f);
        glVertex3f(-0.65f, -0.25f, 0.0f);
        glEnd();

        glColor3f(0.95f, 0.80f, 0.15f);
        glBegin(GL_QUADS);
        glVertex3f(0.95f, 1.05f, 0.0f);
        glVertex3f(2.4f, 1.05f, 0.0f);
        glVertex3f(2.4f, 0.10f, 0.0f);
        glVertex3f(0.95f, 0.10f, 0.0f);
        glEnd();

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL pixelstore stride probe", all_passed() ? "pack/unpack row and skip checks passed" : "one or more pixelstore checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
