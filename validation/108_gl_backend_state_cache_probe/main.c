#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];
static GLuint tex_red;
static GLuint tex_green;
static GLuint tex_blue;
static GLuint tex_white;
static GLuint cube_tex;
static uint8_t solid_pixels[4][4];
static uint8_t cube_faces[6][4 * 4 * 4];

static const GLenum cube_targets[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

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
    return d <= 12;
}

static bool pixel_rgb(const uint8_t pixel[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(pixel[0], r) && near_byte(pixel[1], g) && near_byte(pixel[2], b);
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void upload_solid_texture(GLuint texture, uint8_t *pixel, GLfloat r, GLfloat g, GLfloat b)
{
    pixel[0] = byte_from_float(r);
    pixel[1] = byte_from_float(g);
    pixel[2] = byte_from_float(b);
    pixel[3] = 255;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void setup_textures(void)
{
    GLuint textures[4];

    glGenTextures(4, textures);
    tex_red = textures[0];
    tex_green = textures[1];
    tex_blue = textures[2];
    tex_white = textures[3];

    glActiveTexture(GL_TEXTURE0);
    upload_solid_texture(tex_red, solid_pixels[0], 1.0f, 0.0f, 0.0f);
    upload_solid_texture(tex_green, solid_pixels[1], 0.0f, 1.0f, 0.0f);
    upload_solid_texture(tex_blue, solid_pixels[2], 0.0f, 0.0f, 1.0f);
    upload_solid_texture(tex_white, solid_pixels[3], 1.0f, 1.0f, 1.0f);

    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    for (int face = 0; face < 6; ++face) {
        uint8_t r = face == 4 ? 255 : 32;
        uint8_t g = face == 4 ? 0 : 32;
        uint8_t b = face == 4 ? 255 : 32;
        for (int i = 0; i < 4 * 4; ++i) {
            cube_faces[face][i * 4 + 0] = r;
            cube_faces[face][i * 4 + 1] = g;
            cube_faces[face][i * 4 + 2] = b;
            cube_faces[face][i * 4 + 3] = 255;
        }
        glTexImage2D(cube_targets[face], 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, cube_faces[face]);
    }
    glActiveTexture(GL_TEXTURE0);
}

static void reset_state(void)
{
    for (int unit = 0; unit < 4; ++unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glDisable(GL_TEXTURE_1D);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_3D);
        glDisable(GL_TEXTURE_CUBE_MAP);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    (void)glGetError();
}

static void clear_rgb(GLfloat r, GLfloat g, GLfloat b)
{
    glClearColor(r, g, b, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void read_center(uint8_t pixel[4])
{
    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_quad(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                      GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(x - w, y + h, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(x + w, y + h, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(x + w, y - h, 0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(x - w, y - h, 0.0f);
    glEnd();
}

static void draw_quad_cw(GLfloat r, GLfloat g, GLfloat b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, 0.55f, 0.0f);
    glVertex3f(0.55f, -0.55f, 0.0f);
    glVertex3f(-0.55f, -0.55f, 0.0f);
    glEnd();
}

static void draw_cube_quad(GLfloat x)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - 0.55f, 0.55f, 0.0f);
    glTexCoord3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x + 0.55f, 0.55f, 0.0f);
    glTexCoord3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x + 0.55f, -0.55f, 0.0f);
    glTexCoord3f(0.0f, 0.0f, 1.0f);
    glVertex3f(x - 0.55f, -0.55f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    bool ok;

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_TEXTURE_2D);
    draw_quad(0.0f, 0.0f, 0.30f, 0.30f, 0.0f, 0.0f, 1.0f, 1.0f);
    read_center(pixel);
    expect_bool("shader texture color transitions", pixel_rgb(pixel, 0, 0, 255) && consume_error(GL_NO_ERROR), 0);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBindTexture(GL_TEXTURE_2D, tex_red);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    read_center(pixel);
    expect_bool("texture descriptor switch", pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 1);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 0.0f, 0.0f, 1.0f, 1.0f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 0.50f, 0.25f, 0.75f, 1.0f);
    read_center(pixel);
    expect_bool("texture env shader switch", pixel_rgb(pixel, 0, 64, 0) && consume_error(GL_NO_ERROR), 2);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_red);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    glActiveTexture(GL_TEXTURE0);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    read_center(pixel);
    expect_bool("multitexture disables unit1", pixel_rgb(pixel, 255, 0, 0) && consume_error(GL_NO_ERROR), 3);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_red);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_blue);
    glActiveTexture(GL_TEXTURE0);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    read_center(pixel);
    expect_bool("single texture enables unit1", pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR), 4);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    draw_cube_quad(-1.3f);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    read_center(pixel);
    expect_bool("cube to texture2d transition", pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 5);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex_green);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    draw_cube_quad(0.0f);
    read_center(pixel);
    expect_bool("texture2d to cube transition", pixel_rgb(pixel, 255, 0, 255) && consume_error(GL_NO_ERROR), 6);

    reset_state();
    clear_rgb(0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    draw_quad(-1.3f, 0.0f, 0.45f, 0.45f, 1.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_BLEND);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 1.0f, 0.0f, 0.0f, 1.0f);
    read_center(pixel);
    expect_bool("blend disable transition", pixel_rgb(pixel, 255, 0, 0) && consume_error(GL_NO_ERROR), 7);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    draw_quad_cw(1.0f, 0.0f, 0.0f);
    glDisable(GL_CULL_FACE);
    draw_quad_cw(0.0f, 1.0f, 0.0f);
    read_center(pixel);
    expect_bool("cull disable transition", pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR), 8);

    reset_state();
    clear_rgb(0.0f, 0.0f, 0.0f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 100, 100);
    draw_quad(0.0f, 0.0f, 0.55f, 0.55f, 1.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_SCISSOR_TEST);
    draw_quad(0.0f, 0.0f, 0.45f, 0.45f, 0.0f, 1.0f, 0.0f, 1.0f);
    read_center(pixel);
    ok = pixel_rgb(pixel, 0, 255, 0) && consume_error(GL_NO_ERROR);
    expect_bool("scissor disable transition", ok, 9);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.18f, -1.38f, 0.0f);
    glVertex3f(x + 0.18f, -1.38f, 0.0f);
    glVertex3f(x + 0.18f, -1.63f, 0.0f);
    glVertex3f(x - 0.18f, -1.63f, 0.0f);
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
    debugPrint("NXGL backend state cache probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    setup_textures();
    run_probe();

    for (;;) {
        reset_state();
        clear_rgb(0.02f, 0.02f, 0.04f);
        draw_quad(0.0f, 0.15f, 0.95f, 0.55f,
                  all_passed() ? 0.05f : 0.45f,
                  all_passed() ? 0.45f : 0.08f,
                  all_passed() ? 0.9f : 0.08f,
                  1.0f);
        for (int i = 0; i < 10; ++i) {
            draw_result_bar(-2.25f + (float)i * 0.50f, results[i]);
        }

        nxglSwapBuffers("NXGL backend cache", all_passed() ? "all checks passed" : "backend cache check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
