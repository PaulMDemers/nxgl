#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[11];
static GLuint cube_tex;
static uint8_t face_pixels[6][4 * 4 * 4];
static uint8_t face_mip1[6][2 * 2 * 4];
static uint8_t face_mip2[6][1 * 1 * 4];

static const GLenum faces[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

static const uint8_t face_colors[6][4] = {
    { 255, 32, 32, 255 },
    { 32, 255, 32, 255 },
    { 32, 32, 255, 255 },
    { 255, 255, 32, 255 },
    { 255, 32, 255, 255 },
    { 32, 255, 255, 255 }
};

static const uint8_t face_mip1_colors[6][4] = {
    { 160, 48, 48, 255 },
    { 48, 160, 48, 255 },
    { 48, 48, 160, 255 },
    { 160, 160, 48, 255 },
    { 160, 48, 160, 255 },
    { 48, 160, 160, 255 }
};

static const uint8_t face_mip2_colors[6][4] = {
    { 90, 24, 24, 255 },
    { 24, 90, 24, 255 },
    { 24, 24, 90, 255 },
    { 90, 90, 24, 255 },
    { 90, 24, 90, 255 },
    { 24, 90, 90, 255 }
};

static const GLfloat dirs[6][3] = {
    { 1.0f, 0.0f, 0.0f },
    { -1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, -1.0f }
};

static const int sample_x[6] = { 145, 215, 285, 355, 425, 495 };
static const int sample_y = 240;

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 10;
}

static bool pixel_matches(const uint8_t *actual, const uint8_t *expected)
{
    return near_byte(actual[0], expected[0]) &&
           near_byte(actual[1], expected[1]) &&
           near_byte(actual[2], expected[2]);
}

static void fill_face(int face)
{
    for (int i = 0; i < 4 * 4; ++i) {
        face_pixels[face][i * 4 + 0] = face_colors[face][0];
        face_pixels[face][i * 4 + 1] = face_colors[face][1];
        face_pixels[face][i * 4 + 2] = face_colors[face][2];
        face_pixels[face][i * 4 + 3] = face_colors[face][3];
    }
}

static void fill_pixels(uint8_t *pixels, int width, int height, const uint8_t color[4])
{
    for (int i = 0; i < width * height; ++i) {
        pixels[i * 4 + 0] = color[0];
        pixels[i * 4 + 1] = color[1];
        pixels[i * 4 + 2] = color[2];
        pixels[i * 4 + 3] = color[3];
    }
}

static void setup_cube(void)
{
    for (int i = 0; i < 6; ++i) {
        fill_face(i);
        fill_pixels(face_mip1[i], 2, 2, face_mip1_colors[i]);
        fill_pixels(face_mip2[i], 1, 1, face_mip2_colors[i]);
    }

    glGenTextures(1, &cube_tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(faces[i], 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_pixels[i]);
    }
}

static void upload_cube_mip_chain(void)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 2);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 1000.0f);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 0.0f);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(faces[i], 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_pixels[i]);
        glTexImage2D(faces[i], 1, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_mip1[i]);
        glTexImage2D(faces[i], 2, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, face_mip2[i]);
    }
}

static void draw_cube_sample_quad(int face, float x)
{
    const GLfloat *d = dirs[face];

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord3f(d[0], d[1], d[2]);
    glVertex3f(x - 0.26f, 0.28f, 0.0f);
    glTexCoord3f(d[0], d[1], d[2]);
    glVertex3f(x + 0.26f, 0.28f, 0.0f);
    glTexCoord3f(d[0], d[1], d[2]);
    glVertex3f(x + 0.26f, -0.28f, 0.0f);
    glTexCoord3f(d[0], d[1], d[2]);
    glVertex3f(x - 0.26f, -0.28f, 0.0f);
    glEnd();
}

