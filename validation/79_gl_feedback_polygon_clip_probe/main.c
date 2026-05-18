#include "nxgl.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

#include <stdbool.h>
#include <string.h>

static bool results[8];

static bool nearf(GLfloat a, GLfloat b, GLfloat tolerance)
{
    GLfloat d = a - b;
    if (d < 0.0f) d = -d;
    return d <= tolerance;
}

static bool consume_error(GLenum expected)
{
    return glGetError() == expected;
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
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthRange(0.0f, 1.0f);
    glViewport(0, 0, 640, 480);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    nxglSetCamera(0.0f, 0.0f, -5.2f, 0.0f, 0.0f, 0.0f);
}

static void draw_quad(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat x1, GLfloat y1, GLfloat z1)
{
    glBegin(GL_QUADS);
    glVertex3f(x0, y1, z0);
    glVertex3f(x1, y1, z1);
    glVertex3f(x1, y0, z1);
    glVertex3f(x0, y0, z0);
    glEnd();
}

static GLint feedback_quad(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat x1, GLfloat y1, GLfloat z1, GLfloat *buffer)
{
    memset(buffer, 0, sizeof(GLfloat) * 96u);
    glFeedbackBuffer(96, GL_3D, buffer);
    glRenderMode(GL_FEEDBACK);
    draw_quad(x0, y0, z0, x1, y1, z1);
    return glRenderMode(GL_RENDER);
}

static GLint select_quad(GLfloat x0, GLfloat y0, GLfloat z0, GLfloat x1, GLfloat y1, GLfloat z1, GLuint *buffer)
{
    memset(buffer, 0, sizeof(GLuint) * 32u);
    glSelectBuffer(32, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(79);
    draw_quad(x0, y0, z0, x1, y1, z1);
    return glRenderMode(GL_RENDER);
}

static bool polygon_feedback_has_vertices(const GLfloat *buffer, GLint total_count, GLint vertex_count)
{
    if (total_count != 2 + vertex_count * 3) {
        return false;
    }
    if (!nearf(buffer[0], (GLfloat)GL_POLYGON_TOKEN, 0.01f) || !nearf(buffer[1], (GLfloat)vertex_count, 0.01f)) {
        return false;
    }
    for (int i = 0; i < vertex_count; ++i) {
        GLfloat x = buffer[2 + i * 3];
        GLfloat y = buffer[3 + i * 3];
        GLfloat z = buffer[4 + i * 3];
        if (x < -1.0f || x > 641.0f || y < -1.0f || y > 481.0f || z < -0.01f || z > 1.01f) {
            return false;
        }
    }
    return true;
}

static void run_probe(void)
{
    GLfloat fb[96];
    GLuint select[32];
    GLdouble clip_x_positive[4] = { 1.0, 0.0, 0.0, 0.0 };
    GLint count;
    bool ok;

    reset_state();
    count = feedback_quad(-20.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, fb);
    ok = polygon_feedback_has_vertices(fb, count, 4) && consume_error(GL_NO_ERROR);
    expect_bool("feedback polygon left clipped", ok, 0);

    reset_state();
    count = feedback_quad(-0.5f, -0.5f, 5.5f, 0.5f, 0.5f, 0.0f, fb);
    ok = polygon_feedback_has_vertices(fb, count, 4) && fb[4] <= 0.01f && consume_error(GL_NO_ERROR);
    expect_bool("feedback polygon near clipped", ok, 1);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    count = feedback_quad(-1.0f, -0.5f, 0.0f, 1.0f, 0.5f, 0.0f, fb);
    ok = polygon_feedback_has_vertices(fb, count, 4) && fb[2] >= 319.0f && consume_error(GL_NO_ERROR);
    expect_bool("feedback polygon user clipped", ok, 2);

    reset_state();
    count = feedback_quad(-20.0f, -0.5f, 5.5f, -10.0f, 0.5f, 5.6f, fb);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("feedback fully clipped polygon", ok, 3);

    reset_state();
    count = select_quad(-20.0f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, select);
    ok = count == 1 && select[0] == 1 && select[3] == 79 && consume_error(GL_NO_ERROR);
    expect_bool("selection left-clipped polygon hit", ok, 4);

    reset_state();
    count = select_quad(-0.5f, -0.5f, 5.5f, 0.5f, 0.5f, 0.0f, select);
    ok = count == 1 && select[0] == 1 && select[3] == 79 && select[1] <= select[2] && consume_error(GL_NO_ERROR);
    expect_bool("selection near-clipped polygon hit", ok, 5);

    reset_state();
    glClipPlane(GL_CLIP_PLANE0, clip_x_positive);
    glEnable(GL_CLIP_PLANE0);
    count = select_quad(-2.0f, -0.5f, 0.0f, -1.0f, 0.5f, 0.0f, select);
    ok = count == 0 && consume_error(GL_NO_ERROR);
    expect_bool("selection user-clipped polygon miss", ok, 6);

    reset_state();
    count = feedback_quad(-0.5f, -20.0f, 0.0f, 0.5f, 0.5f, 0.0f, fb);
    ok = polygon_feedback_has_vertices(fb, count, 4) && consume_error(GL_NO_ERROR);
    expect_bool("feedback polygon bottom clipped", ok, 7);
}

static void draw_bar(float x, bool pass)
{
    glColor3f(pass ? 0.12f : 0.90f, pass ? 0.80f : 0.12f, 0.18f);
    glRectf(x - 0.17f, -1.42f, x + 0.17f, -1.68f);
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
    debugPrint("NXGL feedback polygon clip probe starting\n");

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
        for (int i = 0; i < 8; ++i) {
            draw_bar(-2.1f + (float)i * 0.6f, results[i]);
        }
        nxglSwapBuffers("NXGL polygon clipping", all_passed() ? "all checks passed" : "polygon clip check failed");
        Sleep(16);
    }
}
