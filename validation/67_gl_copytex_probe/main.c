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

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static uint8_t *pixel_at(uint8_t *pixels, int w, int x, int y)
{
    return pixels + ((y * w + x) * 4);
}

static uint8_t *voxel_at(uint8_t *pixels, int w, int h, int x, int y, int z)
{
    return pixels + (((z * h + y) * w + x) * 4);
}

static void set_pixel(uint8_t *pixels, int w, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p = pixel_at(pixels, w, x, y);
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
}

static bool bytes_equal(const uint8_t *a, const uint8_t *b, int bytes)
{
    for (int i = 0; i < bytes; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static void fill_patch(uint8_t *pixels, int w, int h, uint8_t base)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            set_pixel(pixels, w, x, y,
                      (uint8_t)(base + x * 30),
                      (uint8_t)(40 + y * 55),
                      (uint8_t)(210 - x * 20 - y * 15));
        }
    }
}

static void fill_black(uint8_t *pixels, int count)
{
    memset(pixels, 0, (size_t)count * 4u);
    for (int i = 0; i < count; ++i) {
        pixels[i * 4 + 3] = 255;
    }
}

static void draw_source(const uint8_t *pixels, int w, int h, GLfloat x, GLfloat y, GLint *sx, GLint *sy)
{
    GLfloat raster[4];

    glRasterPos3f(x, y, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    *sx = (GLint)raster[0];
    *sy = (GLint)raster[1];
    glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glPixelZoom(1.0f, 1.0f);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void run_probe(void)
{
    uint8_t src4[4 * 4 * 4];
    uint8_t src2[2 * 2 * 4];
    uint8_t black2d[4 * 4 * 4];
    uint8_t black3d[4 * 4 * 2 * 4];
    uint8_t out4[4 * 4 * 4];
    uint8_t out3d[4 * 4 * 2 * 4];
    GLuint tex2d = 0;
    GLuint tex3d = 0;
    GLuint cube = 0;
    GLint sx = 0;
    GLint sy = 0;
    GLint iv = 0;
    bool ok;

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fill_patch(src4, 4, 4, 70);
    glGenTextures(1, &tex2d);
    glBindTexture(GL_TEXTURE_2D, tex2d);
    draw_source(src4, 4, 4, -1.7f, -1.1f, &sx, &sy);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sx, sy, 4, 4, 0);
    memset(out4, 0, sizeof(out4));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out4);
    ok = bytes_equal(out4, src4, sizeof(src4)) && consume_error(GL_NO_ERROR);
    expect_bool("copyteximage2d exact", ok, 0);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iv);
    ok = iv == 4 && consume_error(GL_NO_ERROR);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &iv);
    ok = ok && iv == GL_RGBA && consume_error(GL_NO_ERROR);
    expect_bool("copyteximage2d metadata", ok, 1);

    reset_state();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fill_black(black2d, 16);
    fill_patch(src2, 2, 2, 130);
    glBindTexture(GL_TEXTURE_2D, tex2d);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, black2d);
    draw_source(src2, 2, 2, -1.1f, -1.1f, &sx, &sy);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, sx, sy, 2, 2);
    memset(out4, 0, sizeof(out4));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out4);
    ok = pixel_rgb(pixel_at(out4, 4, 1, 1), src2[0], src2[1], src2[2]) &&
         pixel_rgb(pixel_at(out4, 4, 2, 1), src2[4], src2[5], src2[6]) &&
         pixel_rgb(pixel_at(out4, 4, 1, 2), src2[8], src2[9], src2[10]) &&
         pixel_rgb(pixel_at(out4, 4, 2, 2), src2[12], src2[13], src2[14]) &&
         pixel_rgb(pixel_at(out4, 4, 0, 0), 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("copytexsubimage2d offset", ok, 2);

    reset_state();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fill_patch(src4, 4, 4, 35);
    glGenTextures(1, &cube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube);
    draw_source(src4, 4, 4, -0.5f, -1.1f, &sx, &sy);
    glCopyTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, sx, sy, 4, 4, 0);
    memset(out4, 0, sizeof(out4));
    glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, GL_UNSIGNED_BYTE, out4);
    ok = bytes_equal(out4, src4, sizeof(src4)) && consume_error(GL_NO_ERROR);
    expect_bool("copyteximage cube face", ok, 3);

    reset_state();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    fill_black(black3d, 32);
    fill_patch(src2, 2, 2, 190);
    glGenTextures(1, &tex3d);
    glBindTexture(GL_TEXTURE_3D, tex3d);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 4, 4, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, black3d);
    draw_source(src2, 2, 2, 0.2f, -1.1f, &sx, &sy);
    glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 1, 1, 1, sx, sy, 2, 2);
    memset(out3d, 0, sizeof(out3d));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out3d);
    ok = pixel_rgb(voxel_at(out3d, 4, 4, 1, 1, 1), src2[0], src2[1], src2[2]) &&
         pixel_rgb(voxel_at(out3d, 4, 4, 2, 1, 1), src2[4], src2[5], src2[6]) &&
         pixel_rgb(voxel_at(out3d, 4, 4, 1, 2, 1), src2[8], src2[9], src2[10]) &&
         pixel_rgb(voxel_at(out3d, 4, 4, 2, 2, 1), src2[12], src2[13], src2[14]) &&
         pixel_rgb(voxel_at(out3d, 4, 4, 1, 1, 0), 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("copytexsubimage3d slice", ok, 4);

    reset_state();
    glBindTexture(GL_TEXTURE_2D, tex2d);
    glCopyTexImage2D(GL_TEXTURE_3D, 0, GL_RGBA, 0, 0, 1, 1, 0);
    ok = consume_error(GL_INVALID_ENUM);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX, 0, 0, 1, 1, 0);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("copyteximage invalid enums", ok, 5);

    glCopyTexImage2D(GL_TEXTURE_2D, -1, GL_RGBA, 0, 0, 1, 1, 0);
    ok = consume_error(GL_INVALID_VALUE);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 0, 1, 0);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, -1, 0, 1, 1, 0);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("copyteximage invalid values", ok, 6);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, black2d);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 3, 3, 0, 0, 2, 2);
    ok = consume_error(GL_INVALID_VALUE);
    glCopyTexSubImage2D(GL_TEXTURE_3D, 0, 0, 0, 0, 0, 1, 1);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("copytexsubimage2d guards", ok, 7);

    glBindTexture(GL_TEXTURE_3D, tex3d);
    glCopyTexSubImage3D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 0, 1, 1);
    ok = consume_error(GL_INVALID_ENUM);
    glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 2, 0, 0, 1, 1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glCopyTexSubImage3D(GL_TEXTURE_3D, 0, 3, 3, 1, 0, 0, 2, 2);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("copytexsubimage3d guards", ok, 8);

    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out3d);
    ok = consume_error(GL_NO_ERROR);
    expect_bool("post-error texture intact", ok, 9);
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
    debugPrint("NXGL copy texture probe starting\n");

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

        glColor3f(0.85f, 0.18f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(-2.35f, 0.92f, 0.0f);
        glVertex3f(-0.85f, 0.92f, 0.0f);
        glVertex3f(-0.85f, -0.20f, 0.0f);
        glVertex3f(-2.35f, -0.20f, 0.0f);
        glEnd();

        glColor3f(0.10f, 0.64f, 0.95f);
        glBegin(GL_QUADS);
        glVertex3f(-0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, 0.70f, 0.0f);
        glVertex3f(0.55f, -0.42f, 0.0f);
        glVertex3f(-0.55f, -0.42f, 0.0f);
        glEnd();

        glColor3f(0.96f, 0.80f, 0.14f);
        glBegin(GL_QUADS);
        glVertex3f(0.85f, 0.92f, 0.0f);
        glVertex3f(2.35f, 0.92f, 0.0f);
        glVertex3f(2.35f, -0.20f, 0.0f);
        glVertex3f(0.85f, -0.20f, 0.0f);
        glEnd();

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL copy texture", all_passed() ? "copy texture checks passed" : "one or more copy texture checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