static void render_cube_samples(bool cube_enabled)
{
    glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    nxglSetCamera(0.0f, 0.0f, -5.4f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cube_tex);
    if (cube_enabled) {
        glEnable(GL_TEXTURE_CUBE_MAP);
    } else {
        glDisable(GL_TEXTURE_CUBE_MAP);
    }

    for (int i = 0; i < 6; ++i) {
        draw_cube_sample_quad(i, -1.75f + (float)i * 0.70f);
    }
}

static void run_probe(void)
{
    GLint value = 0;
    bool ok = true;
    uint8_t pixel[4];

    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &value);
    expect_bool("cube binding before render", value == (GLint)cube_tex && consume_error(GL_NO_ERROR), 0);

    glEnable(GL_TEXTURE_CUBE_MAP);
    expect_bool("cube enabled before render", glIsEnabled(GL_TEXTURE_CUBE_MAP) == GL_TRUE && consume_error(GL_NO_ERROR), 1);

    glTexCoord3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_POINTS);
    glVertex3f(-2.9f, -2.0f, 0.0f);
    glEnd();
    expect_bool("texcoord3 accepts r", consume_error(GL_NO_ERROR), 2);

    glMultiTexCoord3f(GL_TEXTURE0, 1.0f, 0.0f, 0.0f);
    expect_bool("multitexcoord3 unit0", consume_error(GL_NO_ERROR), 3);

    glMultiTexCoord3f(GL_TEXTURE3 + 1, 1.0f, 0.0f, 0.0f);
    expect_bool("multitexcoord3 rejects unit", consume_error(GL_INVALID_ENUM), 4);

    render_cube_samples(true);
    for (int i = 0; i < 6; ++i) {
        memset(pixel, 0, sizeof(pixel));
        glReadPixels(sample_x[i], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        ok = ok && pixel_matches(pixel, face_colors[i]) && consume_error(GL_NO_ERROR);
    }
    expect_bool("cube render readback faces", ok, 5);

    glDisable(GL_TEXTURE_CUBE_MAP);
    render_cube_samples(false);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[4], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("cube disable stops sampling", pixel_matches(pixel, (uint8_t[4]){ 255, 255, 255, 255 }) && consume_error(GL_NO_ERROR), 6);

    glEnable(GL_TEXTURE_CUBE_MAP);
    render_cube_samples(true);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[0], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("cube re-enable restores sampling", pixel_matches(pixel, face_colors[0]) && consume_error(GL_NO_ERROR), 7);

    upload_cube_mip_chain();
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 1.0f);
    render_cube_samples(true);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[4], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("cube lod bias selects mip", ok && pixel_matches(pixel, face_mip1_colors[4]) && consume_error(GL_NO_ERROR), 8);

    upload_cube_mip_chain();
    ok = consume_error(GL_NO_ERROR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 2);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 2);
    render_cube_samples(true);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[0], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("cube base/max level clamp", ok && pixel_matches(pixel, face_mip2_colors[0]) && consume_error(GL_NO_ERROR), 9);

    upload_cube_mip_chain();
    ok = consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 2.0f);
    render_cube_samples(true);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[5], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    ok = ok && pixel_matches(pixel, face_mip2_colors[5]) && consume_error(GL_NO_ERROR);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, -1000.0f);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 1.0f);
    glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_LOD_BIAS, 2.0f);
    render_cube_samples(true);
    memset(pixel, 0, sizeof(pixel));
    glReadPixels(sample_x[5], sample_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    expect_bool("cube min/max lod clamp", ok && pixel_matches(pixel, face_mip1_colors[5]) && consume_error(GL_NO_ERROR), 10);
}

static void draw_result_bar(float x, bool pass)
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_CUBE_MAP);
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
    for (int i = 0; i < 11; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL cube-map render probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    setup_cube();
    run_probe();

    for (;;) {
        render_cube_samples(true);
        for (int i = 0; i < 11; ++i) {
            draw_result_bar(-2.50f + (float)i * 0.50f, results[i]);
        }

        nxglSwapBuffers("NXGL cube render", all_passed() ? "cube render checks passed" : "one or more cube render checks failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
