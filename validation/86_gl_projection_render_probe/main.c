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
    return d <= 12;
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

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat raster[4];
    GLfloat feedback[8];
    GLint hits;
    GLuint select[8];
    bool ok;

    reset_state();
    set_ortho_projection();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.1f, 0.85f, 0.2f);
    draw_rect(0.75f, -0.25f, 1.25f, 0.25f, 0.0f);
    read_color(480, 240, pixel);
    ok = pixel_rgb(pixel, 25, 217, 51) && consume_error(GL_NO_ERROR);
    expect_bool("ortho projected render hit", ok, 0);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 0, 0, 0) && consume_error(GL_NO_ERROR);
    expect_bool("ortho projected render miss", ok, 1);

    glRasterPos3f(1.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = glIsEnabled(GL_LIGHTING) == GL_FALSE &&
         near_float(raster[0], 480.0f, 2.0f) &&
         near_float(raster[1], 240.0f, 2.0f) &&
         near_float(raster[2], 0.5f, 0.02f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("ortho raster position", ok, 2);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-4.0, 4.0, -3.0, 3.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glRasterPos3f(1.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = near_float(raster[0], 400.0f, 2.0f) && consume_error(GL_NO_ERROR);
    expect_bool("projection push changed raster", ok, 3);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glRasterPos3f(1.0f, 0.0f, 0.0f);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = near_float(raster[0], 480.0f, 2.0f) && consume_error(GL_NO_ERROR);
    expect_bool("projection pop restored raster", ok, 4);

    memset(feedback, 0, sizeof(feedback));
    glFeedbackBuffer(8, GL_3D, feedback);
    glRenderMode(GL_FEEDBACK);
    glBegin(GL_POINTS);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glEnd();
    ok = glRenderMode(GL_RENDER) == 4 &&
         near_float(feedback[0], (GLfloat)GL_POINT_TOKEN, 0.01f) &&
         near_float(feedback[1], 480.0f, 2.0f) &&
         near_float(feedback[2], 240.0f, 2.0f) &&
         near_float(feedback[3], 0.5f, 0.02f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("feedback projected point", ok, 5);

    memset(select, 0, sizeof(select));
    glSelectBuffer(8, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(86);
    glBegin(GL_POINTS);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glEnd();
    hits = glRenderMode(GL_RENDER);
    ok = hits == 1 && select[0] == 1 && select[3] == 86 && consume_error(GL_NO_ERROR);
    expect_bool("selection projected point", ok, 6);

    reset_state();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.85f, 0.25f, 0.1f);
    draw_rect(0.35f, -0.20f, 0.65f, 0.20f, -2.0f);
    read_color(400, 240, pixel);
    ok = pixel_rgb(pixel, 217, 64, 25) && consume_error(GL_NO_ERROR);
    expect_bool("frustum projected render", ok, 7);
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
    debugPrint("NXGL projection render probe starting\n");

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
        glColor3f(0.1f, 0.45f, 0.9f);
        draw_rect(-1.6f, -0.95f, 1.6f, 0.95f, 0.0f);
        for (int i = 0; i < 8; ++i) {
            draw_bar(-1.45f + (float)i * 0.42f, results[i]);
        }
        nxglSwapBuffers("NXGL projection render", all_passed() ? "all checks passed" : "projection check failed");
        Sleep(16);
    }
}
