#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[12];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat actual, GLfloat expected)
{
    GLfloat d = actual - expected;
    if (d < 0.0f) d = -d;
    return d < 0.0005f;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static GLfloat cf(uint8_t v)
{
    return (GLfloat)v / 255.0f;
}

static int intensity(const uint8_t p[4])
{
    return ((int)p[0] + (int)p[1] + (int)p[2]) / 3;
}

static void reset_state(void)
{
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_LINE_STIPPLE);
    glDisable(GL_POLYGON_STIPPLE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void clear_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    glClearColor(cf(r), cf(g), cf(b), 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void setup_white_light(void)
{
    GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat spot_dir[3] = { 0.0f, 0.0f, -1.0f };

    reset_state();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spot_dir);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 0.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
}

static void read_pixel(int x, int y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_quad(float left, float top, float right, float bottom, float z)
{
    glBegin(GL_QUADS);
    glVertex3f(left, top, z);
    glVertex3f(right, top, z);
    glVertex3f(right, bottom, z);
    glVertex3f(left, bottom, z);
    glEnd();
}

static void draw_unlit_result_bar(float x, bool pass)
{
    glDisable(GL_LIGHTING);
    glColor3f(pass ? 0.1f : 0.9f, pass ? 0.8f : 0.1f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.28f, 0.0f);
    glVertex3f(x + 0.16f, -1.53f, 0.0f);
    glVertex3f(x - 0.16f, -1.53f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    GLfloat values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLfloat position[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    GLfloat behind[4] = { 0.0f, 0.0f, -1.0f, 1.0f };
    GLfloat direction[3] = { 0.0f, 0.0f, -1.0f };
    GLfloat diagonal[3] = { 0.0f, -1.0f, -1.0f };
    GLint iv[4] = { 0, 0, 0, 0 };
    uint8_t center[4] = { 0, 0, 0, 0 };
    uint8_t side[4] = { 0, 0, 0, 0 };
    uint8_t dim[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    setup_white_light();
    glGetLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, values);
    ok = near_float(values[0], 0.0f) && near_float(values[1], 0.0f) && near_float(values[2], -1.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_EXPONENT, values);
    ok = ok && near_float(values[0], 0.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, values);
    ok = ok && near_float(values[0], 180.0f);
    glGetLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, values);
    ok = ok && near_float(values[0], 1.0f);
    glGetLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, values);
    ok = ok && near_float(values[0], 0.0f);
    glGetLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, values);
    expect_bool("default spot attenuation state", ok && near_float(values[0], 0.0f) && consume_error(GL_NO_ERROR), 0);

    setup_white_light();
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, diagonal);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 8.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 35.0f);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.25f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.125f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, values);
    ok = near_float(values[0], 0.0f) && near_float(values[1], -1.0f) && near_float(values[2], -1.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_EXPONENT, values);
    ok = ok && near_float(values[0], 8.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, values);
    ok = ok && near_float(values[0], 35.0f);
    glGetLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, values);
    ok = ok && near_float(values[0], 0.5f);
    glGetLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, values);
    ok = ok && near_float(values[0], 0.25f);
    glGetLightfv(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, values);
    expect_bool("spot attenuation setter query", ok && near_float(values[0], 0.125f) && consume_error(GL_NO_ERROR), 1);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    draw_quad(-0.20f, 0.20f, 0.20f, -0.20f, 0.0f);
    read_pixel(320, 240, center);
    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 4.0f);
    draw_quad(-0.20f, 0.20f, 0.20f, -0.20f, 0.0f);
    read_pixel(320, 240, dim);
    expect_bool("constant attenuation render", intensity(center) > 180 && intensity(dim) > 35 && intensity(dim) < intensity(center) - 90 && consume_error(GL_NO_ERROR), 2);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0f);
    draw_quad(-0.20f, 0.20f, 0.20f, -0.20f, 0.0f);
    draw_quad(1.30f, 0.20f, 1.70f, -0.20f, 0.0f);
    read_pixel(320, 240, center);
    read_pixel(412, 240, side);
    expect_bool("linear attenuation distance render", intensity(center) > intensity(side) + 45 && intensity(side) > 10 && consume_error(GL_NO_ERROR), 3);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 15.0f);
    draw_quad(-0.12f, 0.12f, 0.12f, -0.12f, 0.0f);
    draw_quad(1.25f, 0.12f, 1.49f, -0.12f, 0.0f);
    read_pixel(320, 240, center);
    read_pixel(404, 240, side);
    expect_bool("spot cutoff render", intensity(center) > 180 && intensity(side) < 20 && consume_error(GL_NO_ERROR), 4);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 60.0f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 16.0f);
    draw_quad(-0.12f, 0.12f, 0.12f, -0.12f, 0.0f);
    draw_quad(0.55f, 0.12f, 0.79f, -0.12f, 0.0f);
    read_pixel(320, 240, center);
    read_pixel(361, 240, side);
    expect_bool("spot exponent falloff render", intensity(center) > intensity(side) + 70 && intensity(side) > 10 && consume_error(GL_NO_ERROR), 5);

    setup_white_light();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, diagonal);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 5.0f);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 45.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.75f);
    glEndList();
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.0f);
    glCallList(list);
    glDeleteLists(list, 1);
    glGetLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, values);
    ok = near_float(values[1], -1.0f) && near_float(values[2], -1.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_EXPONENT, values);
    ok = ok && near_float(values[0], 5.0f);
    glGetLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, values);
    ok = ok && near_float(values[0], 45.0f);
    glGetLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, values);
    expect_bool("display-list light state", ok && near_float(values[0], 0.75f) && consume_error(GL_NO_ERROR), 6);

    setup_white_light();
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, -1.0f);
    ok = consume_error(GL_INVALID_VALUE);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 129.0f);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 91.0f);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, -0.1f);
    ok = ok && consume_error(GL_INVALID_VALUE);
    glLightf(GL_LIGHT0 - 1, GL_SPOT_CUTOFF, 45.0f);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glLightf(GL_LIGHT0, GL_TEXTURE_2D, 1.0f);
    ok = ok && consume_error(GL_INVALID_ENUM);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, NULL);
    expect_bool("spot attenuation validation", ok && consume_error(GL_INVALID_VALUE), 7);

    setup_white_light();
    glBegin(GL_TRIANGLES);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0f);
    ok = consume_error(GL_INVALID_OPERATION);
    glGetLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, values);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, -0.1f, 0.0f);
    glVertex3f(0.0f, 0.1f, 0.0f);
    glEnd();
    expect_bool("light begin guards", ok && consume_error(GL_NO_ERROR), 8);

    setup_white_light();
    iv[0] = 0;
    iv[1] = 0;
    iv[2] = -1;
    glLightiv(GL_LIGHT0, GL_SPOT_DIRECTION, iv);
    glLighti(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 2);
    glGetLightiv(GL_LIGHT0, GL_SPOT_DIRECTION, iv);
    ok = iv[0] == 0 && iv[1] == 0 && iv[2] == -1;
    glGetLightiv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, iv);
    expect_bool("integer light wrappers", ok && iv[0] == 2 && consume_error(GL_NO_ERROR), 9);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    draw_quad(-0.15f, 0.15f, 0.15f, -0.15f, 0.0f);
    read_pixel(320, 240, center);
    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, behind);
    draw_quad(-0.15f, 0.15f, 0.15f, -0.15f, 0.0f);
    read_pixel(320, 240, side);
    expect_bool("positional light direction render", intensity(center) > 180 && intensity(side) < 20 && consume_error(GL_NO_ERROR), 10);

    setup_white_light();
    clear_rgb(0, 0, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, direction);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
    draw_quad(1.25f, 0.12f, 1.49f, -0.12f, 0.0f);
    read_pixel(404, 240, side);
    expect_bool("spot cutoff 180 disables cone", intensity(side) > 60 && consume_error(GL_NO_ERROR), 11);
}

static bool all_passed(void)
{
    for (int i = 0; i < 12; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL light spot attenuation probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    run_probe();

    for (;;) {
        reset_state();
        clear_rgb(4, 4, 10);
        glDisable(GL_LIGHTING);
        glColor3f(all_passed() ? 0.10f : 0.65f, all_passed() ? 0.55f : 0.08f, all_passed() ? 0.85f : 0.08f);
        draw_quad(-1.25f, 0.75f, 1.25f, -0.75f, 0.0f);
        for (int i = 0; i < 12; ++i) {
            draw_unlit_result_bar(-2.75f + (float)i * 0.48f, results[i]);
        }

        nxglSwapBuffers("NXGL light spot attenuation", all_passed() ? "all checks passed" : "light spot check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
