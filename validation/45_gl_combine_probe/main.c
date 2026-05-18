#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[19];
static GLuint solid_texture;
static uint8_t solid_pixel[4];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
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

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) {
        d = -d;
    }
    return d <= 8;
}

static bool pixel_matches(const uint8_t *actual, const GLfloat *expected)
{
    return near_byte(actual[0], byte_from_float(expected[0])) &&
           near_byte(actual[1], byte_from_float(expected[1])) &&
           near_byte(actual[2], byte_from_float(expected[2]));
}

static bool alpha_matches(const uint8_t *actual, GLfloat expected)
{
    return near_byte(actual[3], byte_from_float(expected));
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) {
        d = -d;
    }
    return d < 0.0005f;
}

static void upload_solid_texture(void)
{
    solid_pixel[0] = byte_from_float(0.50f);
    solid_pixel[1] = byte_from_float(0.25f);
    solid_pixel[2] = byte_from_float(0.75f);
    solid_pixel[3] = byte_from_float(0.40f);
    glBindTexture(GL_TEXTURE_2D, solid_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, solid_pixel);
}

static void reset_combine_state(void)
{
    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
    glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);
}

static void run_static_probe(void)
{
    GLint value = 0;
    GLfloat fvalue = 0.0f;
    GLfloat env_color[4] = { 0.20f, 0.60f, 0.40f, 1.0f };
    GLuint list;

    reset_combine_state();
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &value);
    expect_bool("combine env mode", value == GL_COMBINE && consume_error(GL_NO_ERROR), 0);

    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &value);
    results[1] = value == GL_MODULATE && consume_error(GL_NO_ERROR);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE0_RGB, &value);
    results[1] = results[1] && value == GL_TEXTURE && consume_error(GL_NO_ERROR);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_SOURCE1_RGB, &value);
    results[1] = results[1] && value == GL_PREVIOUS && consume_error(GL_NO_ERROR);
    expect_bool("default combine state", results[1], 1);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_SUBTRACT);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &value);
    results[2] = value == GL_SUBTRACT && consume_error(GL_NO_ERROR);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_RGB_SCALE, &fvalue);
    expect_bool("rgb combine setters", results[2] && near_float(fvalue, 2.0f) && consume_error(GL_NO_ERROR), 2);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 4.0f);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, &value);
    results[3] = value == GL_ADD && consume_error(GL_NO_ERROR);
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_ALPHA_SCALE, &fvalue);
    expect_bool("alpha combine setters", results[3] && near_float(fvalue, 4.0f) && consume_error(GL_NO_ERROR), 3);

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env_color);
    glEndList();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
    glCallList(list);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_COMBINE_RGB, &value);
    expect_bool("display list combine replay", value == GL_INTERPOLATE && consume_error(GL_NO_ERROR), 4);
    glDeleteLists(list, 1);

    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_TEXTURE_2D);
    expect_bool("invalid combine mode", consume_error(GL_INVALID_ENUM), 5);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_LIGHTING);
    expect_bool("invalid combine source", consume_error(GL_INVALID_ENUM), 6);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_TEXTURE);
    expect_bool("invalid rgb operand", consume_error(GL_INVALID_ENUM), 7);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 3.0f);
    expect_bool("invalid combine scale", consume_error(GL_INVALID_VALUE), 8);
}

static void draw_quad(GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.75f, 0.75f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.75f, 0.75f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.75f, -0.75f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.75f, -0.75f, 0.0f);
    glEnd();
}

static void draw_alpha_quad(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-0.75f, 0.75f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.75f, 0.75f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(0.75f, -0.75f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-0.75f, -0.75f, 0.0f);
    glEnd();
}

static void run_render_case(GLenum combine, const GLfloat *env, const GLfloat *expected, int slot, const char *name)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    reset_combine_state();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, combine);
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
    glBindTexture(GL_TEXTURE_2D, solid_texture);
    draw_quad(0.25f, 0.50f, 0.80f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool(name, pixel_matches(pixel, expected) && consume_error(GL_NO_ERROR), slot);
}

static void run_render_probe(void)
{
    GLfloat env[4] = { 0.20f, 0.60f, 0.40f, 1.0f };
    GLfloat modulate_expected[4] = { 0.125f, 0.125f, 0.600f, 1.0f };
    GLfloat replace_expected[4] = { 0.500f, 0.250f, 0.750f, 1.0f };
    GLfloat add_expected[4] = { 0.750f, 0.750f, 1.000f, 1.0f };
    GLfloat add_signed_expected[4] = { 0.250f, 0.250f, 1.000f, 1.0f };
    GLfloat subtract_expected[4] = { 0.250f, 0.000f, 0.000f, 1.0f };
    GLfloat interpolate_expected[4] = { 0.300f, 0.350f, 0.780f, 1.0f };
    GLfloat dot3_expected[4] = { 0.300f, 0.300f, 0.300f, 1.0f };

    run_render_case(GL_MODULATE, NULL, modulate_expected, 9, "combine render modulate");
    run_render_case(GL_REPLACE, NULL, replace_expected, 10, "combine render replace");
    run_render_case(GL_ADD, NULL, add_expected, 11, "combine render add");
    run_render_case(GL_ADD_SIGNED, NULL, add_signed_expected, 12, "combine render add signed");
    run_render_case(GL_SUBTRACT, NULL, subtract_expected, 13, "combine render subtract");
    run_render_case(GL_INTERPOLATE, env, interpolate_expected, 14, "combine render interpolate");
    run_render_case(GL_DOT3_RGB, NULL, dot3_expected, 15, "combine render dot3");
}

static void run_alpha_case(GLenum combine_alpha, GLfloat primary_alpha, const GLfloat *env, GLfloat expected, int slot, const char *name)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };

    reset_combine_state();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, combine_alpha);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_CONSTANT);
    if (env != NULL) {
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_ALPHA_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, solid_texture);
    draw_alpha_quad(0.25f, 0.50f, 0.80f, primary_alpha);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool(name, alpha_matches(pixel, expected) && consume_error(GL_NO_ERROR), slot);
}

static void run_alpha_render_probe(void)
{
    GLfloat env[4] = { 0.20f, 0.60f, 0.40f, 0.25f };

    run_alpha_case(GL_REPLACE, 0.80f, NULL, 0.40f, 16, "combine alpha replace");
    run_alpha_case(GL_ADD, 0.35f, NULL, 0.75f, 17, "combine alpha add");
    run_alpha_case(GL_INTERPOLATE, 0.80f, env, 0.70f, 18, "combine alpha interpolate");
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
    GLfloat env[4] = { 0.20f, 0.60f, 0.40f, 1.0f };

    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, solid_texture);

    reset_combine_state();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
    draw_quad(0.25f, 0.50f, 0.80f);

    glTranslatef(1.9f, 0.0f, 0.0f);
    reset_combine_state();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_SUBTRACT);
    draw_quad(0.25f, 0.50f, 0.80f);

    glTranslatef(-3.8f, 0.0f, 0.0f);
    reset_combine_state();
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env);
    draw_quad(0.25f, 0.50f, 0.80f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 19; ++i) {
        if (!results[i]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL combine probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    glGenTextures(1, &solid_texture);
    upload_solid_texture();
    run_static_probe();
    run_render_probe();
    run_alpha_render_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        draw_visual_scene();
        for (int i = 0; i < 19; ++i) {
            draw_result_bar(-2.95f + (float)i * 0.32f, results[i]);
        }

        nxglSwapBuffers("NXGL combine", all_passed() ? "combine checks passed" : "one or more combine checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
