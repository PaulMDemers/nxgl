#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static GLfloat cf(uint8_t v)
{
    return (GLfloat)v / 255.0f;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_float(GLfloat a, GLfloat b)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d < 0.0005f;
}

static int intensity(const uint8_t p[4])
{
    return ((int)p[0] + (int)p[1] + (int)p[2]) / 3;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
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
    glDisable(GL_COLOR_MATERIAL);
    glShadeModel(GL_SMOOTH);
    glFrontFace(GL_CCW);
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

static void read_pixel(int x, int y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_back_quad(float left, float top, float right, float bottom)
{
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(left, top, 0.0f);
    glVertex3f(right, top, 0.0f);
    glVertex3f(right, bottom, 0.0f);
    glVertex3f(left, bottom, 0.0f);
    glEnd();
}

static void draw_front_quad(float left, float top, float right, float bottom)
{
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(left, bottom, 0.0f);
    glVertex3f(right, bottom, 0.0f);
    glVertex3f(right, top, 0.0f);
    glVertex3f(left, top, 0.0f);
    glEnd();
}

static void setup_diffuse_light(GLfloat zdir)
{
    GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    GLfloat green[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    GLfloat light_pos[4] = { 0.0f, 0.0f, zdir, 0.0f };

    reset_state();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, red);
    glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    glMaterialfv(GL_BACK, GL_AMBIENT, zero);
    glMaterialfv(GL_BACK, GL_DIFFUSE, green);
    glMaterialfv(GL_BACK, GL_SPECULAR, zero);
    glMaterialfv(GL_BACK, GL_EMISSION, zero);
}

static void setup_specular_light(bool local_viewer)
{
    GLfloat zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_pos[4] = { 0.0f, 0.0f, 1.0f, 0.0f };

    reset_state();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, local_viewer ? 1 : 0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, zero);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 48.0f);
}

static void run_probe(void)
{
    GLfloat values[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GLint ivalue = 0;
    uint8_t center[4] = { 0, 0, 0, 0 };
    uint8_t side[4] = { 0, 0, 0, 0 };
    GLuint list;
    bool ok;

    setup_diffuse_light(-1.0f);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, values);
    ok = near_float(values[0], 1.0f) && near_float(values[1], 0.0f);
    glGetMaterialfv(GL_BACK, GL_DIFFUSE, values);
    expect_bool("front back material queries", ok && near_float(values[0], 0.0f) && near_float(values[1], 1.0f) && consume_error(GL_NO_ERROR), 0);

    setup_diffuse_light(-1.0f);
    glColorMaterial(GL_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(0.0f, 0.0f, 1.0f);
    glGetMaterialfv(GL_BACK, GL_DIFFUSE, values);
    ok = near_float(values[2], 1.0f);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, values);
    expect_bool("back color material target", ok && near_float(values[0], 1.0f) && near_float(values[2], 0.0f) && consume_error(GL_NO_ERROR), 1);

    setup_diffuse_light(-1.0f);
    clear_rgb(0, 0, 0);
    draw_back_quad(-0.35f, 0.35f, 0.35f, -0.35f);
    read_pixel(320, 240, center);
    expect_bool("one-sided back face remains dark", intensity(center) < 25 && consume_error(GL_NO_ERROR), 2);

    setup_diffuse_light(-1.0f);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    clear_rgb(0, 0, 0);
    draw_back_quad(-0.35f, 0.35f, 0.35f, -0.35f);
    read_pixel(320, 240, center);
    expect_bool("two-sided back face uses back material", center[1] > 180 && center[0] < 60 && center[2] < 60 && consume_error(GL_NO_ERROR), 3);

    setup_diffuse_light(1.0f);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    clear_rgb(0, 0, 0);
    draw_front_quad(-0.35f, 0.35f, 0.35f, -0.35f);
    read_pixel(320, 240, center);
    expect_bool("two-sided front face uses front material", center[0] > 180 && center[1] < 60 && center[2] < 60 && consume_error(GL_NO_ERROR), 4);

    setup_specular_light(false);
    clear_rgb(0, 0, 0);
    draw_front_quad(1.00f, 0.20f, 1.40f, -0.20f);
    read_pixel(394, 240, center);
    setup_specular_light(true);
    clear_rgb(0, 0, 0);
    draw_front_quad(1.00f, 0.20f, 1.40f, -0.20f);
    read_pixel(394, 240, side);
    expect_bool("local viewer changes specular", intensity(center) > 150 && intensity(side) < intensity(center) - 80 && consume_error(GL_NO_ERROR), 5);

    setup_specular_light(true);
    clear_rgb(0, 0, 0);
    draw_front_quad(-0.20f, 0.20f, 0.20f, -0.20f);
    read_pixel(320, 240, center);
    expect_bool("local viewer center specular remains bright", intensity(center) > 140 && consume_error(GL_NO_ERROR), 6);

    setup_diffuse_light(-1.0f);
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glEndList();
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);
    glCallList(list);
    glDeleteLists(list, 1);
    glGetIntegerv(GL_LIGHT_MODEL_TWO_SIDE, &ivalue);
    ok = ivalue == GL_TRUE && consume_error(GL_NO_ERROR);
    glGetIntegerv(GL_LIGHT_MODEL_LOCAL_VIEWER, &ivalue);
    expect_bool("display-list light model flags", ok && ivalue == GL_TRUE && consume_error(GL_NO_ERROR), 7);

    setup_diffuse_light(-1.0f);
    glBegin(GL_TRIANGLES);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    ok = consume_error(GL_INVALID_OPERATION);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, 1.0f);
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glVertex3f(-0.1f, -0.1f, 0.0f);
    glVertex3f(0.1f, -0.1f, 0.0f);
    glVertex3f(0.0f, 0.1f, 0.0f);
    glEnd();
    expect_bool("light model begin guards", ok && consume_error(GL_NO_ERROR), 8);

    setup_diffuse_light(-1.0f);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);
    clear_rgb(0, 0, 0);
    draw_back_quad(-0.35f, 0.35f, 0.35f, -0.35f);
    read_pixel(320, 240, center);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    clear_rgb(0, 0, 0);
    draw_back_quad(-0.35f, 0.35f, 0.35f, -0.35f);
    read_pixel(320, 240, side);
    expect_bool("two-sided flag toggles render", center[1] > side[1] + 120 && intensity(side) < 25 && consume_error(GL_NO_ERROR), 9);
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
    debugPrint("NXGL two-sided lighting probe starting\n");

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
        draw_front_quad(-1.25f, 0.75f, 1.25f, -0.75f);
        for (int i = 0; i < 10; ++i) {
            glColor3f(results[i] ? 0.1f : 0.9f, results[i] ? 0.8f : 0.1f, 0.15f);
            draw_front_quad(-2.75f + (float)i * 0.58f - 0.16f, -1.28f,
                            -2.75f + (float)i * 0.58f + 0.16f, -1.53f);
        }

        nxglSwapBuffers("NXGL two-sided lighting", all_passed() ? "all checks passed" : "two-sided light check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
