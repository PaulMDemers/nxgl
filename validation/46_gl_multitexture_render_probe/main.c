#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[18];
static GLuint tex0;
static GLuint tex1;
static GLuint tex2;
static GLuint tex3;
static uint8_t pixel0[4];
static uint8_t pixel1[4];
static uint8_t strip2[2 * 4];
static uint8_t strip3[2 * 4];

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
    if (value <= 0.0f) return 0;
    if (value >= 1.0f) return 255;
    return (uint8_t)(value * 255.0f + 0.5f);
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 8;
}

static bool pixel_matches(const uint8_t *actual, const GLfloat *expected)
{
    return near_byte(actual[0], byte_from_float(expected[0])) &&
           near_byte(actual[1], byte_from_float(expected[1])) &&
           near_byte(actual[2], byte_from_float(expected[2]));
}

static void upload_solid_texture(GLenum unit, GLuint texture, uint8_t *pixel, GLfloat r, GLfloat g, GLfloat b)
{
    pixel[0] = byte_from_float(r);
    pixel[1] = byte_from_float(g);
    pixel[2] = byte_from_float(b);
    pixel[3] = 255;
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void upload_strip_texture(GLenum unit, GLuint texture, uint8_t *pixels,
                                 GLfloat left_r, GLfloat left_g, GLfloat left_b,
                                 GLfloat right_r, GLfloat right_g, GLfloat right_b)
{
    pixels[0] = byte_from_float(left_r);
    pixels[1] = byte_from_float(left_g);
    pixels[2] = byte_from_float(left_b);
    pixels[3] = 255;
    pixels[4] = byte_from_float(right_r);
    pixels[5] = byte_from_float(right_g);
    pixels[6] = byte_from_float(right_b);
    pixels[7] = 255;
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

static void setup_textures(void)
{
    glGenTextures(1, &tex0);
    glGenTextures(1, &tex1);
    glGenTextures(1, &tex2);
    glGenTextures(1, &tex3);
    upload_solid_texture(GL_TEXTURE0, tex0, pixel0, 0.50f, 0.25f, 0.75f);
    upload_solid_texture(GL_TEXTURE1, tex1, pixel1, 0.20f, 0.80f, 0.50f);
    upload_strip_texture(GL_TEXTURE2, tex2, strip2, 0.90f, 0.05f, 0.05f, 0.05f, 0.65f, 0.20f);
    upload_strip_texture(GL_TEXTURE3, tex3, strip3, 0.85f, 0.15f, 0.05f, 0.20f, 0.10f, 0.55f);
    glActiveTexture(GL_TEXTURE0);
}

static void reset_texture_env(GLenum unit)
{
    glActiveTexture(unit);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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

static void reset_both_texture_envs(void)
{
    reset_texture_env(GL_TEXTURE0);
    reset_texture_env(GL_TEXTURE1);
    glActiveTexture(GL_TEXTURE0);
}

static void reset_all_texture_envs(void)
{
    reset_texture_env(GL_TEXTURE0);
    reset_texture_env(GL_TEXTURE1);
    reset_texture_env(GL_TEXTURE2);
    reset_texture_env(GL_TEXTURE3);
    glActiveTexture(GL_TEXTURE0);
}

static void draw_multitex_quad(GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glVertex3f(-0.75f, 0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 0.0f);
    glVertex3f(0.75f, 0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 1.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 1.0f, 1.0f);
    glVertex3f(0.75f, -0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 1.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 1.0f);
    glVertex3f(-0.75f, -0.75f, 0.0f);
    glEnd();
}

static void draw_unit23_quad(GLfloat unit2_u, GLfloat unit3_u)
{
    glColor3f(0.90f, 0.80f, 0.70f);
    glBegin(GL_QUADS);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE2, unit2_u, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE3, unit3_u, 0.0f);
    glVertex3f(-0.75f, 0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE2, unit2_u, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE3, unit3_u, 0.0f);
    glVertex3f(0.75f, 0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE2, unit2_u, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE3, unit3_u, 0.0f);
    glVertex3f(0.75f, -0.75f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE0, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE2, unit2_u, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE3, unit3_u, 0.0f);
    glVertex3f(-0.75f, -0.75f, 0.0f);
    glEnd();
}

static void render_center_pixel(uint8_t *pixel)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    draw_multitex_quad(0.90f, 0.80f, 0.70f);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void render_unit23_pixel(uint8_t *pixel, GLfloat unit2_u, GLfloat unit3_u)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    draw_unit23_quad(unit2_u, unit3_u);
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void run_static_probe(void)
{
    GLint value = 0;

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
    expect_bool("unit0 texture binding", value == (GLint)tex0 && consume_error(GL_NO_ERROR), 0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &value);
    expect_bool("unit1 texture binding", value == (GLint)tex1 && consume_error(GL_NO_ERROR), 1);
    expect_bool("unit1 texture enabled", glIsEnabled(GL_TEXTURE_2D) == GL_TRUE && consume_error(GL_NO_ERROR), 2);

    glMultiTexCoord2f(GL_TEXTURE3 + 1, 0.0f, 0.0f);
    expect_bool("invalid multitex coord unit", consume_error(GL_INVALID_ENUM), 3);

    glClientActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE, &value);
    expect_bool("client unit preserved", value == GL_TEXTURE1 && consume_error(GL_NO_ERROR), 4);
    glClientActiveTexture(GL_TEXTURE0);
}

static void run_render_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    GLfloat tex0_expected[4] = { 0.45f, 0.20f, 0.525f, 1.0f };
    GLfloat tex01_expected[4] = { 0.09f, 0.16f, 0.2625f, 1.0f };
    GLfloat unit1_replace_expected[4] = { 0.20f, 0.80f, 0.50f, 1.0f };
    GLfloat unit1_add_expected[4] = { 0.65f, 1.00f, 1.00f, 1.0f };
    GLfloat unit1_interpolate_expected[4] = { 0.3875f, 0.50f, 0.50625f, 1.0f };
    GLfloat unit2_replace_expected[4] = { 0.05f, 0.65f, 0.20f, 1.0f };
    GLfloat unit3_replace_expected[4] = { 0.20f, 0.10f, 0.55f, 1.0f };
    GLfloat unit23_add_expected[4] = { 0.25f, 0.75f, 0.75f, 1.0f };
    GLfloat crossbar_unit2_expected[4] = { 0.05f, 0.65f, 0.20f, 1.0f };
    GLfloat crossbar_add_expected[4] = { 0.55f, 0.90f, 0.95f, 1.0f };
    GLfloat env[4] = { 0.25f, 0.50f, 0.75f, 1.0f };

    reset_both_texture_envs();
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    render_center_pixel(pixel);
    expect_bool("unit0 render readback", pixel_matches(pixel, tex0_expected) && consume_error(GL_NO_ERROR), 5);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    render_center_pixel(pixel);
    expect_bool("unit0 plus unit1 readback", pixel_matches(pixel, tex01_expected) && consume_error(GL_NO_ERROR), 6);

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    render_center_pixel(pixel);
    expect_bool("unit1 disable restores unit0", pixel_matches(pixel, tex0_expected) && consume_error(GL_NO_ERROR), 7);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    render_center_pixel(pixel);
    expect_bool("unit0 replace then unit1", pixel_matches(pixel, (GLfloat[4]){ 0.10f, 0.20f, 0.375f, 1.0f }) && consume_error(GL_NO_ERROR), 8);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    render_center_pixel(pixel);
    expect_bool("multitex mode reset", pixel_matches(pixel, tex01_expected) && consume_error(GL_NO_ERROR), 9);

    reset_both_texture_envs();
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE0);
    render_center_pixel(pixel);
    expect_bool("unit1 replace stage", pixel_matches(pixel, unit1_replace_expected) && consume_error(GL_NO_ERROR), 10);

    reset_both_texture_envs();
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
    glActiveTexture(GL_TEXTURE0);
    render_center_pixel(pixel);
    expect_bool("unit1 combine add previous", pixel_matches(pixel, unit1_add_expected) && consume_error(GL_NO_ERROR), 11);

    reset_both_texture_envs();
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, env);
    glActiveTexture(GL_TEXTURE0);
    render_center_pixel(pixel);
    expect_bool("unit1 combine interpolate", pixel_matches(pixel, unit1_interpolate_expected) && consume_error(GL_NO_ERROR), 12);

    reset_all_texture_envs();
    for (GLenum unit = GL_TEXTURE0; unit <= GL_TEXTURE3; ++unit) {
        glActiveTexture(unit);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE0);
    render_unit23_pixel(pixel, 1.0f, 0.0f);
    expect_bool("unit2 distinct coord replace", pixel_matches(pixel, unit2_replace_expected) && consume_error(GL_NO_ERROR), 13);

    reset_all_texture_envs();
    for (GLenum unit = GL_TEXTURE0; unit <= GL_TEXTURE3; ++unit) {
        glActiveTexture(unit);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex3);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE0);
    render_unit23_pixel(pixel, 0.0f, 1.0f);
    expect_bool("unit3 distinct coord replace", pixel_matches(pixel, unit3_replace_expected) && consume_error(GL_NO_ERROR), 14);

    reset_all_texture_envs();
    for (GLenum unit = GL_TEXTURE0; unit <= GL_TEXTURE3; ++unit) {
        glActiveTexture(unit);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex3);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
    glActiveTexture(GL_TEXTURE0);
    render_unit23_pixel(pixel, 1.0f, 1.0f);
    expect_bool("unit2 plus unit3 chain", pixel_matches(pixel, unit23_add_expected) && consume_error(GL_NO_ERROR), 15);

    reset_all_texture_envs();
    for (GLenum unit = GL_TEXTURE0; unit <= GL_TEXTURE3; ++unit) {
        glActiveTexture(unit);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE2);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    render_unit23_pixel(pixel, 1.0f, 0.0f);
    expect_bool("unit1 crossbar source texture2", pixel_matches(pixel, crossbar_unit2_expected) && consume_error(GL_NO_ERROR), 16);

    reset_all_texture_envs();
    for (GLenum unit = GL_TEXTURE0; unit <= GL_TEXTURE3; ++unit) {
        glActiveTexture(unit);
        glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex0);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tex3);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE2);
    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0);
    glActiveTexture(GL_TEXTURE0);
    render_unit23_pixel(pixel, 1.0f, 1.0f);
    expect_bool("unit3 crossbar add tex2 tex0", pixel_matches(pixel, crossbar_add_expected) && consume_error(GL_NO_ERROR), 17);
}

static void draw_result_bar(float x, bool pass)
{
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.15f, 0.0f);
    glVertex3f(x + 0.16f, -2.40f, 0.0f);
    glVertex3f(x - 0.16f, -2.40f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 18; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL multitexture probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    setup_textures();
    run_static_probe();
    run_render_probe();

    for (;;) {
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex0);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex1);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        draw_multitex_quad(0.90f, 0.80f, 0.70f);

        for (int i = 0; i < 18; ++i) {
            draw_result_bar(-2.95f + (float)i * 0.34f, results[i]);
        }

        nxglSwapBuffers("NXGL multitexture", all_passed() ? "multitexture checks passed" : "one or more multitexture checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
