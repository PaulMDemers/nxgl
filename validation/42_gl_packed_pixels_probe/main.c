#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static GLuint tex_a;
static GLuint tex_b;
static GLuint tex_c;
static bool results[8];
static uint8_t diag_pixel[4];
static GLenum diag_error;

#define TEX_W 32
#define TEX_H 32

static uint16_t rgb565[TEX_W * TEX_H];
static uint16_t rgb565_rev[TEX_W * TEX_H];
static uint16_t rgba4444[TEX_W * TEX_H];
static uint16_t rgba5551[TEX_W * TEX_H];
static uint32_t rgba8888_rev[TEX_W * TEX_H];
static uint8_t out[TEX_W * TEX_H * 4];
static uint8_t packed_out[TEX_W * TEX_H * 2];

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
    return d <= 8;
}

static bool pixel_rgba(const uint8_t *p, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return near_byte(p[0], r) && near_byte(p[1], g) &&
           near_byte(p[2], b) && near_byte(p[3], a);
}

static uint16_t pack565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((((uint16_t)r * 31u + 127u) / 255u) << 11) |
           (uint16_t)((((uint16_t)g * 63u + 127u) / 255u) << 5) |
           (uint16_t)(((uint16_t)b * 31u + 127u) / 255u);
}

static uint16_t pack565_rev(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)(((uint16_t)r * 31u + 127u) / 255u) |
           (uint16_t)((((uint16_t)g * 63u + 127u) / 255u) << 5) |
           (uint16_t)((((uint16_t)b * 31u + 127u) / 255u) << 11);
}

static uint16_t pack4444(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint16_t)((((uint16_t)r * 15u + 127u) / 255u) << 12) |
           (uint16_t)((((uint16_t)g * 15u + 127u) / 255u) << 8) |
           (uint16_t)((((uint16_t)b * 15u + 127u) / 255u) << 4) |
           (uint16_t)(((uint16_t)a * 15u + 127u) / 255u);
}

static uint16_t pack5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint16_t)((((uint16_t)r * 31u + 127u) / 255u) << 11) |
           (uint16_t)((((uint16_t)g * 31u + 127u) / 255u) << 6) |
           (uint16_t)((((uint16_t)b * 31u + 127u) / 255u) << 1) |
           (uint16_t)(a >= 128 ? 1u : 0u);
}

static uint32_t pack8888_rev(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | ((uint32_t)a << 24);
}

static void write_u16(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xff);
    dst[1] = (uint8_t)((value >> 8) & 0xff);
    dst[2] = (uint8_t)((value >> 16) & 0xff);
    dst[3] = (uint8_t)((value >> 24) & 0xff);
}

