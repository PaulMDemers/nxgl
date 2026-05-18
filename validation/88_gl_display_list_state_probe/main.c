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

static bool near_float(GLfloat a, GLfloat b, GLfloat eps)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= eps;
}

static bool near_double(GLdouble a, GLdouble b, GLdouble eps)
{
    GLdouble d = a - b;
    if (d < 0.0) d = -d;
    return d <= eps;
}

static bool near_byte(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;
    if (d < 0) d = -d;
    return d <= 14;
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
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CLIP_PLANE0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xff);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glScissor(0, 0, 640, 480);
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

static GLuint make_matrix_list(void)
{
    static const GLfloat m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.35f, -0.25f, 0.0f, 1.0f
    };
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m);
    glEndList();
    return list;
}

static GLuint make_projection_raster_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRasterPos4f(0.0f, 0.0f, 0.0f, 1.0f);
    glEndList();
    return list;
}

static GLuint make_clear_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEnable(GL_SCISSOR_TEST);
    glScissor(300, 220, 80, 80);
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_SCISSOR_TEST);
    glEndList();
    return list;
}

static GLuint make_depth_range_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glDepthRange(0.25f, 0.75f);
    glEndList();
    return list;
}

static GLuint make_clip_plane_list(void)
{
    GLdouble equation[4] = { 1.0, 0.0, 0.0, 0.0 };
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glClipPlane(GL_CLIP_PLANE0, equation);
    glEnable(GL_CLIP_PLANE0);
    glEndList();
    return list;
}

static GLuint make_projected_quad_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.85f, 0.12f, 0.10f);
    draw_rect(-2.5f, -0.25f, -1.5f, 0.25f, 0.0f);
    glEndList();
    return list;
}

static GLuint make_feedback_line_list(void)
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-2.0, 2.0, -1.5, 1.5, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPassThrough(88.0f);
    glBegin(GL_LINES);
    glVertex3f(-2.5f, 0.0f, 0.0f);
    glVertex3f(-1.5f, 0.0f, 0.0f);
    glEnd();
    glEndList();
    return list;
}

static void run_probe(void)
{
    uint8_t pixel[4];
    GLfloat matrix[16];
    GLfloat raster[4];
    GLfloat range[2];
    GLfloat feedback[16];
    GLdouble equation[4];
    GLuint select[8];
    GLuint list;
    GLint hits;
    bool ok;

    reset_state();
    list = make_matrix_list();
    glCallList(list);
    glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    ok = near_float(matrix[12], 0.35f, 0.001f) && near_float(matrix[13], -0.25f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("listed load matrix", ok, 0);
    glDeleteLists(list, 1);

    reset_state();
    list = make_projection_raster_list();
    glCallList(list);
    glGetFloatv(GL_PROJECTION_MATRIX, matrix);
    glGetFloatv(GL_CURRENT_RASTER_POSITION, raster);
    ok = near_float(matrix[0], 0.5f, 0.001f) &&
         near_float(matrix[5], 0.6666667f, 0.001f) &&
         near_float(raster[0], 320.0f, 2.0f) &&
         near_float(raster[1], 240.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("listed projection raster", ok, 1);
    glDeleteLists(list, 1);

    reset_state();
    glClearColor(0.10f, 0.20f, 0.30f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    list = make_clear_list();
    glCallList(list);
    read_color(320, 240, pixel);
    ok = pixel_rgb(pixel, 255, 51, 76);
    read_color(100, 100, pixel);
    ok = ok && pixel_rgb(pixel, 25, 51, 76) && consume_error(GL_NO_ERROR);
    expect_bool("listed clear scissor mask", ok, 2);
    glDeleteLists(list, 1);

    reset_state();
    list = make_depth_range_list();
    glCallList(list);
    glGetFloatv(GL_DEPTH_RANGE, range);
    ok = near_float(range[0], 0.25f, 0.001f) && near_float(range[1], 0.75f, 0.001f) && consume_error(GL_NO_ERROR);
    expect_bool("listed depth range", ok, 3);
    glDeleteLists(list, 1);

    reset_state();
    set_ortho_projection();
    list = make_clip_plane_list();
    glCallList(list);
    glGetClipPlane(GL_CLIP_PLANE0, equation);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.10f, 0.80f, 0.20f);
    draw_rect(-0.80f, -0.25f, -0.20f, 0.25f, 0.0f);
    read_color(224, 240, pixel);
    ok = glIsEnabled(GL_CLIP_PLANE0) &&
         near_double(equation[0], 1.0, 0.001) &&
         pixel_rgb(pixel, 0, 0, 0) &&
         consume_error(GL_NO_ERROR);
    expect_bool("listed clip plane", ok, 4);
    glDeleteLists(list, 1);

    reset_state();
    memset(select, 0, sizeof(select));
    list = make_projected_quad_list();
    glSelectBuffer(8, select);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(88);
    glCallList(list);
    hits = glRenderMode(GL_RENDER);
    ok = hits == 1 && select[0] == 1 && select[3] == 88 && consume_error(GL_NO_ERROR);
    expect_bool("listed projected selection", ok, 5);
    glDeleteLists(list, 1);

    reset_state();
    list = make_projected_quad_list();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCallList(list);
    read_color(24, 240, pixel);
    ok = pixel_rgb(pixel, 217, 31, 25) && consume_error(GL_NO_ERROR);
    expect_bool("listed projected render", ok, 6);
    glDeleteLists(list, 1);

    reset_state();
    memset(feedback, 0, sizeof(feedback));
    list = make_feedback_line_list();
    glFeedbackBuffer(16, GL_3D, feedback);
    glRenderMode(GL_FEEDBACK);
    glCallList(list);
    ok = glRenderMode(GL_RENDER) == 9 &&
         near_float(feedback[0], (GLfloat)GL_PASS_THROUGH_TOKEN, 0.01f) &&
         near_float(feedback[1], 88.0f, 0.01f) &&
         near_float(feedback[2], (GLfloat)GL_LINE_TOKEN, 0.01f) &&
         near_float(feedback[3], 0.0f, 2.0f) &&
         near_float(feedback[6], 80.0f, 2.0f) &&
         consume_error(GL_NO_ERROR);
    expect_bool("listed feedback line", ok, 7);
    glDeleteLists(list, 1);
}

static void draw_bar(float x, bool pass)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
    debugPrint("NXGL display-list state probe starting\n");

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
        nxglSwapBuffers("NXGL display-list state", all_passed() ? "all checks passed" : "display-list state check failed");
        Sleep(16);
    }
}
