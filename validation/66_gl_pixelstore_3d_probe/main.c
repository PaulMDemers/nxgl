#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define TEX_W 2
#define TEX_H 2
#define TEX_D 2
#define STORE_W 5
#define STORE_H 4
#define STORE_D 3

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

static uint8_t *stored_pixel(uint8_t *volume, int x, int y, int z)
{
    return volume + (((z * STORE_H + y) * STORE_W + x) * 4);
}

static uint8_t *tight_pixel(uint8_t *volume, int x, int y, int z)
{
    return volume + (((z * TEX_H + y) * TEX_W + x) * 4);
}

static void set_stored(uint8_t *volume, int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p = stored_pixel(volume, x, y, z);
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
}

static void set_tight(uint8_t *volume, int x, int y, int z, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t *p = tight_pixel(volume, x, y, z);
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = 255;
}

static bool pixel_rgb(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b)
{
    return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
}

static void make_skipped_volume(uint8_t *storage, uint8_t *expected)
{
    memset(storage, 0x19, STORE_W * STORE_H * STORE_D * 4);
    memset(expected, 0, TEX_W * TEX_H * TEX_D * 4);

    for (int z = 0; z < TEX_D; ++z) {
        for (int y = 0; y < TEX_H; ++y) {
            for (int x = 0; x < TEX_W; ++x) {
                uint8_t r = (uint8_t)(60 + z * 120 + x * 30);
                uint8_t g = (uint8_t)(40 + y * 90 + z * 20);
                uint8_t b = (uint8_t)(200 - x * 40 - y * 30);
                set_stored(storage, x + 2, y + 1, z + 1, r, g, b);
                set_tight(expected, x, y, z, r, g, b);
            }
        }
    }
}

static bool volume_equal(const uint8_t *a, const uint8_t *b, int bytes)
{
    for (int i = 0; i < bytes; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static bool packed_volume_matches(const uint8_t *packed, const uint8_t *expected)
{
    for (int z = 0; z < TEX_D; ++z) {
        for (int y = 0; y < TEX_H; ++y) {
            for (int x = 0; x < TEX_W; ++x) {
                const uint8_t *p = packed + ((((z + 1) * STORE_H + (y + 1)) * STORE_W + (x + 2)) * 4);
                const uint8_t *e = expected + (((z * TEX_H + y) * TEX_W + x) * 4);
                if (!pixel_rgb(p, e[0], e[1], e[2])) return false;
            }
        }
    }
    return packed[0] == 0x55;
}

static void reset_pixel_store(void)
{
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
}

static void set_unpack_3d_skip(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, STORE_W);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 1);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, STORE_H);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 1);
}

static void set_pack_3d_skip(void)
{
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, STORE_W);
    glPixelStorei(GL_PACK_SKIP_ROWS, 1);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 2);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, STORE_H);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 1);
}

static void run_probe(void)
{
    uint8_t storage[STORE_W * STORE_H * STORE_D * 4];
    uint8_t expected[TEX_W * TEX_H * TEX_D * 4];
    uint8_t out[TEX_W * TEX_H * TEX_D * 4];
    uint8_t packed[STORE_W * STORE_H * STORE_D * 4];
    uint8_t compact[TEX_W * TEX_H * STORE_D * 4];
    uint8_t black[TEX_W * TEX_H * TEX_D * 4];
    GLuint tex = 0;
    GLint iv[8] = { 0 };
    bool ok;

    reset_pixel_store();
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, STORE_H);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 1);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, STORE_H);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 1);
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &iv[0]);
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &iv[1]);
    glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &iv[2]);
    glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &iv[3]);
    ok = iv[0] == STORE_H && iv[1] == 1 && iv[2] == STORE_H && iv[3] == 1 && consume_error(GL_NO_ERROR);
    expect_bool("3d pixelstore queries", ok, 0);

    make_skipped_volume(storage, expected);
    reset_pixel_store();
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    set_unpack_3d_skip();
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_W, TEX_H, TEX_D, 0, GL_RGBA, GL_UNSIGNED_BYTE, storage);
    reset_pixel_store();
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = volume_equal(out, expected, sizeof(out)) && consume_error(GL_NO_ERROR);
    expect_bool("teximage3d unpack image skip", ok, 1);

    memset(packed, 0x55, sizeof(packed));
    set_pack_3d_skip();
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, packed);
    ok = packed_volume_matches(packed, expected) && consume_error(GL_NO_ERROR);
    expect_bool("getteximage3d pack image skip", ok, 2);

    memset(black, 0, sizeof(black));
    for (int i = 0; i < TEX_W * TEX_H * TEX_D; ++i) black[i * 4 + 3] = 255;
    memset(storage, 0x22, sizeof(storage));
    set_stored(storage, 2, 1, 1, 240, 220, 35);
    reset_pixel_store();
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_W, TEX_H, TEX_D, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    set_unpack_3d_skip();
    glTexSubImage3D(GL_TEXTURE_3D, 0, 1, 1, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, storage);
    reset_pixel_store();
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = pixel_rgb(tight_pixel(out, 1, 1, 1), 240, 220, 35) && consume_error(GL_NO_ERROR);
    expect_bool("texsubimage3d unpack image skip", ok, 3);

    make_skipped_volume(storage, expected);
    memset(compact, 0x33, sizeof(compact));
    memcpy(compact + TEX_W * TEX_H * 4, expected, sizeof(expected));
    reset_pixel_store();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_W, TEX_H, TEX_D, 0, GL_RGBA, GL_UNSIGNED_BYTE, compact);
    reset_pixel_store();
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = volume_equal(out, expected, sizeof(out)) && consume_error(GL_NO_ERROR);
    expect_bool("default image height expands", ok, 4);

    memset(compact, 0x55, sizeof(compact));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, 0);
    glPixelStorei(GL_PACK_SKIP_IMAGES, 1);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, compact);
    ok = compact[0] == 0x55 && volume_equal(compact + TEX_W * TEX_H * 4, expected, sizeof(expected)) && consume_error(GL_NO_ERROR);
    expect_bool("default pack image height expands", ok, 5);

    reset_pixel_store();
    glPixelStorei(GL_PACK_IMAGE_HEIGHT, -1);
    ok = consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_PACK_SKIP_IMAGES, -1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, -1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, -1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("3d pixelstore validation", ok, 6);

    glBindTexture(GL_TEXTURE_2D, tex);
    ok = consume_error(GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_3D, tex);
    ok = ok && consume_error(GL_NO_ERROR);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    expect_bool("3d target guard rails", ok, 7);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, TEX_W, TEX_H, TEX_D, 0, GL_RGB, GL_FLOAT, storage);
    ok = consume_error(GL_INVALID_ENUM);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 1, 1, 1, GL_BGR, GL_UNSIGNED_BYTE, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    expect_bool("3d validation still intact", ok, 8);

    reset_pixel_store();
    glGetIntegerv(GL_PACK_IMAGE_HEIGHT, &iv[0]);
    glGetIntegerv(GL_PACK_SKIP_IMAGES, &iv[1]);
    glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &iv[2]);
    glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &iv[3]);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == 0 && iv[3] == 0 && consume_error(GL_NO_ERROR);
    expect_bool("3d pixelstore defaults", ok, 9);
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
    debugPrint("NXGL 3D pixelstore probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

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

        nxglSwapBuffers("NXGL 3D pixelstore", all_passed() ? "3d pack/unpack image checks passed" : "one or more 3d pixelstore checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