static uint16_t read_u16(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static void upload_packed_textures(void)
{
    GLuint textures[3];
    uint16_t sub565[1];

    for (int i = 0; i < TEX_W * TEX_H; ++i) {
        int selector = i & 3;
        if (selector == 0) {
            rgb565[i] = pack565(255, 0, 0);
            rgb565_rev[i] = pack565_rev(255, 0, 0);
            rgba4444[i] = pack4444(255, 0, 0, 255);
            rgba5551[i] = pack5551(255, 0, 0, 255);
            rgba8888_rev[i] = pack8888_rev(20, 40, 240, 255);
        } else if (selector == 1) {
            rgb565[i] = pack565(0, 255, 0);
            rgb565_rev[i] = pack565_rev(0, 255, 0);
            rgba4444[i] = pack4444(0, 255, 0, 128);
            rgba5551[i] = pack5551(0, 255, 0, 255);
            rgba8888_rev[i] = pack8888_rev(220, 30, 70, 255);
        } else if (selector == 2) {
            rgb565[i] = pack565(0, 0, 255);
            rgb565_rev[i] = pack565_rev(0, 0, 255);
            rgba4444[i] = pack4444(0, 0, 255, 64);
            rgba5551[i] = pack5551(0, 0, 255, 0);
            rgba8888_rev[i] = pack8888_rev(10, 200, 90, 255);
        } else {
            rgb565[i] = pack565(255, 255, 255);
            rgb565_rev[i] = pack565_rev(255, 255, 255);
            rgba4444[i] = pack4444(255, 255, 255, 0);
            rgba5551[i] = pack5551(255, 255, 255, 0);
            rgba8888_rev[i] = pack8888_rev(255, 220, 30, 255);
        }
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(3, textures);
    tex_a = textures[0];
    tex_b = textures[1];
    tex_c = textures[2];
    expect_bool("texture names allocated", tex_a != 0 && tex_b != 0 && tex_c != 0 && glGetError() == GL_NO_ERROR, 0);

    glBindTexture(GL_TEXTURE_2D, tex_a);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_W, TEX_H, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, rgb565);
    diag_error = glGetError();
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    diag_pixel[0] = out[0];
    diag_pixel[1] = out[1];
    diag_pixel[2] = out[2];
    diag_pixel[3] = out[3];
    expect_bool("565 upload readback", pixel_rgba(out + 0, 255, 0, 0, 255) &&
                pixel_rgba(out + 4, 0, 255, 0, 255) &&
                pixel_rgba(out + 8, 0, 0, 255, 255) &&
                diag_error == GL_NO_ERROR && glGetError() == GL_NO_ERROR, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_W, TEX_H, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, rgb565_rev);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("565 rev upload readback", pixel_rgba(out + 0, 255, 0, 0, 255) &&
                pixel_rgba(out + 4, 0, 255, 0, 255) &&
                pixel_rgba(out + 8, 0, 0, 255, 255) &&
                glGetError() == GL_NO_ERROR, 2);

    glBindTexture(GL_TEXTURE_2D, tex_b);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, rgba4444);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("4444 upload readback", pixel_rgba(out + 0, 255, 0, 0, 255) &&
                near_byte(out[7], 136) && near_byte(out[11], 68) &&
                pixel_rgba(out + 12, 255, 255, 255, 0) &&
                glGetError() == GL_NO_ERROR, 3);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, rgba5551);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("5551 upload readback", pixel_rgba(out + 0, 255, 0, 0, 255) &&
                pixel_rgba(out + 4, 0, 255, 0, 255) &&
                pixel_rgba(out + 8, 0, 0, 255, 0) &&
                glGetError() == GL_NO_ERROR, 4);

    glBindTexture(GL_TEXTURE_2D, tex_c);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, rgba8888_rev);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("8888 rev upload readback", pixel_rgba(out + 0, 20, 40, 240, 255) &&
                pixel_rgba(out + 4, 220, 30, 70, 255) &&
                pixel_rgba(out + 12, 255, 220, 30, 255) &&
                glGetError() == GL_NO_ERROR, 5);

    sub565[0] = pack565(255, 255, 0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 3, 3, 1, 1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, sub565);
    memset(out, 0, sizeof(out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, out);
    expect_bool("packed subimage update", pixel_rgba(out + ((3 * TEX_W + 3) * 4), 255, 255, 0, 255) && glGetError() == GL_NO_ERROR, 6);

    memset(packed_out, 0, sizeof(packed_out));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, packed_out);
    results[7] = read_u16(packed_out + ((3 * TEX_W + 3) * 2)) == pack565(255, 255, 0) && glGetError() == GL_NO_ERROR;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_SHORT_5_6_5, rgb565);
    results[7] = results[7] && glGetError() == GL_INVALID_ENUM && glGetError() == GL_NO_ERROR;
    expect_bool("packed getteximage and invalid combo", results[7], 7);

    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

static void draw_textured_quad(float x, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - 0.75f,  0.75f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 0.75f,  0.75f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 0.75f, -0.75f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - 0.75f, -0.75f, 0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -1.95f, 0.0f);
    glVertex3f(x + 0.18f, -1.95f, 0.0f);
    glVertex3f(x + 0.18f, -2.25f, 0.0f);
    glVertex3f(x - 0.18f, -2.25f, 0.0f);
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

static uint32_t result_mask(void)
{
    uint32_t mask = 0;
    for (int i = 0; i < 8; ++i) {
        if (results[i]) {
            mask |= 1u << i;
        }
    }
    return mask;
}

int main(void)
{
    char detail[64];

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL packed pixels probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    upload_packed_textures();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.5f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_BLEND);

        draw_textured_quad(-2.0f, tex_a);
        draw_textured_quad(0.0f, tex_b);
        draw_textured_quad(2.0f, tex_c);

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.45f + (float)i * 0.7f, results[i]);
        }

        if (all_passed()) {
            strcpy(detail, "packed pixel checks passed");
        } else {
            sprintf(detail, "mask %02lX p%02X%02X%02X e%04lX", (unsigned long)result_mask(),
                    diag_pixel[0], diag_pixel[1], diag_pixel[2], (unsigned long)diag_error);
        }
        nxglSwapBuffers("NXGL packed pixels", detail);
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
