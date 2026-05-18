#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[15];
static GLuint checker_texture;
static GLuint solid_texture;
static uint8_t checker_pixels[32 * 32 * 4];
static uint8_t solid_pixel[4];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.0005f;
}

static bool color_matches(const GLfloat *actual, const GLfloat *expected)
{
    return near_float(actual[0], expected[0]) &&
           near_float(actual[1], expected[1]) &&
           near_float(actual[2], expected[2]) &&
           near_float(actual[3], expected[3]);
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) {
        d = -d;
    }
    return d <= 8;
}

static uint8_t byte_from_float(GLfloat value)
{
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 1.0f) {
        return 255;
    }
    return (uint8_t)(value * 255.0f + 0.5f);
}

static bool pixel_matches(const uint8_t *actual, const GLfloat *expected)
{
    return near_byte(actual[0], byte_from_float(expected[0])) &&
           near_byte(actual[1], byte_from_float(expected[1])) &&
           near_byte(actual[2], byte_from_float(expected[2]));
}

static void make_checker_texture(void)
{
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bool bright = ((x / 8) ^ (y / 8)) == 0;
            uint8_t *p = checker_pixels + ((y * 32 + x) * 4);
            p[0] = bright ? 240 : 30;
            p[1] = bright ? 220 : 70;
            p[2] = bright ? 80 : 180;
            p[3] = 255;
        }
    }

    glGenTextures(1, &checker_texture);
    glBindTexture(GL_TEXTURE_2D, checker_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, checker_pixels);

    glGenTextures(1, &solid_texture);
}

static void run_static_probe(void)
{
    GLfloat color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat env_color[4] = { 0.25f, 0.50f, 0.75f, 1.0f };
    GLfloat list_color[4] = { 0.90f, 0.20f, 0.40f, 0.80f };
    GLint mode = 0;
    GLint add_mode = GL_ADD;
    GLuint list;

    glActiveTexture(GL_TEXTURE0);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
    expect_bool("default env state",
                mode == GL_MODULATE &&
                color_matches(color, (GLfloat[4]){ 0.0f, 0.0f, 0.0f, 0.0f }) &&
                consume_error(GL_NO_ERROR),
                0);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    expect_bool("integer env mode setter", mode == GL_REPLACE && consume_error(GL_NO_ERROR), 1);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (GLfloat)GL_DECAL);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    expect_bool("float env mode setter", mode == GL_DECAL && consume_error(GL_NO_ERROR), 2);

    glTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &add_mode);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    expect_bool("vector env mode setter", mode == GL_ADD && consume_error(GL_NO_ERROR), 3);

    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
    expect_bool("env color query", color_matches(color, env_color) && consume_error(GL_NO_ERROR), 4);

    glActiveTexture(GL_TEXTURE1);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    results[5] = mode == GL_MODULATE && consume_error(GL_NO_ERROR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    results[5] = results[5] && mode == GL_BLEND && consume_error(GL_NO_ERROR);
    glActiveTexture(GL_TEXTURE0);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    expect_bool("per-unit env state", results[5] && mode == GL_ADD && consume_error(GL_NO_ERROR), 5);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, list_color);
    glEndList();
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);
    glCallList(list);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &mode);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
    expect_bool("display list texenv replay",
                mode == GL_MODULATE && color_matches(color, list_color) && consume_error(GL_NO_ERROR),
                6);
    glDeleteLists(list, 1);

    glTexEnvi(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    expect_bool("invalid texenv target", consume_error(GL_INVALID_ENUM), 7);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_MIN_FILTER, GL_MODULATE);
    expect_bool("invalid texenv pname", consume_error(GL_INVALID_ENUM), 8);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_TEXTURE_2D);
    expect_bool("invalid texenv mode", consume_error(GL_INVALID_ENUM), 9);

    glActiveTexture(GL_TEXTURE0);
}

static void upload_solid_texture(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    solid_pixel[0] = byte_from_float(r);
    solid_pixel[1] = byte_from_float(g);
    solid_pixel[2] = byte_from_float(b);
    solid_pixel[3] = byte_from_float(a);
    glBindTexture(GL_TEXTURE_2D, solid_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, solid_pixel);
}

static void draw_quad(float x0, float y0, float x1, float y1, GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x0, y1, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(x1, y1, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(x1, y0, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x0, y0, 0.0f);
    glEnd();
}

static void run_render_case(GLenum mode, const GLfloat *env, const GLfloat *expected, int slot, const char *name)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    upload_solid_texture(0.50f, 0.25f, 0.75f, 1.0f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
    if (env != NULL) {
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    draw_quad(-0.75f, -0.75f, 0.75f, 0.75f, 0.25f, 0.50f, 0.80f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool(name, pixel_matches(pixel, expected) && consume_error(GL_NO_ERROR), slot);
}

static void run_render_probe(void)
{
    GLfloat blend_env[4] = { 0.90f, 0.20f, 0.10f, 1.0f };
    GLfloat modulate_expected[4] = { 0.125f, 0.125f, 0.600f, 1.0f };
    GLfloat replace_expected[4] = { 0.500f, 0.250f, 0.750f, 1.0f };
    GLfloat decal_expected[4] = { 0.500f, 0.250f, 0.750f, 1.0f };
    GLfloat blend_expected[4] = { 0.575f, 0.425f, 0.275f, 1.0f };
    GLfloat add_expected[4] = { 0.750f, 0.750f, 1.000f, 1.0f };

    run_render_case(GL_MODULATE, NULL, modulate_expected, 10, "render modulate");
    run_render_case(GL_REPLACE, NULL, replace_expected, 11, "render replace");
    run_render_case(GL_DECAL, NULL, decal_expected, 12, "render decal");
    run_render_case(GL_BLEND, blend_env, blend_expected, 13, "render blend");
    run_render_case(GL_ADD, NULL, add_expected, 14, "render add");
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
    glEnable(GL_TEXTURE_2D);
}

static void draw_visual_scene(void)
{
    GLfloat blend_color[4] = { 0.1f, 0.6f, 0.9f, 1.0f };

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, checker_texture);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    draw_quad(-2.75f, -0.75f, -1.05f, 1.05f, 1.0f, 0.35f, 0.30f);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    draw_quad(-0.85f, -0.75f, 0.85f, 1.05f, 0.25f, 1.0f, 0.45f);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, blend_color);
    draw_quad(1.05f, -0.75f, 2.75f, 1.05f, 0.40f, 0.55f, 1.0f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 15; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL texture env probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    make_checker_texture();
    run_static_probe();
    run_render_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);

        draw_visual_scene();
        for (int i = 0; i < 15; ++i) {
            draw_result_bar(-2.80f + (float)i * 0.40f, results[i]);
        }

        nxglSwapBuffers("NXGL texture env", all_passed() ? "texture env checks passed" : "one or more texture env checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
