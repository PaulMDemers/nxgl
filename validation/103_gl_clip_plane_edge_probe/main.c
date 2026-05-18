#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[10];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t actual, uint8_t expected)
{
    int d = (int)actual - (int)expected;
    if (d < 0) d = -d;
    return d <= 16;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
}

static bool neard(GLdouble actual, GLdouble expected)
{
    GLdouble d = actual - expected;
    if (d < 0.0) d = -d;
    return d <= 0.02;
}

static void expect_bool(const char *name, bool condition, int slot)
{
    debugPrint("%s: %s\n", condition ? "PASS" : "FAIL", name);
    results[slot] = condition;
}

static void reset_state(void)
{
    for (int i = 0; i < 6; ++i) {
        glDisable(GL_CLIP_PLANE0 + i);
    }
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_CUBE_MAP);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glPointSize(13.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    (void)glGetError();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static GLint feedback_point_count(GLfloat x)
{
    GLfloat buffer[32];
    memset(buffer, 0, sizeof(buffer));
    glFeedbackBuffer(32, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex3f(x, 0.0f, 0.0f);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static GLint select_point_count(GLfloat x)
{
    GLuint buffer[16];
    memset(buffer, 0, sizeof(buffer));
    glSelectBuffer(16, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(3);
    glBegin(GL_POINTS);
    glVertex3f(x, 0.0f, 0.0f);
    glEnd();
    return glRenderMode(GL_RENDER);
}

static void enable_box_planes(void)
{
    static const GLdouble planes[6][4] = {
        {  1.0,  0.0,  0.0, 1.0 },
        { -1.0,  0.0,  0.0, 1.0 },
        {  0.0,  1.0,  0.0, 1.0 },
        {  0.0, -1.0,  0.0, 1.0 },
        {  0.0,  0.0,  1.0, 6.0 },
        {  0.0,  0.0, -1.0, 6.0 }
    };

    for (int i = 0; i < 6; ++i) {
        glClipPlane(GL_CLIP_PLANE0 + i, planes[i]);
        glEnable(GL_CLIP_PLANE0 + i);
    }
}

static void draw_point(GLfloat x, GLfloat y, GLfloat z)
{
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
}

static void draw_quad(GLfloat x0, GLfloat x1)
{
    glBegin(GL_QUADS);
    glVertex3f(x0, 0.45f, 0.0f);
    glVertex3f(x1, 0.45f, 0.0f);
    glVertex3f(x1, -0.45f, 0.0f);
    glVertex3f(x0, -0.45f, 0.0f);
    glEnd();
}

static void run_probe(void)
{
    const GLdouble x_positive[4] = { 1.0, 0.0, 0.0, 0.0 };
    const GLdouble shifted[4] = { 1.0, 0.0, 0.0, -0.25 };
    GLdouble plane[4];
    GLboolean enabled[6];
    uint8_t pixel[4];
    GLuint list;
    bool ok;

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnable(GL_CLIP_PLANE0);
    ok = feedback_point_count(0.0f) == 4 &&
         feedback_point_count(-0.02f) == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback boundary inclusion", ok, 0);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnable(GL_CLIP_PLANE0);
    ok = select_point_count(0.0f) == 1 &&
         select_point_count(-0.02f) == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("selection boundary inclusion", ok, 1);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnable(GL_CLIP_PLANE0);
    glColor3f(0.9f, 0.8f, 0.1f);
    draw_point(0.0f, 0.0f, 0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 230, 204, 25) && consume_error(GL_NO_ERROR);
    expect_bool("render boundary point", ok, 2);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnable(GL_CLIP_PLANE0);
    glColor3f(0.9f, 0.1f, 0.1f);
    draw_quad(-0.60f, 0.60f);
    read_color(300, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0);
    read_color(340, 240, pixel);
    ok = ok && pixel_rgb(pixel, 230, 25, 25) && consume_error(GL_NO_ERROR);
    expect_bool("render clipped polygon edge", ok, 3);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    enable_box_planes();
    glColor3f(0.1f, 0.8f, 0.9f);
    draw_point(0.0f, 0.0f, 0.0f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 25, 204, 230) && consume_error(GL_NO_ERROR);
    expect_bool("all six planes visible", ok, 4);

    reset_state();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    enable_box_planes();
    glColor3f(0.7f, 0.2f, 0.9f);
    draw_point(1.25f, 0.0f, 0.0f);
    read_color(397, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("all six planes reject", ok, 5);

    reset_state();
    glTranslatef(0.25f, 0.0f, 0.0f);
    glClipPlane(GL_CLIP_PLANE0, shifted);
    glGetClipPlane(GL_CLIP_PLANE0, plane);
    ok = neard(plane[0], 1.0) && neard(plane[3], -0.50) && consume_error(GL_NO_ERROR);
    expect_bool("modelview transformed edge", ok, 6);

    reset_state();
    enable_box_planes();
    for (int i = 0; i < 6; ++i) {
        glGetBooleanv(GL_CLIP_PLANE0 + i, &enabled[i]);
    }
    ok = enabled[0] == GL_TRUE && enabled[1] == GL_TRUE && enabled[2] == GL_TRUE &&
         enabled[3] == GL_TRUE && enabled[4] == GL_TRUE && enabled[5] == GL_TRUE &&
         consume_error(GL_NO_ERROR);
    expect_bool("all plane enable queries", ok, 7);

    reset_state();
    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnable(GL_CLIP_PLANE0);
    glEndList();
    glCallList(list);
    ok = feedback_point_count(0.0f) == 4 && feedback_point_count(-0.02f) == 0 &&
         consume_error(GL_NO_ERROR);
    expect_bool("listed boundary replay", ok, 8);

    reset_state();
    glBegin(GL_POINTS);
    glClipPlane(GL_CLIP_PLANE0, x_positive);
    glEnd();
    ok = consume_error(GL_INVALID_OPERATION);
    glBegin(GL_POINTS);
    glGetClipPlane(GL_CLIP_PLANE0, plane);
    glEnd();
    ok = ok && consume_error(GL_INVALID_OPERATION);
    glClipPlane(GL_CLIP_PLANE0 + 6, x_positive);
    ok = ok && consume_error(GL_INVALID_ENUM);
    expect_bool("clip edge validation", ok, 9);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.88f, pass ? 0.78f : 0.10f, 0.18f);
    glBegin(GL_QUADS);
    glVertex3f(x - 0.13f, -0.75f, 0.0f);
    glVertex3f(x + 0.13f, -0.75f, 0.0f);
    glVertex3f(x + 0.13f, -0.98f, 0.0f);
    glVertex3f(x - 0.13f, -0.98f, 0.0f);
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
    debugPrint("NXGL clip plane edge probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(0.14f, 0.42f, 0.82f);
        glRectf(-1.45f, 0.42f, 1.45f, -0.42f);
        for (int i = 0; i < 10; ++i) {
            draw_bar(-1.35f + (float)i * 0.30f, results[i]);
        }
        nxglSwapBuffers("NXGL clip plane edges", all_passed() ? "all checks passed" : "clip edge check failed");
        Sleep(16);
    }

    nxglShutdown();
    return 0;
}
