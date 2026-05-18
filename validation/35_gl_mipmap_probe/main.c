#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static GLuint tex_complete;
static GLuint tex_incomplete;
static bool results[10];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void make_level(uint8_t *pixels, int w, int h, int level)
{
    static const uint8_t colors[5][3] = {
        { 255,  40,  30 },
        {  30, 220,  60 },
        {  40, 100, 255 },
        { 240, 210,  40 },
        { 255, 255, 255 }
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            bool alt = ((x / (w > 8 ? 8 : 1)) ^ (y / (h > 8 ? 8 : 1))) != 0;
            uint8_t *p = pixels + (y * w + x) * 4;
            p[0] = alt ? colors[level][0] : (uint8_t)(colors[level][0] / 4);
            p[1] = alt ? colors[level][1] : (uint8_t)(colors[level][1] / 4);
            p[2] = alt ? colors[level][2] : (uint8_t)(colors[level][2] / 4);
            p[3] = 255;
        }
    }
}

static void upload_full_chain(GLuint texture)
{
    uint8_t level0[32 * 32 * 4];
    uint8_t level1[16 * 16 * 4];
    uint8_t level2[8 * 8 * 4];
    uint8_t level3[4 * 4 * 4];
    uint8_t level4[2 * 2 * 4];
    uint8_t level5[1 * 1 * 4];

    make_level(level0, 32, 32, 0);
    make_level(level1, 16, 16, 1);
    make_level(level2, 8, 8, 2);
    make_level(level3, 4, 4, 3);
    make_level(level4, 2, 2, 4);
    make_level(level5, 1, 1, 4);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, level0);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, level1);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, level2);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, level3);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, level4);
    glTexImage2D(GL_TEXTURE_2D, 5, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, level5);
}

static void run_static_probe(void)
{
    GLuint textures[2] = { 0, 0 };
    GLint value = 0;
    uint8_t red[32 * 32 * 4];
    uint8_t sub[4 * 4 * 4];

    glGenTextures(2, textures);
    tex_complete = textures[0];
    tex_incomplete = textures[1];
    expect_bool("texture names allocated", tex_complete != 0 && tex_incomplete != 0, 0);

    upload_full_chain(tex_complete);
    expect_bool("full mip chain upload", glGetError() == GL_NO_ERROR, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &value);
    expect_bool("max level parameter query", value == 5 && glGetError() == GL_NO_ERROR, 2);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 2, GL_TEXTURE_WIDTH, &value);
    expect_bool("level 2 width query", value == 8 && glGetError() == GL_NO_ERROR, 3);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 5, GL_TEXTURE_HEIGHT, &value);
    expect_bool("level 5 height query", value == 1 && glGetError() == GL_NO_ERROR, 4);

    memset(sub, 255, sizeof(sub));
    glTexSubImage2D(GL_TEXTURE_2D, 2, 2, 2, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, sub);
    expect_bool("level 2 subimage update", glGetError() == GL_NO_ERROR, 5);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 6, GL_TEXTURE_WIDTH, &value);
    expect_bool("undefined level returns zero", value == 0 && glGetError() == GL_NO_ERROR, 6);

    make_level(red, 32, 32, 0);
    glBindTexture(GL_TEXTURE_2D, tex_incomplete);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, red);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    expect_bool("incomplete texture state accepted", glGetError() == GL_NO_ERROR, 7);

    glTexImage2D(GL_TEXTURE_2D, -1, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, red);
    expect_bool("negative level rejected", glGetError() == GL_INVALID_VALUE, 8);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 20);
    expect_bool("invalid base level rejected", glGetError() == GL_INVALID_VALUE, 9);
}

static void draw_textured_quad(float x, GLuint texture)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x - 0.95f,  0.95f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x + 0.95f,  0.95f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x + 0.95f, -0.95f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x - 0.95f, -0.95f, 0.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.08f, 0.0f);
    glVertex3f(x + 0.16f, -2.08f, 0.0f);
    glVertex3f(x + 0.16f, -2.34f, 0.0f);
    glVertex3f(x - 0.16f, -2.34f, 0.0f);
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
    debugPrint("NXGL mipmap probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_static_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.5f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_BLEND);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);

        draw_textured_quad(-1.35f, tex_complete);
        draw_textured_quad(1.35f, tex_incomplete);

        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.65f + (float)i * 0.58f, results[i]);
        }

        nxglSwapBuffers("NXGL mipmaps", all_passed() ? "mip levels/completeness passed" : "one or more mipmap checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
