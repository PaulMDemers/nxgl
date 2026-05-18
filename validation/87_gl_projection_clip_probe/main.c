#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool results[8];

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
}

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 14;
}

static bool near_float(GLfloat a, GLfloat b, GLfloat eps)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= eps;
}

static bool pixel_rgb(const uint8_t p[4], uint8_t r, uint8_t g, uint8_t b)
{
    return near_byte(p[0], r) && near_byte(p[1], g) && near_byte(p[2], b);
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
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    glViewport(0, 0, 640, 480);
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

static void set_ortho_projection(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void set_frustum_projection(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void read_color(GLint x, GLint y, uint8_t pixel[4])
{
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
}

static void draw_rect(GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z)
{
    glBegin(GL_QUADS);
    glVertex3f(x0, y1, z);
    glVertex3f(x1, y1, z);
    glVertex3f(x1, y0, z);
    glVertex3f(x0, y0, z);
    glEnd();
}

static void draw_near_crossing_triangle(void)
{
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.55f, -0.35f, -2.0f);
    glVertex3f(0.55f, -0.35f, -2.0f);
    glVertex3f(0.0f, 0.55f, -0.5f);
    glEnd();
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat feedback[16];
    GLint hits;
    GLuint select[8];
    bool ok;

    reset_state();
    set_ortho_projection();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.85f, 0.12f, 0.10f);
    draw_rect(-2.5f, -0.25f, -1.50f, 0.25f, 0.0f);
    read_color(24, 240, pixel);
    ok = pixel_rgb(pixel, 217, 31, 25) && consume_error(GL_NO_ERROR);
    expect_bool("ortho left clipped visible", ok, 0);
    read_color(112, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("ortho left clipped bound", ok, 1);

    reset_state();
    set_ortho_projection();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.15f, 0.85f, 0.20f);
    draw_rect(2.10f, -0.25f, 2.50f, 0.25f, 0.0f);
    read_color(620, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("ortho fully clipped miss", ok, 2);

    reset_state();
    set_frustum_projection();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.12f, 0.55f, 0.90f);
    draw_near_crossing_triangle();
    read_color(320, 250, pixel);
    ok = pixel_rgb(pixel, 31, 140, 230) && consume_error(GL_NO_ERROR);
    expect_bool("frustum near crossing visible", ok, 3);

    reset_state();
    set_frustum_projection();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.90f, 0.50f, 0.12f);
    draw_rect(-0.25f, -0.25f, 0.25f, 0.25f, -0.5f);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("frustum near fully clipped", ok, 4);

    reset_state();
    set_ortho_projection();
    memset(feedback, 0, sizeof(feedback));
    glFeedbackBuffer(16, GL_3D, feedback);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_LINES);
    glVertex3f(-2.5f, 0.0f, 0.0f);
    glVertex3f(-1.5f, 0.0f, 0.0f);
    glEnd();
    ok = glRenderMode(GL_RENDER) == 7 &&
         near_float(feedback[0], (GLfloat)GL_LINE_TOKEN, 0.01f) &&
         near_float(feedback[1], 0.0f, 2.0f) &&
         near_float(feedback[2], 240.0f, 2.0f) &&
         near_float(feedback[4], 80.0f, 2.0f) &&
         near_float(feedback[5], 240.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback clipped line", ok, 5);

    reset_state();
    set_ortho_projection();
    memset(feedback, 0, sizeof(feedback));
    glFeedbackBuffer(16, GL_3D, feedback);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_QUADS);
    glVertex3f(-2.5f, 0.25f, 0.0f);
    glVertex3f(-1.5f, 0.25f, 0.0f);
    glVertex3f(-1.5f, -0.25f, 0.0f);
    glVertex3f(-2.5f, -0.25f, 0.0f);
    glEnd();
    ok = glRenderMode(GL_RENDER) > 0 &&
         near_float(feedback[0], (GLfloat)GL_POLYGON_TOKEN, 0.01f) &&
         feedback[1] >= 3.0f &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback clipped polygon", ok, 6);

    reset_state();
    set_ortho_projection();
    memset(select, 0, sizeof(select));
    glSelectBuffer(8, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(87);
    draw_rect(-2.5f, -0.25f, -1.5f, 0.25f, 0.0f);
    hits = glRenderMode(GL_RENDER);
    ok = hits == 1 && select[0] == 1 && select[3] == 87 && consume_error(GL_NO_ERROR);
    expect_bool("selection clipped polygon", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(pass ? 0.10f : 0.90f, pass ? 0.78f : 0.12f, 0.20f);
    glRectf(x - 0.17f, -0.72f, x + 0.17f, -0.88f);
}

static bool all_passed(void)
{
    for (int i = 0; i < 8; ++i) {
        if (!results[i]) return false;
    }
    return true;
}

int main(void)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("NXGL projection clip probe starting\n");

    if (nxglInit() != 0) {
        debugPrint("nxglInit failed\n");
        return 1;
    }

    memset(results, 0, sizeof(results));
    run_probe();

    for (;;) {
        reset_state();
        set_ortho_projection();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glColor3f(0.15f, 0.45f, 0.85f);
        draw_rect(-2.5f, -0.45f, -1.20f, 0.45f, 0.0f);
        for (int i = 0; i < 8; ++i) {
            draw_bar(-1.45f + (float)i * 0.42f, results[i]);
        }
        nxglSwapBuffers("NXGL projection clipping", all_passed() ? "all checks passed" : "projection clip check failed");
        Sleep(16);
    }
}
