#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static GLuint tex_rgb;
static GLuint tex_bgr;
static GLuint tex_bgra;
static bool results[8];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void make_rgb(uint8_t *pixels, int w, int h)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t *p = pixels + (y * w + x) * 3;
            bool alt = ((x / 8) ^ (y / 8)) != 0;
            p[0] = alt ? 255 : 20;
            p[1] = alt ? 20 : 220;
            p[2] = 30;
        }
    }
}

static void make_bgr(uint8_t *pixels, int w, int h)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t *p = pixels + (y * w + x) * 3;
            bool alt = ((x / 8) ^ (y / 8)) != 0;
            p[0] = alt ? 255 : 20;
            p[1] = alt ? 220 : 30;
            p[2] = alt ? 20 : 255;
        }
    }
}

static void make_bgra(uint8_t *pixels, int w, int h)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t *p = pixels + (y * w + x) * 4;
            bool alt = ((x / 8) ^ (y / 8)) != 0;
            p[0] = alt ? 40 : 255;
            p[1] = alt ? 255 : 30;
            p[2] = alt ? 255 : 180;
            p[3] = 255;
        }
    }
}

static void make_sub_rgba(uint8_t *pixels, int w, int h)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint8_t *p = pixels + (y * w + x) * 4;
            p[0] = 255;
            p[1] = 255;
            p[2] = 255;
            p[3] = 255;
        }
    }
}

static void upload_textures(void)
{
    uint8_t rgb[32 * 32 * 3];
    uint8_t bgr[32 * 32 * 3];
    uint8_t bgra[32 * 32 * 4];
    uint8_t sub[12 * 12 * 4];
    GLint value = 0;
    GLuint textures[3];

    make_rgb(rgb, 32, 32);
    make_bgr(bgr, 32, 32);
    make_bgra(bgra, 32, 32);
    make_sub_rgba(sub, 12, 12);

    glGenTextures(3, textures);
    tex_rgb = textures[0];
    tex_bgr = textures[1];
    tex_bgra = textures[2];
    expect_bool("three texture names allocated", tex_rgb != 0 && tex_bgr != 0 && tex_bgra != 0, 0);

    glBindTexture(GL_TEXTURE_2D, tex_rgb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &value);
    expect_bool("texture parameter storage", value == GL_NEAREST && glGetError() == GL_NO_ERROR, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &value);
    expect_bool("RGB upload and level width query", value == 32 && glGetError() == GL_NO_ERROR, 2);

    glBindTexture(GL_TEXTURE_2D, tex_bgr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &value);
    expect_bool("wrap parameter storage", value == GL_CLAMP_TO_EDGE && glGetError() == GL_NO_ERROR, 3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_BGR, GL_UNSIGNED_BYTE, bgr);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &value);
    expect_bool("BGR upload and level height query", value == 32 && glGetError() == GL_NO_ERROR, 4);

    glBindTexture(GL_TEXTURE_2D, tex_bgra);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_BGRA, GL_UNSIGNED_BYTE, bgra);
    expect_bool("BGRA upload conversion", glGetError() == GL_NO_ERROR, 5);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 10, 10, 12, 12, GL_RGBA, GL_UNSIGNED_BYTE, sub);
    expect_bool("RGBA subimage update", glGetError() == GL_NO_ERROR, 6);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    expect_bool("invalid MAG mipmap filter rejected", glGetError() == GL_INVALID_ENUM, 7);
}

static void draw_textured_quad(float x, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - 0.8f,  0.8f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 0.8f,  0.8f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 0.8f, -0.8f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - 0.8f, -0.8f, 0.0f);
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

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL texture upload probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    upload_textures();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.5f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_BLEND);

        draw_textured_quad(-2.0f, tex_rgb);
        draw_textured_quad(0.0f, tex_bgr);
        draw_textured_quad(2.0f, tex_bgra);

        for (int i = 0; i < 8; ++i) {
            draw_result_bar(-2.45f + (float)i * 0.7f, results[i]);
        }

        nxglSwapBuffers("NXGL texture upload", all_passed() ? "RGB/BGR/BGRA/subimage passed" : "one or more texture checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
