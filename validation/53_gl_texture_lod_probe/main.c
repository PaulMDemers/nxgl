#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[16];

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d < 0.001f;
}

static void fill_solid(uint8_t *pixels, int width, int height, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < width * height; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }
}

static void fill_solid_3d(uint8_t *pixels, int width, int height, int depth, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < width * height * depth; ++i) {
        pixels[i * 4 + 0] = r;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = b;
        pixels[i * 4 + 3] = 255;
    }
}

static void upload_lod_chain(GLuint texture)
{
    uint8_t level0[32 * 32 * 4];
    uint8_t level1[16 * 16 * 4];
    uint8_t level2[8 * 8 * 4];
    uint8_t level3[4 * 4 * 4];

    fill_solid(level0, 32, 32, 220, 20, 20);
    fill_solid(level1, 16, 16, 20, 210, 30);
    fill_solid(level2, 8, 8, 30, 70, 230);
    fill_solid(level3, 4, 4, 230, 205, 30);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, level0);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, level1);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, level2);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, level3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);
}

static void upload_3d_lod_chain(GLuint texture)
{
    uint8_t level0[8 * 8 * 4 * 4];
    uint8_t level1[4 * 4 * 2 * 4];
    uint8_t level2[2 * 2 * 1 * 4];
    uint8_t level3[1 * 1 * 1 * 4];

    fill_solid_3d(level0, 8, 8, 4, 205, 30, 40);
    fill_solid_3d(level1, 4, 4, 2, 35, 205, 55);
    fill_solid_3d(level2, 2, 2, 1, 35, 80, 225);
    fill_solid_3d(level3, 1, 1, 1, 230, 210, 45);

    glBindTexture(GL_TEXTURE_3D, texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 8, 8, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, level0);
    glTexImage3D(GL_TEXTURE_3D, 1, GL_RGBA, 4, 4, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, level1);
    glTexImage3D(GL_TEXTURE_3D, 2, GL_RGBA, 2, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, level2);
    glTexImage3D(GL_TEXTURE_3D, 3, GL_RGBA, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, level3);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 3);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 1000.0f);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_LOD_BIAS, 0.0f);
}

static void draw_center_textured_quad(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
    glEnd();
}

static void draw_center_textured_3d_quad(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_TEXTURE_3D);
    glBegin(GL_QUADS);
    glTexCoord3f(0.0f, 0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord3f(1.0f, 0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
    glTexCoord3f(1.0f, 1.0f, 0.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
    glTexCoord3f(0.0f, 1.0f, 0.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
    glEnd();
}

static bool center_matches(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    int dr;
    int dg;
    int db;

    glReadPixels(320, 240, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    if (glGetError() != GL_NO_ERROR) {
        return false;
    }
    dr = (int)pixel[0] - (int)r;
    dg = (int)pixel[1] - (int)g;
    db = (int)pixel[2] - (int)b;
    if (dr < 0) dr = -dr;
    if (dg < 0) dg = -dg;
    if (db < 0) db = -db;
    return dr <= 4 && dg <= 4 && db <= 4;
}

static void run_probe(void)
{
    GLuint tex[3] = { 0, 0, 0 };
    GLuint tex3d = 0;
    GLfloat f = 0.0f;
    GLfloat fv[2] = { 0.0f, 0.0f };
    GLint iv[2] = { 0, 0 };
    GLboolean residences[3] = { GL_FALSE, GL_FALSE, GL_FALSE };
    GLuint list;
    bool ok;

    glGenTextures(3, tex);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    expect_bool("generated texture names", tex[0] != 0 && tex[1] != 0 && glIsTexture(tex[0]) == GL_TRUE && consume_error(GL_NO_ERROR), 0);

    ok = true;
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, &f);
    ok = ok && near_float(f, -1000.0f) && consume_error(GL_NO_ERROR);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, &f);
    ok = ok && near_float(f, 1000.0f) && consume_error(GL_NO_ERROR);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, &f);
    ok = ok && near_float(f, 0.0f) && consume_error(GL_NO_ERROR);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &f);
    ok = ok && near_float(f, 1.0f) && consume_error(GL_NO_ERROR);
    expect_bool("default lod state", ok, 1);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -2.5f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 5.5f);
    fv[0] = 0.75f;
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, fv);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, &f);
    ok = near_float(f, -2.5f) && consume_error(GL_NO_ERROR);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, &f);
    ok = ok && near_float(f, 5.5f) && consume_error(GL_NO_ERROR);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, &f);
    ok = ok && near_float(f, 0.75f) && consume_error(GL_NO_ERROR);
    expect_bool("float lod params", ok, 2);

    iv[0] = -3;
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, iv);
    iv[0] = 7;
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, iv);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, iv);
    ok = iv[0] == -3 && consume_error(GL_NO_ERROR);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, iv);
    ok = ok && iv[0] == 7 && consume_error(GL_NO_ERROR);
    expect_bool("integer lod vectors", ok, 3);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 2.5f);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &f);
    ok = near_float(f, 1.0f) && consume_error(GL_NO_ERROR);
    fv[0] = -0.5f;
    fv[1] = 0.25f;
    glPrioritizeTextures(2, tex, fv);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &f);
    ok = ok && near_float(f, 0.0f) && consume_error(GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &f);
    ok = ok && near_float(f, 0.25f) && consume_error(GL_NO_ERROR);
    expect_bool("priority clamp", ok, 4);

    residences[0] = residences[1] = residences[2] = GL_FALSE;
    ok = glAreTexturesResident(2, tex, residences) == GL_TRUE &&
         residences[0] == GL_TRUE && residences[1] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    tex[2] = 15;
    residences[2] = GL_TRUE;
    ok = ok && glAreTexturesResident(3, tex, residences) == GL_FALSE &&
         residences[2] == GL_FALSE && consume_error(GL_NO_ERROR);
    expect_bool("resident query", ok, 5);

    glBindTexture(GL_TEXTURE_2D, tex[0]);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.25f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glEndList();
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1000);
    glCallList(list);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, &f);
    ok = near_float(f, -1.25f) && consume_error(GL_NO_ERROR);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, iv);
    ok = ok && iv[0] == 1 && consume_error(GL_NO_ERROR);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, iv);
    ok = ok && iv[0] == 4 && consume_error(GL_NO_ERROR);
    expect_bool("display-list lod replay", ok, 6);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLfloat)GL_LINEAR);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iv);
    ok = iv[0] == GL_LINEAR && consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLfloat)GL_NEAREST);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &f);
    ok = ok && near_float(f, (GLfloat)GL_NEAREST) && consume_error(GL_NO_ERROR);
    expect_bool("float enum params", ok, 7);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 123.0f);
    ok = consume_error(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, -1);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, NULL);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glGetTexParameterfv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_MIN_LOD, &f);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("validation paths", ok, 8);

    upload_lod_chain(tex[1]);
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 2.0f);
    draw_center_textured_quad();
    ok = ok && center_matches(30, 70, 230);
    expect_bool("lod bias selects mip level", ok, 10);

    upload_lod_chain(tex[1]);
    ok = consume_error(GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    draw_center_textured_quad();
    ok = ok && center_matches(20, 210, 30);
    expect_bool("base/max level clamp render", ok, 11);

    upload_lod_chain(tex[1]);
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 3.0f);
    draw_center_textured_quad();
    ok = ok && center_matches(230, 205, 30);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 3.0f);
    draw_center_textured_quad();
    ok = ok && center_matches(20, 210, 30);
    expect_bool("min/max lod clamp render", ok, 12);

    glGenTextures(1, &tex3d);
    upload_3d_lod_chain(tex3d);
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_LOD_BIAS, 2.0f);
    draw_center_textured_3d_quad();
    ok = ok && center_matches(35, 80, 225);
    expect_bool("3d lod bias selects mip", ok, 13);

    upload_3d_lod_chain(tex3d);
    ok = consume_error(GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 1);
    draw_center_textured_3d_quad();
    ok = ok && center_matches(35, 205, 55);
    expect_bool("3d base/max level clamp", ok, 14);

    upload_3d_lod_chain(tex3d);
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_LOD, 3.0f);
    draw_center_textured_3d_quad();
    ok = ok && center_matches(230, 210, 45);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 1.0f);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_LOD_BIAS, 3.0f);
    draw_center_textured_3d_quad();
    ok = ok && center_matches(35, 205, 55);
    expect_bool("3d min/max lod clamp", ok, 15);

    glDeleteTextures(1, tex);
    expect_bool("delete clears texture name", glIsTexture(tex[0]) == GL_FALSE && consume_error(GL_NO_ERROR), 9);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static void draw_swatch(float x, float y, float r, float g, float b)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.38f, y + 0.34f, 0.0f);
    glVertex3f(x + 0.38f, y + 0.34f, 0.0f);
    glVertex3f(x + 0.38f, y - 0.34f, 0.0f);
    glVertex3f(x - 0.38f, y - 0.34f, 0.0f);
    glEnd();
}

static bool all_passed(void)
{
    for (int i = 0; i < 16; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL texture LOD probe starting\n");

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
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);

        draw_swatch(-0.90f, 0.28f, 0.12f, 0.42f, 0.90f);
        draw_swatch(0.0f, 0.28f, 0.12f, 0.82f, 0.32f);
        draw_swatch(0.90f, 0.28f, 0.88f, 0.72f, 0.15f);
        for (int i = 0; i < 16; ++i) {
            draw_result_bar(-2.90f + (float)i * 0.38f, results[i]);
        }

        nxglSwapBuffers("NXGL texture LOD", all_passed() ? "texture lod checks passed" : "one or more texture lod checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
